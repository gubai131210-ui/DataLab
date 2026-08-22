#pragma once

// Report-facing stable text IDs. Domain Facts keep numeric values and stable IDs;
// presentation resolves these IDs by report locale (not UI/system locale).

#include "domain/report_types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::domain {

struct ReportTextEntry {
    std::string id;
    std::string zh_cn;
    std::string en_us;
};

struct MissingTranslation {
    std::string text_id;
    std::string requested_language_tag;
    std::string fallback_language_tag;
    std::string fallback_text;
};

struct ReportTextCoverage {
    std::string language_tag;
    std::size_t catalog_size = 0;
    std::size_t translated_count = 0;
    std::size_t missing_count = 0;
    double coverage_ratio = 0.0;
};

struct ReportTextResolveResult {
    std::string text;
    bool used_fallback = false;
};

const std::vector<ReportTextEntry>& report_text_catalog();

bool catalog_has_text_id(const std::string& text_id);

ReportTextResolveResult resolve_report_text(
    const std::string& text_id,
    const std::string& language_tag,
    std::vector<MissingTranslation>* missing_out = nullptr);

ReportTextCoverage report_text_coverage(const std::string& language_tag);

// Locale-aware number/date formatting without Qt (report locale only).
std::string format_report_number(
    double value, const ReportLocaleSettings& locale, int precision = 6);
std::string format_report_percent(
    double ratio_0_to_1, const ReportLocaleSettings& locale, int precision = 2);
std::string format_report_datetime_utc(
    const std::string& iso_utc, const ReportLocaleSettings& locale);

std::string localize_template_kind(
    ReportTemplateKind kind, const std::string& language_tag,
    std::vector<MissingTranslation>* missing_out = nullptr);

std::string localize_evidence_kind(
    EvidenceKind kind, const std::string& language_tag,
    std::vector<MissingTranslation>* missing_out = nullptr);

std::string localize_evidence_ref_label(
    const EvidenceRef& ref, const std::string& language_tag,
    std::vector<MissingTranslation>* missing_out = nullptr);

std::string localize_evidence_status(
    const std::string& status, const std::string& language_tag,
    std::vector<MissingTranslation>* missing_out = nullptr);

std::string format_evidence_ref_display_line(
    const EvidenceRef& ref, const std::string& language_tag, bool include_row_count,
    std::vector<MissingTranslation>* missing_out = nullptr);

}  // namespace datalab::domain
