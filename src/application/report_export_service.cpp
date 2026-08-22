#include "application/report_export_service.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace datalab::application {
namespace {

bool looks_like_number(const std::string& text, double* value)
{
    if (text.empty()) {
        return false;
    }
    char* end = nullptr;
    const double parsed = std::strtod(text.c_str(), &end);
    if (end == text.c_str()) {
        return false;
    }
    while (end != nullptr && *end != '\0' && std::isspace(static_cast<unsigned char>(*end))) {
        ++end;
    }
    if (end == nullptr || *end != '\0') {
        return false;
    }
    if (value != nullptr) {
        *value = parsed;
    }
    return true;
}

}  // namespace

ReportExportPaths make_report_export_paths(const std::string& pdf_base_name)
{
    ReportExportPaths paths;
    std::string base = pdf_base_name;
    if (base.size() >= 4) {
        const std::string lower = base.substr(base.size() - 4);
        if (lower == ".pdf" || lower == ".PDF") {
            base = base.substr(0, base.size() - 4);
        }
    }
    if (base.empty()) {
        base = "datalab_report";
    }
    paths.pdf_file_name = base + ".pdf";
    paths.audit_json_file_name = base + ".audit.json";
    paths.manifest_file_name = base + ".manifest.json";
    return paths;
}

domain::PdfComplianceAssessment assess_pdf_export_pipeline()
{
    domain::PdfComplianceAssessment assessment;
    assessment.export_pipeline = "Qt QPdfWriter + QPainter";
    assessment.pdfa_status = domain::PdfComplianceStatus::not_validated;
    // Tagged PDF / structure tree are not available on the QPainter path.
    assessment.pdfua_status = domain::PdfComplianceStatus::unsupported;
    assessment.validator_invoked = false;
    assessment.blockers = {
        "No veraPDF (or equivalent) PDF/A validator run",
        "No PAC (or equivalent) PDF/UA validator run",
        "QPainter path does not emit a tagged PDF structure tree",
        "XMP PDF/A output-intent package not validated",
        "Embedded-font / color-space conformance not proven",
        "Must not treat metadata title/author or successful export as compliance"};
    assessment.summary =
        "PDF/A=not_validated; PDF/UA=unsupported on QPainter pipeline; "
        "do not claim compliance until an external validator records pass/fail.";
    return assessment;
}

domain::PdfComplianceAssessment merge_external_pdfa_validator_result(
    domain::PdfComplianceAssessment baseline,
    const domain::ExternalPdfaValidatorResult& external)
{
    // PDF/UA cannot become validated on the QPainter pipeline from PDF/A tools.
    baseline.pdfua_status = domain::PdfComplianceStatus::unsupported;

    if (!external.tool_configured) {
        baseline.summary =
            "PDF/A=not_validated (DATALAB_VERAPDF unset); "
            "PDF/UA=unsupported on QPainter pipeline; do not claim compliance.";
        return baseline;
    }

    // Do not invent a validator identity; empty name cannot become validated_pass.
    baseline.validator_name = external.validator_name;
    baseline.validator_version = external.validator_version;

    if (!external.tool_available) {
        baseline.pdfa_status = domain::PdfComplianceStatus::not_validated;
        baseline.validator_invoked = false;
        baseline.blockers.insert(
            baseline.blockers.begin(),
            "Configured PDF/A validator path is missing or not executable");
        baseline.summary =
            "PDF/A=not_validated (validator configured but unavailable); "
            "PDF/UA=unsupported; do not claim compliance.";
        return baseline;
    }

    if (!external.tool_invoked) {
        baseline.pdfa_status = domain::PdfComplianceStatus::not_validated;
        baseline.validator_invoked = false;
        baseline.summary =
            "PDF/A=not_validated (validator configured but not invoked); "
            "PDF/UA=unsupported; do not claim compliance.";
        return baseline;
    }

    baseline.validator_invoked = true;
    // Remove the "No veraPDF run" blocker once a real invocation happened.
    std::vector<std::string> filtered;
    for (const std::string& blocker : baseline.blockers) {
        if (blocker.find("No veraPDF") != std::string::npos) {
            continue;
        }
        filtered.push_back(blocker);
    }
    baseline.blockers = std::move(filtered);

    if (baseline.validator_name.empty()) {
        baseline.pdfa_status = domain::PdfComplianceStatus::not_validated;
        baseline.blockers.insert(
            baseline.blockers.begin(),
            "External PDF/A validator invoked without validator_name; pass forbidden");
        baseline.summary =
            "PDF/A=not_validated (validator invoked but unnamed); "
            "PDF/UA=unsupported; do not claim compliance.";
        return baseline;
    }

    if (external.exit_code == 0) {
        baseline.pdfa_status = domain::PdfComplianceStatus::validated_pass;
        baseline.summary =
            "PDF/A=validated_pass via " + baseline.validator_name
            + (baseline.validator_version.empty()
                   ? ""
                   : (" " + baseline.validator_version))
            + "; PDF/UA=unsupported on QPainter pipeline "
              "(PAC/tagged-PDF not available); do not claim PDF/UA compliance.";
        if (!external.notes.empty()) {
            baseline.summary += " " + external.notes;
        }
    } else {
        baseline.pdfa_status = domain::PdfComplianceStatus::validated_fail;
        baseline.blockers.insert(
            baseline.blockers.begin(),
            "External PDF/A validator reported non-zero exit ("
                + std::to_string(external.exit_code) + ")");
        baseline.summary =
            "PDF/A=validated_fail via " + baseline.validator_name
            + "; PDF/UA=unsupported; do not claim compliance.";
        if (!external.notes.empty()) {
            baseline.summary += " " + external.notes;
        }
    }
    return baseline;
}

domain::PdfComplianceAssessment merge_optional_pac_result(
    domain::PdfComplianceAssessment baseline,
    const domain::ExternalPdfaValidatorResult& pac)
{
    // Hard rule: QPainter pipeline cannot produce tagged-PDF / PDF/UA pass.
    baseline.pdfua_status = domain::PdfComplianceStatus::unsupported;

    if (!pac.tool_configured) {
        if (baseline.summary.find("PDF/UA=unsupported") == std::string::npos) {
            baseline.summary +=
                " PDF/UA=unsupported (DATALAB_PAC unset; no tagged-PDF).";
        }
        return baseline;
    }

    baseline.blockers.push_back(
        "PAC probe configured; QPainter export remains untagged — "
        "PDF/UA cannot become validated_pass");

    if (!pac.tool_available) {
        baseline.summary +=
            " PAC configured but unavailable; PDF/UA=unsupported.";
        return baseline;
    }

    if (!pac.tool_invoked) {
        baseline.summary +=
            " PAC configured but not invoked; PDF/UA=unsupported.";
        return baseline;
    }

    // Record PAC identity for audit trail without upgrading UA status.
    if (!pac.validator_name.empty()) {
        if (baseline.validator_name.empty()) {
            baseline.validator_name = pac.validator_name;
        } else if (baseline.validator_name.find("PAC") == std::string::npos) {
            baseline.validator_name += "+" + pac.validator_name;
        }
        if (!pac.validator_version.empty()) {
            baseline.validator_version =
                baseline.validator_version.empty()
                    ? pac.validator_version
                    : (baseline.validator_version + "; PAC " + pac.validator_version);
        }
    }
    baseline.validator_invoked = baseline.validator_invoked || pac.tool_invoked;
    baseline.summary +=
        " PAC probe exit_code=" + std::to_string(pac.exit_code)
        + " recorded; PDF/UA remains unsupported on QPainter "
          "(exit 0 is not UA compliance).";
    if (!pac.notes.empty()) {
        baseline.summary += " " + pac.notes;
    }
    return baseline;
}

void apply_pdf_compliance_assessment(
    domain::ReportExportManifest& manifest,
    const domain::PdfComplianceAssessment& assessment)
{
    manifest.pdfa_status = assessment.pdfa_status;
    manifest.pdfua_status = assessment.pdfua_status;
    manifest.export_pipeline = assessment.export_pipeline;
    manifest.compliance_blockers = assessment.blockers;
    manifest.validator_notes = assessment.summary;
    if (assessment.validator_invoked) {
        manifest.validator_name = assessment.validator_name;
        manifest.validator_version = assessment.validator_version;
    } else {
        // Avoid leaving a name that could imply a pass without invocation.
        if (assessment.pdfa_status == domain::PdfComplianceStatus::not_validated
            || assessment.pdfa_status == domain::PdfComplianceStatus::unsupported) {
            manifest.validator_name.clear();
            manifest.validator_version.clear();
        } else {
            manifest.validator_name = assessment.validator_name;
            manifest.validator_version = assessment.validator_version;
        }
    }
}

domain::ReportExportManifest build_export_manifest(
    const domain::ReportDocument& document,
    const ReportExportPaths& paths)
{
    domain::ReportExportManifest manifest;
    manifest.schema_version = 1;
    manifest.report_id = document.provenance.report_id;
    manifest.template_id = document.profile.profile_id.empty()
        ? domain::report_template_kind_id(document.profile.template_kind)
        : document.profile.profile_id;
    manifest.template_version = document.profile.template_version;
    manifest.locale_language_tag = document.profile.locale.language_tag;
    manifest.timezone_id = document.profile.locale.timezone_id;
    manifest.generated_at_utc = document.provenance.generated_at_utc;
    manifest.software_version = document.provenance.software_version.empty()
        ? document.software_version
        : document.provenance.software_version;
    manifest.algorithm_version = document.provenance.algorithm_version;
    manifest.facts_hash = document.provenance.facts_hash;
    manifest.input_snapshot_hash = document.provenance.input_snapshot_hash;
    manifest.pdf_relative_path = paths.pdf_file_name;
    manifest.audit_json_relative_path = paths.audit_json_file_name;
    manifest.title = "DataLab Quality Report / DataLab 质量分析报告";
    manifest.author = "DataLab";
    std::ostringstream ids;
    for (std::size_t index = 0; index < document.pages.size(); ++index) {
        if (index > 0) {
            ids << ',';
        }
        const std::string& page_id = document.pages[index].source_page.id;
        ids << (page_id.empty() ? ("page-" + std::to_string(index + 1)) : page_id);
    }
    manifest.analysis_page_ids = ids.str();
    manifest.consistency_status = "not_checked";
    apply_pdf_compliance_assessment(manifest, assess_pdf_export_pipeline());
    return manifest;
}

bool manifest_matches_document(
    const domain::ReportExportManifest& manifest,
    const domain::ReportDocument& document,
    std::string* mismatch_reason)
{
    const auto fail = [&](const char* reason) {
        if (mismatch_reason != nullptr) {
            *mismatch_reason = reason;
        }
        return false;
    };
    if (manifest.report_id != document.provenance.report_id) {
        return fail("report_id mismatch");
    }
    const std::string expected_template = document.profile.profile_id.empty()
        ? domain::report_template_kind_id(document.profile.template_kind)
        : document.profile.profile_id;
    if (manifest.template_id != expected_template) {
        return fail("template_id mismatch");
    }
    if (manifest.template_version != document.profile.template_version) {
        return fail("template_version mismatch");
    }
    if (manifest.locale_language_tag != document.profile.locale.language_tag) {
        return fail("locale mismatch");
    }
    if (manifest.facts_hash != document.provenance.facts_hash) {
        return fail("facts_hash mismatch");
    }
    if (manifest.input_snapshot_hash != document.provenance.input_snapshot_hash) {
        return fail("input_snapshot_hash mismatch");
    }
    if (manifest.pdfa_status == domain::PdfComplianceStatus::validated_pass
        || manifest.pdfua_status == domain::PdfComplianceStatus::validated_pass) {
        if (manifest.validator_name.empty()) {
            return fail("validated_pass without validator_name is forbidden");
        }
    }
    // QPainter export cannot honestly claim PDF/UA validation.
    if (manifest.pdfua_status == domain::PdfComplianceStatus::validated_pass
        && manifest.export_pipeline.find("QPainter") != std::string::npos) {
        return fail("PDF/UA validated_pass forbidden on QPainter pipeline");
    }
    return true;
}

std::string format_report_display_value(const std::string& raw_text)
{
    if (raw_text.empty()) {
        return "—";
    }
    double value = 0.0;
    if (!looks_like_number(raw_text, &value)) {
        return raw_text;
    }
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return value > 0.0 ? "+∞" : "-∞";
    }
    return raw_text;
}

}  // namespace datalab::application
