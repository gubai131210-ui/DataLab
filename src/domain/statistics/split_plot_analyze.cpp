#include "domain/statistics/split_plot_analyze.h"

#include "domain/statistics/hypothesis_tests.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
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

struct Observation {
    double y = 0.0;
    std::string htc;
    std::string etc_a;
    std::string etc_b;
    std::string wp;
    std::size_t source_row = 0;
};

double mean_of(const std::vector<double>& values)
{
    if (values.empty()) {
        return 0.0;
    }
    return std::accumulate(values.cbegin(), values.cend(), 0.0)
        / static_cast<double>(values.size());
}

}  // namespace

SplitPlotAnalyzeResult split_plot_analyze(
    const std::vector<double>& response,
    const std::vector<std::string>& htc_factor,
    const std::vector<std::string>& etc_factor_a,
    const std::vector<std::string>& whole_plot_ids,
    const std::vector<std::string>& etc_factor_b,
    const std::vector<std::size_t>& source_rows,
    const SplitPlotAnalyzeOptions& options)
{
    SplitPlotAnalyzeResult result;
    result.include_htc_etc_interaction = options.include_htc_etc_interaction;
    result.include_etc_interaction = options.include_etc_interaction;

    if (response.size() < 4 || response.size() != htc_factor.size()
        || response.size() != etc_factor_a.size()
        || response.size() != whole_plot_ids.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "length_mismatch", "响应、难改/易改因子与 WP 列长度须一致。");
        return result;
    }
    const bool two_etc = !etc_factor_b.empty();
    if (two_etc && etc_factor_b.size() != response.size()) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "etc_b_length_mismatch", "第二易改因子列长度须与响应一致。");
        return result;
    }

    std::vector<Observation> observations;
    std::set<std::string> htc_levels;
    std::set<std::string> etc_a_levels;
    std::set<std::string> etc_b_levels;
    std::set<std::string> wp_levels;

    for (std::size_t index = 0; index < response.size(); ++index) {
        if (!std::isfinite(response[index]) || htc_factor[index].empty()
            || etc_factor_a[index].empty() || whole_plot_ids[index].empty()) {
            continue;
        }
        if (two_etc && etc_factor_b[index].empty()) {
            continue;
        }
        Observation obs;
        obs.y = response[index];
        obs.htc = htc_factor[index];
        obs.etc_a = etc_factor_a[index];
        obs.etc_b = two_etc ? etc_factor_b[index] : std::string{};
        obs.wp = whole_plot_ids[index];
        obs.source_row = index < source_rows.size() ? source_rows[index] : index;
        observations.push_back(obs);
        htc_levels.insert(obs.htc);
        etc_a_levels.insert(obs.etc_a);
        wp_levels.insert(obs.wp);
        if (two_etc) {
            etc_b_levels.insert(obs.etc_b);
        }
    }

    result.observation_count = observations.size();
    result.whole_plot_count = wp_levels.size();
    if (htc_levels.size() < 2 || etc_a_levels.size() < 2 || wp_levels.size() < 2
        || observations.size() < 4) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "insufficient_split_plot",
                       "裂区分析需要难改/易改因子各≥2 水平且至少 2 个 whole plot。");
        return result;
    }

    const double grand_mean = mean_of([&] {
        std::vector<double> values;
        values.reserve(observations.size());
        for (const auto& obs : observations) {
            values.push_back(obs.y);
        }
        return values;
    }());

    std::map<std::string, double> htc_means;
    std::map<std::string, double> etc_a_means;
    std::map<std::string, double> wp_means;
    std::map<std::string, std::vector<double>> wp_values;
    for (const auto& obs : observations) {
        htc_means[obs.htc] += obs.y;
        etc_a_means[obs.etc_a] += obs.y;
        wp_means[obs.wp] += obs.y;
        wp_values[obs.wp].push_back(obs.y);
    }
    for (auto& [level, sum] : htc_means) {
        const std::size_t count = std::count_if(
            observations.cbegin(), observations.cend(),
            [&](const Observation& obs) { return obs.htc == level; });
        sum = count > 0 ? sum / static_cast<double>(count) : 0.0;
    }
    for (auto& [level, sum] : etc_a_means) {
        const std::size_t count = std::count_if(
            observations.cbegin(), observations.cend(),
            [&](const Observation& obs) { return obs.etc_a == level; });
        sum = count > 0 ? sum / static_cast<double>(count) : 0.0;
    }
    for (auto& [level, sum] : wp_means) {
        sum /= static_cast<double>(wp_values[level].size());
    }

    double ss_htc = 0.0;
    for (const auto& [level, mean] : htc_means) {
        const std::size_t count = std::count_if(
            observations.cbegin(), observations.cend(),
            [&](const Observation& obs) { return obs.htc == level; });
        ss_htc += static_cast<double>(count) * (mean - grand_mean) * (mean - grand_mean);
    }
    double ss_etc_a = 0.0;
    for (const auto& [level, mean] : etc_a_means) {
        const std::size_t count = std::count_if(
            observations.cbegin(), observations.cend(),
            [&](const Observation& obs) { return obs.etc_a == level; });
        ss_etc_a += static_cast<double>(count) * (mean - grand_mean) * (mean - grand_mean);
    }
    double ss_wp = 0.0;
    for (const auto& [wp, mean] : wp_means) {
        ss_wp += static_cast<double>(wp_values[wp].size())
            * (mean - grand_mean) * (mean - grand_mean);
    }

    double ss_total = 0.0;
    for (const auto& obs : observations) {
        ss_total += (obs.y - grand_mean) * (obs.y - grand_mean);
    }

    const std::size_t df_htc = htc_levels.size() - 1;
    const std::size_t df_etc_a = etc_a_levels.size() - 1;
    const std::size_t df_wp_error = wp_levels.size() > htc_levels.size()
        ? wp_levels.size() - htc_levels.size() : 1;
    const std::size_t df_sp_error = observations.size() > wp_levels.size()
        ? observations.size() - wp_levels.size() : 1;

    const double ms_htc = df_htc > 0 ? ss_htc / static_cast<double>(df_htc) : 0.0;
    const double ms_etc_a = df_etc_a > 0 ? ss_etc_a / static_cast<double>(df_etc_a) : 0.0;
    const double ss_wp_error = std::max(0.0, ss_wp - ss_htc);
    const double ss_sp_error = std::max(0.0, ss_total - ss_wp - ss_etc_a);
    const double ms_wp_error = df_wp_error > 0
        ? ss_wp_error / static_cast<double>(df_wp_error) : 0.0;
    const double ms_sp_error = df_sp_error > 0
        ? ss_sp_error / static_cast<double>(df_sp_error) : 0.0;

    auto add_effect = [&](const std::string& term, const std::string& layer,
                          double ss, std::size_t df, double ms_num, double ms_den) {
        SplitPlotAnovaEffect effect;
        effect.term = term;
        effect.error_layer = layer;
        effect.sum_of_squares = ss;
        effect.degrees_of_freedom = df;
        effect.mean_square = df > 0
            ? std::optional<double>(ss / static_cast<double>(df))
            : std::nullopt;
        if (ms_den > 0.0 && df > 0) {
            effect.f_statistic = ms_num / ms_den;
            effect.p_value = f_right_tail(
                *effect.f_statistic, static_cast<double>(df),
                ms_den == ms_wp_error
                    ? static_cast<double>(df_wp_error)
                    : static_cast<double>(df_sp_error));
        }
        result.anova_effects.push_back(effect);
    };

    add_effect("Hard-to-change", "WP", ss_htc, df_htc, ms_htc, ms_wp_error);
    add_effect("Easy-to-change A", "SP", ss_etc_a, df_etc_a, ms_etc_a, ms_sp_error);
    SplitPlotAnovaEffect wp_error;
    wp_error.term = "WP Error";
    wp_error.error_layer = "WP";
    wp_error.sum_of_squares = ss_wp_error;
    wp_error.degrees_of_freedom = df_wp_error;
    wp_error.mean_square = ms_wp_error;
    result.anova_effects.push_back(wp_error);
    SplitPlotAnovaEffect sp_error;
    sp_error.term = "SP Error";
    sp_error.error_layer = "SP";
    sp_error.sum_of_squares = ss_sp_error;
    sp_error.degrees_of_freedom = df_sp_error;
    sp_error.mean_square = ms_sp_error;
    result.anova_effects.push_back(sp_error);

    std::map<std::string, double> wp_htc_mean;
    for (const auto& obs : observations) {
        wp_htc_mean[obs.wp + "|" + obs.htc] += obs.y;
    }
    std::map<std::string, std::size_t> wp_htc_count;
    for (const auto& obs : observations) {
        ++wp_htc_count[obs.wp + "|" + obs.htc];
    }
    for (auto& [key, sum] : wp_htc_mean) {
        sum /= static_cast<double>(wp_htc_count[key]);
    }

    for (const auto& obs : observations) {
        SplitPlotFitRow row;
        row.source_row = obs.source_row;
        row.observed = obs.y;
        row.whole_plot_id = obs.wp;
        row.fitted = htc_means[obs.htc] + etc_a_means[obs.etc_a] - grand_mean;
        row.residual = obs.y - row.fitted;
        const double wp_full = wp_means[obs.wp];
        const double wp_fixed = htc_means[obs.htc];
        row.whole_plot_residual = wp_full - wp_fixed;
        result.fits.push_back(row);
        result.observation_source_rows.push_back(obs.source_row);
    }

    if (ss_total > 0.0) {
        result.wp_r_squared = ss_htc / ss_total;
        result.sp_r_squared = (ss_htc + ss_etc_a) / ss_total;
    }

    return result;
}

}  // namespace datalab::domain::statistics
