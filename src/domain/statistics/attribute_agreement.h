#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct AgreementEstimate {
    std::size_t valid_count = 0;
    std::size_t agreement_count = 0;
    double agreement_percent = 0.0;
    double kappa = 0.0;
    double kappa_standard_error = 0.0;
    double confidence_level = 0.95;
    double kappa_ci_low = 0.0;
    double kappa_ci_high = 0.0;
    double expected_agreement = 0.0;
    bool identifiable = true;
    std::string method = "cohen_unweighted";
    std::string variance_method = "simple_binomial";
    std::string ci_method = "normal_approximation";
};

struct AttributeEvaluatorAgreement {
    std::string evaluator;
    AgreementEstimate estimate;
};

struct AttributePairAgreement {
    std::string first_evaluator;
    std::string second_evaluator;
    AgreementEstimate estimate;
};

struct AttributeStandardAgreement {
    std::string evaluator;
    AgreementEstimate estimate;
};

struct KendallConcordanceEstimate {
    double coefficient = 0.0;
    double chi_square = 0.0;
    double degrees_of_freedom = 0.0;
    double p_value = 1.0;
    std::size_t subject_count = 0;
    std::size_t rater_count = 0;
    bool identifiable = false;
    std::string not_computed_reason;
};

struct KendallCorrelationEstimate {
    double tau = 0.0;
    double standard_error = 0.0;
    double z = 0.0;
    double p_value = 1.0;
    std::size_t pair_count = 0;
    bool identifiable = false;
    std::string not_computed_reason;
};

struct AttributeEvaluatorKendallConcordance {
    std::string evaluator;
    KendallConcordanceEstimate estimate;
};

struct AttributeStandardKendall {
    std::string evaluator;
    KendallCorrelationEstimate estimate;
};

struct AttributeAgreementResult {
    std::size_t item_count = 0;
    std::size_t evaluator_count = 0;
    std::size_t rating_count = 0;
    std::size_t missing_rating_count = 0;
    double confidence_level = 0.95;
    std::vector<AttributeEvaluatorAgreement> within_evaluator;
    std::vector<AttributePairAgreement> between_evaluator;
    std::vector<AttributeStandardAgreement> against_standard;
    AgreementEstimate overall;
    bool overall_available = false;
    bool ratings_are_ordinal = false;
    std::optional<KendallConcordanceEstimate> between_kendall;
    std::vector<AttributeEvaluatorKendallConcordance> within_kendall;
    std::vector<AttributeStandardKendall> against_standard_kendall;
    std::optional<KendallCorrelationEstimate> overall_kendall;
    std::vector<std::string> agreement_item_labels;
    std::vector<std::string> agreement_evaluator_labels;
    std::vector<std::vector<double>> agreement_percent_matrix;
    std::vector<DiagnosticMessage> diagnostics;
};

// Ratings are aligned row-wise with items and evaluators. Empty strings are
// treated as missing ratings. Standards may be empty, or one label per item.
// Kendall W/τ is computed only when ratings_are_ordinal is true.
// kappa_weight_scheme: "none" | "linear" | "quadratic" (Cohen weighted).
AttributeAgreementResult attribute_agreement(
    const std::vector<std::string>& ratings,
    const std::vector<std::string>& items,
    const std::vector<std::string>& evaluators,
    const std::vector<std::string>& standards = {},
    double confidence_level = 0.95,
    bool ratings_are_ordinal = false,
    const std::string& kappa_weight_scheme = "none");

}  // namespace datalab::domain::statistics
