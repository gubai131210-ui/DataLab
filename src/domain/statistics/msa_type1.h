#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <vector>

namespace datalab::domain::statistics {

struct MsaType1Result {
    std::size_t count = 0;
    double mean = 0.0;
    double standard_deviation = 0.0;
    double bias = 0.0;
    double bias_standard_error = 0.0;
    double t_statistic = 0.0;
    double p_value = 1.0;
    double bias_ci_lower = 0.0;
    double bias_ci_upper = 0.0;
    double degrees_of_freedom = 0.0;
    double cg = 0.0;
    double cgk = 0.0;
    double percent_tolerance = 0.0;
    bool inference_available = false;
    std::vector<RuleEvidence> rules;
    std::vector<DiagnosticMessage> diagnostics;
};

struct BiasLinearityLevel {
    double reference = 0.0;
    std::size_t valid_count = 0;
    double bias = 0.0;
    std::vector<std::size_t> source_rows;
};

struct BiasLinearityResult {
    double intercept = 0.0;
    double slope = 0.0;
    double slope_standard_error = 0.0;
    double slope_ci_lower = 0.0;
    double slope_ci_upper = 0.0;
    double r_squared = 0.0;
    double bias_at_low = 0.0;
    double bias_at_high = 0.0;
    std::vector<BiasLinearityLevel> levels;
    std::vector<RuleEvidence> rules;
    std::vector<DiagnosticMessage> diagnostics;
};

struct StabilityResult {
    double center = 0.0;
    double sigma = 0.0;
    double lower_control_limit = 0.0;
    double upper_control_limit = 0.0;
    std::vector<double> values;
    std::vector<std::size_t> out_of_control;
    std::vector<std::size_t> source_rows;
    std::vector<std::vector<int>> triggered_tests;
    std::vector<int> primary_test_by_point;
    std::string limit_source = "estimated_individuals";
    std::vector<RuleEvidence> rules;
    std::vector<DiagnosticMessage> diagnostics;
};

MsaType1Result msa_type1(const std::vector<double>& measurements,
                         double reference,
                         double tolerance = 0.0,
                         double confidence_level = 0.95);
BiasLinearityResult bias_linearity(const std::vector<double>& references,
                                   const std::vector<double>& measurements,
                                   double confidence_level = 0.95);
StabilityResult gage_stability(const std::vector<double>& measurements);

}  // namespace datalab::domain::statistics
