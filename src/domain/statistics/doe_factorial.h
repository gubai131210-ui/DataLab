#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <cstdint>
#include <map>
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
    std::vector<double> standard_errors;
    std::vector<double> t_statistics;
    double lenth_pse = 0.0;
    double pareto_reference = 0.0;
    std::string pareto_method;
    std::vector<DoeAnovaRow> anova_rows;
    std::vector<DoeResidualObservation> residuals;
    double residual_sum_of_squares = 0.0;
    std::size_t residual_degrees_of_freedom = 0;
    double residual_mean_square = 0.0;
    std::vector<std::vector<double>> xtx_inverse;
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
    std::string mid_level;  // optional; Taguchi L9 coded 0
};

struct DoeDesignOptions {
    std::vector<DoeFactor> factors;
    std::size_t center_point_count = 0;
    std::size_t block_count = 1;
    bool randomize = false;
    std::uint64_t random_seed = 0;
    // 0 = full factorial 2^k. p > 0 → 2^(k-p) with default or custom generators.
    std::size_t fraction_p = 0;
    // Optional override, e.g. "D=ABC;E=ABD". Empty → built-in default table.
    std::string generators_text;
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
    std::string design_kind = "full";
    std::size_t fraction_p = 0;
    int resolution = 0;
    std::vector<std::string> generators;
    std::vector<std::string> defining_relation;
    std::vector<std::string> alias_lines;
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

// Generates a 2-level full (p=0) or fractional (p>0) factorial with optional
// center points. Fractional designs use default highest-resolution generators
// for supported (k,p), or options.generators_text when provided.
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

struct DoeCodedGrid {
    std::size_t x_factor_index = 0;
    std::size_t y_factor_index = 1;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<std::vector<double>> z;
    std::vector<std::string> held_factor_names;
    std::vector<std::string> held_actual_values;
    std::vector<double> held_coded_values;
    std::vector<DiagnosticMessage> diagnostics;
};

// Evaluates the fitted coded bilinear model on a regular [-1, 1] grid.
// Non-axis factors use hold_coded when provided (size == factors); otherwise 0.
// Does not change response-optimizer ±1 prediction.
DoeCodedGrid evaluate_coded_grid(
    const DoeResponseAnalysisResult& fit,
    const DoeFactorialDesign& design,
    std::size_t x_factor_index = 0,
    std::size_t y_factor_index = 1,
    std::size_t resolution = 25,
    const std::vector<double>* hold_coded = nullptr);

// Converts actual-unit hold map to coded values for non-axis factors.
// Axis factors listed in holds are ignored with a diagnostic.
// Returns coded vector sized like design.factors (axis slots left 0).
std::vector<double> resolve_contour_hold_coded(
    const DoeFactorialDesign& design,
    std::size_t x_factor_index,
    std::size_t y_factor_index,
    const std::map<std::string, std::string>& hold_actual,
    std::vector<std::string>& held_actual_values,
    std::vector<DiagnosticMessage>& diagnostics);

}  // namespace datalab::domain::statistics
