#pragma once

#include "domain/database/database_provider.h"

namespace datalab::infrastructure {

void register_builtin_database_providers(domain::DatabaseProviderRegistry& registry);

void ensure_default_database_providers_registered();

}  // namespace datalab::infrastructure
