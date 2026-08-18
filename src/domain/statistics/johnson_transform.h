#pragma once

#include "domain/quality_types.h"

#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class JohnsonFamily {
    none,
    sb,
    sl,
    su
};

struct JohnsonParameters {
    JohnsonFamily family = JohnsonFamily::none;
    double gamma = 0.0;
    double eta = 0.0;
    double epsilon = 0.0;
    double lambda = 1.0;
};

struct JohnsonTransformResult {
    bool found = false;
    JohnsonParameters parameters;
    double p_value = 0.0;
    double anderson_darling = 0.0;
    double p_criterion = 0.10;
    std::vector<double> transformed;
    std::vector<DiagnosticMessage> diagnostics;
};

std::string johnson_family_name(JohnsonFamily family);

std::optional<double> johnson_transform_value(
    const JohnsonParameters& parameters,
    double value);

std::optional<double> johnson_inverse_value(
    const JohnsonParameters& parameters,
    double z);

JohnsonTransformResult fit_johnson_transform(
    const std::vector<double>& observations,
    double p_criterion = 0.10);

}  // namespace datalab::domain::statistics
