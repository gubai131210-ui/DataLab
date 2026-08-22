#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/regression.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct RsmAnovaRow {
    std::string source;
    double sum_of_squares = 0.0;
    std::size_t degrees_of_freedom = 0;
    double mean_square = 0.0;
    double f_statistic = 0.0;
    std::optional<double> p_value;
};

struct RsmAnalysisResult {
    std::string response_name;
    std::vector<std::string> factor_names;
    std::vector<std::string> term_names;
    RegressionResult regression;
    std::optional<RsmAnovaRow> pure_error_anova_row;
    std::optional<RsmAnovaRow> lack_of_fit_anova_row;
    std::size_t replicate_group_count = 0;
    std::size_t center_like_replicate_count = 0;
    std::vector<DiagnosticMessage> diagnostics;
};

struct RsmCodedGrid {
    std::size_t x_factor_index = 0;
    std::size_t y_factor_index = 1;
    std::vector<double> x;
    std::vector<double> y;
    std::vector<std::vector<double>> z;
    std::vector<DiagnosticMessage> diagnostics;
};

// Fits coded second-order model: intercept + linear + 2FI + pure quadratic.
// predictors[row][factor] are coded (or will be coded from raw via code_rsm_factors).
RsmAnalysisResult fit_rsm_analysis(
    const std::vector<double>& response,
    const std::vector<std::vector<double>>& coded_factors,
    const std::vector<std::string>& factor_names,
    const std::string& response_name = {},
    const std::vector<std::size_t>& source_rows = {});

// Maps each factor column to [-1, 1] using min/max; if already in [-1,1], leave as-is.
std::vector<std::vector<double>> code_rsm_factors(
    const std::vector<std::vector<double>>& raw_factors,
    std::vector<DiagnosticMessage>& diagnostics);

// Phase 4.4: code using design low/high/center (same formula as CCD/BBD generators).
std::vector<std::vector<double>> code_rsm_factors_from_design_bounds(
    const std::vector<std::vector<double>>& raw_factors,
    const std::vector<double>& lows,
    const std::vector<double>& highs,
    const std::vector<double>& centers,
    std::vector<DiagnosticMessage>& diagnostics);

RsmCodedGrid evaluate_rsm_grid(
    const RsmAnalysisResult& fit,
    std::size_t x_factor_index,
    std::size_t y_factor_index,
    std::size_t resolution = 25,
    const std::vector<double>* hold_coded = nullptr);

}  // namespace datalab::domain::statistics
