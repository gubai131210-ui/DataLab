#pragma once

#include "domain/report_types.h"

#include <string>

namespace datalab::application {

struct ReportExportPaths {
    std::string pdf_file_name;
    std::string audit_json_file_name;
    std::string manifest_file_name;
};

ReportExportPaths make_report_export_paths(const std::string& pdf_base_name);

domain::ReportExportManifest build_export_manifest(
    const domain::ReportDocument& document,
    const ReportExportPaths& paths);

// Honest static assessment of the QPdfWriter/QPainter export pipeline.
// Does not open files and never claims validated_pass.
domain::PdfComplianceAssessment assess_pdf_export_pipeline();

// Merge an optional veraPDF (or equivalent) CLI result into a baseline assessment.
// - PDF/UA remains unsupported on QPainter pipelines even if PDF/A validates.
// - validated_pass requires a non-empty validator_name and exit_code == 0.
// - Missing/unconfigured tool keeps PDF/A as not_validated (never invents pass).
domain::PdfComplianceAssessment merge_external_pdfa_validator_result(
    domain::PdfComplianceAssessment baseline,
    const domain::ExternalPdfaValidatorResult& external);

// Merge optional PAC probe. PDF/UA MUST remain unsupported on QPainter even if
// PAC exit_code==0 (no tagged-PDF). Never invents validated_pass for UA.
domain::PdfComplianceAssessment merge_optional_pac_result(
    domain::PdfComplianceAssessment baseline,
    const domain::ExternalPdfaValidatorResult& pac);

// Applies assessment fields onto an export manifest (identity fields untouched).
void apply_pdf_compliance_assessment(
    domain::ReportExportManifest& manifest,
    const domain::PdfComplianceAssessment& assessment);

// Compares manifest identity fields against the document that produced the export.
// Does not claim PDF/A or PDF/UA compliance.
bool manifest_matches_document(
    const domain::ReportExportManifest& manifest,
    const domain::ReportDocument& document,
    std::string* mismatch_reason = nullptr);

// Formats report display cells without inventing statistics.
std::string format_report_display_value(const std::string& raw_text);

}  // namespace datalab::application
