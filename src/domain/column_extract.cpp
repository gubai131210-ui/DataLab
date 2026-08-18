#include "domain/column_extract.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace datalab::domain {
namespace {

std::string trim_copy(std::string value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.pop_back();
    }
    return value;
}

std::string to_upper(std::string value)
{
    for (char& character : value) {
        character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
    }
    return value;
}

bool is_excluded(const std::vector<std::size_t>& excluded_rows, std::size_t row)
{
    return std::find(excluded_rows.begin(), excluded_rows.end(), row) != excluded_rows.end();
}

bool looks_like_time(const std::string& value)
{
    const std::string trimmed = trim_copy(value);
    return trimmed.find('-') != std::string::npos
        || trimmed.find('/') != std::string::npos
        || trimmed.find(':') != std::string::npos
        || trimmed.find('T') != std::string::npos
        || trimmed.find('t') != std::string::npos;
}

}  // namespace

bool parse_finite_number(const std::string& cell, double& value)
{
    const std::string trimmed = trim_copy(cell);
    if (trimmed.empty()) {
        return false;
    }
    char* end = nullptr;
    value = std::strtod(trimmed.c_str(), &end);
    while (end != nullptr && std::isspace(static_cast<unsigned char>(*end)) != 0) {
        ++end;
    }
    return end != trimmed.c_str() && end != nullptr && *end == '\0'
        && std::isfinite(value);
}

bool is_missing_cell(const std::string& cell)
{
    const std::string trimmed = trim_copy(cell);
    if (trimmed.empty() || trimmed == "*") {
        return true;
    }
    const std::string upper = to_upper(trimmed);
    return upper == "NA" || upper == "N/A" || upper == "NAN" || upper == "NULL";
}

void populate_data_table_contract(DataTable& table)
{
    table.import_metadata.original_row_count = table.rows.size();
    table.row_ids.resize(table.rows.size());
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        table.row_ids[row] = static_cast<RowId>(row);
    }

    table.column_types.assign(table.columns.size(), ColumnType::unknown);
    table.cell_states.assign(
        table.rows.size(), std::vector<CellState>(table.columns.size(), CellState::missing));
    for (std::size_t column = 0; column < table.columns.size(); ++column) {
        bool has_valid = false;
        bool has_numeric = false;
        bool all_numeric = true;
        bool all_time_like = true;
        for (std::size_t row = 0; row < table.rows.size(); ++row) {
            if (column >= table.rows[row].size()
                || is_missing_cell(table.rows[row][column])) {
                continue;
            }
            double numeric_value = 0.0;
            const bool numeric = parse_finite_number(table.rows[row][column], numeric_value);
            has_valid = true;
            has_numeric = has_numeric || numeric;
            all_numeric = all_numeric && numeric;
            all_time_like = all_time_like && looks_like_time(table.rows[row][column]);
        }
        if (has_valid && (all_numeric || (has_numeric && !all_time_like))) {
            table.column_types[column] = ColumnType::numeric;
        } else if (has_valid && all_time_like) {
            table.column_types[column] = ColumnType::time;
        } else if (has_valid) {
            table.column_types[column] = ColumnType::categorical;
        }
        for (std::size_t row = 0; row < table.rows.size(); ++row) {
            if (column >= table.rows[row].size()
                || is_missing_cell(table.rows[row][column])) {
                table.cell_states[row][column] = CellState::missing;
            } else if (table.column_types[column] == ColumnType::numeric) {
                double numeric_value = 0.0;
                table.cell_states[row][column] =
                    parse_finite_number(table.rows[row][column], numeric_value)
                    ? CellState::valid : CellState::invalid;
            } else {
                table.cell_states[row][column] = CellState::valid;
            }
        }
    }
}

ExtractedNumericColumn extract_numeric_column(
    const DataTable& table,
    std::size_t column_index,
    const std::vector<std::size_t>& excluded_rows)
{
    ExtractedNumericColumn extracted;
    extracted.total_count = table.rows.size();
    if (column_index >= table.columns.size()) {
        extracted.column_valid = false;
        extracted.name = "C" + std::to_string(column_index + 1);
        extracted.missing_count = table.rows.size();
        return extracted;
    }
    extracted.name = table.columns[column_index];
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        if (is_excluded(excluded_rows, row_index)) {
            ++extracted.excluded_count;
            continue;
        }
        const auto& row = table.rows[row_index];
        if (column_index >= row.size() || is_missing_cell(row[column_index])) {
            ++extracted.missing_count;
            continue;
        }
        double value = 0.0;
        if (!parse_finite_number(row[column_index], value)) {
            ++extracted.invalid_count;
            continue;
        }
        extracted.values.push_back(value);
        extracted.source_rows.push_back(row_index);
    }
    return extracted;
}

std::vector<std::string> extract_text_column(
    const DataTable& table,
    std::size_t column_index)
{
    std::vector<std::string> values;
    values.reserve(table.rows.size());
    for (const auto& row : table.rows) {
        if (column_index < row.size()) {
            values.push_back(row[column_index]);
        } else {
            values.emplace_back();
        }
    }
    return values;
}

std::string column_label(const DataTable& table, std::size_t column_index)
{
    if (column_index >= table.columns.size()) {
        return "C" + std::to_string(column_index + 1);
    }
    return "C" + std::to_string(column_index + 1) + "  " + table.columns[column_index];
}

}  // namespace datalab::domain
