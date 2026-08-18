#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/hypothesis_tests.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct RankSumResult {
    std::size_t first_count = 0;
    std::size_t second_count = 0;
    double rank_sum = 0.0;
    double z_statistic = 0.0;
    std::optional<double> p_value;
    std::optional<double> p_value_without_tie_correction;
    std::optional<double> location_difference;
    bool tie_correction = false;
    bool continuity_correction = true;
    std::string approximation = "normal";
    std::optional<double> effect_size;
    bool small_sample_warning = false;
    std::vector<DiagnosticMessage> diagnostics;
};

struct SignedRankResult {
    std::size_t count = 0;
    double positive_rank_sum = 0.0;
    double negative_rank_sum = 0.0;
    double z_statistic = 0.0;
    std::optional<double> p_value;
    bool continuity_correction = true;
    std::string approximation = "normal";
    bool tie_correction = false;
    bool small_sample_warning = false;
    std::vector<DiagnosticMessage> diagnostics;
};

struct KruskalWallisGroup {
    std::string label;
    std::size_t count = 0;
    double median = 0.0;
    double mean_rank = 0.0;
    std::optional<double> z_value;
};

struct KruskalWallisResult {
    std::vector<KruskalWallisGroup> groups;
    double h_statistic = 0.0;
    double adjusted_h_statistic = 0.0;
    double degrees_of_freedom = 0.0;
    std::optional<double> p_value;
    std::optional<double> p_value_unadjusted;
    bool tie_correction = false;
    bool small_sample_warning = false;
    std::string approximation = "chi_square";
    std::optional<double> effect_size;
    std::vector<DiagnosticMessage> diagnostics;
};

RankSumResult mann_whitney(
    const std::vector<double>& first,
    const std::vector<double>& second,
    TestAlternative alternative = TestAlternative::two_sided);

SignedRankResult wilcoxon_signed_rank(
    const std::vector<double>& first,
    const std::vector<double>& second,
    TestAlternative alternative = TestAlternative::two_sided);

KruskalWallisResult kruskal_wallis(
    const std::vector<std::vector<double>>& groups,
    const std::vector<std::string>& labels = {});

}  // namespace datalab::domain::statistics
