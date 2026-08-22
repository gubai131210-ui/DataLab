#pragma once

#include "domain/database/database_provider.h"

#include <QString>

#include <memory>

namespace datalab::infrastructure {

std::unique_ptr<datalab::domain::IDatabaseProvider> create_odbc_database_provider();
domain::DatabaseProviderDescriptor odbc_provider_descriptor();
bool odbc_driver_available(QString* reason = nullptr);

}  // namespace datalab::infrastructure
