#pragma once

#include "domain/quality_types.h"

#include <string>

namespace datalab::infrastructure {

// Identifies worksheet contents and physical row order for result row linking.
// Ignores derived type/cell metadata; an absent cell is equivalent to an empty cell.
std::string table_fingerprint(const domain::DataTable& table);

} // namespace datalab::infrastructure
