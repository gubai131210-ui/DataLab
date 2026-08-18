#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class AnovaFactorEncoding {
    reference,
    effect
};

struct TwoFactorAnovaInput {
    std::vector<std::string> factor_a;
    std::vector<std::string> factor_b;
    std::vector<double> response;
    AnovaFactorEncoding encoding = AnovaFactorEncoding::reference;
    std::vector<std::size_t> source_rows;
};

struct AnovaEffectResult {
    std::string term;
    std::optional<double> sequential_sum_of_squares;
    std::optional<double> adjusted_sum_of_squares;
    std::size_t degrees_of_freedom = 0;
    std::optional<double> mean_square;
    std::optional<double> f_statistic;
    std::optional<double> p_value;
    bool estimable = true;
    std::string estimability = "estimable";
};

struct AnovaFactorMean {
    std::string level;
    std::size_t count = 0;
    double mean = 0.0;
};

struct AnovaInteractionMean {
    std::string factor_a_level;
    std::string factor_b_level;
    std::size_t count = 0;
    double mean = 0.0;
};

struct TwoFactorAnovaResult {
    std::size_t observation_count = 0;
    std::size_t omitted_observation_count = 0;
    QualityEvidence evidence;
    double grand_mean = 0.0;
    double total_sum_of_squares = 0.0;
    double error_sum_of_squares = 0.0;
    std::size_t error_degrees_of_freedom = 0;
    double error_mean_square = 0.0;
    std::vector<AnovaEffectResult> effects;
    std::vector<AnovaFactorMean> factor_a_means;
    std::vector<AnovaFactorMean> factor_b_means;
    std::vector<AnovaInteractionMean> interaction_means;
    std::vector<DiagnosticMessage> diagnostics;
};

TwoFactorAnovaResult two_factor_anova(const TwoFactorAnovaInput& input);

}  // namespace datalab::domain::statistics
