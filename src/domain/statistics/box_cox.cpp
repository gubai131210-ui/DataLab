#include "domain/statistics/box_cox.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

void add_error(
    std::vector<DiagnosticMessage>& diagnostics,
    const char* code,
    const char* message)
{
    diagnostics.push_back({DiagnosticMessage::Severity::error, code, message});
}

double transformed_standard_deviation(
    const std::vector<double>& observations,
    double lambda,
    double geometric_mean)
{
    std::vector<double> transformed;
    transformed.reserve(observations.size());
    for (const double value : observations) {
        const double standardized = lambda == 0.0
            ? geometric_mean * std::log(value)
            : (std::pow(value, lambda) - 1.0) / (lambda
                * std::pow(geometric_mean, lambda - 1.0));
        transformed.push_back(standardized);
    }
    const double mean = std::accumulate(
        transformed.cbegin(), transformed.cend(), 0.0)
        / static_cast<double>(transformed.size());
    double sum_squared = 0.0;
    for (const double value : transformed) {
        sum_squared += (value - mean) * (value - mean);
    }
    return std::sqrt(sum_squared / static_cast<double>(transformed.size() - 1));
}

double rounded_lambda(double lambda)
{
    const double candidates[] = {-2.0, -1.0, -0.5, 0.0, 0.5, 1.0, 2.0};
    return *std::min_element(std::begin(candidates), std::end(candidates),
        [lambda](double first, double second) {
            return std::abs(first - lambda) < std::abs(second - lambda);
        });
}

}  // namespace

BoxCoxResult box_cox_transform(
    const std::vector<double>& observations,
    std::optional<double> requested_lambda,
    bool round_interpretable_lambda)
{
    BoxCoxResult result;
    if (observations.size() < 2) {
        add_error(result.diagnostics, "insufficient_observations",
                  "Box-Cox 变换至少需要两个有效观测。");
        return result;
    }
    double log_sum = 0.0;
    for (const double value : observations) {
        if (!(value > 0.0) || !std::isfinite(value)) {
            add_error(result.diagnostics, "nonpositive_observation",
                      "Box-Cox 变换要求所有观测严格大于 0。");
            return result;
        }
        log_sum += std::log(value);
    }
    const double geometric_mean = std::exp(
        log_sum / static_cast<double>(observations.size()));
    double selected_lambda = requested_lambda.value_or(0.0);
    if (requested_lambda.has_value()
        && (*requested_lambda < -5.0 || *requested_lambda > 5.0)) {
        add_error(result.diagnostics, "lambda_out_of_range",
                  "Box-Cox lambda 必须位于 [-5, 5]。");
        return result;
    }
    if (!requested_lambda.has_value()) {
        double best_standard_deviation = std::numeric_limits<double>::infinity();
        for (int step = -500; step <= 500; ++step) {
            const double lambda = static_cast<double>(step) / 100.0;
            const double standard_deviation = transformed_standard_deviation(
                observations, lambda, geometric_mean);
            result.lambdas.push_back(lambda);
            result.standard_deviations.push_back(standard_deviation);
            if (standard_deviation < best_standard_deviation) {
                best_standard_deviation = standard_deviation;
                selected_lambda = lambda;
            }
        }
        if (round_interpretable_lambda) {
            selected_lambda = rounded_lambda(selected_lambda);
        }
    } else {
        result.lambdas.push_back(selected_lambda);
        result.standard_deviations.push_back(
            transformed_standard_deviation(observations, selected_lambda, geometric_mean));
    }
    result.lambda = selected_lambda;
    result.transformed_standard_deviation = transformed_standard_deviation(
        observations, selected_lambda, geometric_mean);
    result.transformed_values.reserve(observations.size());
    for (const double value : observations) {
        result.transformed_values.push_back(selected_lambda == 0.0
            ? std::log(value)
            : (std::pow(value, selected_lambda) - 1.0) / selected_lambda);
    }
    return result;
}

double box_cox_apply(double value, double lambda)
{
    if (!(value > 0.0) || !std::isfinite(value) || !std::isfinite(lambda)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (lambda == 0.0) {
        return std::log(value);
    }
    return (std::pow(value, lambda) - 1.0) / lambda;
}

std::optional<double> box_cox_transform_limit(double limit, double lambda)
{
    const double transformed = box_cox_apply(limit, lambda);
    if (!std::isfinite(transformed)) {
        return std::nullopt;
    }
    return transformed;
}

bool box_cox_limits_order_ok(
    double original_lsl,
    double original_usl,
    double lambda)
{
    if (!(original_lsl < original_usl)) {
        return false;
    }
    const auto t_lsl = box_cox_transform_limit(original_lsl, lambda);
    const auto t_usl = box_cox_transform_limit(original_usl, lambda);
    if (!t_lsl.has_value() || !t_usl.has_value()) {
        return false;
    }
    // Box-Cox (x^λ - 1)/λ and log(λ=0) are strictly increasing on (0, ∞) for all λ.
    return *t_lsl < *t_usl;
}

}  // namespace datalab::domain::statistics
