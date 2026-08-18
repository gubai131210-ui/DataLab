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

// 按源行对齐若干数值列（complete-case）：仅保留所有列在该源行都有值的行，
// 输出按第一列源行升序；aligned[i][j] = 第 i 个对齐观测的第 j 列值（行主序）。
// 语义与原 paired_t/regression 内联实现逐字一致。
std::vector<std::vector<double>> align_complete_rows(
    const std::vector<domain::ExtractedNumericColumn>& columns);

struct AlignedNumericRows {
    std::vector<std::vector<double>> values;
    std::vector<std::size_t> source_rows;
};

AlignedNumericRows align_complete_rows_with_source(
    const std::vector<domain::ExtractedNumericColumn>& columns);

}  // namespace datalab::application
