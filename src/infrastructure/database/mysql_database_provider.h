#pragma once

#include "domain/database/database_provider.h"

#include <QString>

#include <memory>

namespace datalab::infrastructure {

std::unique_ptr<datalab::domain::IDatabaseProvider> create_mysql_database_provider();
domain::DatabaseProviderDescriptor mysql_provider_descriptor();
bool mysql_driver_available(QString* reason = nullptr);

}  // namespace datalab::infrastructure
