#pragma once

#include <cmath>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace datalab::tests::minitab {

struct GoldenTolerance {
    double abs_tol = 1.0e-4;
    double rel_tol = 1.0e-4;
};

struct GoldenDocument {
    std::map<std::string, std::string> config;
    std::map<std::string, std::vector<std::vector<std::string>>> sections;
};

std::optional<GoldenDocument> load_golden_tsv(const std::string& path, std::string* error);

bool compare_double(
    double actual,
    double expected,
    const GoldenTolerance& tolerance,
    std::string* message = nullptr);

std::optional<double> parse_double(const std::string& text);

std::optional<std::size_t> find_table_row(
    const std::vector<std::vector<std::string>>& rows,
    const std::string& key_column,
    const std::string& key_value);

std::optional<double> table_cell_as_double(
    const std::vector<std::vector<std::string>>& rows,
    const std::string& key_column,
    const std::string& key_value,
    const std::string& value_column);

}  // namespace datalab::tests::minitab
