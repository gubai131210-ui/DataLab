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
    QualityEvidence evidence;
    std::vector<RuleEvidence> rules;
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
    QualityEvidence evidence;
    std::vector<RuleEvidence> rules;
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
    std::optional<double> threshold;
    std::string not_computed_reason;
    QualityEvidence evidence;
    std::vector<RuleEvidence> rules;
    std::vector<DiagnosticMessage> diagnostics;
};
WeibullResult fit_weibull(const std::vector<double>& times,
                          const std::vector<bool>& events);
WeibullResult fit_weibull3(const std::vector<double>& times,
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
    std::optional<double> threshold;
    std::size_t failures = 0;
    std::size_t observations = 0;
    bool identifiable = false;
    bool converged = false;
    std::string not_computed_reason;
    QualityEvidence evidence;
    std::vector<RuleEvidence> rules;
    std::vector<DiagnosticMessage> diagnostics;
};
ExponentialResult fit_exponential(const std::vector<double>& times,
                                  const std::vector<bool>& events);
ExponentialResult fit_exponential2(const std::vector<double>& times,
                                   const std::vector<bool>& events);

struct LognormalResult {
    double location = 0.0;
    double scale = 0.0;
    double log_likelihood = 0.0;
    double aic = 0.0;
    double bic = 0.0;
    std::optional<double> b10;
    std::optional<double> b50;
    std::optional<double> b90;
    std::optional<double> median_life;
    std::optional<double> threshold;
    std::size_t failures = 0;
    std::size_t observations = 0;
    double censoring_fraction = 0.0;
    bool identifiable = false;
    bool converged = false;
    int iterations = 0;
    std::string not_computed_reason;
    QualityEvidence evidence;
    std::vector<RuleEvidence> rules;
    std::vector<DiagnosticMessage> diagnostics;
};
LognormalResult fit_lognormal(const std::vector<double>& times,
                              const std::vector<bool>& events);
LognormalResult fit_lognormal3(const std::vector<double>& times,
                               const std::vector<bool>& events);

struct ParametricDistributionCandidate {
    std::string name;
    double aic = 0.0;
    double bic = 0.0;
    bool converged = false;
    std::vector<DiagnosticMessage> diagnostics;
};

double percentile_life_weibull(double shape, double scale, double percentile);
double percentile_life_weibull3(double shape, double scale, double threshold,
                                double percentile);
double percentile_life_exponential(double rate, double percentile);
double percentile_life_exponential2(double rate, double threshold, double percentile);
double percentile_life_lognormal(double location, double scale, double percentile);
double percentile_life_lognormal3(double location, double scale, double threshold,
                                  double percentile);
std::optional<double> percentile_life_km(
    const std::vector<KaplanMeierPoint>& points,
    double percentile);

double cdf_weibull3(double time, double shape, double scale, double threshold);
double cdf_exponential2(double time, double rate, double threshold);
double cdf_lognormal3(double time, double location, double scale, double threshold);

std::vector<ParametricDistributionCandidate> compare_parametric_distributions(
    const std::vector<double>& times,
    const std::vector<bool>& events);

std::optional<bool> parse_reliability_event(const std::string& text);

}  // namespace datalab::domain::statistics
