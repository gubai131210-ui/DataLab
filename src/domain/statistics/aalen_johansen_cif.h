#pragma once

// Aalen–Johansen nonparametric cumulative incidence (competing risks).
// formula_reference / aalen_johansen_cif — not Fine-Gray regression, not vendor_oracle,
// and not the same as cause-specific KM with competing failures censored.

#include "domain/quality_types.h"
#include "domain/statistics/censoring_contract.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct AalenJohansenCifPoint {
    double time = 0.0;
    double overall_survival = 1.0;  // S(t) after this time's events
    double cif = 0.0;
    std::size_t at_risk = 0;
    std::size_t cause_failures = 0;
    std::size_t all_failures = 0;
};

struct AalenJohansenCifModeResult {
    std::string failure_mode;
    std::size_t failure_count = 0;
    std::vector<AalenJohansenCifPoint> points;
    std::optional<double> cif_at_last_event;
    std::optional<double> cif_at_warranty;
};

struct AalenJohansenCifResult {
    bool ran = false;
    double warranty_time = 0.0;
    std::vector<AalenJohansenCifModeResult> modes;
    std::string evidence_type = "formula_reference";
    std::string algorithm_id = "aalen_johansen_cif";
    std::vector<DiagnosticMessage> diagnostics;
};

// Exact + right only; labeled exact failures define competing causes.
// Left/interval omitted (use km_interval path). warranty_time<=0 skips CIF@Tw.
AalenJohansenCifResult aalen_johansen_cif(
    const std::vector<CensoringObservation>& observations,
    double warranty_time = 0.0);

}  // namespace datalab::domain::statistics
