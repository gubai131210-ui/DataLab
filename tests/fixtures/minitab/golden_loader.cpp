#include "golden_loader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace datalab::tests::minitab {
namespace {

std::string trim(std::string text)
{
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    text.erase(text.begin(),
               std::find_if(text.begin(), text.end(), not_space));
    text.erase(std::find_if(text.rbegin(), text.rend(), not_space).base(),
                text.end());
    return text;
}

std::vector<std::string> split_tsv(const std::string& line)
{
    std::vector<std::string> cells;
    std::string cell;
    std::istringstream stream(line);
    while (std::getline(stream, cell, '\t')) {
        cells.push_back(trim(cell));
    }
    return cells;
}

bool starts_with(const std::string& text, const std::string& prefix)
{
    return text.size() >= prefix.size()
        && text.compare(0, prefix.size(), prefix) == 0;
}

}  // namespace

std::optional<GoldenDocument> load_golden_tsv(const std::string& path, std::string* error)
{
    std::ifstream input(path);
    if (!input.is_open()) {
        if (error != nullptr) {
            *error = "无法打开 golden 文件: " + path;
        }
        return std::nullopt;
    }

    GoldenDocument document;
    std::string current_section;
    std::vector<std::vector<std::string>>* current_rows = nullptr;

    for (std::string line; std::getline(input, line);) {
        line = trim(line);
        if (line.empty() || starts_with(line, "#")) {
            if (starts_with(line, "# section:")) {
                current_section = trim(line.substr(std::string("# section:").size()));
                current_rows = &document.sections[current_section];
            } else if (starts_with(line, "# config:")) {
                const std::string config_line =
                    trim(line.substr(std::string("# config:").size()));
                const std::size_t separator = config_line.find('=');
                if (separator != std::string::npos) {
                    document.config[trim(config_line.substr(0, separator))] =
                        trim(config_line.substr(separator + 1));
                }
            }
            continue;
        }

        const std::vector<std::string> cells = split_tsv(line);
        if (cells.empty()) {
            continue;
        }
        if (current_section.empty()) {
            current_section = "default";
            current_rows = &document.sections[current_section];
        }
        current_rows->push_back(cells);
    }

    if (document.sections.empty() && document.config.empty()) {
        if (error != nullptr) {
            *error = "golden 文件为空: " + path;
        }
        return std::nullopt;
    }
    return document;
}

bool compare_double(
    double actual,
    double expected,
    const GoldenTolerance& tolerance,
    std::string* message)
{
    if (!std::isfinite(actual) || !std::isfinite(expected)) {
        if (message != nullptr) {
            *message = "非有限数值: actual=" + std::to_string(actual)
                + " expected=" + std::to_string(expected);
        }
        return false;
    }
    const double diff = std::abs(actual - expected);
    const double scale = std::max({1.0, std::abs(actual), std::abs(expected)});
    if (diff <= tolerance.abs_tol || diff <= tolerance.rel_tol * scale) {
        return true;
    }
    if (message != nullptr) {
        *message = "actual=" + std::to_string(actual)
            + " expected=" + std::to_string(expected)
            + " diff=" + std::to_string(diff);
    }
    return false;
}

std::optional<double> parse_double(const std::string& text)
{
    if (text.empty() || text == "*" || text == "NA" || text == "N/A") {
        return std::nullopt;
    }
    try {
        std::size_t consumed = 0;
        const double value = std::stod(text, &consumed);
        if (consumed == 0) {
            return std::nullopt;
        }
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::size_t> find_table_row(
    const std::vector<std::vector<std::string>>& rows,
    const std::string& key_column,
    const std::string& key_value)
{
    if (rows.empty()) {
        return std::nullopt;
    }
    std::size_t key_index = 0;
    for (std::size_t index = 0; index < rows.front().size(); ++index) {
        if (rows.front()[index] == key_column) {
            key_index = index;
            break;
        }
    }
    for (std::size_t row = 1; row < rows.size(); ++row) {
        if (row < rows.size() && key_index < rows[row].size()
            && rows[row][key_index] == key_value) {
            return row;
        }
    }
    return std::nullopt;
}

std::optional<double> table_cell_as_double(
    const std::vector<std::vector<std::string>>& rows,
    const std::string& key_column,
    const std::string& key_value,
    const std::string& value_column)
{
    if (rows.empty()) {
        return std::nullopt;
    }
    std::size_t key_index = 0;
    std::size_t value_index = 0;
    for (std::size_t index = 0; index < rows.front().size(); ++index) {
        if (rows.front()[index] == key_column) {
            key_index = index;
        }
        if (rows.front()[index] == value_column) {
            value_index = index;
        }
    }
    const auto row = find_table_row(rows, key_column, key_value);
    if (!row.has_value() || value_index >= rows[*row].size()) {
        return std::nullopt;
    }
    return parse_double(rows[*row][value_index]);
}

}  // namespace datalab::tests::minitab
