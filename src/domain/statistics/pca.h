#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <vector>

namespace datalab::domain::statistics {

// Selects the matrix from which principal components are extracted.
enum class PcaMode {
    covariance,
    standardized
};

struct PcaOptions {
    PcaMode mode = PcaMode::covariance;
    std::size_t component_count = 0;  // Zero means retain all components.
    std::size_t max_iterations = 100;
    double tolerance = 1.0e-10;
    double anomaly_quantile = 0.99;
};

struct PcaResult {
    PcaMode mode = PcaMode::covariance;
    std::size_t observation_count = 0;
    std::size_t variable_count = 0;
    std::size_t retained_component_count = 0;

    std::vector<std::size_t> valid_rows;
    std::vector<std::size_t> constant_columns;
    std::vector<double> means;
    std::vector<double> scales;
    std::vector<std::vector<double>> covariance_matrix;
    std::vector<std::vector<double>> correlation_matrix;

    // Eigenvectors/coefficients are stored by column: coefficients[variable][component].
    // loadings are correlation-style v * sqrt(λ), not Minitab "coefficients".
    std::vector<double> eigenvalues;
    std::vector<double> explained_variance_ratio;
    std::vector<double> cumulative_explained_variance_ratio;
    std::vector<std::vector<double>> coefficients;
    std::vector<std::vector<double>> loadings;
    std::vector<std::vector<double>> scores;

    std::vector<double> hotelling_t2;
    std::vector<double> q_residuals;
    double hotelling_t2_limit = 0.0;
    double q_residual_limit = 0.0;
    std::vector<bool> hotelling_t2_anomaly;
    std::vector<bool> q_residual_anomaly;
    std::vector<bool> anomaly;

    bool converged = false;
    std::vector<DiagnosticMessage> diagnostics;
};

// Computes PCA for row-oriented data. Non-finite values cause the complete row
// to be excluded and are reported in diagnostics. The returned row-wise
// scores and diagnostics use the original row order only for valid_rows.
PcaResult principal_component_analysis(
    const std::vector<std::vector<double>>& rows,
    const PcaOptions& options = {});

// Short alias retained for callers that use the statistical operation name.
PcaResult pca(
    const std::vector<std::vector<double>>& rows,
    const PcaOptions& options = {});

}  // namespace datalab::domain::statistics
