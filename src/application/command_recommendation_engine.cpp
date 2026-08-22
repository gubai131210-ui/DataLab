#include "application/command_recommendation_engine.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace datalab::application {
namespace {

struct TypeCounts {
    int numeric = 0;
    int categorical = 0;
    int time = 0;
    int unknown = 0;
};

TypeCounts count_types(const std::vector<datalab::domain::ColumnType>& column_types)
{
    TypeCounts counts;
    for (const datalab::domain::ColumnType type : column_types) {
        switch (type) {
        case datalab::domain::ColumnType::numeric:
            ++counts.numeric;
            break;
        case datalab::domain::ColumnType::categorical:
            ++counts.categorical;
            break;
        case datalab::domain::ColumnType::time:
            ++counts.time;
            break;
        case datalab::domain::ColumnType::unknown:
        default:
            ++counts.unknown;
            break;
        }
    }
    return counts;
}

void add_candidate(
    std::vector<Recommendation>* out,
    const char* command_id,
    double score,
    const char* reason_key)
{
    out->push_back(Recommendation{command_id, score, reason_key});
}

bool intent_allows_id(CommandWizardIntent intent, const std::string& command_id)
{
    if (intent == CommandWizardIntent::any) {
        return true;
    }

    static const std::unordered_map<CommandWizardIntent, std::unordered_set<std::string>> k_allowed = {
        {CommandWizardIntent::describe,
         {"descriptive", "histogram", "normality_test", "boxplot", "pareto", "chi_square"}},
        {CommandWizardIntent::compare,
         {"two_sample_t", "mann_whitney", "variance_test", "one_way_anova", "kruskal_wallis",
          "boxplot"}},
        {CommandWizardIntent::associate,
         {"correlation", "regression", "scatter_plot", "pca"}},
        {CommandWizardIntent::control_chart, {"imr"}},
        {CommandWizardIntent::capability, {"capability", "nonnormal_capability"}},
        {CommandWizardIntent::reliability, {"reliability", "cox_regression"}},
        {CommandWizardIntent::graph,
         {"histogram", "boxplot", "scatter_plot", "time_series_plot"}},
    };

    const auto it = k_allowed.find(intent);
    if (it == k_allowed.end()) {
        return false;
    }
    return it->second.count(command_id) > 0;
}

std::vector<Recommendation> merge_and_rank(
    std::vector<Recommendation> candidates,
    CommandWizardIntent intent,
    std::size_t top_n)
{
    std::unordered_map<std::string, Recommendation> best;
    for (Recommendation& item : candidates) {
        if (!intent_allows_id(intent, item.command_id)) {
            continue;
        }
        auto found = best.find(item.command_id);
        if (found == best.end() || item.score > found->second.score) {
            best[item.command_id] = std::move(item);
        }
    }

    std::vector<Recommendation> ranked;
    ranked.reserve(best.size());
    for (auto& entry : best) {
        ranked.push_back(std::move(entry.second));
    }
    std::sort(ranked.begin(), ranked.end(), [](const Recommendation& a, const Recommendation& b) {
        if (a.score != b.score) {
            return a.score > b.score;
        }
        return a.command_id < b.command_id;
    });

    if (ranked.size() > top_n) {
        ranked.resize(top_n);
    }
    return ranked;
}

void append_graph_rules(const TypeCounts& counts, std::vector<Recommendation>* out)
{
    if (counts.time >= 1) {
        add_candidate(out, "time_series_plot", 1.0, "reason.graph_explore");
    }
    if (counts.numeric >= 2) {
        add_candidate(out, "scatter_plot", 0.95, "reason.graph_explore");
    }
    if (counts.numeric >= 1) {
        add_candidate(out, "histogram", 0.9, "reason.graph_explore");
        add_candidate(out, "boxplot", 0.85, "reason.graph_explore");
    }
    if (counts.numeric == 1 && counts.categorical >= 1) {
        add_candidate(out, "boxplot", 0.92, "reason.graph_explore");
    }
}

void append_structural_rules(
    const TypeCounts& counts,
    CommandWizardIntent intent,
    std::vector<Recommendation>* out)
{
    if (counts.numeric == 1 && counts.categorical == 0) {
        if (intent == CommandWizardIntent::any || intent == CommandWizardIntent::describe) {
            add_candidate(out, "descriptive", 1.0, "reason.univariate_describe");
            add_candidate(out, "histogram", 0.95, "reason.univariate_describe");
            add_candidate(out, "normality_test", 0.9, "reason.univariate_describe");
            add_candidate(out, "boxplot", 0.85, "reason.univariate_describe");
        }
        if (intent == CommandWizardIntent::control_chart) {
            add_candidate(out, "imr", 1.0, "reason.imr");
        }
        if (intent == CommandWizardIntent::capability) {
            add_candidate(out, "capability", 1.0, "reason.capability");
            add_candidate(out, "nonnormal_capability", 0.9, "reason.capability");
        }
    }

    if (counts.numeric == 2 && counts.categorical == 0) {
        if (intent == CommandWizardIntent::any || intent == CommandWizardIntent::compare) {
            add_candidate(out, "two_sample_t", 1.0, "reason.two_sample_compare");
            add_candidate(out, "mann_whitney", 0.9, "reason.two_sample_compare");
            add_candidate(out, "variance_test", 0.85, "reason.two_sample_compare");
        }
        if (intent == CommandWizardIntent::any || intent == CommandWizardIntent::associate) {
            add_candidate(out, "correlation", 1.0, "reason.associate");
            add_candidate(out, "regression", 0.95, "reason.associate");
            add_candidate(out, "scatter_plot", 0.9, "reason.associate");
        }
    }

    if (counts.numeric == 1 && counts.categorical == 1) {
        if (intent == CommandWizardIntent::any || intent == CommandWizardIntent::compare
            || intent == CommandWizardIntent::describe) {
            add_candidate(out, "one_way_anova", 1.0, "reason.factor_group");
            add_candidate(out, "boxplot", 0.9, "reason.factor_group");
            add_candidate(out, "kruskal_wallis", 0.85, "reason.factor_group");
        }
    }

    if (counts.numeric >= 3) {
        if (intent == CommandWizardIntent::any || intent == CommandWizardIntent::associate) {
            add_candidate(out, "regression", 1.0, "reason.multivariate");
            add_candidate(out, "pca", 0.9, "reason.multivariate");
            add_candidate(out, "correlation", 0.85, "reason.multivariate");
        }
    }

    if (counts.categorical >= 1 && counts.numeric == 0) {
        if (intent == CommandWizardIntent::any || intent == CommandWizardIntent::describe
            || intent == CommandWizardIntent::compare) {
            add_candidate(out, "pareto", 1.0, "reason.categorical");
            add_candidate(out, "chi_square", 0.9, "reason.categorical");
        }
    }
}

}  // namespace

RecommendResult recommend(
    const std::vector<datalab::domain::ColumnType>& column_types,
    CommandWizardIntent intent,
    std::size_t top_n,
    const std::vector<std::string>& /*column_names*/)
{
    RecommendResult result;
    const std::size_t limit = top_n == 0 ? 8 : std::min(top_n, static_cast<std::size_t>(8));

    if (column_types.empty()) {
        result.hint_key = "hint.select_columns";
        return result;
    }

    const TypeCounts counts = count_types(column_types);
    const int typed = counts.numeric + counts.categorical + counts.time;
    if (typed == 0) {
        // 全 unknown：禁止假装能力合格；空列表 + 提示核对列类型。
        result.hint_key = "hint.check_column_types";
        return result;
    }

    std::vector<Recommendation> candidates;

    if (intent == CommandWizardIntent::reliability) {
        add_candidate(&candidates, "reliability", 1.0, "reason.reliability");
        add_candidate(&candidates, "cox_regression", 0.9, "reason.reliability");
    } else if (intent == CommandWizardIntent::graph) {
        append_graph_rules(counts, &candidates);
    } else {
        append_structural_rules(counts, intent, &candidates);
        if (intent == CommandWizardIntent::any && counts.time >= 1) {
            add_candidate(&candidates, "time_series_plot", 0.7, "reason.graph_explore");
        }
    }

    result.recommendations = merge_and_rank(std::move(candidates), intent, limit);
    if (result.recommendations.empty() && typed > 0) {
        result.hint_key = "hint.check_column_types";
    }
    return result;
}

}  // namespace datalab::application
