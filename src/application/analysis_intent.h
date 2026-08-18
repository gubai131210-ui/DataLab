#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace datalab::application {

// UI-independent values collected by AnalysisSetupDialog.
// Analysis command builders consume this DTO, making them directly testable.
struct AnalysisIntent {
    std::string command_id;
    std::map<std::string, std::vector<std::size_t>> roles;
    std::map<std::string, std::string> inputs;

    std::vector<int> role_indices(const std::string& id) const;
    int first_role_index(const std::string& id) const;
    std::string line_text(const std::string& id) const;
    std::optional<double> line_number(const std::string& id) const;
    std::optional<int> line_int(const std::string& id) const;
};

}  // namespace datalab::application
