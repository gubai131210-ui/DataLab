#include "application/report_assembly_service.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <sstream>

namespace datalab::application {
namespace {

std::string optional_double_text(const std::optional<double>& value)
{
    if (!value.has_value()) {
        return "-";
    }
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.12g", *value);
    return buffer;
}

void append_rule_list(
    std::vector<domain::RuleEvidence>& target, const std::vector<domain::RuleEvidence>& source)
{
    target.insert(target.end(), source.begin(), source.end());
}

domain::StatisticTable truncate_table(const domain::StatisticTable& table, std::size_t max_rows)
{
    domain::StatisticTable truncated = table;
    if (truncated.rows.size() <= max_rows) {
        return truncated;
    }
    truncated.rows.resize(max_rows);
    if (truncated.row_ids.size() > max_rows) {
        truncated.row_ids.resize(max_rows);
    }
    if (truncated.rule_ids.size() > max_rows) {
        truncated.rule_ids.resize(max_rows);
    }
    return truncated;
}

bool is_risk_diagnostic(const domain::DiagnosticMessage& message)
{
    using Severity = domain::DiagnosticMessage::Severity;
    return message.severity == Severity::warning || message.severity == Severity::error;
}

domain::EvidenceRef make_ref(
    const std::string& evidence_id,
    domain::EvidenceKind kind,
    domain::EvidenceRole role,
    const std::string& label_text_id)
{
    domain::EvidenceRef ref;
    ref.evidence_id = evidence_id;
    ref.kind = kind;
    ref.role = role;
    ref.label_text_id = label_text_id;
    ref.status = "present";
    return ref;
}

// Gate honesty refs never advertise vendor_oracle / golden — clamp to formula_reference.
std::string gate_evidence_type_key(const std::string& /*facts_evidence_type*/)
{
    return "evidence_type=formula_reference";
}

// Harvest formula_reference EvidenceRefs from Facts only.
// Never invent vendor_oracle / golden / reference_implementation entries.
void append_formula_ref(
    std::vector<domain::EvidenceRef>& out,
    const std::string& evidence_id,
    const std::string& evidence_type,
    const std::string& algorithm_id)
{
    if (!evidence_type.empty() && evidence_type != "formula_reference") {
        return;
    }
    domain::EvidenceRef ref = make_ref(
        evidence_id,
        domain::EvidenceKind::formula_reference,
        domain::EvidenceRole::supporting,
        "evidence.formula_reference");
    ref.formula_ref_id = algorithm_id.empty() ? "formula_reference" : algorithm_id;
    if (!algorithm_id.empty()) {
        ref.parameter_keys.push_back("algorithm_id=" + algorithm_id);
    }
    ref.parameter_keys.push_back("evidence_type=formula_reference");
    out.push_back(std::move(ref));
}

void append_gate_limiting_refs_from_facts(
    std::vector<domain::EvidenceRef>& out,
    const domain::OutputPage& page,
    const std::string& page_prefix)
{
    const domain::InterpretationFacts& facts = page.facts;

    if (facts.capability.has_value()
        && !facts.capability->pass_fail_judgment_allowed) {
        const domain::CapabilityFacts& cap = *facts.capability;
        const bool johnson_path = cap.research_preview
            || cap.gate_status == "gated_research"
            || (!cap.johnson_family.empty())
            || (cap.method.find("johnson") != std::string::npos);
        domain::EvidenceRef gate = make_ref(
            page_prefix
                + (johnson_path ? ":gate:johnson" : ":gate:capability_pass_fail_blocked"),
            domain::EvidenceKind::diagnostic,
            domain::EvidenceRole::limiting,
            johnson_path ? "evidence.johnson_capability_gated"
                         : "evidence.capability_stability_prerequisite");
        gate.diagnostic_code = johnson_path
            ? "johnson_capability_gated"
            : "capability_pass_fail_blocked_by_stability_prerequisite";
        gate.parameter_keys.push_back(
            "gate_status="
            + (cap.gate_status.empty() ? "stability_unverified" : cap.gate_status));
        gate.parameter_keys.push_back(
            "method=" + (cap.method.empty() ? "capability" : cap.method));
        gate.parameter_keys.push_back("pass_fail_judgment_allowed=false");
        gate.parameter_keys.push_back(gate_evidence_type_key(cap.evidence_type));
        if (johnson_path && !cap.johnson_family.empty()) {
            gate.parameter_keys.push_back("johnson_family=" + cap.johnson_family);
        }
        out.push_back(std::move(gate));
        for (const domain::DiagnosticMessage& diagnostic : page.diagnostics) {
            if (diagnostic.code == "johnson_spec_outside_support"
                && diagnostic.message
                       == "规格限落在 Johnson 变换定义域外，无法计算 Pp/Ppk。") {
                domain::EvidenceRef spec_gate = make_ref(
                    page_prefix + ":gate:johnson_spec_limit",
                    domain::EvidenceKind::diagnostic,
                    domain::EvidenceRole::limiting,
                    "evidence.johnson_spec_limit_gate");
                spec_gate.diagnostic_code = diagnostic.code;
                spec_gate.parameter_keys.push_back("capability_table_skipped=true");
                spec_gate.parameter_keys.push_back("evidence_type=formula_reference");
                spec_gate.parameter_keys.push_back("not_vendor_oracle=true");
                out.push_back(std::move(spec_gate));
                break;
            }
        }
    }

    if (facts.box_cox.has_value()) {
        const domain::BoxCoxFacts& box_cox = *facts.box_cox;
        const std::string assumption_status = box_cox.assumption_status.empty()
            ? "not_verified"
            : box_cox.assumption_status;
        if (assumption_status == "not_verified") {
            domain::EvidenceRef gate = make_ref(
                page_prefix + ":gate:box_cox_not_pass_fail",
                domain::EvidenceKind::diagnostic,
                domain::EvidenceRole::limiting,
                "evidence.box_cox_not_pass_fail");
            gate.diagnostic_code = "box_cox_not_pass_fail";
            gate.parameter_keys.push_back("assumption_status=" + assumption_status);
            gate.parameter_keys.push_back("pass_fail_judgment_allowed=false");
            gate.parameter_keys.push_back("evidence_type=formula_reference");
            out.push_back(std::move(gate));
        }
        for (const domain::DiagnosticMessage& diagnostic : page.diagnostics) {
            if (diagnostic.code == "box_cox_invalid_spec_limit"
                || diagnostic.code == "box_cox_spec_limits_order") {
                domain::EvidenceRef gate = make_ref(
                    page_prefix + ":gate:box_cox_spec_limit",
                    domain::EvidenceKind::diagnostic,
                    domain::EvidenceRole::limiting,
                    "evidence.box_cox_spec_limit_gate");
                gate.diagnostic_code = diagnostic.code;
                gate.parameter_keys.push_back("capability_table_skipped=true");
                gate.parameter_keys.push_back("evidence_type=formula_reference");
                gate.parameter_keys.push_back("not_vendor_oracle=true");
                out.push_back(std::move(gate));
                break;
            }
        }
    }

    if (facts.reliability.has_value()) {
        const domain::ReliabilityFacts& rel = *facts.reliability;
        const bool zero_failures = rel.failure_count.has_value()
            && *rel.failure_count == 0;
        const bool all_censored = rel.not_computed_reason == "all_censored"
            || (rel.valid_count.has_value() && rel.failure_count.has_value()
                && *rel.valid_count > 0 && *rel.failure_count == 0
                && rel.censored_count.has_value()
                && *rel.censored_count == *rel.valid_count);
        if (all_censored) {
            domain::EvidenceRef gate = make_ref(
                page_prefix + ":gate:reliability_all_censored",
                domain::EvidenceKind::diagnostic,
                domain::EvidenceRole::limiting,
                "evidence.reliability_all_censored");
            gate.diagnostic_code = "censoring_all_censored";
            gate.parameter_keys.push_back("not_computed_reason=all_censored");
            gate.parameter_keys.push_back(gate_evidence_type_key(rel.evidence_type));
            gate.parameter_keys.push_back("pass_fail_judgment_allowed=false");
            out.push_back(std::move(gate));
        } else if (zero_failures) {
            domain::EvidenceRef gate = make_ref(
                page_prefix + ":gate:reliability_zero_failure",
                domain::EvidenceKind::diagnostic,
                domain::EvidenceRole::limiting,
                "evidence.reliability_zero_failure");
            gate.diagnostic_code = "censoring_zero_failures";
            gate.parameter_keys.push_back("failure_count=0");
            gate.parameter_keys.push_back(gate_evidence_type_key(rel.evidence_type));
            gate.parameter_keys.push_back("pass_fail_judgment_allowed=false");
            out.push_back(std::move(gate));
        }

        if (!rel.cif_algorithm_id.empty()) {
            domain::EvidenceRef gate = make_ref(
                page_prefix + ":gate:cif_not_fine_gray",
                domain::EvidenceKind::diagnostic,
                domain::EvidenceRole::limiting,
                "evidence.cif_not_fine_gray");
            gate.diagnostic_code = "cif_not_fine_gray";
            gate.parameter_keys.push_back("cif_algorithm_id=" + rel.cif_algorithm_id);
            gate.parameter_keys.push_back(gate_evidence_type_key(rel.cif_evidence_type));
            gate.parameter_keys.push_back("not_vendor_oracle=true");
            gate.parameter_keys.push_back("not_fine_gray=true");
            gate.parameter_keys.push_back("not_cause_specific_reliability=true");
            out.push_back(std::move(gate));
        }

        if (!rel.fine_gray_algorithm_id.empty()) {
            domain::EvidenceRef gate = make_ref(
                page_prefix + ":gate:fine_gray_formula_reference_only",
                domain::EvidenceKind::diagnostic,
                domain::EvidenceRole::limiting,
                "evidence.fine_gray_formula_reference_only");
            gate.diagnostic_code = "fine_gray_formula_reference_only";
            gate.parameter_keys.push_back(
                "fine_gray_algorithm_id=" + rel.fine_gray_algorithm_id);
            if (!rel.fine_gray_kind.empty()) {
                gate.parameter_keys.push_back("fine_gray_kind=" + rel.fine_gray_kind);
            }
            gate.parameter_keys.push_back(
                gate_evidence_type_key(rel.fine_gray_evidence_type));
            gate.parameter_keys.push_back("not_vendor_oracle=true");
            gate.parameter_keys.push_back("not_cause_specific_cox=true");
            gate.parameter_keys.push_back("not_pinned_r_survival_finegray=true");
            out.push_back(std::move(gate));
        }
    }

    if (facts.km_interval.has_value()) {
        const domain::KmIntervalFacts& km = *facts.km_interval;
        domain::EvidenceRef gate = make_ref(
            page_prefix + ":gate:km_not_long_term_guarantee",
            domain::EvidenceKind::diagnostic,
            domain::EvidenceRole::limiting,
            "evidence.km_not_long_term_guarantee");
        gate.diagnostic_code = "km_not_long_term_guarantee";
        gate.parameter_keys.push_back(
            "gate_status="
            + (km.gate_status.empty() ? "open_with_limits" : km.gate_status));
        gate.parameter_keys.push_back(gate_evidence_type_key(km.evidence_type));
        gate.parameter_keys.push_back(
            "algorithm_id="
            + (km.algorithm_id.empty() ? "turnbull_npmle_simplified_grid"
                                       : km.algorithm_id));
        gate.parameter_keys.push_back("pass_fail_judgment_allowed=false");
        out.push_back(std::move(gate));
    }

    if (facts.warranty.has_value()) {
        const domain::WarrantyFacts& warranty = *facts.warranty;
        domain::EvidenceRef legal = make_ref(
            page_prefix + ":gate:warranty_not_legal_promise",
            domain::EvidenceKind::diagnostic,
            domain::EvidenceRole::limiting,
            "evidence.warranty_not_legal_promise");
        legal.diagnostic_code = "warranty_not_legal_promise";
        legal.parameter_keys.push_back(gate_evidence_type_key(warranty.evidence_type));
        legal.parameter_keys.push_back("pass_fail_judgment_allowed=false");
        out.push_back(std::move(legal));

        if (warranty.quantity_label.empty()
            || warranty.quantity_label == "prediction") {
            domain::EvidenceRef prediction = make_ref(
                page_prefix + ":gate:warranty_prediction_not_observation",
                domain::EvidenceKind::diagnostic,
                domain::EvidenceRole::limiting,
                "evidence.warranty_prediction_not_observation");
            prediction.diagnostic_code = "warranty_prediction_not_observation";
            prediction.parameter_keys.push_back(
                "quantity_label="
                + (warranty.quantity_label.empty() ? "prediction"
                                                   : warranty.quantity_label));
            prediction.parameter_keys.push_back("pass_fail_judgment_allowed=false");
            out.push_back(std::move(prediction));
        }

        if (!warranty.strata.empty()) {
            bool any_proportional = false;
            bool any_mode_specific = false;
            for (const domain::WarrantyStratumFacts& stratum : warranty.strata) {
                if (stratum.exposure_attribution == "proportional_scalar") {
                    any_proportional = true;
                }
                if (stratum.uses_mode_specific_reliability) {
                    any_mode_specific = true;
                }
            }
            if (any_proportional || warranty.exposure_source == "scalar") {
                domain::EvidenceRef gate = make_ref(
                    page_prefix + ":gate:warranty_strata_exposure_honesty",
                    domain::EvidenceKind::diagnostic,
                    domain::EvidenceRole::limiting,
                    "evidence.warranty_strata_exposure_honesty");
                gate.diagnostic_code = "warranty_strata_exposure_honesty";
                gate.parameter_keys.push_back(
                    "proportional_scalar="
                    + std::string(any_proportional ? "true" : "false"));
                gate.parameter_keys.push_back(
                    "exposure_source="
                    + (warranty.exposure_source.empty() ? "unknown"
                                                        : warranty.exposure_source));
                gate.parameter_keys.push_back(
                    "measured_denominator="
                    + std::string(any_proportional ? "false" : "true"));
                gate.parameter_keys.push_back(gate_evidence_type_key(warranty.evidence_type));
                gate.parameter_keys.push_back("not_vendor_oracle=true");
                out.push_back(std::move(gate));
            }

            domain::EvidenceRef basis = make_ref(
                page_prefix + ":gate:warranty_strata_reliability_basis",
                domain::EvidenceKind::diagnostic,
                domain::EvidenceRole::limiting,
                "evidence.warranty_strata_reliability_basis");
            basis.diagnostic_code = "warranty_strata_reliability_basis";
            basis.parameter_keys.push_back(
                "uses_pooled_reliability="
                + std::string(warranty.uses_pooled_reliability ? "true" : "false"));
            basis.parameter_keys.push_back(
                "uses_mode_specific_reliability="
                + std::string(
                    (warranty.uses_mode_specific_reliability || any_mode_specific)
                        ? "true"
                        : "false"));
            if (!warranty.stratum_kind.empty()) {
                basis.parameter_keys.push_back("stratum_kind=" + warranty.stratum_kind);
            }
            basis.parameter_keys.push_back(
                "strata_count=" + std::to_string(warranty.strata.size()));
            // Pooled R must not be narrated as mode-specific R(Tw).
            if (warranty.uses_pooled_reliability && !any_mode_specific) {
                basis.parameter_keys.push_back("pooled_as_mode_specific=false");
            }
            basis.parameter_keys.push_back(gate_evidence_type_key(warranty.evidence_type));
            basis.parameter_keys.push_back("not_vendor_oracle=true");
            out.push_back(std::move(basis));
        }
    }

    for (const domain::DiagnosticMessage& diagnostic : page.diagnostics) {
        if (diagnostic.code != "invalid_exposure_value"
            && diagnostic.code != "warranty_zero_exposure") {
            continue;
        }
        domain::EvidenceRef gate = make_ref(
            page_prefix + ":gate:warranty_exposure",
            domain::EvidenceKind::diagnostic,
            domain::EvidenceRole::limiting,
            "evidence.warranty_exposure_gate");
        gate.diagnostic_code = diagnostic.code;
        gate.parameter_keys.push_back("warranty_summary_skipped=true");
        gate.parameter_keys.push_back("evidence_type=formula_reference");
        gate.parameter_keys.push_back("not_vendor_oracle=true");
        gate.parameter_keys.push_back("no_silent_imputation=true");
        out.push_back(std::move(gate));
        break;
    }

    if (facts.rsm.has_value()) {
        const domain::RsmFacts& rsm = *facts.rsm;
        if (!rsm.pure_error_available) {
            domain::EvidenceRef gate = make_ref(
                page_prefix + ":gate:rsm_insufficient_pure_error",
                domain::EvidenceKind::diagnostic,
                domain::EvidenceRole::limiting,
                "evidence.rsm_insufficient_pure_error");
            gate.diagnostic_code = "rsm_insufficient_pure_error";
            gate.parameter_keys.push_back("pure_error_available=false");
            gate.parameter_keys.push_back(
                "pure_error_df=" + std::to_string(rsm.pure_error_df));
            gate.parameter_keys.push_back(
                "lack_of_fit_available="
                + std::string(rsm.lack_of_fit_available ? "true" : "false"));
            gate.parameter_keys.push_back(gate_evidence_type_key(rsm.evidence_type));
            // Honesty: residual MS must not be treated as pure error.
            gate.parameter_keys.push_back("residual_ms_as_pure_error=false");
            out.push_back(std::move(gate));
        }
        if (rsm.lack_of_fit_available) {
            domain::EvidenceRef gate = make_ref(
                page_prefix + ":gate:rsm_lof_formula_reference",
                domain::EvidenceKind::diagnostic,
                domain::EvidenceRole::limiting,
                "evidence.rsm_lof_formula_reference");
            gate.diagnostic_code = "rsm_lof_formula_reference";
            gate.parameter_keys.push_back("lack_of_fit_available=true");
            gate.parameter_keys.push_back(
                "pure_error_available="
                + std::string(rsm.pure_error_available ? "true" : "false"));
            gate.parameter_keys.push_back(
                "pure_error_df=" + std::to_string(rsm.pure_error_df));
            gate.parameter_keys.push_back(
                "lack_of_fit_df=" + std::to_string(rsm.lack_of_fit_df));
            gate.parameter_keys.push_back(gate_evidence_type_key(rsm.evidence_type));
            gate.parameter_keys.push_back("residual_ms_as_pure_error=false");
            out.push_back(std::move(gate));
        }
        if (rsm.surface_is_static) {
            domain::EvidenceRef gate = make_ref(
                page_prefix + ":gate:rsm_static_surface",
                domain::EvidenceKind::diagnostic,
                domain::EvidenceRole::limiting,
                "evidence.rsm_static_surface");
            gate.diagnostic_code = "rsm_static_surface";
            gate.parameter_keys.push_back("surface_is_static=true");
            if (!rsm.design_source_id.empty()) {
                gate.parameter_keys.push_back(
                    "design_source_id=" + rsm.design_source_id);
            }
            if (!rsm.design_kind.empty()) {
                gate.parameter_keys.push_back("design_kind=" + rsm.design_kind);
            }
            gate.parameter_keys.push_back(gate_evidence_type_key(rsm.evidence_type));
            out.push_back(std::move(gate));
        }
    }

    if (facts.design_generation.has_value()) {
        const domain::DesignGenerationFacts& design = *facts.design_generation;
        const std::string kind =
            design.design_kind.empty() ? "design" : design.design_kind;

        domain::EvidenceRef formula_only = make_ref(
            page_prefix + ":gate:design_formula_reference_only",
            domain::EvidenceKind::diagnostic,
            domain::EvidenceRole::limiting,
            "evidence.design_formula_reference_only");
        formula_only.diagnostic_code = "design_formula_reference_only";
        formula_only.parameter_keys.push_back("design_kind=" + kind);
        if (!design.design_source_id.empty()) {
            formula_only.parameter_keys.push_back(
                "design_source_id=" + design.design_source_id);
        }
        if (!design.ccd_variant.empty()) {
            formula_only.parameter_keys.push_back(
                "ccd_variant=" + design.ccd_variant);
        }
        formula_only.parameter_keys.push_back(gate_evidence_type_key(design.evidence_type));
        formula_only.parameter_keys.push_back("not_vendor_oracle=true");
        formula_only.parameter_keys.push_back("commercial_alignment=false");
        out.push_back(std::move(formula_only));

        if (kind == "ccd" && design.beyond_range_detected) {
            domain::EvidenceRef beyond = make_ref(
                page_prefix + ":gate:ccd_beyond_range",
                domain::EvidenceKind::diagnostic,
                domain::EvidenceRole::limiting,
                "evidence.ccd_beyond_range");
            beyond.diagnostic_code = design.allow_beyond_range
                ? "ccd_ccc_beyond_range_allowed"
                : "ccd_ccc_beyond_range";
            beyond.parameter_keys.push_back("beyond_range_detected=true");
            beyond.parameter_keys.push_back(
                "allow_beyond_range="
                + std::string(design.allow_beyond_range ? "true" : "false"));
            if (!design.ccd_variant.empty()) {
                beyond.parameter_keys.push_back("ccd_variant=" + design.ccd_variant);
            }
            beyond.parameter_keys.push_back(
                "alpha=" + std::to_string(design.alpha));
            beyond.parameter_keys.push_back(gate_evidence_type_key(design.evidence_type));
            beyond.parameter_keys.push_back("not_vendor_oracle=true");
            // Star points beyond cube are not an executable process claim.
            beyond.parameter_keys.push_back("executable_process_claim=false");
            out.push_back(std::move(beyond));
        }

        if (kind == "bbd") {
            domain::EvidenceRef no_corners = make_ref(
                page_prefix + ":gate:bbd_no_corners",
                domain::EvidenceKind::diagnostic,
                domain::EvidenceRole::limiting,
                "evidence.bbd_no_corners");
            no_corners.diagnostic_code = "bbd_no_corners";
            no_corners.parameter_keys.push_back("design_kind=bbd");
            if (!design.design_source_id.empty()) {
                no_corners.parameter_keys.push_back(
                    "design_source_id=" + design.design_source_id);
            }
            no_corners.parameter_keys.push_back("has_corner_points=false");
            no_corners.parameter_keys.push_back(
                "domain_wide_prediction_optimal=false");
            no_corners.parameter_keys.push_back(gate_evidence_type_key(design.evidence_type));
            no_corners.parameter_keys.push_back("not_vendor_oracle=true");
            out.push_back(std::move(no_corners));
        }
    }

    // Graph Builder / EDA: Hexbin product uses rectangular bins, not hex tessellation.
    bool hexbin_page = false;
    for (const domain::PlotSpec& plot : page.plots) {
        if (plot.kind == domain::PlotKind::hexbin) {
            hexbin_page = true;
            break;
        }
    }
    if (!hexbin_page) {
        for (const domain::DiagnosticMessage& diagnostic : page.diagnostics) {
            if (diagnostic.code == "hexbin_rectangular_bins") {
                hexbin_page = true;
                break;
            }
        }
    }
    if (!hexbin_page
        && (page.method_name.find("Hexbin") != std::string::npos
            || page.title.find("Hexbin") != std::string::npos)) {
        hexbin_page = true;
    }
    if (hexbin_page) {
        domain::EvidenceRef gate = make_ref(
            page_prefix + ":gate:hexbin_rectangular_bins",
            domain::EvidenceKind::diagnostic,
            domain::EvidenceRole::limiting,
            "evidence.hexbin_rectangular_bins");
        gate.diagnostic_code = "hexbin_rectangular_bins";
        gate.parameter_keys.push_back("binning=rectangular");
        gate.parameter_keys.push_back("hexagonal_tessellation=false");
        gate.parameter_keys.push_back("product_name=Hexbin");
        gate.parameter_keys.push_back("evidence_type=formula_reference");
        gate.parameter_keys.push_back("not_vendor_oracle=true");
        out.push_back(std::move(gate));
    }

    bool density_page = false;
    for (const domain::PlotSpec& plot : page.plots) {
        if (plot.kind == domain::PlotKind::density) {
            density_page = true;
            break;
        }
    }
    if (!density_page) {
        for (const domain::DiagnosticMessage& diagnostic : page.diagnostics) {
            if (diagnostic.code == "density_curve_not_discrete_marks") {
                density_page = true;
                break;
            }
        }
    }
    if (!density_page
        && (page.method_name.find("Density") != std::string::npos
            || page.title.find("密度图") != std::string::npos)) {
        density_page = true;
    }
    if (density_page) {
        domain::EvidenceRef gate = make_ref(
            page_prefix + ":gate:density_curve_not_discrete_marks",
            domain::EvidenceKind::diagnostic,
            domain::EvidenceRole::limiting,
            "evidence.density_curve_not_discrete_marks");
        gate.diagnostic_code = "density_curve_not_discrete_marks";
        gate.parameter_keys.push_back("kde_continuous_grid=true");
        gate.parameter_keys.push_back("discrete_row_mark_linkage=false");
        gate.parameter_keys.push_back("evidence_type=formula_reference");
        gate.parameter_keys.push_back("not_vendor_oracle=true");
        out.push_back(std::move(gate));
    }
}

int evidence_retention_priority(const domain::EvidenceRef& ref)
{
    // Lower = keep first under max_evidence_rows truncation.
    if (ref.role == domain::EvidenceRole::limiting) {
        return 0;
    }
    if (ref.kind == domain::EvidenceKind::dataset_snapshot
        || ref.kind == domain::EvidenceKind::filter) {
        return 1;
    }
    if (ref.role == domain::EvidenceRole::provenance) {
        return 2;
    }
    return 3;
}

std::vector<domain::EvidenceRef> prioritize_evidence_for_truncation(
    std::vector<domain::EvidenceRef> evidence)
{
    std::stable_sort(
        evidence.begin(),
        evidence.end(),
        [](const domain::EvidenceRef& left, const domain::EvidenceRef& right) {
            return evidence_retention_priority(left) < evidence_retention_priority(right);
        });
    return evidence;
}

void append_formula_refs_from_facts(
    std::vector<domain::EvidenceRef>& out,
    const domain::OutputPage& page,
    const std::string& page_prefix)
{
    const domain::InterpretationFacts& facts = page.facts;

    if (facts.capability.has_value()) {
        const domain::CapabilityFacts& cap = *facts.capability;
        const std::string method_id = cap.method.empty() ? "capability" : cap.method;
        append_formula_ref(
            out, page_prefix + ":formula:capability", cap.evidence_type, method_id);
        if (cap.hartigan_dip_status != "not_run") {
            append_formula_ref(
                out,
                page_prefix + ":formula:hartigan_dip",
                "formula_reference",
                "hartigan_dip_1985");
        }
        if (cap.mixture_status != "not_run") {
            append_formula_ref(
                out,
                page_prefix + ":formula:mixture",
                cap.mixture_evidence_type,
                cap.mixture_algorithm_id.empty() ? "gaussian_mixture_k_bic"
                                                 : cap.mixture_algorithm_id);
        }
    }

    if (facts.rsm.has_value()) {
        append_formula_ref(
            out,
            page_prefix + ":formula:rsm",
            facts.rsm->evidence_type,
            "rsm");
    }

    if (facts.reliability.has_value()) {
        const domain::ReliabilityFacts& rel = *facts.reliability;
        const std::string dist_id =
            rel.distribution.empty() ? "reliability" : rel.distribution;
        append_formula_ref(
            out, page_prefix + ":formula:reliability", rel.evidence_type, dist_id);
        for (std::size_t i = 0; i < rel.mode_fits.size(); ++i) {
            const domain::ReliabilityModeFitFacts& mode = rel.mode_fits[i];
            const std::string mode_key = mode.failure_mode.empty()
                ? std::to_string(i)
                : mode.failure_mode;
            append_formula_ref(
                out,
                page_prefix + ":formula:mode_fit:" + mode_key,
                mode.evidence_type,
                mode.algorithm_id.empty() ? "cause_specific_right_censored_competing"
                                          : mode.algorithm_id);
        }
        if (!rel.cif_algorithm_id.empty()) {
            append_formula_ref(
                out,
                page_prefix + ":formula:cif",
                rel.cif_evidence_type,
                rel.cif_algorithm_id);
        }
        if (!rel.fine_gray_algorithm_id.empty()) {
            append_formula_ref(
                out,
                page_prefix + ":formula:fine_gray",
                rel.fine_gray_evidence_type,
                rel.fine_gray_algorithm_id);
        }
    }

    if (facts.warranty.has_value()) {
        append_formula_ref(
            out,
            page_prefix + ":formula:warranty",
            facts.warranty->evidence_type,
            "warranty_claims_per_1000");
    }

    if (facts.km_interval.has_value()) {
        append_formula_ref(
            out,
            page_prefix + ":formula:km_interval",
            facts.km_interval->evidence_type,
            facts.km_interval->algorithm_id.empty()
                ? "turnbull_npmle_simplified_grid"
                : facts.km_interval->algorithm_id);
    }

    if (facts.design_generation.has_value()) {
        const domain::DesignGenerationFacts& design = *facts.design_generation;
        const std::string design_alg = !design.design_source_id.empty()
            ? design.design_source_id
            : (design.design_kind.empty() ? "design_generation" : design.design_kind);
        append_formula_ref(
            out,
            page_prefix + ":formula:design_generation",
            design.evidence_type,
            design_alg);
    }
}

}  // namespace

std::string stable_content_digest(const std::string& text)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (unsigned char ch : text) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= 1099511628211ull;
    }
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%016llx", static_cast<unsigned long long>(hash));
    return buffer;
}

std::string facts_fingerprint(const domain::InterpretationFacts& facts)
{
    std::ostringstream stream;
    if (facts.capability.has_value()) {
        const auto& c = *facts.capability;
        stream << "cap|" << optional_double_text(c.cp) << '|' << optional_double_text(c.cpk)
               << '|' << optional_double_text(c.ppk) << '|' << c.assumption_status << '|'
               << c.method << ';';
    }
    if (facts.spc.has_value()) {
        const auto& s = *facts.spc;
        stream << "spc|"
               << (s.out_of_control_count.has_value()
                       ? std::to_string(*s.out_of_control_count)
                       : "-")
               << '|' << optional_double_text(s.estimated_sigma) << ';';
    }
    if (facts.reliability.has_value()) {
        const auto& r = *facts.reliability;
        stream << "rel|" << r.distribution << '|' << optional_double_text(r.shape) << '|'
               << optional_double_text(r.scale) << '|' << r.identifiable << '|' << r.converged
               << ';';
    }
    if (facts.doe.has_value()) {
        const auto& d = *facts.doe;
        stream << "doe|" << d.design_kind << '|' << d.run_count << '|' << d.factor_count << ';';
    }
    if (facts.descriptive.has_value()) {
        const auto& d = *facts.descriptive;
        stream << "desc|" << d.n << '|' << optional_double_text(d.mean) << '|'
               << optional_double_text(d.standard_deviation) << ';';
    }
    stream << "empty=" << (!facts.capability && !facts.spc && !facts.reliability && !facts.doe
                               && !facts.descriptive);
    return stable_content_digest(stream.str());
}

std::string input_snapshot_fingerprint(const domain::DataTable& table)
{
    std::ostringstream stream;
    stream << table.name << '|' << table.source_path << '|' << table.rows.size() << '|'
           << table.columns.size() << '|' << table.import_metadata.dataset_id << '|'
           << table.import_metadata.filter_summary << '|'
           << table.import_metadata.provider_id << '|' << table.import_metadata.source_object;
    for (const std::string& column : table.columns) {
        stream << '#' << column;
    }
    // Bound cost: fingerprint header + first/last few cells only.
    const std::size_t row_limit = std::min<std::size_t>(table.rows.size(), 32);
    for (std::size_t row = 0; row < row_limit; ++row) {
        stream << '@' << (row < table.row_ids.size() ? table.row_ids[row] : row);
        for (const std::string& cell : table.rows[row]) {
            stream << ':' << cell;
        }
    }
    if (table.rows.size() > row_limit) {
        stream << "...n=" << table.rows.size();
    }
    return stable_content_digest(stream.str());
}

std::vector<domain::RuleEvidence> collect_page_rules(const domain::OutputPage& page)
{
    std::vector<domain::RuleEvidence> rules;
    if (page.facts.regression.has_value()) {
        append_rule_list(rules, page.facts.regression->rules);
    }
    if (page.facts.anova.has_value()) {
        append_rule_list(rules, page.facts.anova->rules);
    }
    if (page.facts.spc.has_value()) {
        append_rule_list(rules, page.facts.spc->rules);
    }
    if (page.facts.msa.has_value()) {
        append_rule_list(rules, page.facts.msa->rules);
    }
    if (page.facts.reliability.has_value()) {
        append_rule_list(rules, page.facts.reliability->rules);
    }
    return rules;
}

domain::EvidenceBundle build_evidence_bundle(
    const domain::DataTable& table,
    const domain::OutputPage& page,
    const domain::ReportProvenance& provenance,
    const domain::ReportProfile& profile)
{
    domain::EvidenceBundle bundle;
    bundle.schema_version = 1;
    bundle.method_metadata = page.method_metadata;
    bundle.quality_evidence.method_version = page.method_metadata.version;
    bundle.quality_evidence.valid_count = page.method_metadata.valid_count;
    bundle.quality_evidence.missing_count = page.method_metadata.missing_count;
    bundle.quality_evidence.assumption_status = page.method_metadata.assumption_status;
    bundle.quality_evidence.not_computed_reason = page.method_metadata.not_computed_reason;
    bundle.quality_evidence.parameter_source = page.method_metadata.parameter_source;
    bundle.quality_evidence.source_rows.assign(
        page.method_metadata.source_rows.begin(), page.method_metadata.source_rows.end());
    bundle.provenance = provenance;
    bundle.rules = collect_page_rules(page);

    const std::string page_prefix = page.id.empty() ? "page" : page.id;

    domain::EvidenceRef dataset = make_ref(
        page_prefix + ":dataset",
        domain::EvidenceKind::dataset_snapshot,
        domain::EvidenceRole::provenance,
        "evidence.dataset_snapshot");
    dataset.source_dataset_id = provenance.source_dataset_id;
    if (table.row_ids.empty()) {
        dataset.status = "missing";
        dataset.notes_text_id = "evidence.missing_row_ids";
    } else {
        const std::size_t limit = std::min(table.row_ids.size(), profile.max_evidence_rows);
        dataset.source_rows.assign(table.row_ids.begin(), table.row_ids.begin() + static_cast<std::ptrdiff_t>(limit));
        if (table.row_ids.size() > profile.max_evidence_rows) {
            dataset.status = "truncated";
        }
    }
    bundle.evidence.push_back(dataset);

    if (!provenance.import_plan_summary.empty() || !table.import_metadata.provider_id.empty()) {
        domain::EvidenceRef import_plan = make_ref(
            page_prefix + ":import_plan",
            domain::EvidenceKind::import_plan,
            domain::EvidenceRole::provenance,
            "evidence.import_plan");
        import_plan.source_dataset_id = provenance.source_dataset_id;
        if (provenance.import_plan_summary.empty() && table.import_metadata.provider_id.empty()) {
            import_plan.status = "missing";
        }
        bundle.evidence.push_back(import_plan);
    }

    domain::EvidenceRef filter = make_ref(
        page_prefix + ":filter",
        domain::EvidenceKind::filter,
        domain::EvidenceRole::provenance,
        "evidence.filter");
    filter.source_dataset_id = provenance.source_dataset_id;
    if (provenance.filter_summary.empty() && table.import_metadata.filter_summary.empty()
        && page.configuration.excluded_rows.empty()
        && page.configuration.hidden_rows.empty()) {
        filter.status = "missing";
    }
    filter.notes_text_id = "evidence.filter_excluded_hidden";
    filter.parameter_keys.push_back(
        "excluded_count=" + std::to_string(page.configuration.excluded_rows.size()));
    filter.parameter_keys.push_back(
        "hidden_count=" + std::to_string(page.configuration.hidden_rows.size()));
    bundle.evidence.push_back(filter);

    domain::EvidenceRef row_selection = make_ref(
        page_prefix + ":row_selection",
        domain::EvidenceKind::row_selection,
        domain::EvidenceRole::provenance,
        "evidence.row_selection");
    row_selection.source_dataset_id = provenance.source_dataset_id;
    row_selection.parameter_keys.push_back(
        "excluded_row_count=" + std::to_string(provenance.excluded_row_count));
    row_selection.parameter_keys.push_back(
        "hidden_row_count=" + std::to_string(provenance.hidden_row_count));
    if (provenance.excluded_row_count == 0 && provenance.hidden_row_count == 0
        && page.configuration.excluded_rows.empty()
        && page.configuration.hidden_rows.empty()) {
        row_selection.status = "not_applicable";
    }
    bundle.evidence.push_back(row_selection);

    domain::EvidenceRef parameters = make_ref(
        page_prefix + ":parameters",
        domain::EvidenceKind::parameter,
        domain::EvidenceRole::supporting,
        "evidence.parameters");
    if (!page.method_metadata.parameters.empty()) {
        parameters.parameter_keys.push_back("method_metadata.parameters");
    }
    if (!page.parameter_summary.empty()) {
        parameters.parameter_keys.push_back("parameter_summary");
    }
    if (parameters.parameter_keys.empty()) {
        parameters.status = "missing";
    }
    bundle.evidence.push_back(parameters);

    domain::EvidenceRef algorithm = make_ref(
        page_prefix + ":algorithm_version",
        domain::EvidenceKind::algorithm_version,
        domain::EvidenceRole::provenance,
        "evidence.algorithm_version");
    if (page.method_metadata.algorithm.empty() && page.method_metadata.version.empty()) {
        algorithm.status = "missing";
    }
    bundle.evidence.push_back(algorithm);

    // Gate honesty refs before plot flood so customer truncation cannot drop them.
    append_gate_limiting_refs_from_facts(bundle.evidence, page, page_prefix);
    std::vector<std::string> gate_diagnostic_codes;
    for (const domain::EvidenceRef& ref : bundle.evidence) {
        if (ref.role == domain::EvidenceRole::limiting
            && !ref.diagnostic_code.empty()) {
            gate_diagnostic_codes.push_back(ref.diagnostic_code);
        }
    }

    for (std::size_t plot_index = 0; plot_index < page.plots.size(); ++plot_index) {
        const domain::PlotSpec& plot = page.plots[plot_index];
        domain::EvidenceRef plot_ref = make_ref(
            page_prefix + ":plot:" + std::to_string(plot_index),
            domain::EvidenceKind::plot,
            domain::EvidenceRole::supporting,
            "evidence.plot");
        plot_ref.source_dataset_id = provenance.source_dataset_id;
        plot_ref.parameter_keys.push_back("title=" + plot.title);
        plot_ref.parameter_keys.push_back(
            "source_row_count=" + std::to_string(plot.source_rows.size()));
        plot_ref.parameter_keys.push_back(
            "excluded_row_count="
            + std::to_string(page.configuration.excluded_rows.size()));
        plot_ref.parameter_keys.push_back(
            "hidden_row_count=" + std::to_string(page.configuration.hidden_rows.size()));
        const std::size_t row_limit =
            std::min(plot.source_rows.size(), profile.max_evidence_rows);
        plot_ref.source_rows.assign(
            plot.source_rows.begin(),
            plot.source_rows.begin() + static_cast<std::ptrdiff_t>(row_limit));
        if (plot.source_rows.size() > profile.max_evidence_rows) {
            plot_ref.status = "truncated";
        }
        if (plot.title.empty() && plot.source_rows.empty()) {
            plot_ref.status = "missing";
        }
        bundle.evidence.push_back(plot_ref);
    }

    for (const domain::RuleEvidence& rule : bundle.rules) {
        domain::EvidenceRef rule_ref = make_ref(
            page_prefix + ":rule:" + (rule.id.empty() ? "unnamed" : rule.id),
            domain::EvidenceKind::rule,
            domain::EvidenceRole::supporting,
            "evidence.rule");
        rule_ref.rule_id = rule.id;
        rule_ref.source_rows = rule.related_rows;
        rule_ref.status = rule.status.empty() ? "present" : rule.status;
        if (rule.related_rows.empty()) {
            // Missing RowId is allowed; mark notes for audit transparency.
            rule_ref.notes_text_id = "evidence.rule_without_row_ids";
        }
        bundle.evidence.push_back(rule_ref);
    }

    for (const domain::DiagnosticMessage& diagnostic : page.diagnostics) {
        if (!diagnostic.code.empty()
            && std::find(
                   gate_diagnostic_codes.begin(),
                   gate_diagnostic_codes.end(),
                   diagnostic.code)
                != gate_diagnostic_codes.end()) {
            // Already emitted as an early Facts-based limiting gate ref.
            continue;
        }
        domain::EvidenceRef diagnostic_ref = make_ref(
            page_prefix + ":diagnostic:"
                + (diagnostic.code.empty() ? "message" : diagnostic.code),
            domain::EvidenceKind::diagnostic,
            is_risk_diagnostic(diagnostic) ? domain::EvidenceRole::limiting
                                           : domain::EvidenceRole::diagnostic,
            "evidence.diagnostic");
        diagnostic_ref.diagnostic_code = diagnostic.code;
        diagnostic_ref.source_rows = diagnostic.related_rows;
        if (diagnostic.related_rows.empty()) {
            diagnostic_ref.notes_text_id = "evidence.diagnostic_without_row_ids";
        }
        bundle.evidence.push_back(diagnostic_ref);
    }

    // Profile gate: customer excludes formula refs; engineer=summary, audit=full appendix.
    if (profile.include_formula_references) {
        append_formula_refs_from_facts(bundle.evidence, page, page_prefix);
    }

    if (bundle.evidence.empty()) {
        domain::EvidenceRef empty = make_ref(
            page_prefix + ":empty",
            domain::EvidenceKind::other,
            domain::EvidenceRole::diagnostic,
            "evidence.empty");
        empty.status = "not_applicable";
        bundle.evidence.push_back(empty);
    }

    return bundle;
}

domain::ReportPageView apply_report_profile(
    const domain::OutputPage& page,
    const domain::EvidenceBundle& evidence,
    const domain::ReportProfile& profile)
{
    domain::ReportPageView view;
    view.source_page = page;
    view.show_parameter_summary = profile.include_parameters;
    view.show_method_metadata = profile.include_algorithm_versions || profile.include_parameters;
    view.show_provenance = profile.include_input_snapshot || profile.include_import_plan
        || profile.include_filter_detail || profile.include_source_hashes;
    view.show_hashes = profile.include_source_hashes;
    view.show_evidence_appendix = profile.include_full_evidence_appendix;

    if (profile.include_executive_summary) {
        view.visible_interpretation = page.interpretation;
    }

    if (profile.include_statistic_tables) {
        for (const domain::StatisticTable& table : page.tables) {
            if (table.rows.size() > profile.max_preview_rows) {
                view.truncated_table_row_count += table.rows.size() - profile.max_preview_rows;
            }
            view.visible_tables.push_back(truncate_table(table, profile.max_preview_rows));
        }
    }

    if (profile.include_plots) {
        const std::size_t plot_limit = std::min(page.plots.size(), profile.max_plots);
        for (std::size_t index = 0; index < plot_limit; ++index) {
            domain::PlotSpec plot = page.plots[index];
            // Presentation-only visibility caption; does not recompute series.
            if (!page.configuration.excluded_rows.empty()
                || !page.configuration.hidden_rows.empty()) {
                const std::string visibility =
                    "excluded=" + std::to_string(page.configuration.excluded_rows.size())
                    + " hidden=" + std::to_string(page.configuration.hidden_rows.size())
                    + " (display omits both; analysis omits excluded only)";
                if (plot.subtitle.empty()) {
                    plot.subtitle = visibility;
                } else if (plot.subtitle.find("excluded=") == std::string::npos) {
                    plot.subtitle += "    " + visibility;
                }
            }
            view.visible_plots.push_back(std::move(plot));
        }
    }

    if (profile.include_key_risks_and_limits || profile.include_diagnostics) {
        for (const domain::DiagnosticMessage& diagnostic : page.diagnostics) {
            if (is_risk_diagnostic(diagnostic)) {
                if (profile.include_key_risks_and_limits) {
                    view.visible_diagnostics.push_back(diagnostic);
                }
            } else if (profile.include_diagnostics
                       && profile.template_kind != domain::ReportTemplateKind::customer) {
                view.visible_diagnostics.push_back(diagnostic);
            }
        }
    }

    if (profile.include_rule_evidence) {
        view.visible_rules = evidence.rules;
        if (profile.include_anomaly_rows) {
            // Keep rule related_rows as-is for engineer/audit anomaly linkage.
        }
    }

    const std::size_t evidence_limit = profile.max_evidence_rows;
    std::vector<domain::EvidenceRef> ordered =
        prioritize_evidence_for_truncation(evidence.evidence);
    if (ordered.size() > evidence_limit) {
        view.truncated_evidence_count = ordered.size() - evidence_limit;
        view.visible_evidence.assign(
            ordered.begin(),
            ordered.begin() + static_cast<std::ptrdiff_t>(evidence_limit));
        for (domain::EvidenceRef& ref : view.visible_evidence) {
            if (ref.role != domain::EvidenceRole::limiting
                && ref.status == "present") {
                ref.status = "truncated";
            }
        }
    } else {
        view.visible_evidence = std::move(ordered);
    }

    if (!profile.include_formula_references) {
        std::vector<domain::EvidenceRef> without_formula;
        without_formula.reserve(view.visible_evidence.size());
        for (const domain::EvidenceRef& ref : view.visible_evidence) {
            if (ref.kind != domain::EvidenceKind::formula_reference) {
                without_formula.push_back(ref);
            }
        }
        view.visible_evidence = std::move(without_formula);
    }

    if (!profile.include_full_evidence_appendix
        && profile.template_kind == domain::ReportTemplateKind::customer) {
        // Customer keeps limiting/provenance risk evidence only.
        std::vector<domain::EvidenceRef> filtered;
        for (const domain::EvidenceRef& ref : view.visible_evidence) {
            if (ref.role == domain::EvidenceRole::limiting
                || ref.kind == domain::EvidenceKind::dataset_snapshot
                || ref.kind == domain::EvidenceKind::filter) {
                filtered.push_back(ref);
            }
        }
        view.visible_evidence = std::move(filtered);
    }

    return view;
}

domain::ReportDocument build_report_document(
    const domain::DataTable& table,
    const std::vector<domain::OutputPage>& pages,
    const domain::ReportProfile& profile,
    const ReportAssemblyOptions& options)
{
    domain::ReportDocument document;
    document.schema_version = 1;
    document.profile = profile;
    document.software_version = options.software_version;

    domain::ReportProvenance provenance;
    provenance.report_id = options.report_id_prefix + "-" + report_template_kind_id(profile.template_kind);
    provenance.source_dataset_id = table.import_metadata.dataset_id.empty()
        ? table.name
        : table.import_metadata.dataset_id;
    provenance.source_path = table.source_path;
    if (!table.import_metadata.provider_id.empty()) {
        provenance.source_kind = "database_snapshot";
    } else if (!table.source_path.empty() || !table.name.empty()) {
        provenance.source_kind = "worksheet_snapshot";
    } else {
        provenance.source_kind = "unknown";
    }
    provenance.import_plan_summary = table.import_metadata.provider_id.empty()
        ? table.import_metadata.sheet_name
        : (table.import_metadata.provider_id + ":" + table.import_metadata.source_object + ":"
           + table.import_metadata.object_kind);
    provenance.filter_summary = table.import_metadata.filter_summary;
    if (!pages.empty()) {
        const std::string visibility_summary =
            "excluded=" + std::to_string(pages.front().configuration.excluded_rows.size())
            + ";hidden=" + std::to_string(pages.front().configuration.hidden_rows.size());
        if (provenance.filter_summary.empty()) {
            provenance.filter_summary = visibility_summary;
        } else {
            provenance.filter_summary += ";" + visibility_summary;
        }
    }
    provenance.row_count_n = table.rows.size();
    provenance.column_count = table.columns.size();
    provenance.excluded_row_count = 0;
    provenance.hidden_row_count = 0;
    if (!pages.empty()) {
        provenance.excluded_row_count = pages.front().configuration.excluded_rows.size();
        provenance.hidden_row_count = pages.front().configuration.hidden_rows.size();
        provenance.algorithm_version = pages.front().method_metadata.version.empty()
            ? pages.front().method_metadata.algorithm
            : (pages.front().method_metadata.algorithm + "@" + pages.front().method_metadata.version);
    }
    provenance.software_version = options.software_version;
    provenance.generated_at_utc = options.generated_at_utc;
    provenance.input_snapshot_hash = input_snapshot_fingerprint(table);
    if (!pages.empty()) {
        provenance.facts_hash = facts_fingerprint(pages.front().facts);
        provenance.configuration_hash = stable_content_digest(pages.front().parameter_summary);
    }
    document.provenance = provenance;

    domain::EvidenceBundle merged;
    merged.schema_version = 1;
    merged.provenance = provenance;

    for (const domain::OutputPage& page : pages) {
        domain::EvidenceBundle page_bundle =
            build_evidence_bundle(table, page, provenance, profile);
        merged.rules.insert(merged.rules.end(), page_bundle.rules.begin(), page_bundle.rules.end());
        merged.evidence.insert(
            merged.evidence.end(), page_bundle.evidence.begin(), page_bundle.evidence.end());
        if (merged.method_metadata.algorithm.empty()) {
            merged.method_metadata = page_bundle.method_metadata;
            merged.quality_evidence = page_bundle.quality_evidence;
        }
        document.pages.push_back(apply_report_profile(page, page_bundle, profile));
    }
    document.evidence = std::move(merged);
    return document;
}

bool report_document_preserves_facts(
    const domain::ReportDocument& document,
    const std::vector<domain::OutputPage>& original_pages)
{
    if (document.pages.size() != original_pages.size()) {
        return false;
    }
    for (std::size_t index = 0; index < original_pages.size(); ++index) {
        if (facts_fingerprint(document.pages[index].source_page.facts)
            != facts_fingerprint(original_pages[index].facts)) {
            return false;
        }
        if (document.pages[index].source_page.method_metadata.valid_count
            != original_pages[index].method_metadata.valid_count) {
            return false;
        }
    }
    return true;
}

}  // namespace datalab::application
