#include "domain/statistics/stepwise_regression.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace datalab::domain::statistics {
namespace {

std::vector<std::vector<double>> subset_predictors(
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::size_t>& indices)
{
    std::vector<std::vector<double>> out;
    out.reserve(predictors.size());
    for (const auto& row : predictors) {
        std::vector<double> selected;
        selected.reserve(indices.size());
        for (std::size_t index : indices) {
            selected.push_back(row[index]);
        }
        out.push_back(std::move(selected));
    }
    return out;
}

std::optional<double> term_p_value(
    const RegressionResult& model, const std::string& term)
{
    for (const auto& coefficient : model.coefficients) {
        if (coefficient.term == term) {
            return coefficient.p_value;
        }
    }
    return std::nullopt;
}

void fill_information_criteria(
    StepwiseStep& step,
    const RegressionResult& model,
    std::size_t n,
    std::size_t predictor_count)
{
    if (n < 2 || !(model.error_sum_of_squares >= 0.0)) {
        return;
    }
    const double k = static_cast<double>(predictor_count + 1);  // intercept + predictors
    const double sse = model.error_sum_of_squares;
    const double aic = n * std::log(sse / static_cast<double>(n)) + 2.0 * k;
    step.aic = aic;
    if (n > k + 1) {
        step.aicc = aic + (2.0 * k * (k + 1.0))
            / (static_cast<double>(n) - k - 1.0);
    }
    step.bic = n * std::log(sse / static_cast<double>(n)) + k * std::log(static_cast<double>(n));
}

double criterion_value(const StepwiseStep& step, const std::string& criterion)
{
    if (criterion == "bic" && step.bic.has_value()) {
        return *step.bic;
    }
    if (criterion == "aicc" && step.aicc.has_value()) {
        return *step.aicc;
    }
    if (step.aicc.has_value()) {
        return *step.aicc;
    }
    return std::numeric_limits<double>::infinity();
}

}  // namespace

StepwiseRegressionResult fit_stepwise_regression(
    const std::vector<double>& response,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& predictor_labels,
    const std::string& method,
    double alpha_enter,
    double alpha_remove,
    double confidence_level,
    const std::vector<std::size_t>& source_rows)
{
    StepwiseRegressionResult result;
    result.method = method;
    result.alpha_enter = alpha_enter;
    result.alpha_remove = alpha_remove;
    const bool info_criterion =
        method == "forward_aicc" || method == "forward_bic";
    result.criterion = info_criterion
        ? (method == "forward_bic" ? "bic" : "aicc") : "alpha";
    if (response.size() < 4 || predictors.size() != response.size()
        || predictors.empty() || predictors.front().size() < 2
        || predictor_labels.size() != predictors.front().size()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "stepwise_invalid",
            "逐步回归需要 ≥4 观测与 ≥2 候选预测变量。"});
        return result;
    }
    if (!(alpha_enter > 0.0 && alpha_enter < 1.0)
        || !(alpha_remove > 0.0 && alpha_remove < 1.0)) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "stepwise_alpha",
            "α_enter / α_remove 必须在 (0,1)。"});
        return result;
    }
    if (alpha_remove < alpha_enter) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "stepwise_alpha_order",
            "α_remove < α_enter 可能导致振荡；仍继续。"});
    }

    const std::size_t p = predictors.front().size();
    result.observation_count = response.size();
    result.candidate_count = p;
    std::set<std::size_t> in_model;
    std::set<std::size_t> out_model;
    for (std::size_t i = 0; i < p; ++i) {
        out_model.insert(i);
    }

    const bool do_forward = method == "forward" || method == "stepwise"
        || method == "forward_aicc" || method == "forward_bic";
    const bool do_backward = method == "backward" || method == "stepwise";
    if (method == "backward") {
        for (std::size_t i = 0; i < p; ++i) {
            in_model.insert(i);
        }
        out_model.clear();
    }

    auto fit_current = [&](const std::set<std::size_t>& active) {
        std::vector<std::size_t> indices(active.begin(), active.end());
        std::vector<std::string> labels;
        for (std::size_t index : indices) {
            labels.push_back(predictor_labels[index]);
        }
        if (indices.empty()) {
            // Intercept-only: use a constant predictor column of zeros? fit needs ≥1 predictor.
            // Emulate with a dummy zero column labeled "(none)" — better: skip and synthesize.
            RegressionResult empty;
            empty.observation_count = response.size();
            empty.error_sum_of_squares = 0.0;
            double mean = 0.0;
            for (double y : response) {
                mean += y;
            }
            mean /= static_cast<double>(response.size());
            for (double y : response) {
                empty.error_sum_of_squares += (y - mean) * (y - mean);
            }
            empty.total_sum_of_squares = empty.error_sum_of_squares;
            empty.r_squared = 0.0;
            empty.adjusted_r_squared = 0.0;
            return empty;
        }
        return fit_linear_regression(
            response, subset_predictors(predictors, indices), labels,
            confidence_level, source_rows);
    };

    RegressionResult current = fit_current(in_model);
    {
        StepwiseStep start;
        start.step = 0;
        start.action = "start";
        start.term = "-";
        start.r_squared = current.r_squared;
        start.adjusted_r_squared = current.adjusted_r_squared;
        start.error_sum_of_squares = current.error_sum_of_squares;
        fill_information_criteria(start, current, response.size(), in_model.size());
        result.steps.push_back(start);
    }

    if (info_criterion) {
        for (std::size_t iter = 0; iter < p + 1; ++iter) {
            if (out_model.empty()) {
                break;
            }
            std::size_t best = 0;
            double best_p = std::numeric_limits<double>::infinity();
            bool found = false;
            for (std::size_t candidate : out_model) {
                std::set<std::size_t> trial = in_model;
                trial.insert(candidate);
                if (trial.size() + 1 >= response.size()) {
                    continue;
                }
                const auto trial_fit = fit_current(trial);
                const auto p_value =
                    term_p_value(trial_fit, predictor_labels[candidate]);
                if (!p_value.has_value()) {
                    continue;
                }
                if (*p_value < best_p) {
                    best_p = *p_value;
                    best = candidate;
                    found = true;
                }
            }
            if (!found) {
                break;
            }
            in_model.insert(best);
            out_model.erase(best);
            current = fit_current(in_model);
            StepwiseStep step;
            step.step = result.steps.size();
            step.action = "enter";
            step.term = predictor_labels[best];
            step.r_squared = current.r_squared;
            step.adjusted_r_squared = current.adjusted_r_squared;
            step.error_sum_of_squares = current.error_sum_of_squares;
            step.entered_p_value = best_p;
            fill_information_criteria(step, current, response.size(), in_model.size());
            result.steps.push_back(step);
        }
        StepwiseStep stop;
        stop.step = result.steps.size();
        stop.action = "stop";
        stop.term = "-";
        stop.r_squared = current.r_squared;
        stop.adjusted_r_squared = current.adjusted_r_squared;
        stop.error_sum_of_squares = current.error_sum_of_squares;
        fill_information_criteria(stop, current, response.size(), in_model.size());
        result.steps.push_back(stop);

        double best_crit = std::numeric_limits<double>::infinity();
        std::size_t best_idx = 0;
        RegressionResult best_model = current;
        std::set<std::size_t> best_in = in_model;
        for (std::size_t si = 0; si < result.steps.size(); ++si) {
            const double crit = criterion_value(result.steps[si], result.criterion);
            if (crit < best_crit) {
                best_crit = crit;
                best_idx = si;
            }
        }
        result.best_step_index = best_idx;
        // Reconstruct model at best step by replaying enters up to best_idx.
        std::set<std::size_t> replay;
        for (std::size_t si = 1; si <= best_idx && si < result.steps.size(); ++si) {
            if (result.steps[si].action == "enter") {
                for (std::size_t index = 0; index < p; ++index) {
                    if (predictor_labels[index] == result.steps[si].term) {
                        replay.insert(index);
                    }
                }
            }
        }
        if (!replay.empty() || best_idx == 0) {
            best_model = fit_current(replay);
            best_in = replay;
        }
        in_model = best_in;
        current = best_model;
    } else for (std::size_t iter = 0; iter < p * 4 + 4; ++iter) {
        bool changed = false;

        if (do_backward && !in_model.empty()) {
            std::size_t worst = 0;
            double worst_p = -1.0;
            bool found = false;
            for (std::size_t index : in_model) {
                const auto p_value = term_p_value(current, predictor_labels[index]);
                if (!p_value.has_value()) {
                    continue;
                }
                if (*p_value > worst_p) {
                    worst_p = *p_value;
                    worst = index;
                    found = true;
                }
            }
            if (found && worst_p > alpha_remove) {
                in_model.erase(worst);
                out_model.insert(worst);
                current = fit_current(in_model);
                StepwiseStep step;
                step.step = result.steps.size();
                step.action = "remove";
                step.term = predictor_labels[worst];
                step.r_squared = current.r_squared;
                step.adjusted_r_squared = current.adjusted_r_squared;
                step.error_sum_of_squares = current.error_sum_of_squares;
                step.removed_p_value = worst_p;
                fill_information_criteria(step, current, response.size(), in_model.size());
                result.steps.push_back(step);
                changed = true;
            }
        }

        if (!changed && do_forward && !out_model.empty()) {
            std::size_t best = 0;
            double best_p = std::numeric_limits<double>::infinity();
            bool found = false;
            for (std::size_t candidate : out_model) {
                std::set<std::size_t> trial = in_model;
                trial.insert(candidate);
                if (trial.size() + 1 >= response.size()) {
                    continue;
                }
                const auto trial_fit = fit_current(trial);
                const auto p_value =
                    term_p_value(trial_fit, predictor_labels[candidate]);
                if (!p_value.has_value()) {
                    continue;
                }
                if (*p_value < best_p) {
                    best_p = *p_value;
                    best = candidate;
                    found = true;
                }
            }
            if (found && best_p < alpha_enter) {
                in_model.insert(best);
                out_model.erase(best);
                current = fit_current(in_model);
                StepwiseStep step;
                step.step = result.steps.size();
                step.action = "enter";
                step.term = predictor_labels[best];
                step.r_squared = current.r_squared;
                step.adjusted_r_squared = current.adjusted_r_squared;
                step.error_sum_of_squares = current.error_sum_of_squares;
                step.entered_p_value = best_p;
                fill_information_criteria(step, current, response.size(), in_model.size());
                result.steps.push_back(step);
                changed = true;
            }
        }

        if (!changed) {
            StepwiseStep stop;
            stop.step = result.steps.size();
            stop.action = "stop";
            stop.term = "-";
            stop.r_squared = current.r_squared;
            stop.adjusted_r_squared = current.adjusted_r_squared;
            stop.error_sum_of_squares = current.error_sum_of_squares;
            fill_information_criteria(stop, current, response.size(), in_model.size());
            result.steps.push_back(stop);
            break;
        }
    }

    for (std::size_t index : in_model) {
        result.selected_terms.push_back(predictor_labels[index]);
    }
    result.final_model = current;
    if (result.selected_terms.empty()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "stepwise_empty",
            "未选入任何预测变量（仅截距）。"});
    }
    result.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "stepwise_scope",
        info_criterion
            ? ("Forward " + result.criterion + " 信息准则；非 Best subsets；非 Minitab golden。")
            : "α 逐步选择；非 Best subsets；非 Minitab golden。"});
    return result;
}

}  // namespace datalab::domain::statistics
