#include "infrastructure/database/odbc_database_provider.h"

#include "domain/column_extract.h"
#include "domain/database/keyset_sql.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
#include <QSqlIndex>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>
#include <QVariant>

#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <sstream>

namespace datalab::infrastructure {
namespace {

QString make_connection_name()
{
    return QStringLiteral("datalab_odbc_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

std::string sanitize_error(const QSqlError& error)
{
    QString text = error.text();
    text.replace(QRegularExpression(QStringLiteral("PWD\\s*=\\s*[^;]+"),
                                    QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("PWD=***"));
    text.replace(QRegularExpression(QStringLiteral("password\\s*=\\s*[^;\\s]+"),
                                    QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("password=***"));
    return text.toStdString();
}

domain::ColumnType normalize_odbc_type(QMetaType::Type type)
{
    switch (type) {
    case QMetaType::Int:
    case QMetaType::LongLong:
    case QMetaType::UInt:
    case QMetaType::ULongLong:
    case QMetaType::Double:
    case QMetaType::Float:
        return domain::ColumnType::numeric;
    case QMetaType::QDate:
    case QMetaType::QTime:
    case QMetaType::QDateTime:
        return domain::ColumnType::time;
    case QMetaType::QString:
        return domain::ColumnType::categorical;
    default:
        return domain::ColumnType::unknown;
    }
}

class OdbcDatabaseProvider final : public datalab::domain::IDatabaseProvider {
public:
    ~OdbcDatabaseProvider() override { disconnect(); }

    domain::DatabaseProviderDescriptor descriptor() const override
    {
        return odbc_provider_descriptor();
    }

    domain::DatabaseResult<domain::ConnectionInfo> connect(
        const domain::ConnectionOptions& options) override
    {
        QString reason;
        if (!odbc_driver_available(&reason)) {
            return domain::DatabaseResult<domain::ConnectionInfo>::failure(
                "driver_missing", reason.toStdString());
        }
        if (options.dsn.empty()) {
            return domain::DatabaseResult<domain::ConnectionInfo>::failure(
                "missing_connection", "ODBC 需要 DSN。");
        }
        disconnect();
        connection_name_ = make_connection_name();
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QODBC"), connection_name_);
        if (!options.dsn.empty()) {
            // Never log full DSN; store only display name locally.
            db.setDatabaseName(QString::fromStdString(options.dsn));
            dsn_display_ = options.display_name.empty() ? "odbc-dsn" : options.display_name;
        } else {
            return domain::DatabaseResult<domain::ConnectionInfo>::failure(
                "missing_connection",
                "ODBC 当前仅支持 DSN 连接（不持久化完整连接串/密码）。");
        }
        db.setUserName(QString::fromStdString(options.username));
        db.setPassword(QString::fromStdString(options.password));
        if (!db.open()) {
            const std::string message = sanitize_error(db.lastError());
            disconnect();
            return domain::DatabaseResult<domain::ConnectionInfo>::failure(
                "connect_failed", message);
        }
        domain::ConnectionInfo info;
        info.connection_id = connection_name_.toStdString();
        info.provider_id = "odbc";
        info.driver_name = "QODBC";
        info.read_only = options.read_only;
        info.host = options.host;
        info.database = options.database.empty() ? dsn_display_ : options.database;
        info.username = options.username;
        info.server_version = "ODBC";
        connected_ = true;
        return domain::DatabaseResult<domain::ConnectionInfo>::success(std::move(info));
    }

    domain::DatabaseResult<std::vector<domain::DatabaseObject>> list_objects(
        const domain::ObjectFilter& filter) override
    {
        QSqlDatabase* db = database();
        if (db == nullptr) {
            return domain::DatabaseResult<std::vector<domain::DatabaseObject>>::failure(
                "not_connected", "尚未连接 ODBC。");
        }
        std::vector<domain::DatabaseObject> objects;
        const auto append_tables = [&](QSql::TableType type, domain::DatabaseObjectKind kind) {
            if ((kind == domain::DatabaseObjectKind::table && !filter.include_tables)
                || (kind == domain::DatabaseObjectKind::view && !filter.include_views)) {
                return;
            }
            const QStringList names = db->tables(type);
            for (const QString& name : names) {
                domain::DatabaseObject object;
                object.ref.name = name.toStdString();
                object.ref.kind = kind;
                if (!filter.name_contains.empty()
                    && !name.contains(QString::fromStdString(filter.name_contains),
                                      Qt::CaseInsensitive)) {
                    continue;
                }
                objects.push_back(std::move(object));
            }
        };
        append_tables(QSql::Tables, domain::DatabaseObjectKind::table);
        append_tables(QSql::Views, domain::DatabaseObjectKind::view);
        return domain::DatabaseResult<std::vector<domain::DatabaseObject>>::success(
            std::move(objects));
    }

    domain::DatabaseResult<domain::TableMetadata> describe(
        const domain::ObjectRef& object) override
    {
        QSqlDatabase* db = database();
        if (db == nullptr) {
            return domain::DatabaseResult<domain::TableMetadata>::failure(
                "not_connected", "尚未连接 ODBC。");
        }
        const QSqlRecord record = db->record(QString::fromStdString(object.name));
        if (record.isEmpty()) {
            return domain::DatabaseResult<domain::TableMetadata>::failure(
                "object_not_found",
                "对象不存在或驱动未返回列元数据：" + object.name);
        }
        domain::TableMetadata metadata;
        metadata.object.ref = object;
        metadata.object.selectable = true;
        for (int index = 0; index < record.count(); ++index) {
            domain::ColumnMetadata column;
            column.name = record.fieldName(index).toStdString();
            column.ordinal = index + 1;
            const QSqlField field = record.field(index);
            column.native_type = QString::fromUtf8(field.metaType().name()).toStdString();
            column.normalized_type =
                normalize_odbc_type(static_cast<QMetaType::Type>(field.metaType().id()));
            column.nullable = field.requiredStatus() != QSqlField::Required;
            if (db->driver() != nullptr) {
                column.quoted_identifier =
                    db->driver()
                        ->escapeIdentifier(record.fieldName(index), QSqlDriver::FieldName)
                        .toStdString();
            } else {
                column.quoted_identifier = "\"" + column.name + "\"";
            }
            column.selectable = true;
            metadata.columns.push_back(std::move(column));
        }
        const QSqlIndex primary = db->primaryIndex(QString::fromStdString(object.name));
        for (int index = 0; index < primary.count(); ++index) {
            const std::string pk = primary.fieldName(index).toStdString();
            metadata.primary_key_columns.push_back(pk);
            for (domain::ColumnMetadata& column : metadata.columns) {
                if (column.name == pk) {
                    column.primary_key = true;
                }
            }
        }
        metadata.supports_keyset_paging = !metadata.primary_key_columns.empty();
        metadata.estimated_rows_unknown = true;
        return domain::DatabaseResult<domain::TableMetadata>::success(std::move(metadata));
    }

    domain::DatabaseResult<domain::QueryPreview> preview(
        const domain::ImportPlan& plan,
        std::uint64_t preview_limit) override
    {
        domain::ImportPlan preview_plan = plan;
        if (preview_limit > 0) {
            preview_plan.row_limit = preview_limit;
        }
        auto imported = materialize(preview_plan, preview_limit, true, nullptr);
        if (!imported.ok) {
            return domain::DatabaseResult<domain::QueryPreview>::failure(
                imported.error_code, imported.error_message, imported.diagnostics);
        }
        domain::QueryPreview preview;
        auto described = describe(plan.object_ref);
        if (described.ok) {
            preview.metadata = std::move(described.value);
        }
        preview.sample = std::move(imported.value.table);
        preview.preview_row_count = preview.sample.rows.size();
        preview.truncated = plan.row_limit.has_value()
            ? false
            : (preview_limit > 0 && preview.sample.rows.size() >= preview_limit);
        preview.diagnostics = std::move(imported.value.diagnostics);
        preview.diagnostics.push_back({
            "odbc_driver_variance",
            "ODBC 元数据能力因驱动而异；行数估计通常不可用。",
            "info",
            false});
        return domain::DatabaseResult<domain::QueryPreview>::success(std::move(preview));
    }

    domain::DatabaseResult<domain::ImportedTable> import_table(
        const domain::ImportPlan& plan,
        domain::ImportCancellationToken* cancel = nullptr) override
    {
        return materialize(plan, 0, false, cancel);
    }

    void disconnect() override
    {
        if (!connection_name_.isEmpty()) {
            {
                QSqlDatabase db = QSqlDatabase::database(connection_name_);
                if (db.isOpen()) {
                    db.close();
                }
            }
            QSqlDatabase::removeDatabase(connection_name_);
            connection_name_.clear();
        }
        connected_ = false;
        dsn_display_.clear();
    }

private:
    domain::DatabaseResult<domain::ImportedTable> materialize(
        const domain::ImportPlan& plan,
        std::uint64_t preview_limit,
        bool is_preview,
        domain::ImportCancellationToken* cancel)
    {
        QSqlDatabase* db = database();
        if (db == nullptr) {
            return domain::DatabaseResult<domain::ImportedTable>::failure(
                "not_connected", "尚未连接 ODBC。");
        }
        auto described = describe(plan.object_ref);
        if (!described.ok) {
            return domain::DatabaseResult<domain::ImportedTable>::failure(
                described.error_code, described.error_message, described.diagnostics);
        }
        const domain::TableMetadata& metadata = described.value;
        const std::string plan_error = domain::validate_import_plan(plan, metadata);
        if (!plan_error.empty()) {
            return domain::DatabaseResult<domain::ImportedTable>::failure(
                "invalid_import_plan", plan_error);
        }

        const std::vector<std::string>& ordered_columns = plan.column_order.empty()
            ? plan.selected_columns
            : plan.column_order;
        std::map<std::string, domain::ColumnMetadata> by_name;
        for (const domain::ColumnMetadata& column : metadata.columns) {
            by_name[column.name] = column;
        }

        QString table_name = QString::fromStdString(plan.object_ref.name);
        if (db->driver() != nullptr) {
            table_name = db->driver()->escapeIdentifier(table_name, QSqlDriver::TableName);
        }
        QStringList select_parts;
        for (const std::string& name : ordered_columns) {
            select_parts.append(QString::fromStdString(by_name[name].quoted_identifier));
        }
        QString sql = QStringLiteral("SELECT %1 FROM %2")
            .arg(select_parts.join(QStringLiteral(", ")), table_name);

        QVariantList bound;
        QStringList where_parts;
        for (const domain::StructuredFilter& filter : plan.structured_filter) {
            const QString quoted =
                QString::fromStdString(by_name[filter.column].quoted_identifier);
            if (filter.op == "IS NULL" || filter.op == "IS NOT NULL") {
                where_parts.append(quoted + QLatin1Char(' ') + QString::fromStdString(filter.op));
            } else {
                where_parts.append(quoted + QLatin1Char(' ')
                    + QString::fromStdString(filter.op) + QStringLiteral(" ?"));
                bound.append(QString::fromStdString(filter.value));
            }
        }
        bool used_keyset = false;
        std::vector<std::string> order_columns;
        if (plan.keyset_after.has_value()) {
            std::map<std::string, std::string> quoted;
            for (const auto& [name, column] : by_name) {
                quoted[name] = column.quoted_identifier;
            }
            const domain::KeysetSqlFragment keyset =
                domain::build_keyset_sql_fragment(*plan.keyset_after, quoted);
            if (keyset.where_sql.empty()) {
                return domain::DatabaseResult<domain::ImportedTable>::failure(
                    "invalid_keyset", "无法构建 keyset 谓词。");
            }
            where_parts.append(QStringLiteral("(%1)").arg(QString::fromStdString(keyset.where_sql)));
            for (const std::string& value : keyset.bound_values) {
                bound.append(QString::fromStdString(value));
            }
            order_columns = keyset.order_columns;
            used_keyset = true;
        }
        if (!where_parts.isEmpty()) {
            sql += QStringLiteral(" WHERE ") + where_parts.join(QStringLiteral(" AND "));
        }
        if (!order_columns.empty()) {
            QStringList order_parts;
            for (const std::string& key : order_columns) {
                order_parts.append(QString::fromStdString(by_name[key].quoted_identifier));
            }
            sql += QStringLiteral(" ORDER BY ") + order_parts.join(QStringLiteral(", "));
        } else if (!plan.order_key.empty()) {
            sql += QStringLiteral(" ORDER BY ")
                + QString::fromStdString(by_name[plan.order_key].quoted_identifier);
        } else if (!metadata.primary_key_columns.empty()) {
            QStringList order_parts;
            for (const std::string& key : metadata.primary_key_columns) {
                order_parts.append(QString::fromStdString(by_name[key].quoted_identifier));
            }
            sql += QStringLiteral(" ORDER BY ") + order_parts.join(QStringLiteral(", "));
        }

        std::optional<std::uint64_t> effective_limit = plan.row_limit;
        if (is_preview && preview_limit > 0) {
            if (!effective_limit.has_value() || preview_limit < *effective_limit) {
                effective_limit = preview_limit;
            }
        }
        // ODBC TOP/LIMIT syntax varies; prefer LIMIT for drivers that accept it,
        // otherwise fall back to fetching and truncating in-process with diagnostic.
        if (effective_limit.has_value()) {
            sql += QStringLiteral(" LIMIT ?");
            bound.append(qulonglong(*effective_limit));
            if (!used_keyset && plan.row_offset.has_value() && *plan.row_offset > 0) {
                sql += QStringLiteral(" OFFSET ?");
                bound.append(qulonglong(*plan.row_offset));
            }
        }

        QSqlQuery query(*db);
        if (!query.prepare(sql)) {
            return domain::DatabaseResult<domain::ImportedTable>::failure(
                "prepare_failed",
                sanitize_error(query.lastError())
                    + "（部分 ODBC 驱动不支持 LIMIT/OFFSET；请改用支持的驱动或 keyset 小页。）");
        }
        for (const QVariant& value : bound) {
            query.addBindValue(value);
        }
        if (!query.exec()) {
            return domain::DatabaseResult<domain::ImportedTable>::failure(
                "query_failed",
                sanitize_error(query.lastError())
                    + "（部分 ODBC 驱动不支持 LIMIT/OFFSET；请改用支持的驱动或 keyset 小页。）");
        }

        domain::ImportedTable imported;
        imported.plan = plan;
        domain::DataTable& table = imported.table;
        table.source_path = "odbc://" + dsn_display_ + "/" + plan.object_ref.name;
        table.name = plan.target_worksheet_name.empty()
            ? plan.object_ref.name
            : plan.target_worksheet_name;
        for (const std::string& name : ordered_columns) {
            table.columns.push_back(name);
            table.column_types.push_back(by_name[name].normalized_type);
        }
        imported.row_id_is_synthetic = true;
        std::uint64_t synthetic = 0;
        const std::uint64_t soft_limit =
            effective_limit.value_or(std::numeric_limits<std::uint64_t>::max());
        while (query.next()) {
            if (cancel != nullptr && cancel->is_cancellation_requested()) {
                return domain::DatabaseResult<domain::ImportedTable>::failure(
                    "cancelled", "导入已取消，未写入工作表。");
            }
            if (synthetic >= soft_limit) {
                imported.diagnostics.push_back({
                    "odbc_client_truncated",
                    "已在客户端截断到 row_limit（驱动可能忽略 SQL LIMIT）。",
                    "warning",
                    false});
                break;
            }
            std::vector<std::string> row(ordered_columns.size());
            std::vector<domain::CellState> states(
                ordered_columns.size(), domain::CellState::valid);
            for (int column = 0; column < static_cast<int>(ordered_columns.size()); ++column) {
                const QVariant value = query.value(column);
                if (value.isNull()) {
                    states[static_cast<std::size_t>(column)] = domain::CellState::missing;
                    continue;
                }
                row[static_cast<std::size_t>(column)] = value.toString().toStdString();
            }
            table.rows.push_back(std::move(row));
            table.cell_states.push_back(std::move(states));
            table.row_ids.push_back(synthetic++);
        }
        if (used_keyset) {
            imported.diagnostics.push_back(
                {"keyset_paging", "使用 keyset 分页。", "info", false});
        }
        imported.diagnostics.push_back({
            "odbc_driver_variance",
            "ODBC 能力因驱动而异；请检查主键/LIMIT 支持。",
            "info",
            false});

        table.import_metadata.schema_version = 1;
        table.import_metadata.sheet_name = plan.object_ref.name;
        table.import_metadata.original_row_count = table.rows.size();
        table.import_metadata.column_count = table.columns.size();
        table.import_metadata.provider_id = "odbc";
        table.import_metadata.source_object = dsn_display_ + "." + plan.object_ref.name;
        table.import_metadata.object_kind = "table";
        table.import_metadata.filter_summary = domain::summarize_import_filters(plan);
        table.import_metadata.selected_columns = ordered_columns;
        table.import_metadata.row_id_is_synthetic = true;
        table.import_metadata.imported_at =
            QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
        table.import_metadata.dataset_id = std::to_string(
            std::hash<std::string>{}(table.source_path + std::to_string(table.rows.size())));
        const std::string contract = domain::validate_data_table_contract(table);
        if (!contract.empty()) {
            return domain::DatabaseResult<domain::ImportedTable>::failure(
                "contract_failed", contract, imported.diagnostics);
        }
        return domain::DatabaseResult<domain::ImportedTable>::success(std::move(imported));
    }

    QSqlDatabase* database()
    {
        if (!connected_ || connection_name_.isEmpty()) {
            return nullptr;
        }
        owned_ = QSqlDatabase::database(connection_name_);
        return (owned_.isValid() && owned_.isOpen()) ? &owned_ : nullptr;
    }

    QString connection_name_;
    QSqlDatabase owned_;
    bool connected_ = false;
    std::string dsn_display_;
};

}  // namespace

bool odbc_driver_available(QString* reason)
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QODBC"))) {
        if (reason != nullptr) {
            *reason = QStringLiteral(
                "未安装 QODBC 驱动；ODBC Provider 不可用（保持诚实 ❌）。");
        }
        return false;
    }
    return true;
}

domain::DatabaseProviderDescriptor odbc_provider_descriptor()
{
    domain::DatabaseProviderDescriptor descriptor;
    descriptor.id = "odbc";
    descriptor.display_name = "ODBC";
    descriptor.driver_names = {"QODBC"};
    QString reason;
    descriptor.available = odbc_driver_available(&reason);
    descriptor.unavailable_reason = reason.toStdString();
    descriptor.capabilities.connect = true;
    descriptor.capabilities.list_tables = true;
    descriptor.capabilities.list_views = true;
    descriptor.capabilities.describe_columns = true;
    descriptor.capabilities.preview_rows = true;
    descriptor.capabilities.import_rows = true;
    descriptor.capabilities.structured_filter = true;
    descriptor.capabilities.keyset_pagination = true;
    descriptor.capabilities.catalog_schema = false;
    descriptor.capabilities.read_only = true;
    return descriptor;
}

std::unique_ptr<datalab::domain::IDatabaseProvider> create_odbc_database_provider()
{
    return std::make_unique<OdbcDatabaseProvider>();
}

}  // namespace datalab::infrastructure
