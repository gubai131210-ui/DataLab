#pragma once

#include "domain/database/database_provider.h"
#include "domain/database_types.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace datalab::application {

class DatabaseImportService final {
public:
    explicit DatabaseImportService(domain::DatabaseProviderRegistry& registry);

    std::vector<domain::DatabaseProviderDescriptor> list_providers() const;

    domain::DatabaseResult<domain::ConnectionInfo> connect(
        const domain::ConnectionOptions& options);

    domain::DatabaseResult<std::vector<domain::DatabaseObject>> list_objects(
        const domain::ObjectFilter& filter = {});

    domain::DatabaseResult<domain::TableMetadata> describe(
        const domain::ObjectRef& object);

    domain::DatabaseResult<domain::QueryPreview> preview(
        const domain::ImportPlan& plan,
        std::uint64_t preview_limit);

    domain::DatabaseResult<domain::ImportedTable> import_table(
        const domain::ImportPlan& plan,
        domain::ImportCancellationToken* cancel = nullptr);

    domain::ImportPlan build_plan(
        const domain::ObjectRef& object,
        const std::vector<std::string>& selected_columns,
        const std::vector<domain::StructuredFilter>& filters,
        const std::string& order_key,
        std::optional<std::uint64_t> row_limit,
        domain::DatabaseNullPolicy null_policy,
        const std::string& target_worksheet_name) const;

    void disconnect();

    bool has_active_provider() const;
    std::string active_provider_id() const;
    std::string active_connection_id() const;

private:
    domain::DatabaseProviderRegistry& registry_;
    std::unique_ptr<domain::IDatabaseProvider> provider_;
    std::string active_provider_id_;
    std::string active_connection_id_;
};

}  // namespace datalab::application
