#pragma once

#include "domain/column_extract.h"
#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::application {

struct SubgroupInput {
    std::vector<std::vector<double>> values;
    std::vector<std::vector<std::size_t>> source_rows;
    std::vector<std::string> labels;
};

// 严格子组构造：按子组标识列分组，或按固定大小顺序切分。
// 任一子组不完整（大小不一致、含缺失/非法值）即返回 nullopt 并给出中文错误。
std::optional<SubgroupInput> build_strict_subgroups(
    const domain::DataTable& table,
    const domain::ExtractedNumericColumn& extracted,
    const domain::AnalysisConfiguration& configuration,
    std::string& error);

// 取分析使用的第一个变量列：优先 variable_columns，否则 measurement_column。
std::size_t first_variable(const domain::AnalysisConfiguration& configuration);

}  // namespace datalab::application
