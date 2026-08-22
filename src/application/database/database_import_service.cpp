#include "application/database/database_import_service.h"

namespace datalab::application {

DatabaseImportService::DatabaseImportService(domain::DatabaseProviderRegistry& registry)
    : registry_(registry)
{
}

std::vector<domain::DatabaseProviderDescriptor> DatabaseImportService::list_providers() const
{
    return registry_.descriptors();
}

domain::DatabaseResult<domain::ConnectionInfo> DatabaseImportService::connect(
    const domain::ConnectionOptions& options)
{
    disconnect();
    auto created = registry_.create(options.provider_id);
    if (!created.ok) {
        return domain::DatabaseResult<domain::ConnectionInfo>::failure(
            created.error_code, created.error_message, created.diagnostics);
    }
    provider_ = std::move(created.value);
    active_provider_id_ = options.provider_id;
    auto connected = provider_->connect(options);
    if (!connected.ok) {
        disconnect();
        return connected;
    }
    active_connection_id_ = connected.value.connection_id;
    return connected;
}

domain::DatabaseResult<std::vector<domain::DatabaseObject>> DatabaseImportService::list_objects(
    const domain::ObjectFilter& filter)
{
    if (!provider_) {
        return domain::DatabaseResult<std::vector<domain::DatabaseObject>>::failure(
            "not_connected", "尚未连接数据库。");
    }
    return provider_->list_objects(filter);
}

domain::DatabaseResult<domain::TableMetadata> DatabaseImportService::describe(
    const domain::ObjectRef& object)
{
    if (!provider_) {
        return domain::DatabaseResult<domain::TableMetadata>::failure(
            "not_connected", "尚未连接数据库。");
    }
    return provider_->describe(object);
}

domain::DatabaseResult<domain::QueryPreview> DatabaseImportService::preview(
    const domain::ImportPlan& plan,
    std::uint64_t preview_limit)
{
    if (!provider_) {
        return domain::DatabaseResult<domain::QueryPreview>::failure(
            "not_connected", "尚未连接数据库。");
    }
    return provider_->preview(plan, preview_limit);
}

domain::DatabaseResult<domain::ImportedTable> DatabaseImportService::import_table(
    const domain::ImportPlan& plan,
    domain::ImportCancellationToken* cancel)
{
    if (!provider_) {
        return domain::DatabaseResult<domain::ImportedTable>::failure(
            "not_connected", "尚未连接数据库。");
    }
    return provider_->import_table(plan, cancel);
}

domain::ImportPlan DatabaseImportService::build_plan(
    const domain::ObjectRef& object,
    const std::vector<std::string>& selected_columns,
    const std::vector<domain::StructuredFilter>& filters,
    const std::string& order_key,
    std::optional<std::uint64_t> row_limit,
    domain::DatabaseNullPolicy null_policy,
    const std::string& target_worksheet_name) const
{
    domain::ImportPlan plan;
    plan.source_connection_id = active_connection_id_;
    plan.provider_id = active_provider_id_;
    plan.object_ref = object;
    plan.selected_columns = selected_columns;
    plan.column_order = selected_columns;
    plan.structured_filter = filters;
    plan.order_key = order_key;
    plan.row_limit = row_limit;
    plan.null_policy = null_policy;
    plan.target_worksheet_name = target_worksheet_name.empty()
        ? object.name
        : target_worksheet_name;
    return plan;
}

void DatabaseImportService::disconnect()
{
    if (provider_) {
        provider_->disconnect();
        provider_.reset();
    }
    active_provider_id_.clear();
    active_connection_id_.clear();
}

bool DatabaseImportService::has_active_provider() const
{
    return static_cast<bool>(provider_);
}

std::string DatabaseImportService::active_provider_id() const
{
    return active_provider_id_;
}

std::string DatabaseImportService::active_connection_id() const
{
    return active_connection_id_;
}

}  // namespace datalab::application
