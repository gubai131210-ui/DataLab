#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class TaguchiSnType {
    larger_is_better,
    smaller_is_better,
    nominal_is_best
};

struct TaguchiAnalyzeOptions {
    TaguchiSnType sn_type = TaguchiSnType::larger_is_better;
};

struct TaguchiRunSummary {
    std::size_t source_row = 0;
    std::vector<std::string> factor_levels;  // one per factor
    double mean = 0.0;
    std::optional<double> sn_ratio;
    std::size_t replicate_count = 0;
};

struct TaguchiLevelAverage {
    std::string level;
    double average = 0.0;
    std::size_t count = 0;
};

struct TaguchiFactorResponse {
    std::string factor_name;
    std::vector<TaguchiLevelAverage> level_averages;
    double delta = 0.0;
    int rank = 0;
};

struct TaguchiAnalyzeResult {
    TaguchiSnType sn_type = TaguchiSnType::larger_is_better;
    std::string sn_type_name = "larger";
    std::size_t factor_count = 0;
    std::size_t response_count = 0;
    std::size_t run_count = 0;
    std::vector<std::string> factor_names;
    std::vector<TaguchiRunSummary> runs;
    std::vector<TaguchiFactorResponse> means_table;
    std::vector<TaguchiFactorResponse> sn_table;
    std::string algorithm_id = "taguchi_analyze_static_sn";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

TaguchiSnType parse_taguchi_sn_type(const std::string& text);
std::string taguchi_sn_type_name(TaguchiSnType type);

// Per-run S/N from replicate responses Y1..Yn (formula_reference).
std::optional<double> sn_ratio_larger(const std::vector<double>& y);
std::optional<double> sn_ratio_smaller(const std::vector<double>& y);
std::optional<double> sn_ratio_nominal(const std::vector<double>& y);
std::optional<double> sn_ratio(
    const std::vector<double>& y, TaguchiSnType type);

// factor_levels[run][factor], responses[run][replicate].
TaguchiAnalyzeResult analyze_taguchi_static(
    const std::vector<std::vector<std::string>>& factor_levels,
    const std::vector<std::vector<double>>& responses,
    const std::vector<std::string>& factor_names,
    const std::vector<std::size_t>& source_rows = {},
    const TaguchiAnalyzeOptions& options = {});

}  // namespace datalab::domain::statistics
