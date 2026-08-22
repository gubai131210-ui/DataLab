#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace datalab::domain::statistics {

struct CcfResult {
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::size_t max_lag = 0;
    double alpha = 0.05;
    double band_half_width = 0.0;
    std::vector<double> lags;
    std::vector<double> ccf;
    std::vector<DiagnosticMessage> diagnostics;
};

CcfResult compute_ccf(
    const std::vector<double>& x,
    const std::vector<double>& y,
    std::size_t max_lag = 0,
    double alpha = 0.05);

}  // namespace datalab::domain::statistics
