#pragma once

// Phase 0 report/evidence contracts.
// Presentation-only types: ReportProfile must never recompute statistics.
// Locale and display text live outside InterpretationFacts numeric fields.

#include "domain/quality_types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::domain {

enum class ReportTemplateKind {
    customer,
    engineer,
    audit
};

enum class EvidenceKind {
    dataset_snapshot,
    import_plan,
    filter,
    parameter,
    rule,
    diagnostic,
    formula_reference,
    reference_implementation,
    vendor_oracle,
    golden,
    algorithm_version,
    row_selection,
    plot,
    table_cell,
    other
};

enum class EvidenceRole {
    supporting,
    contradicting,
    limiting,
    provenance,
    diagnostic
};

enum class PdfComplianceStatus {
    not_validated,
    unsupported,
    validated_pass,
    validated_fail
};

// Stable translation key. Domain Facts must store LocaleTextId, not rendered text.
struct LocaleTextId {
    std::string id;
    std::string fallback_zh_cn;
};

struct ReportLocaleSettings {
    std::string language_tag = "zh-CN";  // BCP 47; report locale, not UI locale
    std::string timezone_id = "Asia/Shanghai";
    std::string number_format_locale = "zh-CN";
    std::string date_format_locale = "zh-CN";
    bool use_percent_sign_localized = true;
};

struct ReportProvenance {
    std::string report_id;
    std::string source_dataset_id;
    std::string source_path;
    std::string source_kind = "worksheet_snapshot";  // worksheet_snapshot | database_snapshot | unknown
    std::string import_plan_summary;
    std::string filter_summary;
    std::size_t row_count_n = 0;
    std::size_t column_count = 0;
    std::size_t excluded_row_count = 0;
    std::size_t hidden_row_count = 0;
    std::string algorithm_version;
    std::string software_version;
    std::string generated_at_utc;
    std::string input_snapshot_hash;
    std::string facts_hash;
    std::string configuration_hash;
};

struct EvidenceRef {
    std::string evidence_id;  // stable across locales
    EvidenceKind kind = EvidenceKind::other;
    EvidenceRole role = EvidenceRole::supporting;
    std::string label_text_id;  // LocaleTextId::id
    std::string source_dataset_id;
    std::vector<RowId> source_rows;
    std::vector<std::string> parameter_keys;
    std::string formula_ref_id;
    std::string reference_impl_id;
    std::string vendor_oracle_id;
    std::string golden_id;
    std::string diagnostic_code;
    std::string rule_id;
    std::string status = "present";  // present | missing | truncated | not_applicable
    std::string notes_text_id;
};

struct EvidenceBundle {
    int schema_version = 1;
    std::vector<EvidenceRef> evidence;
    std::vector<RuleEvidence> rules;
    MethodMetadata method_metadata;
    QualityEvidence quality_evidence;
    ReportProvenance provenance;
};

// Display policy only. Must not alter OutputPage::facts numeric values.
struct ReportProfile {
    int schema_version = 1;
    std::string profile_id;
    ReportTemplateKind template_kind = ReportTemplateKind::engineer;
    std::string template_version = "1";
    ReportLocaleSettings locale;
    bool include_executive_summary = true;
    bool include_key_risks_and_limits = true;
    bool include_plots = true;
    bool include_statistic_tables = true;
    bool include_parameters = true;
    bool include_rule_evidence = true;
    bool include_diagnostics = true;
    bool include_anomaly_rows = true;
    bool include_input_snapshot = false;
    bool include_import_plan = false;
    bool include_filter_detail = false;
    bool include_algorithm_versions = false;
    bool include_source_hashes = false;
    bool include_formula_references = false;
    bool include_full_evidence_appendix = false;
    std::size_t max_preview_rows = 50;
    std::size_t max_evidence_rows = 200;
    // Chart-in-report: customer keeps primary chart(s); engineer/audit keep more.
    std::size_t max_plots = 8;
};

struct ReportExportManifest {
    int schema_version = 1;
    std::string report_id;
    std::string template_id;
    std::string template_version;
    std::string locale_language_tag;
    std::string timezone_id;
    std::string generated_at_utc;
    std::string software_version;
    std::string algorithm_version;
    std::string facts_hash;
    std::string input_snapshot_hash;
    std::string pdf_relative_path;
    std::string audit_json_relative_path;
    std::string title;
    std::string author = "DataLab";
    std::string analysis_page_ids;
    std::string consistency_status = "not_checked";  // ok | mismatch | not_checked
    PdfComplianceStatus pdfa_status = PdfComplianceStatus::not_validated;
    // QPainter path has no tagged structure tree; default matches pipeline assessment.
    PdfComplianceStatus pdfua_status = PdfComplianceStatus::unsupported;
    std::string validator_name;
    std::string validator_version;
    std::string validator_notes =
        "No PDF/A or PDF/UA validator invoked; status remains not_validated.";
    std::vector<std::string> compliance_blockers;  // honest assessment checklist
    std::string export_pipeline = "Qt QPdfWriter + QPainter";
};

// Static capability assessment of the current PDF export pipeline.
// Never returns validated_pass without an external validator run.
struct PdfComplianceAssessment {
    PdfComplianceStatus pdfa_status = PdfComplianceStatus::not_validated;
    PdfComplianceStatus pdfua_status = PdfComplianceStatus::unsupported;
    std::string export_pipeline = "Qt QPdfWriter + QPainter";
    std::vector<std::string> blockers;
    std::string summary;
    std::string validator_name;
    std::string validator_version;
    bool validator_invoked = false;
};

// Optional external PDF/A CLI result (e.g. veraPDF). PDF/UA must not be set to
// validated_pass from this alone on the QPainter pipeline.
struct ExternalPdfaValidatorResult {
    bool tool_configured = false;
    bool tool_invoked = false;
    bool tool_available = false;
    int exit_code = -1;
    std::string validator_name;     // e.g. "veraPDF"
    std::string validator_version;
    std::string stdout_excerpt;
    std::string notes;
};

inline std::string report_template_kind_id(ReportTemplateKind kind)
{
    switch (kind) {
    case ReportTemplateKind::customer:
        return "customer";
    case ReportTemplateKind::engineer:
        return "engineer";
    case ReportTemplateKind::audit:
        return "audit";
    }
    return "engineer";
}

inline ReportProfile make_report_profile(ReportTemplateKind kind)
{
    ReportProfile profile;
    profile.template_kind = kind;
    profile.profile_id = report_template_kind_id(kind);
    profile.template_version = "1";

    switch (kind) {
    case ReportTemplateKind::customer:
        profile.include_executive_summary = true;
        profile.include_key_risks_and_limits = true;
        profile.include_plots = true;
        profile.include_statistic_tables = false;
        profile.include_parameters = false;
        profile.include_rule_evidence = false;
        profile.include_diagnostics = true;  // risks/limits only; not raw tech dumps
        profile.include_anomaly_rows = false;
        profile.include_input_snapshot = false;
        profile.include_import_plan = false;
        profile.include_filter_detail = false;
        profile.include_algorithm_versions = false;
        profile.include_source_hashes = false;
        profile.include_formula_references = false;
        profile.include_full_evidence_appendix = false;
        profile.max_preview_rows = 20;
        profile.max_evidence_rows = 20;
        profile.max_plots = 1;
        break;
    case ReportTemplateKind::engineer:
        profile.include_executive_summary = true;
        profile.include_key_risks_and_limits = true;
        profile.include_plots = true;
        profile.include_statistic_tables = true;
        profile.include_parameters = true;
        profile.include_rule_evidence = true;
        profile.include_diagnostics = true;
        profile.include_anomaly_rows = true;
        profile.include_input_snapshot = false;
        profile.include_import_plan = false;
        profile.include_filter_detail = true;
        profile.include_algorithm_versions = true;
        profile.include_source_hashes = false;
        profile.include_formula_references = true;
        profile.include_full_evidence_appendix = false;
        profile.max_preview_rows = 50;
        profile.max_evidence_rows = 200;
        profile.max_plots = 8;
        break;
    case ReportTemplateKind::audit:
        profile.include_executive_summary = true;
        profile.include_key_risks_and_limits = true;
        profile.include_plots = true;
        profile.include_statistic_tables = true;
        profile.include_parameters = true;
        profile.include_rule_evidence = true;
        profile.include_diagnostics = true;
        profile.include_anomaly_rows = true;
        profile.include_input_snapshot = true;
        profile.include_import_plan = true;
        profile.include_filter_detail = true;
        profile.include_algorithm_versions = true;
        profile.include_source_hashes = true;
        profile.include_formula_references = true;
        profile.include_full_evidence_appendix = true;
        profile.max_preview_rows = 200;
        profile.max_evidence_rows = 1000;
        profile.max_plots = 16;
        break;
    }
    return profile;
}

inline bool report_profile_changes_facts()
{
    // Contract invariant documented for Phase 0/1 tests:
    // selecting a ReportProfile only changes presentation flags.
    return false;
}

// Presentation view of one OutputPage under a ReportProfile.
// source_page.facts must remain byte-identical across template kinds.
struct ReportPageView {
    OutputPage source_page;
    std::vector<InterpretationSection> visible_interpretation;
    std::vector<StatisticTable> visible_tables;
    std::vector<PlotSpec> visible_plots;
    std::vector<DiagnosticMessage> visible_diagnostics;
    std::vector<RuleEvidence> visible_rules;
    std::vector<EvidenceRef> visible_evidence;
    std::size_t truncated_evidence_count = 0;
    std::size_t truncated_table_row_count = 0;
    bool show_parameter_summary = false;
    bool show_method_metadata = false;
    bool show_provenance = false;
    bool show_hashes = false;
    bool show_evidence_appendix = false;
};

struct ReportDocument {
    int schema_version = 1;
    ReportProfile profile;
    ReportProvenance provenance;
    EvidenceBundle evidence;
    std::vector<ReportPageView> pages;
    std::string software_version = "DataLab";
};

inline std::string evidence_kind_id(EvidenceKind kind)
{
    switch (kind) {
    case EvidenceKind::dataset_snapshot:
        return "dataset_snapshot";
    case EvidenceKind::import_plan:
        return "import_plan";
    case EvidenceKind::filter:
        return "filter";
    case EvidenceKind::parameter:
        return "parameter";
    case EvidenceKind::rule:
        return "rule";
    case EvidenceKind::diagnostic:
        return "diagnostic";
    case EvidenceKind::formula_reference:
        return "formula_reference";
    case EvidenceKind::reference_implementation:
        return "reference_implementation";
    case EvidenceKind::vendor_oracle:
        return "vendor_oracle";
    case EvidenceKind::golden:
        return "golden";
    case EvidenceKind::algorithm_version:
        return "algorithm_version";
    case EvidenceKind::row_selection:
        return "row_selection";
    case EvidenceKind::plot:
        return "plot";
    case EvidenceKind::table_cell:
        return "table_cell";
    case EvidenceKind::other:
        return "other";
    }
    return "other";
}

inline EvidenceKind evidence_kind_from_id(const std::string& id)
{
    if (id == "dataset_snapshot") {
        return EvidenceKind::dataset_snapshot;
    }
    if (id == "import_plan") {
        return EvidenceKind::import_plan;
    }
    if (id == "filter") {
        return EvidenceKind::filter;
    }
    if (id == "parameter") {
        return EvidenceKind::parameter;
    }
    if (id == "rule") {
        return EvidenceKind::rule;
    }
    if (id == "diagnostic") {
        return EvidenceKind::diagnostic;
    }
    if (id == "formula_reference") {
        return EvidenceKind::formula_reference;
    }
    if (id == "reference_implementation") {
        return EvidenceKind::reference_implementation;
    }
    if (id == "vendor_oracle") {
        return EvidenceKind::vendor_oracle;
    }
    if (id == "golden") {
        return EvidenceKind::golden;
    }
    if (id == "algorithm_version") {
        return EvidenceKind::algorithm_version;
    }
    if (id == "row_selection") {
        return EvidenceKind::row_selection;
    }
    if (id == "plot") {
        return EvidenceKind::plot;
    }
    if (id == "table_cell") {
        return EvidenceKind::table_cell;
    }
    return EvidenceKind::other;
}

inline std::string evidence_role_id(EvidenceRole role)
{
    switch (role) {
    case EvidenceRole::supporting:
        return "supporting";
    case EvidenceRole::contradicting:
        return "contradicting";
    case EvidenceRole::limiting:
        return "limiting";
    case EvidenceRole::provenance:
        return "provenance";
    case EvidenceRole::diagnostic:
        return "diagnostic";
    }
    return "supporting";
}

inline EvidenceRole evidence_role_from_id(const std::string& id)
{
    if (id == "contradicting") {
        return EvidenceRole::contradicting;
    }
    if (id == "limiting") {
        return EvidenceRole::limiting;
    }
    if (id == "provenance") {
        return EvidenceRole::provenance;
    }
    if (id == "diagnostic") {
        return EvidenceRole::diagnostic;
    }
    return EvidenceRole::supporting;
}

inline ReportTemplateKind report_template_kind_from_id(const std::string& id)
{
    if (id == "customer") {
        return ReportTemplateKind::customer;
    }
    if (id == "audit") {
        return ReportTemplateKind::audit;
    }
    return ReportTemplateKind::engineer;
}

inline std::string pdf_compliance_status_id(PdfComplianceStatus status)
{
    switch (status) {
    case PdfComplianceStatus::not_validated:
        return "not_validated";
    case PdfComplianceStatus::unsupported:
        return "unsupported";
    case PdfComplianceStatus::validated_pass:
        return "validated_pass";
    case PdfComplianceStatus::validated_fail:
        return "validated_fail";
    }
    return "not_validated";
}

inline PdfComplianceStatus pdf_compliance_status_from_id(const std::string& id)
{
    if (id == "unsupported") {
        return PdfComplianceStatus::unsupported;
    }
    if (id == "validated_pass") {
        return PdfComplianceStatus::validated_pass;
    }
    if (id == "validated_fail") {
        return PdfComplianceStatus::validated_fail;
    }
    return PdfComplianceStatus::not_validated;
}

}  // namespace datalab::domain
