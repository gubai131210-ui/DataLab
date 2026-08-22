#pragma once

#include "domain/database_types.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace datalab::domain {

class IDatabaseProvider {
public:
    virtual ~IDatabaseProvider() = default;

    virtual DatabaseProviderDescriptor descriptor() const = 0;

    virtual DatabaseResult<ConnectionInfo> connect(const ConnectionOptions& options) = 0;

    virtual DatabaseResult<std::vector<DatabaseObject>> list_objects(
        const ObjectFilter& filter) = 0;

    virtual DatabaseResult<TableMetadata> describe(const ObjectRef& object) = 0;

    virtual DatabaseResult<QueryPreview> preview(
        const ImportPlan& plan,
        std::uint64_t preview_limit) = 0;

    virtual DatabaseResult<ImportedTable> import_table(
        const ImportPlan& plan,
        ImportCancellationToken* cancel = nullptr) = 0;

    virtual void disconnect() = 0;
};

using DatabaseProviderFactory = std::function<std::unique_ptr<IDatabaseProvider>()>;

class DatabaseProviderRegistry {
public:
    DatabaseResult<bool> register_factory(
        DatabaseProviderDescriptor descriptor,
        DatabaseProviderFactory factory);

    std::vector<DatabaseProviderDescriptor> descriptors() const;
    const DatabaseProviderDescriptor* find(std::string_view provider_id) const;

    DatabaseResult<std::unique_ptr<IDatabaseProvider>> create(
        std::string_view provider_id) const;

private:
    struct Entry {
        DatabaseProviderDescriptor descriptor;
        DatabaseProviderFactory factory;
    };

    std::vector<Entry> entries_;
};

DatabaseProviderRegistry& default_database_provider_registry();

}  // namespace datalab::domain
