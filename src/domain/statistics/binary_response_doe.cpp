#include "domain/statistics/binary_response_doe.h"

#include "domain/statistics/logistic_regression.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace datalab::domain::statistics {
namespace {

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const std::string& message)
{
    diagnostics.push_back({severity, std::string{code}, message, {}});
}

std::vector<std::vector<double>> build_factor_design(
    const std::vector<std::vector<std::string>>& factor_columns,
    const std::vector<std::string>& factor_labels,
    bool include_ab_interaction,
    std::vector<std::string>& predictor_labels)
{
    if (factor_columns.empty()) {
        return {};
    }
    const std::size_t row_count = factor_columns.front().size();
    std::vector<std::vector<std::string>> levels;
    levels.resize(factor_columns.size());
    for (std::size_t factor = 0; factor < factor_columns.size(); ++factor) {
        std::set<std::string> unique;
        for (std::size_t row = 0; row < row_count; ++row) {
            if (row < factor_columns[factor].size()
                && !factor_columns[factor][row].empty()) {
                unique.insert(factor_columns[factor][row]);
            }
        }
        levels[factor].assign(unique.cbegin(), unique.cend());
    }

    std::size_t column_count = 0;
    for (const auto& level_set : levels) {
        if (level_set.size() > 1) {
            column_count += level_set.size() - 1;
        }
    }
    if (include_ab_interaction && factor_columns.size() >= 2
        && levels[0].size() > 1 && levels[1].size() > 1) {
        column_count += (levels[0].size() - 1) * (levels[1].size() - 1);
    }

    predictor_labels.clear();
    for (std::size_t factor = 0; factor < levels.size(); ++factor) {
        const std::string prefix = factor < factor_labels.size()
            ? factor_labels[factor] : ("F" + std::to_string(factor + 1));
        for (std::size_t index = 1; index < levels[factor].size(); ++index) {
            predictor_labels.push_back(prefix + "[" + levels[factor][index] + "]");
        }
    }
    if (include_ab_interaction && factor_columns.size() >= 2) {
        for (std::size_t ia = 1; ia < levels[0].size(); ++ia) {
            for (std::size_t ib = 1; ib < levels[1].size(); ++ib) {
                predictor_labels.push_back(
                    "AB[" + levels[0][ia] + "×" + levels[1][ib] + "]");
            }
        }
    }

    std::vector<std::vector<double>> design(row_count,
                                            std::vector<double>(column_count, 0.0));
    for (std::size_t row = 0; row < row_count; ++row) {
        std::size_t column = 0;
        std::vector<std::vector<double>> dummy(factor_columns.size());
        for (std::size_t factor = 0; factor < factor_columns.size(); ++factor) {
            dummy[factor].assign(levels[factor].size() - 1, 0.0);
            if (row >= factor_columns[factor].size()) {
                continue;
            }
            for (std::size_t index = 1; index < levels[factor].size(); ++index) {
                if (factor_columns[factor][row] == levels[factor][index]) {
                    dummy[factor][index - 1] = 1.0;
                }
            }
        }
        for (const auto& block : dummy) {
            for (double value : block) {
                if (column < column_count) {
                    design[row][column++] = value;
                }
            }
        }
        if (include_ab_interaction && factor_columns.size() >= 2) {
            for (double value_a : dummy[0]) {
                for (double value_b : dummy[1]) {
                    if (column < column_count) {
                        design[row][column++] = value_a * value_b;
                    }
                }
            }
        }
    }
    return design;
}

}  // namespace

BinaryResponseDoeResult analyze_binary_response_doe(
    const std::vector<std::vector<std::string>>& factor_columns,
    const std::vector<int>& events,
    const std::vector<int>& trials,
    const std::vector<std::string>& factor_labels,
    const std::vector<std::size_t>& source_rows,
    const BinaryResponseDoeOptions& options)
{
    BinaryResponseDoeResult result;
    result.include_ab_interaction = options.include_ab_interaction;
    result.factor_count = factor_columns.size();
    if (factor_columns.empty() || events.empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "binary_doe_empty", "需要至少一个因子列与响应。");
        return result;
    }
    const std::size_t row_count = factor_columns.front().size();
    result.design_row_count = row_count;

    std::vector<std::string> predictor_labels;
    const std::vector<std::vector<double>> design = build_factor_design(
        factor_columns, factor_labels, options.include_ab_interaction,
        predictor_labels);
    if (design.empty() || design.front().empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "binary_doe_design", "无法构建因子设计矩阵。");
        return result;
    }

    std::vector<int> response;
    std::vector<std::vector<double>> predictors;
    std::vector<std::size_t> expanded_source_rows;
    for (std::size_t row = 0; row < row_count; ++row) {
        int event = row < events.size() ? events[row] : 0;
        int trial = options.use_events_trials
            ? (row < trials.size() ? trials[row] : 0) : 1;
        if (trial <= 0) {
            continue;
        }
        if (event < 0 || event > trial) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "binary_doe_events_trials",
                           "events 必须满足 0 ≤ events ≤ trials。");
            return result;
        }
        result.event_count += event;
        result.trial_count += trial;
        const std::size_t source = row < source_rows.size() ? source_rows[row] : row;
        for (int copy = 0; copy < trial; ++copy) {
            response.push_back(copy < event ? 1 : 0);
            predictors.push_back(row < design.size() ? design[row] : design.back());
            expanded_source_rows.push_back(source);
        }
    }
    result.expanded_observation_count = response.size();
    if (response.size() < 4) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "binary_doe_insufficient",
                       "有效展开观测不足（至少 4 个 trial）。");
        return result;
    }

    const LogisticRegressionResult fit = fit_logistic_regression(
        response, predictors, predictor_labels);
    result.converged = fit.converged;
    result.iteration_count = fit.iteration_count;
    result.deviance = fit.deviance;
    result.aic = fit.aic;
    result.observation_source_rows = expanded_source_rows;
    for (const auto& coef : fit.coefficients) {
        BinaryResponseDoeCoefficient row;
        row.term = coef.term;
        row.coefficient = coef.coefficient;
        row.standard_error = coef.standard_error;
        row.z_statistic = coef.z_statistic;
        row.p_value = coef.p_value;
        row.odds_ratio = coef.odds_ratio;
        result.coefficients.push_back(row);
    }
    if (!fit.converged) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "binary_doe_not_converged", "Logit IRWLS 未收敛。");
    }
    if (fit.complete_separation) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "binary_doe_separation", "检测到完全分离。");
    }
    return result;
}

}  // namespace datalab::domain::statistics
