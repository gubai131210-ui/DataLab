#include "infrastructure/database/database_provider_bootstrap.h"

#include "infrastructure/database/mysql_database_provider.h"
#include "infrastructure/database/odbc_database_provider.h"
#include "infrastructure/database/postgresql_database_provider.h"
#include "infrastructure/database/sqlite_database_provider.h"

namespace datalab::infrastructure {
namespace {

void register_gated(
    domain::DatabaseProviderRegistry& registry,
    domain::DatabaseProviderDescriptor descriptor,
    domain::DatabaseProviderFactory factory)
{
    if (descriptor.available) {
        (void)registry.register_factory(descriptor, std::move(factory));
    } else {
        (void)registry.register_factory(descriptor, {});
    }
}

}  // namespace

void register_builtin_database_providers(domain::DatabaseProviderRegistry& registry)
{
    domain::DatabaseProviderDescriptor sqlite = sqlite_provider_descriptor();
    (void)registry.register_factory(
        sqlite,
        [] { return create_sqlite_database_provider(); });

    register_gated(
        registry,
        postgresql_provider_descriptor(),
        [] { return create_postgresql_database_provider(); });
    register_gated(
        registry,
        mysql_provider_descriptor(),
        [] { return create_mysql_database_provider(); });
    register_gated(
        registry,
        odbc_provider_descriptor(),
        [] { return create_odbc_database_provider(); });
}

void ensure_default_database_providers_registered()
{
    static const bool registered = [] {
        register_builtin_database_providers(
            domain::default_database_provider_registry());
        return true;
    }();
    (void)registered;
}

}  // namespace datalab::infrastructure
