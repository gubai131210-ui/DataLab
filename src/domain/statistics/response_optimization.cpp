#include "domain/statistics/response_optimization.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <string>

namespace datalab::domain::statistics {
namespace {

constexpr double kEpsilon = 1.0e-12;

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const char* message)
{
    diagnostics.push_back({severity, code, message});
}

bool finite(double value)
{
    return std::isfinite(value);
}

bool valid_confidence(double confidence_level)
{
    return finite(confidence_level) && confidence_level > 0.0
        && confidence_level < 1.0;
}

std::size_t coefficient_count(const ResponseModel& model)
{
    return 1 + model.main_effect_coefficients.size()
        + model.interaction_coefficients.size();
}

std::optional<std::size_t> factor_index(
    const ResponseModel& model,
    const std::string& name)
{
    for (std::size_t index = 0; index < model.factor_names.size(); ++index) {
        if (model.factor_names[index] == name) {
            return index;
        }
    }
    return std::nullopt;
}

std::vector<double> design_row(
    const ResponseModel& model,
    const std::vector<int>& coded_levels)
{
    std::vector<double> row;
    row.reserve(coefficient_count(model));
    row.push_back(1.0);
    for (int level : coded_levels) {
        row.push_back(static_cast<double>(level));
    }
    for (const InteractionCoefficient& interaction :
         model.interaction_coefficients) {
        const std::size_t first =
            factor_index(model, interaction.first_factor).value();
        const std::size_t second =
            factor_index(model, interaction.second_factor).value();
        row.push_back(static_cast<double>(
            coded_levels[first] * coded_levels[second]));
    }
    return row;
}

double quadratic_form(
    const std::vector<double>& row,
    const std::vector<std::vector<double>>& covariance)
{
    double result = 0.0;
    for (std::size_t first = 0; first < row.size(); ++first) {
        for (std::size_t second = 0; second < row.size(); ++second) {
            result += row[first] * covariance[first][second] * row[second];
        }
    }
    return result;
}

double clamp_unit(double value)
{
    return std::clamp(value, 0.0, 1.0);
}

double desirability(
    double value,
    const ResponseObjective& objective)
{
    if (objective.goal == ResponseGoal::maximize) {
        if (value <= objective.lower) {
            return 0.0;
        }
        if (value >= objective.upper) {
            return 1.0;
        }
        return clamp_unit(
            (value - objective.lower) / (objective.upper - objective.lower));
    }
    if (objective.goal == ResponseGoal::minimize) {
        if (value <= objective.lower) {
            return 1.0;
        }
        if (value >= objective.upper) {
            return 0.0;
        }
        return clamp_unit(
            (objective.upper - value) / (objective.upper - objective.lower));
    }
    if (value <= objective.lower || value >= objective.upper) {
        return value == objective.target ? 1.0 : 0.0;
    }
    if (value <= objective.target) {
        return clamp_unit(
            (value - objective.lower) / (objective.target - objective.lower));
    }
    return clamp_unit(
        (objective.upper - value) / (objective.upper - objective.target));
}

std::optional<std::size_t> model_index(
    const std::vector<ResponseModel>& models,
    const std::string& response_name)
{
    for (std::size_t index = 0; index < models.size(); ++index) {
        if (models[index].response_name == response_name) {
            return index;
        }
    }
    return std::nullopt;
}

}  // namespace

std::vector<DiagnosticMessage> diagnose_response_model(const ResponseModel& model)
{
    std::vector<DiagnosticMessage> diagnostics;
    if (model.response_name.empty()) {
        add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                       "empty_response_name", "响应名称不能为空。");
    }
    if (model.factor_names.empty()) {
        add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                       "empty_factor_list", "响应模型至少需要一个因子。");
    }
    std::set<std::string> names;
    for (const std::string& name : model.factor_names) {
        if (name.empty() || !names.insert(name).second) {
            add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_factor_names",
                           "因子名称不能为空且必须唯一。");
            break;
        }
    }
    if (model.main_effect_coefficients.size() != model.factor_names.size()) {
        add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                       "main_effect_shape_mismatch",
                       "主效应系数数量必须等于因子数量。");
    }
    if (!finite(model.intercept)) {
        add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_intercept", "截距必须是有限数值。");
    }
    for (double coefficient : model.main_effect_coefficients) {
        if (!finite(coefficient)) {
            add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_main_effect",
                           "主效应系数必须是有限数值。");
            break;
        }
    }
    std::set<std::pair<std::string, std::string>> interactions;
    for (const InteractionCoefficient& interaction :
         model.interaction_coefficients) {
        const bool names_exist =
            factor_index(model, interaction.first_factor).has_value()
            && factor_index(model, interaction.second_factor).has_value();
        const bool distinct =
            interaction.first_factor != interaction.second_factor;
        const auto key = std::minmax(
            interaction.first_factor, interaction.second_factor);
        if (!names_exist || !distinct || !interactions.insert(key).second) {
            add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_interaction",
                           "交互效应必须引用两个不同且已定义的因子，且不能重复。");
            break;
        }
        if (!finite(interaction.coefficient)) {
            add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_interaction_coefficient",
                           "交互效应系数必须是有限数值。");
            break;
        }
    }
    if (!finite(model.residual_standard_error)
        || model.residual_standard_error < 0.0) {
        add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_residual_standard_error",
                       "残差标准误必须是非负有限数值。");
    }
    if (!finite(model.residual_degrees_of_freedom)
        || model.residual_degrees_of_freedom <= 0.0) {
        add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_residual_degrees_of_freedom",
                       "残差自由度必须大于零。");
    }
    if (!valid_confidence(model.confidence_level)) {
        add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_confidence_level",
                       "置信水平必须大于零且小于一。");
    }
    const std::size_t expected_size = coefficient_count(model);
    if (!model.coefficient_covariance.empty()) {
        if (model.coefficient_covariance.size() != expected_size) {
            add_diagnostic(diagnostics, DiagnosticMessage::Severity::error,
                           "covariance_shape_mismatch",
                           "系数协方差矩阵维度必须匹配模型项数量。");
        } else {
            for (const std::vector<double>& row :
                 model.coefficient_covariance) {
                if (row.size() != expected_size) {
                    add_diagnostic(diagnostics,
                                   DiagnosticMessage::Severity::error,
                                   "covariance_shape_mismatch",
                                   "系数协方差矩阵必须是方阵。");
                    break;
                }
                for (double value : row) {
                    if (!finite(value)) {
                        add_diagnostic(diagnostics,
                                       DiagnosticMessage::Severity::error,
                                       "invalid_covariance",
                                       "系数协方差矩阵必须只包含有限数值。");
                        break;
                    }
                }
            }
        }
    } else if (model.observation_count == 0) {
        add_diagnostic(diagnostics, DiagnosticMessage::Severity::warning,
                       "approximate_confidence_standard_error",
                       "未提供系数协方差矩阵和观测数，无法计算置信区间。");
    }
    return diagnostics;
}

ResponsePrediction predict_response(
    const ResponseModel& model,
    const std::vector<int>& coded_levels)
{
    ResponsePrediction result;
    result.response_name = model.response_name;
    result.coded_levels = coded_levels;
    result.diagnostics = diagnose_response_model(model);
    if (coded_levels.size() != model.factor_names.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "coded_level_shape_mismatch",
                       "编码组合数量必须等于因子数量。");
        return result;
    }
    for (int level : coded_levels) {
        if (level != -1 && level != 1) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_coded_level",
                           "2-level 设计编码只能使用 -1 或 +1。");
            return result;
        }
    }
    const bool has_error = std::any_of(
        result.diagnostics.cbegin(), result.diagnostics.cend(),
        [](const DiagnosticMessage& diagnostic) {
            return diagnostic.severity == DiagnosticMessage::Severity::error;
        });
    if (has_error) {
        return result;
    }

    const std::vector<double> row = design_row(model, coded_levels);
    result.predicted_value = model.intercept;
    for (std::size_t index = 0; index < model.factor_names.size(); ++index) {
        result.predicted_value +=
            model.main_effect_coefficients[index] * row[index + 1];
    }
    const std::size_t interaction_offset = 1 + model.factor_names.size();
    for (std::size_t index = 0; index < model.interaction_coefficients.size();
         ++index) {
        result.predicted_value +=
            model.interaction_coefficients[index].coefficient
            * row[interaction_offset + index];
    }

    double standard_error = 0.0;
    if (!model.coefficient_covariance.empty()) {
        standard_error = quadratic_form(row, model.coefficient_covariance);
        if (standard_error < -kEpsilon) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_prediction_variance",
                           "系数协方差矩阵产生了负的预测方差。");
            return result;
        }
        standard_error = std::sqrt(std::max(0.0, standard_error));
    } else if (model.observation_count > 0) {
        standard_error = model.residual_standard_error
            / std::sqrt(static_cast<double>(model.observation_count));
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "approximate_prediction_standard_error",
                       "未提供系数协方差矩阵，置信标准误采用残差标准误除以观测数平方根近似。");
    } else {
        return result;
    }

    const double critical = student_t_quantile(
        0.5 + model.confidence_level / 2.0,
        model.residual_degrees_of_freedom);
    if (!finite(critical) || critical <= 0.0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_interval_critical_value",
                       "无法计算指定置信水平的 t 临界值。");
        return result;
    }
    const double prediction_standard_error = std::sqrt(
        standard_error * standard_error
        + model.residual_standard_error * model.residual_standard_error);
    PredictionInterval interval;
    interval.standard_error = standard_error;
    interval.prediction_standard_error = prediction_standard_error;
    interval.critical_value = critical;
    interval.confidence_lower = result.predicted_value - critical * standard_error;
    interval.confidence_upper = result.predicted_value + critical * standard_error;
    interval.prediction_lower =
        result.predicted_value - critical * prediction_standard_error;
    interval.prediction_upper =
        result.predicted_value + critical * prediction_standard_error;
    result.interval = interval;
    return result;
}

ResponseOptimizationResult optimize_response_desirability(
    const std::vector<ResponseModel>& models,
    const std::vector<ResponseObjective>& objectives)
{
    ResponseOptimizationResult result;
    if (models.empty() || objectives.empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "empty_optimization_input",
                       "响应优化至少需要一个模型和一个目标。");
        return result;
    }
    const std::vector<DiagnosticMessage> model_diagnostics =
        diagnose_response_model(models.front());
    if (model_diagnostics.end() != std::find_if(
            model_diagnostics.cbegin(), model_diagnostics.cend(),
            [](const DiagnosticMessage& diagnostic) {
                return diagnostic.severity == DiagnosticMessage::Severity::error;
            })) {
        result.diagnostics.insert(result.diagnostics.end(),
                                  model_diagnostics.cbegin(),
                                  model_diagnostics.cend());
        return result;
    }
    const std::vector<std::string>& factor_names = models.front().factor_names;
    for (const ResponseModel& model : models) {
        if (model.factor_names != factor_names) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "factor_set_mismatch",
                           "所有响应模型必须使用相同顺序的因子。");
            return result;
        }
        const std::vector<DiagnosticMessage> diagnostics =
            diagnose_response_model(model);
        if (std::any_of(diagnostics.cbegin(), diagnostics.cend(),
                        [](const DiagnosticMessage& diagnostic) {
                            return diagnostic.severity
                                == DiagnosticMessage::Severity::error;
                        })) {
            result.diagnostics.insert(result.diagnostics.end(),
                                      diagnostics.cbegin(), diagnostics.cend());
            return result;
        }
    }
    std::set<std::string> objective_names;
    for (const ResponseObjective& objective : objectives) {
        if (!model_index(models, objective.response_name).has_value()
            || !objective_names.insert(objective.response_name).second) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_objective_response",
                           "每个优化目标必须引用唯一的响应模型。");
            return result;
        }
        if (!finite(objective.lower) || !finite(objective.upper)
            || objective.upper <= objective.lower || !finite(objective.weight)
            || objective.weight <= 0.0) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_objective_limits",
                           "目标上下限必须有限且 upper 大于 lower，权重必须大于零。");
            return result;
        }
        if (objective.goal == ResponseGoal::target
            && (!finite(objective.target)
                || objective.target < objective.lower
                || objective.target > objective.upper)) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_target_value",
                           "target 目标的 target 必须位于 lower 和 upper 之间。");
            return result;
        }
    }
    if (factor_names.size() >= std::numeric_limits<std::size_t>::digits) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "factor_count_overflow",
                       "因子数量过大，无法枚举所有 2-level 组合。");
        return result;
    }
    const std::size_t candidate_count = std::size_t{1} << factor_names.size();
    result.candidates.reserve(candidate_count);
    for (std::size_t code = 0; code < candidate_count; ++code) {
        OptimizationCandidate candidate;
        candidate.coded_levels.reserve(factor_names.size());
        for (std::size_t factor = 0; factor < factor_names.size(); ++factor) {
            candidate.coded_levels.push_back(
                (code & (std::size_t{1} << factor)) != 0 ? 1 : -1);
        }
        double log_desirability = 0.0;
        bool zero_desirability = false;
        for (const ResponseObjective& objective : objectives) {
            const std::size_t index =
                model_index(models, objective.response_name).value();
            ResponsePrediction prediction =
                predict_response(models[index], candidate.coded_levels);
            candidate.predictions.push_back(prediction);
            const double value_desirability =
                desirability(prediction.predicted_value, objective);
            candidate.desirabilities.push_back(value_desirability);
            if (value_desirability <= 0.0) {
                zero_desirability = true;
            } else {
                log_desirability += objective.weight
                    * std::log(value_desirability);
            }
        }
        if (!zero_desirability) {
            double weight_total = 0.0;
            for (const ResponseObjective& objective : objectives) {
                weight_total += objective.weight;
            }
            candidate.overall_desirability =
                std::exp(log_desirability / weight_total);
        }
        result.candidates.push_back(std::move(candidate));
    }
    if (!result.candidates.empty()) {
        result.best_candidate = *std::max_element(
            result.candidates.cbegin(), result.candidates.cend(),
            [](const OptimizationCandidate& first,
               const OptimizationCandidate& second) {
                return first.overall_desirability
                    < second.overall_desirability;
            });
    }
    return result;
}

}  // namespace datalab::domain::statistics
