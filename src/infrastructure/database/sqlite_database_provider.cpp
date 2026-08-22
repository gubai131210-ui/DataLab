#include "infrastructure/database/sqlite_database_provider.h"

#include "domain/column_extract.h"
#include "domain/database/keyset_sql.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
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
    return QStringLiteral("datalab_sqlite_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

std::string sanitize_error(const QSqlError& error)
{
    QString text = error.text();
    // Never expose connection strings / passwords from Qt SQL diagnostics.
    text.replace(QRegularExpression(QStringLiteral("password\\s*=\\s*[^;\\s]+"),
                                    QRegularExpression::CaseInsensitiveOption),
                 QStringLiteral("password=***"));
    return text.toStdString();
}

domain::ColumnType normalize_sqlite_type(const QString& native)
{
    const QString upper = native.toUpper();
    if (upper.contains(QStringLiteral("INT"))
        || upper.contains(QStringLiteral("REAL"))
        || upper.contains(QStringLiteral("FLOA"))
        || upper.contains(QStringLiteral("DOUB"))
        || upper.contains(QStringLiteral("NUM"))) {
        return domain::ColumnType::numeric;
    }
    if (upper.contains(QStringLiteral("DATE"))
        || upper.contains(QStringLiteral("TIME"))) {
        return domain::ColumnType::time;
    }
    if (upper.contains(QStringLiteral("CHAR"))
        || upper.contains(QStringLiteral("CLOB"))
        || upper.contains(QStringLiteral("TEXT"))) {
        return domain::ColumnType::categorical;
    }
    return domain::ColumnType::unknown;
}

std::string quote_sqlite_identifier(const std::string& identifier)
{
    std::string escaped;
    escaped.reserve(identifier.size() + 2);
    escaped.push_back('"');
    for (const char character : identifier) {
        if (character == '"') {
            escaped.push_back('"');
            escaped.push_back('"');
        } else {
            escaped.push_back(character);
        }
    }
    escaped.push_back('"');
    return escaped;
}

class SqliteDatabaseProvider final : public datalab::domain::IDatabaseProvider {
public:
    ~SqliteDatabaseProvider() override
    {
        disconnect();
    }

    domain::DatabaseProviderDescriptor descriptor() const override
    {
        return sqlite_provider_descriptor();
    }

    domain::DatabaseResult<domain::ConnectionInfo> connect(
        const domain::ConnectionOptions& options) override
    {
        QString reason;
        if (!sqlite_driver_available(&reason)) {
            return domain::DatabaseResult<domain::ConnectionInfo>::failure(
                "driver_missing", reason.toStdString());
        }
        if (options.file_path.empty()) {
            return domain::DatabaseResult<domain::ConnectionInfo>::failure(
                "missing_file_path", "SQLite 连接需要 file_path。");
        }

        disconnect();
        connection_name_ = make_connection_name();
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection_name_);
        db.setDatabaseName(QString::fromStdString(options.file_path));
        if (options.read_only) {
            db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        }
        if (!db.open()) {
            const std::string message = sanitize_error(db.lastError());
            disconnect();
            return domain::DatabaseResult<domain::ConnectionInfo>::failure(
                "connect_failed", message);
        }

        domain::ConnectionInfo info;
        info.connection_id = connection_name_.toStdString();
        info.provider_id = "sqlite";
        info.driver_name = "QSQLITE";
        info.read_only = options.read_only;
        QSqlQuery version_query(db);
        if (version_query.exec(QStringLiteral("SELECT sqlite_version()"))
            && version_query.next()) {
            info.server_version = version_query.value(0).toString().toStdString();
        }
        connected_ = true;
        file_path_ = options.file_path;
        return domain::DatabaseResult<domain::ConnectionInfo>::success(std::move(info));
    }

    domain::DatabaseResult<std::vector<domain::DatabaseObject>> list_objects(
        const domain::ObjectFilter& filter) override
    {
        QSqlDatabase* db = database();
        if (db == nullptr) {
            return domain::DatabaseResult<std::vector<domain::DatabaseObject>>::failure(
                "not_connected", "尚未连接 SQLite 数据库。");
        }

        std::vector<domain::DatabaseObject> objects;
        QSqlQuery query(*db);
        // Prefer PRAGMA table_list when available (SQLite >= 3.37).
        const bool used_table_list = query.exec(
            QStringLiteral("PRAGMA table_list"));
        if (used_table_list) {
            while (query.next()) {
                const QString schema = query.value(0).toString();
                const QString name = query.value(1).toString();
                const QString type = query.value(2).toString().toLower();
                domain::DatabaseObject object;
                object.ref.schema = schema.toStdString();
                object.ref.name = name.toStdString();
                if (type == QStringLiteral("view")) {
                    object.ref.kind = domain::DatabaseObjectKind::view;
                } else if (type == QStringLiteral("table")) {
                    object.ref.kind = domain::DatabaseObjectKind::table;
                } else {
                    object.ref.kind = domain::DatabaseObjectKind::system;
                }
                const bool is_system = schema.compare(QStringLiteral("main"), Qt::CaseInsensitive) != 0
                    || name.startsWith(QStringLiteral("sqlite_"), Qt::CaseInsensitive);
                if (is_system) {
                    object.ref.kind = domain::DatabaseObjectKind::system;
                }
                if (!accepts_object(object, filter, is_system)) {
                    continue;
                }
                objects.push_back(std::move(object));
            }
        } else {
            if (!query.exec(QStringLiteral(
                    "SELECT name, type FROM sqlite_master "
                    "WHERE type IN ('table','view') ORDER BY name"))) {
                return domain::DatabaseResult<std::vector<domain::DatabaseObject>>::failure(
                    "list_objects_failed", sanitize_error(query.lastError()));
            }
            while (query.next()) {
                domain::DatabaseObject object;
                object.ref.schema = "main";
                object.ref.name = query.value(0).toString().toStdString();
                const QString type = query.value(1).toString().toLower();
                object.ref.kind = type == QStringLiteral("view")
                    ? domain::DatabaseObjectKind::view
                    : domain::DatabaseObjectKind::table;
                const bool is_system = QString::fromStdString(object.ref.name)
                    .startsWith(QStringLiteral("sqlite_"), Qt::CaseInsensitive);
                if (is_system) {
                    object.ref.kind = domain::DatabaseObjectKind::system;
                }
                if (!accepts_object(object, filter, is_system)) {
                    continue;
                }
                objects.push_back(std::move(object));
            }
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
                "not_connected", "尚未连接 SQLite 数据库。");
        }
        if (object.name.empty()) {
            return domain::DatabaseResult<domain::TableMetadata>::failure(
                "missing_object", "缺少表/视图名称。");
        }

        domain::TableMetadata metadata;
        metadata.object.ref = object;
        metadata.object.selectable = true;

        const QString quoted = QString::fromStdString(quote_sqlite_identifier(object.name));
        QSqlQuery query(*db);
        const QString pragma = QStringLiteral("PRAGMA table_xinfo(%1)").arg(quoted);
        if (!query.exec(pragma)) {
            return domain::DatabaseResult<domain::TableMetadata>::failure(
                "describe_failed", sanitize_error(query.lastError()));
        }
        while (query.next()) {
            domain::ColumnMetadata column;
            column.ordinal = query.value(0).toInt();
            column.name = query.value(1).toString().toStdString();
            column.display_name = column.name;
            column.native_type = query.value(2).toString().toStdString();
            column.normalized_type = normalize_sqlite_type(query.value(2).toString());
            column.nullable = query.value(3).toInt() == 0;
            column.default_expression = query.value(4).toString().toStdString();
            column.primary_key = query.value(5).toInt() > 0;
            // table_xinfo: cid,name,type,notnull,dflt_value,pk,hidden
            if (query.record().count() > 6) {
                const int hidden = query.value(6).toInt();
                column.hidden = hidden != 0;
                column.generated = hidden == 2 || hidden == 3;
            }
            column.selectable = !column.hidden || column.generated;
            column.quoted_identifier = quote_sqlite_identifier(column.name);
            if (column.primary_key) {
                metadata.primary_key_columns.push_back(column.name);
            }
            metadata.columns.push_back(std::move(column));
        }

        QSqlQuery count_query(*db);
        if (count_query.exec(QStringLiteral("SELECT COUNT(*) FROM %1").arg(quoted))
            && count_query.next()) {
            metadata.estimated_rows = count_query.value(0).toULongLong();
            metadata.estimated_rows_unknown = false;
            metadata.object.estimated_rows = metadata.estimated_rows;
            metadata.object.estimated_rows_unknown = false;
        }
        metadata.supports_keyset_paging = !metadata.primary_key_columns.empty();
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
            "预览行数限制不影响最终 ImportPlan。",
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
        file_path_.clear();
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
                "not_connected", "尚未连接 SQLite 数据库。");
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
            .arg(QString::fromStdString(quote_sqlite_identifier(plan.object_ref.name)));

        QVariantList bound;
        QStringList where_parts;
        if (!plan.structured_filter.empty()) {
            for (const domain::StructuredFilter& filter : plan.structured_filter) {
                const QString quoted = QString::fromStdString(
                    by_name[filter.column].quoted_identifier);
                if (filter.op == "IS NULL" || filter.op == "IS NOT NULL") {
                    where_parts.append(quoted + QLatin1Char(' ')
                        + QString::fromStdString(filter.op));
                } else {
                    where_parts.append(quoted + QLatin1Char(' ')
                        + QString::fromStdString(filter.op) + QStringLiteral(" ?"));
                    bound.append(QString::fromStdString(filter.value));
                }
            }
        }

        std::string effective_order = plan.order_key;
        if (effective_order.empty() && !metadata.primary_key_columns.empty()) {
            effective_order = metadata.primary_key_columns.front();
        }

        bool used_keyset = false;
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
            if (!keyset.order_columns.empty()) {
                effective_order = keyset.order_columns.front();
            }
            used_keyset = true;
        }

        if (!where_parts.isEmpty()) {
            sql += QStringLiteral(" WHERE ") + where_parts.join(QStringLiteral(" AND "));
        }

        if (plan.keyset_after.has_value() && plan.keyset_after->columns.size() > 1) {
            QStringList order_parts;
            for (const std::string& key : plan.keyset_after->columns) {
                order_parts.append(QString::fromStdString(by_name[key].quoted_identifier));
            }
            sql += QStringLiteral(" ORDER BY ") + order_parts.join(QStringLiteral(", "));
        } else if (!effective_order.empty()) {
            sql += QStringLiteral(" ORDER BY ")
                + QString::fromStdString(by_name[effective_order].quoted_identifier);
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
        } else if (plan.row_offset.has_value() && *plan.row_offset > 0) {
            return domain::DatabaseResult<domain::ImportedTable>::failure(
                "invalid_paging",
                "row_offset 需要同时设置 row_limit（预览分页）。");
        }

        bool in_transaction = db->transaction();
        if (!in_transaction) {
            // Read-only connections may reject explicit transactions; continue without.
        }

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
        table.source_path = "sqlite://" + file_path_ + "/" + plan.object_ref.name;
        table.name = plan.target_worksheet_name.empty()
            ? plan.object_ref.name
            : plan.target_worksheet_name;

        for (std::size_t index = 0; index < ordered_columns.size(); ++index) {
            if (!plan.aliases.empty() && index < plan.aliases.size()
                && !plan.aliases[index].empty()) {
                table.columns.push_back(plan.aliases[index]);
            } else {
                table.columns.push_back(ordered_columns[index]);
            }
            table.column_types.push_back(by_name[ordered_columns[index]].normalized_type);
        }

        const bool use_pk_row_id = metadata.primary_key_columns.size() == 1
            && std::find(ordered_columns.begin(), ordered_columns.end(),
                         metadata.primary_key_columns.front())
                != ordered_columns.end();
        imported.row_id_is_synthetic = !use_pk_row_id;
        const std::string pk_name = use_pk_row_id ? metadata.primary_key_columns.front() : "";
        std::size_t pk_index = 0;
        if (use_pk_row_id) {
            pk_index = static_cast<std::size_t>(
                std::find(ordered_columns.begin(), ordered_columns.end(), pk_name)
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
            std::vector<domain::CellState> states(ordered_columns.size(), domain::CellState::valid);
            for (int column = 0; column < static_cast<int>(ordered_columns.size()); ++column) {
                const QVariant value = query.value(column);
                const domain::ColumnType type = table.column_types[static_cast<std::size_t>(column)];
                if (value.isNull()) {
                    if (plan.null_policy == domain::DatabaseNullPolicy::reject_null) {
                        if (in_transaction) {
                            db->rollback();
                        }
                        return domain::DatabaseResult<domain::ImportedTable>::failure(
                            "null_rejected",
                            "存在数据库 NULL，且 null_policy=reject_null。");
                    }
                    row[static_cast<std::size_t>(column)] = {};
                    states[static_cast<std::size_t>(column)] = domain::CellState::missing;
                    continue;
                }

                const QString text = value.toString();
                row[static_cast<std::size_t>(column)] = text.toStdString();
                if (text.isEmpty()) {
                    // Empty string is distinct from NULL: keep valid for text-like.
                    states[static_cast<std::size_t>(column)] =
                        (type == domain::ColumnType::numeric)
                        ? domain::CellState::invalid
                        : domain::CellState::valid;
                } else if (type == domain::ColumnType::numeric) {
                    double number = 0.0;
                    states[static_cast<std::size_t>(column)] =
                        domain::parse_finite_number(row[static_cast<std::size_t>(column)], number)
                        ? domain::CellState::valid
                        : domain::CellState::invalid;
                    if (states[static_cast<std::size_t>(column)] == domain::CellState::invalid) {
                        imported.diagnostics.push_back({
                            "invalid_number",
                            "列 " + ordered_columns[static_cast<std::size_t>(column)]
                                + " 存在无法转换为数值的单元格。",
                            "warning",
                            false});
                    }
                }
            }

            domain::RowId row_id = synthetic;
            if (use_pk_row_id) {
                bool ok = false;
                const qulonglong parsed = query.value(static_cast<int>(pk_index)).toULongLong(&ok);
                if (ok) {
                    row_id = static_cast<domain::RowId>(parsed);
                } else {
                    imported.row_id_is_synthetic = true;
                    row_id = synthetic;
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
            imported.diagnostics.push_back({
                "keyset_paging",
                "预览/分页使用 keyset（WHERE order_key > ?），避免不稳定 OFFSET。",
                "info",
                false});
        } else if (plan.row_offset.has_value() && *plan.row_offset > 0) {
            imported.diagnostics.push_back({
                "offset_paging_fallback",
                "使用 OFFSET 分页；稳定性依赖 ORDER BY。有主键时应改用 keyset。",
                "warning",
                false});
        }

        const std::string contract = finalize_imported_table(table, plan, metadata, imported);
        if (!contract.empty()) {
            return domain::DatabaseResult<domain::ImportedTable>::failure(
                "contract_failed", contract, imported.diagnostics);
        }
        return domain::DatabaseResult<domain::ImportedTable>::success(std::move(imported));
    }

    static std::string finalize_imported_table(
        domain::DataTable& table,
        const domain::ImportPlan& plan,
        const domain::TableMetadata& metadata,
        domain::ImportedTable& imported)
    {
        table.import_metadata.schema_version = 1;
        table.import_metadata.sheet_name = plan.object_ref.name;
        table.import_metadata.original_row_count = table.rows.size();
        table.import_metadata.column_count = table.columns.size();
        table.import_metadata.provider_id = plan.provider_id.empty() ? "sqlite" : plan.provider_id;
        table.import_metadata.source_object =
            (plan.object_ref.schema.empty() ? "main" : plan.object_ref.schema)
            + "." + plan.object_ref.name;
        table.import_metadata.object_kind =
            metadata.object.ref.kind == domain::DatabaseObjectKind::view ? "view" : "table";
        table.import_metadata.filter_summary = domain::summarize_import_filters(plan);
        table.import_metadata.selected_columns = plan.column_order.empty()
            ? plan.selected_columns
            : plan.column_order;
        table.import_metadata.row_id_is_synthetic = imported.row_id_is_synthetic;
        table.import_metadata.imported_at = plan.source_snapshot_time;
        if (table.import_metadata.imported_at.empty()) {
            table.import_metadata.imported_at = QDateTime::currentDateTimeUtc()
                .toString(Qt::ISODate)
                .toStdString();
        }

        std::ostringstream identity;
        identity << table.source_path << '|' << table.rows.size() << '|';
        for (const std::string& column : table.columns) {
            identity << column << ',';
        }
        identity << '|' << table.import_metadata.filter_summary;
        table.import_metadata.dataset_id =
            std::to_string(std::hash<std::string>{}(identity.str()));

        for (const domain::ImportDiagnostic& diagnostic : imported.diagnostics) {
            if (diagnostic.severity == "warning") {
                table.import_warnings.push_back(diagnostic.message);
            }
        }
        table.import_metadata.warnings = table.import_warnings;
        return domain::validate_data_table_contract(table);
    }

    QSqlDatabase* database()
    {
        if (!connected_ || connection_name_.isEmpty()) {
            return nullptr;
        }
        owned_ = QSqlDatabase::database(connection_name_);
        if (!owned_.isValid() || !owned_.isOpen()) {
            return nullptr;
        }
        return &owned_;
    }

    static bool accepts_object(
        const domain::DatabaseObject& object,
        const domain::ObjectFilter& filter,
        bool is_system)
    {
        if (is_system && !filter.include_system) {
            return false;
        }
        if (object.ref.kind == domain::DatabaseObjectKind::table && !filter.include_tables) {
            return false;
        }
        if (object.ref.kind == domain::DatabaseObjectKind::view && !filter.include_views) {
            return false;
        }
        if (!filter.name_contains.empty()) {
            const QString haystack = QString::fromStdString(object.ref.name);
            const QString needle = QString::fromStdString(filter.name_contains);
            if (!haystack.contains(needle, Qt::CaseInsensitive)) {
                return false;
            }
        }
        if (!filter.schema.empty() && object.ref.schema != filter.schema) {
            return false;
        }
        return true;
    }

    QString connection_name_;
    QSqlDatabase owned_;
    bool connected_ = false;
    std::string file_path_;
};

}  // namespace

bool sqlite_driver_available(QString* reason)
{
    const QStringList drivers = QSqlDatabase::drivers();
    if (!drivers.contains(QStringLiteral("QSQLITE"))) {
        if (reason != nullptr) {
            *reason = QStringLiteral("未安装 Qt QSQLITE 驱动，无法使用 SQLite Provider。");
        }
        return false;
    }
    return true;
}

domain::DatabaseProviderDescriptor sqlite_provider_descriptor()
{
    domain::DatabaseProviderDescriptor descriptor;
    descriptor.id = "sqlite";
    descriptor.display_name = "SQLite";
    descriptor.driver_names = {"QSQLITE"};
    descriptor.capabilities.connect = true;
    descriptor.capabilities.list_tables = true;
    descriptor.capabilities.list_views = true;
    descriptor.capabilities.describe_columns = true;
    descriptor.capabilities.preview_rows = true;
    descriptor.capabilities.import_rows = true;
    descriptor.capabilities.structured_filter = true;
    descriptor.capabilities.keyset_pagination = true;
    descriptor.capabilities.transactions = true;
    descriptor.capabilities.catalog_schema = false;
    descriptor.capabilities.ssl = false;
    descriptor.capabilities.timeout = false;
    descriptor.capabilities.read_only = true;
    QString reason;
    descriptor.available = sqlite_driver_available(&reason);
    descriptor.unavailable_reason = reason.toStdString();
    return descriptor;
}

std::unique_ptr<datalab::domain::IDatabaseProvider> create_sqlite_database_provider()
{
    return std::make_unique<SqliteDatabaseProvider>();
}

}  // namespace datalab::infrastructure
