#include "domain/statistics/distribution_identification.h"

#include "domain/quality_diagnostics.h"
#include "domain/statistics/anderson_darling.h"
#include "domain/statistics/descriptive_statistics.h"
#include "domain/statistics/normal_distribution.h"
#include "domain/statistics/reliability.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <vector>

namespace datalab::domain::statistics {
namespace {

void note(DistributionIdentificationResult& result,
          const char* code,
          const char* message)
{
    add_warning(result.diagnostics, code, message);
}

std::vector<std::size_t> default_source_rows(std::size_t count)
{
    std::vector<std::size_t> rows(count);
    std::iota(rows.begin(), rows.end(), 0);
    return rows;
}

std::vector<double> sorted_positive_values(
    const std::vector<double>& observations,
    std::vector<std::size_t>* source_rows_out)
{
    std::vector<std::pair<double, std::size_t>> pairs;
    pairs.reserve(observations.size());
    for (std::size_t index = 0; index < observations.size(); ++index) {
        if (std::isfinite(observations[index]) && observations[index] > 0.0) {
            pairs.emplace_back(observations[index], (*source_rows_out)[index]);
        }
    }
    std::stable_sort(pairs.begin(), pairs.end(),
                     [](const auto& left, const auto& right) {
                         return left.first < right.first;
                     });
    std::vector<double> values;
    values.reserve(pairs.size());
    source_rows_out->clear();
    source_rows_out->reserve(pairs.size());
    for (const auto& item : pairs) {
        values.push_back(item.first);
        source_rows_out->push_back(item.second);
    }
    return values;
}

NormalProbabilityResult probability_plot_with_inverse(
    const std::vector<double>& ordered_values,
    const std::vector<std::size_t>& source_rows,
    const std::function<double(double)>& inverse_cdf)
{
    NormalProbabilityResult plot;
    plot.ordered_values = ordered_values;
    plot.source_rows = source_rows;
    const std::size_t n = ordered_values.size();
    plot.theoretical_quantiles.reserve(n);
    for (std::size_t index = 0; index < n; ++index) {
        const double probability = (static_cast<double>(index) + 0.625)
            / (static_cast<double>(n) + 0.25);
        plot.theoretical_quantiles.push_back(inverse_cdf(probability));
    }
    return plot;
}

std::vector<double> normal_cdf_values(
    const std::vector<double>& ordered,
    double mean,
    double standard_deviation)
{
    std::vector<double> cdf;
    cdf.reserve(ordered.size());
    for (const double value : ordered) {
        cdf.push_back(standard_normal_cdf((value - mean) / standard_deviation));
    }
    return cdf;
}

DistributionIdentificationCandidate make_normal_candidate(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows,
    double alpha)
{
    DistributionIdentificationCandidate candidate;
    candidate.distribution = "Normal";
    const auto summary = DescriptiveStatistics::calculate(observations);
    if (!summary.has_value() || !summary->sample_standard_deviation.has_value()
        || observations.size() < 3
        || !(*summary->sample_standard_deviation > 0.0)) {
        candidate.status = "not_computed";
        return candidate;
    }
    candidate.location = summary->mean;
    candidate.scale = *summary->sample_standard_deviation;
    std::vector<double> ordered = observations;
    std::sort(ordered.begin(), ordered.end());
    const auto ad = anderson_darling_from_sorted_cdf(
        normal_cdf_values(ordered, *candidate.location, *candidate.scale));
    candidate.anderson_darling = ad.statistic;
    candidate.adjusted_anderson_darling = ad.adjusted;
    if (ad.adjusted.has_value()) {
        candidate.p_value = std::clamp(
            anderson_darling_p_value_normal(*ad.adjusted), 0.0, 1.0);
        candidate.decision = *candidate.p_value < alpha ? "reject" : "fail_to_reject";
    }
    candidate.status = "computed";
    candidate.probability_plot = normal_probability_plot(observations, source_rows);
    return candidate;
}

DistributionIdentificationCandidate make_lognormal_candidate(
    const std::vector<double>& positive,
    const std::vector<std::size_t>& positive_rows,
    double alpha)
{
    DistributionIdentificationCandidate candidate;
    candidate.distribution = "Lognormal";
    if (positive.size() < 3) {
        candidate.status = "not_computed";
        return candidate;
    }
    std::vector<double> logged;
    logged.reserve(positive.size());
    for (const double value : positive) {
        logged.push_back(std::log(value));
    }
    const auto summary = DescriptiveStatistics::calculate(logged);
    if (!summary.has_value() || !summary->sample_standard_deviation.has_value()
        || !(*summary->sample_standard_deviation > 0.0)) {
        candidate.status = "not_computed";
        return candidate;
    }
    candidate.location = summary->mean;
    candidate.scale = *summary->sample_standard_deviation;
    std::vector<double> ordered = positive;
    std::sort(ordered.begin(), ordered.end());
    std::vector<double> cdf;
    cdf.reserve(ordered.size());
    for (const double value : ordered) {
        cdf.push_back(standard_normal_cdf(
            (std::log(value) - *candidate.location) / *candidate.scale));
    }
    const auto ad = anderson_darling_from_sorted_cdf(cdf);
    candidate.anderson_darling = ad.statistic;
    candidate.adjusted_anderson_darling = ad.adjusted;
    if (ad.adjusted.has_value()) {
        candidate.p_value = std::clamp(
            anderson_darling_p_value_normal(*ad.adjusted), 0.0, 1.0);
        candidate.decision = *candidate.p_value < alpha ? "reject" : "fail_to_reject";
    }
    candidate.status = "computed";
    candidate.probability_plot = probability_plot_with_inverse(
        positive, positive_rows,
        [&](double probability) {
            return std::exp(*candidate.location
                + *candidate.scale * standard_normal_quantile(probability));
        });
    return candidate;
}

DistributionIdentificationCandidate make_exponential_candidate(
    const std::vector<double>& positive,
    const std::vector<std::size_t>& positive_rows)
{
    DistributionIdentificationCandidate candidate;
    candidate.distribution = "Exponential";
    if (positive.empty()) {
        candidate.status = "not_computed";
        return candidate;
    }
    const std::vector<bool> events(positive.size(), true);
    const ExponentialResult fitted = fit_exponential(positive, events);
    if (!fitted.identifiable || !fitted.converged || !(fitted.mean_life > 0.0)) {
        candidate.status = "not_computed";
        return candidate;
    }
    candidate.scale = fitted.mean_life;
    std::vector<double> ordered = positive;
    std::sort(ordered.begin(), ordered.end());
    std::vector<double> cdf;
    cdf.reserve(ordered.size());
    for (const double value : ordered) {
        cdf.push_back(1.0 - std::exp(-value / fitted.mean_life));
    }
    const auto ad = anderson_darling_from_sorted_cdf(cdf);
    candidate.anderson_darling = ad.statistic;
    candidate.adjusted_anderson_darling = ad.adjusted;
    candidate.decision = "not_computed";
    candidate.status = "computed";
    candidate.probability_plot = probability_plot_with_inverse(
        positive, positive_rows,
        [&](double probability) {
            return -fitted.mean_life * std::log(1.0 - probability);
        });
    return candidate;
}

DistributionIdentificationCandidate make_weibull_candidate(
    const std::vector<double>& positive,
    const std::vector<std::size_t>& positive_rows)
{
    DistributionIdentificationCandidate candidate;
    candidate.distribution = "Weibull";
    if (positive.size() < 2) {
        candidate.status = "not_computed";
        return candidate;
    }
    const std::vector<bool> events(positive.size(), true);
    const WeibullResult fitted = fit_weibull(positive, events);
    if (!fitted.identifiable || !fitted.converged
        || !(fitted.shape > 0.0) || !(fitted.scale > 0.0)) {
        candidate.status = "not_computed";
        return candidate;
    }
    candidate.shape = fitted.shape;
    candidate.scale = fitted.scale;
    std::vector<double> ordered = positive;
    std::sort(ordered.begin(), ordered.end());
    std::vector<double> cdf;
    cdf.reserve(ordered.size());
    for (const double value : ordered) {
        cdf.push_back(1.0 - std::exp(
            -std::pow(value / fitted.scale, fitted.shape)));
    }
    const auto ad = anderson_darling_from_sorted_cdf(cdf);
    candidate.anderson_darling = ad.statistic;
    candidate.adjusted_anderson_darling = ad.adjusted;
    candidate.decision = "not_computed";
    candidate.status = "computed";
    const double shape = fitted.shape;
    const double scale = fitted.scale;
    candidate.probability_plot = probability_plot_with_inverse(
        positive, positive_rows,
        [shape, scale](double probability) {
            return scale * std::pow(-std::log(1.0 - probability), 1.0 / shape);
        });
    return candidate;
}

}  // namespace

DistributionIdentificationResult identify_individual_distributions(
    const std::vector<double>& observations,
    const std::vector<std::size_t>& source_rows,
    double alpha)
{
    DistributionIdentificationResult result;
    std::vector<std::size_t> rows = source_rows.empty()
        ? default_source_rows(observations.size())
        : source_rows;
    if (rows.size() != observations.size()) {
        rows = default_source_rows(observations.size());
    }

    std::vector<double> valid;
    std::vector<std::size_t> valid_rows;
    valid.reserve(observations.size());
    valid_rows.reserve(observations.size());
    for (std::size_t index = 0; index < observations.size(); ++index) {
        if (std::isfinite(observations[index])) {
            valid.push_back(observations[index]);
            valid_rows.push_back(rows[index]);
        }
    }
    if (valid.size() < 3) {
        note(result, "insufficient_data", "个体分布识别至少需要 3 个有效数值观测。");
        return result;
    }

    const bool all_positive = std::all_of(
        valid.begin(), valid.end(), [](double value) { return value > 0.0; });

    result.candidates.push_back(
        make_normal_candidate(valid, valid_rows, alpha));

    if (!all_positive) {
        note(result, "non_positive_values",
             "存在非正值；Weibull、Lognormal 与 Exponential 仅对全部正值计算 AD。");
        for (const char* name : {"Weibull", "Lognormal", "Exponential"}) {
            DistributionIdentificationCandidate candidate;
            candidate.distribution = name;
            candidate.status = "not_computed";
            result.candidates.push_back(candidate);
        }
    } else {
        std::vector<std::size_t> positive_rows = valid_rows;
        const std::vector<double> positive = sorted_positive_values(valid, &positive_rows);
        result.candidates.push_back(make_weibull_candidate(positive, positive_rows));
        result.candidates.push_back(make_lognormal_candidate(positive, positive_rows, alpha));
        result.candidates.push_back(make_exponential_candidate(positive, positive_rows));
    }

    std::stable_sort(result.candidates.begin(), result.candidates.end(),
                     [](const auto& left, const auto& right) {
                         if (left.anderson_darling.has_value()
                             && right.anderson_darling.has_value()) {
                             return *left.anderson_darling < *right.anderson_darling;
                         }
                         if (left.anderson_darling.has_value()) {
                             return true;
                         }
                         if (right.anderson_darling.has_value()) {
                             return false;
                         }
                         return left.distribution < right.distribution;
                     });
    return result;
}

}  // namespace datalab::domain::statistics
