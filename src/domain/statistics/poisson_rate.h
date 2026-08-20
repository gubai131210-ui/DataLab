#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/proportion_test.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct OnePoissonRateResult {
    std::size_t events = 0;
    double exposure = 0.0;
    std::size_t row_count = 0;
    std::size_t missing_count = 0;
    double rate = 0.0;
    double hypothesized = 0.0;
    std::string method;
    std::optional<double> z_statistic;
    std::optional<double> p_value;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    double confidence_level = 0.95;
    TestAlternative alternative = TestAlternative::two_sided;
    std::vector<DiagnosticMessage> diagnostics;
};

struct TwoPoissonRateResult {
    std::size_t first_events = 0;
    double first_exposure = 0.0;
    double first_rate = 0.0;
    std::size_t second_events = 0;
    double second_exposure = 0.0;
    double second_rate = 0.0;
    double difference = 0.0;
    std::optional<double> ratio;
    std::string comparison = "difference";
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

OnePoissonRateResult one_poisson_rate_test(
    std::size_t events,
    double exposure,
    double hypothesized,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided,
    ProportionMethod method = ProportionMethod::exact,
    std::size_t row_count = 1,
    std::size_t missing_count = 0);

TwoPoissonRateResult two_poisson_rate_test(
    std::size_t first_events,
    double first_exposure,
    std::size_t second_events,
    double second_exposure,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided,
    ProportionMethod method = ProportionMethod::exact,
    const std::string& comparison = "difference");

}  // namespace datalab::domain::statistics
