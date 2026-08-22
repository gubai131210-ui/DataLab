#pragma once

#include "domain/database_types.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace datalab::domain {

// Build bound keyset predicate fragments using quoted identifiers from metadata.
// Returns empty where_sql when cursor is empty. Uses expanded OR form for portability.
struct KeysetSqlFragment {
    std::string where_sql;                 // e.g. ("a" > ?) OR ("a" = ? AND "b" > ?)
    std::vector<std::string> bound_values; // in bind order
    std::vector<std::string> order_columns;
};

KeysetSqlFragment build_keyset_sql_fragment(
    const KeysetCursor& cursor,
    const std::map<std::string, std::string>& quoted_identifiers);

}  // namespace datalab::domain
