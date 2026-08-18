#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct KaplanMeierPoint {
    double time = 0.0;
    std::size_t at_risk = 0;
    std::size_t failures = 0;
    std::size_t censored = 0;
    double survival = 1.0;
    double standard_error = 0.0;
    double confidence_lower = 1.0;
    double confidence_upper = 1.0;
    std::vector<std::size_t> source_rows;
};
struct KaplanMeierResult {
    std::vector<KaplanMeierPoint> points;
    std::optional<double> median_life;
    double confidence_level = 0.95;
    double censoring_fraction = 0.0;
    std::size_t failure_count = 0;
    std::size_t censored_count = 0;
    std::size_t valid_count = 0;
    bool survival_identifiable = false;
    std::string not_computed_reason;
    std::vector<DiagnosticMessage> diagnostics;
};
KaplanMeierResult kaplan_meier(const std::vector<double>& times,
                               const std::vector<bool>& events,
                               double confidence_level = 0.95,
                               const std::vector<std::size_t>& source_rows = {});

struct LogRankResult {
    double chi_square = 0.0;
    double degrees_of_freedom = 1.0;
    double p_value = 1.0;
    std::size_t group_one_n = 0;
    std::size_t group_two_n = 0;
    std::size_t group_one_failures = 0;
    std::size_t group_two_failures = 0;
    std::size_t group_one_censored = 0;
    std::size_t group_two_censored = 0;
    std::vector<DiagnosticMessage> diagnostics;
};

LogRankResult log_rank_test(const std::vector<double>& times,
                            const std::vector<bool>& events,
                            const std::vector<int>& groups);

struct WeibullResult {
    double shape = 0.0;
    double scale = 0.0;
    double log_likelihood = 0.0;
    double aic = 0.0;
    double bic = 0.0;
    std::optional<double> b10;
    std::optional<double> median_life;
    std::optional<double> b50;
    std::optional<double> b90;
    std::size_t failures = 0;
    std::size_t observations = 0;
    double censoring_fraction = 0.0;
    bool identifiable = false;
    bool converged = false;
    int iterations = 0;
    bool parameter_boundary_hit = false;
    std::string not_computed_reason;
    std::vector<DiagnosticMessage> diagnostics;
};
WeibullResult fit_weibull(const std::vector<double>& times,
                          const std::vector<bool>& events);

struct ExponentialResult {
    double rate = 0.0;
    double mean_life = 0.0;
    double log_likelihood = 0.0;
    double aic = 0.0;
    double bic = 0.0;
    std::optional<double> b10;
    std::optional<double> b50;
    std::optional<double> b90;
    std::size_t failures = 0;
    std::size_t observations = 0;
    bool identifiable = false;
    std::vector<DiagnosticMessage> diagnostics;
};
ExponentialResult fit_exponential(const std::vector<double>& times,
                                  const std::vector<bool>& events);

std::optional<bool> parse_reliability_event(const std::string& text);

}  // namespace datalab::domain::statistics
