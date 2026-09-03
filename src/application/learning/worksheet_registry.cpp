#include "application/learning/worksheet_registry.h"

namespace datalab::application::learning {

void WorksheetRegistry::clear()
{
    worksheets_.clear();
    active_worksheet_name_.clear();
}

std::string WorksheetRegistry::resolve_key(const domain::DataTable& current) const
{
    if (!active_worksheet_name_.empty()) {
        return active_worksheet_name_;
    }
    if (!current.name.empty()) {
        return current.name;
    }
    return "工作表1";
}

void WorksheetRegistry::save_current(const domain::DataTable& current)
{
    if (current.columns.empty() && current.rows.empty()) {
        return;
    }
    const std::string key = resolve_key(current);
    if (active_worksheet_name_.empty()) {
        active_worksheet_name_ = key;
    }
    worksheets_[key] = current;
}

std::optional<domain::DataTable> WorksheetRegistry::activate(
    const std::string& name, const domain::DataTable& live_table)
{
    if (name.empty()) {
        return std::nullopt;
    }
    save_current(live_table);

    auto iterator = worksheets_.find(name);
    if (iterator == worksheets_.end()) {
        if (live_table.name == name
            || (live_table.name.empty() && active_worksheet_name_ == name)) {
            worksheets_[name] = live_table;
            iterator = worksheets_.find(name);
        } else {
            return std::nullopt;
        }
    }

    active_worksheet_name_ = name;
    return iterator->second;
}

void WorksheetRegistry::import_new(const std::string& name, domain::DataTable table)
{
    table.name = name;
    worksheets_[name] = std::move(table);
    active_worksheet_name_ = name;
}

}  // namespace datalab::application::learning
