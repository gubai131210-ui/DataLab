#include "application/column_assembly.h"

#include <map>

namespace datalab::application {

std::optional<SubgroupInput> build_strict_subgroups(
    const domain::DataTable& table,
    const domain::ExtractedNumericColumn& extracted,
    const domain::AnalysisConfiguration& configuration,
    std::string& error)
{
    if (extracted.missing_count > 0) {
        error = "子组控制图要求测量列没有缺失或非法值；请先处理第 "
            + std::to_string(extracted.missing_count) + " 个无效单元格。";
        return std::nullopt;
    }
    SubgroupInput result;
    if (configuration.selection.subgroup_column.has_value()) {
        const std::vector<std::string> labels = domain::extract_text_column(
            table, *configuration.selection.subgroup_column);
        std::map<std::string, std::size_t> indices;
        for (std::size_t index = 0; index < extracted.values.size(); ++index) {
            const std::size_t row = extracted.source_rows[index];
            const std::string label = row < labels.size() ? labels[row] : std::string();
            if (domain::is_missing_cell(label)) {
                error = "子组标识列包含缺失标签，无法严格构造子组（原始行 "
                    + std::to_string(row + 1) + "）。";
                return std::nullopt;
            }
            auto [iterator, inserted] = indices.emplace(label, result.values.size());
            if (inserted) {
                result.values.emplace_back();
                result.source_rows.emplace_back();
                result.labels.push_back(label);
            }
            result.values[iterator->second].push_back(extracted.values[index]);
            result.source_rows[iterator->second].push_back(row);
        }
    } else {
        const int configured_size = configuration.subgroup_size.value_or(5);
        if (configured_size < 2) {
            error = "子组大小必须至少为 2。";
            return std::nullopt;
        }
        const std::size_t size = static_cast<std::size_t>(configured_size);
        if (extracted.values.size() % size != 0) {
            error = "测量值数量不能被子组大小整除，存在不完整尾部子组。";
            return std::nullopt;
        }
        for (std::size_t offset = 0; offset < extracted.values.size(); offset += size) {
            result.values.emplace_back(
                extracted.values.begin() + static_cast<std::ptrdiff_t>(offset),
                extracted.values.begin() + static_cast<std::ptrdiff_t>(offset + size));
            result.source_rows.emplace_back(
                extracted.source_rows.begin() + static_cast<std::ptrdiff_t>(offset),
                extracted.source_rows.begin() + static_cast<std::ptrdiff_t>(offset + size));
            result.labels.push_back(std::to_string(result.values.size()));
        }
    }
    if (result.values.empty()) {
        error = "无法构造有效子组。";
        return std::nullopt;
    }
    const std::size_t expected_size = result.values.front().size();
    for (const auto& subgroup : result.values) {
        if (subgroup.size() != expected_size || subgroup.size() < 2) {
            error = "各子组必须具有相同且至少为 2 的观测数。";
            return std::nullopt;
        }
    }
    return result;
}

std::size_t first_variable(const domain::AnalysisConfiguration& configuration)
{
    if (!configuration.variable_columns.empty()) {
        return configuration.variable_columns.front();
    }
    return configuration.selection.measurement_column;
}

std::vector<std::vector<double>> align_complete_rows(
    const std::vector<domain::ExtractedNumericColumn>& columns)
{
    std::vector<std::vector<double>> aligned;
    if (columns.empty()) {
        return aligned;
    }
    std::map<std::size_t, double> driver;
    for (std::size_t index = 0; index < columns[0].values.size(); ++index) {
        driver[columns[0].source_rows[index]] = columns[0].values[index];
    }
    std::vector<std::map<std::size_t, double>> others;
    others.reserve(columns.size() - 1);
    for (std::size_t column = 1; column < columns.size(); ++column) {
        std::map<std::size_t, double> by_row;
        for (std::size_t index = 0; index < columns[column].values.size(); ++index) {
            by_row[columns[column].source_rows[index]] = columns[column].values[index];
        }
        others.push_back(std::move(by_row));
    }
    for (const auto& [row, driver_value] : driver) {
        std::vector<double> row_values;
        row_values.reserve(columns.size());
        row_values.push_back(driver_value);
        bool complete = true;
        for (const auto& column : others) {
            const auto match = column.find(row);
            if (match == column.end()) {
                complete = false;
                break;
            }
            row_values.push_back(match->second);
        }
        if (complete) {
            aligned.push_back(std::move(row_values));
        }
    }
    return aligned;
}

}  // namespace datalab::application
