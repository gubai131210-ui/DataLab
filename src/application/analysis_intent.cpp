#include "application/analysis_intent.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace datalab::application {
namespace {

std::string trim(std::string value)
{
    value.erase(value.begin(),
                std::find_if(value.begin(), value.end(),
                             [](unsigned char character) {
                                 return !std::isspace(character);
                             }));
    value.erase(std::find_if(value.rbegin(), value.rend(),
                             [](unsigned char character) {
                                 return !std::isspace(character);
                             }).base(),
                value.end());
    return value;
}

}  // namespace

std::vector<int> AnalysisIntent::role_indices(const std::string& id) const
{
    const auto found = roles.find(id);
    if (found == roles.end()) {
        return {};
    }
    std::vector<int> result;
    result.reserve(found->second.size());
    for (const std::size_t index : found->second) {
        result.push_back(static_cast<int>(index));
    }
    return result;
}

int AnalysisIntent::first_role_index(const std::string& id) const
{
    const auto found = roles.find(id);
    return found == roles.end() || found->second.empty()
        ? -1
        : static_cast<int>(found->second.front());
}

std::string AnalysisIntent::line_text(const std::string& id) const
{
    const auto found = inputs.find(id);
    return found == inputs.end() ? std::string{} : found->second;
}

std::optional<double> AnalysisIntent::line_number(const std::string& id) const
{
    const std::string value = trim(line_text(id));
    if (value.empty()) {
        return std::nullopt;
    }
    char* end = nullptr;
    const double parsed = std::strtod(value.c_str(), &end);
    return end != value.c_str() && *end == '\0' ? std::optional<double>(parsed)
                                                : std::nullopt;
}

std::optional<int> AnalysisIntent::line_int(const std::string& id) const
{
    const std::string value = trim(line_text(id));
    if (value.empty()) {
        return std::nullopt;
    }
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    return end != value.c_str() && *end == '\0' ? std::optional<int>(static_cast<int>(parsed))
                                                : std::nullopt;
}

}  // namespace datalab::application
