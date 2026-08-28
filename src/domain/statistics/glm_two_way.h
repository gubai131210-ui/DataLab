#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct GlmTwoWayOptions {
    bool include_interaction = true;
};

struct GlmFittedMean {
    std::string factor;
    std::string level;
    std::size_t count = 0;
    double fitted_mean = 0.0;
};

struct GlmCoefficient {
    std::string term;
    double coefficient = 0.0;
    std::optional<double> standard_error;
    std::optional<double> t_statistic;
    std::optional<double> p_value;
};

struct GlmAnovaEffect {
    std::string term;
    std::optional<double> adjusted_sum_of_squares;
    std::size_t degrees_of_freedom = 0;
    std::optional<double> mean_square;
    std::optional<double> f_statistic;
    std::optional<double> p_value;
    bool estimable = true;
};

struct GlmTwoWayResult {
    std::size_t observation_count = 0;
    std::size_t omitted_observation_count = 0;
    bool include_interaction = true;
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
    std::string algorithm_id = "glm_two_way_type3";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

// Unbalanced two-way GLM with Type III Adj SS and fitted means.
GlmTwoWayResult glm_two_way_analyze(
    const std::vector<std::string>& factor_a,
    const std::vector<std::string>& factor_b,
    const std::vector<double>& response,
    const std::vector<std::size_t>& source_rows = {},
    const GlmTwoWayOptions& options = {});

}  // namespace datalab::domain::statistics
