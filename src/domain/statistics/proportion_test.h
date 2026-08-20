#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/hypothesis_tests.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class ProportionMethod {
    exact,
    normal,
    wilson,
    agresti_coull
};

struct OneProportionResult {
    std::size_t events = 0;
    std::size_t trials = 0;
    std::size_t row_count = 0;
    std::size_t missing_count = 0;
    double proportion = 0.0;
    double hypothesized = 0.0;
    std::string method;
    std::string ci_method;
    std::optional<double> z_statistic;
    std::optional<double> p_value;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    double confidence_level = 0.95;
    TestAlternative alternative = TestAlternative::two_sided;
    std::vector<DiagnosticMessage> diagnostics;
};

OneProportionResult one_proportion_test(
    std::size_t events,
    std::size_t trials,
    double hypothesized,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided,
    ProportionMethod method = ProportionMethod::exact,
    std::size_t row_count = 1,
    std::size_t missing_count = 0);

}  // namespace datalab::domain::statistics
