#pragma once

#include "domain/database/database_provider.h"

#include <QString>

#include <memory>

namespace datalab::infrastructure {

std::unique_ptr<datalab::domain::IDatabaseProvider> create_sqlite_database_provider();

domain::DatabaseProviderDescriptor sqlite_provider_descriptor();

bool sqlite_driver_available(QString* reason = nullptr);

}  // namespace datalab::infrastructure
