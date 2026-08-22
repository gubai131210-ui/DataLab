#pragma once

#include "domain/quality_types.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain {

enum class DatabaseObjectKind {
    table,
    view,
    materialized_view,
    system,
    unknown
};

enum class DatabaseNullPolicy {
    keep_null,
    empty_string_as_null,
    reject_null
};

struct ProviderCapabilities {
    bool connect = false;
    bool list_tables = false;
    bool list_views = false;
    bool describe_columns = false;
    bool preview_rows = false;
    bool import_rows = false;
    bool structured_filter = false;
    bool keyset_pagination = false;
    bool transactions = false;
    bool catalog_schema = false;
    bool ssl = false;
    bool timeout = false;
    bool read_only = true;
};

struct DatabaseProviderDescriptor {
    std::string id;
    std::string display_name;
    std::vector<std::string> driver_names;
    ProviderCapabilities capabilities;
    bool available = false;
    std::string unavailable_reason;
};

struct ConnectionOptions {
    std::string provider_id;
    std::string display_name;
    std::string host;
    int port = 0;
    std::string database;
    std::string schema;
    std::string username;
    // Transient only: never serialize into ImportPlan / project JSON / reports.
    std::string password;
    std::string credential_ref;
    std::string file_path;
    std::string dsn;
    std::string ssl_mode;
    bool read_only = true;
    int timeout_ms = 15000;
};

struct ConnectionProfile {
    std::string connection_id;
    std::string provider_id;
    std::string display_name;
    std::string host;
    int port = 0;
    std::string database;
    std::string schema;
    std::string username;
    std::string credential_ref;
    std::string file_path;
    std::string ssl_mode;
    bool read_only = true;
    int timeout_ms = 15000;
};

struct ConnectionInfo {
    std::string connection_id;
    std::string provider_id;
    std::string driver_name;
    std::string server_version;
    bool read_only = true;
    std::string host;
    int port = 0;
    std::string database;
    std::string schema;
    std::string username;
};

struct ObjectFilter {
    std::string catalog;
    std::string schema;
    std::string name_contains;
    bool include_tables = true;
    bool include_views = true;
    bool include_system = false;
};

struct ObjectRef {
    std::string catalog;
    std::string schema;
    std::string name;
    DatabaseObjectKind kind = DatabaseObjectKind::unknown;
};

struct DatabaseObject {
    ObjectRef ref;
    bool selectable = true;
    std::optional<std::uint64_t> estimated_rows;
    bool estimated_rows_unknown = true;
};

struct ColumnMetadata {
    int ordinal = 0;
    std::string name;
    std::string display_name;
    std::string native_type;
    ColumnType normalized_type = ColumnType::unknown;
    bool nullable = true;
    std::string default_expression;
    bool primary_key = false;
    bool generated = false;
    bool hidden = false;
    bool selectable = true;
    bool read_only = false;
    std::string quoted_identifier;
};

struct ForeignKeyMetadata {
    std::string name;
    std::vector<std::string> columns;
    ObjectRef referenced_object;
    std::vector<std::string> referenced_columns;
};

struct TableMetadata {
    DatabaseObject object;
    std::vector<ColumnMetadata> columns;
    std::vector<std::string> primary_key_columns;
    std::vector<ForeignKeyMetadata> foreign_keys;
    bool supports_keyset_paging = false;
    std::optional<std::uint64_t> estimated_rows;
    bool estimated_rows_unknown = true;
};

struct StructuredFilter {
    std::string column;
    std::string op;
    std::string value;
};

// Exclusive keyset cursor. Prefer over OFFSET.
// Single-column: columns={k}, after_values={v}  =>  k > ?
// Composite: lexicographic continuation
//   (c1 > ?) OR (c1 = ? AND c2 > ?) OR (c1 = ? AND c2 = ? AND c3 > ?) ...
struct KeysetCursor {
    std::vector<std::string> columns;
    std::vector<std::string> after_values;

    static KeysetCursor single(std::string column, std::string after_value)
    {
        KeysetCursor cursor;
        cursor.columns.push_back(std::move(column));
        cursor.after_values.push_back(std::move(after_value));
        return cursor;
    }

    bool empty() const
    {
        return columns.empty();
    }
};

struct ImportPlan {
    std::string source_connection_id;
    std::string provider_id;
    ObjectRef object_ref;
    std::vector<std::string> selected_columns;
    std::vector<std::string> column_order;
    std::vector<std::string> aliases;
    std::vector<StructuredFilter> structured_filter;
    std::string order_key;
    std::optional<std::uint64_t> row_limit;
    std::optional<std::uint64_t> row_offset;
    std::optional<KeysetCursor> keyset_after;
    std::optional<std::uint64_t> page_size;
    DatabaseNullPolicy null_policy = DatabaseNullPolicy::keep_null;
    std::string target_worksheet_name;
    std::string source_snapshot_time;
};

class ImportCancellationToken {
public:
    ImportCancellationToken();
    void request_cancel();
    bool is_cancellation_requested() const;

private:
    std::shared_ptr<std::atomic<bool>> cancelled_;
};

std::string validate_import_plan(
    const ImportPlan& plan,
    const TableMetadata& metadata);

std::string summarize_import_filters(const ImportPlan& plan);

struct ImportDiagnostic {
    std::string code;
    std::string message;
    std::string severity = "info";
    bool contains_secrets = false;
};

struct QueryPreview {
    TableMetadata metadata;
    DataTable sample;
    std::uint64_t preview_row_count = 0;
    bool truncated = false;
    std::vector<ImportDiagnostic> diagnostics;
};

struct ImportedTable {
    DataTable table;
    ImportPlan plan;
    std::vector<ImportDiagnostic> diagnostics;
    bool row_id_is_synthetic = false;
};

template <typename T>
struct DatabaseResult {
    bool ok = false;
    T value{};
    std::string error_code;
    std::string error_message;
    std::vector<ImportDiagnostic> diagnostics;

    static DatabaseResult success(T value)
    {
        DatabaseResult result;
        result.ok = true;
        result.value = std::move(value);
        return result;
    }

    static DatabaseResult failure(
        std::string code,
        std::string message,
        std::vector<ImportDiagnostic> diagnostics = {})
    {
        DatabaseResult result;
        result.ok = false;
        result.error_code = std::move(code);
        result.error_message = std::move(message);
        result.diagnostics = std::move(diagnostics);
        return result;
    }
};

}  // namespace datalab::domain
