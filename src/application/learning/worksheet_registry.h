#pragma once

#include "domain/quality_types.h"

#include <map>
#include <optional>
#include <string>

namespace datalab::application::learning {

/// In-memory worksheet store for learning-center imports (does not touch output pages).
class WorksheetRegistry {
public:
    void clear();

    void save_current(const domain::DataTable& current);
    std::optional<domain::DataTable> activate(
        const std::string& name, const domain::DataTable& live_table);
    void import_new(const std::string& name, domain::DataTable table);

    [[nodiscard]] const std::map<std::string, domain::DataTable>& worksheets() const
    {
        return worksheets_;
    }
    [[nodiscard]] const std::string& active_name() const { return active_worksheet_name_; }

private:
    std::string resolve_key(const domain::DataTable& current) const;

    std::map<std::string, domain::DataTable> worksheets_;
    std::string active_worksheet_name_;
};

}  // namespace datalab::application::learning
