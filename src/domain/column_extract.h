#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::domain {

bool is_missing_cell(const std::string& cell);

struct ExtractedNumericColumn {
    std::string name;
    std::vector<double> values;
    std::vector<std::size_t> source_rows;
    std::size_t missing_count = 0;
    std::size_t invalid_count = 0;
    std::size_t excluded_count = 0;
    std::size_t total_count = 0;
    bool column_valid = true;
};

ExtractedNumericColumn extract_numeric_column(
    const DataTable& table,
    std::size_t column_index,
    const std::vector<std::size_t>& excluded_rows);

std::vector<std::string> extract_text_column(
    const DataTable& table,
    std::size_t column_index);

std::string column_label(const DataTable& table, std::size_t column_index);

}  // namespace datalab::domain
