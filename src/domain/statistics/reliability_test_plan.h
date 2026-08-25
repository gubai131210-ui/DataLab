#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct ReliabilityTestPlanOptions {
    double shape_beta = 1.0;       // assumed Weibull β
    double target_reliability = 0.9;  // R at mission time
    double confidence_level = 0.9;    // CL
    double test_time = 1.0;           // T0
    double mission_time = 1.0;        // tm
    std::size_t allowed_failures = 0; // r
};

struct ReliabilityTestPlanResult {
    double shape_beta = 1.0;
    double target_reliability = 0.9;
    double confidence_level = 0.9;
    double test_time = 1.0;
    double mission_time = 1.0;
    double time_ratio_delta = 1.0;  // δ = (T0/tm)^β
    double test_reliability = 0.9;  // R_test = R^δ
    std::size_t allowed_failures = 0;
    std::optional<std::size_t> sample_size;  // n
    std::string algorithm_id = "reliability_demo_test_plan_weibull";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

// Zero-failure: n = ceil(ln(1-CL)/(δ ln R)); optional binomial search for r > 0.
ReliabilityTestPlanResult plan_reliability_demonstration(
    const ReliabilityTestPlanOptions& options);

}  // namespace datalab::domain::statistics
