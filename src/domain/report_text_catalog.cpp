#include "domain/report_text_catalog.h"
#include "domain/report_text_catalog_parts.h"

#include <cmath>
#include <cstdio>
#include <sstream>

namespace datalab::domain {
namespace {

bool is_english(const std::string& language_tag)
{
    return language_tag == "en" || language_tag == "en-US" || language_tag == "en_US"
        || language_tag.rfind("en-", 0) == 0;
}

std::string pick_text(
    const ReportTextEntry& entry,
    const std::string& language_tag,
    bool* used_fallback)
{
    if (is_english(language_tag)) {
        if (!entry.en_us.empty()) {
            if (used_fallback != nullptr) {
                *used_fallback = false;
            }
            return entry.en_us;
        }
        if (used_fallback != nullptr) {
            *used_fallback = true;
        }
        return entry.zh_cn;
    }
    if (!entry.zh_cn.empty()) {
        if (used_fallback != nullptr) {
            *used_fallback = false;
        }
        return entry.zh_cn;
    }
    if (used_fallback != nullptr) {
        *used_fallback = true;
    }
    return entry.en_us;
}

}  // namespace

const std::vector<ReportTextEntry>& report_text_catalog()
{
    static const std::vector<ReportTextEntry> catalog = [] {
        std::vector<ReportTextEntry> entries;
        entries.reserve(2551);
        append_report_text_catalog_part1(entries);
        append_report_text_catalog_part2(entries);
        append_report_text_catalog_part3(entries);
        append_report_text_catalog_part4(entries);
        append_report_text_catalog_part5(entries);
        append_report_text_catalog_part6(entries);
        append_report_text_catalog_part7(entries);
        append_report_text_catalog_part8(entries);
        append_report_text_catalog_part9(entries);
        append_report_text_catalog_part10(entries);
        append_report_text_catalog_part11(entries);
        append_report_text_catalog_part12(entries);
        append_report_text_catalog_part13(entries);
        append_report_text_catalog_part14(entries);
        append_report_text_catalog_part15(entries);
        append_report_text_catalog_part16(entries);
        return entries;
    }();
    return catalog;
}

bool catalog_has_text_id(const std::string& text_id)
{
    for (const ReportTextEntry& entry : report_text_catalog()) {
        if (entry.id == text_id) {
            return true;
        }
    }
    return false;
}

ReportTextResolveResult resolve_report_text(
    const std::string& text_id,
    const std::string& language_tag,
    std::vector<MissingTranslation>* missing_out)
{
    ReportTextResolveResult result;
    for (const ReportTextEntry& entry : report_text_catalog()) {
        if (entry.id != text_id) {
            continue;
        }
        bool used_fallback = false;
        result.text = pick_text(entry, language_tag, &used_fallback);
        result.used_fallback = used_fallback;
        if (used_fallback && missing_out != nullptr) {
            MissingTranslation missing;
            missing.text_id = text_id;
            missing.requested_language_tag = language_tag;
            missing.fallback_language_tag = is_english(language_tag) ? "zh-CN" : "en-US";
            missing.fallback_text = result.text;
            missing_out->push_back(missing);
        }
        return result;
    }
    result.text = text_id;
    result.used_fallback = true;
    if (missing_out != nullptr) {
        MissingTranslation missing;
        missing.text_id = text_id;
        missing.requested_language_tag = language_tag;
        missing.fallback_language_tag = "id";
        missing.fallback_text = text_id;
        missing_out->push_back(missing);
    }
    return result;
}

ReportTextCoverage report_text_coverage(const std::string& language_tag)
{
    ReportTextCoverage coverage;
    coverage.language_tag = language_tag;
    coverage.catalog_size = report_text_catalog().size();
    for (const ReportTextEntry& entry : report_text_catalog()) {
        const bool has = is_english(language_tag) ? !entry.en_us.empty() : !entry.zh_cn.empty();
        if (has) {
            ++coverage.translated_count;
        } else {
            ++coverage.missing_count;
        }
    }
    coverage.coverage_ratio = coverage.catalog_size == 0
        ? 0.0
        : static_cast<double>(coverage.translated_count)
            / static_cast<double>(coverage.catalog_size);
    return coverage;
}

std::string format_report_number(
    double value, const ReportLocaleSettings& locale, int precision)
{
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return value > 0.0 ? "+∞" : "-∞";
    }
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "%.*g", precision, value);
    std::string text = buffer;
    if (is_english(locale.number_format_locale) || is_english(locale.language_tag)) {
        // Keep '.' decimal separator for en-US.
        return text;
    }
    // zh-CN display uses '.' for engineering reports in DataLab; do not invent thousand separators
    // that would alter stored Facts. Formatting must not change Facts fingerprint values.
    return text;
}

std::string format_report_percent(
    double ratio_0_to_1, const ReportLocaleSettings& locale, int precision)
{
    const double percent = ratio_0_to_1 * 100.0;
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "%.*f", precision, percent);
    if (is_english(locale.language_tag)) {
        return std::string(buffer) + "%";
    }
    return std::string(buffer) + "%";
}

std::string format_report_datetime_utc(
    const std::string& iso_utc, const ReportLocaleSettings& locale)
{
    if (iso_utc.empty()) {
        return resolve_report_text("report.unrecorded", locale.language_tag).text;
    }
    // Keep ISO for audit stability; locale only affects label language elsewhere.
    return iso_utc + " (" + locale.timezone_id + ")";
}

std::string localize_template_kind(
    ReportTemplateKind kind,
    const std::string& language_tag,
    std::vector<MissingTranslation>* missing_out)
{
    switch (kind) {
    case ReportTemplateKind::customer:
        return resolve_report_text("template.customer", language_tag, missing_out).text;
    case ReportTemplateKind::audit:
        return resolve_report_text("template.audit", language_tag, missing_out).text;
    case ReportTemplateKind::engineer:
    default:
        return resolve_report_text("template.engineer", language_tag, missing_out).text;
    }
}

std::string localize_evidence_kind(
    EvidenceKind kind,
    const std::string& language_tag,
    std::vector<MissingTranslation>* missing_out)
{
    std::string text_id = "evidence." + evidence_kind_id(kind);
    if (kind == EvidenceKind::parameter) {
        text_id = "evidence.parameters";
    }
    return resolve_report_text(text_id, language_tag, missing_out).text;
}

std::string localize_evidence_ref_label(
    const EvidenceRef& ref,
    const std::string& language_tag,
    std::vector<MissingTranslation>* missing_out)
{
    if (!ref.label_text_id.empty()) {
        return resolve_report_text(ref.label_text_id, language_tag, missing_out).text;
    }
    return localize_evidence_kind(ref.kind, language_tag, missing_out);
}

std::string localize_evidence_status(
    const std::string& status,
    const std::string& language_tag,
    std::vector<MissingTranslation>* missing_out)
{
    if (status == "present") {
        return resolve_report_text("status.present", language_tag, missing_out).text;
    }
    if (status == "missing") {
        return resolve_report_text("status.missing", language_tag, missing_out).text;
    }
    if (status == "truncated") {
        return resolve_report_text("status.truncated", language_tag, missing_out).text;
    }
    if (status == "not_applicable") {
        return resolve_report_text("status.not_applicable", language_tag, missing_out).text;
    }
    return status;
}

std::string format_evidence_ref_display_line(
    const EvidenceRef& ref,
    const std::string& language_tag,
    const bool include_row_count,
    std::vector<MissingTranslation>* missing_out)
{
    std::string line = localize_evidence_ref_label(ref, language_tag, missing_out)
        + " | id=" + ref.evidence_id + " | status="
        + localize_evidence_status(ref.status, language_tag, missing_out);
    if (include_row_count) {
        line += " | rows=" + std::to_string(ref.source_rows.size());
    }
    if (!ref.formula_ref_id.empty()) {
        line += " | formula=" + ref.formula_ref_id;
    }
    if (!ref.notes_text_id.empty()) {
        line += " | notes="
            + resolve_report_text(ref.notes_text_id, language_tag, missing_out).text;
    }
    return line;
}

}  // namespace datalab::domain
