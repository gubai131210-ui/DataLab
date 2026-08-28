#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/glm_two_way.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct GlmThreeFactorOptions {
    bool include_ab_interaction = true;
    bool include_ac_interaction = true;
    bool include_bc_interaction = true;
};

struct GlmThreeFactorResult {
    std::size_t observation_count = 0;
    std::size_t omitted_observation_count = 0;
    bool include_ab_interaction = true;
    bool include_ac_interaction = true;
    bool include_bc_interaction = true;
    bool design_balanced = true;
    double error_sum_of_squares = 0.0;
    std::size_t error_degrees_of_freedom = 0;
    double error_mean_square = 0.0;
    std::vector<GlmAnovaEffect> anova_effects;
    std::vector<GlmCoefficient> coefficients;
    std::vector<GlmFittedMean> fitted_means;
    std::vector<double> residuals;
    std::vector<double> fitted;
    std::vector<std::size_t> observation_source_rows;
    std::optional<double> residual_normality_p;
    std::string algorithm_id = "glm_three_factor_type3";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

GlmThreeFactorResult glm_three_factor_analyze(
    const std::vector<std::string>& factor_a,
    const std::vector<std::string>& factor_b,
    const std::vector<std::string>& factor_c,
    const std::vector<double>& response,
    const std::vector<std::size_t>& source_rows = {},
    const GlmThreeFactorOptions& options = {});

}  // namespace datalab::domain::statistics
