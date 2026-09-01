#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct NonlinearRegressionOptions {
    std::string model_id = "growth";
    std::string algorithm = "gn";
    std::vector<double> starting_values;
    std::size_t max_iterations = 100;
    double tolerance = 1.0e-6;
    double lm_lambda = 0.01;
};

struct NonlinearParameterEstimate {
    std::string name;
    double estimate = 0.0;
    double standard_error = 0.0;
    std::optional<double> lower_ci;
    std::optional<double> upper_ci;
};

struct NonlinearRegressionResult {
    std::size_t observation_count = 0;
    std::string model_id;
    std::string algorithm;
    bool converged = false;
    std::size_t iteration_count = 0;
    double sse = 0.0;
    std::size_t error_df = 0;
    double mse = 0.0;
    double s = 0.0;
    double r_squared = 0.0;
    std::vector<NonlinearParameterEstimate> parameters;
    std::vector<std::size_t> observation_source_rows;
    std::string algorithm_id = "nonlinear_regression_gn_lm";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

NonlinearRegressionResult fit_nonlinear_regression(
    const std::vector<double>& response,
    const std::vector<double>& predictor,
    const std::vector<std::size_t>& source_rows = {},
    const NonlinearRegressionOptions& options = {});

inline NonlinearRegressionResult nonlinear_regression_fit(
    const std::vector<double>& response,
    const std::vector<double>& predictor,
    const std::vector<std::size_t>& source_rows = {},
    const NonlinearRegressionOptions& options = {})
{
    return fit_nonlinear_regression(response, predictor, source_rows, options);
}

}  // namespace datalab::domain::statistics
