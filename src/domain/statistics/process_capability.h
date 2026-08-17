#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace datalab::domain::statistics {

struct ProcessCapabilityResult {
    double mean = 0.0;
    double within_standard_deviation = 0.0;
    double overall_standard_deviation = 0.0;
    std::size_t sample_size = 0;
    std::optional<double> cp;
    std::optional<double> cpl;
    std::optional<double> cpu;
    std::optional<double> cpk;
    std::optional<double> pp;
    std::optional<double> ppl;
    std::optional<double> ppu;
    std::optional<double> ppk;
    std::optional<double> observed_ppm_below;
    std::optional<double> observed_ppm_above;
    std::optional<double> observed_ppm_total;
    std::optional<double> expected_ppm_within_below;
    std::optional<double> expected_ppm_within_above;
    std::optional<double> expected_ppm_within_total;
    std::optional<double> expected_ppm_overall_below;
    std::optional<double> expected_ppm_overall_above;
    std::optional<double> expected_ppm_overall_total;
    std::vector<DiagnosticMessage> diagnostics;
};

class ProcessCapability final {
public:
    static ProcessCapabilityResult calculate(
        double mean,
        double within_standard_deviation,
        double overall_standard_deviation,
        const SpecificationLimits& specifications);

    static ProcessCapabilityResult calculate(
        const std::vector<double>& observations,
        double within_standard_deviation,
        const SpecificationLimits& specifications);
};

}  // namespace datalab::domain::statistics
