#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct MixedEffectsRemlOptions {
    std::string reml_method = "newton";
};

struct MixedEffectsVarianceRow {
    std::string source;
    double variance_component = 0.0;
    double percent_contribution = 0.0;
};

struct MixedEffectsFixedRow {
    std::string term;
    double coefficient = 0.0;
    double standard_error = 0.0;
    double t_statistic = 0.0;
    double p_value = 0.0;
};

struct MixedEffectsRemlResult {
    std::size_t observation_count = 0;
    std::size_t random_level_count = 0;
    bool converged = false;
    double residual_variance = 0.0;
    double random_variance = 0.0;
    std::vector<MixedEffectsVarianceRow> variance_components;
    std::vector<MixedEffectsFixedRow> fixed_effects;
    std::vector<std::size_t> observation_source_rows;
    std::string algorithm_id = "mixed_effects_reml_variance";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

MixedEffectsRemlResult mixed_effects_reml_analyze(
    const std::vector<double>& response,
    const std::vector<std::string>& random_factor,
    const std::vector<std::string>& fixed_a = {},
    const std::vector<std::string>& fixed_b = {},
    const std::vector<double>& covariate = {},
    const std::vector<std::size_t>& source_rows = {},
    const MixedEffectsRemlOptions& options = {});

}  // namespace datalab::domain::statistics
