#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class CensoringType {
    exact,
    right,
    left,
    interval
};

struct CensoringObservation {
    CensoringType type = CensoringType::exact;
    double time = 0.0;          // exact / right / left
    double interval_left = 0.0; // interval (and optional left encoding)
    double interval_right = 0.0;
    std::string time_unit;
    std::string group;
    std::string failure_mode;
    std::optional<double> exposure;
    std::size_t source_row = 0;
};

struct CensoringContractResult {
    bool ok = false;
    std::size_t exact_count = 0;
    std::size_t right_censored_count = 0;
    std::size_t left_censored_count = 0;
    std::size_t interval_censored_count = 0;
    std::size_t failure_count = 0;  // exact failures
    std::size_t valid_count = 0;
    std::string time_unit;
    std::vector<double> times_for_right_censored_km;  // exact + right only
    std::vector<bool> events_for_right_censored_km;   // true=failure
    std::vector<std::size_t> source_rows_for_km;
    std::vector<DiagnosticMessage> diagnostics;
};

std::optional<CensoringType> parse_censoring_type(const std::string& text);

// Stable id for worksheet / JSON columns (exact|right|left|interval).
std::string censoring_type_id(CensoringType type);

// Export normalized per-observation censoring state for worksheet audit trail.
// Columns: source_row_1based, time, censoring_type, event, failure_mode, group,
// interval_left, interval_right, exposure, time_unit.
DataTable censoring_observations_to_worksheet(
    const std::vector<CensoringObservation>& observations);

struct CensoringWorksheetImportResult {
    bool ok = false;
    std::vector<CensoringObservation> observations;
    std::vector<DiagnosticMessage> diagnostics;
};

// Import worksheet rows produced by censoring_observations_to_worksheet (or
// compatible column names). Does not invent missing interval bounds.
CensoringWorksheetImportResult censoring_observations_from_worksheet(
    const DataTable& table);

// Validates Phase 5 censoring contract. Right-censored KM export only includes
// exact + right; left/interval are counted but blocked for classic KM path.
CensoringContractResult validate_censoring_contract(
    const std::vector<CensoringObservation>& observations,
    bool allow_left_interval_for_km = false);

struct WarrantySummaryOptions {
    double warranty_time = 0.0;
    std::string time_unit;
    double exposure = 0.0;                 // units on field / shipped
    double reliability_at_warranty = 0.0;  // R(Tw) in [0,1]
    std::size_t observed_failures = 0;
    std::size_t censored_count = 0;
    std::size_t valid_count = 0;
    std::string model_name;                // weibull | lognormal | km | ...
    std::string design_or_dataset_id;
    bool reliability_is_prediction = true; // prediction vs observation label
};

struct WarrantySummaryResult {
    bool ok = false;
    double warranty_time = 0.0;
    std::string time_unit;
    double exposure = 0.0;
    double reliability_at_warranty = 0.0;
    double failure_probability = 0.0;      // 1 - R(Tw)
    double expected_failures = 0.0;        // exposure * F(Tw)
    double claims_per_1000 = 0.0;          // 1000 * F(Tw)
    std::size_t observed_failures = 0;
    std::size_t censored_count = 0;
    std::size_t valid_count = 0;
    std::string model_name;
    std::string quantity_label = "prediction";  // prediction | observation
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

WarrantySummaryResult summarize_warranty(const WarrantySummaryOptions& options);

struct WarrantyStratumInput {
    std::string label;
    std::string kind;  // failure_mode | group
    double exposure = 0.0;
    std::size_t observed_failures = 0;
    std::size_t censored_count = 0;
    std::size_t valid_count = 0;
    // When set for failure_mode strata, expected_failures uses this R(Tw)
    // instead of the pooled overall value (cause-specific fit required).
    std::optional<double> reliability_at_warranty;
    std::vector<std::size_t> source_rows;
};

struct WarrantyStratumResult {
    std::string label;
    std::string kind;
    double exposure = 0.0;
    std::size_t observed_failures = 0;
    std::size_t censored_count = 0;
    std::size_t valid_count = 0;
    double expected_failures = 0.0;
    double share_of_total_exposure = 0.0;
    std::string exposure_attribution;     // measured_column | proportional_scalar | zero
    std::optional<double> reliability_at_warranty;
    bool uses_mode_specific_reliability = false;
    std::vector<std::size_t> source_rows;
};

struct WarrantyStrataSummaryResult {
    bool ok = false;
    WarrantySummaryResult overall;
    std::vector<WarrantyStratumResult> strata;
    std::string stratum_kind;  // failure_mode | group
    bool uses_pooled_reliability = true;
    bool uses_mode_specific_reliability = false;
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

// Stratum expected_failures = exposure * F(Tw).
// Default F from pooled R(Tw). Optional per-stratum reliability_at_warranty
// (failure_mode only) enables cause-specific F without inventing values.
WarrantyStrataSummaryResult summarize_warranty_strata(
    const WarrantySummaryOptions& overall_options,
    const std::vector<WarrantyStratumInput>& strata);

// Cause-specific per-failure-mode fits: competing exact failures are treated as
// right-censored for the target mode. Evidence remains formula_reference
// (not vendor_oracle / golden). Left/interval rows are omitted from mode fits.
struct ReliabilityModeFitResult {
    std::string failure_mode;
    std::size_t failure_count = 0;
    std::size_t competing_failure_count = 0;
    std::size_t right_censored_count = 0;
    std::size_t valid_count = 0;
    bool identifiable = false;
    bool converged = false;
    std::optional<double> shape;
    std::optional<double> scale;
    std::optional<double> location;
    std::optional<double> rate;
    std::optional<double> median_life;
    std::optional<double> reliability_at_warranty;
    std::string not_computed_reason;
    std::string evidence_type = "formula_reference";
    std::string algorithm_id = "cause_specific_right_censored_competing";
    std::vector<std::size_t> source_rows;
    std::vector<DiagnosticMessage> diagnostics;
};

struct ReliabilityModeFitsResult {
    bool ran = false;
    std::string model;
    std::string fitting_scheme = "cause_specific";
    double warranty_time = 0.0;
    std::vector<ReliabilityModeFitResult> modes;
    std::vector<DiagnosticMessage> diagnostics;
};

ReliabilityModeFitsResult fit_reliability_by_failure_mode(
    const std::vector<CensoringObservation>& observations,
    const std::string& model,
    double warranty_time);

}  // namespace datalab::domain::statistics
