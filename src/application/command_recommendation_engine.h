#pragma once

// G6 命令 Wizard：按列类型 + 意图推荐 analysis_commands id（纯逻辑，无 Qt）。
// 不调用 AnalysisService；不改 domain 统计公式。

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::application {

enum class CommandWizardIntent {
    any,
    describe,
    compare,
    associate,
    control_chart,
    capability,
    reliability,
    graph
};

struct Recommendation {
    std::string command_id;
    double score = 0.0;
    std::string reason_key;
};

struct RecommendResult {
    std::vector<Recommendation> recommendations;
    std::optional<std::string> hint_key;
};

// G6_ENGINE：列类型 + 意图 → Top-N 推荐（默认 ≤8）。
// column_names 可选（本 Wave 不作列名启发；预留扩展）。
RecommendResult recommend(
    const std::vector<datalab::domain::ColumnType>& column_types,
    CommandWizardIntent intent,
    std::size_t top_n = 8,
    const std::vector<std::string>& column_names = {});

}  // namespace datalab::application
