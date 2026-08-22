#include "domain/statistics/autocorrelation.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const char* code,
    const char* message)
{
    diagnostics.push_back({severity, code, message});
}

double normal_quantile_two_sided(double alpha)
{
    // Approximate inverse normal for common α; α=0.05 → 1.95996398454.
    if (alpha <= 0.01) {
        return 2.57582930355;
    }
    if (alpha >= 0.1) {
        return 1.64485362695;
    }
    return 1.95996398454;
}

double chi_square_sf(double statistic, double df)
{
    // Upper tail via incomplete gamma regularized; reuse erfc-based for df=1..large
    // by Wilson–Hilferty approximation for p-value when df is moderate.
    if (!(statistic >= 0.0) || !(df > 0.0)) {
        return 1.0;
    }
    const double h = 2.0 / (9.0 * df);
    const double z = (std::cbrt(statistic / df) - (1.0 - h)) / std::sqrt(h);
    return 0.5 * std::erfc(z / std::sqrt(2.0));
}

}  // namespace

AcfPacfResult compute_acf_pacf(
    const std::vector<double>& series,
    std::size_t max_lag,
    double alpha)
{
    AcfPacfResult result;
    result.alpha = alpha > 0.0 && alpha < 1.0 ? alpha : 0.05;
    result.confidence_band_method = "white_noise_fixed";

    std::vector<double> values;
    values.reserve(series.size());
    for (const double value : series) {
        if (std::isfinite(value)) {
            values.push_back(value);
        } else {
            ++result.missing_count;
        }
    }
    result.n = values.size();
    if (result.n < 3) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "acf_insufficient_n",
                       "ACF/PACF 至少需要 3 个有效观测。");
        return result;
    }

    if (max_lag == 0) {
        max_lag = std::min<std::size_t>(40, result.n / 4);
        if (max_lag < 1) {
            max_lag = 1;
        }
    }
    if (max_lag >= result.n) {
        max_lag = result.n - 1;
    }
    result.max_lag = max_lag;
    result.band_half_width =
        normal_quantile_two_sided(result.alpha) / std::sqrt(static_cast<double>(result.n));

    add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::info,
                   "acf_confidence_band_white_noise",
                   "默认置信带为 NIST 白噪声固定带宽 ±z/√n（独立性检验）；"
                   "非 ARIMA 识别用的 Bartlett 变带宽。");

    const double mean =
        std::accumulate(values.cbegin(), values.cend(), 0.0) / static_cast<double>(result.n);
    double denom = 0.0;
    for (const double value : values) {
        const double d = value - mean;
        denom += d * d;
    }
    if (!(denom > 0.0)) {
        add_diagnostic(result.diagnostics, DiagnosticMessage::Severity::error,
                       "acf_zero_variance",
                       "序列方差为 0，无法计算自相关。");
        return result;
    }

    result.lags.reserve(max_lag + 1);
    result.acf.reserve(max_lag + 1);
    result.pacf.reserve(max_lag + 1);
    result.lags.push_back(0.0);
    result.acf.push_back(1.0);
    result.pacf.push_back(1.0);

    std::vector<double> r(max_lag + 1, 0.0);
    r[0] = 1.0;
    for (std::size_t lag = 1; lag <= max_lag; ++lag) {
        double numer = 0.0;
        for (std::size_t t = 0; t + lag < result.n; ++t) {
            numer += (values[t] - mean) * (values[t + lag] - mean);
        }
        r[lag] = numer / denom;
        result.lags.push_back(static_cast<double>(lag));
        result.acf.push_back(r[lag]);
    }

    // Durbin–Levinson PACF.
    std::vector<std::vector<double>> phi(max_lag + 1, std::vector<double>(max_lag + 1, 0.0));
    std::vector<double> pacf(max_lag + 1, 0.0);
    pacf[0] = 1.0;
    if (max_lag >= 1) {
        phi[1][1] = r[1];
        pacf[1] = r[1];
        double v = 1.0 - r[1] * r[1];
        for (std::size_t k = 2; k <= max_lag; ++k) {
            double numer = r[k];
            for (std::size_t j = 1; j < k; ++j) {
                numer -= phi[k - 1][j] * r[k - j];
            }
            if (!(v > 1.0e-15)) {
                pacf[k] = 0.0;
                break;
            }
            phi[k][k] = numer / v;
            pacf[k] = phi[k][k];
            for (std::size_t j = 1; j < k; ++j) {
                phi[k][j] = phi[k - 1][j] - phi[k][k] * phi[k - 1][k - j];
            }
            v *= (1.0 - phi[k][k] * phi[k][k]);
        }
    }
    for (std::size_t lag = 1; lag <= max_lag; ++lag) {
        result.pacf.push_back(pacf[lag]);
    }

    double q = 0.0;
    for (std::size_t lag = 1; lag <= max_lag; ++lag) {
        q += (r[lag] * r[lag]) / static_cast<double>(result.n - lag);
    }
    q *= static_cast<double>(result.n) * static_cast<double>(result.n + 2);
    result.ljung_box_statistic = q;
    result.ljung_box_p_value = chi_square_sf(q, static_cast<double>(max_lag));
    return result;
}

}  // namespace datalab::domain::statistics
