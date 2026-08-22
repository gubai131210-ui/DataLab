#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct AcceptanceOcPoint {
    double fraction_defective = 0.0;
    double probability_accept = 0.0;
};

struct AcceptanceSamplingResult {
    std::size_t sample_size = 0;
    std::size_t acceptance_number = 0;
    std::optional<std::size_t> lot_size;
    std::string model = "binomial";
    std::vector<AcceptanceOcPoint> oc_curve;
    std::optional<double> pa_at_aql;
    std::optional<double> pa_at_rql;
    std::optional<double> aql;
    std::optional<double> rql;
    std::vector<DiagnosticMessage> diagnostics;
};

// Binomial OC: Pa(p)=P(X<=c), X~Bin(n,p). Optional AQL/RQL for risk points.
AcceptanceSamplingResult acceptance_sampling_binomial(
    std::size_t sample_size,
    std::size_t acceptance_number,
    std::optional<double> aql = std::nullopt,
    std::optional<double> rql = std::nullopt,
    std::optional<std::size_t> lot_size = std::nullopt);

struct AnomGroup {
    std::string label;
    std::size_t n = 0;
    double mean = 0.0;
    bool outside_limits = false;
};

struct AnomResult {
    std::vector<AnomGroup> groups;
    double overall_mean = 0.0;
    double pooled_sd = 0.0;
    double udl = 0.0;
    double ldl = 0.0;
    double alpha = 0.05;
    std::size_t total_n = 0;
    std::string decision_limit_method = "nelson_normal_approx";
    std::vector<DiagnosticMessage> diagnostics;
};

// One-way normal ANOM: response groups (already split). Equal-n preferred.
AnomResult analysis_of_means(
    const std::vector<std::vector<double>>& groups,
    const std::vector<std::string>& labels = {},
    double alpha = 0.05);

struct PoissonGofResult {
    std::size_t n = 0;
    double lambda_hat = 0.0;
    double chi_square = 0.0;
    double degrees_of_freedom = 0.0;
    std::optional<double> p_value;
    std::size_t category_count = 0;
    std::size_t expected_below_five_count = 0;
    std::string validity_status = "ok";
    std::vector<DiagnosticMessage> diagnostics;
};

// Counts as non-negative integers (doubles that are whole). Estimate λ=mean.
PoissonGofResult poisson_goodness_of_fit(const std::vector<double>& counts);

}  // namespace datalab::domain::statistics
