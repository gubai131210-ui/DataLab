#include "domain/database/keyset_sql.h"

#include <sstream>

namespace datalab::domain {

KeysetSqlFragment build_keyset_sql_fragment(
    const KeysetCursor& cursor,
    const std::map<std::string, std::string>& quoted_identifiers)
{
    KeysetSqlFragment fragment;
    if (cursor.columns.empty() || cursor.columns.size() != cursor.after_values.size()) {
        return fragment;
    }
    for (const std::string& column : cursor.columns) {
        if (quoted_identifiers.count(column) == 0) {
            return {};
        }
        fragment.order_columns.push_back(column);
    }

    std::ostringstream where;
    for (std::size_t depth = 0; depth < cursor.columns.size(); ++depth) {
        if (depth > 0) {
            where << " OR ";
        }
        where << '(';
        for (std::size_t index = 0; index < depth; ++index) {
            where << quoted_identifiers.at(cursor.columns[index]) << " = ? AND ";
            fragment.bound_values.push_back(cursor.after_values[index]);
        }
        where << quoted_identifiers.at(cursor.columns[depth]) << " > ?";
        fragment.bound_values.push_back(cursor.after_values[depth]);
        where << ')';
    }
    fragment.where_sql = where.str();
    return fragment;
}

}  // namespace datalab::domain
