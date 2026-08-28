#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class MixtureModelOrder {
    linear,
    quadratic
};

struct MixtureAnalyzeOptions {
    MixtureModelOrder model_order = MixtureModelOrder::linear;
    double sum_tolerance = 0.05;
};

struct MixtureCoefficient {
    std::string term;
    double coefficient = 0.0;
    double standard_error = 0.0;
    std::optional<double> t_statistic;
    std::optional<double> p_value;
};

struct MixtureAnovaEffect {
    std::string term;
    std::optional<double> sequential_sum_of_squares;
    std::optional<double> adjusted_sum_of_squares;
    std::size_t degrees_of_freedom = 0;
    std::optional<double> mean_square;
    std::optional<double> f_statistic;
    std::optional<double> p_value;
};

struct MixtureFitRow {
    std::size_t source_row = 0;
    double observed = 0.0;
    double fitted = 0.0;
    double residual = 0.0;
};

struct MixtureAnalyzeResult {
    std::size_t component_count = 0;
    std::size_t observation_count = 0;
    std::string model_order = "linear";
    double r_squared = 0.0;
    double adjusted_r_squared = 0.0;
    std::vector<std::string> component_names;
    std::vector<MixtureCoefficient> coefficients;
    std::vector<MixtureAnovaEffect> anova_effects;
    std::vector<MixtureFitRow> fits;
    std::string algorithm_id = "mixture_scheffe_ols";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

MixtureModelOrder parse_mixture_model_order(const std::string& text);
std::string mixture_model_order_name(MixtureModelOrder order);

// Scheffé OLS without intercept: b = (X'X)^-1 X'y.
// components[obs][component], response[obs].
MixtureAnalyzeResult analyze_mixture_scheffe(
    const std::vector<std::vector<double>>& components,
    const std::vector<double>& response,
    const std::vector<std::string>& component_names,
    const std::vector<std::size_t>& source_rows = {},
    const MixtureAnalyzeOptions& options = {});

}  // namespace datalab::domain::statistics
