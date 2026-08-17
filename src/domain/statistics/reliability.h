#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
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
};
struct KaplanMeierResult {
    std::vector<KaplanMeierPoint> points;
    std::optional<double> median_life;
    double confidence_level = 0.95;
    double censoring_fraction = 0.0;
    bool survival_identifiable = false;
    std::vector<DiagnosticMessage> diagnostics;
};
KaplanMeierResult kaplan_meier(const std::vector<double>& times,
                               const std::vector<bool>& events,
                               double confidence_level = 0.95);

struct LogRankResult {
    double chi_square = 0.0;
    double degrees_of_freedom = 1.0;
    double p_value = 1.0;
    std::size_t group_one_failures = 0;
    std::size_t group_two_failures = 0;
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
    bool identifiable = false;
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

}  // namespace datalab::domain::statistics
