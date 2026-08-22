#pragma once

// Fixed-covariate Cox proportional hazards (Breslow ties, formula_reference).
// algorithm_id=cox_ph_fixed_covariates — not Fine-Gray, not ALT, not vendor_oracle.

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct CoxRegressionCoefficient {
    std::string term;
    double beta = 0.0;
    double standard_error = 0.0;
    double z_statistic = 0.0;
    double p_value = 0.0;
    double hazard_ratio = 0.0;
    double confidence_lower = 0.0;
    double confidence_upper = 0.0;
};

struct CoxRegressionResult {
    std::size_t n = 0;
    std::size_t events = 0;
    std::size_t censored = 0;
    bool converged = false;
    double log_likelihood = 0.0;
    std::string ties_method = "breslow";
    std::string evidence_type = "formula_reference";
    std::string algorithm_id = "cox_ph_fixed_covariates";
    std::vector<CoxRegressionCoefficient> coefficients;
    std::vector<std::size_t> source_rows;
    std::vector<DiagnosticMessage> diagnostics;
};

// Exact + right censoring only. times/events/covariates rows must align 1:1.
CoxRegressionResult fit_cox_regression(
    const std::vector<double>& times,
    const std::vector<bool>& events,
    const std::vector<std::vector<double>>& covariates,
    const std::vector<std::string>& covariate_labels = {},
    const std::vector<std::size_t>& source_rows = {},
    double confidence_level = 0.95,
    const std::string& ties_method = "breslow",
    std::size_t max_iterations = 100,
    double tolerance = 1.0e-8);

}  // namespace datalab::domain::statistics
