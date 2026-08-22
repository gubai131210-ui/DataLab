#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/hypothesis_tests.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct GrubbsTestResult {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    double mean = 0.0;
    double sample_standard_deviation = 0.0;
    std::optional<double> g_statistic;
    std::optional<double> p_value;
    std::optional<double> outlier_value;
    std::optional<std::size_t> outlier_index;
    std::optional<std::size_t> source_row;
    std::string direction;
    std::string alternative = "two_sided";
    double alpha = 0.05;
    std::string assumption_status = "not_verified";
    std::vector<DiagnosticMessage> diagnostics;
};

GrubbsTestResult grubbs_outlier_test(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows = {},
    TestAlternative alternative = TestAlternative::two_sided,
    double alpha = 0.05,
    std::size_t missing_count = 0);

struct DixonTestResult {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    double mean = 0.0;
    double sample_standard_deviation = 0.0;
    std::optional<double> r_statistic;
    std::optional<double> critical_value;
    std::optional<double> p_value;
    std::optional<double> outlier_value;
    std::optional<std::size_t> outlier_index;
    std::optional<std::size_t> source_row;
    std::string direction;
    std::string alternative = "two_sided";
    std::string ratio = "r10";
    double alpha = 0.05;
    std::string assumption_status = "not_verified";
    std::vector<DiagnosticMessage> diagnostics;
};

// Dixon r10 (Dixon's Q). n in [3,30]; larger n → diagnostic.
DixonTestResult dixon_r10_outlier_test(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows = {},
    TestAlternative alternative = TestAlternative::two_sided,
    double alpha = 0.05,
    std::size_t missing_count = 0);

}  // namespace datalab::domain::statistics
