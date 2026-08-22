#include "infrastructure/database/mysql_database_provider.h"

#include "domain/column_extract.h"
#include "domain/database/keyset_sql.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

#include <algorithm>
#include <functional>
#include <map>
#include <sstream>

namespace datalab::infrastructure {
namespace {

QString make_connection_name()
{
    return QStringLiteral("datalab_mysql_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

std::string sanitize_error(const QSqlError& error)
{
    QString text = error.text();
    text.replace(QRegularExpression(QStringLiteral("password\\s*=\\s*[^;\\s]+"),
                                    QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("password=***"));
    return text.toStdString();
}

domain::ColumnType normalize_mysql_type(const QString& native)
{
    const QString upper = native.toUpper();
    if (upper.contains(QStringLiteral("INT"))
        || upper.contains(QStringLiteral("DECIMAL"))
        || upper.contains(QStringLiteral("NUMERIC"))
        || upper.contains(QStringLiteral("FLOAT"))
        || upper.contains(QStringLiteral("DOUBLE"))
        || upper.contains(QStringLiteral("REAL"))
        || upper.contains(QStringLiteral("BIT"))) {
        return domain::ColumnType::numeric;
    }
    if (upper.contains(QStringLiteral("DATE"))
        || upper.contains(QStringLiteral("TIME"))
        || upper.contains(QStringLiteral("YEAR"))) {
        return domain::ColumnType::time;
    }
    if (upper.contains(QStringLiteral("CHAR"))
        || upper.contains(QStringLiteral("TEXT"))
        || upper.contains(QStringLiteral("JSON"))
        || upper.contains(QStringLiteral("ENUM"))
        || upper.contains(QStringLiteral("SET"))) {
        return domain::ColumnType::categorical;
    }
    return domain::ColumnType::unknown;
}

std::string quote_mysql_identifier(const std::string& identifier)
{
    std::string escaped;
    escaped.reserve(identifier.size() + 2);
    escaped.push_back('`');
    for (const char character : identifier) {
        if (character == '`') {
            escaped.push_back('`');
            escaped.push_back('`');
        } else {
            escaped.push_back(character);
        }
    }
    escaped.push_back('`');
    return escaped;
}

class MysqlDatabaseProvider final : public datalab::domain::IDatabaseProvider {
public:
    ~MysqlDatabaseProvider() override { disconnect(); }

    domain::DatabaseProviderDescriptor descriptor() const override
    {
        return mysql_provider_descriptor();
    }

    domain::DatabaseResult<domain::ConnectionInfo> connect(
        const domain::ConnectionOptions& options) override
    {
        QString reason;
        if (!mysql_driver_available(&reason)) {
            return domain::DatabaseResult<domain::ConnectionInfo>::failure(
                "driver_missing", reason.toStdString());
        }
        if (options.host.empty() || options.database.empty()) {
            return domain::DatabaseResult<domain::ConnectionInfo>::failure(
                "missing_connection", "MySQL 需要 host 与 database。");
        }
        disconnect();
        connection_name_ = make_connection_name();
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QMYSQL"), connection_name_);
        db.setHostName(QString::fromStdString(options.host));
        db.setPort(options.port > 0 ? options.port : 3306);
        db.setDatabaseName(QString::fromStdString(options.database));
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
        info.provider_id = "mysql";
        info.driver_name = "QMYSQL";
        info.read_only = options.read_only;
        info.host = options.host;
        info.port = options.port > 0 ? options.port : 3306;
        info.database = options.database;
        info.schema = options.database;
        info.username = options.username;
        QSqlQuery version_query(db);
        if (version_query.exec(QStringLiteral("SELECT VERSION()")) && version_query.next()) {
            info.server_version = version_query.value(0).toString().toStdString();
        }
        connected_ = true;
        database_name_ = options.database;
        return domain::DatabaseResult<domain::ConnectionInfo>::success(std::move(info));
    }

    domain::DatabaseResult<std::vector<domain::DatabaseObject>> list_objects(
        const domain::ObjectFilter& filter) override
    {
        QSqlDatabase* db = database();
        if (db == nullptr) {
            return domain::DatabaseResult<std::vector<domain::DatabaseObject>>::failure(
                "not_connected", "尚未连接 MySQL。");
        }
        QSqlQuery query(*db);
        query.prepare(QStringLiteral(
            "SELECT TABLE_SCHEMA, TABLE_NAME, TABLE_TYPE "
            "FROM INFORMATION_SCHEMA.TABLES "
            "WHERE TABLE_SCHEMA = DATABASE() "
            "ORDER BY TABLE_NAME"));
        if (!query.exec()) {
            return domain::DatabaseResult<std::vector<domain::DatabaseObject>>::failure(
                "list_failed", sanitize_error(query.lastError()));
        }
        std::vector<domain::DatabaseObject> objects;
        while (query.next()) {
            domain::DatabaseObject object;
            object.ref.schema = query.value(0).toString().toStdString();
            object.ref.name = query.value(1).toString().toStdString();
            const QString type = query.value(2).toString().toUpper();
            object.ref.kind = type.contains(QStringLiteral("VIEW"))
                ? domain::DatabaseObjectKind::view
                : domain::DatabaseObjectKind::table;
            if (object.ref.kind == domain::DatabaseObjectKind::table && !filter.include_tables) {
                continue;
            }
            if (object.ref.kind == domain::DatabaseObjectKind::view && !filter.include_views) {
                continue;
            }
            if (!filter.name_contains.empty()) {
                const QString haystack = QString::fromStdString(object.ref.name);
                if (!haystack.contains(QString::fromStdString(filter.name_contains),
                                       Qt::CaseInsensitive)) {
                    continue;
                }
            }
            objects.push_back(std::move(object));
        }
        return domain::DatabaseResult<std::vector<domain::DatabaseObject>>::success(
            std::move(objects));
    }

    domain::DatabaseResult<domain::TableMetadata> describe(
        const domain::ObjectRef& object) override
    {
        QSqlDatabase* db = database();
        if (db == nullptr) {
            return domain::DatabaseResult<domain::TableMetadata>::failure(
                "not_connected", "尚未连接 MySQL。");
        }
        domain::TableMetadata metadata;
        metadata.object.ref = object;
        metadata.object.ref.schema = database_name_;
        metadata.object.selectable = true;

        QSqlQuery query(*db);
        query.prepare(QStringLiteral(
            "SELECT COLUMN_NAME, DATA_TYPE, IS_NULLABLE, COLUMN_DEFAULT, "
            "ORDINAL_POSITION, COLUMN_KEY, EXTRA, GENERATION_EXPRESSION "
            "FROM INFORMATION_SCHEMA.COLUMNS "
            "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = ? "
            "ORDER BY ORDINAL_POSITION"));
        query.addBindValue(QString::fromStdString(object.name));
        if (!query.exec()) {
            return domain::DatabaseResult<domain::TableMetadata>::failure(
                "describe_failed", sanitize_error(query.lastError()));
        }
        while (query.next()) {
            domain::ColumnMetadata column;
            column.name = query.value(0).toString().toStdString();
            column.native_type = query.value(1).toString().toStdString();
            column.normalized_type = normalize_mysql_type(query.value(1).toString());
            column.nullable =
                query.value(2).toString().compare(QStringLiteral("YES"), Qt::CaseInsensitive) == 0;
            column.default_expression = query.value(3).toString().toStdString();
            column.ordinal = query.value(4).toInt();
            const QString key = query.value(5).toString().toUpper();
            column.primary_key = key == QStringLiteral("PRI");
            const QString extra = query.value(6).toString().toUpper();
            column.generated = extra.contains(QStringLiteral("GENERATED"))
                || !query.value(7).toString().isEmpty();
            column.quoted_identifier = quote_mysql_identifier(column.name);
            column.selectable = true;
            if (column.primary_key) {
                metadata.primary_key_columns.push_back(column.name);
            }
            metadata.columns.push_back(std::move(column));
        }
        if (metadata.columns.empty()) {
            return domain::DatabaseResult<domain::TableMetadata>::failure(
                "object_not_found", "对象不存在或不可见：" + object.name);
        }
        metadata.supports_keyset_paging = !metadata.primary_key_columns.empty();
        metadata.estimated_rows_unknown = true;
        QSqlQuery estimate(*db);
        estimate.prepare(QStringLiteral(
            "SELECT TABLE_ROWS FROM INFORMATION_SCHEMA.TABLES "
            "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = ?"));
        estimate.addBindValue(QString::fromStdString(object.name));
        if (estimate.exec() && estimate.next()) {
            const qlonglong value = estimate.value(0).toLongLong();
            if (value >= 0) {
                metadata.estimated_rows = static_cast<std::uint64_t>(value);
                metadata.estimated_rows_unknown = false;
            }
        }
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
            "preview_not_import",
            "预览行数限制不影响最终 ImportPlan。TABLE_ROWS 为估计值。",
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
        database_name_.clear();
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
                "not_connected", "尚未连接 MySQL。");
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
        QStringList select_parts;
        for (const std::string& name : ordered_columns) {
            select_parts.append(QString::fromStdString(by_name[name].quoted_identifier));
        }
        QString sql = QStringLiteral("SELECT %1 FROM %2")
            .arg(select_parts.join(QStringLiteral(", ")))
            .arg(QString::fromStdString(quote_mysql_identifier(plan.object_ref.name)));

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
        if (effective_limit.has_value()) {
            sql += QStringLiteral(" LIMIT ?");
            bound.append(qulonglong(*effective_limit));
            if (!used_keyset && plan.row_offset.has_value() && *plan.row_offset > 0) {
                sql += QStringLiteral(" OFFSET ?");
                bound.append(qulonglong(*plan.row_offset));
            }
        }

        bool in_transaction = db->transaction();
        QSqlQuery query(*db);
        if (!query.prepare(sql)) {
            if (in_transaction) {
                db->rollback();
            }
            return domain::DatabaseResult<domain::ImportedTable>::failure(
                "prepare_failed", sanitize_error(query.lastError()));
        }
        for (const QVariant& value : bound) {
            query.addBindValue(value);
        }
        if (!query.exec()) {
            if (in_transaction) {
                db->rollback();
            }
            return domain::DatabaseResult<domain::ImportedTable>::failure(
                "query_failed", sanitize_error(query.lastError()));
        }

        domain::ImportedTable imported;
        imported.plan = plan;
        domain::DataTable& table = imported.table;
        table.source_path = "mysql://" + database_name_ + "/" + plan.object_ref.name;
        table.name = plan.target_worksheet_name.empty()
            ? plan.object_ref.name
            : plan.target_worksheet_name;
        for (std::size_t index = 0; index < ordered_columns.size(); ++index) {
            table.columns.push_back(ordered_columns[index]);
            table.column_types.push_back(by_name[ordered_columns[index]].normalized_type);
        }
        const bool use_pk_row_id = metadata.primary_key_columns.size() == 1
            && std::find(ordered_columns.begin(), ordered_columns.end(),
                         metadata.primary_key_columns.front())
                != ordered_columns.end();
        imported.row_id_is_synthetic = !use_pk_row_id;
        std::size_t pk_index = 0;
        if (use_pk_row_id) {
            pk_index = static_cast<std::size_t>(
                std::find(ordered_columns.begin(), ordered_columns.end(),
                          metadata.primary_key_columns.front())
                - ordered_columns.begin());
        }

        std::uint64_t synthetic = 0;
        while (query.next()) {
            if (cancel != nullptr && cancel->is_cancellation_requested()) {
                if (in_transaction) {
                    db->rollback();
                }
                return domain::DatabaseResult<domain::ImportedTable>::failure(
                    "cancelled", "导入已取消，未写入工作表。");
            }
            std::vector<std::string> row(ordered_columns.size());
            std::vector<domain::CellState> states(
                ordered_columns.size(), domain::CellState::valid);
            for (int column = 0; column < static_cast<int>(ordered_columns.size()); ++column) {
                const QVariant value = query.value(column);
                if (value.isNull()) {
                    if (plan.null_policy == domain::DatabaseNullPolicy::reject_null) {
                        if (in_transaction) {
                            db->rollback();
                        }
                        return domain::DatabaseResult<domain::ImportedTable>::failure(
                            "null_rejected", "存在数据库 NULL，且 null_policy=reject_null。");
                    }
                    states[static_cast<std::size_t>(column)] = domain::CellState::missing;
                    continue;
                }
                row[static_cast<std::size_t>(column)] = value.toString().toStdString();
                if (table.column_types[static_cast<std::size_t>(column)]
                    == domain::ColumnType::numeric) {
                    double number = 0.0;
                    states[static_cast<std::size_t>(column)] =
                        domain::parse_finite_number(
                            row[static_cast<std::size_t>(column)], number)
                        ? domain::CellState::valid
                        : domain::CellState::invalid;
                }
            }
            domain::RowId row_id = synthetic;
            if (use_pk_row_id) {
                bool ok = false;
                const qulonglong parsed =
                    query.value(static_cast<int>(pk_index)).toULongLong(&ok);
                row_id = ok ? static_cast<domain::RowId>(parsed) : synthetic;
                if (!ok) {
                    imported.row_id_is_synthetic = true;
                }
            }
            ++synthetic;
            table.rows.push_back(std::move(row));
            table.cell_states.push_back(std::move(states));
            table.row_ids.push_back(row_id);
        }
        if (in_transaction && !db->commit()) {
            db->rollback();
            return domain::DatabaseResult<domain::ImportedTable>::failure(
                "commit_failed", sanitize_error(db->lastError()));
        }
        if (used_keyset) {
            imported.diagnostics.push_back(
                {"keyset_paging", "使用 keyset 分页。", "info", false});
        } else if (plan.row_offset.has_value() && *plan.row_offset > 0) {
            imported.diagnostics.push_back({
                "offset_paging_fallback",
                "使用 OFFSET 分页；有主键时应改用 keyset。TABLE_ROWS 为估计值。",
                "warning",
                false});
        }

        table.import_metadata.schema_version = 1;
        table.import_metadata.sheet_name = plan.object_ref.name;
        table.import_metadata.original_row_count = table.rows.size();
        table.import_metadata.column_count = table.columns.size();
        table.import_metadata.provider_id = "mysql";
        table.import_metadata.source_object = database_name_ + "." + plan.object_ref.name;
        table.import_metadata.object_kind =
            metadata.object.ref.kind == domain::DatabaseObjectKind::view ? "view" : "table";
        table.import_metadata.filter_summary = domain::summarize_import_filters(plan);
        table.import_metadata.selected_columns = ordered_columns;
        table.import_metadata.row_id_is_synthetic = imported.row_id_is_synthetic;
        table.import_metadata.imported_at =
            QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
        std::ostringstream identity;
        identity << table.source_path << '|' << table.rows.size();
        table.import_metadata.dataset_id =
            std::to_string(std::hash<std::string>{}(identity.str()));
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
    std::string database_name_;
};

}  // namespace

bool mysql_driver_available(QString* reason)
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QMYSQL"))) {
        if (reason != nullptr) {
            *reason = QStringLiteral(
                "未安装 QMYSQL 驱动；MySQL Provider 不可用（保持诚实 ❌）。");
        }
        return false;
    }
    return true;
}

domain::DatabaseProviderDescriptor mysql_provider_descriptor()
{
    domain::DatabaseProviderDescriptor descriptor;
    descriptor.id = "mysql";
    descriptor.display_name = "MySQL";
    descriptor.driver_names = {"QMYSQL"};
    QString reason;
    descriptor.available = mysql_driver_available(&reason);
    descriptor.unavailable_reason = reason.toStdString();
    descriptor.capabilities.connect = true;
    descriptor.capabilities.list_tables = true;
    descriptor.capabilities.list_views = true;
    descriptor.capabilities.describe_columns = true;
    descriptor.capabilities.preview_rows = true;
    descriptor.capabilities.import_rows = true;
    descriptor.capabilities.structured_filter = true;
    descriptor.capabilities.keyset_pagination = true;
    descriptor.capabilities.transactions = true;
    descriptor.capabilities.catalog_schema = true;
    descriptor.capabilities.read_only = true;
    return descriptor;
}

std::unique_ptr<datalab::domain::IDatabaseProvider> create_mysql_database_provider()
{
    return std::make_unique<MysqlDatabaseProvider>();
}

}  // namespace datalab::infrastructure
