#include "domain/column_extract.h"

#include <algorithm>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <sstream>
#include <regex>
#include <string>
#include <unordered_set>

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

bool parse_time_cell_to_epoch(
    const std::string& cell,
    double& epoch_seconds,
    std::string& original_text)
{
    const std::string trimmed = trim_copy(cell);
    if (trimmed.empty() || is_missing_cell(trimmed)) {
        return false;
    }
    static const std::regex timestamp_pattern(
        R"(^([0-9]{4})([-/])([0-9]{1,2})\2([0-9]{1,2})(?:[ Tt]([0-9]{1,2}):([0-9]{2})(?::([0-9]{2})(\.[0-9]{1,9})?)?(?:([Zz])|([+-])([0-9]{2}):?([0-9]{2}))?)?$)");
    std::smatch fields;
    if (!std::regex_match(trimmed, fields, timestamp_pattern)) {
        return false;
    }
    const int year = std::stoi(fields[1].str());
    const int month = std::stoi(fields[3].str());
    const int day = std::stoi(fields[4].str());
    const int hour = fields[5].matched ? std::stoi(fields[5].str()) : 0;
    const int minute = fields[6].matched ? std::stoi(fields[6].str()) : 0;
    const int second = fields[7].matched ? std::stoi(fields[7].str()) : 0;
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    static const int month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (year < 1 || month < 1 || month > 12 || day < 1
        || day > month_days[month - 1] + (month == 2 && leap ? 1 : 0)
        || hour > 23 || minute > 59 || second > 59) {
        return false;
    }
    const double fraction = fields[8].matched ? std::stod(fields[8].str()) : 0.0;
    if (fields[9].matched || fields[10].matched) {
        const int offset_hour = fields[11].matched ? std::stoi(fields[11].str()) : 0;
        const int offset_minute = fields[12].matched ? std::stoi(fields[12].str()) : 0;
        if (offset_hour > 23 || offset_minute > 59) {
            return false;
        }
        const auto days_before_year = [](int y) {
            const int previous = y - 1;
            return 365LL * previous + previous / 4 - previous / 100 + previous / 400;
        };
        long long days = days_before_year(year) - days_before_year(1970) + day - 1;
        for (int m = 1; m < month; ++m) {
            days += month_days[m - 1] + (m == 2 && leap ? 1 : 0);
        }
        const int offset = (offset_hour * 60 + offset_minute) * 60
            * (fields[10].str() == "-" ? -1 : 1);
        epoch_seconds = static_cast<double>(days * 86400 + hour * 3600 + minute * 60 + second)
            - offset + fraction;
    } else {
        std::tm local{};
        local.tm_year = year - 1900;
        local.tm_mon = month - 1;
        local.tm_mday = day;
        local.tm_hour = hour;
        local.tm_min = minute;
        local.tm_sec = second;
        local.tm_isdst = -1;
        const std::time_t seconds = std::mktime(&local);
        if (seconds == static_cast<std::time_t>(-1) || local.tm_year != year - 1900
            || local.tm_mon != month - 1 || local.tm_mday != day || local.tm_hour != hour
            || local.tm_min != minute || local.tm_sec != second) {
            return false;
        }
        epoch_seconds = static_cast<double>(seconds) + fraction;
    }
    original_text = trimmed;
    return std::isfinite(epoch_seconds);
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

bool cell_looks_like_time(const std::string& value)
{
    return looks_like_time(value);
}

void populate_data_table_contract(DataTable& table)
{
    table.import_metadata.original_row_count = table.rows.size();
    table.import_metadata.column_count = table.columns.size();
    for (auto& row : table.rows) {
        row.resize(table.columns.size());
    }
    table.row_ids.resize(table.rows.size());
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        table.row_ids[row] = static_cast<RowId>(row);
    }

    std::ostringstream identity;
    identity << table.source_path << '|' << table.rows.size() << '|';
    for (const std::string& column : table.columns) {
        identity << column << ',';
    }
    table.import_metadata.dataset_id = std::to_string(std::hash<std::string>{}(identity.str()));

    refresh_data_table_contract(table);
}

void refresh_data_table_contract(
    DataTable& table, const std::vector<std::size_t>& changed_columns)
{
    table.import_metadata.original_row_count = table.rows.size();
    table.import_metadata.column_count = table.columns.size();
    const std::size_t old_id_count = table.row_ids.size();
    RowId next_id = 0;
    for (const RowId id : table.row_ids) {
        next_id = std::max(next_id, id + 1);
    }
    table.row_ids.resize(table.rows.size());
    for (std::size_t row = old_id_count; row < table.row_ids.size(); ++row) {
        table.row_ids[row] = next_id++;
    }
    table.column_types.resize(table.columns.size(), ColumnType::unknown);
    table.cell_states.resize(table.rows.size());
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        table.rows[row].resize(table.columns.size());
        table.cell_states[row].resize(table.columns.size(), CellState::missing);
    }
    std::vector<std::size_t> columns = changed_columns;
    if (columns.empty()) {
        columns.resize(table.columns.size());
        for (std::size_t column = 0; column < columns.size(); ++column) {
            columns[column] = column;
        }
    }
    for (const std::size_t column : columns) {
        if (column >= table.columns.size()) {
            continue;
        }
        table.column_types[column] = ColumnType::unknown;
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

std::string validate_data_table_contract(const DataTable& table)
{
    if (table.column_types.size() != table.columns.size()) {
        return "导入结果的列类型数量与列名不一致。";
    }
    if (table.row_ids.size() != table.rows.size()) {
        return "导入结果的 RowId 数量与数据行不一致。";
    }
    if (table.cell_states.size() != table.rows.size()) {
        return "导入结果的单元格状态行数与数据行不一致。";
    }
    if (table.import_metadata.original_row_count != table.rows.size()) {
        return "导入元数据中的原始行数与数据行不一致。";
    }
    if (table.import_metadata.column_count != table.columns.size()) {
        return "导入元数据中的列数与列名不一致。";
    }
    std::unordered_set<RowId> seen;
    seen.reserve(table.row_ids.size());
    for (std::size_t row = 0; row < table.rows.size(); ++row) {
        if (table.rows[row].size() != table.columns.size()) {
            return "第 " + std::to_string(row + 1) + " 行的字段数与列数不一致。";
        }
        if (table.cell_states[row].size() != table.columns.size()) {
            return "第 " + std::to_string(row + 1) + " 行的单元格状态数与列数不一致。";
        }
        if (!seen.insert(table.row_ids[row]).second) {
            return "导入结果包含重复的 RowId。";
        }
    }
    return {};
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
