#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class ResponseGoal {
    maximize,
    minimize,
    target
};

struct InteractionCoefficient {
    std::string first_factor;
    std::string second_factor;
    double coefficient = 0.0;
};

// Coefficients use the coded-variable model y = b0 + sum(bi*xi) +
// sum(bij*xi*xj), where xi is normally -1 or +1.
struct ResponseModel {
    std::string response_name;
    std::vector<std::string> factor_names;
    double intercept = 0.0;
    std::vector<double> main_effect_coefficients;
    std::vector<InteractionCoefficient> interaction_coefficients;
    double residual_standard_error = 0.0;
    double residual_degrees_of_freedom = 0.0;
    std::size_t observation_count = 0;
    double confidence_level = 0.95;
    // Optional covariance matrix in the order: intercept, main effects,
    // interactions in interaction_coefficients order.
    std::vector<std::vector<double>> coefficient_covariance;
};

struct PredictionInterval {
    double confidence_lower = 0.0;
    double confidence_upper = 0.0;
    double prediction_lower = 0.0;
    double prediction_upper = 0.0;
    double standard_error = 0.0;
    double prediction_standard_error = 0.0;
    double critical_value = 0.0;
};

struct ResponsePrediction {
    std::string response_name;
    std::vector<int> coded_levels;
    double predicted_value = 0.0;
    std::optional<PredictionInterval> interval;
    std::vector<DiagnosticMessage> diagnostics;
};

struct ResponseObjective {
    std::string response_name;
    ResponseGoal goal = ResponseGoal::maximize;
    double lower = 0.0;
    double upper = 1.0;
    double target = 0.0;
    double weight = 1.0;
};

struct OptimizationCandidate {
    std::vector<int> coded_levels;
    std::vector<ResponsePrediction> predictions;
    std::vector<double> desirabilities;
    double overall_desirability = 0.0;
};

struct ResponseOptimizationResult {
    std::vector<OptimizationCandidate> candidates;
    std::optional<OptimizationCandidate> best_candidate;
    std::vector<DiagnosticMessage> diagnostics;
};

// Validates the model shape, factor names, interval settings, and covariance.
std::vector<DiagnosticMessage> diagnose_response_model(const ResponseModel& model);

// Predicts one response at a coded 2-level point. Levels must be -1 or +1.
ResponsePrediction predict_response(
    const ResponseModel& model,
    const std::vector<int>& coded_levels);

// Enumerates all 2^factor_count coded combinations and ranks them by desirability.
// For maximize/minimize, lower/upper define the useful range. For target,
// lower <= target <= upper defines the acceptable range around the target.
ResponseOptimizationResult optimize_response_desirability(
    const std::vector<ResponseModel>& models,
    const std::vector<ResponseObjective>& objectives);

}  // namespace datalab::domain::statistics
