#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class AdfRegression {
    none,
    drift,
    trend
};

struct AdfOptions {
    AdfRegression regression = AdfRegression::drift;
    std::size_t lags = 0;  // 0 → default floor((T-1)^(1/3))
};

struct AdfCoefficient {
    std::string name;
    double estimate = 0.0;
    double standard_error = 0.0;
    double t_statistic = 0.0;
};

struct AdfResult {
    AdfRegression regression = AdfRegression::drift;
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::size_t lags = 0;
    std::size_t used_observations = 0;
    std::optional<double> gamma;
    std::optional<double> tau;
    std::optional<double> critical_1;
    std::optional<double> critical_5;
    std::optional<double> critical_10;
    bool reject_unit_root_at_5 = false;
    std::vector<AdfCoefficient> coefficients;
    std::vector<DiagnosticMessage> diagnostics;
};

AdfResult augmented_dickey_fuller(
    const std::vector<double>& series,
    const AdfOptions& options = {});

}  // namespace datalab::domain::statistics
