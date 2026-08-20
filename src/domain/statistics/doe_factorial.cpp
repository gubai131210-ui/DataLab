#include "domain/statistics/doe_factorial.h"

#include "domain/column_extract.h"
#include "domain/statistics/regression.h"
#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <utility>

namespace datalab::domain::statistics {
namespace {

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const char* message)
{
    diagnostics.push_back({severity, code, message});
}

bool is_binary_level(int level)
{
    return level == -1 || level == 1;
}

std::string interaction_name(const DoeFactor& first, const DoeFactor& second)
{
    return first.name + "*" + second.name;
}

double median_absolute(std::vector<double> values)
{
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const std::size_t middle = values.size() / 2;
    if (values.size() % 2 == 1) {
        return values[middle];
    }
    return 0.5 * (values[middle - 1] + values[middle]);
}

double lenth_pseudo_standard_error(const std::vector<double>& effects)
{
    std::vector<double> absolute;
    absolute.reserve(effects.size());
    for (const double effect : effects) {
        absolute.push_back(std::abs(effect));
    }
    const double scale = 1.5 * median_absolute(absolute);
    std::vector<double> trimmed;
    for (const double value : absolute) {
        if (value < 2.5 * scale) {
            trimmed.push_back(value);
        }
    }
    if (trimmed.empty()) {
        trimmed = std::move(absolute);
    }
    return 1.5 * median_absolute(trimmed);
}

}  // namespace

DoeFactorialDesign generate_2_level_factorial(const DoeDesignOptions& options)
{
    DoeFactorialDesign design;
    design.factors = options.factors;

    if (options.factors.empty()) {
        add_diagnostic(design.diagnostics, DiagnosticMessage::Severity::error,
                       "empty_doe_factor_list", "DOE 至少需要一个因子。");
        return design;
    }
    if (options.block_count == 0) {
        add_diagnostic(design.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_doe_block_count", "区组数必须大于零。");
        return design;
    }

    std::set<std::string> names;
    for (const DoeFactor& factor : options.factors) {
        if (factor.name.empty() || factor.low_level.empty()
            || factor.high_level.empty()) {
            add_diagnostic(design.diagnostics, DiagnosticMessage::Severity::error,
                           "incomplete_doe_factor",
                           "每个 DOE 因子必须有名称、低水平和高水平。");
            return design;
        }
        if (!names.insert(factor.name).second) {
            add_diagnostic(design.diagnostics, DiagnosticMessage::Severity::error,
                           "duplicate_doe_factor_name", "DOE 因子名称必须唯一。");
            return design;
        }
    }

    const std::size_t factor_count = options.factors.size();
    const std::size_t max_size = std::numeric_limits<std::size_t>::max();
    std::size_t factorial_run_count = 1;
    for (std::size_t index = 0; index < factor_count; ++index) {
        if (factorial_run_count > max_size / 2) {
            add_diagnostic(design.diagnostics, DiagnosticMessage::Severity::error,
                           "doe_factor_count_overflow",
                           "因子数量过大，无法生成全因子运行数。");
            return design;
        }
        factorial_run_count *= 2;
    }

    if (options.center_point_count > max_size - factorial_run_count) {
        add_diagnostic(design.diagnostics, DiagnosticMessage::Severity::error,
                       "doe_center_point_count_overflow",
                       "中心点数量过大，无法生成 DOE 运行列表。");
        return design;
    }
    design.runs.reserve(factorial_run_count + options.center_point_count);
    for (std::size_t standard_order = 0;
         standard_order < factorial_run_count; ++standard_order) {
        DoeRun run;
        run.standard_order = standard_order;
        run.coded_levels.reserve(factor_count);
        for (std::size_t factor = 0; factor < factor_count; ++factor) {
            const bool high = (standard_order & (std::size_t{1} << factor)) != 0;
            run.coded_levels.push_back(high ? 1 : -1);
        }
        design.runs.push_back(std::move(run));
    }
    for (std::size_t index = 0; index < options.center_point_count; ++index) {
        DoeRun run;
        run.standard_order = factorial_run_count + index;
        run.center_point = true;
        run.coded_levels.assign(factor_count, 0);
        design.runs.push_back(std::move(run));
    }

    if (options.randomize) {
        randomize_design(design, options.random_seed);
    } else {
        for (std::size_t index = 0; index < design.runs.size(); ++index) {
            design.runs[index].run_order = index;
        }
    }
    assign_blocks(design, options.block_count);
    return design;
}

void randomize_design(DoeFactorialDesign& design, std::uint64_t seed)
{
    std::mt19937_64 generator(seed);
    std::shuffle(design.runs.begin(), design.runs.end(), generator);
    for (std::size_t index = 0; index < design.runs.size(); ++index) {
        design.runs[index].run_order = index;
    }
}

void assign_blocks(DoeFactorialDesign& design, std::size_t block_count)
{
    if (block_count == 0) {
        add_diagnostic(design.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_doe_block_count", "区组数必须大于零。");
        return;
    }
    for (std::size_t index = 0; index < design.runs.size(); ++index) {
        design.runs[index].block = (index % block_count) + 1;
    }
}

DoeValidationResult validate_design(const DoeFactorialDesign& design)
{
    DoeValidationResult result;
    result.valid = true;

    if (design.factors.empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "empty_doe_factor_list", "DOE 至少需要一个因子。");
        result.valid = false;
        return result;
    }
    std::set<std::string> names;
    for (const DoeFactor& factor : design.factors) {
        if (factor.name.empty() || factor.low_level.empty()
            || factor.high_level.empty()) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "incomplete_doe_factor",
                           "每个 DOE 因子必须有名称、低水平和高水平。");
            result.valid = false;
        }
        if (!names.insert(factor.name).second) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "duplicate_doe_factor_name", "DOE 因子名称必须唯一。");
            result.valid = false;
        }
    }

    std::size_t factorial_run_count = 1;
    const std::size_t max_size = std::numeric_limits<std::size_t>::max();
    for (std::size_t index = 0; index < design.factors.size(); ++index) {
        if (factorial_run_count > max_size / 2) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "doe_factor_count_overflow",
                           "因子数量过大，无法验证全因子运行数。");
            result.valid = false;
            return result;
        }
        factorial_run_count *= 2;
    }

    std::map<std::vector<int>, std::size_t> factorial_points;
    std::size_t center_points = 0;
    for (const DoeRun& run : design.runs) {
        if (run.coded_levels.size() != design.factors.size()) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_doe_run_shape",
                           "每个 DOE 运行必须包含一个对应每个因子的编码。");
            result.valid = false;
            continue;
        }
        if (run.center_point) {
            ++center_points;
            if (std::any_of(run.coded_levels.cbegin(), run.coded_levels.cend(),
                            [](int level) { return level != 0; })) {
                add_diagnostic(result.diagnostics,
                               DiagnosticMessage::Severity::error,
                               "invalid_doe_center_point",
                               "中心点的所有编码必须为零。");
                result.valid = false;
            }
            continue;
        }
        if (std::any_of(run.coded_levels.cbegin(), run.coded_levels.cend(),
                        [](int level) { return !is_binary_level(level); })) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_doe_factor_level",
                           "全因子运行的编码只能为 -1 或 +1。");
            result.valid = false;
            continue;
        }
        ++factorial_points[run.coded_levels];
    }
    if (factorial_points.size() != factorial_run_count
        || std::any_of(factorial_points.cbegin(), factorial_points.cend(),
                       [](const auto& point) { return point.second != 1; })) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "incomplete_doe_factorial_coverage",
                       "设计未完整覆盖每个 2 水平因子组合，或存在重复运行。");
        result.valid = false;
    }
    if (design.runs.size() != factorial_run_count + center_points) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_doe_run_count", "DOE 运行数与设计内容不一致。");
        result.valid = false;
    }
    for (const DoeRun& run : design.runs) {
        if (run.block == 0) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_doe_block", "DOE 区组编号必须从 1 开始。");
            result.valid = false;
            break;
        }
    }
    return result;
}

DoeEffectSummaryResult summarize_effects(
    const DoeFactorialDesign& design,
    const std::vector<double>& responses)
{
    DoeEffectSummaryResult result;
    const DoeValidationResult validation = validate_design(design);
    result.diagnostics = validation.diagnostics;
    if (!validation.valid) {
        return result;
    }
    if (responses.size() != design.runs.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_doe_response_shape",
                       "响应值数量必须与 DOE 运行数一致。");
        return result;
    }

    struct Accumulator {
        double positive_sum = 0.0;
        double negative_sum = 0.0;
        std::size_t positive_count = 0;
        std::size_t negative_count = 0;
    };
    std::vector<std::pair<std::string, std::vector<int>>> terms;
    for (std::size_t factor = 0; factor < design.factors.size(); ++factor) {
        terms.push_back({design.factors[factor].name, {static_cast<int>(factor)}});
    }
    for (std::size_t first = 0; first < design.factors.size(); ++first) {
        for (std::size_t second = first + 1;
             second < design.factors.size(); ++second) {
            terms.push_back({interaction_name(
                design.factors[first], design.factors[second]),
                {static_cast<int>(first), static_cast<int>(second)}});
        }
    }

    for (const auto& [term, factors] : terms) {
        Accumulator accumulator;
        for (std::size_t row = 0; row < design.runs.size(); ++row) {
            const DoeRun& run = design.runs[row];
            if (run.center_point || !std::isfinite(responses[row])) {
                continue;
            }
            int sign = 1;
            for (const int factor : factors) {
                sign *= run.coded_levels[static_cast<std::size_t>(factor)];
            }
            if (sign > 0) {
                accumulator.positive_sum += responses[row];
                ++accumulator.positive_count;
            } else {
                accumulator.negative_sum += responses[row];
                ++accumulator.negative_count;
            }
        }
        if (accumulator.positive_count == 0 || accumulator.negative_count == 0) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "insufficient_doe_effect_data",
                           "计算 DOE 效应时正负对比组必须都有有效响应。");
            continue;
        }
        const double positive_mean = accumulator.positive_sum
            / static_cast<double>(accumulator.positive_count);
        const double negative_mean = accumulator.negative_sum
            / static_cast<double>(accumulator.negative_count);
        const double effect = positive_mean - negative_mean;
        result.effects.push_back({
            term,
            effect,
            effect / 2.0,
            positive_mean,
            negative_mean,
            accumulator.positive_count,
            accumulator.negative_count});
    }
    if (std::any_of(responses.cbegin(), responses.cend(),
                    [](double value) { return !std::isfinite(value); })) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "missing_doe_responses",
                       "缺失或非有限响应未参与 DOE 效应摘要。");
    }
    return result;
}

DoeResponseAnalysisResult fit_response_analysis(
    const DoeFactorialDesign& design,
    const std::vector<double>& responses,
    const std::string& response_name)
{
    DoeResponseAnalysisResult result;
    result.response_name = response_name;
    if (responses.size() != design.runs.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_doe_response_shape",
                       "响应值数量必须与 DOE 运行数一致。");
        return result;
    }
    if (design.factors.empty()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "empty_doe_factor_list", "DOE 至少需要一个因子。");
        return result;
    }
    if (design.factors.size() >= std::numeric_limits<std::size_t>::digits) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "doe_factor_count_overflow",
                       "因子数量过大，无法诊断完整 2 水平组合。");
        return result;
    }

    std::map<std::vector<int>, std::size_t> factorial_points;
    for (std::size_t row = 0; row < design.runs.size(); ++row) {
        const DoeRun& run = design.runs[row];
        if (run.coded_levels.size() != design.factors.size()) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_doe_run_shape",
                           "每个 DOE 运行必须包含一个对应每个因子的编码。");
            continue;
        }
        if (!run.center_point) {
            ++factorial_points[run.coded_levels];
        }
    }
    const std::size_t expected_points = std::size_t{1} << design.factors.size();
    if (factorial_points.size() != expected_points) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "missing_doe_runs",
                       "设计缺少一个或多个 2 水平因子组合。");
    }
    if (std::any_of(factorial_points.cbegin(), factorial_points.cend(),
                    [](const auto& point) { return point.second > 1; })) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "duplicate_doe_runs", "设计包含重复的因子组合。");
    }

    std::vector<double> usable_responses;
    std::vector<std::vector<double>> predictors;
    std::vector<std::size_t> source_rows;
    std::vector<std::string> labels;
    const std::size_t model_label_count = design.factors.size()
        + design.factors.size() * (design.factors.size() - 1) / 2;
    labels.reserve(model_label_count + design.runs.size());
    for (const DoeFactor& factor : design.factors) {
        labels.push_back(factor.name);
    }
    for (std::size_t first = 0; first < design.factors.size(); ++first) {
        for (std::size_t second = first + 1;
             second < design.factors.size(); ++second) {
            labels.push_back(interaction_name(
                design.factors[first], design.factors[second]));
        }
    }
    std::set<std::size_t> observed_blocks;
    for (const DoeRun& run : design.runs) {
        if (run.block == 0) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                           "invalid_doe_block",
                           "DOE 区组编号必须从 1 开始。");
        } else {
            observed_blocks.insert(run.block);
        }
    }
    if (observed_blocks.size() > 1) {
        for (auto block = std::next(observed_blocks.cbegin());
             block != observed_blocks.cend(); ++block) {
            labels.push_back("Block[" + std::to_string(*block) + "]");
        }
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                       "block_terms_included", "区组项已作为模型中的分类项纳入拟合。");
    }
    const std::size_t block_label_count =
        labels.size() - model_label_count;
    if (std::any_of(result.diagnostics.cbegin(), result.diagnostics.cend(),
                    [](const DiagnosticMessage& diagnostic) {
                        return diagnostic.severity == DiagnosticMessage::Severity::error;
                    })) {
        return result;
    }

    for (std::size_t row = 0; row < design.runs.size(); ++row) {
        if (!std::isfinite(responses[row])) {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "missing_doe_response", "缺失响应运行未参与拟合。");
            continue;
        }
        const DoeRun& run = design.runs[row];
        if (run.coded_levels.size() != design.factors.size()) {
            continue;
        }
        std::vector<double> predictor_row;
        predictor_row.reserve(labels.size());
        for (const int level : run.coded_levels) {
            predictor_row.push_back(static_cast<double>(level));
        }
        for (std::size_t first = 0; first < design.factors.size(); ++first) {
            for (std::size_t second = first + 1;
                 second < design.factors.size(); ++second) {
                predictor_row.push_back(static_cast<double>(
                    run.coded_levels[first] * run.coded_levels[second]));
            }
        }
        if (block_label_count > 0) {
            auto block = std::next(observed_blocks.cbegin());
            for (std::size_t index = 0; index < block_label_count;
                 ++index, ++block) {
                predictor_row.push_back(run.block == *block ? 1.0 : 0.0);
            }
        }
        usable_responses.push_back(responses[row]);
        predictors.push_back(std::move(predictor_row));
        source_rows.push_back(row);
    }

    const RegressionResult regression = fit_linear_regression(
        usable_responses, predictors, labels);
    result.diagnostics.insert(result.diagnostics.end(),
                              regression.diagnostics.cbegin(),
                              regression.diagnostics.cend());
    if (std::any_of(regression.diagnostics.cbegin(), regression.diagnostics.cend(),
                    [](const DiagnosticMessage& diagnostic) {
                        return diagnostic.severity == DiagnosticMessage::Severity::error;
                    })) {
        return result;
    }
    result.term_names.push_back("Constant");
    result.coefficients.reserve(regression.coefficients.size());
    for (const RegressionCoefficient& coefficient : regression.coefficients) {
        result.coefficients.push_back(coefficient.coefficient);
    }
    result.effects.assign(result.coefficients.size(), 0.0);
    result.standard_errors.assign(regression.coefficients.size(), 0.0);
    result.t_statistics.assign(regression.coefficients.size(), 0.0);
    for (std::size_t index = 0; index < regression.coefficients.size(); ++index) {
        result.standard_errors[index] = regression.coefficients[index].standard_error;
        result.t_statistics[index] = regression.coefficients[index].t_statistic;
    }
    for (std::size_t index = 1; index < result.coefficients.size(); ++index) {
        result.term_names.push_back(labels[index - 1]);
        result.effects[index] = 2.0 * result.coefficients[index];
    }
    result.residual_sum_of_squares = regression.error_sum_of_squares;
    result.residual_degrees_of_freedom =
        regression.observation_count - regression.predictor_count - 1;
    result.residual_mean_square = regression.error_mean_square;
    result.xtx_inverse = regression.xtx_inverse;
    result.r_squared = regression.r_squared;
    for (std::size_t index = 1; index < regression.coefficients.size(); ++index) {
        const RegressionCoefficient& coefficient = regression.coefficients[index];
        const double f_statistic = coefficient.t_statistic * coefficient.t_statistic;
        const DoeAnovaRow row = {
            labels[index - 1],
            f_statistic * regression.error_mean_square,
            1,
            regression.error_mean_square,
            f_statistic,
            f_right_tail(f_statistic, 1.0,
                         static_cast<double>(result.residual_degrees_of_freedom))};
        result.anova_rows.push_back(row);
        if (index - 1 < model_label_count) {
            result.model_anova_rows.push_back(row);
        } else {
            result.block_anova_rows.push_back(row);
        }
    }
    const DoeAnovaRow error_row = {
        "Error", regression.error_sum_of_squares,
        result.residual_degrees_of_freedom, regression.error_mean_square, 0.0,
        std::nullopt};
    result.anova_rows.push_back(error_row);
    const DoeAnovaRow total_row = {
        "Total", regression.total_sum_of_squares, regression.observation_count - 1,
        regression.total_sum_of_squares
            / static_cast<double>(std::max<std::size_t>(1, regression.observation_count - 1)),
        0.0, std::nullopt};
    result.anova_rows.push_back(total_row);

    // Replicates at identical coded points estimate pure error independently
    // of the fitted model and are the denominator for lack-of-fit diagnostics.
    struct ReplicateAccumulator {
        double sum = 0.0;
        double squared_sum = 0.0;
        std::size_t count = 0;
    };
    std::map<std::vector<int>, ReplicateAccumulator> replicate_groups;
    for (std::size_t index = 0; index < source_rows.size(); ++index) {
        const std::size_t source_row = source_rows[index];
        const DoeRun& run = design.runs[source_row];
        ReplicateAccumulator& group = replicate_groups[run.coded_levels];
        group.sum += usable_responses[index];
        group.squared_sum += usable_responses[index] * usable_responses[index];
        ++group.count;
    }
    double pure_error_ss = 0.0;
    std::size_t pure_error_df = 0;
    bool has_replicates = false;
    for (const auto& [point, group] : replicate_groups) {
        if (group.count > 1) {
            has_replicates = true;
            pure_error_ss += group.squared_sum
                - group.sum * group.sum / static_cast<double>(group.count);
            pure_error_df += group.count - 1;
        }
    }
    if (has_replicates) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                       "replicated_doe_runs",
                       "检测到重复因子组合，已用于纯误差估计。");
    }
    if (pure_error_df > 0) {
        const double pure_error_ms = pure_error_ss
            / static_cast<double>(pure_error_df);
        const DoeAnovaRow pure_error_row = {
            "Pure Error", pure_error_ss, pure_error_df, pure_error_ms, 0.0,
            std::nullopt};
        result.pure_error_anova_row = pure_error_row;
    } else {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                       "insufficient_pure_error",
                       "没有重复运行，无法估计纯误差和失拟。");
    }
    if (pure_error_df > 0
        && result.residual_degrees_of_freedom >= pure_error_df) {
        const std::size_t lack_of_fit_df =
            result.residual_degrees_of_freedom - pure_error_df;
        const double lack_of_fit_ss = std::max(
            0.0, regression.error_sum_of_squares - pure_error_ss);
        if (lack_of_fit_df > 0) {
            const double lack_of_fit_ms = lack_of_fit_ss
                / static_cast<double>(lack_of_fit_df);
            const double pure_error_ms = pure_error_ss
                / static_cast<double>(pure_error_df);
            const double f_statistic = pure_error_ms > 0.0
                ? lack_of_fit_ms / pure_error_ms : 0.0;
            const std::optional<double> lack_of_fit_p_value =
                pure_error_ms > 0.0
                    ? std::optional<double>(f_right_tail(
                        f_statistic,
                        static_cast<double>(lack_of_fit_df),
                        static_cast<double>(pure_error_df)))
                    : std::nullopt;
            const DoeAnovaRow lack_of_fit_row = {
                "Lack of Fit", lack_of_fit_ss, lack_of_fit_df, lack_of_fit_ms,
                f_statistic, lack_of_fit_p_value};
            result.lack_of_fit_anova_row = lack_of_fit_row;
        } else {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "insufficient_lack_of_fit_degrees_of_freedom",
                           "纯误差占用了全部残差自由度，无法检验失拟。");
        }
    }

    double factorial_sum = 0.0;
    std::size_t factorial_count = 0;
    double center_sum = 0.0;
    double center_squared_sum = 0.0;
    for (std::size_t index = 0; index < source_rows.size(); ++index) {
        const std::size_t source_row = source_rows[index];
        const DoeRun& run = design.runs[source_row];
        if (run.center_point) {
            center_sum += usable_responses[index];
            center_squared_sum += usable_responses[index] * usable_responses[index];
        } else {
            factorial_sum += usable_responses[index];
            ++factorial_count;
        }
    }
    result.center_points.count = 0;
    for (const std::size_t source_row : source_rows) {
        if (design.runs[source_row].center_point) {
            ++result.center_points.count;
        }
    }
    result.center_points.degrees_of_freedom =
        result.center_points.count > 0 ? result.center_points.count - 1 : 0;
    if (result.center_points.count > 0) {
        result.center_points.mean = center_sum
            / static_cast<double>(result.center_points.count);
        result.center_points.sum_of_squares = center_squared_sum
            - center_sum * center_sum
                / static_cast<double>(result.center_points.count);
    }
    if (factorial_count > 0 && result.center_points.count > 0) {
        result.curvature.factorial_mean = factorial_sum
            / static_cast<double>(factorial_count);
        result.curvature.center_mean = result.center_points.mean;
        result.curvature.difference = result.curvature.center_mean
            - result.curvature.factorial_mean;
        result.curvature.sum_of_squares =
            result.curvature.difference * result.curvature.difference
            / (1.0 / static_cast<double>(factorial_count)
               + 1.0 / static_cast<double>(result.center_points.count));
        result.curvature.degrees_of_freedom = 1;
        if (pure_error_df > 0 && pure_error_ss > 0.0) {
            result.curvature.available = true;
            result.curvature.f_statistic = result.curvature.sum_of_squares
                / (pure_error_ss / static_cast<double>(pure_error_df));
            result.curvature.p_value = f_right_tail(
                result.curvature.f_statistic, 1.0,
                static_cast<double>(pure_error_df));
        } else {
            add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::warning,
                           "insufficient_curvature_error",
                           "中心点与角点均存在，但没有可用纯误差无法检验曲率。");
        }
    } else if (result.center_points.count == 0) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                       "no_center_points", "设计没有中心点，未执行曲率检验。");
    }
    for (std::size_t index = 0; index < regression.observations.size(); ++index) {
        const RegressionObservation& observation = regression.observations[index];
        result.residuals.push_back({
            source_rows[index], observation.response, observation.fitted,
            observation.residual, observation.standardized_residual});
    }

    std::vector<double> effect_magnitudes;
    for (std::size_t index = 1; index < result.effects.size(); ++index) {
        effect_magnitudes.push_back(std::abs(result.effects[index]));
    }
    if (result.residual_degrees_of_freedom > 0
        && !result.t_statistics.empty()) {
        result.pareto_method = "standardized_t";
        result.pareto_reference = student_t_quantile(
            0.975, static_cast<double>(result.residual_degrees_of_freedom));
    } else if (!effect_magnitudes.empty()) {
        result.lenth_pse = lenth_pseudo_standard_error(
            std::vector<double>(result.effects.begin() + 1, result.effects.end()));
        const double effect_df = std::max(
            1.0, static_cast<double>(effect_magnitudes.size()) / 3.0);
        result.pareto_reference = student_t_quantile(0.975, effect_df)
            * result.lenth_pse;
        result.pareto_method = "lenth_pse";
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                       "lenth_pse_unreplicated",
                       "误差自由度为 0，标准化效应 Pareto 使用 Lenth PSE 参考线。");
    }
    return result;
}

double predict_coded_response(
    const DoeResponseAnalysisResult& fit,
    const DoeFactorialDesign& design,
    const std::vector<double>& coded)
{
    double predicted = 0.0;
    for (std::size_t term = 0; term < fit.term_names.size()
         && term < fit.coefficients.size(); ++term) {
        const std::string& name = fit.term_names[term];
        const double coefficient = fit.coefficients[term];
        if (name == "Constant") {
            predicted += coefficient;
            continue;
        }
        if (name.rfind("Block[", 0) == 0) {
            continue;
        }
        const auto star = name.find('*');
        if (star == std::string::npos) {
            for (std::size_t factor = 0; factor < design.factors.size()
                 && factor < coded.size(); ++factor) {
                if (design.factors[factor].name == name) {
                    predicted += coefficient * coded[factor];
                    break;
                }
            }
            continue;
        }
        const std::string first = name.substr(0, star);
        const std::string second = name.substr(star + 1);
        double first_level = 0.0;
        double second_level = 0.0;
        for (std::size_t factor = 0; factor < design.factors.size()
             && factor < coded.size(); ++factor) {
            if (design.factors[factor].name == first) {
                first_level = coded[factor];
            }
            if (design.factors[factor].name == second) {
                second_level = coded[factor];
            }
        }
        predicted += coefficient * first_level * second_level;
    }
    return predicted;
}

DoeCodedGrid evaluate_coded_grid(
    const DoeResponseAnalysisResult& fit,
    const DoeFactorialDesign& design,
    const std::size_t x_factor_index,
    const std::size_t y_factor_index,
    const std::size_t resolution,
    const std::vector<double>* hold_coded)
{
    DoeCodedGrid grid;
    grid.x_factor_index = x_factor_index;
    grid.y_factor_index = y_factor_index;
    if (design.factors.size() < 2) {
        add_diagnostic(grid.diagnostics, DiagnosticMessage::Severity::info,
                       "contour_requires_two_factors",
                       "等值线/曲面图需要至少两个连续因子。");
        return grid;
    }
    if (x_factor_index >= design.factors.size()
        || y_factor_index >= design.factors.size()
        || x_factor_index == y_factor_index) {
        add_diagnostic(grid.diagnostics, DiagnosticMessage::Severity::error,
                       "invalid_contour_factors",
                       "等值线因子索引无效。");
        return grid;
    }
    if (fit.coefficients.empty() || fit.term_names.empty()) {
        add_diagnostic(grid.diagnostics, DiagnosticMessage::Severity::error,
                       "missing_factorial_coefficients",
                       "没有可用于求值的析因系数。");
        return grid;
    }
    add_diagnostic(grid.diagnostics, DiagnosticMessage::Severity::info,
                   "factorial_contour_no_quadratic",
                   "二水平模型无平方项，等值线为双线性面，不能表示曲率。");
    std::vector<double> coded(design.factors.size(), 0.0);
    if (hold_coded != nullptr && hold_coded->size() == design.factors.size()) {
        coded = *hold_coded;
    }
    bool any_nonzero_hold = false;
    for (std::size_t factor = 0; factor < design.factors.size(); ++factor) {
        if (factor != x_factor_index && factor != y_factor_index) {
            grid.held_factor_names.push_back(design.factors[factor].name);
            grid.held_coded_values.push_back(coded[factor]);
            if (std::abs(coded[factor]) > 1.0e-12) {
                any_nonzero_hold = true;
            }
        }
    }
    if (!grid.held_factor_names.empty()) {
        if (any_nonzero_hold) {
            add_diagnostic(grid.diagnostics, DiagnosticMessage::Severity::info,
                           "contour_factors_held_at_actual",
                           "未作图的因子按指定实际单位 hold（已转换为编码）求值。");
        } else {
            add_diagnostic(grid.diagnostics, DiagnosticMessage::Severity::info,
                           "contour_factors_held_at_zero",
                           "未作图的因子在编码 0 处保持不变。");
        }
    }
    const std::size_t steps = std::max<std::size_t>(2, resolution);
    grid.x.resize(steps);
    grid.y.resize(steps);
    for (std::size_t index = 0; index < steps; ++index) {
        const double fraction =
            static_cast<double>(index) / static_cast<double>(steps - 1);
        grid.x[index] = -1.0 + 2.0 * fraction;
        grid.y[index] = -1.0 + 2.0 * fraction;
    }
    grid.z.assign(steps, std::vector<double>(steps, 0.0));
    for (std::size_t row = 0; row < steps; ++row) {
        for (std::size_t column = 0; column < steps; ++column) {
            coded[x_factor_index] = grid.x[column];
            coded[y_factor_index] = grid.y[row];
            grid.z[row][column] = predict_coded_response(fit, design, coded);
        }
    }
    return grid;
}

std::vector<double> resolve_contour_hold_coded(
    const DoeFactorialDesign& design,
    const std::size_t x_factor_index,
    const std::size_t y_factor_index,
    const std::map<std::string, std::string>& hold_actual,
    std::vector<std::string>& held_actual_values,
    std::vector<DiagnosticMessage>& diagnostics)
{
    std::vector<double> coded(design.factors.size(), 0.0);
    held_actual_values.clear();
    for (std::size_t factor = 0; factor < design.factors.size(); ++factor) {
        if (factor == x_factor_index || factor == y_factor_index) {
            continue;
        }
        const auto& name = design.factors[factor].name;
        const auto found = hold_actual.find(name);
        if (found == hold_actual.end() || found->second.empty()) {
            held_actual_values.push_back("");
            continue;
        }
        held_actual_values.push_back(found->second);
        const auto& low = design.factors[factor].low_level;
        const auto& high = design.factors[factor].high_level;
        double low_value = 0.0;
        double high_value = 0.0;
        double actual_value = 0.0;
        const bool low_ok = parse_finite_number(low, low_value);
        const bool high_ok = parse_finite_number(high, high_value);
        const bool actual_ok = parse_finite_number(found->second, actual_value);
        if (low_ok && high_ok && actual_ok) {
            if (!(std::abs(high_value - low_value) > 0.0)) {
                add_diagnostic(diagnostics, DiagnosticMessage::Severity::warning,
                               "invalid_hold_levels",
                               "因子高低水平相等，hold 回退编码 0。");
                coded[factor] = 0.0;
                continue;
            }
            double coded_value = 2.0 * (actual_value - low_value) / (high_value - low_value) - 1.0;
            if (coded_value < -1.0 || coded_value > 1.0) {
                add_diagnostic(diagnostics, DiagnosticMessage::Severity::warning,
                               "hold_out_of_range",
                               "hold 实际值超出高低水平，已 clamp 到编码 [-1,1]。");
                coded_value = std::clamp(coded_value, -1.0, 1.0);
            }
            coded[factor] = coded_value;
            continue;
        }
        if (found->second == low) {
            coded[factor] = -1.0;
        } else if (found->second == high) {
            coded[factor] = 1.0;
        } else {
            add_diagnostic(diagnostics, DiagnosticMessage::Severity::warning,
                           "invalid_hold_value",
                           "hold 实际值无法匹配高低水平，该因子回退编码 0。");
            coded[factor] = 0.0;
        }
    }
    for (const auto& [name, value] : hold_actual) {
        if (value.empty()) {
            continue;
        }
        bool known = false;
        for (std::size_t factor = 0; factor < design.factors.size(); ++factor) {
            if (design.factors[factor].name != name) {
                continue;
            }
            known = true;
            if (factor == x_factor_index || factor == y_factor_index) {
                add_diagnostic(diagnostics, DiagnosticMessage::Severity::info,
                               "hold_ignored_axis_factor",
                               "轴因子的 hold 条目已忽略。");
            }
            break;
        }
        if (!known) {
            add_diagnostic(diagnostics, DiagnosticMessage::Severity::warning,
                           "unknown_hold_factor",
                           "hold 中出现未知因子名，已忽略。");
        }
    }
    return coded;
}

}  // namespace datalab::domain::statistics
