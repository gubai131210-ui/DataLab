#include "domain/statistics/split_plot_design.h"

#include <algorithm>
#include <numeric>
#include <random>
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

}  // namespace

SplitPlotDesignResult generate_split_plot_design(const SplitPlotDesignOptions& options)
{
    SplitPlotDesignResult result;
    result.factors = options.factors;
    result.htc_factor_index = options.htc_factor_index;
    result.random_seed = options.random_seed;
    result.randomized = options.randomize;

    const std::size_t k = options.factors.size();
    if (k < 2 || k > 4) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "split_design_factor_count",
                       "裂区设计需要 2～4 个因子。");
        return result;
    }
    if (options.htc_factor_index >= k) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "split_design_htc_index",
                       "HTC 因子索引超出因子数。");
        return result;
    }
    if (options.whole_plot_replicates < 1) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "split_design_replicates",
                       "Whole plot 复制数至少为 1。");
        return result;
    }

    std::vector<DoeFactor> etc_factors;
    etc_factors.reserve(k - 1);
    for (std::size_t index = 0; index < k; ++index) {
        if (index != options.htc_factor_index) {
            etc_factors.push_back(options.factors[index]);
        }
    }

    DoeDesignOptions etc_options;
    etc_options.factors = etc_factors;
    etc_options.fraction_p = options.etc_fraction_p;
    const DoeFactorialDesign etc_design = generate_2_level_factorial(etc_options);
    if (etc_design.runs.empty()) {
        result.diagnostics.insert(result.diagnostics.end(),
                                  etc_design.diagnostics.cbegin(),
                                  etc_design.diagnostics.cend());
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "split_design_etc_empty", "ETC 子设计生成失败。");
        return result;
    }
    result.etc_run_count = etc_design.runs.size();

    const std::size_t htc_levels = 2;
    result.whole_plot_count = htc_levels * options.whole_plot_replicates;
    result.factor_count = k;
    result.htc_factor_name = options.factors[options.htc_factor_index].name;
    for (const auto& factor : options.factors) {
        result.factor_names.push_back(factor.name);
    }
    std::size_t standard_order = 1;
    for (std::size_t htc_level = 0; htc_level < htc_levels; ++htc_level) {
        for (std::size_t replicate = 0; replicate < options.whole_plot_replicates;
             ++replicate) {
            const std::size_t whole_plot_id =
                htc_level * options.whole_plot_replicates + replicate + 1;
            for (const DoeRun& etc_run : etc_design.runs) {
                SplitPlotDesignRun run;
                run.standard_order = standard_order++;
                run.run_order = run.standard_order;
                run.whole_plot = whole_plot_id;
                run.block = etc_run.block;
                run.point_type = etc_run.center_point ? "Center" : "Factorial";
                run.coded_levels.assign(k, 0);
                run.coded_levels[options.htc_factor_index] =
                    htc_level == 0 ? -1 : 1;
                std::size_t etc_index = 0;
                for (std::size_t factor_index = 0; factor_index < k; ++factor_index) {
                    if (factor_index == options.htc_factor_index) {
                        continue;
                    }
                    run.coded_levels[factor_index] =
                        etc_run.coded_levels[etc_index++];
                }
                run.factor_levels.resize(k);
                for (std::size_t factor_index = 0; factor_index < k; ++factor_index) {
                    const auto& factor = options.factors[factor_index];
                    run.factor_levels[factor_index] =
                        run.coded_levels[factor_index] < 0 ? factor.low_level : factor.high_level;
                }
                result.runs.push_back(std::move(run));
            }
        }
    }

    result.run_count = result.runs.size();

    if (options.randomize && result.runs.size() > 1) {
        std::mt19937_64 rng(options.random_seed);
        std::vector<std::size_t> order(result.runs.size());
        std::iota(order.begin(), order.end(), 0);
        std::shuffle(order.begin(), order.end(), rng);
        for (std::size_t index = 0; index < order.size(); ++index) {
            result.runs[order[index]].run_order = index + 1;
        }
    }

    return result;
}

}  // namespace datalab::domain::statistics
