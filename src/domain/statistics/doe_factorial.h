#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct DoeAnovaRow {
    std::string source;
    double sum_of_squares = 0.0;
    std::size_t degrees_of_freedom = 0;
    double mean_square = 0.0;
    double f_statistic = 0.0;
    std::optional<double> p_value;
};

struct DoeResidualObservation {
    std::size_t run_index = 0;
    double response = 0.0;
    double fitted = 0.0;
    double residual = 0.0;
    double standardized_residual = 0.0;
};

struct DoeCenterPointSummary {
    std::size_t count = 0;
    double mean = 0.0;
    double sum_of_squares = 0.0;
    std::size_t degrees_of_freedom = 0;
};

struct DoeCurvatureTest {
    bool available = false;
    double factorial_mean = 0.0;
    double center_mean = 0.0;
    double difference = 0.0;
    double sum_of_squares = 0.0;
    std::size_t degrees_of_freedom = 0;
    double f_statistic = 0.0;
    std::optional<double> p_value;
};

struct DoeResponseAnalysisResult {
    std::string response_name;
    std::vector<std::string> term_names;
    std::vector<double> coefficients;
    std::vector<double> effects;
    std::vector<DoeAnovaRow> anova_rows;
    std::vector<DoeResidualObservation> residuals;
    double residual_sum_of_squares = 0.0;
    std::size_t residual_degrees_of_freedom = 0;
    double residual_mean_square = 0.0;
    double r_squared = 0.0;
    std::vector<DoeAnovaRow> model_anova_rows;
    std::vector<DoeAnovaRow> block_anova_rows;
    std::optional<DoeAnovaRow> pure_error_anova_row;
    std::optional<DoeAnovaRow> lack_of_fit_anova_row;
    DoeCenterPointSummary center_points;
    DoeCurvatureTest curvature;
    std::vector<DiagnosticMessage> diagnostics;
};

struct DoeFactor {
    std::string name;
    std::string low_level;
    std::string high_level;
};

struct DoeDesignOptions {
    std::vector<DoeFactor> factors;
    std::size_t center_point_count = 0;
    std::size_t block_count = 1;
    bool randomize = false;
    std::uint64_t random_seed = 0;
};

struct DoeRun {
    std::size_t standard_order = 0;
    std::size_t run_order = 0;
    std::size_t block = 1;
    bool center_point = false;
    std::vector<int> coded_levels;
};

struct DoeFactorialDesign {
    std::vector<DoeFactor> factors;
    std::vector<DoeRun> runs;
    std::vector<DiagnosticMessage> diagnostics;
};

struct DoeValidationResult {
    bool valid = false;
    std::vector<DiagnosticMessage> diagnostics;
};

struct DoeEffectSummary {
    std::string term;
    double effect = 0.0;
    double coefficient = 0.0;
    double positive_mean = 0.0;
    double negative_mean = 0.0;
    std::size_t positive_count = 0;
    std::size_t negative_count = 0;
};

struct DoeEffectSummaryResult {
    std::vector<DoeEffectSummary> effects;
    std::vector<DiagnosticMessage> diagnostics;
};

// Generates a complete 2-level factorial design with optional center points.
DoeFactorialDesign generate_2_level_factorial(const DoeDesignOptions& options);

// Shuffles run order using a reproducible seed and updates run_order.
void randomize_design(DoeFactorialDesign& design, std::uint64_t seed);

// Assigns runs to numbered blocks in round-robin order.
void assign_blocks(DoeFactorialDesign& design, std::size_t block_count);

// Checks factor metadata, run shape, coding, uniqueness, and expected coverage.
DoeValidationResult validate_design(const DoeFactorialDesign& design);

// Summarizes all main effects and two-factor interactions for the supplied responses.
DoeEffectSummaryResult summarize_effects(
    const DoeFactorialDesign& design,
    const std::vector<double>& responses);

// Fits the coded main-effect and two-factor interaction model to a design.
// Non-finite responses are omitted; the result retains their diagnostics.
DoeResponseAnalysisResult fit_response_analysis(
    const DoeFactorialDesign& design,
    const std::vector<double>& responses,
    const std::string& response_name = {});

}  // namespace datalab::domain::statistics
