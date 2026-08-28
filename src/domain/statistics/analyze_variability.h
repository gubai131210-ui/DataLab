#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct AnalyzeVariabilityOptions {
    std::string estimation_method = "lse";  // lse only in this wave
};

struct VariabilityRunRow {
    std::size_t source_row = 0;
    double std_dev = 0.0;
    double log_std_dev = 0.0;
    std::size_t replicate_count = 0;
};

struct DispersionCoefficient {
    std::string term;
    double coefficient = 0.0;
    double effect = 0.0;  // 2 * coef for 2-level
    std::optional<double> standard_error;
};

struct DispersionAnovaEffect {
    std::string term;
    std::optional<double> sum_of_squares;
    std::size_t degrees_of_freedom = 0;
    std::optional<double> mean_square;
    std::optional<double> f_statistic;
    std::optional<double> p_value;
};

struct AnalyzeVariabilityResult {
    std::size_t run_count = 0;
    std::size_t factor_count = 0;
    std::size_t replicate_count = 0;
    std::string estimation_method = "lse";
    std::vector<std::string> factor_names;
    std::vector<VariabilityRunRow> runs;
    std::vector<DispersionCoefficient> coefficients;
    std::vector<DispersionAnovaEffect> anova_effects;
    std::string algorithm_id = "analyze_variability_ln_sigma_lse";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

// Per-row std dev from replicate columns; fit ln(s) ~ ±1 coded factors; effect=2*coef.
AnalyzeVariabilityResult analyze_variability_dispersion(
    const std::vector<std::vector<std::string>>& factor_levels,
    const std::vector<std::vector<double>>& replicates,
    const std::vector<std::string>& factor_names,
    const std::vector<std::size_t>& source_rows = {},
    const AnalyzeVariabilityOptions& options = {});

}  // namespace datalab::domain::statistics
