#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace datalab::domain::statistics {

enum class CorrelationMethod {
    pearson,
    spearman
};

struct CorrelationPairResult {
    std::size_t first_column = 0;
    std::size_t second_column = 0;
    std::size_t count = 0;
    double coefficient = 0.0;
    std::optional<double> p_value;
    std::optional<double> confidence_lower;
    std::optional<double> confidence_upper;
    std::vector<DiagnosticMessage> diagnostics;
};

struct CorrelationResult {
    CorrelationMethod method = CorrelationMethod::pearson;
    std::vector<std::vector<double>> coefficients;
    std::vector<std::vector<std::size_t>> counts;
    std::vector<CorrelationPairResult> pairs;
    double confidence_level = 0.95;
    std::vector<DiagnosticMessage> diagnostics;
};

CorrelationResult correlation_matrix(
    const std::vector<std::vector<double>>& columns,
    CorrelationMethod method = CorrelationMethod::pearson,
    double confidence_level = 0.95);

}  // namespace datalab::domain::statistics
