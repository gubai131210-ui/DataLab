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
    std::optional<double> location_estimate;
    std::optional<double> ci_lower;
    std::optional<double> ci_upper;
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

struct DunnComparison {
    std::string first_label;
    std::string second_label;
    double mean_rank_difference = 0.0;
    double standard_error = 0.0;
    double z_statistic = 0.0;
    std::optional<double> p_value;
    std::optional<double> adjusted_p_value;
    bool significant = false;
};

struct KruskalWallisResult {
    std::vector<KruskalWallisGroup> groups;
    std::vector<DunnComparison> dunn_comparisons;
    double h_statistic = 0.0;
    double adjusted_h_statistic = 0.0;
    double degrees_of_freedom = 0.0;
    std::optional<double> p_value;
    std::optional<double> p_value_unadjusted;
    bool tie_correction = false;
    bool small_sample_warning = false;
    std::string approximation = "chi_square";
    std::optional<double> effect_size;
    double family_alpha = 0.05;
    std::vector<DiagnosticMessage> diagnostics;
};

RankSumResult mann_whitney(
    const std::vector<double>& first,
    const std::vector<double>& second,
    TestAlternative alternative = TestAlternative::two_sided,
    double confidence_level = 0.95);

SignedRankResult wilcoxon_signed_rank(
    const std::vector<double>& first,
    const std::vector<double>& second,
    TestAlternative alternative = TestAlternative::two_sided);

KruskalWallisResult kruskal_wallis(
    const std::vector<std::vector<double>>& groups,
    const std::vector<std::string>& labels = {});

// Approximate Steel–Dwass: pairwise Wilcoxon Z + asymptotic Tukey–Kramer critical.
// Reuses DunnComparison fields (Z / P / significant). formula_reference only.
std::vector<DunnComparison> steel_dwass_pairwise(
    const std::vector<std::vector<double>>& groups,
    const std::vector<std::string>& labels = {},
    double family_alpha = 0.05);

struct FriedmanTreatment {
    std::string label;
    std::size_t count = 0;
    double median = 0.0;
    double mean_rank = 0.0;
};

struct FriedmanResult {
    std::vector<FriedmanTreatment> treatments;
    double s_statistic = 0.0;
    double adjusted_s_statistic = 0.0;
    double degrees_of_freedom = 0.0;
    std::optional<double> p_value;
    bool tie_correction = false;
    std::size_t block_count = 0;
    std::size_t treatment_count = 0;
    std::string approximation = "chi_square";
    std::vector<DiagnosticMessage> diagnostics;
};

// Stacked complete-case: one observation per (block, treatment). Unbalanced → error.
FriedmanResult friedman_test(
    const std::vector<double>& responses,
    const std::vector<std::string>& treatments,
    const std::vector<std::string>& blocks);

// Approximate Nemenyi on Friedman mean ranks: SE=√(k(k+1)/(6b)),
// significant ⇔ |Z| ≥ q_{α,k,∞}/√2 (same asymptotic family as Steel–Dwass).
std::vector<DunnComparison> nemenyi_pairwise(
    const FriedmanResult& friedman,
    double family_alpha = 0.05);

struct McNemarResult {
    std::size_t a = 0;  // +/+
    std::size_t b = 0;  // +/-
    std::size_t c = 0;  // -/+
    std::size_t d = 0;  // -/-
    std::size_t discordant = 0;
    std::size_t pair_count = 0;
    double chi_square = 0.0;
    double degrees_of_freedom = 1.0;
    std::optional<double> p_value;
    bool continuity_correction = true;
    std::string method = "edwards";
    std::string first_positive_label;
    std::string second_positive_label;
    std::vector<DiagnosticMessage> diagnostics;
};

// Paired binary labels (same length). Edwards continuity correction when b+c>0.
McNemarResult mcnemar_test(
    const std::vector<std::string>& first,
    const std::vector<std::string>& second);

struct SignTestResult {
    std::size_t n_nonzero = 0;
    std::size_t n_positive = 0;
    std::size_t n_negative = 0;
    std::size_t n_ties = 0;
    double hypothesized_median = 0.0;
    std::optional<double> sample_median;
    std::optional<double> p_value;
    std::string approximation = "binomial_exact";
    bool small_sample_warning = false;
    std::vector<DiagnosticMessage> diagnostics;
};

// One-sample sign test vs hypothesized_median (ties dropped).
SignTestResult sign_test(
    const std::vector<double>& values,
    double hypothesized_median = 0.0,
    TestAlternative alternative = TestAlternative::two_sided);

// Paired: first - second vs 0 (complete-case pairs already aligned by caller).
SignTestResult sign_test_paired(
    const std::vector<double>& first,
    const std::vector<double>& second,
    TestAlternative alternative = TestAlternative::two_sided);

}  // namespace datalab::domain::statistics
