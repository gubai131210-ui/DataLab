#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/normal_probability.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct NormalityTestResult {
    std::size_t count = 0;
    std::size_t missing_count = 0;
    double mean = 0.0;
    double sample_standard_deviation = 0.0;
    double alpha = 0.05;
    std::optional<double> anderson_darling;
    std::optional<double> adjusted_anderson_darling;
    std::optional<double> p_value;
    std::string decision = "not_computed";
    bool sample_size_warning = false;
    QualityEvidence evidence;
    std::vector<std::string> diagnostics;
    std::vector<DiagnosticMessage> messages;
    NormalProbabilityResult probability_plot;
};

NormalityTestResult normality_test(const std::vector<double>& observations);

NormalityTestResult normality_test(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows);

}  // namespace datalab::domain::statistics
