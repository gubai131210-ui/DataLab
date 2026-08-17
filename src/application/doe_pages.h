#pragma once

// DOE 页面装配（阶段 2.3 薄壳化）：doe_factorial 的响应分析页与设计矩阵页组装。
// 参照 chart_pages 模式：AnalysisService 只做校验与计算，页面骨架下沉到共享构建器。
// 输出与 AnalysisService::doe_factorial 原实现逐字一致（golden 测试兜底）。

#include "domain/quality_types.h"
#include "domain/statistics/doe_factorial.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::application {

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

}  // namespace datalab::application
