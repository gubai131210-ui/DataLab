#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct FactorAnalysisOptions {
    std::size_t factor_count = 0;  // 0 → Kaiser (eigenvalue > 1)
    bool use_kaiser_rule = true;
    bool varimax_rotation = false;
};

struct FactorLoadingRow {
    std::string variable;
    std::vector<double> loadings;
    double communality = 0.0;
};

struct FactorVarianceRow {
    std::size_t factor_index = 0;
    double eigenvalue = 0.0;
    double percent_variance = 0.0;
    double cumulative_percent = 0.0;
};

struct FactorAnalysisResult {
    std::size_t observation_count = 0;
    std::size_t variable_count = 0;
    std::size_t retained_factor_count = 0;
    bool varimax_applied = false;
    std::vector<std::string> variable_names;
    std::vector<double> eigenvalues;
    std::vector<FactorLoadingRow> loadings_table;
    std::vector<FactorVarianceRow> variance_explained;
    std::string algorithm_id = "factor_analysis_pca_extraction";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

// PCA extraction on correlation matrix; optional varimax; no Hotelling T2.
FactorAnalysisResult factor_analysis_extract(
    const std::vector<std::vector<double>>& rows,
    const std::vector<std::string>& variable_names,
    const std::vector<std::size_t>& source_rows = {},
    const FactorAnalysisOptions& options = {});

}  // namespace datalab::domain::statistics
