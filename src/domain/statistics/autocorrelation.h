#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace datalab::domain::statistics {

struct AcfPacfResult {
    std::vector<double> lags;
    std::vector<double> acf;
    std::vector<double> pacf;
    std::size_t n = 0;
    std::size_t missing_count = 0;
    std::size_t max_lag = 0;
    double alpha = 0.05;
    double band_half_width = 0.0;
    std::string confidence_band_method = "white_noise_fixed";
    std::optional<double> ljung_box_statistic;
    std::optional<double> ljung_box_p_value;
    std::vector<DiagnosticMessage> diagnostics;
};

// ACF/PACF with NIST white-noise fixed confidence bands ±z/√n.
AcfPacfResult compute_acf_pacf(
    const std::vector<double>& series,
    std::size_t max_lag = 0,
    double alpha = 0.05);

}  // namespace datalab::domain::statistics
