#pragma once

// DOE 页面装配（阶段 2.3 薄壳化）：doe_factorial 的响应分析页与设计矩阵页组装。
// 参照 chart_pages 模式：AnalysisService 只做校验与计算，页面骨架下沉到共享构建器。
// 输出与 AnalysisService::doe_factorial 原实现逐字一致（golden 测试兜底）。

#include "domain/quality_types.h"
#include "domain/statistics/doe_factorial.h"
#include "domain/statistics/response_surface_design.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::application {

// Factorial worksheet re-import (PointType / midpoint-aware center runs).
struct ImportedFactorialRuns {
    domain::statistics::DoeFactorialDesign design;
    std::vector<std::size_t> source_rows;
    std::size_t skipped_level_rows = 0;
    std::size_t center_run_count = 0;
};

ImportedFactorialRuns import_factorial_runs_from_worksheet(
    const domain::DataTable& table,
    const domain::AnalysisConfiguration& configuration);

// 组装 DOE 响应分析页（系数与效应 / ANOVA 三表 / 残差诊断 / 主效应与交互作用图）。
domain::OutputPage doe_response_page(
    const domain::DataTable& table,
    const domain::AnalysisConfiguration& configuration,
    const domain::statistics::DoeFactorialDesign& design,
    const std::vector<double>& responses,
    const domain::statistics::DoeResponseAnalysisResult& fit);

// 组装 2 水平全因子设计矩阵页。
domain::OutputPage doe_design_page(
    const domain::AnalysisConfiguration& configuration,
    const std::vector<domain::statistics::DoeFactor>& factors,
    const domain::statistics::DoeFactorialDesign& design);

// Phase 4: CCD / BBD design matrix page (coded + actual + point labels).
domain::OutputPage response_surface_design_page(
    const domain::AnalysisConfiguration& configuration,
    const domain::statistics::ResponseSurfaceDesign& design);

// Convert a generated response-surface design into a fillable DataTable worksheet.
// Columns: RunID, StdOrder, RunOrder, Block, PointType, factor actuals…, Response (empty).
domain::DataTable response_surface_design_to_worksheet(
    const domain::statistics::ResponseSurfaceDesign& design,
    const std::string& design_source_id);

// Convert a generated 2-level factorial/fractional design into a fillable worksheet.
// Columns: RunID, StdOrder, RunOrder, Block, PointType, factor actuals…, Response (empty).
domain::DataTable factorial_design_to_worksheet(
    const domain::statistics::DoeFactorialDesign& design,
    const std::string& design_source_id = {});

}  // namespace datalab::application
