#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct AttributeSample {
    double defectives = 0.0;
    double inspected = 0.0;
    std::size_t source_row = 0;
};

struct ConfidenceInterval {
    std::optional<double> lower;
    std::optional<double> upper;
};

struct AttributeCapabilityResult {
    std::string method;
    std::size_t sample_count = 0;
    std::size_t missing_count = 0;
    double defectives_total = 0.0;
    double inspected_total = 0.0;
    std::optional<double> average_p;
    std::optional<double> percent_defective;
    std::optional<double> ppm_defective;
    std::optional<double> process_z;
    ConfidenceInterval average_p_interval;
    ConfidenceInterval percent_defective_interval;
    ConfidenceInterval ppm_interval;
    ConfidenceInterval process_z_interval;
    std::optional<double> mean_dpu;
    std::optional<double> mean_defective;
    std::optional<double> minimum_dpu;
    std::optional<double> maximum_dpu;
    ConfidenceInterval mean_dpu_interval;
    ConfidenceInterval mean_defective_interval;
    std::optional<double> target;
    std::vector<double> cumulative_values;
    std::vector<std::size_t> source_rows;
    std::vector<std::size_t> defectives;
    std::vector<std::size_t> inspected;
    std::string assumption_status = "not_verified";
    std::vector<DiagnosticMessage> diagnostics;
};

AttributeCapabilityResult binomial_capability(
    const std::vector<AttributeSample>& samples,
    std::size_t missing_count = 0,
    std::optional<double> target = std::nullopt,
    double confidence_level = 0.95);

AttributeCapabilityResult poisson_capability(
    const std::vector<AttributeSample>& samples,
    std::size_t missing_count = 0,
    std::optional<double> target = std::nullopt,
    double confidence_level = 0.95);

ConfidenceInterval clopper_pearson_interval(
    double defectives,
    double inspected,
    double alpha);

ConfidenceInterval garwood_rate(
    double defectives_total,
    double exposure,
    double alpha);

}  // namespace datalab::domain::statistics
