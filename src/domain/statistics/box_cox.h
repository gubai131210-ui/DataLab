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

// Classic power transform used for dual-scale reporting (not geometric-mean scaled).
double box_cox_apply(double value, double lambda);

// Transform a specification limit; returns nullopt for non-positive / nonfinite.
std::optional<double> box_cox_transform_limit(double limit, double lambda);

// True when transformed LSL/USL preserve strict order (Box-Cox/log strictly increasing on (0,∞)).
bool box_cox_limits_order_ok(
    double original_lsl,
    double original_usl,
    double lambda);

}  // namespace datalab::domain::statistics
