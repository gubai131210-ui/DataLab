#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct ToleranceIntervalResult {
    std::size_t valid_count = 0;
    std::size_t missing_count = 0;
    double mean = 0.0;
    double sample_standard_deviation = 0.0;
    double coverage = 0.95;
    double confidence_level = 0.95;
    std::string interval_type = "two_sided";
    std::string method;
    std::optional<double> k_factor;
    std::optional<double> achieved_confidence;
    std::optional<double> lower;
    std::optional<double> upper;
    std::string method_family = "normal";
    std::vector<std::size_t> source_rows;
    std::vector<DiagnosticMessage> diagnostics;
    std::string assumption_status = "not_verified";
};

ToleranceIntervalResult normal_tolerance_interval(
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows = {},
    double coverage = 0.95,
    double confidence_level = 0.95,
    const std::string& interval_type = "two_sided");

ToleranceIntervalResult nonparametric_tolerance_interval(
    const std::vector<double>& values,
    const std::vector<std::size_t>& source_rows = {},
    double coverage = 0.95,
    double confidence_level = 0.95,
    const std::string& interval_type = "two_sided");

}  // namespace datalab::domain::statistics
