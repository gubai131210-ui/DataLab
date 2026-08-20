#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/hypothesis_tests.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct PairedTTestResult {
    std::size_t count = 0;
    double mean_difference = 0.0;
    double standardized_difference = 0.0;
    double sample_standard_deviation = 0.0;
    double standard_error = 0.0;
    double t_statistic = 0.0;
    double degrees_of_freedom = 0.0;
    std::optional<double> p_value;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    std::vector<DiagnosticMessage> diagnostics;
};

struct TukeyComparison {
    std::string first_label;
    std::string second_label;
    double mean_difference = 0.0;
    double standardized_difference = 0.0;
    double standard_error = 0.0;
    double q_statistic = 0.0;
    double confidence_lower = 0.0;
    double confidence_upper = 0.0;
    double adjusted_p_value = 1.0;
    bool significant = false;
};

struct TukeyResult {
    double confidence_level = 0.95;
    double family_confidence_level = 0.95;
    double individual_confidence_level = 0.95;
    double alpha = 0.05;
    double error_mean_square = 0.0;
    double error_degrees_of_freedom = 0.0;
    std::string method = "conservative_sidak_t_studentized_range_approximation";
    std::vector<TukeyComparison> comparisons;
    std::vector<RuleEvidence> rules;
    std::vector<DiagnosticMessage> diagnostics;
};

struct TwoProportionsResult {
    std::size_t first_events = 0;
    std::size_t first_trials = 0;
    std::size_t second_events = 0;
    std::size_t second_trials = 0;
    double first_proportion = 0.0;
    double second_proportion = 0.0;
    double difference = 0.0;
    double z_statistic = 0.0;
    std::string method = "normal";
    std::string ci_method = "wald";
    std::optional<double> p_value;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    std::optional<double> fisher_p_value;
    std::vector<DiagnosticMessage> diagnostics;
};

struct ChiSquareCell {
    std::string row_label;
    std::string column_label;
    double observed = 0.0;
    double expected = 0.0;
    double raw_residual = 0.0;
    double standardized_residual = 0.0;
    double adjusted_residual = 0.0;
    double contribution = 0.0;
};

struct ChiSquareResult {
    std::size_t rows = 0;
    std::size_t columns = 0;
    double pearson_statistic = 0.0;
    double likelihood_ratio_statistic = 0.0;
    double degrees_of_freedom = 0.0;
    std::optional<double> p_value;
    std::optional<double> likelihood_ratio_p_value;
    std::vector<ChiSquareCell> cells;
    std::vector<DiagnosticMessage> diagnostics;
};

PairedTTestResult paired_t_test(
    const std::vector<double>& first,
    const std::vector<double>& second,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided);

TukeyResult tukey_multiple_comparisons(
    const std::vector<std::vector<double>>& groups,
    const std::vector<std::string>& labels = {},
    double confidence_level = 0.95);

struct TukeyGroupingRow {
    std::string label;
    std::size_t count = 0;
    double mean = 0.0;
    std::string grouping;
};

// Compact letter display from existing Tukey pairwise `significant` flags.
// Same letter ⇒ not significantly different under the product's Tukey rule.
// Does not recompute critical values.
std::vector<TukeyGroupingRow> tukey_grouping_letters(
    const std::vector<std::string>& labels,
    const std::vector<double>& means,
    const std::vector<std::size_t>& counts,
    const std::vector<TukeyComparison>& comparisons);

TwoProportionsResult two_proportions_test(
    std::size_t first_events,
    std::size_t first_trials,
    std::size_t second_events,
    std::size_t second_trials,
    double confidence_level = 0.95,
    TestAlternative alternative = TestAlternative::two_sided,
    bool newcombe_wilson_ci = false);

ChiSquareResult chi_square_association(
    const std::vector<std::vector<double>>& observed,
    const std::vector<std::string>& row_labels = {},
    const std::vector<std::string>& column_labels = {});

struct ChiSquareGofCategory {
    std::string category;
    double observed = 0.0;
    double test_proportion = 0.0;
    double expected = 0.0;
    double residual = 0.0;
    double contribution = 0.0;
};

struct ChiSquareGofResult {
    std::size_t total_count = 0;
    double degrees_of_freedom = 0.0;
    double pearson_statistic = 0.0;
    std::optional<double> p_value;
    std::string proportion_source = "equal";
    std::size_t expected_below_five_count = 0;
    std::optional<double> minimum_expected_count;
    std::string validity_status = "ok";
    std::string recommendation;
    std::vector<ChiSquareGofCategory> categories;
    std::vector<DiagnosticMessage> diagnostics;
};

ChiSquareGofResult chi_square_goodness_of_fit(
    const std::vector<std::string>& categories,
    const std::vector<double>& counts,
    const std::vector<double>& proportions = {});

}  // namespace datalab::domain::statistics
