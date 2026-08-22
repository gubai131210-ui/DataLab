#pragma once

#include "domain/database/database_provider.h"

#include <QString>

#include <memory>

namespace datalab::infrastructure {

std::unique_ptr<datalab::domain::IDatabaseProvider> create_postgresql_database_provider();

domain::DatabaseProviderDescriptor postgresql_provider_descriptor();

bool postgresql_driver_available(QString* reason = nullptr);

}  // namespace datalab::infrastructure
