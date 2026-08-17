#pragma once

#include "domain/quality_types.h"

#include <optional>
#include <vector>

namespace datalab::domain::statistics {

struct BoxCoxResult {
    double lambda = 0.0;
    double transformed_standard_deviation = 0.0;
    std::vector<double> lambdas;
    std::vector<double> standard_deviations;
    std::vector<double> transformed_values;
    std::vector<DiagnosticMessage> diagnostics;
};

BoxCoxResult box_cox_transform(
    const std::vector<double>& observations,
    std::optional<double> requested_lambda = std::nullopt,
    bool round_interpretable_lambda = true);

}  // namespace datalab::domain::statistics
