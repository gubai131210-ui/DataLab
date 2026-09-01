#pragma once

#include "domain/statistics/mixture_analyze.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct MixtureProcessVariableOptions {
    MixtureModelOrder component_order = MixtureModelOrder::linear;
    bool include_component_process_interaction = true;
    double sum_tolerance = 0.05;
};

struct MixtureProcessVariableResult {
    std::size_t component_count = 0;
    std::size_t observation_count = 0;
    std::string component_order = "linear";
    bool include_component_process_interaction = true;
    double r_squared = 0.0;
    double adjusted_r_squared = 0.0;
    std::vector<std::string> component_names;
    std::vector<MixtureCoefficient> coefficients;
    std::vector<MixtureAnovaEffect> anova_effects;
    std::vector<MixtureFitRow> fits;
    std::string algorithm_id = "mixture_process_variable_scheffe_ols";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

MixtureProcessVariableResult analyze_mixture_process_variable(
    const std::vector<std::vector<double>>& components,
    const std::vector<double>& process_variable,
    const std::vector<double>& response,
    const std::vector<std::string>& component_names,
    const std::vector<std::size_t>& source_rows = {},
    const MixtureProcessVariableOptions& options = {});

}  // namespace datalab::domain::statistics
