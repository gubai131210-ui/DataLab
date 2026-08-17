#pragma once

#include "domain/quality_types.h"

#include <cstddef>
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

struct AttributeAgreementResult {
    std::size_t item_count = 0;
    std::size_t evaluator_count = 0;
    std::size_t rating_count = 0;
    std::size_t missing_rating_count = 0;
    double confidence_level = 0.95;
    std::vector<AttributeEvaluatorAgreement> within_evaluator;
    std::vector<AttributePairAgreement> between_evaluator;
    std::vector<AttributeStandardAgreement> against_standard;
    std::vector<DiagnosticMessage> diagnostics;
};

// Ratings are aligned row-wise with items and evaluators. Empty strings are
// treated as missing ratings. Standards may be empty, or one label per item.
AttributeAgreementResult attribute_agreement(
    const std::vector<std::string>& ratings,
    const std::vector<std::string>& items,
    const std::vector<std::string>& evaluators,
    const std::vector<std::string>& standards = {},
    double confidence_level = 0.95);

}  // namespace datalab::domain::statistics
