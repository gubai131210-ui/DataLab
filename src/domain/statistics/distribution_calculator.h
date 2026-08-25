#pragma once

#include "domain/quality_types.h"

#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class DistCalcDistribution {
    normal,
    student_t,
    chi_square,
    f,
    weibull
};

enum class DistCalcOperation {
    pdf,
    cdf,
    quantile
};

struct DistributionCalculatorOptions {
    DistCalcDistribution distribution = DistCalcDistribution::normal;
    DistCalcOperation operation = DistCalcOperation::cdf;
    // normal: mean, sd; t/chi2: df; F: df1, df2; Weibull: shape, scale
    double param1 = 0.0;
    double param2 = 1.0;
    double param3 = 1.0;  // F df2 or unused
    double value = 0.0;   // x for pdf/cdf, p for quantile
};

struct DistributionCalculatorResult {
    std::string distribution;
    std::string operation;
    double param1 = 0.0;
    double param2 = 1.0;
    double param3 = 1.0;
    double value = 0.0;
    std::optional<double> result;
    std::string algorithm_id = "distribution_calculator_reuse";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

DistCalcDistribution parse_distcalc_distribution(const std::string& text);
DistCalcOperation parse_distcalc_operation(const std::string& text);
std::string distcalc_distribution_name(DistCalcDistribution d);
std::string distcalc_operation_name(DistCalcOperation op);

DistributionCalculatorResult evaluate_distribution_calculator(
    const DistributionCalculatorOptions& options);

}  // namespace datalab::domain::statistics
