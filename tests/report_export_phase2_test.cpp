#include "application/analysis_service.h"
#include "application/graph_service.h"
#include "application/interpretation_service.h"
#include "application/report_assembly_service.h"
#include "application/report_export_service.h"
#include "application/report_localization.h"
#include "domain/quality_types.h"
#include "domain/report_types.h"
#include "infrastructure/report_export_writer.h"
#include "infrastructure/report_painter.h"
#include "infrastructure/report_serialization.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

using datalab::application::AnalysisService;
using datalab::application::GraphService;
using datalab::application::InterpretationService;
using datalab::application::ReportAssemblyOptions;
using datalab::application::localize_report_document;
using datalab::application::manifest_matches_document;
using datalab::application::apply_pdf_compliance_assessment;
using datalab::application::assess_pdf_export_pipeline;
using datalab::application::build_evidence_bundle;
using datalab::application::build_export_manifest;
using datalab::application::build_report_document;
using datalab::application::format_report_display_value;
using datalab::application::make_report_export_paths;
using datalab::application::merge_external_pdfa_validator_result;
using datalab::application::merge_optional_pac_result;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::CapabilityFacts;
using datalab::domain::DataTable;
using datalab::domain::ExternalPdfaValidatorResult;
using datalab::domain::OutputPage;
using datalab::domain::PdfComplianceStatus;
using datalab::domain::ReportDocument;
using datalab::domain::ReportExportManifest;
using datalab::domain::ReportTemplateKind;
using datalab::domain::make_report_profile;
using datalab::domain::resolve_report_text;
using datalab::infrastructure::export_report_package;
using datalab::infrastructure::report_export_manifest_from_json;
using datalab::infrastructure::report_export_manifest_to_json;
using datalab::infrastructure::run_optional_verapdf;

namespace {

DataTable sample_table()
{
    DataTable table;
    table.name = "PinLength";
    table.source_path = "samples/capability/PinLength.csv";
    table.import_metadata.dataset_id = "phase0_report_capability_pin";
    table.import_metadata.filter_summary = "none";
    table.columns = {"长度μ", "Length"};
    table.rows = {{"10.1", ""}, {"inf", "-1.5"}, {"9.9", "12%"}};
    table.row_ids = {1, 2, 3};
    return table;
}

OutputPage sample_page()
{
    OutputPage page;
    page.id = "cap-phase2";
    page.title = "过程能力";
    page.method_name = "Capability Analysis";
    page.parameter_summary = "LSL=9;USL=11";
    page.method_metadata.algorithm = "capability_normal";
    page.method_metadata.version = "2";
    page.method_metadata.valid_count = 3;
    CapabilityFacts capability;
    capability.cp = 1.33;
    capability.cpk = 1.21;
    capability.assumption_status = "not_verified";
    capability.method = "normal";
    page.facts.capability = capability;
    return page;
}

ReportDocument sample_document()
{
    ReportAssemblyOptions options;
    options.generated_at_utc = "2026-08-21T12:00:00Z";
    options.software_version = "DataLab";
    return build_report_document(
        sample_table(),
        {sample_page()},
        make_report_profile(ReportTemplateKind::audit),
        options);
}

bool utf8_codepoint_at(
    const std::string& text, std::size_t& index, std::uint32_t& codepoint)
{
    if (index >= text.size()) {
        return false;
    }
    const auto byte = static_cast<unsigned char>(text[index]);
    if (byte < 0x80) {
        codepoint = byte;
        index += 1;
        return true;
    }
    if ((byte & 0xE0) == 0xC0 && index + 1 < text.size()) {
        codepoint = ((byte & 0x1F) << 6)
            | (static_cast<unsigned char>(text[index + 1]) & 0x3F);
        index += 2;
        return true;
    }
    if ((byte & 0xF0) == 0xE0 && index + 2 < text.size()) {
        codepoint = ((byte & 0x0F) << 12)
            | ((static_cast<unsigned char>(text[index + 1]) & 0x3F) << 6)
            | (static_cast<unsigned char>(text[index + 2]) & 0x3F);
        index += 3;
        return true;
    }
    if ((byte & 0xF8) == 0xF0 && index + 3 < text.size()) {
        codepoint = ((byte & 0x07) << 18)
            | ((static_cast<unsigned char>(text[index + 1]) & 0x3F) << 12)
            | ((static_cast<unsigned char>(text[index + 2]) & 0x3F) << 6)
            | (static_cast<unsigned char>(text[index + 3]) & 0x3F);
        index += 4;
        return true;
    }
    index += 1;
    return false;
}

bool contains_cjk(const std::string& text)
{
    std::size_t index = 0;
    std::uint32_t codepoint = 0;
    while (utf8_codepoint_at(text, index, codepoint)) {
        if ((codepoint >= 0x4E00 && codepoint <= 0x9FFF)
            || (codepoint >= 0x3400 && codepoint <= 0x4DBF)
            || (codepoint >= 0x3000 && codepoint <= 0x303F)
            || (codepoint >= 0xFF00 && codepoint <= 0xFFEF)) {
            return true;
        }
    }
    return false;
}

void assert_visible_layer_no_cjk(
    const datalab::domain::ReportPageView& page, const char* slice_name)
{
    const auto check = [&](const std::string& field, const char* kind) {
        if (!contains_cjk(field)) {
            return;
        }
        const std::string preview =
            field.size() > 120 ? field.substr(0, 120) + "..." : field;
        QVERIFY2(
            false,
            qPrintable(QStringLiteral("%1 %2 contains CJK after en-US localization: %3")
                           .arg(QString::fromUtf8(slice_name))
                           .arg(QString::fromUtf8(kind))
                           .arg(QString::fromStdString(preview))));
    };
    check(page.source_page.title, "title");
    check(page.source_page.parameter_summary, "parameter_summary");
    for (const auto& section : page.visible_interpretation) {
        check(section.heading, "interpretation.heading");
        for (const auto& bullet : section.bullets) {
            check(bullet, "interpretation.bullet");
        }
    }
    for (const auto& diagnostic : page.visible_diagnostics) {
        check(diagnostic.message, "diagnostic.message");
    }
    for (const auto& table : page.visible_tables) {
        check(table.title, "table.title");
        for (const auto& header : table.headers) {
            check(header, "table.header");
        }
    }
    for (const auto& plot : page.visible_plots) {
        check(plot.title, "plot.title");
    }
}

void assert_visible_layer_excludes_substrings(
    const datalab::domain::ReportPageView& page,
    const char* slice_name,
    std::initializer_list<const char*> forbidden)
{
    const auto check = [&](const std::string& field, const char* kind) {
        for (const char* token : forbidden) {
            if (field.find(token) == std::string::npos) {
                continue;
            }
            const std::string preview =
                field.size() > 120 ? field.substr(0, 120) + "..." : field;
            QVERIFY2(
                false,
                qPrintable(QStringLiteral("%1 %2 leaked forbidden token '%3' after en-US localization: %4")
                               .arg(QString::fromUtf8(slice_name))
                               .arg(QString::fromUtf8(kind))
                               .arg(QString::fromUtf8(token))
                               .arg(QString::fromStdString(preview))));
        }
    };
    check(page.source_page.title, "title");
    check(page.source_page.parameter_summary, "parameter_summary");
    for (const auto& section : page.visible_interpretation) {
        check(section.heading, "interpretation.heading");
        for (const auto& bullet : section.bullets) {
            check(bullet, "interpretation.bullet");
        }
    }
    for (const auto& diagnostic : page.visible_diagnostics) {
        check(diagnostic.message, "diagnostic.message");
    }
    for (const auto& table : page.visible_tables) {
        check(table.title, "table.title");
        for (const auto& header : table.headers) {
            check(header, "table.header");
        }
    }
    for (const auto& plot : page.visible_plots) {
        check(plot.title, "plot.title");
    }
}

void assert_visible_layer_no_english_catalog_leaks(
    const datalab::domain::ReportPageView& page,
    const char* slice_name,
    std::initializer_list<const char*> extra_forbidden = {})
{
    static const char* forbidden[] = {
        "Quality Analysis Report",
        "Page ",
        "Formula reference (not vendor_oracle)",
        "Central composite design (CCD)",
        "Warranty summary",
        "Reliability analysis",
        "Scatterplot (faceted)",
        "Scatterplot",
        "Design matrix",
        "Design information",
        "Kaplan-Meier survival table",
        "Claims per 1000",
        "Box-Cox transformation",
        "Facet =",
        "process pass/fail decision",
    };
    const auto check = [&](const std::string& field, const char* kind) {
        auto scan = [&](const char* token) {
            if (field.find(token) == std::string::npos) {
                return;
            }
            QVERIFY2(
                false,
                qPrintable(QStringLiteral("%1 %2 leaked English catalog token '%3' after zh-CN localization")
                               .arg(QString::fromUtf8(slice_name))
                               .arg(QString::fromUtf8(kind))
                               .arg(QString::fromUtf8(token))));
        };
        for (const char* token : forbidden) {
            scan(token);
        }
        for (const char* token : extra_forbidden) {
            scan(token);
        }
    };
    check(page.source_page.title, "title");
    check(page.source_page.parameter_summary, "parameter_summary");
    for (const auto& section : page.visible_interpretation) {
        check(section.heading, "interpretation.heading");
        for (const auto& bullet : section.bullets) {
            check(bullet, "interpretation.bullet");
        }
    }
    for (const auto& diagnostic : page.visible_diagnostics) {
        check(diagnostic.message, "diagnostic.message");
    }
    for (const auto& table : page.visible_tables) {
        check(table.title, "table.title");
        for (const auto& header : table.headers) {
            check(header, "table.header");
        }
    }
    for (const auto& plot : page.visible_plots) {
        check(plot.title, "plot.title");
    }
}

}  // namespace

class ReportExportPhase2Test final : public QObject {
    Q_OBJECT

private slots:
    void manifest_matches_document_identity_fields()
    {
        const ReportDocument document = sample_document();
        const auto paths = make_report_export_paths("quality_report.pdf");
        ReportExportManifest manifest = build_export_manifest(document, paths);
        std::string reason;
        QVERIFY(manifest_matches_document(manifest, document, &reason));
        QCOMPARE(manifest.report_id, document.provenance.report_id);
        QCOMPARE(manifest.template_id, document.profile.profile_id);
        QCOMPARE(manifest.locale_language_tag, document.profile.locale.language_tag);
        QCOMPARE(manifest.facts_hash, document.provenance.facts_hash);
        QCOMPARE(
            static_cast<int>(manifest.pdfa_status),
            static_cast<int>(PdfComplianceStatus::not_validated));
        QCOMPARE(
            static_cast<int>(manifest.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));
        QVERIFY(!manifest.compliance_blockers.empty());
        QVERIFY(manifest.validator_notes.find("do not claim compliance") != std::string::npos);
        const auto assessment = assess_pdf_export_pipeline();
        QCOMPARE(
            static_cast<int>(assessment.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));
        QVERIFY(assessment.blockers.size() >= 3);
    }

    void manifest_detects_mismatch_and_forbids_fake_validation()
    {
        const ReportDocument document = sample_document();
        auto paths = make_report_export_paths("quality_report.pdf");
        ReportExportManifest manifest = build_export_manifest(document, paths);
        manifest.facts_hash = "tampered";
        std::string reason;
        QVERIFY(!manifest_matches_document(manifest, document, &reason));
        QVERIFY(reason.find("facts_hash") != std::string::npos);

        manifest = build_export_manifest(document, paths);
        manifest.pdfa_status = PdfComplianceStatus::validated_pass;
        manifest.validator_name.clear();
        QVERIFY(!manifest_matches_document(manifest, document, &reason));
    }

    void merge_verapdf_result_keeps_pdfua_unsupported()
    {
        const auto baseline = assess_pdf_export_pipeline();
        ExternalPdfaValidatorResult missing;
        missing.tool_configured = false;
        auto merged = merge_external_pdfa_validator_result(baseline, missing);
        QCOMPARE(
            static_cast<int>(merged.pdfa_status),
            static_cast<int>(PdfComplianceStatus::not_validated));
        QCOMPARE(
            static_cast<int>(merged.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));
        QVERIFY(!merged.validator_invoked);

        ExternalPdfaValidatorResult pass;
        pass.tool_configured = true;
        pass.tool_available = true;
        pass.tool_invoked = true;
        pass.exit_code = 0;
        pass.validator_name = "veraPDF";
        pass.validator_version = "1.24-mock";
        merged = merge_external_pdfa_validator_result(baseline, pass);
        QCOMPARE(
            static_cast<int>(merged.pdfa_status),
            static_cast<int>(PdfComplianceStatus::validated_pass));
        QCOMPARE(
            static_cast<int>(merged.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));
        QVERIFY(merged.validator_invoked);
        QCOMPARE(merged.validator_name, std::string("veraPDF"));

        ReportExportManifest manifest =
            build_export_manifest(sample_document(), make_report_export_paths("x.pdf"));
        apply_pdf_compliance_assessment(manifest, merged);
        QCOMPARE(manifest.validator_name, std::string("veraPDF"));
        std::string reason;
        QVERIFY(manifest_matches_document(manifest, sample_document(), &reason));

        // PDF/UA pass on QPainter path must be rejected even with a validator name.
        manifest.pdfua_status = PdfComplianceStatus::validated_pass;
        QVERIFY(!manifest_matches_document(manifest, sample_document(), &reason));
        QVERIFY(reason.find("PDF/UA") != std::string::npos);

        ExternalPdfaValidatorResult fail;
        fail.tool_configured = true;
        fail.tool_available = true;
        fail.tool_invoked = true;
        fail.exit_code = 1;
        fail.validator_name = "veraPDF";
        merged = merge_external_pdfa_validator_result(baseline, fail);
        QCOMPARE(
            static_cast<int>(merged.pdfa_status),
            static_cast<int>(PdfComplianceStatus::validated_fail));
        QCOMPARE(
            static_cast<int>(merged.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));

        ExternalPdfaValidatorResult unnamed_pass;
        unnamed_pass.tool_configured = true;
        unnamed_pass.tool_available = true;
        unnamed_pass.tool_invoked = true;
        unnamed_pass.exit_code = 0;
        unnamed_pass.validator_name.clear();
        merged = merge_external_pdfa_validator_result(baseline, unnamed_pass);
        QCOMPARE(
            static_cast<int>(merged.pdfa_status),
            static_cast<int>(PdfComplianceStatus::not_validated));
        QCOMPARE(
            static_cast<int>(merged.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));

        ExternalPdfaValidatorResult timeout_like;
        timeout_like.tool_configured = true;
        timeout_like.tool_available = true;
        timeout_like.tool_invoked = false;
        timeout_like.exit_code = 0;  // must not become pass when not invoked
        timeout_like.validator_name = "veraPDF";
        timeout_like.notes = "timed out";
        merged = merge_external_pdfa_validator_result(baseline, timeout_like);
        QCOMPARE(
            static_cast<int>(merged.pdfa_status),
            static_cast<int>(PdfComplianceStatus::not_validated));
        QVERIFY(!merged.validator_invoked);
    }

    void merge_pac_result_never_validates_pdfua_even_on_exit_zero()
    {
        const auto baseline = assess_pdf_export_pipeline();
        ExternalPdfaValidatorResult pac_pass;
        pac_pass.tool_configured = true;
        pac_pass.tool_available = true;
        pac_pass.tool_invoked = true;
        pac_pass.exit_code = 0;
        pac_pass.validator_name = "PAC";
        pac_pass.validator_version = "mock";
        pac_pass.notes = "mock PAC exit 0";
        auto merged = merge_optional_pac_result(baseline, pac_pass);
        QCOMPARE(
            static_cast<int>(merged.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));
        QVERIFY(merged.summary.find("unsupported") != std::string::npos);
        QVERIFY(merged.summary.find("exit 0 is not UA compliance") != std::string::npos
                || merged.summary.find("exit_code=0") != std::string::npos);

        ExternalPdfaValidatorResult pdfa_then_pac;
        pdfa_then_pac = pac_pass;
        auto after_pdfa = merge_external_pdfa_validator_result(baseline, [&] {
            ExternalPdfaValidatorResult vera;
            vera.tool_configured = true;
            vera.tool_available = true;
            vera.tool_invoked = true;
            vera.exit_code = 0;
            vera.validator_name = "veraPDF";
            return vera;
        }());
        merged = merge_optional_pac_result(after_pdfa, pdfa_then_pac);
        QCOMPARE(
            static_cast<int>(merged.pdfa_status),
            static_cast<int>(PdfComplianceStatus::validated_pass));
        QCOMPARE(
            static_cast<int>(merged.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));
    }

    void optional_verapdf_unset_leaves_not_validated()
    {
        qputenv("DATALAB_VERAPDF", QByteArray());
        const ExternalPdfaValidatorResult external =
            run_optional_verapdf(QStringLiteral("missing.pdf"));
        QVERIFY(!external.tool_configured);
        const auto merged =
            merge_external_pdfa_validator_result(assess_pdf_export_pipeline(), external);
        QCOMPARE(
            static_cast<int>(merged.pdfa_status),
            static_cast<int>(PdfComplianceStatus::not_validated));
        QCOMPARE(
            static_cast<int>(merged.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));
    }

    void after_pdf_hook_keeps_pdfua_unsupported_when_unset()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path = directory.filePath(QStringLiteral("hook.pdf"));
        const ReportDocument document = sample_document();
        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("hook.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        const auto exported = export_report_package(
            pdf_path,
            document,
            manifest,
            [&document](ReportExportManifest& mutable_manifest, const QString& temp_pdf) {
                const ExternalPdfaValidatorResult external = run_optional_verapdf(temp_pdf);
                apply_pdf_compliance_assessment(
                    mutable_manifest,
                    merge_external_pdfa_validator_result(
                        assess_pdf_export_pipeline(), external));
                std::string reason;
                QVERIFY(manifest_matches_document(mutable_manifest, document, &reason));
            });
        QVERIFY(exported.ok);
        QCOMPARE(
            static_cast<int>(exported.manifest.pdfa_status),
            static_cast<int>(PdfComplianceStatus::not_validated));
        QCOMPARE(
            static_cast<int>(exported.manifest.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));
    }

    void optional_pdf_byte_scan_finds_report_id_and_facts_hash_literals()
    {
        // Best-effort optional PDF "text extract": QPainter header embeds ASCII
        // report_id / facts_hash. This is NOT full PDF text extraction and does
        // not claim PDF/A/UA compliance.
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path = directory.filePath(QStringLiteral("scan.pdf"));
        const ReportDocument document = sample_document();
        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("scan.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());
        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY(exported.ok);
        QVERIFY(QFileInfo::exists(pdf_path));

        QFile pdf_file(pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY(bytes.size() > 100);
        const QByteArray report_id =
            QByteArray::fromStdString(document.provenance.report_id);
        const QByteArray facts_hash =
            QByteArray::fromStdString(document.provenance.facts_hash);
        QVERIFY2(!report_id.isEmpty(), "sample document must carry report_id");
        QVERIFY2(
            bytes.contains(report_id) || bytes.contains("report_id="),
            "PDF should embed report_id header literal when QPainter draws ASCII");
        if (!facts_hash.empty()) {
            QVERIFY2(
                bytes.contains(facts_hash) || bytes.contains("facts_hash="),
                "PDF should embed facts_hash header literal when present");
        }
    }

    void paginated_table_repeats_header_on_each_content_page()
    {
        // Algorithm-level proof that table headers repeat after page breaks.
        // Complements (does not replace) human visual acceptance of PDF layout.
        // Does not claim PDF/A or PDF/UA compliance.
        double cursor_y = 0.0;
        const double page_height = 100.0;
        const double header_height = 32.0;
        const double row_height = 24.0;
        int header_paints = 0;
        int page_count = 1;
        int rows_painted = 0;

        datalab::infrastructure::ReportPainter::paint_paginated_table(
            20,
            header_height,
            [&](std::size_t) { return row_height; },
            [&](double needed) { return cursor_y + needed > page_height; },
            [&] {
                ++page_count;
                cursor_y = 0.0;
            },
            [&] {
                ++header_paints;
                cursor_y += header_height;
            },
            [&](std::size_t) {
                ++rows_painted;
                cursor_y += row_height;
            });

        QCOMPARE(rows_painted, 20);
        QVERIFY2(page_count >= 2, "20 tall rows must span multiple pages");
        QCOMPARE(header_paints, page_count);
        QVERIFY2(
            header_paints >= 2,
            "each content page after a table break must re-paint column headers");
    }

    void manifest_json_round_trip()
    {
        const ReportDocument document = sample_document();
        ReportExportManifest original =
            build_export_manifest(document, make_report_export_paths("r.pdf"));
        original.consistency_status = "ok";
        const ReportExportManifest restored =
            report_export_manifest_from_json(report_export_manifest_to_json(original));
        QCOMPARE(restored.report_id, original.report_id);
        QCOMPARE(restored.template_id, original.template_id);
        QCOMPARE(restored.locale_language_tag, original.locale_language_tag);
        QCOMPARE(restored.facts_hash, original.facts_hash);
        QCOMPARE(restored.consistency_status, std::string("ok"));
        QCOMPARE(
            static_cast<int>(restored.pdfa_status),
            static_cast<int>(PdfComplianceStatus::not_validated));
    }

    void display_formatting_handles_empty_and_non_finite()
    {
        QCOMPARE(format_report_display_value(""), std::string("—"));
        QCOMPARE(format_report_display_value("nan"), std::string("NaN"));
        // Platform strtod may accept "inf"
        const std::string inf_text = format_report_display_value("inf");
        QVERIFY(inf_text == "+∞" || inf_text == "inf");
        QCOMPARE(format_report_display_value("长度μ"), std::string("长度μ"));
        QCOMPARE(format_report_display_value("-12.5"), std::string("-12.5"));
    }

    void atomic_package_export_writes_pdf_audit_and_manifest()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path = directory.filePath(QStringLiteral("phase2_report.pdf"));
        const ReportDocument document = sample_document();
        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("phase2_report.pdf"));
        QVERIFY(manifest_matches_document(manifest, document, nullptr));
        manifest.consistency_status = "ok";

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));
        QVERIFY(QFileInfo::exists(exported.pdf_path));
        QVERIFY(QFileInfo::exists(exported.audit_json_path));
        QVERIFY(QFileInfo::exists(exported.manifest_path));
        QVERIFY(QFileInfo(exported.pdf_path).size() > 500);
        QVERIFY(!QFile::exists(exported.pdf_path + QStringLiteral(".tmp")));

        QFile manifest_file(exported.manifest_path);
        QVERIFY(manifest_file.open(QIODevice::ReadOnly));
        const auto loaded = report_export_manifest_from_json(
            QJsonDocument::fromJson(manifest_file.readAll()).object());
        QCOMPARE(loaded.report_id, document.provenance.report_id);
        QCOMPARE(loaded.template_id, document.profile.profile_id);
        QCOMPARE(loaded.locale_language_tag, document.profile.locale.language_tag);
        QCOMPARE(loaded.facts_hash, document.provenance.facts_hash);
        QCOMPARE(loaded.consistency_status, std::string("ok"));
        QCOMPARE(
            static_cast<int>(loaded.pdfa_status),
            static_cast<int>(PdfComplianceStatus::not_validated));
        QCOMPARE(
            static_cast<int>(loaded.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));
        QVERIFY(!loaded.compliance_blockers.empty());
    }

    void en_us_localized_package_keeps_locale_honesty_and_audit_evidence()
    {
        // PDF联调 deepen: en-US localize → atomic package; assert locale +
        // honesty blockers + audit JSON carries localized interpretation.
        // Does NOT claim PDF/A or PDF/UA compliance.
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("en_us_honesty.pdf"));

        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "最大类别为“泄漏”，计数 12，单项占比 40%。",
            "保修摘要：T_w = 365 day，暴露量 = 1000，claims/1000 = 12.5（口径 = units）。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "不得把统计预测写成法律/质量承诺；分母与时间窗口必须一并阅读。",
            "Johnson 变换后的 Pp/Ppk 是变换尺度上的 overall 指数；"
            "未拒绝变换后正态假设不等于原始数据服从正态分布，也不能写成过程合格。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T01:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(sample_table(), {page}, profile, options);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized =
            datalab::application::localize_report_document(document);
        document = localized.document;

        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("en_us_honesty.pdf"));
        QVERIFY(manifest_matches_document(manifest, document, nullptr));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));
        QVERIFY(QFileInfo::exists(exported.pdf_path));
        QVERIFY(QFileInfo::exists(exported.audit_json_path));
        QVERIFY(QFileInfo::exists(exported.manifest_path));

        QFile manifest_file(exported.manifest_path);
        QVERIFY(manifest_file.open(QIODevice::ReadOnly));
        const auto loaded = report_export_manifest_from_json(
            QJsonDocument::fromJson(manifest_file.readAll()).object());
        QCOMPARE(loaded.locale_language_tag, std::string("en-US"));
        QCOMPARE(
            static_cast<int>(loaded.pdfa_status),
            static_cast<int>(PdfComplianceStatus::not_validated));
        QCOMPARE(
            static_cast<int>(loaded.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));
        QVERIFY(!loaded.compliance_blockers.empty());
        QVERIFY(
            loaded.validator_notes.find("do not claim compliance") != std::string::npos
            || loaded.validator_notes.find("not_validated") != std::string::npos);

        QFile audit_file(exported.audit_json_path);
        QVERIFY(audit_file.open(QIODevice::ReadOnly));
        const QJsonObject audit =
            QJsonDocument::fromJson(audit_file.readAll()).object();
        QCOMPARE(audit.value(QStringLiteral("schema_version")).toInt(), 1);
        const QJsonObject profile_obj =
            audit.value(QStringLiteral("profile")).toObject();
        const QJsonObject locale_obj =
            profile_obj.value(QStringLiteral("locale")).toObject();
        QCOMPARE(
            locale_obj.value(QStringLiteral("language_tag")).toString().toStdString(),
            std::string("en-US"));
        QVERIFY(audit.contains(QStringLiteral("evidence")));
        const QJsonObject evidence_obj =
            audit.value(QStringLiteral("evidence")).toObject();
        QVERIFY(evidence_obj.contains(QStringLiteral("quality_evidence"))
                || evidence_obj.contains(QStringLiteral("provenance")));
        const auto& conc =
            document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("最大类别") == std::string::npos);
        QVERIFY(conc[0].find("12") != std::string::npos);
        QVERIFY(conc[1].find("Warranty") != std::string::npos
                || conc[1].find("365") != std::string::npos);
        QVERIFY(conc[1].find("保修摘要") == std::string::npos);
        const auto& lim =
            document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(lim[0].find("法律/质量") == std::string::npos);
        QVERIFY(lim[1].find("Johnson") != std::string::npos);
        QVERIFY(lim[1].find("也不能写成过程合格") == std::string::npos);

        // Optional byte scan: report_id literal only — not a compliance proof.
        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY(bytes.size() > 500);
        const QByteArray report_id =
            QByteArray::fromStdString(document.provenance.report_id);
        QVERIFY2(
            report_id.isEmpty() || bytes.contains(report_id)
                || bytes.contains("report_id="),
            "PDF may embed report_id header when drawn");
    }

    void failed_export_does_not_leave_fake_success_pdf()
    {
        // Use an invalid directory name that cannot be created as a writable file path parent
        // by pointing pdf path at a non-writable nested path under temp after removing parent.
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString missing_dir =
            directory.filePath(QStringLiteral("missing_child_dir/nope.pdf"));
        const ReportDocument document = sample_document();
        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("nope.pdf"));
        manifest.consistency_status = "ok";
        const auto exported = export_report_package(missing_dir, document, manifest);
        QVERIFY(!exported.ok);
        QVERIFY(!QFileInfo::exists(missing_dir));
        QVERIFY(!QFileInfo::exists(missing_dir + QStringLiteral(".tmp")));
    }

    void evidence_bundle_label_text_ids_resolve_in_en_us()
    {
        OutputPage page = sample_page();
        page.diagnostics.push_back({
            datalab::domain::DiagnosticMessage::Severity::warning,
            "assumption_not_verified",
            "能力指标未验证过程稳定性和正态性；数值仅供调查，不能单独作为过程合格结论。"});
        datalab::domain::PlotSpec plot;
        plot.kind = datalab::domain::PlotKind::histogram;
        plot.title = "Length 直方图";
        plot.source_rows = {1, 2, 3};
        page.plots.push_back(plot);

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-21T12:00:00Z";
        options.software_version = "DataLab";
        const auto profile = make_report_profile(ReportTemplateKind::audit);
        const ReportDocument document = build_report_document(
            sample_table(), {page}, profile, options);
        const datalab::domain::EvidenceBundle bundle = build_evidence_bundle(
            sample_table(), page, document.provenance, profile);

        QVERIFY(!bundle.evidence.empty());
        bool saw_dataset = false;
        bool saw_plot = false;
        for (const datalab::domain::EvidenceRef& ref : bundle.evidence) {
            if (!ref.label_text_id.empty()) {
                const std::string label =
                    resolve_report_text(ref.label_text_id, "en-US").text;
                QVERIFY2(
                    label.find("数据集") == std::string::npos,
                    (ref.label_text_id + " -> " + label).c_str());
                QVERIFY2(
                    label.find("快照") == std::string::npos,
                    (ref.label_text_id + " -> " + label).c_str());
                if (ref.label_text_id == "evidence.dataset_snapshot") {
                    saw_dataset = true;
                    QCOMPARE(label, std::string("Dataset snapshot"));
                }
                if (ref.label_text_id == "evidence.plot") {
                    saw_plot = true;
                    QCOMPARE(label, std::string("Chart embed (with excluded/hidden counts)"));
                }
            }
            if (!ref.notes_text_id.empty()) {
                const std::string notes =
                    resolve_report_text(ref.notes_text_id, "en-US").text;
                QVERIFY2(
                    notes.find("关联行") == std::string::npos,
                    (ref.notes_text_id + " -> " + notes).c_str());
            }
        }
        QVERIFY(saw_dataset);
        QVERIFY(saw_plot);
        QVERIFY(!bundle.rules.empty() || !page.diagnostics.empty());
    }

    void pdf_evidence_appendix_uses_localized_label_text_ids()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("evidence_labels.pdf"));

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(sample_table(), {sample_page()}, profile, options);

        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("evidence_labels.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(bytes.size() > 500, "audit PDF should contain drawn evidence appendix");
        QVERIFY2(
            bytes.contains("Dataset snapshot"),
            "PDF evidence appendix should render localized label_text_id");
        QVERIFY2(
            bytes.contains("Evidence appendix"),
            "PDF should render localized evidence appendix heading");
        QVERIFY2(
            !bytes.contains("数据集快照"),
            "en-US PDF evidence appendix must not leak Chinese labels");
        QVERIFY2(
            !bytes.contains("| dataset_snapshot |"),
            "PDF should not use raw evidence kind slug as display column");
        QVERIFY2(
            bytes.contains("id=cap-phase2:dataset"),
            "stable evidence_id must remain in PDF for audit traceability");
    }

    void pdf_empty_chart_renders_localized_no_data_message()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("empty_chart_en.pdf"));

        OutputPage page = sample_page();
        datalab::domain::PlotSpec plot;
        plot.kind = datalab::domain::PlotKind::control;
        plot.title = "I Chart";
        page.plots.push_back(plot);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(sample_table(), {page}, profile, options);

        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("empty_chart_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("No displayable data"),
            "en-US PDF empty chart should render localized empty-state text");
        QVERIFY2(
            !bytes.contains("没有可显示的数据"),
            "en-US PDF empty chart must not leak Chinese empty-state text");
    }

    void pdf_graph_scatter_all_excluded_renders_localized_no_data_en_us()
    {
        // Phase 3 S5 pre-filter: Graph scatter with all rows excluded → localized empty chart state.
        DataTable table;
        table.columns = {"x", "y"};
        table.rows = {{"1.0", "2.0"}, {"2.0", "3.0"}, {"3.0", "4.0"}};
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "scatter";
        configuration.graph.x_column = 0;
        configuration.graph.y_column = 1;
        configuration.excluded_rows = {0, 1, 2};
        OutputPage page = GraphService::run(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(!page.plots.empty());
        QCOMPARE(page.plots.front().x_values.size(), static_cast<std::size_t>(0));

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        document = localize_report_document(document).document;
        QVERIFY(!document.pages.empty());

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("graph_scatter_all_excluded_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("graph_scatter_all_excluded_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));
        QCOMPARE(exported.manifest.consistency_status, std::string("ok"));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("No displayable data"),
            "en-US PDF all-excluded scatter should render localized empty-state text");
        QVERIFY2(
            !bytes.contains("没有可显示的数据"),
            "en-US PDF all-excluded scatter must not leak Chinese empty-state text");
        QVERIFY2(
            bytes.contains("Scatterplot"),
            "en-US PDF should still render localized scatter page title");
        QVERIFY2(
            !bytes.contains("散点图"),
            "en-US PDF all-excluded scatter must not leak Chinese page title");
    }

    void audit_json_evidence_refs_carry_label_text_ids()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("audit_evidence_ids.pdf"));

        OutputPage page = sample_page();
        datalab::domain::PlotSpec plot;
        plot.kind = datalab::domain::PlotKind::histogram;
        plot.title = "Length 直方图";
        plot.source_rows = {1, 2, 3};
        page.plots.push_back(plot);

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(sample_table(), {page}, profile, options);

        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("audit_evidence_ids.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));
        QVERIFY(QFileInfo::exists(exported.audit_json_path));

        QFile audit_file(exported.audit_json_path);
        QVERIFY(audit_file.open(QIODevice::ReadOnly));
        const QJsonObject audit =
            QJsonDocument::fromJson(audit_file.readAll()).object();
        QVERIFY(audit.contains(QStringLiteral("evidence")));
        const QJsonObject evidence_root =
            audit.value(QStringLiteral("evidence")).toObject();
        const QJsonArray evidence_refs =
            evidence_root.value(QStringLiteral("evidence")).toArray();
        QVERIFY2(!evidence_refs.isEmpty(), "audit bundle must serialize evidence refs");

        bool saw_dataset = false;
        bool saw_plot = false;
        for (const QJsonValue& value : evidence_refs) {
            const QJsonObject ref = value.toObject();
            const std::string evidence_id =
                ref.value(QStringLiteral("evidence_id")).toString().toStdString();
            const std::string label_text_id =
                ref.value(QStringLiteral("label_text_id")).toString().toStdString();
            const std::string kind =
                ref.value(QStringLiteral("kind")).toString().toStdString();
            QVERIFY2(!evidence_id.empty(), "evidence_id required for audit traceability");
            QVERIFY2(!label_text_id.empty(), evidence_id.c_str());
            QVERIFY2(!kind.empty(), evidence_id.c_str());
            const std::string label =
                resolve_report_text(label_text_id, "en-US").text;
            QVERIFY2(label.find("数据集") == std::string::npos, label.c_str());
            if (label_text_id == "evidence.dataset_snapshot") {
                saw_dataset = true;
                QCOMPARE(label, std::string("Dataset snapshot"));
            }
            if (label_text_id == "evidence.plot") {
                saw_plot = true;
                QCOMPARE(label, std::string("Chart embed (with excluded/hidden counts)"));
            }
        }
        QVERIFY(saw_dataset);
        QVERIFY(saw_plot);
    }

    void pdf_warranty_summary_page_localizes_to_en_us_without_chinese_leak()
    {
        // Phase 5 byte-scan: en-US audit PDF from real warranty service page.
        // Best-effort QPainter ASCII embed — not full PDF text extraction.
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("warranty_en.pdf"));

        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.exposure = 1000.0;
        configuration.reliability.reliability_at_warranty = 0.95;
        configuration.reliability.model = "weibull";
        configuration.reliability.warranty_observed_failures = 2;
        configuration.reliability.warranty_censored_count = 1;
        configuration.reliability.warranty_valid_count = 10;
        DataTable table;
        const OutputPage page =
            AnalysisService::reliability_warranty(table, configuration);
        QVERIFY(page.facts.warranty.has_value());
        QVERIFY2(
            page.facts.warranty->claims_per_1000 > 0.0,
            "warranty page must carry claims_per_1000 for PDF scan");

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        const auto localized = localize_report_document(document);
        document = localized.document;
        QVERIFY(!document.pages.empty());
        QCOMPARE(
            document.pages[0].source_page.title,
            std::string("Warranty summary"));
        QVERIFY2(
            document.pages[0].source_page.parameter_summary.find("Exposure")
                != std::string::npos,
            "parameter_summary should localize 暴露量 to Exposure");

        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("warranty_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(bytes.size() > 500, "warranty audit PDF should contain drawn content");
        QVERIFY2(
            bytes.contains("Warranty summary"),
            "en-US warranty PDF should render localized page/table title");
        QVERIFY2(
            bytes.contains("Claims per 1000"),
            "warranty table property labels should appear in PDF");
        QVERIFY2(
            !bytes.contains("保修摘要"),
            "en-US warranty PDF must not leak Chinese page title");
        QVERIFY2(
            !bytes.contains("暴露量"),
            "en-US warranty PDF must not leak Chinese parameter summary tokens");
    }

    void pdf_warranty_invalid_exposure_localizes_to_en_us_without_chinese_leak()
    {
        // Phase 5 REL exposure gate: invalid column value + zero column sum → en-US PDF byte scan.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto export_warranty_pdf = [&](const DataTable& table,
                                       const AnalysisConfiguration& configuration,
                                       const QString& file_stem) -> QByteArray {
            OutputPage page =
                AnalysisService::reliability_warranty(table, configuration);
            InterpretationService::enrich(page);

            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            QVERIFY(!document.pages.empty());

            QTemporaryDir directory;
            QVERIFY(directory.isValid());
            const QString pdf_path = directory.filePath(file_stem + QStringLiteral(".pdf"));
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(file_stem.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        DataTable invalid_table;
        invalid_table.columns = {"time", "censor_type", "exposure"};
        invalid_table.rows = {
            {"10", "exact", "1.5"},
            {"15", "right", "-1"},
            {"20", "exact", "1.0"}};
        AnalysisConfiguration invalid_cfg;
        invalid_cfg.chart_type = "reliability_warranty";
        invalid_cfg.reliability.warranty_time = 1000.0;
        invalid_cfg.reliability.time_unit = "hours";
        invalid_cfg.reliability.exposure = 9999.0;
        invalid_cfg.reliability.reliability_at_warranty = 0.98;
        invalid_cfg.reliability.exposure_column = 2;
        invalid_cfg.reliability.model = "weibull";
        const OutputPage invalid_page =
            AnalysisService::reliability_warranty(invalid_table, invalid_cfg);
        QVERIFY(!invalid_page.diagnostics.empty());
        QCOMPARE(
            QString::fromStdString(invalid_page.diagnostics.front().code),
            QStringLiteral("invalid_exposure_value"));

        const QByteArray invalid_bytes = export_warranty_pdf(
            invalid_table,
            invalid_cfg,
            QStringLiteral("warranty_invalid_exposure_en"));
        QVERIFY2(
            invalid_bytes.contains("not silently imputed"),
            "en-US PDF should render localized invalid exposure diagnostic");
        QVERIFY2(
            !invalid_bytes.contains("静默补齐"),
            "en-US warranty PDF must not leak Chinese invalid exposure diagnostic");
        QVERIFY2(
            !invalid_bytes.contains("Claims per 1000"),
            "invalid exposure must not render warranty summary metrics");

        DataTable zero_table;
        zero_table.columns = {"exposure"};
        zero_table.rows = {{"10"}, {"20"}, {"30"}};
        AnalysisConfiguration zero_cfg;
        zero_cfg.chart_type = "reliability_warranty";
        zero_cfg.reliability.warranty_time = 1000.0;
        zero_cfg.reliability.time_unit = "hours";
        zero_cfg.reliability.reliability_at_warranty = 0.9;
        zero_cfg.reliability.exposure_column = 0;
        zero_cfg.excluded_rows = {0, 1, 2};
        zero_cfg.reliability.exposure = 500.0;
        const OutputPage zero_page =
            AnalysisService::reliability_warranty(zero_table, zero_cfg);
        QVERIFY(!zero_page.diagnostics.empty());
        QCOMPARE(
            QString::fromStdString(zero_page.diagnostics.front().code),
            QStringLiteral("warranty_zero_exposure"));

        const QByteArray zero_bytes = export_warranty_pdf(
            zero_table,
            zero_cfg,
            QStringLiteral("warranty_zero_exposure_en"));
        QVERIFY2(
            zero_bytes.contains("silent fallback"),
            "en-US PDF should render localized zero-exposure diagnostic");
        QVERIFY2(
            !zero_bytes.contains("静默"),
            "en-US warranty PDF must not leak Chinese zero-exposure diagnostic");
        QVERIFY2(
            !zero_bytes.contains("Claims per 1000"),
            "zero exposure must not render warranty summary metrics");
    }

    void pdf_warranty_exposure_gate_localizes_and_audit_evidence_carries_label_text_id()
    {
        // Phase 5 REL exposure gate → audit en-US PDF + audit JSON gate traceability.
        DataTable invalid_table;
        invalid_table.columns = {"time", "censor_type", "exposure"};
        invalid_table.rows = {
            {"10", "exact", "1.5"},
            {"15", "right", "-1"},
            {"20", "exact", "1.0"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.exposure = 9999.0;
        configuration.reliability.reliability_at_warranty = 0.98;
        configuration.reliability.exposure_column = 2;
        configuration.reliability.model = "weibull";
        OutputPage page =
            AnalysisService::reliability_warranty(invalid_table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(!page.facts.warranty.has_value());
        QVERIFY(!page.diagnostics.empty());
        QCOMPARE(
            QString::fromStdString(page.diagnostics.front().code),
            QStringLiteral("invalid_exposure_value"));

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(invalid_table, {page}, profile, options);
        document = localize_report_document(document).document;
        QVERIFY(!document.pages.empty());

        bool saw_exposure_gate = false;
        for (const auto& ref : document.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:warranty_exposure") == std::string::npos) {
                continue;
            }
            saw_exposure_gate = true;
            QCOMPARE(ref.label_text_id, std::string("evidence.warranty_exposure_gate"));
            QCOMPARE(ref.diagnostic_code, std::string("invalid_exposure_value"));
            QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
        }
        QVERIFY2(saw_exposure_gate, "audit evidence must include warranty exposure gate ref");

        bool saw_limitation = false;
        for (const auto& section : document.pages[0].visible_interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("warranty summary metrics were skipped")
                        != std::string::npos) {
                    saw_limitation = true;
                }
                QVERIFY2(
                    bullet.find("已跳过保修摘要指标") == std::string::npos,
                    "localized interpretation must not leak Chinese exposure gate bullet");
            }
        }
        QVERIFY2(
            saw_limitation,
            "en-US interpretation must surface warranty exposure gate limitation bullet");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("warranty_exposure_gate_audit_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("warranty_exposure_gate_audit_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("not silently imputed"),
            "en-US PDF should render localized invalid exposure diagnostic");
        QVERIFY2(
            bytes.contains("Warranty exposure gate"),
            "audit PDF evidence appendix should render localized exposure gate label");
        QVERIFY2(
            bytes.contains("warranty summary metrics were skipped"),
            "en-US PDF should render localized warranty exposure gate limitation bullet");
        QVERIFY2(
            !bytes.contains("保修摘要"),
            "en-US warranty exposure gate PDF must not leak Chinese page title");

        QVERIFY(QFileInfo::exists(exported.audit_json_path));
        QFile audit_file(exported.audit_json_path);
        QVERIFY(audit_file.open(QIODevice::ReadOnly));
        const QJsonObject audit =
            QJsonDocument::fromJson(audit_file.readAll()).object();
        const QJsonArray evidence_refs =
            audit.value(QStringLiteral("evidence")).toObject()
                .value(QStringLiteral("evidence")).toArray();
        bool saw_audit_exposure_gate = false;
        for (const QJsonValue& value : evidence_refs) {
            const QJsonObject ref = value.toObject();
            const std::string label_text_id =
                ref.value(QStringLiteral("label_text_id")).toString().toStdString();
            if (label_text_id != "evidence.warranty_exposure_gate") {
                continue;
            }
            saw_audit_exposure_gate = true;
            QCOMPARE(
                ref.value(QStringLiteral("diagnostic_code")).toString(),
                QStringLiteral("invalid_exposure_value"));
            const std::string label =
                resolve_report_text(label_text_id, "en-US").text;
            QVERIFY2(
                label.find("Warranty exposure gate") != std::string::npos,
                label.c_str());
            QVERIFY2(label.find("暴露量门禁") == std::string::npos, label.c_str());
        }
        QVERIFY2(
            saw_audit_exposure_gate,
            "audit JSON must serialize warranty exposure gate with label_text_id");
    }

    void pdf_warranty_exposure_cross_template_table_visibility_and_en_us_locale()
    {
        // Phase 5 REL exposure gate × customer/engineer/audit — no summary tables + en-US diagnostics.
        DataTable zero_table;
        zero_table.columns = {"exposure"};
        zero_table.rows = {{"10"}, {"20"}, {"30"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.reliability_at_warranty = 0.9;
        configuration.reliability.exposure_column = 0;
        configuration.excluded_rows = {0, 1, 2};
        configuration.reliability.exposure = 500.0;
        OutputPage page =
            AnalysisService::reliability_warranty(zero_table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(!page.facts.warranty.has_value());
        QCOMPARE(
            QString::fromStdString(page.diagnostics.front().code),
            QStringLiteral("warranty_zero_exposure"));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(zero_table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QCOMPARE(
            customer_doc.provenance.facts_hash,
            engineer_doc.provenance.facts_hash);
        QCOMPARE(engineer_doc.provenance.facts_hash, audit_doc.provenance.facts_hash);

        const auto assert_zero_exposure_en = [&](const ReportDocument& doc,
                                                 const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            QVERIFY(doc.pages[0].visible_tables.empty());
            bool saw_zero_exposure = false;
            for (const auto& diagnostic : doc.pages[0].visible_diagnostics) {
                if (diagnostic.code == "warranty_zero_exposure") {
                    saw_zero_exposure = true;
                    QVERIFY2(
                        diagnostic.message.find("silent fallback") != std::string::npos,
                        label);
                }
            }
            QVERIFY2(
                saw_zero_exposure,
                "en-US profile must surface localized zero-exposure diagnostic");
            bool saw_exposure_gate = false;
            for (const auto& ref : doc.pages[0].visible_evidence) {
                if (ref.evidence_id.find(":gate:warranty_exposure") != std::string::npos) {
                    saw_exposure_gate = true;
                    QCOMPARE(ref.label_text_id, std::string("evidence.warranty_exposure_gate"));
                    QCOMPARE(ref.diagnostic_code, std::string("warranty_zero_exposure"));
                    QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                }
            }
            QVERIFY2(
                saw_exposure_gate,
                "all en-US profiles must retain warranty exposure limiting gate ref");
            bool saw_limitation = false;
            for (const auto& section : doc.pages[0].visible_interpretation) {
                for (const auto& bullet : section.bullets) {
                    if (bullet.find("warranty summary metrics were skipped")
                            != std::string::npos) {
                        saw_limitation = true;
                    }
                }
            }
            QVERIFY2(
                saw_limitation,
                "en-US profile must surface localized warranty exposure gate limitation");
        };
        assert_zero_exposure_en(customer_doc, "customer en-US");
        assert_zero_exposure_en(engineer_doc, "engineer en-US");
        assert_zero_exposure_en(audit_doc, "audit en-US");

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("warranty_zero_exposure_audit_en.pdf"));
        QVERIFY2(
            audit_bytes.contains("silent fallback"),
            "audit PDF should render localized zero-exposure diagnostic");
        QVERIFY2(
            audit_bytes.contains("Warranty exposure gate"),
            "audit PDF should render localized exposure gate label in evidence appendix");
        QVERIFY2(
            audit_bytes.contains("warranty summary metrics were skipped"),
            "audit PDF should render localized warranty exposure gate limitation bullet");
        QVERIFY2(
            !audit_bytes.contains("Claims per 1000"),
            "zero exposure must not render warranty summary metrics in audit PDF");
    }

    void pdf_warranty_exposure_column_override_localizes_to_en_us_without_chinese_leak()
    {
        // Phase 5 REL: exposure column overrides scalar → en-US engineer PDF byte scan.
        DataTable table;
        table.columns = {"time", "censor_type", "exposure"};
        table.rows = {
            {"10", "exact", "1.5"},
            {"15", "right", "2.5"},
            {"20", "exact", "1.0"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.exposure = 9999.0;
        configuration.reliability.reliability_at_warranty = 0.98;
        configuration.reliability.exposure_column = 2;
        configuration.reliability.model = "weibull";
        OutputPage page =
            AnalysisService::reliability_warranty(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.warranty.has_value());
        QCOMPARE(
            QString::fromStdString(page.facts.warranty->exposure_source),
            QStringLiteral("column_sum"));
        QVERIFY(qAbs(page.facts.warranty->exposure - 5.0) < 1e-9);

        bool saw_override = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "warranty_exposure_column_overrides_scalar") {
                saw_override = true;
            }
        }
        QVERIFY2(saw_override, "service must emit exposure column override diagnostic");

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        document = localize_report_document(document).document;
        QVERIFY(!document.pages.empty());

        bool saw_localized_override = false;
        for (const auto& diagnostic : document.pages[0].visible_diagnostics) {
            if (diagnostic.code == "warranty_exposure_column_overrides_scalar") {
                saw_localized_override = true;
                QVERIFY2(
                    diagnostic.message.find("column sum and ignores the scalar")
                        != std::string::npos,
                    diagnostic.message.c_str());
            }
        }
        QVERIFY2(
            saw_localized_override,
            "engineer visible_diagnostics must include localized override diagnostic");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("warranty_exposure_override_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("warranty_exposure_override_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("column sum and ignores the scalar"),
            "en-US PDF should render localized exposure override diagnostic");
        QVERIFY2(
            bytes.contains("column_sum"),
            "en-US PDF should show column_sum exposure source in summary table");
        QVERIFY2(
            !bytes.contains("标量被忽略"),
            "en-US warranty override PDF must not leak Chinese override diagnostic");
        QVERIFY2(
            !bytes.contains("9999"),
            "ignored scalar exposure must not appear as effective exposure in PDF");
    }

    void pdf_warranty_strata_tables_localize_to_en_us_without_chinese_leak()
    {
        // Phase 5 REL-4: failure-mode denominator table in en-US audit PDF.
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("warranty_strata_en.pdf"));

        DataTable table;
        table.columns = {"time", "censor_type", "mode", "exposure"};
        table.rows = {
            {"10", "exact", "wear", "10"},
            {"12", "exact", "wear", "10"},
            {"15", "right", "wear", "20"},
            {"8", "exact", "early", "5"},
            {"30", "right", "", "5"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.reliability_at_warranty = 0.95;
        configuration.reliability.censoring_type_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.exposure_column = 3;
        configuration.reliability.model = "weibull";
        const OutputPage page =
            AnalysisService::reliability_warranty(table, configuration);
        QVERIFY(page.facts.warranty.has_value());
        QCOMPARE(
            QString::fromStdString(page.facts.warranty->stratum_kind),
            QStringLiteral("failure_mode"));
        QVERIFY(page.facts.warranty->strata.size() >= 2);

        bool saw_stratum_table = false;
        for (const auto& table_block : page.tables) {
            if (table_block.title == "失效模式分母追溯") {
                saw_stratum_table = true;
                break;
            }
        }
        QVERIFY2(saw_stratum_table, "warranty service must emit failure-mode stratum table");

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        const auto localized = localize_report_document(document);
        document = localized.document;
        QVERIFY(!document.pages.empty());
        QVERIFY(!document.pages[0].visible_tables.empty());

        bool localized_stratum_title = false;
        for (const auto& table_block : document.pages[0].visible_tables) {
            if (table_block.title == "Failure-mode denominator trace") {
                localized_stratum_title = true;
            }
            QVERIFY2(
                table_block.title.find("失效模式") == std::string::npos,
                "visible_tables must not retain Chinese stratum title after localization");
        }
        QVERIFY2(localized_stratum_title, "audit visible_tables must include localized stratum table");

        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("warranty_strata_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(bytes.size() > 800, "strata warranty PDF should contain multiple tables");
        QVERIFY2(
            bytes.contains("Failure-mode denominator trace"),
            "en-US PDF should render localized stratum table title");
        QVERIFY2(
            bytes.contains("Warranty summary"),
            "en-US PDF should still render localized summary table title");
        QVERIFY2(
            !bytes.contains("失效模式分母追溯"),
            "en-US warranty strata PDF must not leak Chinese stratum table title");
    }

    void pdf_warranty_cross_template_table_visibility_and_en_us_locale()
    {
        // REL-4 cross-template: customer hides statistic tables; engineer/audit keep them.
        DataTable table;
        table.columns = {"time", "censor_type", "mode", "exposure"};
        table.rows = {
            {"10", "exact", "wear", "10"},
            {"12", "exact", "wear", "10"},
            {"15", "right", "wear", "20"},
            {"8", "exact", "early", "5"},
            {"30", "right", "", "5"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.reliability_at_warranty = 0.95;
        configuration.reliability.censoring_type_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.exposure_column = 3;
        configuration.reliability.model = "weibull";
        const OutputPage page =
            AnalysisService::reliability_warranty(table, configuration);
        QVERIFY(page.facts.warranty.has_value());

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            return document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QVERIFY(customer_doc.pages[0].visible_tables.empty());
        QVERIFY(!engineer_doc.pages[0].visible_tables.empty());
        QVERIFY(!audit_doc.pages[0].visible_tables.empty());
        QCOMPARE(
            customer_doc.provenance.facts_hash,
            engineer_doc.provenance.facts_hash);
        QCOMPARE(engineer_doc.provenance.facts_hash, audit_doc.provenance.facts_hash);

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            std::string reason;
            QVERIFY2(
                manifest_matches_document(manifest, document, &reason),
                reason.c_str());
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("warranty_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("warranty_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("warranty_audit_en.pdf"));

        QVERIFY2(
            customer_bytes.contains("Warranty summary"),
            "customer PDF should still render localized page title");
        QVERIFY2(
            !customer_bytes.contains("Failure-mode denominator trace"),
            "customer PDF must omit stratum statistic table");
        QVERIFY2(
            !customer_bytes.contains("Claims per 1000"),
            "customer PDF must omit warranty statistic table rows");

        for (const QByteArray& bytes :
             {engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("Warranty summary"),
                "engineer/audit PDF should render localized page title");
            QVERIFY2(
                bytes.contains("Failure-mode denominator trace"),
                "engineer/audit PDF should render localized stratum table title");
            QVERIFY2(
                bytes.contains("Claims per 1000"),
                "engineer/audit PDF should render warranty summary table rows");
            QVERIFY2(
                !bytes.contains("失效模式分母追溯"),
                "en-US engineer/audit PDF must not leak Chinese stratum title");
        }

        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include full evidence appendix heading");
        QVERIFY2(
            !engineer_bytes.contains("Evidence appendix"),
            "engineer PDF should not include audit-only evidence appendix");
    }

    void pdf_warranty_summary_cross_template_table_visibility_and_en_us_locale()
    {
        // Phase 5 REL-4 scalar warranty: customer hides statistic tables; engineer/audit keep them.
        DataTable table;
        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.exposure = 1000.0;
        configuration.reliability.reliability_at_warranty = 0.95;
        configuration.reliability.model = "weibull";
        configuration.reliability.warranty_observed_failures = 2;
        configuration.reliability.warranty_censored_count = 1;
        configuration.reliability.warranty_valid_count = 10;
        const OutputPage page =
            AnalysisService::reliability_warranty(table, configuration);
        QVERIFY(page.facts.warranty.has_value());
        QVERIFY2(
            page.facts.warranty->claims_per_1000 > 0.0,
            "scalar warranty fixture must carry claims_per_1000");

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            return document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QVERIFY(customer_doc.pages[0].visible_tables.empty());
        QVERIFY(!engineer_doc.pages[0].visible_tables.empty());
        QVERIFY(!audit_doc.pages[0].visible_tables.empty());
        QCOMPARE(
            customer_doc.provenance.facts_hash,
            engineer_doc.provenance.facts_hash);
        QCOMPARE(engineer_doc.provenance.facts_hash, audit_doc.provenance.facts_hash);

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            std::string reason;
            QVERIFY2(
                manifest_matches_document(manifest, document, &reason),
                reason.c_str());
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("warranty_summary_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("warranty_summary_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("warranty_summary_audit_en.pdf"));

        QVERIFY2(
            customer_bytes.contains("Warranty summary"),
            "customer PDF should still render localized page title");
        QVERIFY2(
            !customer_bytes.contains("Claims per 1000"),
            "customer PDF must omit warranty statistic table rows");
        QVERIFY2(
            !customer_bytes.contains("Failure-mode denominator trace"),
            "scalar warranty customer PDF must not include stratum table");

        for (const QByteArray& bytes : {engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("Warranty summary"),
                "engineer/audit PDF should render localized page title");
            QVERIFY2(
                bytes.contains("Claims per 1000"),
                "engineer/audit PDF should render warranty summary table rows");
            QVERIFY2(
                !bytes.contains("保修摘要"),
                "en-US warranty summary PDF must not leak Chinese page title");
            QVERIFY2(
                !bytes.contains("失效模式分母追溯"),
                "scalar warranty PDF must not leak Chinese stratum table title");
        }

        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include full evidence appendix heading");
        QVERIFY2(
            !engineer_bytes.contains("Evidence appendix"),
            "engineer PDF should not include audit-only evidence appendix");
    }

    void pdf_johnson_capability_gate_localizes_and_audit_evidence_carries_label_text_id()
    {
        // Phase 6 CAP-NN-3: Johnson gate must stay closed; en-US PDF + audit JSON traceability.
        std::vector<double> values;
        for (int i = 1; i <= 40; ++i) {
            values.push_back(std::exp(0.05 * static_cast<double>(i))
                + 0.1 * static_cast<double>(i % 3));
        }
        DataTable table;
        table.columns = {"x"};
        for (double value : values) {
            table.rows.push_back({std::to_string(value)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "johnson";
        configuration.variable_columns = {0};
        configuration.specifications.lower = values.front();
        configuration.specifications.upper = values.back() * 1.2;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(
            QString::fromStdString(page.facts.capability->gate_status),
            QStringLiteral("gated_research"));

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        const auto localized = localize_report_document(document);
        document = localized.document;
        QVERIFY(!document.pages.empty());
        QCOMPARE(
            document.pages[0].source_page.facts.capability->pass_fail_judgment_allowed,
            false);

        bool saw_gate_diagnostic = false;
        for (const auto& diagnostic : document.pages[0].visible_diagnostics) {
            if (diagnostic.code == "johnson_capability_gated") {
                saw_gate_diagnostic = true;
                QVERIFY2(
                    diagnostic.message.find("research/preview") != std::string::npos,
                    "localized Johnson gate diagnostic must be en-US");
                QVERIFY2(
                    diagnostic.message.find("研究/预览") == std::string::npos,
                    "Johnson gate diagnostic must not leak Chinese after localization");
            }
        }
        QVERIFY2(saw_gate_diagnostic, "audit visible_diagnostics must include Johnson gate");

        bool saw_gate_evidence = false;
        for (const auto& ref : document.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:johnson") != std::string::npos) {
                saw_gate_evidence = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.johnson_capability_gated"));
                QCOMPARE(ref.diagnostic_code, std::string("johnson_capability_gated"));
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
            }
        }
        QVERIFY2(saw_gate_evidence, "audit evidence bundle must include Johnson limiting gate ref");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("johnson_gate_en.pdf"));
        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("johnson_gate_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("Johnson-transform process capability"),
            "en-US PDF should render localized Johnson capability page title");
        QVERIFY2(
            bytes.contains("Johnson capability is research/preview only"),
            "en-US PDF should render localized Johnson gate diagnostic");
        QVERIFY2(
            bytes.contains("Johnson capability gate"),
            "audit PDF evidence appendix should render localized Johnson gate label");
        QVERIFY2(
            !bytes.contains("Johnson 变换过程能力"),
            "en-US Johnson capability PDF must not leak Chinese page title");
        QVERIFY2(
            !bytes.contains("不得输出过程合格判定"),
            "en-US Johnson capability PDF must not leak Chinese gate diagnostic");

        QVERIFY(QFileInfo::exists(exported.audit_json_path));
        QFile audit_file(exported.audit_json_path);
        QVERIFY(audit_file.open(QIODevice::ReadOnly));
        const QJsonObject audit =
            QJsonDocument::fromJson(audit_file.readAll()).object();
        const QJsonArray evidence_refs =
            audit.value(QStringLiteral("evidence")).toObject()
                .value(QStringLiteral("evidence")).toArray();
        bool saw_audit_gate = false;
        for (const QJsonValue& value : evidence_refs) {
            const QJsonObject ref = value.toObject();
            const std::string label_text_id =
                ref.value(QStringLiteral("label_text_id")).toString().toStdString();
            if (label_text_id != "evidence.johnson_capability_gated") {
                continue;
            }
            saw_audit_gate = true;
            QCOMPARE(
                ref.value(QStringLiteral("diagnostic_code")).toString(),
                QStringLiteral("johnson_capability_gated"));
            const std::string label =
                resolve_report_text(label_text_id, "en-US").text;
            QVERIFY2(
                label.find("Johnson capability gate") != std::string::npos,
                label.c_str());
            QVERIFY2(label.find("Johnson 能力门禁") == std::string::npos, label.c_str());
        }
        QVERIFY2(
            saw_audit_gate,
            "audit JSON must serialize Johnson gate evidence with label_text_id");
    }

    void pdf_johnson_spec_outside_support_localizes_to_en_us_without_chinese_leak()
    {
        // Phase 6 CAP-NN-3: spec outside Johnson support → skip overall table + en-US PDF scan.
        std::vector<double> values;
        for (int i = 1; i <= 40; ++i) {
            values.push_back(std::exp(0.05 * static_cast<double>(i))
                + 0.1 * static_cast<double>(i % 3));
        }
        DataTable table;
        table.columns = {"x"};
        for (double value : values) {
            table.rows.push_back({std::to_string(value)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "johnson";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -1000.0;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);

        bool saw_spec_outside = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "johnson_spec_outside_support"
                && diagnostic.message
                       == "规格限落在 Johnson 变换定义域外，无法计算 Pp/Ppk。") {
                saw_spec_outside = true;
            }
        }
        QVERIFY2(saw_spec_outside, "service must emit johnson_spec_outside_support error");
        QVERIFY(std::none_of(
            page.tables.cbegin(), page.tables.cend(),
            [](const datalab::domain::StatisticTable& table_out) {
                return table_out.title == "Overall Capability";
            }));

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        document = localize_report_document(document).document;
        QVERIFY(!document.pages.empty());

        bool saw_localized_diag = false;
        for (const auto& diagnostic : document.pages[0].visible_diagnostics) {
            if (diagnostic.code == "johnson_spec_outside_support"
                && diagnostic.message
                       == "Specification limits fall outside the Johnson transform support; "
                          "Pp/Ppk cannot be computed.") {
                saw_localized_diag = true;
            }
        }
        QVERIFY2(
            saw_localized_diag,
            "audit visible_diagnostics must include localized full spec-outside message");

        bool saw_spec_gate = false;
        for (const auto& ref : document.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:johnson_spec_limit") != std::string::npos) {
                saw_spec_gate = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.johnson_spec_limit_gate"));
                QCOMPARE(ref.diagnostic_code, std::string("johnson_spec_outside_support"));
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
            }
        }
        QVERIFY2(saw_spec_gate, "audit evidence must include Johnson spec-limit gate ref");

        bool saw_limitation = false;
        for (const auto& section : document.pages[0].visible_interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("overall capability indices table was skipped")
                        != std::string::npos) {
                    saw_limitation = true;
                }
                QVERIFY2(
                    bullet.find("已跳过 overall 能力指数表") == std::string::npos,
                    "localized interpretation must not leak Chinese spec-limit gate");
            }
        }
        QVERIFY2(
            saw_limitation,
            "en-US interpretation must surface Johnson spec-limit gate limitation bullet");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("johnson_spec_outside_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("johnson_spec_outside_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("outside the Johnson transform support"),
            "en-US PDF should render localized Johnson spec-outside diagnostic");
        QVERIFY2(
            !bytes.contains("规格限落在 Johnson"),
            "en-US Johnson PDF must not leak Chinese spec-outside diagnostic");
        QVERIFY2(
            !bytes.contains("Overall Capability"),
            "en-US PDF must omit overall capability table when spec-limit gate blocks it");
        QVERIFY2(
            bytes.contains("overall capability indices table was skipped"),
            "en-US PDF should render localized Johnson spec-limit gate limitation bullet");
        QVERIFY2(
            bytes.contains("Johnson spec limit gate"),
            "audit PDF evidence appendix should render localized Johnson spec-limit gate label");
    }

    void pdf_johnson_spec_outside_cross_template_table_visibility_and_en_us_locale()
    {
        // Phase 6 CAP-NN-3: spec outside Johnson support × customer/engineer/audit PDF byte scan.
        std::vector<double> values;
        for (int i = 1; i <= 40; ++i) {
            values.push_back(std::exp(0.05 * static_cast<double>(i))
                + 0.1 * static_cast<double>(i % 3));
        }
        DataTable table;
        table.columns = {"x"};
        for (double value : values) {
            table.rows.push_back({std::to_string(value)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "johnson";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -1000.0;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QVERIFY(std::none_of(
            page.tables.cbegin(), page.tables.cend(),
            [](const datalab::domain::StatisticTable& table_out) {
                return table_out.title == "Overall Capability";
            }));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QCOMPARE(
            customer_doc.provenance.facts_hash,
            engineer_doc.provenance.facts_hash);
        QCOMPARE(engineer_doc.provenance.facts_hash, audit_doc.provenance.facts_hash);

        const auto assert_no_overall_table = [](const ReportDocument& doc,
                                                const char* label) {
            for (const auto& table_view : doc.pages[0].visible_tables) {
                QVERIFY2(
                    table_view.title.find("Overall Capability") == std::string::npos,
                    label);
                QVERIFY2(
                    table_view.title.find("整体能力") == std::string::npos,
                    label);
            }
        };
        assert_no_overall_table(customer_doc, "customer must omit overall capability table");
        assert_no_overall_table(engineer_doc, "engineer must omit overall capability table");
        assert_no_overall_table(audit_doc, "audit must omit overall capability table");

        const auto has_spec_outside_diag = [](const datalab::domain::ReportPageView& page) {
            for (const auto& diagnostic : page.visible_diagnostics) {
                if (diagnostic.code == "johnson_spec_outside_support"
                    && diagnostic.message.find("outside the Johnson transform support")
                        != std::string::npos) {
                    return true;
                }
            }
            return false;
        };
        QVERIFY2(
            has_spec_outside_diag(customer_doc.pages[0]),
            "customer en-US must surface localized spec-outside diagnostic");
        QVERIFY2(
            has_spec_outside_diag(engineer_doc.pages[0]),
            "engineer en-US must surface localized spec-outside diagnostic");
        QVERIFY2(
            has_spec_outside_diag(audit_doc.pages[0]),
            "audit en-US must surface localized spec-outside diagnostic");

        const auto has_spec_limit_gate = [](const datalab::domain::ReportPageView& page) {
            for (const auto& ref : page.visible_evidence) {
                if (ref.evidence_id.find(":gate:johnson_spec_limit") != std::string::npos) {
                    return true;
                }
            }
            return false;
        };
        QVERIFY2(
            has_spec_limit_gate(customer_doc.pages[0]),
            "customer en-US keeps johnson_spec_limit limiting gate ref");
        QVERIFY2(
            has_spec_limit_gate(audit_doc.pages[0]),
            "audit en-US keeps johnson_spec_limit limiting gate ref");

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("johnson_spec_outside_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("johnson_spec_outside_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("johnson_spec_outside_audit_en.pdf"));

        for (const QByteArray& bytes : {customer_bytes, engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("outside the Johnson transform support"),
                "en-US PDF should render localized spec-outside diagnostic");
            QVERIFY2(
                !bytes.contains("Overall Capability"),
                "en-US PDF must omit overall capability table when spec-limit gate blocks it");
            QVERIFY2(
                !bytes.contains("规格限落在 Johnson"),
                "en-US Johnson spec-outside PDF must not leak Chinese diagnostic");
        }
        QVERIFY2(
            audit_bytes.contains("Johnson spec limit gate"),
            "audit PDF evidence appendix should render localized Johnson spec-limit gate label");
        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include evidence appendix heading");
        QVERIFY2(
            !engineer_bytes.contains("Evidence appendix"),
            "engineer PDF should not include audit-only evidence appendix");
    }

    void pdf_johnson_capability_cross_template_table_visibility_and_en_us_locale()
    {
        // Phase 6 CAP-NN-3: Johnson gate × customer/engineer/audit PDF byte scan.
        std::vector<double> values;
        for (int i = 1; i <= 40; ++i) {
            values.push_back(std::exp(0.05 * static_cast<double>(i))
                + 0.1 * static_cast<double>(i % 3));
        }
        DataTable table;
        table.columns = {"x"};
        for (double value : values) {
            table.rows.push_back({std::to_string(value)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "johnson";
        configuration.variable_columns = {0};
        configuration.specifications.lower = values.front();
        configuration.specifications.upper = values.back() * 1.2;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            return document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QVERIFY(customer_doc.pages[0].visible_tables.empty());
        QVERIFY(!engineer_doc.pages[0].visible_tables.empty());
        QVERIFY(!audit_doc.pages[0].visible_tables.empty());
        QCOMPARE(
            customer_doc.provenance.facts_hash,
            engineer_doc.provenance.facts_hash);
        QCOMPARE(engineer_doc.provenance.facts_hash, audit_doc.provenance.facts_hash);

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("johnson_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("johnson_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("johnson_audit_en.pdf"));

        QVERIFY2(
            customer_bytes.contains("Johnson-transform process capability"),
            "customer PDF should still render localized page title");
        QVERIFY2(
            customer_bytes.contains("Johnson capability is research/preview only"),
            "customer PDF should retain Johnson gate warning diagnostic");
        QVERIFY2(
            !customer_bytes.contains("Selected Family"),
            "customer PDF must omit Johnson transform statistic table rows");

        for (const QByteArray& bytes : {engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("Johnson-transform process capability"),
                "engineer/audit PDF should render localized page title");
            QVERIFY2(
                bytes.contains("Selected Family"),
                "engineer/audit PDF should render Johnson transform table rows");
            QVERIFY2(
                !bytes.contains("Johnson 变换过程能力"),
                "en-US Johnson PDF must not leak Chinese page title");
        }

        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include full evidence appendix heading");
        QVERIFY2(
            audit_bytes.contains("Johnson capability gate"),
            "audit PDF evidence appendix should render localized Johnson gate label");
        QVERIFY2(
            !engineer_bytes.contains("Evidence appendix"),
            "engineer PDF should not include audit-only evidence appendix");
    }

    void pdf_normal_capability_stability_gate_localizes_and_audit_evidence_carries_label_text_id()
    {
        // Phase 6: normal capability stability gate → en-US audit PDF + audit JSON traceability.
        DataTable table;
        table.columns = {"x"};
        for (int i = 0; i < 25; ++i) {
            table.rows.push_back(
                {std::to_string(10.0 + ((i % 5) - 2) * 0.1)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 9.0;
        configuration.specifications.upper = 11.0;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(
            QString::fromStdString(page.facts.capability->gate_status),
            QStringLiteral("stability_unverified"));

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        const auto localized = localize_report_document(document);
        document = localized.document;
        QVERIFY(!document.pages.empty());
        QCOMPARE(
            document.pages[0].source_page.title,
            std::string("Normal process capability analysis"));

        bool saw_pass_fail_block = false;
        for (const auto& diagnostic : document.pages[0].visible_diagnostics) {
            if (diagnostic.code
                    == "capability_pass_fail_blocked_by_stability_prerequisite") {
                saw_pass_fail_block = true;
                QVERIFY2(
                    diagnostic.message.find("process pass/fail judgment is blocked")
                        != std::string::npos,
                    "localized pass/fail block diagnostic must be en-US");
                QVERIFY2(
                    diagnostic.message.find("禁止过程合格判定") == std::string::npos,
                    "pass/fail block diagnostic must not leak Chinese after localization");
            }
        }
        QVERIFY2(
            saw_pass_fail_block,
            "audit visible_diagnostics must include stability pass/fail block");

        bool saw_gate_evidence = false;
        for (const auto& ref : document.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:capability_pass_fail_blocked")
                    != std::string::npos) {
                saw_gate_evidence = true;
                QCOMPARE(
                    ref.label_text_id,
                    std::string("evidence.capability_stability_prerequisite"));
                QCOMPARE(
                    ref.diagnostic_code,
                    std::string("capability_pass_fail_blocked_by_stability_prerequisite"));
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
            }
        }
        QVERIFY2(
            saw_gate_evidence,
            "audit evidence bundle must include capability stability limiting gate ref");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("normal_cap_stability_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("normal_cap_stability_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(bytes.size() > 800, "normal capability audit PDF should contain content");
        QVERIFY2(
            bytes.contains("Normal process capability analysis"),
            "en-US PDF should render localized normal capability page title");
        QVERIFY2(
            bytes.contains("process pass/fail judgment is blocked"),
            "en-US PDF should render localized pass/fail block diagnostic");
        QVERIFY2(
            bytes.contains("Capability stability/normality prerequisite unmet")
                || bytes.contains("pass/fail blocked"),
            "audit PDF evidence appendix should render localized capability gate label");
        QVERIFY2(
            !bytes.contains("正态过程能力分析"),
            "en-US normal capability PDF must not leak Chinese page title");
        QVERIFY2(
            !bytes.contains("禁止过程合格判定"),
            "en-US normal capability PDF must not leak Chinese pass/fail block diagnostic");

        QVERIFY(QFileInfo::exists(exported.audit_json_path));
        QFile audit_file(exported.audit_json_path);
        QVERIFY(audit_file.open(QIODevice::ReadOnly));
        const QJsonArray evidence_refs =
            QJsonDocument::fromJson(audit_file.readAll())
                .object()
                .value(QStringLiteral("evidence"))
                .toObject()
                .value(QStringLiteral("evidence"))
                .toArray();
        bool saw_audit_gate = false;
        for (const QJsonValue& value : evidence_refs) {
            const QJsonObject ref = value.toObject();
            if (ref.value(QStringLiteral("label_text_id")).toString()
                    != QStringLiteral("evidence.capability_stability_prerequisite")) {
                continue;
            }
            saw_audit_gate = true;
            QCOMPARE(
                ref.value(QStringLiteral("diagnostic_code")).toString(),
                QStringLiteral("capability_pass_fail_blocked_by_stability_prerequisite"));
            const std::string label =
                ref.value(QStringLiteral("label")).toString().toStdString();
            QVERIFY2(
                label.find("pass/fail blocked") != std::string::npos
                    || label.find("prerequisite unmet") != std::string::npos,
                label.c_str());
            QVERIFY2(label.find("禁止合格判定") == std::string::npos, label.c_str());
        }
        QVERIFY2(
            saw_audit_gate,
            "audit JSON must serialize capability stability gate with label_text_id");
    }

    void pdf_normal_capability_cross_template_table_visibility_and_en_us_locale()
    {
        // Phase 6 CAP: normal capability stability gate × customer/engineer/audit PDF byte scan.
        DataTable table;
        table.columns = {"x"};
        for (int i = 0; i < 25; ++i) {
            table.rows.push_back(
                {std::to_string(10.0 + ((i % 5) - 2) * 0.1)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 9.0;
        configuration.specifications.upper = 11.0;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(
            QString::fromStdString(page.facts.capability->gate_status),
            QStringLiteral("stability_unverified"));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            return document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QVERIFY(customer_doc.pages[0].visible_tables.empty());
        QVERIFY(!engineer_doc.pages[0].visible_tables.empty());
        QVERIFY(!audit_doc.pages[0].visible_tables.empty());
        QCOMPARE(
            customer_doc.provenance.facts_hash,
            engineer_doc.provenance.facts_hash);
        QCOMPARE(engineer_doc.provenance.facts_hash, audit_doc.provenance.facts_hash);

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("normal_cap_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("normal_cap_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("normal_cap_audit_en.pdf"));

        QVERIFY2(
            customer_bytes.contains("Normal process capability analysis"),
            "customer PDF should still render localized page title");
        QVERIFY2(
            customer_bytes.contains("process pass/fail judgment is blocked"),
            "customer PDF should retain stability pass/fail warning diagnostic");
        QVERIFY2(
            !customer_bytes.contains("Capability indices"),
            "customer PDF must omit capability statistic table rows");

        for (const QByteArray& bytes : {engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("Normal process capability analysis"),
                "engineer/audit PDF should render localized page title");
            QVERIFY2(
                bytes.contains("Capability indices"),
                "engineer/audit PDF should render localized capability indices table");
            QVERIFY2(
                !bytes.contains("正态过程能力分析"),
                "en-US normal capability PDF must not leak Chinese page title");
            QVERIFY2(
                !bytes.contains("能力指数"),
                "en-US normal capability PDF must not leak Chinese capability table title");
        }

        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include full evidence appendix heading");
        QVERIFY2(
            audit_bytes.contains("Capability stability/normality prerequisite unmet")
                || audit_bytes.contains("pass/fail blocked"),
            "audit PDF evidence appendix should render localized capability gate label");
        QVERIFY2(
            !engineer_bytes.contains("Evidence appendix"),
            "engineer PDF should not include audit-only evidence appendix");
    }

    void pdf_pinlength_capability_unicode_columns_localize_without_cross_language_leak()
    {
        // Phase 3 S7 pre-filter: PinLength-like unicode column + zh/en engineer PDF without chrome mixing.
        // Manual S7 still required for real CSV import + mojibake check in Qt widgets.
        DataTable table;
        table.name = "PinLength";
        table.source_path = "samples/capability/PinLength.csv";
        table.import_metadata.dataset_id = "phase0_report_capability_pin";
        table.columns = {"长度μm", "Machine"};
        table.rows.reserve(26);
        for (int i = 0; i < 25; ++i) {
            table.rows.push_back({
                std::to_string(18.0 + static_cast<double>((i % 5) - 2) * 0.4),
                "1.0"});
        }
        table.rows.push_back({"inf", "1.0"});

        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 13.0;
        configuration.specifications.upper = 25.0;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(
            QString::fromStdString(page.facts.capability->method),
            QStringLiteral("normal"));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](const char* language_tag) {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            return document;
        };

        const ReportDocument doc_en = build_localized("en-US");
        const ReportDocument doc_zh = build_localized("zh-CN");
        QVERIFY(!doc_en.pages.empty());
        QVERIFY(!doc_zh.pages.empty());
        QCOMPARE(doc_en.provenance.facts_hash, doc_zh.provenance.facts_hash);

        assert_visible_layer_no_cjk(doc_en.pages[0], "PinLength en-US");
        assert_visible_layer_excludes_substrings(
            doc_en.pages[0],
            "PinLength en-US",
            {"正态过程能力分析", "能力指数", "变量:"});

        QCOMPARE(
            doc_zh.pages[0].source_page.title,
            std::string("正态过程能力分析"));
        assert_visible_layer_no_english_catalog_leaks(
            doc_zh.pages[0],
            "PinLength zh-CN",
            {"Normal process capability analysis",
             "Capability indices",
             "Variable =",
             "Evidence appendix"});

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray en_bytes =
            export_bytes(doc_en, QStringLiteral("pinlength_cap_en.pdf"));
        const QByteArray zh_bytes =
            export_bytes(doc_zh, QStringLiteral("pinlength_cap_zh.pdf"));

        QVERIFY2(
            en_bytes.contains("Normal process capability analysis"),
            "en-US PinLength PDF should render localized page title");
        QVERIFY2(
            en_bytes.contains("Capability indices"),
            "en-US PinLength PDF should render capability table chrome");
        QVERIFY2(
            !en_bytes.contains("正态过程能力分析"),
            "en-US PinLength PDF must not leak Chinese page title");
        QVERIFY2(
            zh_bytes.contains("正态过程能力分析"),
            "zh-CN PinLength PDF should render localized page title");
        QVERIFY2(
            !zh_bytes.contains("Normal process capability analysis"),
            "zh-CN PinLength PDF must not leak English page title");
        QVERIFY2(
            en_bytes.contains("长度μm") || zh_bytes.contains("长度μm"),
            "PinLength PDF should preserve user unicode column label in export path");
    }

    void pdf_nonnormal_capability_stability_gate_localizes_and_audit_evidence_carries_label_text_id()
    {
        // Phase 6: non-normal Weibull capability gate → en-US audit PDF + audit JSON traceability.
        DataTable table;
        table.columns = {"x"};
        for (int i = 1; i <= 30; ++i) {
            table.rows.push_back({std::to_string(0.5 * static_cast<double>(i) + 1.0)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "non_normal";
        configuration.nonnormal_distribution = "weibull";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 1.0;
        configuration.specifications.upper = 20.0;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(
            QString::fromStdString(page.facts.capability->method),
            QStringLiteral("non_normal"));

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        const auto localized = localize_report_document(document);
        document = localized.document;
        QVERIFY(!document.pages.empty());
        QCOMPARE(
            document.pages[0].source_page.title,
            std::string("Nonnormal process capability"));

        bool saw_zscore_diagnostic = false;
        bool saw_stability_diagnostic = false;
        for (const auto& diagnostic : document.pages[0].visible_diagnostics) {
            if (diagnostic.code == "nonnormal_z_score_formula_reference") {
                saw_zscore_diagnostic = true;
                QVERIFY2(
                    diagnostic.message.find("Nonnormal capability computes Pp/Ppk")
                        != std::string::npos,
                    "localized nonnormal Z-score diagnostic must be en-US");
                QVERIFY2(
                    diagnostic.message.find("非正态能力") == std::string::npos,
                    "nonnormal Z-score diagnostic must not leak Chinese");
            }
            if (diagnostic.code == "capability_stability_prerequisite"
                || diagnostic.code == "capability_pass_fail_blocked_by_stability_prerequisite") {
                saw_stability_diagnostic = true;
            }
        }
        QVERIFY2(
            saw_zscore_diagnostic,
            "audit visible_diagnostics must include nonnormal Z-score formula_reference");
        QVERIFY2(
            saw_stability_diagnostic,
            "audit visible_diagnostics must include stability/pass-fail gate diagnostic");

        bool saw_gate_evidence = false;
        for (const auto& ref : document.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:capability_pass_fail_blocked")
                    != std::string::npos) {
                saw_gate_evidence = true;
                QCOMPARE(
                    ref.label_text_id,
                    std::string("evidence.capability_stability_prerequisite"));
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
            }
        }
        QVERIFY2(
            saw_gate_evidence,
            "audit evidence bundle must include nonnormal capability stability gate ref");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("nonnormal_cap_stability_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("nonnormal_cap_stability_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(bytes.size() > 800, "nonnormal capability audit PDF should contain content");
        QVERIFY2(
            bytes.contains("Nonnormal process capability"),
            "en-US PDF should render localized nonnormal capability page title");
        QVERIFY2(
            bytes.contains("Nonnormal capability computes Pp/Ppk")
                || bytes.contains("formula reference"),
            "en-US PDF should render localized nonnormal Z-score diagnostic");
        QVERIFY2(
            bytes.contains("Capability stability/normality prerequisite unmet")
                || bytes.contains("pass/fail blocked"),
            "audit PDF evidence appendix should render localized capability gate label");
        QVERIFY2(
            !bytes.contains("非正态过程能力"),
            "en-US nonnormal capability PDF must not leak Chinese page title");
        QVERIFY2(
            !bytes.contains("非正态能力使用拟合"),
            "en-US nonnormal capability PDF must not leak Chinese Z-score diagnostic");

        QVERIFY(QFileInfo::exists(exported.audit_json_path));
        QFile audit_file(exported.audit_json_path);
        QVERIFY(audit_file.open(QIODevice::ReadOnly));
        const QJsonArray evidence_refs =
            QJsonDocument::fromJson(audit_file.readAll())
                .object()
                .value(QStringLiteral("evidence"))
                .toObject()
                .value(QStringLiteral("evidence"))
                .toArray();
        bool saw_audit_gate = false;
        for (const QJsonValue& value : evidence_refs) {
            const QJsonObject ref = value.toObject();
            if (ref.value(QStringLiteral("label_text_id")).toString()
                    != QStringLiteral("evidence.capability_stability_prerequisite")) {
                continue;
            }
            saw_audit_gate = true;
            const std::string label =
                ref.value(QStringLiteral("label")).toString().toStdString();
            QVERIFY2(label.find("禁止合格判定") == std::string::npos, label.c_str());
        }
        QVERIFY2(
            saw_audit_gate,
            "audit JSON must serialize nonnormal capability stability gate with label_text_id");
    }

    void pdf_nonnormal_capability_cross_template_table_visibility_and_en_us_locale()
    {
        // Phase 6: non-normal Weibull capability gate × customer/engineer/audit PDF byte scan.
        DataTable table;
        table.columns = {"x"};
        for (int i = 1; i <= 30; ++i) {
            table.rows.push_back({std::to_string(0.5 * static_cast<double>(i) + 1.0)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "non_normal";
        configuration.nonnormal_distribution = "weibull";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 1.0;
        configuration.specifications.upper = 20.0;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            return document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QVERIFY(customer_doc.pages[0].visible_tables.empty());
        QVERIFY(!engineer_doc.pages[0].visible_tables.empty());
        QVERIFY(!audit_doc.pages[0].visible_tables.empty());
        QCOMPARE(
            customer_doc.provenance.facts_hash,
            engineer_doc.provenance.facts_hash);
        QCOMPARE(engineer_doc.provenance.facts_hash, audit_doc.provenance.facts_hash);

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("nonnormal_cap_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("nonnormal_cap_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("nonnormal_cap_audit_en.pdf"));

        QVERIFY2(
            customer_bytes.contains("Nonnormal process capability"),
            "customer PDF should still render localized page title");
        QVERIFY2(
            customer_bytes.contains("Nonnormal capability computes Pp/Ppk")
                || customer_bytes.contains("formula reference"),
            "customer PDF should retain nonnormal Z-score warning diagnostic");
        QVERIFY2(
            !customer_bytes.contains("Capability indices"),
            "customer PDF must omit nonnormal capability statistic table rows");

        for (const QByteArray& bytes : {engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("Nonnormal process capability"),
                "engineer/audit PDF should render localized page title");
            QVERIFY2(
                bytes.contains("Capability indices"),
                "engineer/audit PDF should render localized capability indices table");
            QVERIFY2(
                !bytes.contains("非正态过程能力"),
                "en-US nonnormal capability PDF must not leak Chinese page title");
        }

        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include full evidence appendix heading");
        QVERIFY2(
            audit_bytes.contains("Capability stability/normality prerequisite unmet")
                || audit_bytes.contains("pass/fail blocked"),
            "audit PDF evidence appendix should render localized capability gate label");
        QVERIFY2(
            !engineer_bytes.contains("Evidence appendix"),
            "engineer PDF should not include audit-only evidence appendix");
    }

    void pdf_box_cox_honesty_localizes_to_en_us_without_chinese_leak()
    {
        // Phase 6 CAP-NN-2: Box-Cox must not read as pass/fail; en-US PDF honesty bullets.
        DataTable table;
        table.columns = {"Y"};
        table.rows = {{"1.0"}, {"2.0"}, {"4.0"}, {"8.0"}, {"16.0"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "box_cox";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 0.5;
        configuration.specifications.upper = 20.0;
        OutputPage page = AnalysisService::box_cox(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.box_cox.has_value());
        QCOMPARE(
            QString::fromStdString(page.facts.box_cox->assumption_status),
            QStringLiteral("not_verified"));
        QVERIFY(std::any_of(
            page.tables.cbegin(), page.tables.cend(),
            [](const datalab::domain::StatisticTable& table_out) {
                return table_out.title == "变换后过程能力";
            }));

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        const auto localized = localize_report_document(document);
        document = localized.document;
        QVERIFY(!document.pages.empty());
        QCOMPARE(
            document.pages[0].source_page.facts.box_cox->assumption_status,
            std::string("not_verified"));

        bool saw_honesty_limit = false;
        for (const auto& section : document.pages[0].visible_interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("process pass/fail decision") != std::string::npos) {
                    saw_honesty_limit = true;
                }
                QVERIFY2(
                    bullet.find("过程合格判定") == std::string::npos,
                    "localized interpretation must not leak Chinese pass/fail wording");
            }
        }
        QVERIFY2(
            saw_honesty_limit,
            "Box-Cox limitation bullet must block pass/fail wording in en-US PDF");

        bool saw_capability_table = false;
        for (const auto& table_block : document.pages[0].visible_tables) {
            if (table_block.title == "Capability after transform") {
                saw_capability_table = true;
            }
            QVERIFY2(
                table_block.title.find("变换后") == std::string::npos,
                "visible_tables must not retain Chinese capability table title");
        }
        QVERIFY2(saw_capability_table, "engineer PDF must include localized capability table");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("box_cox_en.pdf"));
        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("box_cox_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("Box-Cox transformation"),
            "en-US PDF should render localized Box-Cox page title");
        QVERIFY2(
            bytes.contains("Capability after transform"),
            "en-US PDF should render localized post-transform capability table");
        QVERIFY2(
            bytes.contains("Transform parameters"),
            "en-US PDF should render localized transform parameter table");
        QVERIFY2(
            bytes.contains("do not claim the data are normal")
                || bytes.contains("diagnostic only"),
            "en-US PDF should render Box-Cox normality honesty bullet");
        QVERIFY2(
            bytes.contains("not a process pass/fail decision"),
            "en-US PDF should render Box-Cox capability honesty limitation");
        QVERIFY2(
            !bytes.contains("Box-Cox 变换"),
            "en-US Box-Cox PDF must not leak Chinese page title");
        QVERIFY2(
            !bytes.contains("变换后过程能力"),
            "en-US Box-Cox PDF must not leak Chinese capability table title");
        QVERIFY2(
            !bytes.contains("不能写成数据已正态"),
            "en-US Box-Cox PDF must not leak Chinese normality honesty bullet");
    }

    void pdf_box_cox_cross_template_table_visibility_and_en_us_locale()
    {
        // Phase 6 CAP-NN-2: Box-Cox customer hides statistic tables; engineer/audit keep them.
        DataTable table;
        table.columns = {"Y"};
        table.rows = {{"1.0"}, {"2.0"}, {"4.0"}, {"8.0"}, {"16.0"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "box_cox";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 0.5;
        configuration.specifications.upper = 20.0;
        OutputPage page = AnalysisService::box_cox(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.box_cox.has_value());

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            return document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QVERIFY(customer_doc.pages[0].visible_tables.empty());
        QVERIFY(!engineer_doc.pages[0].visible_tables.empty());
        QVERIFY(!audit_doc.pages[0].visible_tables.empty());
        QCOMPARE(
            customer_doc.provenance.facts_hash,
            engineer_doc.provenance.facts_hash);
        QCOMPARE(engineer_doc.provenance.facts_hash, audit_doc.provenance.facts_hash);

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("box_cox_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("box_cox_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("box_cox_audit_en.pdf"));

        QVERIFY2(
            customer_bytes.contains("Box-Cox transformation"),
            "customer PDF should still render localized page title");
        QVERIFY2(
            customer_bytes.contains("not a process pass/fail decision"),
            "customer PDF should retain Box-Cox pass/fail honesty limitation");
        QVERIFY2(
            !customer_bytes.contains("Capability after transform"),
            "customer PDF must omit post-transform capability statistic table");
        QVERIFY2(
            !customer_bytes.contains("Transform parameters"),
            "customer PDF must omit Box-Cox transform parameter table");

        for (const QByteArray& bytes : {engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("Box-Cox transformation"),
                "engineer/audit PDF should render localized page title");
            QVERIFY2(
                bytes.contains("Capability after transform"),
                "engineer/audit PDF should render localized capability table");
            QVERIFY2(
                bytes.contains("Transform parameters"),
                "engineer/audit PDF should render localized transform parameter table");
            QVERIFY2(
                !bytes.contains("Box-Cox 变换"),
                "en-US Box-Cox PDF must not leak Chinese page title");
        }

        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include full evidence appendix heading");
        QVERIFY2(
            !engineer_bytes.contains("Evidence appendix"),
            "engineer PDF should not include audit-only evidence appendix");

        bool audit_not_pass_fail_gate = false;
        for (const auto& ref : audit_doc.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:box_cox_not_pass_fail") != std::string::npos) {
                audit_not_pass_fail_gate = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.box_cox_not_pass_fail"));
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
            }
        }
        QVERIFY2(
            audit_not_pass_fail_gate,
            "audit evidence bundle must include Box-Cox not-pass/fail limiting gate ref");
        QVERIFY2(
            audit_bytes.contains("not a process pass/fail decision")
                || audit_bytes.contains("Box-Cox transformation is not"),
            "audit PDF should surface Box-Cox pass/fail honesty in body or evidence");
    }

    void pdf_box_cox_invalid_spec_limit_localizes_to_en_us_without_chinese_leak()
    {
        // Phase 6 CAP-NN-2: invalid LSL skips capability table; en-US diagnostic + PDF byte scan.
        DataTable table;
        table.columns = {"Y"};
        table.rows = {{"1.0"}, {"2.0"}, {"4.0"}, {"8.0"}, {"16.0"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "box_cox";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -1.0;
        configuration.specifications.upper = 10.0;
        OutputPage page = AnalysisService::box_cox(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.box_cox.has_value());

        bool saw_invalid_limit = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "box_cox_invalid_spec_limit") {
                saw_invalid_limit = true;
            }
        }
        QVERIFY2(saw_invalid_limit, "service must emit box_cox_invalid_spec_limit");
        QVERIFY(std::none_of(
            page.tables.cbegin(), page.tables.cend(),
            [](const datalab::domain::StatisticTable& table_out) {
                return table_out.title == "变换后过程能力";
            }));

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        document = localize_report_document(document).document;
        QVERIFY(!document.pages.empty());

        bool saw_localized_diag = false;
        for (const auto& diagnostic : document.pages[0].visible_diagnostics) {
            if (diagnostic.code == "box_cox_invalid_spec_limit") {
                saw_localized_diag = true;
                QVERIFY2(
                    diagnostic.message.find("Lower spec limit") != std::string::npos,
                    "en-US visible diagnostic must localize invalid LSL message");
                QVERIFY2(
                    diagnostic.message.find("规格") == std::string::npos,
                    "localized diagnostic must not leak Chinese");
            }
        }
        QVERIFY2(
            saw_localized_diag,
            "engineer visible_diagnostics must include localized invalid spec limit");

        bool saw_spec_limit_limitation = false;
        for (const auto& section : document.pages[0].visible_interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("post-transform capability table was skipped")
                        != std::string::npos
                    || bullet.find(
                           "Spec limits could not be transformed or are out of order")
                        != std::string::npos) {
                    saw_spec_limit_limitation = true;
                }
                QVERIFY2(
                    bullet.find("已跳过变换后过程能力表") == std::string::npos,
                    "localized interpretation must not leak Chinese spec-limit gate");
            }
        }
        QVERIFY2(
            saw_spec_limit_limitation,
            "en-US interpretation must surface spec-limit gate limitation bullet");

        for (const auto& table_block : document.pages[0].visible_tables) {
            QVERIFY2(
                table_block.title != "Capability after transform",
                "invalid spec limit must omit post-transform capability table");
        }

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("box_cox_invalid_spec_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("box_cox_invalid_spec_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("Lower spec limit cannot be transformed"),
            "en-US PDF should render localized invalid LSL diagnostic");
        QVERIFY2(
            !bytes.contains("规格下限"),
            "en-US Box-Cox PDF must not leak Chinese invalid LSL diagnostic");
        QVERIFY2(
            !bytes.contains("Capability after transform"),
            "en-US PDF must omit capability table when spec limit gate blocks it");
        QVERIFY2(
            bytes.contains("post-transform capability table was skipped")
                || bytes.contains("Spec limits could not be transformed"),
            "en-US PDF should render localized spec-limit gate limitation bullet");
        QVERIFY2(
            bytes.contains("not a process pass/fail decision")
                || bytes.contains("process pass/fail decision"),
            "en-US PDF should retain Box-Cox pass/fail honesty limitation");
    }

    void pdf_box_cox_invalid_spec_limit_cross_template_table_visibility_and_en_us_locale()
    {
        // Phase 6 CAP-NN-2: invalid LSL × customer/engineer/audit — no capability table; gate visible.
        DataTable table;
        table.columns = {"Y"};
        table.rows = {{"1.0"}, {"2.0"}, {"4.0"}, {"8.0"}, {"16.0"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "box_cox";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -1.0;
        configuration.specifications.upper = 10.0;
        OutputPage page = AnalysisService::box_cox(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.box_cox.has_value());
        QVERIFY(std::none_of(
            page.tables.cbegin(), page.tables.cend(),
            [](const datalab::domain::StatisticTable& table_out) {
                return table_out.title == "变换后过程能力";
            }));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QCOMPARE(
            customer_doc.provenance.facts_hash,
            engineer_doc.provenance.facts_hash);
        QCOMPARE(engineer_doc.provenance.facts_hash, audit_doc.provenance.facts_hash);

        const auto assert_no_capability_table = [](const ReportDocument& doc,
                                                 const char* label) {
            for (const auto& table_view : doc.pages[0].visible_tables) {
                QVERIFY2(
                    table_view.title.find("Capability after transform")
                        == std::string::npos,
                    label);
                QVERIFY2(
                    table_view.title.find("变换后过程能力") == std::string::npos,
                    label);
            }
        };
        assert_no_capability_table(customer_doc, "customer must omit capability table");
        assert_no_capability_table(engineer_doc, "engineer must omit capability table");
        assert_no_capability_table(audit_doc, "audit must omit capability table");

        const auto has_spec_limit_gate_bullet =
            [](const datalab::domain::ReportPageView& page_view) {
                for (const auto& section : page_view.visible_interpretation) {
                    for (const auto& bullet : section.bullets) {
                        if (bullet.find("post-transform capability table was skipped")
                                != std::string::npos) {
                            return true;
                        }
                    }
                }
                return false;
            };
        QVERIFY2(
            has_spec_limit_gate_bullet(customer_doc.pages[0]),
            "customer en-US should surface spec-limit gate limitation bullet");
        QVERIFY2(
            has_spec_limit_gate_bullet(engineer_doc.pages[0]),
            "engineer en-US should surface spec-limit gate limitation bullet");
        QVERIFY2(
            has_spec_limit_gate_bullet(audit_doc.pages[0]),
            "audit en-US should surface spec-limit gate limitation bullet");

        bool customer_gate_diag = false;
        for (const auto& diagnostic : customer_doc.pages[0].visible_diagnostics) {
            if (diagnostic.code == "box_cox_invalid_spec_limit") {
                customer_gate_diag = true;
                QVERIFY2(
                    diagnostic.message.find("Lower spec limit") != std::string::npos,
                    "customer gate diagnostic must localize");
            }
        }
        QVERIFY2(
            customer_gate_diag,
            "customer en-US should include risk-level invalid spec diagnostic");

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("box_cox_invalid_spec_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("box_cox_invalid_spec_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("box_cox_invalid_spec_audit_en.pdf"));

        for (const QByteArray& bytes :
             {customer_bytes, engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("Lower spec limit cannot be transformed"),
                "en-US PDF should render localized invalid LSL diagnostic");
            QVERIFY2(
                !bytes.contains("规格下限"),
                "en-US PDF must not leak Chinese invalid LSL diagnostic");
            QVERIFY2(
                !bytes.contains("Capability after transform"),
                "en-US PDF must omit capability table when spec gate blocks it");
            QVERIFY2(
                bytes.contains("post-transform capability table was skipped")
                    || bytes.contains("Spec limits could not be transformed"),
                "en-US PDF should render spec-limit gate limitation bullet");
        }
        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include evidence appendix");
        QVERIFY2(
            !customer_bytes.contains("Evidence appendix"),
            "customer PDF should not include audit-only evidence appendix");

        bool audit_spec_limit_gate = false;
        bool audit_not_pass_fail_gate = false;
        for (const auto& ref : audit_doc.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:box_cox_spec_limit") != std::string::npos) {
                audit_spec_limit_gate = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.box_cox_spec_limit_gate"));
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
            }
            if (ref.evidence_id.find(":gate:box_cox_not_pass_fail") != std::string::npos) {
                audit_not_pass_fail_gate = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.box_cox_not_pass_fail"));
            }
        }
        QVERIFY2(
            audit_spec_limit_gate,
            "audit evidence bundle must include Box-Cox spec-limit limiting gate ref");
        QVERIFY2(
            audit_not_pass_fail_gate,
            "audit evidence bundle must include Box-Cox not-pass/fail limiting gate ref");
        QVERIFY2(
            audit_bytes.contains("Box-Cox spec limit gate"),
            "audit PDF should render localized Box-Cox spec-limit gate label");
    }

    void pdf_box_cox_spec_limit_gate_localizes_and_audit_evidence_carries_label_text_id()
    {
        // Phase 6 CAP-NN-2: invalid spec limit → audit en-US PDF + audit JSON gate traceability.
        DataTable table;
        table.columns = {"Y"};
        table.rows = {{"1.0"}, {"2.0"}, {"4.0"}, {"8.0"}, {"16.0"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "box_cox";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -1.0;
        configuration.specifications.upper = 10.0;
        OutputPage page = AnalysisService::box_cox(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.box_cox.has_value());
        QCOMPARE(
            QString::fromStdString(page.facts.box_cox->assumption_status),
            QStringLiteral("not_verified"));

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        document = localize_report_document(document).document;
        QVERIFY(!document.pages.empty());

        bool saw_spec_gate = false;
        bool saw_not_pass_fail = false;
        for (const auto& ref : document.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:box_cox_spec_limit") != std::string::npos) {
                saw_spec_gate = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.box_cox_spec_limit_gate"));
                QCOMPARE(ref.diagnostic_code, std::string("box_cox_invalid_spec_limit"));
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
            }
            if (ref.evidence_id.find(":gate:box_cox_not_pass_fail") != std::string::npos) {
                saw_not_pass_fail = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.box_cox_not_pass_fail"));
                QCOMPARE(ref.diagnostic_code, std::string("box_cox_not_pass_fail"));
            }
        }
        QVERIFY2(saw_spec_gate, "audit evidence must include Box-Cox spec-limit gate ref");
        QVERIFY2(saw_not_pass_fail, "audit evidence must include Box-Cox not-pass/fail gate ref");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("box_cox_spec_gate_audit_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("box_cox_spec_gate_audit_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("Box-Cox transformation"),
            "en-US PDF should render localized Box-Cox page title");
        QVERIFY2(
            bytes.contains("Lower spec limit cannot be transformed"),
            "en-US PDF should render localized invalid LSL diagnostic");
        QVERIFY2(
            bytes.contains("Box-Cox spec limit gate"),
            "audit PDF evidence appendix should render localized spec-limit gate label");
        QVERIFY2(
            !bytes.contains("Box-Cox 变换"),
            "en-US Box-Cox gate PDF must not leak Chinese page title");

        QVERIFY(QFileInfo::exists(exported.audit_json_path));
        QFile audit_file(exported.audit_json_path);
        QVERIFY(audit_file.open(QIODevice::ReadOnly));
        const QJsonObject audit =
            QJsonDocument::fromJson(audit_file.readAll()).object();
        const QJsonArray evidence_refs =
            audit.value(QStringLiteral("evidence")).toObject()
                .value(QStringLiteral("evidence")).toArray();
        bool saw_audit_spec_gate = false;
        for (const QJsonValue& value : evidence_refs) {
            const QJsonObject ref = value.toObject();
            const std::string label_text_id =
                ref.value(QStringLiteral("label_text_id")).toString().toStdString();
            if (label_text_id != "evidence.box_cox_spec_limit_gate") {
                continue;
            }
            saw_audit_spec_gate = true;
            QCOMPARE(
                ref.value(QStringLiteral("diagnostic_code")).toString(),
                QStringLiteral("box_cox_invalid_spec_limit"));
            const std::string label =
                resolve_report_text(label_text_id, "en-US").text;
            QVERIFY2(
                label.find("Box-Cox spec limit gate") != std::string::npos,
                label.c_str());
            QVERIFY2(label.find("规格限门禁") == std::string::npos, label.c_str());
        }
        QVERIFY2(
            saw_audit_spec_gate,
            "audit JSON must serialize Box-Cox spec-limit gate with label_text_id");
    }

    void pdf_graph_builder_faceted_scatter_localizes_to_en_us_without_chinese_leak()
    {
        // Phase 7: real GraphService faceted scatter → en-US engineer PDF byte scan.
        DataTable table;
        table.columns = {"Time", "X", "Y", "Z", "Group"};
        table.rows = {
            {"1", "1", "2", "3", "A"},
            {"2", "2", "3", "4", "A"},
            {"3", "3", "1", "5", "B"},
            {"4", "4", "0", "6", "B"},
            {"5", "5", "2", "7", "B"}};
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "scatter";
        configuration.graph.x_column = 1;
        configuration.graph.y_column = 2;
        configuration.graph.facet_column = 4;
        configuration.graph.facet_max_panels = 4;
        const OutputPage page = GraphService::run(table, configuration);
        QCOMPARE(QString::fromStdString(page.title), QStringLiteral("散点图（分面）"));
        QVERIFY(page.facts.eda.has_value());
        QVERIFY(page.facts.eda->facet_enabled);
        QVERIFY(page.plots.size() >= 2);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        const auto localized = localize_report_document(document);
        document = localized.document;
        QVERIFY(!document.pages.empty());
        QCOMPARE(
            document.pages[0].source_page.title,
            std::string("Scatterplot (faceted)"));
        QVERIFY2(
            document.pages[0].source_page.parameter_summary.find("Facet =")
                != std::string::npos,
            "parameter_summary should localize 分面 = to Facet =");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("graph_facet_scatter_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("graph_facet_scatter_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("Scatterplot (faceted)"),
            "en-US PDF should render localized faceted scatter page title");
        QVERIFY2(
            bytes.contains("Facet ="),
            "en-US PDF should render localized facet parameter caption");
        QVERIFY2(
            !bytes.contains("散点图（分面）"),
            "en-US Graph Builder PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !bytes.contains("分面 ="),
            "en-US Graph Builder PDF must not leak Chinese facet caption");
    }

    void pdf_graph_scatter_faceted_cross_template_plot_visibility_and_en_us_locale()
    {
        // Phase 7: faceted scatter customer truncates plots + hidden/excluded honesty on engineer/audit.
        DataTable table;
        table.columns = {"Time", "X", "Y", "Group"};
        table.rows = {
            {"1", "1", "2", "A"},
            {"2", "2", "3", "A"},
            {"3", "3", "1", "B"},
            {"4", "4", "0", "B"}};
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "scatter";
        configuration.graph.x_column = 1;
        configuration.graph.y_column = 2;
        configuration.graph.facet_column = 3;
        configuration.graph.facet_max_panels = 2;
        configuration.hidden_rows = {1};
        OutputPage page = GraphService::run(table, configuration);
        QVERIFY(page.facts.eda.has_value());
        QVERIFY(page.facts.eda->facet_enabled);
        QVERIFY(page.plots.size() >= 2);
        QVERIFY(page.facts.eda->hidden_excluded_distinct);
        QCOMPARE(page.facts.eda->hidden_count, std::size_t{1});
        QVERIFY(page.facts.eda->display_eligible_n < page.facts.eda->analysis_eligible_n);

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            return document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QVERIFY(customer_doc.pages[0].visible_tables.empty());
        QCOMPARE(customer_doc.pages[0].visible_plots.size(), static_cast<std::size_t>(1));
        QVERIFY2(
            engineer_doc.pages[0].visible_plots.size() >= 2,
            "engineer should expose multiple faceted scatter panels");
        QCOMPARE(
            engineer_doc.pages[0].visible_plots.size(),
            audit_doc.pages[0].visible_plots.size());
        QVERIFY(!customer_doc.pages[0].show_parameter_summary);
        QVERIFY(engineer_doc.pages[0].show_parameter_summary);
        QCOMPARE(
            customer_doc.provenance.facts_hash,
            engineer_doc.provenance.facts_hash);
        QCOMPARE(engineer_doc.provenance.facts_hash, audit_doc.provenance.facts_hash);

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("graph_scatter_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("graph_scatter_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("graph_scatter_audit_en.pdf"));

        QVERIFY2(
            customer_bytes.contains("Scatterplot (faceted)"),
            "customer PDF should still render localized page title");
        QVERIFY2(
            !customer_bytes.contains("Facet ="),
            "customer PDF must omit engineer-only facet parameter summary");
        QVERIFY2(
            !customer_bytes.contains("Display N ="),
            "customer PDF must omit engineer-only display/analysis counts");

        for (const QByteArray& bytes : {engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("Scatterplot (faceted)"),
                "engineer/audit PDF should render localized page title");
            QVERIFY2(
                bytes.contains("Facet ="),
                "engineer/audit PDF should render localized facet parameter caption");
            QVERIFY2(
                bytes.contains("Display N ="),
                "engineer/audit PDF should render localized display N");
            QVERIFY2(
                bytes.contains("Analysis N ="),
                "engineer/audit PDF should render localized analysis N");
            QVERIFY2(
                bytes.contains("hidden = 1"),
                "engineer/audit PDF should render localized hidden count");
            QVERIFY2(
                !bytes.contains("散点图（分面）"),
                "en-US Graph scatter PDF must not leak Chinese page title");
        }

        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include full evidence appendix heading");
        QVERIFY2(
            !engineer_bytes.contains("Evidence appendix"),
            "engineer PDF should not include audit-only evidence appendix");
    }

    void pdf_graph_bar_faceted_cross_template_plot_visibility_and_en_us_locale()
    {
        // Phase 7: faceted bar customer truncates plots + hidden/excluded honesty on engineer/audit.
        DataTable table;
        table.columns = {"x", "cat", "facet"};
        table.rows = {
            {"1.0", "A", "P1"},
            {"2.0", "B", "P1"},
            {"3.0", "A", "P2"},
            {"4.0", "B", "P2"},
            {"5.0", "A", "P3"},
            {"6.0", "B", "P3"},
        };
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "bar";
        configuration.graph.x_column = 1;
        configuration.graph.facet_column = 2;
        configuration.graph.facet_max_panels = 2;
        configuration.hidden_rows = {1};
        OutputPage page = GraphService::run(table, configuration);
        QVERIFY(page.facts.eda.has_value());
        QVERIFY(page.facts.eda->facet_enabled);
        QVERIFY(page.plots.size() >= 2);
        QVERIFY(page.facts.eda->hidden_excluded_distinct);

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QCOMPARE(customer_doc.pages[0].visible_plots.size(), static_cast<std::size_t>(1));
        QVERIFY2(
            engineer_doc.pages[0].visible_plots.size() >= 2,
            "engineer should expose multiple faceted bar panels");
        QCOMPARE(
            engineer_doc.pages[0].visible_plots.size(),
            audit_doc.pages[0].visible_plots.size());
        QVERIFY(!customer_doc.pages[0].show_parameter_summary);
        QVERIFY(engineer_doc.pages[0].show_parameter_summary);

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("graph_bar_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("graph_bar_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("graph_bar_audit_en.pdf"));

        QVERIFY2(
            customer_bytes.contains("Bar chart (faceted)"),
            "customer PDF should still render localized page title");
        QVERIFY2(
            !customer_bytes.contains("Category ="),
            "customer PDF must omit engineer-only category parameter summary");

        for (const QByteArray& bytes : {engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("Bar chart (faceted)"),
                "engineer/audit PDF should render localized page title");
            QVERIFY2(
                bytes.contains("Category ="),
                "engineer/audit PDF should render localized category parameter caption");
            QVERIFY2(
                bytes.contains("Bar display counts omit hidden"),
                "engineer/audit PDF should render localized bar visibility honesty diagnostic");
            QVERIFY2(
                !bytes.contains("条形图（分面）"),
                "en-US bar PDF must not leak Chinese page title");
        }

        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include evidence appendix heading");
        QVERIFY2(
            !engineer_bytes.contains("Evidence appendix"),
            "engineer PDF should not include audit-only evidence appendix");
    }

    void pdf_graph_builder_faceted_bar_and_density_localize_to_en_us_without_chinese_leak()
    {
        // Phase 7: real GraphService faceted bar + density → en-US engineer PDF byte scan.
        DataTable table;
        table.columns = {"x", "cat", "facet"};
        table.rows = {
            {"1.0", "A", "P1"},
            {"2.0", "B", "P1"},
            {"3.0", "A", "P2"},
            {"4.0", "B", "P2"},
            {"5.0", "A", "P3"},
            {"6.0", "B", "P3"},
        };
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto export_localized_pdf = [&](const OutputPage& page,
                                        const QString& filename) -> QByteArray {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest = build_export_manifest(
                document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        AnalysisConfiguration bar_configuration;
        bar_configuration.graph.graph_kind = "bar";
        bar_configuration.graph.x_column = 1;
        bar_configuration.graph.facet_column = 2;
        bar_configuration.graph.facet_max_panels = 2;
        bar_configuration.hidden_rows = {1};
        const OutputPage bar_page = GraphService::run(table, bar_configuration);
        QCOMPARE(
            QString::fromStdString(bar_page.title),
            QStringLiteral("条形图（分面）"));
        QVERIFY(bar_page.facts.eda.has_value());
        QVERIFY(bar_page.facts.eda->facet_enabled);
        QVERIFY(bar_page.plots.size() >= 2);
        QVERIFY(bar_page.facts.eda->hidden_excluded_distinct);

        const QByteArray bar_bytes = export_localized_pdf(
            bar_page, QStringLiteral("graph_facet_bar_en.pdf"));
        QVERIFY2(
            bar_bytes.contains("Bar chart (faceted)"),
            "en-US PDF should render localized faceted bar page title");
        QVERIFY2(
            bar_bytes.contains("Facet ="),
            "en-US bar PDF should render localized facet parameter caption");
        QVERIFY2(
            bar_bytes.contains("Category ="),
            "en-US bar PDF should render localized category parameter caption");
        QVERIFY2(
            bar_bytes.contains("Bar display counts omit hidden"),
            "en-US bar PDF should render localized hidden/excluded honesty diagnostic");
        QVERIFY2(
            !bar_bytes.contains("条形图（分面）"),
            "en-US bar PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !bar_bytes.contains("分面 ="),
            "en-US bar PDF must not leak Chinese facet caption");
        QVERIFY2(
            !bar_bytes.contains("类别 ="),
            "en-US bar PDF must not leak Chinese category caption");
        QVERIFY2(
            !bar_bytes.contains("条形图显示计数省略 hidden"),
            "en-US bar PDF must not leak Chinese bar visibility diagnostic");

        AnalysisConfiguration density_configuration;
        density_configuration.graph.graph_kind = "density";
        density_configuration.graph.x_column = 0;
        density_configuration.graph.facet_column = 2;
        density_configuration.graph.facet_max_panels = 2;
        const OutputPage density_page = GraphService::run(table, density_configuration);
        QCOMPARE(
            QString::fromStdString(density_page.title),
            QStringLiteral("密度图（分面）"));
        QVERIFY(density_page.facts.eda.has_value());
        QVERIFY(density_page.facts.eda->facet_enabled);
        QVERIFY(density_page.plots.size() >= 2);
        bool saw_density_mark_note = false;
        for (const auto& diagnostic : density_page.diagnostics) {
            if (diagnostic.code == "density_curve_not_discrete_marks") {
                saw_density_mark_note = true;
            }
        }
        QVERIFY(saw_density_mark_note);

        const QByteArray density_bytes = export_localized_pdf(
            density_page, QStringLiteral("graph_facet_density_en.pdf"));
        QVERIFY2(
            density_bytes.contains("Density plot (faceted)"),
            "en-US PDF should render localized faceted density page title");
        QVERIFY2(
            density_bytes.contains("Facet ="),
            "en-US density PDF should render localized facet parameter caption");
        QVERIFY2(
            density_bytes.contains("Variable ="),
            "en-US density PDF should render localized variable parameter caption");
        QVERIFY2(
            density_bytes.contains("Density curves are continuous KDE grids"),
            "en-US density PDF should render localized density mark honesty diagnostic");
        QVERIFY2(
            !density_bytes.contains("密度图（分面）"),
            "en-US density PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !density_bytes.contains("分面 ="),
            "en-US density PDF must not leak Chinese facet caption");
        QVERIFY2(
            !density_bytes.contains("变量 ="),
            "en-US density PDF must not leak Chinese variable caption");
    }

    void pdf_graph_builder_faceted_interval_and_violin_localize_to_en_us_without_chinese_leak()
    {
        // Phase 7: real GraphService faceted interval + violin → en-US engineer PDF byte scan.
        DataTable table;
        table.columns = {"y", "cat", "facet", "x"};
        table.rows = {
            {"10", "A", "P1", "1"},
            {"12", "B", "P1", "2"},
            {"14", "A", "P2", "3"},
            {"16", "B", "P2", "4"},
            {"18", "A", "P3", "5"},
            {"20", "B", "P3", "6"},
            {"22", "A", "P4", "7"},
        };
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto export_localized_pdf = [&](const OutputPage& page,
                                        const QString& filename) -> QByteArray {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest = build_export_manifest(
                document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        AnalysisConfiguration interval_configuration;
        interval_configuration.graph.graph_kind = "interval";
        interval_configuration.graph.y_column = 0;
        interval_configuration.graph.by_column = 1;
        interval_configuration.graph.facet_column = 2;
        interval_configuration.graph.facet_max_panels = 2;
        interval_configuration.excluded_rows = {6};
        interval_configuration.hidden_rows = {1};
        const OutputPage interval_page = GraphService::run(table, interval_configuration);
        QCOMPARE(
            QString::fromStdString(interval_page.title),
            QStringLiteral("区间散点图（分面）"));
        QVERIFY(interval_page.facts.eda.has_value());
        QVERIFY(interval_page.facts.eda->facet_enabled);
        QVERIFY(interval_page.plots.size() >= 2);
        QCOMPARE(interval_page.facts.eda->facet_truncated_levels, std::size_t{1});

        const QByteArray interval_bytes = export_localized_pdf(
            interval_page, QStringLiteral("graph_facet_interval_en.pdf"));
        QVERIFY2(
            interval_bytes.contains("Interval scatterplot (faceted)"),
            "en-US PDF should render localized faceted interval page title");
        QVERIFY2(
            interval_bytes.contains("Facet ="),
            "en-US interval PDF should render localized facet parameter caption");
        QVERIFY2(
            interval_bytes.contains("Response ="),
            "en-US interval PDF should render localized response parameter caption");
        QVERIFY2(
            interval_bytes.contains("Group ="),
            "en-US interval PDF should render localized group parameter caption");
        QVERIFY2(
            interval_bytes.contains("controlled Graph Builder shows at most")
                || interval_bytes.contains("controlled facet panels"),
            "en-US interval PDF should render localized facet controlled/truncation diagnostic");
        QVERIFY2(
            !interval_bytes.contains("区间散点图（分面）"),
            "en-US interval PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !interval_bytes.contains("分面 ="),
            "en-US interval PDF must not leak Chinese facet caption");
        QVERIFY2(
            !interval_bytes.contains("响应 ="),
            "en-US interval PDF must not leak Chinese response caption");
        QVERIFY2(
            !interval_bytes.contains("分组 ="),
            "en-US interval PDF must not leak Chinese group caption");

        AnalysisConfiguration violin_configuration;
        violin_configuration.graph.graph_kind = "violin";
        violin_configuration.graph.y_column = 0;
        violin_configuration.graph.by_column = 1;
        violin_configuration.graph.facet_column = 2;
        violin_configuration.graph.facet_max_panels = 2;
        violin_configuration.excluded_rows = {6};
        violin_configuration.hidden_rows = {1};
        const OutputPage violin_page = GraphService::run(table, violin_configuration);
        QCOMPARE(
            QString::fromStdString(violin_page.title),
            QStringLiteral("小提琴图（分面）"));
        QVERIFY(violin_page.facts.eda.has_value());
        QVERIFY(violin_page.facts.eda->facet_enabled);
        QVERIFY(violin_page.plots.size() >= 2);

        const QByteArray violin_bytes = export_localized_pdf(
            violin_page, QStringLiteral("graph_facet_violin_en.pdf"));
        QVERIFY2(
            violin_bytes.contains("Violin plot (faceted)"),
            "en-US PDF should render localized faceted violin page title");
        QVERIFY2(
            violin_bytes.contains("Facet ="),
            "en-US violin PDF should render localized facet parameter caption");
        QVERIFY2(
            violin_bytes.contains("Response ="),
            "en-US violin PDF should render localized response parameter caption");
        QVERIFY2(
            violin_bytes.contains("Group ="),
            "en-US violin PDF should render localized group parameter caption");
        QVERIFY2(
            violin_bytes.contains("controlled Graph Builder shows at most")
                || violin_bytes.contains("controlled facet panels"),
            "en-US violin PDF should render localized facet controlled/truncation diagnostic");
        QVERIFY2(
            !violin_bytes.contains("小提琴图（分面）"),
            "en-US violin PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !violin_bytes.contains("分面 ="),
            "en-US violin PDF must not leak Chinese facet caption");
        QVERIFY2(
            !violin_bytes.contains("响应 ="),
            "en-US violin PDF must not leak Chinese response caption");
        QVERIFY2(
            !violin_bytes.contains("分组 ="),
            "en-US violin PDF must not leak Chinese group caption");
    }

    void pdf_graph_builder_faceted_hexbin_localizes_to_en_us_without_chinese_leak()
    {
        // Phase 7: real GraphService faceted hexbin → en-US engineer PDF byte scan.
        DataTable table;
        table.columns = {"y", "cat", "facet", "x"};
        table.rows = {
            {"10", "A", "P1", "1"},
            {"12", "B", "P1", "2"},
            {"14", "A", "P2", "3"},
            {"16", "B", "P2", "4"},
            {"18", "A", "P3", "5"},
            {"20", "B", "P3", "6"},
            {"22", "A", "P4", "7"},
        };
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "hexbin";
        configuration.graph.x_column = 3;
        configuration.graph.y_column = 0;
        configuration.graph.facet_column = 2;
        configuration.graph.facet_max_panels = 2;
        configuration.excluded_rows = {6};
        configuration.hidden_rows = {1};
        const OutputPage page = GraphService::run(table, configuration);
        QCOMPARE(
            QString::fromStdString(page.title),
            QStringLiteral("Hexbin（分面）"));
        QVERIFY(page.facts.eda.has_value());
        QVERIFY(page.facts.eda->facet_enabled);
        QVERIFY(page.plots.size() >= 2);
        QCOMPARE(page.facts.eda->facet_truncated_levels, std::size_t{1});
        bool saw_rect_bins = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "hexbin_rectangular_bins") {
                saw_rect_bins = true;
            }
        }
        QVERIFY(saw_rect_bins);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        const auto localized = localize_report_document(document);
        document = localized.document;
        QVERIFY(!document.pages.empty());
        QCOMPARE(
            document.pages[0].source_page.title,
            std::string("Hexbin (faceted)"));
        QVERIFY2(
            document.pages[0].source_page.parameter_summary.find("Facet =")
                != std::string::npos,
            "parameter_summary should localize 分面 = to Facet =");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("graph_facet_hexbin_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("graph_facet_hexbin_en.pdf"));
        manifest.consistency_status = "ok";
        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("Hexbin (faceted)"),
            "en-US PDF should render localized faceted hexbin page title");
        QVERIFY2(
            bytes.contains("Facet ="),
            "en-US hexbin PDF should render localized facet parameter caption");
        QVERIFY2(
            bytes.contains("X = x") || bytes.contains("X ="),
            "en-US hexbin PDF should render X parameter caption");
        QVERIFY2(
            bytes.contains("Y = y") || bytes.contains("Y ="),
            "en-US hexbin PDF should render Y parameter caption");
        QVERIFY2(
            bytes.contains("Using rectangular 2D bins"),
            "en-US hexbin PDF should render localized rectangular-bin honesty diagnostic");
        QVERIFY2(
            bytes.contains("controlled Graph Builder shows at most")
                || bytes.contains("controlled facet panels"),
            "en-US hexbin PDF should render localized facet controlled/truncation diagnostic");
        QVERIFY2(
            !bytes.contains("Hexbin（分面）"),
            "en-US hexbin PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !bytes.contains("分面 ="),
            "en-US hexbin PDF must not leak Chinese facet caption");
        QVERIFY2(
            !bytes.contains("使用矩形二维分箱"),
            "en-US hexbin PDF must not leak Chinese rectangular-bin diagnostic");
    }

    void pdf_hexbin_rectangular_bins_gate_localizes_and_audit_evidence_carries_label_text_id()
    {
        // Phase 7: hexbin rectangular-bin honesty → en-US audit PDF + audit JSON traceability.
        DataTable table;
        table.columns = {"y", "cat", "facet", "x"};
        table.rows = {
            {"10", "A", "P1", "1"},
            {"12", "B", "P1", "2"},
            {"14", "A", "P2", "3"},
            {"16", "B", "P2", "4"},
            {"18", "A", "P3", "5"},
            {"20", "B", "P3", "6"},
            {"22", "A", "P4", "7"},
        };
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "hexbin";
        configuration.graph.x_column = 3;
        configuration.graph.y_column = 0;
        configuration.graph.facet_column = 2;
        configuration.graph.facet_max_panels = 2;
        configuration.excluded_rows = {6};
        configuration.hidden_rows = {1};
        OutputPage page = GraphService::run(table, configuration);
        QVERIFY(page.facts.eda.has_value());
        bool saw_rect_bins = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "hexbin_rectangular_bins") {
                saw_rect_bins = true;
            }
        }
        QVERIFY2(saw_rect_bins, "GraphService hexbin must emit rectangular-bin diagnostic");

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        document = localize_report_document(document).document;
        QVERIFY(!document.pages.empty());
        QCOMPARE(
            document.pages[0].source_page.title,
            std::string("Hexbin (faceted)"));

        bool saw_gate_evidence = false;
        for (const auto& ref : document.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:hexbin_rectangular_bins") != std::string::npos) {
                saw_gate_evidence = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.hexbin_rectangular_bins"));
                QCOMPARE(ref.diagnostic_code, std::string("hexbin_rectangular_bins"));
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
            }
        }
        QVERIFY2(
            saw_gate_evidence,
            "audit evidence bundle must include hexbin rectangular-bin limiting gate ref");

        bool saw_localized_diag = false;
        for (const auto& diagnostic : document.pages[0].visible_diagnostics) {
            if (diagnostic.code == "hexbin_rectangular_bins") {
                saw_localized_diag = true;
                QVERIFY2(
                    diagnostic.message.find("rectangular 2D bins") != std::string::npos,
                    "localized hexbin diagnostic must be en-US");
                QVERIFY2(
                    diagnostic.message.find("矩形") == std::string::npos,
                    "hexbin diagnostic must not leak Chinese after localization");
            }
        }
        QVERIFY2(
            saw_localized_diag,
            "audit visible_diagnostics must include localized hexbin honesty diagnostic");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("hexbin_gate_en.pdf"));
        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("hexbin_gate_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("Hexbin (faceted)"),
            "en-US PDF should render localized hexbin page title");
        QVERIFY2(
            bytes.contains("Using rectangular 2D bins"),
            "en-US PDF should render localized rectangular-bin honesty diagnostic");
        QVERIFY2(
            bytes.contains("rectangular 2D bins"),
            "audit PDF evidence appendix should render localized hexbin gate label");
        QVERIFY2(
            !bytes.contains("Hexbin（分面）"),
            "en-US hexbin gate PDF must not leak Chinese page title");

        QVERIFY(QFileInfo::exists(exported.audit_json_path));
        QFile audit_file(exported.audit_json_path);
        QVERIFY(audit_file.open(QIODevice::ReadOnly));
        const QJsonObject audit =
            QJsonDocument::fromJson(audit_file.readAll()).object();
        const QJsonArray evidence_refs =
            audit.value(QStringLiteral("evidence")).toObject()
                .value(QStringLiteral("evidence")).toArray();
        bool saw_audit_gate = false;
        for (const QJsonValue& value : evidence_refs) {
            const QJsonObject ref = value.toObject();
            const std::string label_text_id =
                ref.value(QStringLiteral("label_text_id")).toString().toStdString();
            if (label_text_id != "evidence.hexbin_rectangular_bins") {
                continue;
            }
            saw_audit_gate = true;
            QCOMPARE(
                ref.value(QStringLiteral("diagnostic_code")).toString(),
                QStringLiteral("hexbin_rectangular_bins"));
            const std::string label =
                resolve_report_text(label_text_id, "en-US").text;
            QVERIFY2(
                label.find("rectangular 2D bins") != std::string::npos,
                label.c_str());
            QVERIFY2(label.find("矩形") == std::string::npos, label.c_str());
        }
        QVERIFY2(
            saw_audit_gate,
            "audit JSON must serialize hexbin gate evidence with label_text_id");
    }

    void pdf_graph_hexbin_faceted_cross_template_plot_visibility_and_en_us_locale()
    {
        // Phase 7: faceted hexbin customer truncates plots + hides parameter summary; audit gate.
        DataTable table;
        table.columns = {"y", "cat", "facet", "x"};
        table.rows = {
            {"10", "A", "P1", "1"},
            {"12", "B", "P1", "2"},
            {"14", "A", "P2", "3"},
            {"16", "B", "P2", "4"},
            {"18", "A", "P3", "5"},
            {"20", "B", "P3", "6"},
            {"22", "A", "P4", "7"},
        };
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "hexbin";
        configuration.graph.x_column = 3;
        configuration.graph.y_column = 0;
        configuration.graph.facet_column = 2;
        configuration.graph.facet_max_panels = 2;
        configuration.excluded_rows = {6};
        configuration.hidden_rows = {1};
        OutputPage page = GraphService::run(table, configuration);
        QVERIFY(page.facts.eda.has_value());
        QVERIFY(page.plots.size() >= 2);
        QVERIFY(page.facts.eda->hidden_excluded_distinct);
        QCOMPARE(page.facts.eda->hidden_count, std::size_t{1});
        QCOMPARE(page.facts.eda->excluded_count, std::size_t{1});

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QVERIFY(customer_doc.pages[0].visible_tables.empty());
        QCOMPARE(customer_doc.pages[0].visible_plots.size(), static_cast<std::size_t>(1));
        QVERIFY2(
            engineer_doc.pages[0].visible_plots.size() >= 2,
            "engineer should expose multiple faceted hexbin panels");
        QCOMPARE(
            engineer_doc.pages[0].visible_plots.size(),
            audit_doc.pages[0].visible_plots.size());
        QVERIFY(!customer_doc.pages[0].show_parameter_summary);
        QVERIFY(engineer_doc.pages[0].show_parameter_summary);
        QCOMPARE(
            customer_doc.provenance.facts_hash,
            engineer_doc.provenance.facts_hash);
        QCOMPARE(engineer_doc.provenance.facts_hash, audit_doc.provenance.facts_hash);

        const auto has_hexbin_gate = [](const datalab::domain::ReportPageView& page) {
            for (const auto& ref : page.visible_evidence) {
                if (ref.evidence_id.find(":gate:hexbin_rectangular_bins") != std::string::npos) {
                    return true;
                }
            }
            return false;
        };
        QVERIFY2(
            has_hexbin_gate(customer_doc.pages[0]),
            "customer en-US keeps hexbin rectangular-bin limiting gate ref");
        QVERIFY2(
            has_hexbin_gate(audit_doc.pages[0]),
            "audit en-US keeps hexbin rectangular-bin limiting gate ref");

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("graph_hexbin_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("graph_hexbin_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("graph_hexbin_audit_en.pdf"));

        QVERIFY2(
            customer_bytes.contains("Hexbin (faceted)"),
            "customer PDF should still render localized page title");
        QVERIFY2(
            !customer_bytes.contains("Facet ="),
            "customer PDF must omit engineer-only facet parameter summary");

        for (const QByteArray& bytes : {engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("Hexbin (faceted)"),
                "engineer/audit PDF should render localized page title");
            QVERIFY2(
                bytes.contains("Facet ="),
                "engineer/audit PDF should render localized facet parameter caption");
            QVERIFY2(
                bytes.contains("Using rectangular 2D bins"),
                "engineer/audit PDF should render localized rectangular-bin diagnostic");
            QVERIFY2(
                bytes.contains("Display N ="),
                "engineer/audit PDF should render localized display N");
            QVERIFY2(
                bytes.contains("Analysis N ="),
                "engineer/audit PDF should render localized analysis N");
            QVERIFY2(
                bytes.contains("hidden = 1"),
                "engineer/audit PDF should render localized hidden count");
            QVERIFY2(
                bytes.contains("excluded = 1"),
                "engineer/audit PDF should render localized excluded count");
            QVERIFY2(
                !bytes.contains("Hexbin（分面）"),
                "en-US hexbin PDF must not leak Chinese page title");
        }

        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include full evidence appendix heading");
        QVERIFY2(
            audit_bytes.contains("rectangular 2D bins"),
            "audit PDF evidence appendix should render localized hexbin gate label");
        QVERIFY2(
            !engineer_bytes.contains("Evidence appendix"),
            "engineer PDF should not include audit-only evidence appendix");
    }

    void pdf_density_curve_not_discrete_marks_gate_localizes_and_audit_evidence_carries_label_text_id()
    {
        // Phase 7: density KDE honesty → en-US audit PDF + audit JSON traceability.
        DataTable table;
        table.columns = {"x", "cat", "facet"};
        table.rows = {
            {"1.0", "A", "P1"},
            {"2.0", "B", "P1"},
            {"3.0", "A", "P2"},
            {"4.0", "B", "P2"},
            {"5.0", "A", "P3"},
            {"6.0", "B", "P3"},
        };
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "density";
        configuration.graph.x_column = 0;
        configuration.graph.facet_column = 2;
        configuration.graph.facet_max_panels = 2;
        OutputPage page = GraphService::run(table, configuration);
        QVERIFY(page.facts.eda.has_value());
        bool saw_density_note = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "density_curve_not_discrete_marks") {
                saw_density_note = true;
            }
        }
        QVERIFY2(saw_density_note, "GraphService density must emit curve-mark honesty diagnostic");

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        document = localize_report_document(document).document;
        QVERIFY(!document.pages.empty());
        QCOMPARE(
            document.pages[0].source_page.title,
            std::string("Density plot (faceted)"));

        bool saw_gate_evidence = false;
        for (const auto& ref : document.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:density_curve_not_discrete_marks")
                    != std::string::npos) {
                saw_gate_evidence = true;
                QCOMPARE(
                    ref.label_text_id,
                    std::string("evidence.density_curve_not_discrete_marks"));
                QCOMPARE(ref.diagnostic_code, std::string("density_curve_not_discrete_marks"));
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
            }
        }
        QVERIFY2(
            saw_gate_evidence,
            "audit evidence bundle must include density curve honesty limiting gate ref");

        bool saw_localized_diag = false;
        for (const auto& diagnostic : document.pages[0].visible_diagnostics) {
            if (diagnostic.code == "density_curve_not_discrete_marks") {
                saw_localized_diag = true;
                QVERIFY2(
                    diagnostic.message.find("continuous KDE grids") != std::string::npos,
                    "localized density diagnostic must be en-US");
                QVERIFY2(
                    diagnostic.message.find("密度曲线") == std::string::npos,
                    "density diagnostic must not leak Chinese after localization");
            }
        }
        QVERIFY2(
            saw_localized_diag,
            "audit visible_diagnostics must include localized density honesty diagnostic");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("density_gate_en.pdf"));
        ReportExportManifest manifest =
            build_export_manifest(document, make_report_export_paths("density_gate_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.contains("Density plot (faceted)"),
            "en-US PDF should render localized density page title");
        QVERIFY2(
            bytes.contains("Density curves are continuous KDE grids"),
            "en-US PDF should render localized density mark honesty diagnostic");
        QVERIFY2(
            bytes.contains("not discrete row marks"),
            "audit PDF evidence appendix should render localized density gate label");
        QVERIFY2(
            !bytes.contains("密度图（分面）"),
            "en-US density gate PDF must not leak Chinese page title");

        QVERIFY(QFileInfo::exists(exported.audit_json_path));
        QFile audit_file(exported.audit_json_path);
        QVERIFY(audit_file.open(QIODevice::ReadOnly));
        const QJsonObject audit =
            QJsonDocument::fromJson(audit_file.readAll()).object();
        const QJsonArray evidence_refs =
            audit.value(QStringLiteral("evidence")).toObject()
                .value(QStringLiteral("evidence")).toArray();
        bool saw_audit_gate = false;
        for (const QJsonValue& value : evidence_refs) {
            const QJsonObject ref = value.toObject();
            const std::string label_text_id =
                ref.value(QStringLiteral("label_text_id")).toString().toStdString();
            if (label_text_id != "evidence.density_curve_not_discrete_marks") {
                continue;
            }
            saw_audit_gate = true;
            QCOMPARE(
                ref.value(QStringLiteral("diagnostic_code")).toString(),
                QStringLiteral("density_curve_not_discrete_marks"));
            const std::string label =
                resolve_report_text(label_text_id, "en-US").text;
            QVERIFY2(
                label.find("not discrete row marks") != std::string::npos,
                label.c_str());
            QVERIFY2(label.find("密度") == std::string::npos, label.c_str());
        }
        QVERIFY2(
            saw_audit_gate,
            "audit JSON must serialize density gate evidence with label_text_id");
    }

    void pdf_graph_density_faceted_cross_template_plot_visibility_and_en_us_locale()
    {
        // Phase 7: faceted density customer truncates plots; hidden/excluded honesty + audit density gate.
        DataTable table;
        table.columns = {"x", "cat", "facet"};
        table.rows = {
            {"1.0", "A", "P1"},
            {"2.0", "B", "P1"},
            {"3.0", "A", "P2"},
            {"4.0", "B", "P2"},
            {"5.0", "A", "P3"},
            {"6.0", "B", "P3"},
        };
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "density";
        configuration.graph.x_column = 0;
        configuration.graph.facet_column = 2;
        configuration.graph.facet_max_panels = 2;
        configuration.hidden_rows = {1};
        OutputPage page = GraphService::run(table, configuration);
        QVERIFY(page.facts.eda.has_value());
        QVERIFY(page.plots.size() >= 2);
        QVERIFY(page.facts.eda->hidden_excluded_distinct);
        QCOMPARE(page.facts.eda->hidden_count, std::size_t{1});

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QCOMPARE(customer_doc.pages[0].visible_plots.size(), static_cast<std::size_t>(1));
        QVERIFY2(
            engineer_doc.pages[0].visible_plots.size() >= 2,
            "engineer should expose multiple faceted density panels");
        QCOMPARE(
            engineer_doc.pages[0].visible_plots.size(),
            audit_doc.pages[0].visible_plots.size());
        QVERIFY(!customer_doc.pages[0].show_parameter_summary);
        QVERIFY(engineer_doc.pages[0].show_parameter_summary);

        const auto has_density_gate = [](const datalab::domain::ReportPageView& page) {
            for (const auto& ref : page.visible_evidence) {
                if (ref.evidence_id.find(":gate:density_curve_not_discrete_marks")
                        != std::string::npos) {
                    return true;
                }
            }
            return false;
        };
        QVERIFY2(
            has_density_gate(customer_doc.pages[0]),
            "customer en-US keeps density curve honesty limiting gate ref");
        QVERIFY2(
            has_density_gate(audit_doc.pages[0]),
            "audit en-US keeps density curve honesty limiting gate ref");

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("graph_density_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("graph_density_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("graph_density_audit_en.pdf"));

        QVERIFY2(
            customer_bytes.contains("Density plot (faceted)"),
            "customer PDF should still render localized page title");
        QVERIFY2(
            !customer_bytes.contains("Facet ="),
            "customer PDF must omit engineer-only facet parameter summary");

        for (const QByteArray& bytes : {engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("Density curves are continuous KDE grids"),
                "engineer/audit PDF should render localized density honesty diagnostic");
            QVERIFY2(
                bytes.contains("Display N ="),
                "engineer/audit PDF should render localized display N");
            QVERIFY2(
                bytes.contains("Analysis N ="),
                "engineer/audit PDF should render localized analysis N");
            QVERIFY2(
                bytes.contains("hidden = 1"),
                "engineer/audit PDF should render localized hidden count");
            QVERIFY2(
                !bytes.contains("密度图（分面）"),
                "en-US density PDF must not leak Chinese page title");
        }

        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include evidence appendix heading");
        QVERIFY2(
            audit_bytes.contains("not discrete row marks"),
            "audit PDF evidence appendix should render localized density gate label");
        QVERIFY2(
            !engineer_bytes.contains("Evidence appendix"),
            "engineer PDF should not include audit-only evidence appendix");
    }

    void pdf_graph_builder_faceted_contour_and_matrix_localize_to_en_us_without_chinese_leak()
    {
        // Phase 7: real GraphService faceted contour + matrix → en-US engineer PDF byte scan.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto export_localized_pdf = [&](const DataTable& table,
                                        const OutputPage& page,
                                        const QString& filename) -> QByteArray {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest = build_export_manifest(
                document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        DataTable contour_table;
        contour_table.columns = {"x", "y", "z", "facet"};
        contour_table.rows = {
            {"0", "0", "1", "P1"},
            {"1", "0", "2", "P1"},
            {"0", "1", "3", "P1"},
            {"1", "1", "4", "P1"},
            {"0", "0", "2", "P2"},
            {"1", "0", "3", "P2"},
            {"0", "1", "4", "P2"},
            {"1", "1", "5", "P2"},
            {"0", "0", "3", "P3"},
            {"1", "0", "4", "P3"},
            {"0", "1", "5", "P3"},
            {"1", "1", "6", "P3"},
            {"0", "0", "9", "P4"},
        };
        AnalysisConfiguration contour_configuration;
        contour_configuration.graph.graph_kind = "contour";
        contour_configuration.graph.x_column = 0;
        contour_configuration.graph.y_column = 1;
        contour_configuration.graph.z_column = 2;
        contour_configuration.graph.facet_column = 3;
        contour_configuration.graph.facet_max_panels = 2;
        contour_configuration.graph.contour_levels = 4;
        contour_configuration.excluded_rows = {12};
        contour_configuration.hidden_rows = {1};
        const OutputPage contour_page =
            GraphService::run(contour_table, contour_configuration);
        QCOMPARE(
            QString::fromStdString(contour_page.title),
            QStringLiteral("等值线图（分面）"));
        QVERIFY(contour_page.facts.eda.has_value());
        QVERIFY(contour_page.facts.eda->facet_enabled);
        QVERIFY(contour_page.plots.size() >= 2);
        QCOMPARE(contour_page.facts.eda->facet_truncated_levels, std::size_t{1});

        const QByteArray contour_bytes = export_localized_pdf(
            contour_table,
            contour_page,
            QStringLiteral("graph_facet_contour_en.pdf"));
        QVERIFY2(
            contour_bytes.contains("Contour plot (faceted)"),
            "en-US PDF should render localized faceted contour page title");
        QVERIFY2(
            contour_bytes.contains("Facet ="),
            "en-US contour PDF should render localized facet parameter caption");
        QVERIFY2(
            contour_bytes.contains("X =") && contour_bytes.contains("Z ="),
            "en-US contour PDF should render localized X/Z parameter captions");
        QVERIFY2(
            contour_bytes.contains("controlled Graph Builder shows at most")
                || contour_bytes.contains("controlled facet panels"),
            "en-US contour PDF should render localized facet controlled/truncation diagnostic");
        QVERIFY2(
            !contour_bytes.contains("等值线图（分面）"),
            "en-US contour PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !contour_bytes.contains("分面 ="),
            "en-US contour PDF must not leak Chinese facet caption");

        DataTable matrix_table;
        matrix_table.columns = {"a", "b", "t", "y", "facet"};
        matrix_table.rows = {
            {"1", "10", "1", "5", "P1"},
            {"2", "20", "2", "6", "P1"},
            {"3", "30", "3", "7", "P2"},
            {"4", "40", "4", "8", "P2"},
            {"5", "50", "5", "9", "P3"},
            {"6", "60", "6", "10", "P3"},
            {"7", "70", "7", "11", "P4"},
        };
        AnalysisConfiguration matrix_configuration;
        matrix_configuration.graph.graph_kind = "matrix";
        matrix_configuration.graph.variable_columns = {0, 1};
        matrix_configuration.graph.facet_column = 4;
        matrix_configuration.graph.facet_max_panels = 2;
        matrix_configuration.excluded_rows = {6};
        matrix_configuration.hidden_rows = {1};
        const OutputPage matrix_page =
            GraphService::run(matrix_table, matrix_configuration);
        QCOMPARE(
            QString::fromStdString(matrix_page.title),
            QStringLiteral("矩阵图（分面）"));
        QVERIFY(matrix_page.facts.eda.has_value());
        QVERIFY(matrix_page.facts.eda->facet_enabled);
        QVERIFY(matrix_page.plots.size() >= 2);
        QCOMPARE(matrix_page.facts.eda->facet_truncated_levels, std::size_t{1});

        const QByteArray matrix_bytes = export_localized_pdf(
            matrix_table,
            matrix_page,
            QStringLiteral("graph_facet_matrix_en.pdf"));
        QVERIFY2(
            matrix_bytes.contains("Matrix plot (faceted)"),
            "en-US PDF should render localized faceted matrix page title");
        QVERIFY2(
            matrix_bytes.contains("Facet ="),
            "en-US matrix PDF should render localized facet parameter caption");
        QVERIFY2(
            matrix_bytes.contains("Variable count ="),
            "en-US matrix PDF should render localized variable-count parameter caption");
        QVERIFY2(
            matrix_bytes.contains("controlled Graph Builder shows at most")
                || matrix_bytes.contains("controlled facet panels"),
            "en-US matrix PDF should render localized facet controlled/truncation diagnostic");
        QVERIFY2(
            !matrix_bytes.contains("矩阵图（分面）"),
            "en-US matrix PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !matrix_bytes.contains("分面 ="),
            "en-US matrix PDF must not leak Chinese facet caption");
        QVERIFY2(
            !matrix_bytes.contains("变量数 ="),
            "en-US matrix PDF must not leak Chinese variable-count caption");
    }

    void pdf_graph_builder_faceted_bubble_and_time_series_localize_to_en_us_without_chinese_leak()
    {
        // Phase 7: real GraphService faceted bubble + time series → en-US engineer PDF byte scan.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto export_localized_pdf = [&](const DataTable& table,
                                        const OutputPage& page,
                                        const QString& filename) -> QByteArray {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest = build_export_manifest(
                document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        DataTable bubble_table;
        bubble_table.columns = {"x", "y", "size", "facet"};
        bubble_table.rows = {
            {"1", "10", "1", "P1"},
            {"2", "20", "2", "P1"},
            {"3", "30", "3", "P2"},
            {"4", "40", "4", "P2"},
            {"5", "50", "5", "P3"},
            {"6", "60", "6", "P3"},
            {"7", "70", "7", "P4"},
        };
        AnalysisConfiguration bubble_configuration;
        bubble_configuration.graph.graph_kind = "bubble";
        bubble_configuration.graph.x_column = 0;
        bubble_configuration.graph.y_column = 1;
        bubble_configuration.graph.size_column = 2;
        bubble_configuration.graph.facet_column = 3;
        bubble_configuration.graph.facet_max_panels = 2;
        bubble_configuration.excluded_rows = {6};
        bubble_configuration.hidden_rows = {1};
        const OutputPage bubble_page =
            GraphService::run(bubble_table, bubble_configuration);
        QCOMPARE(
            QString::fromStdString(bubble_page.title),
            QStringLiteral("气泡图（分面）"));
        QVERIFY(bubble_page.facts.eda.has_value());
        QVERIFY(bubble_page.facts.eda->facet_enabled);
        QVERIFY(bubble_page.plots.size() >= 2);
        bool saw_visibility_contract = false;
        for (const auto& diagnostic : bubble_page.diagnostics) {
            if (diagnostic.code == "row_visibility_contract") {
                saw_visibility_contract = true;
            }
        }
        QVERIFY(saw_visibility_contract);

        const QByteArray bubble_bytes = export_localized_pdf(
            bubble_table,
            bubble_page,
            QStringLiteral("graph_facet_bubble_en.pdf"));
        QVERIFY2(
            bubble_bytes.contains("Bubble chart (faceted)"),
            "en-US PDF should render localized faceted bubble page title");
        QVERIFY2(
            bubble_bytes.contains("Facet ="),
            "en-US bubble PDF should render localized facet parameter caption");
        QVERIFY2(
            bubble_bytes.contains("Size ="),
            "en-US bubble PDF should render localized size parameter caption");
        QVERIFY2(
            bubble_bytes.contains("controlled Graph Builder shows at most")
                || bubble_bytes.contains("controlled facet panels"),
            "en-US bubble PDF should render localized facet controlled/truncation diagnostic");
        QVERIFY2(
            bubble_bytes.contains("hidden affects display only"),
            "en-US bubble PDF should render localized row visibility contract diagnostic");
        QVERIFY2(
            !bubble_bytes.contains("气泡图（分面）"),
            "en-US bubble PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !bubble_bytes.contains("分面 ="),
            "en-US bubble PDF must not leak Chinese facet caption");
        QVERIFY2(
            !bubble_bytes.contains("大小 ="),
            "en-US bubble PDF must not leak Chinese size caption");
        QVERIFY2(
            !bubble_bytes.contains("hidden 只影响显示"),
            "en-US bubble PDF must not leak Chinese row visibility contract");

        DataTable series_table;
        series_table.columns = {"a", "b", "t", "y", "facet"};
        series_table.rows = {
            {"1", "10", "1", "5", "P1"},
            {"2", "20", "2", "6", "P1"},
            {"3", "30", "3", "7", "P2"},
            {"4", "40", "4", "8", "P2"},
            {"5", "50", "5", "9", "P3"},
            {"6", "60", "6", "10", "P3"},
            {"7", "70", "7", "11", "P4"},
        };
        AnalysisConfiguration series_configuration;
        series_configuration.graph.graph_kind = "time_series";
        series_configuration.graph.x_column = 2;
        series_configuration.graph.y_column = 3;
        series_configuration.graph.facet_column = 4;
        series_configuration.graph.facet_max_panels = 2;
        series_configuration.excluded_rows = {6};
        series_configuration.hidden_rows = {1};
        const OutputPage series_page =
            GraphService::run(series_table, series_configuration);
        QCOMPARE(
            QString::fromStdString(series_page.title),
            QStringLiteral("时间序列图（分面）"));
        QVERIFY(series_page.facts.eda.has_value());
        QVERIFY(series_page.facts.eda->facet_enabled);
        QVERIFY(series_page.plots.size() >= 2);
        QCOMPARE(series_page.facts.eda->facet_truncated_levels, std::size_t{1});
        bool saw_series_visibility_contract = false;
        for (const auto& diagnostic : series_page.diagnostics) {
            if (diagnostic.code == "row_visibility_contract") {
                saw_series_visibility_contract = true;
            }
        }
        QVERIFY(saw_series_visibility_contract);

        AnalysisConfiguration overlap_configuration = series_configuration;
        overlap_configuration.hidden_rows = {6};
        overlap_configuration.excluded_rows = {6};
        const OutputPage overlap_page =
            GraphService::run(series_table, overlap_configuration);
        bool saw_visibility_overlap = false;
        for (const auto& diagnostic : overlap_page.diagnostics) {
            if (diagnostic.code == "row_visibility_overlap") {
                saw_visibility_overlap = true;
            }
        }
        QVERIFY(saw_visibility_overlap);
        auto overlap_profile = make_report_profile(ReportTemplateKind::engineer);
        overlap_profile.locale.language_tag = "en-US";
        ReportDocument overlap_doc =
            build_report_document(bubble_table, {overlap_page}, overlap_profile, options);
        overlap_doc = localize_report_document(overlap_doc).document;
        QVERIFY(!overlap_doc.pages.empty());
        bool saw_overlap_diagnostic_en = false;
        for (const auto& diagnostic : overlap_doc.pages[0].visible_diagnostics) {
            if (diagnostic.code == "row_visibility_overlap") {
                saw_overlap_diagnostic_en = true;
                QVERIFY2(
                    diagnostic.message.find("marked both hidden and excluded")
                        != std::string::npos,
                    "overlap diagnostic must localize to en-US");
            }
        }
        QVERIFY(saw_overlap_diagnostic_en);

        const QByteArray series_bytes = export_localized_pdf(
            series_table,
            series_page,
            QStringLiteral("graph_facet_time_series_en.pdf"));
        QVERIFY2(
            series_bytes.contains("Time series plot (faceted)"),
            "en-US PDF should render localized faceted time-series page title");
        QVERIFY2(
            series_bytes.contains("Facet ="),
            "en-US time-series PDF should render localized facet parameter caption");
        QVERIFY2(
            series_bytes.contains("Time ="),
            "en-US time-series PDF should render localized time parameter caption");
        QVERIFY2(
            series_bytes.contains("Value ="),
            "en-US time-series PDF should render localized value parameter caption");
        QVERIFY2(
            series_bytes.contains("controlled Graph Builder shows at most")
                || series_bytes.contains("controlled facet panels"),
            "en-US time-series PDF should render localized facet controlled/truncation diagnostic");
        QVERIFY2(
            series_bytes.contains("hidden affects display only"),
            "en-US time-series PDF should render localized row visibility contract diagnostic");
        QVERIFY2(
            !series_bytes.contains("时间序列图（分面）"),
            "en-US time-series PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !series_bytes.contains("分面 ="),
            "en-US time-series PDF must not leak Chinese facet caption");
        QVERIFY2(
            !series_bytes.contains("时间 ="),
            "en-US time-series PDF must not leak Chinese time caption");
        QVERIFY2(
            !series_bytes.contains("数值 ="),
            "en-US time-series PDF must not leak Chinese value caption");
        QVERIFY2(
            !series_bytes.contains("hidden 只影响显示"),
            "en-US time-series PDF must not leak Chinese row visibility contract");
    }

    void pdf_graph_builder_faceted_area_parallel_and_marginal_localize_to_en_us_without_chinese_leak()
    {
        // Phase 7: real GraphService faceted area + parallel + marginal → en-US PDF byte scan.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto export_localized_pdf = [&](const DataTable& table,
                                        const OutputPage& page,
                                        const QString& filename) -> QByteArray {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest = build_export_manifest(
                document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        DataTable area_table;
        area_table.columns = {"a", "b", "t", "y", "facet"};
        area_table.rows = {
            {"1", "10", "1", "5", "P1"},
            {"2", "20", "2", "6", "P1"},
            {"3", "30", "3", "7", "P2"},
            {"4", "40", "4", "8", "P2"},
            {"5", "50", "5", "9", "P3"},
            {"6", "60", "6", "10", "P3"},
            {"7", "70", "7", "11", "P4"},
        };
        AnalysisConfiguration area_configuration;
        area_configuration.graph.graph_kind = "area";
        area_configuration.graph.time_column = 2;
        area_configuration.graph.x_column = 2;
        area_configuration.graph.y_column = 3;
        area_configuration.graph.facet_column = 4;
        area_configuration.graph.facet_max_panels = 2;
        area_configuration.excluded_rows = {6};
        area_configuration.hidden_rows = {1};
        const OutputPage area_page = GraphService::run(area_table, area_configuration);
        QCOMPARE(
            QString::fromStdString(area_page.title),
            QStringLiteral("区域图（分面）"));
        QVERIFY(area_page.facts.eda.has_value());
        QCOMPARE(area_page.facts.eda->kind, std::string("area"));
        QVERIFY(area_page.facts.eda->facet_enabled);
        QVERIFY(area_page.plots.size() >= 2);

        const QByteArray area_bytes = export_localized_pdf(
            area_table, area_page, QStringLiteral("graph_facet_area_en.pdf"));
        QVERIFY2(
            area_bytes.contains("Faceted Area Plot"),
            "en-US PDF should render localized faceted area page title");
        QVERIFY2(
            area_bytes.contains("Facet ="),
            "en-US area PDF should render localized facet parameter caption");
        QVERIFY2(
            area_bytes.contains("Area shows the numeric interval"),
            "en-US area PDF should render localized area-not-CI honesty caption");
        QVERIFY2(
            area_bytes.contains("hidden affects display only"),
            "en-US area PDF should render localized row visibility contract diagnostic");
        QVERIFY2(
            !area_bytes.contains("区域图（分面）"),
            "en-US area PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !area_bytes.contains("分面 ="),
            "en-US area PDF must not leak Chinese facet caption");
        QVERIFY2(
            !area_bytes.contains("面积表示相邻观测"),
            "en-US area PDF must not leak Chinese area-not-CI caption");

        AnalysisConfiguration parallel_configuration;
        parallel_configuration.graph.graph_kind = "parallel";
        parallel_configuration.graph.variable_columns = {0, 1};
        parallel_configuration.graph.facet_column = 4;
        parallel_configuration.graph.facet_max_panels = 2;
        parallel_configuration.excluded_rows = {6};
        parallel_configuration.hidden_rows = {1};
        const OutputPage parallel_page =
            GraphService::run(area_table, parallel_configuration);
        QCOMPARE(
            QString::fromStdString(parallel_page.title),
            QStringLiteral("平行坐标图（分面）"));
        QVERIFY(parallel_page.facts.eda.has_value());
        QVERIFY(parallel_page.facts.eda->facet_enabled);
        QVERIFY(parallel_page.plots.size() >= 2);

        const QByteArray parallel_bytes = export_localized_pdf(
            area_table, parallel_page, QStringLiteral("graph_facet_parallel_en.pdf"));
        QVERIFY2(
            parallel_bytes.contains("Parallel coordinates (faceted)"),
            "en-US PDF should render localized faceted parallel page title");
        QVERIFY2(
            parallel_bytes.contains("Facet ="),
            "en-US parallel PDF should render localized facet parameter caption");
        QVERIFY2(
            parallel_bytes.contains("Variable count ="),
            "en-US parallel PDF should render localized variable-count parameter caption");
        QVERIFY2(
            parallel_bytes.contains("Coordinates scaled to each variable"),
            "en-US parallel PDF should render localized min-max standardization caption");
        QVERIFY2(
            !parallel_bytes.contains("平行坐标图（分面）"),
            "en-US parallel PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !parallel_bytes.contains("分面 ="),
            "en-US parallel PDF must not leak Chinese facet caption");
        QVERIFY2(
            !parallel_bytes.contains("坐标已按各变量"),
            "en-US parallel PDF must not leak Chinese standardization caption");

        DataTable marginal_table;
        marginal_table.columns = {"x", "y", "size", "facet"};
        marginal_table.rows = {
            {"1", "10", "1", "P1"},
            {"2", "20", "2", "P1"},
            {"3", "30", "3", "P2"},
            {"4", "40", "4", "P2"},
            {"5", "50", "5", "P3"},
            {"6", "60", "6", "P3"},
            {"7", "70", "7", "P4"},
        };
        AnalysisConfiguration marginal_configuration;
        marginal_configuration.graph.graph_kind = "marginal";
        marginal_configuration.graph.x_column = 0;
        marginal_configuration.graph.y_column = 1;
        marginal_configuration.graph.facet_column = 3;
        marginal_configuration.graph.facet_max_panels = 2;
        marginal_configuration.excluded_rows = {6};
        marginal_configuration.hidden_rows = {1};
        const OutputPage marginal_page =
            GraphService::run(marginal_table, marginal_configuration);
        QCOMPARE(
            QString::fromStdString(marginal_page.title),
            QStringLiteral("边际图（分面）"));
        QVERIFY(marginal_page.facts.eda.has_value());
        QVERIFY(marginal_page.facts.eda->facet_enabled);
        QVERIFY(marginal_page.plots.size() >= 2);

        const QByteArray marginal_bytes = export_localized_pdf(
            marginal_table, marginal_page, QStringLiteral("graph_facet_marginal_en.pdf"));
        QVERIFY2(
            marginal_bytes.contains("Marginal plot (faceted)"),
            "en-US PDF should render localized faceted marginal page title");
        QVERIFY2(
            marginal_bytes.contains("Facet ="),
            "en-US marginal PDF should render localized facet parameter caption");
        QVERIFY2(
            marginal_bytes.contains("X =") && marginal_bytes.contains("Y ="),
            "en-US marginal PDF should render localized axis parameter captions");
        QVERIFY2(
            !marginal_bytes.contains("边际图（分面）"),
            "en-US marginal PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !marginal_bytes.contains("分面 ="),
            "en-US marginal PDF must not leak Chinese facet caption");
    }

    void pdf_graph_builder_faceted_probability_and_ecdf_localize_to_en_us_without_chinese_leak()
    {
        // Phase 7: real GraphService faceted probability + ECDF → en-US PDF byte scan.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto export_localized_pdf = [&](const DataTable& table,
                                        const OutputPage& page,
                                        const QString& filename) -> QByteArray {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest = build_export_manifest(
                document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        DataTable table;
        table.columns = {"x", "y", "size", "facet"};
        table.rows = {
            {"1", "10", "1", "P1"},
            {"2", "20", "2", "P1"},
            {"3", "30", "3", "P2"},
            {"4", "40", "4", "P2"},
            {"5", "50", "5", "P3"},
            {"6", "60", "6", "P3"},
            {"7", "70", "7", "P4"},
        };
        AnalysisConfiguration probability_configuration;
        probability_configuration.graph.graph_kind = "probability";
        probability_configuration.graph.y_column = 1;
        probability_configuration.graph.facet_column = 3;
        probability_configuration.graph.facet_max_panels = 2;
        probability_configuration.excluded_rows = {6};
        probability_configuration.hidden_rows = {1};
        const OutputPage probability_page =
            GraphService::run(table, probability_configuration);
        QCOMPARE(
            QString::fromStdString(probability_page.title),
            QStringLiteral("正态概率图（分面）"));
        QVERIFY(probability_page.facts.eda.has_value());
        QVERIFY(probability_page.facts.eda->facet_enabled);
        QVERIFY(probability_page.plots.size() >= 2);

        const QByteArray probability_bytes = export_localized_pdf(
            table, probability_page, QStringLiteral("graph_facet_probability_en.pdf"));
        QVERIFY2(
            probability_bytes.contains("Normal probability plot (faceted)"),
            "en-US PDF should render localized faceted probability page title");
        QVERIFY2(
            probability_bytes.contains("Facet ="),
            "en-US probability PDF should render localized facet parameter caption");
        QVERIFY2(
            probability_bytes.contains("Variable ="),
            "en-US probability PDF should render localized variable parameter caption");
        QVERIFY2(
            probability_bytes.contains("Linearity alone is not proof of normality"),
            "en-US probability PDF should render localized linearity honesty caption");
        QVERIFY2(
            probability_bytes.contains("hidden affects display only"),
            "en-US probability PDF should render localized row visibility contract diagnostic");
        QVERIFY2(
            !probability_bytes.contains("正态概率图（分面）"),
            "en-US probability PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !probability_bytes.contains("分面 ="),
            "en-US probability PDF must not leak Chinese facet caption");
        QVERIFY2(
            !probability_bytes.contains("直线性不能单独"),
            "en-US probability PDF must not leak Chinese linearity honesty caption");

        AnalysisConfiguration ecdf_configuration;
        ecdf_configuration.graph.graph_kind = "ecdf";
        ecdf_configuration.graph.y_column = 1;
        ecdf_configuration.graph.facet_column = 3;
        ecdf_configuration.graph.facet_max_panels = 2;
        ecdf_configuration.excluded_rows = {6};
        ecdf_configuration.hidden_rows = {1};
        const OutputPage ecdf_page = GraphService::run(table, ecdf_configuration);
        QCOMPARE(
            QString::fromStdString(ecdf_page.title),
            QStringLiteral("经验累积分布图（分面）"));
        QVERIFY(ecdf_page.facts.eda.has_value());
        QVERIFY(ecdf_page.facts.eda->facet_enabled);
        QVERIFY(ecdf_page.plots.size() >= 2);

        const QByteArray ecdf_bytes = export_localized_pdf(
            table, ecdf_page, QStringLiteral("graph_facet_ecdf_en.pdf"));
        QVERIFY2(
            ecdf_bytes.contains("Empirical CDF (faceted)"),
            "en-US PDF should render localized faceted ECDF page title");
        QVERIFY2(
            ecdf_bytes.contains("Facet ="),
            "en-US ECDF PDF should render localized facet parameter caption");
        QVERIFY2(
            ecdf_bytes.contains("Variable ="),
            "en-US ECDF PDF should render localized variable parameter caption");
        QVERIFY2(
            ecdf_bytes.contains("hidden affects display only"),
            "en-US ECDF PDF should render localized row visibility contract diagnostic");
        QVERIFY2(
            !ecdf_bytes.contains("经验累积分布图（分面）"),
            "en-US ECDF PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !ecdf_bytes.contains("分面 ="),
            "en-US ECDF PDF must not leak Chinese facet caption");
        QVERIFY2(
            !ecdf_bytes.contains("累计比例"),
            "en-US ECDF PDF must not leak Chinese cumulative-proportion axis caption");
    }

    void pdf_graph_builder_faceted_correlation_and_heatmap_localize_to_en_us_without_chinese_leak()
    {
        // Phase 7: real GraphService faceted correlation + correlation-heatmap → en-US PDF byte scan.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto export_localized_pdf = [&](const DataTable& table,
                                        const OutputPage& page,
                                        const QString& filename) -> QByteArray {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest = build_export_manifest(
                document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        DataTable table;
        table.columns = {"a", "b", "row", "col", "z", "facet"};
        table.rows = {
            {"1", "10", "R1", "C1", "1", "P1"},
            {"2", "20", "R1", "C2", "2", "P1"},
            {"3", "30", "R2", "C1", "3", "P1"},
            {"4", "40", "R2", "C2", "4", "P1"},
            {"5", "50", "R1", "C1", "2", "P2"},
            {"6", "60", "R1", "C2", "3", "P2"},
            {"7", "70", "R2", "C1", "4", "P2"},
            {"8", "80", "R2", "C2", "5", "P2"},
            {"9", "90", "R1", "C1", "3", "P3"},
            {"10", "100", "R1", "C2", "4", "P3"},
            {"11", "110", "R2", "C1", "5", "P3"},
            {"12", "120", "R2", "C2", "6", "P3"},
            {"13", "130", "R1", "C1", "9", "P4"},
        };
        AnalysisConfiguration correlation_configuration;
        correlation_configuration.graph.graph_kind = "correlation";
        correlation_configuration.graph.variable_columns = {0, 1};
        correlation_configuration.graph.correlation_method = "pearson";
        correlation_configuration.graph.facet_column = 5;
        correlation_configuration.graph.facet_max_panels = 2;
        correlation_configuration.excluded_rows = {12};
        correlation_configuration.hidden_rows = {1};
        const OutputPage correlation_page =
            GraphService::run(table, correlation_configuration);
        QCOMPARE(
            QString::fromStdString(correlation_page.title),
            QStringLiteral("相关图（分面）"));
        QVERIFY(correlation_page.facts.eda.has_value());
        QVERIFY(correlation_page.facts.eda->facet_enabled);
        QCOMPARE(correlation_page.facts.eda->facet_truncated_levels, std::size_t{1});
        QVERIFY(correlation_page.plots.size() >= 2);
        QVERIFY(correlation_page.plots.front().member_source_rows.empty());

        const QByteArray correlation_bytes = export_localized_pdf(
            table, correlation_page, QStringLiteral("graph_facet_correlation_en.pdf"));
        QVERIFY2(
            correlation_bytes.contains("Correlogram (faceted)"),
            "en-US PDF should render localized faceted correlation page title");
        QVERIFY2(
            correlation_bytes.contains("Facet ="),
            "en-US correlation PDF should render localized facet parameter caption");
        QVERIFY2(
            correlation_bytes.contains("Variable count ="),
            "en-US correlation PDF should render localized variable-count parameter caption");
        QVERIFY2(
            correlation_bytes.contains("Method ="),
            "en-US correlation PDF should render localized method parameter caption");
        QVERIFY2(
            correlation_bytes.contains("Correlation-matrix heatmap cells are not an observation layer"),
            "en-US correlation PDF should render localized correlation-not-obs-layer honesty caption");
        QVERIFY2(
            correlation_bytes.contains("hidden affects display only"),
            "en-US correlation PDF should render localized row visibility contract diagnostic");
        QVERIFY2(
            !correlation_bytes.contains("相关图（分面）"),
            "en-US correlation PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !correlation_bytes.contains("分面 ="),
            "en-US correlation PDF must not leak Chinese facet caption");
        QVERIFY2(
            !correlation_bytes.contains("相关矩阵热图单元格"),
            "en-US correlation PDF must not leak Chinese correlation-not-obs-layer caption");
        QVERIFY2(
            !correlation_bytes.contains("变量数 ="),
            "en-US correlation PDF must not leak Chinese variable-count caption");

        AnalysisConfiguration heatmap_configuration;
        heatmap_configuration.graph.graph_kind = "heatmap";
        heatmap_configuration.graph.variable_columns = {0, 1};
        heatmap_configuration.graph.correlation_method = "pearson";
        heatmap_configuration.graph.facet_column = 5;
        heatmap_configuration.graph.facet_max_panels = 2;
        heatmap_configuration.excluded_rows = {12};
        heatmap_configuration.hidden_rows = {1};
        const OutputPage heatmap_page =
            GraphService::run(table, heatmap_configuration);
        QCOMPARE(
            QString::fromStdString(heatmap_page.title),
            QStringLiteral("热图（分面）"));
        QVERIFY(heatmap_page.facts.eda.has_value());
        QVERIFY(heatmap_page.facts.eda->facet_enabled);
        QVERIFY(heatmap_page.plots.size() >= 2);
        QVERIFY(heatmap_page.plots.front().member_source_rows.empty());

        const QByteArray heatmap_bytes = export_localized_pdf(
            table, heatmap_page, QStringLiteral("graph_facet_heatmap_en.pdf"));
        QVERIFY2(
            heatmap_bytes.contains("Heatmap (faceted)"),
            "en-US PDF should render localized faceted heatmap page title");
        QVERIFY2(
            heatmap_bytes.contains("Color scale fixed to correlation [-1, 1]"),
            "en-US heatmap PDF should render localized correlation color-range caption");
        QVERIFY2(
            heatmap_bytes.contains("Facet ="),
            "en-US heatmap PDF should render localized facet parameter caption");
        QVERIFY2(
            heatmap_bytes.contains("Method ="),
            "en-US heatmap PDF should render localized method parameter caption");
        QVERIFY2(
            heatmap_bytes.contains("Correlation-matrix cells are not an observation layer"),
            "en-US heatmap PDF should render localized heatmap-not-obs-layer honesty caption");
        QVERIFY2(
            heatmap_bytes.contains("hidden affects display only"),
            "en-US heatmap PDF should render localized row visibility contract diagnostic");
        QVERIFY2(
            !heatmap_bytes.contains("热图（分面）"),
            "en-US heatmap PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !heatmap_bytes.contains("分面 ="),
            "en-US heatmap PDF must not leak Chinese facet caption");
        QVERIFY2(
            !heatmap_bytes.contains("颜色范围固定为相关系数"),
            "en-US heatmap PDF must not leak Chinese color-range caption");
        QVERIFY2(
            !heatmap_bytes.contains("相关矩阵单元格不是观测层"),
            "en-US heatmap PDF must not leak Chinese heatmap-not-obs-layer caption");
    }

    void pdf_graph_builder_faceted_category_heatmap_localizes_to_en_us_without_chinese_leak()
    {
        // Phase 7: category heatmap (row/col/z) faceted path → en-US PDF byte scan.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto export_localized_pdf = [&](const DataTable& table,
                                        const OutputPage& page,
                                        const QString& filename) -> QByteArray {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest = build_export_manifest(
                document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        DataTable table;
        table.columns = {"a", "b", "row", "col", "z", "facet"};
        table.rows = {
            {"1", "10", "R1", "C1", "1", "P1"},
            {"2", "20", "R1", "C2", "2", "P1"},
            {"3", "30", "R2", "C1", "3", "P1"},
            {"4", "40", "R2", "C2", "4", "P1"},
            {"5", "50", "R1", "C1", "2", "P2"},
            {"6", "60", "R1", "C2", "3", "P2"},
            {"7", "70", "R2", "C1", "4", "P2"},
            {"8", "80", "R2", "C2", "5", "P2"},
            {"9", "90", "R1", "C1", "3", "P3"},
            {"10", "100", "R1", "C2", "4", "P3"},
            {"11", "110", "R2", "C1", "5", "P3"},
            {"12", "120", "R2", "C2", "6", "P3"},
            {"13", "130", "R1", "C1", "9", "P4"},
        };
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "heatmap";
        configuration.graph.x_column = 3;
        configuration.graph.y_column = 2;
        configuration.graph.z_column = 4;
        configuration.graph.facet_column = 5;
        configuration.graph.facet_max_panels = 2;
        configuration.excluded_rows = {12};
        configuration.hidden_rows = {1};
        const OutputPage heatmap_page = GraphService::run(table, configuration);
        QCOMPARE(
            QString::fromStdString(heatmap_page.title),
            QStringLiteral("热图（分面）"));
        QVERIFY(heatmap_page.facts.eda.has_value());
        QVERIFY(heatmap_page.facts.eda->facet_enabled);
        QVERIFY(heatmap_page.plots.size() >= 2);
        QVERIFY(!heatmap_page.plots.front().member_source_rows.empty());
        bool hidden_leaked = false;
        for (const auto& members : heatmap_page.plots.front().member_source_rows) {
            if (std::find(members.begin(), members.end(), std::size_t{1}) != members.end()) {
                hidden_leaked = true;
            }
        }
        QVERIFY(!hidden_leaked);

        const QByteArray heatmap_bytes = export_localized_pdf(
            table, heatmap_page, QStringLiteral("graph_facet_category_heatmap_en.pdf"));
        QVERIFY2(
            heatmap_bytes.contains("Heatmap (faceted)"),
            "en-US PDF should render localized faceted category-heatmap page title");
        QVERIFY2(
            heatmap_bytes.contains("Cells are within-group means"),
            "en-US category-heatmap PDF should render localized cell-mean caption");
        QVERIFY2(
            heatmap_bytes.contains("Facet ="),
            "en-US category-heatmap PDF should render localized facet parameter caption");
        QVERIFY2(
            heatmap_bytes.contains("hidden affects display only"),
            "en-US category-heatmap PDF should render localized row visibility contract diagnostic");
        QVERIFY2(
            !heatmap_bytes.contains("热图（分面）"),
            "en-US category-heatmap PDF must not leak Chinese faceted page title");
        QVERIFY2(
            !heatmap_bytes.contains("分面 ="),
            "en-US category-heatmap PDF must not leak Chinese facet caption");
        QVERIFY2(
            !heatmap_bytes.contains("单元格为组内均值"),
            "en-US category-heatmap PDF must not leak Chinese cell-mean caption");
    }

    void pdf_graph_builder_pie_localizes_to_en_us_without_chinese_leak()
    {
        // Phase 7: pie chart (no faceting) → en-US PDF byte scan.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        DataTable table;
        table.columns = {"x", "y", "cat"};
        table.rows = {
            {"1", "10", "A"},
            {"2", "20", "A"},
            {"3", "30", "B"},
            {"4", "40", "B"},
            {"5", "50", "C"},
        };
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "pie";
        configuration.graph.x_column = 2;
        configuration.graph.other_threshold_percent = 5.0;
        configuration.hidden_rows = {1};
        configuration.excluded_rows = {3};
        const OutputPage pie_page = GraphService::run(table, configuration);
        QCOMPARE(QString::fromStdString(pie_page.title), QStringLiteral("饼图"));
        QVERIFY(pie_page.facts.eda.has_value());
        QVERIFY(!pie_page.plots.empty());

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        ReportDocument document =
            build_report_document(table, {pie_page}, profile, options);
        document = localize_report_document(document).document;
        QTemporaryDir directory;
        QVERIFY2(directory.isValid(), "temp dir required for PDF export");
        const QString pdf_path = directory.filePath(QStringLiteral("graph_pie_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("graph_pie_en.pdf"));
        manifest.consistency_status = "ok";
        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));
        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray pie_bytes = pdf_file.readAll();
        QVERIFY2(
            pie_bytes.contains("Pie chart"),
            "en-US PDF should render localized pie page title");
        QVERIFY2(
            pie_bytes.contains("Category ="),
            "en-US pie PDF should render localized category parameter caption");
        QVERIFY2(
            pie_bytes.contains("Small-category merge threshold ="),
            "en-US pie PDF should render localized merge-threshold parameter caption");
        QVERIFY2(
            pie_bytes.contains("hidden affects display only"),
            "en-US pie PDF should render localized row visibility contract diagnostic");
        QVERIFY2(
            !pie_bytes.contains("饼图"),
            "en-US pie PDF must not leak Chinese page title");
        QVERIFY2(
            !pie_bytes.contains("类别 ="),
            "en-US pie PDF must not leak Chinese category caption");
        QVERIFY2(
            !pie_bytes.contains("小类别合并阈值"),
            "en-US pie PDF must not leak Chinese merge-threshold caption");
        QVERIFY2(
            !pie_bytes.contains("组成比例"),
            "en-US pie PDF must not leak Chinese composition-proportion axis caption");
    }

    void pdf_rsm_lof_gate_localizes_and_audit_evidence_carries_label_text_id()
    {
        // Phase 4 DOE-4: RSM with replicated centers → LOF gate + en-US audit PDF/JSON.
        const std::vector<std::vector<double>> coded = {
            {-1.0, -1.0}, {1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0},
            {-1.0, 0.0}, {1.0, 0.0}, {0.0, -1.0}, {0.0, 1.0},
            {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}};
        std::vector<double> response;
        response.reserve(coded.size());
        for (std::size_t i = 0; i < coded.size(); ++i) {
            const double x1 = coded[i][0];
            const double x2 = coded[i][1];
            double y = 10.0 + 3.0 * x1 - 2.0 * x2 + 1.5 * x1 * x2
                + 0.8 * x1 * x1 - 0.4 * x2 * x2;
            if (std::fabs(x1) < 1.0e-9 && std::fabs(x2) < 1.0e-9) {
                y += (i == 8) ? 0.4 : ((i == 9) ? -0.2 : -0.1);
            }
            response.push_back(y);
        }
        DataTable table;
        table.name = "rsm_lof";
        table.columns = {"Y", "A", "B"};
        for (std::size_t i = 0; i < coded.size(); ++i) {
            table.rows.push_back({
                std::to_string(response[i]),
                std::to_string(coded[i][0]),
                std::to_string(coded[i][1])});
        }
        AnalysisConfiguration configuration;
        configuration.chart_type = "rsm_response";
        configuration.variable_columns = {0, 1, 2};
        OutputPage page = AnalysisService::rsm_response(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.rsm.has_value());
        QVERIFY(page.facts.rsm->pure_error_available);
        QVERIFY(page.facts.rsm->lack_of_fit_available);
        QCOMPARE(
            QString::fromStdString(page.facts.rsm->evidence_type),
            QStringLiteral("formula_reference"));

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        const auto localized = localize_report_document(document);
        document = localized.document;
        QVERIFY(!document.pages.empty());
        QCOMPARE(
            document.pages[0].source_page.title,
            std::string("Response surface analysis"));

        bool saw_lof_evidence = false;
        for (const auto& ref : document.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:rsm_lof_formula_reference")
                    != std::string::npos) {
                saw_lof_evidence = true;
                QCOMPARE(
                    ref.label_text_id,
                    std::string("evidence.rsm_lof_formula_reference"));
                QCOMPARE(ref.diagnostic_code, std::string("rsm_lof_formula_reference"));
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
            }
        }
        QVERIFY2(
            saw_lof_evidence,
            "audit evidence bundle must include RSM LOF limiting gate ref");

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("rsm_lof_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("rsm_lof_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(bytes.size() > 800, "RSM audit PDF should contain drawn content");
        QVERIFY2(
            bytes.contains("Response surface analysis"),
            "en-US PDF should render localized RSM page title");
        QVERIFY2(
            bytes.contains("RSM lack-of-fit ANOVA"),
            "audit PDF evidence appendix should render localized LOF gate label");
        QVERIFY2(
            bytes.contains("Replicated coded points detected")
                || bytes.contains("Lack-of-fit ANOVA evidence_type"),
            "en-US PDF should render localized RSM LOF/replicate diagnostic");
        QVERIFY2(
            !bytes.contains("响应曲面分析"),
            "en-US RSM PDF must not leak Chinese page title");
        QVERIFY2(
            !bytes.contains("失拟 ANOVA"),
            "en-US RSM PDF must not leak Chinese LOF diagnostic");

        QVERIFY(QFileInfo::exists(exported.audit_json_path));
        QFile audit_file(exported.audit_json_path);
        QVERIFY(audit_file.open(QIODevice::ReadOnly));
        const QJsonArray evidence_refs =
            QJsonDocument::fromJson(audit_file.readAll())
                .object()
                .value(QStringLiteral("evidence"))
                .toObject()
                .value(QStringLiteral("evidence"))
                .toArray();
        bool saw_audit_lof = false;
        for (const QJsonValue& value : evidence_refs) {
            const QJsonObject ref = value.toObject();
            if (ref.value(QStringLiteral("label_text_id")).toString()
                    != QStringLiteral("evidence.rsm_lof_formula_reference")) {
                continue;
            }
            saw_audit_lof = true;
            QCOMPARE(
                ref.value(QStringLiteral("diagnostic_code")).toString(),
                QStringLiteral("rsm_lof_formula_reference"));
            const std::string label = resolve_report_text(
                "evidence.rsm_lof_formula_reference", "en-US").text;
            QVERIFY2(
                label.find("RSM lack-of-fit ANOVA") != std::string::npos,
                label.c_str());
            QVERIFY2(label.find("vendor_oracle") != std::string::npos, label.c_str());
        }
        QVERIFY2(
            saw_audit_lof,
            "audit JSON must serialize RSM LOF gate with label_text_id");
    }

    void pdf_rsm_lof_cross_template_table_visibility_and_en_us_locale()
    {
        // Phase 4 DOE-4: RSM LOF gate × customer/engineer/audit PDF byte scan.
        const std::vector<std::vector<double>> coded = {
            {-1.0, -1.0}, {1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0},
            {-1.0, 0.0}, {1.0, 0.0}, {0.0, -1.0}, {0.0, 1.0},
            {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}};
        std::vector<double> response;
        response.reserve(coded.size());
        for (std::size_t i = 0; i < coded.size(); ++i) {
            const double x1 = coded[i][0];
            const double x2 = coded[i][1];
            double y = 10.0 + 3.0 * x1 - 2.0 * x2 + 1.5 * x1 * x2
                + 0.8 * x1 * x1 - 0.4 * x2 * x2;
            if (std::fabs(x1) < 1.0e-9 && std::fabs(x2) < 1.0e-9) {
                y += (i == 8) ? 0.4 : ((i == 9) ? -0.2 : -0.1);
            }
            response.push_back(y);
        }
        DataTable table;
        table.name = "rsm_lof";
        table.columns = {"Y", "A", "B"};
        for (std::size_t i = 0; i < coded.size(); ++i) {
            table.rows.push_back({
                std::to_string(response[i]),
                std::to_string(coded[i][0]),
                std::to_string(coded[i][1])});
        }
        AnalysisConfiguration configuration;
        configuration.chart_type = "rsm_response";
        configuration.variable_columns = {0, 1, 2};
        OutputPage page = AnalysisService::rsm_response(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.rsm.has_value());
        QVERIFY(page.facts.rsm->lack_of_fit_available);

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](ReportTemplateKind kind) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            return document;
        };

        const ReportDocument customer_doc =
            build_localized(ReportTemplateKind::customer);
        const ReportDocument engineer_doc =
            build_localized(ReportTemplateKind::engineer);
        const ReportDocument audit_doc =
            build_localized(ReportTemplateKind::audit);
        QVERIFY(customer_doc.pages[0].visible_tables.empty());
        QVERIFY(!engineer_doc.pages[0].visible_tables.empty());
        QVERIFY(!audit_doc.pages[0].visible_tables.empty());
        QCOMPARE(
            customer_doc.provenance.facts_hash,
            engineer_doc.provenance.facts_hash);
        QCOMPARE(engineer_doc.provenance.facts_hash, audit_doc.provenance.facts_hash);

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest =
                build_export_manifest(document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const QByteArray customer_bytes =
            export_bytes(customer_doc, QStringLiteral("rsm_lof_customer_en.pdf"));
        const QByteArray engineer_bytes =
            export_bytes(engineer_doc, QStringLiteral("rsm_lof_engineer_en.pdf"));
        const QByteArray audit_bytes =
            export_bytes(audit_doc, QStringLiteral("rsm_lof_audit_en.pdf"));

        QVERIFY2(
            customer_bytes.contains("Response surface analysis"),
            "customer PDF should still render localized page title");
        QVERIFY2(
            !customer_bytes.contains("Coefficients (coded units)"),
            "customer PDF must omit RSM coefficient statistic table");
        QVERIFY2(
            !customer_bytes.contains("Pure error and lack of fit"),
            "customer PDF must omit pure-error/LOF statistic table");

        for (const QByteArray& bytes : {engineer_bytes, audit_bytes}) {
            QVERIFY2(
                bytes.contains("Response surface analysis"),
                "engineer/audit PDF should render localized page title");
            QVERIFY2(
                bytes.contains("Coefficients (coded units)")
                    || bytes.contains("Pure error and lack of fit"),
                "engineer/audit PDF should render at least one RSM statistic table");
            QVERIFY2(
                !bytes.contains("响应曲面分析"),
                "en-US RSM PDF must not leak Chinese page title");
            QVERIFY2(
                !bytes.contains("纯误差与失拟"),
                "en-US RSM PDF must not leak Chinese LOF table title");
        }

        QVERIFY2(
            audit_bytes.contains("Evidence appendix"),
            "audit PDF should include full evidence appendix heading");
        QVERIFY2(
            audit_bytes.contains("RSM lack-of-fit ANOVA"),
            "audit PDF evidence appendix should render localized LOF gate label");
        QVERIFY2(
            !engineer_bytes.contains("Evidence appendix"),
            "engineer PDF should not include audit-only evidence appendix");
    }

    void pdf_reliability_km_and_weibull_localize_to_en_us_without_chinese_leak()
    {
        // Phase 5 REL-2/REL-3: KM + Weibull + Lognormal → en-US engineer PDF byte scan.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto export_localized_pdf = [&](const DataTable& table,
                                        const OutputPage& page,
                                        const QString& filename) -> QByteArray {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest = build_export_manifest(
                document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        DataTable km_table;
        km_table.columns = {"time", "censor_type", "mode"};
        km_table.rows = {
            {"10", "exact", "wear"},
            {"15", "right", ""},
            {"20", "failure", "wear"},
            {"25", "censored", ""},
            {"30", "exact", "wear"}};
        AnalysisConfiguration km_cfg;
        km_cfg.reliability.time_column = 0;
        km_cfg.reliability.censoring_type_column = 1;
        km_cfg.reliability.failure_mode_column = 2;
        km_cfg.reliability.model = "kaplan_meier";
        km_cfg.reliability.time_unit = "hours";
        OutputPage km_page = AnalysisService::reliability(km_table, km_cfg);
        InterpretationService::enrich(km_page);
        QVERIFY(km_page.facts.reliability.has_value());

        const QByteArray km_bytes = export_localized_pdf(
            km_table, km_page, QStringLiteral("reliability_km_en.pdf"));
        QVERIFY2(
            km_bytes.contains("Reliability analysis"),
            "en-US KM PDF should render localized page title");
        QVERIFY2(
            km_bytes.contains("Kaplan-Meier survival table"),
            "en-US KM PDF should render localized survival table title");
        QVERIFY2(
            km_bytes.contains("Lifetime column ="),
            "en-US KM PDF should render localized lifetime parameter caption");
        QVERIFY2(
            !km_bytes.contains("可靠性分析"),
            "en-US KM PDF must not leak Chinese page title");
        QVERIFY2(
            !km_bytes.contains("Kaplan-Meier 生存表"),
            "en-US KM PDF must not leak Chinese KM table title");

        DataTable weibull_table;
        weibull_table.columns = {"time", "event"};
        weibull_table.rows = {
            {"10", "1"}, {"20", "1"}, {"30", "1"}, {"40", "1"}, {"50", "1"},
            {"25", "0"}, {"35", "0"}};
        AnalysisConfiguration weibull_cfg;
        weibull_cfg.reliability.time_column = 0;
        weibull_cfg.reliability.event_column = 1;
        weibull_cfg.reliability.model = "weibull";
        weibull_cfg.reliability.time_unit = "hours";
        OutputPage weibull_page =
            AnalysisService::reliability(weibull_table, weibull_cfg);
        InterpretationService::enrich(weibull_page);
        QVERIFY(weibull_page.facts.reliability.has_value());
        QCOMPARE(
            QString::fromStdString(weibull_page.facts.reliability->evidence_type),
            QStringLiteral("formula_reference"));

        const QByteArray weibull_bytes = export_localized_pdf(
            weibull_table, weibull_page, QStringLiteral("reliability_weibull_en.pdf"));
        QVERIFY2(
            weibull_bytes.contains("Reliability analysis"),
            "en-US Weibull PDF should render localized page title");
        QVERIFY2(
            weibull_bytes.contains("Weibull parameters"),
            "en-US Weibull PDF should render localized parameter table title");
        QVERIFY2(
            weibull_bytes.contains("Percentile life"),
            "en-US Weibull PDF should render localized percentile table title");
        QVERIFY2(
            weibull_bytes.contains("Using 2-parameter Weibull"),
            "en-US Weibull PDF should render localized two-parameter hint diagnostic");
        QVERIFY2(
            !weibull_bytes.contains("可靠性分析"),
            "en-US Weibull PDF must not leak Chinese page title");
        QVERIFY2(
            !weibull_bytes.contains("Weibull 参数"),
            "en-US Weibull PDF must not leak Chinese parameter table title");
        QVERIFY2(
            !weibull_bytes.contains("百分位寿命"),
            "en-US Weibull PDF must not leak Chinese percentile table title");

        AnalysisConfiguration lognormal_cfg = weibull_cfg;
        lognormal_cfg.reliability.model = "lognormal";
        OutputPage lognormal_page =
            AnalysisService::reliability(weibull_table, lognormal_cfg);
        InterpretationService::enrich(lognormal_page);
        QVERIFY(lognormal_page.facts.reliability.has_value());
        QCOMPARE(
            QString::fromStdString(lognormal_page.facts.reliability->evidence_type),
            QStringLiteral("formula_reference"));

        const QByteArray lognormal_bytes = export_localized_pdf(
            weibull_table, lognormal_page, QStringLiteral("reliability_lognormal_en.pdf"));
        QVERIFY2(
            lognormal_bytes.contains("Reliability analysis"),
            "en-US Lognormal PDF should render localized page title");
        QVERIFY2(
            lognormal_bytes.contains("Lognormal parameters"),
            "en-US Lognormal PDF should render localized parameter table title");
        QVERIFY2(
            lognormal_bytes.contains("Percentile life"),
            "en-US Lognormal PDF should render localized percentile table title");
        QVERIFY2(
            lognormal_bytes.contains("Using 2-parameter lognormal"),
            "en-US Lognormal PDF should render localized two-parameter hint diagnostic");
        QVERIFY2(
            !lognormal_bytes.contains("Lognormal 参数"),
            "en-US Lognormal PDF must not leak Chinese parameter table title");
        QVERIFY2(
            !lognormal_bytes.contains("可靠性分析"),
            "en-US Lognormal PDF must not leak Chinese page title");
        QVERIFY2(
            !lognormal_bytes.contains("百分位寿命"),
            "en-US Lognormal PDF must not leak Chinese percentile table title");
        QVERIFY2(
            !lognormal_bytes.contains("二参数对数正态"),
            "en-US Lognormal PDF must not leak Chinese lognormal hint diagnostic");
    }

    void pdf_reliability_km_long_table_survival_table_spans_pages_en_us()
    {
        // Phase 3 S1 pre-filter: ≥50 KM survival steps → multi-page engineer PDF.
        // Manual cross-page header repeat still required (phase3-cross-page-pdf-manual-acceptance §5 S1).
        // Sample for Qt Creator import: samples/phase0_baselines/reliability_km_long_table.csv
        DataTable table;
        table.columns = {"time", "censor_type", "mode"};
        table.rows.reserve(55);
        for (int i = 1; i <= 55; ++i) {
            table.rows.push_back(
                {std::to_string(i), "exact", "wear"});
        }
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.censoring_type_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.model = "kaplan_meier";
        configuration.reliability.time_unit = "hours";
        OutputPage page = AnalysisService::reliability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.reliability.has_value());
        QVERIFY(page.facts.reliability->failure_count.has_value());
        QCOMPARE(*page.facts.reliability->failure_count, static_cast<std::size_t>(55));
        std::size_t km_table_rows = 0;
        for (const auto& statistic_table : page.tables) {
            if (statistic_table.title.find("Kaplan-Meier") != std::string::npos
                || statistic_table.title.find("生存表") != std::string::npos) {
                km_table_rows = statistic_table.rows.size();
                break;
            }
        }
        QVERIFY2(
            km_table_rows >= 50,
            "long-table fixture must produce ≥50 KM survival table rows");

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        document = localize_report_document(document).document;
        QVERIFY(!document.pages.empty());
        QVERIFY(!document.pages[0].visible_tables.empty());

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("reliability_km_long_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("reliability_km_long_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.size() > 2500,
            "long KM survival table PDF should contain substantial drawn content");
        QVERIFY2(
            bytes.count("/Type /Page") >= 2,
            "long KM survival table should span multiple PDF pages");
        QVERIFY2(
            bytes.contains("Kaplan-Meier survival table"),
            "en-US long-table KM PDF should render localized survival table title");
        QVERIFY2(
            !bytes.contains("Kaplan-Meier 生存表"),
            "en-US long-table KM PDF must not leak Chinese survival table title");
        QVERIFY2(
            !bytes.contains("可靠性分析"),
            "en-US long-table KM PDF must not leak Chinese page title");
    }

    void pdf_audit_km_long_table_evidence_appendix_spans_pages_en_us()
    {
        // Phase 3 S6 pre-filter: audit template + long KM table → multi-page PDF with Evidence appendix.
        // Manual S6 layout sign-off still required (phase3-cross-page-pdf-manual-acceptance §5 S6).
        DataTable table;
        table.columns = {"time", "censor_type", "mode"};
        table.rows.reserve(55);
        for (int i = 1; i <= 55; ++i) {
            table.rows.push_back(
                {std::to_string(i), "exact", "wear"});
        }
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.censoring_type_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.model = "kaplan_meier";
        configuration.reliability.time_unit = "hours";
        OutputPage page = AnalysisService::reliability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.reliability.has_value());

        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        ReportDocument document =
            build_report_document(table, {page}, profile, options);
        document = localize_report_document(document).document;
        QVERIFY(!document.pages.empty());
        QVERIFY(document.pages[0].show_evidence_appendix);
        QVERIFY(!document.pages[0].visible_evidence.empty());
        QVERIFY(!document.pages[0].visible_tables.empty());

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString pdf_path =
            directory.filePath(QStringLiteral("reliability_km_long_audit_en.pdf"));
        ReportExportManifest manifest = build_export_manifest(
            document, make_report_export_paths("reliability_km_long_audit_en.pdf"));
        manifest.consistency_status = "ok";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        const auto exported = export_report_package(pdf_path, document, manifest);
        QVERIFY2(exported.ok, qPrintable(exported.error_message));
        QCOMPARE(
            static_cast<int>(exported.manifest.pdfa_status),
            static_cast<int>(PdfComplianceStatus::not_validated));
        QCOMPARE(
            static_cast<int>(exported.manifest.pdfua_status),
            static_cast<int>(PdfComplianceStatus::unsupported));

        QFile pdf_file(exported.pdf_path);
        QVERIFY(pdf_file.open(QIODevice::ReadOnly));
        const QByteArray bytes = pdf_file.readAll();
        QVERIFY2(
            bytes.count("/Type /Page") >= 2,
            "audit long-table KM PDF should span multiple pages");
        QVERIFY2(
            bytes.contains("Evidence appendix"),
            "audit long-table KM PDF should render localized evidence appendix heading");
        QVERIFY2(
            bytes.contains("Kaplan-Meier survival table"),
            "audit long-table KM PDF should retain localized survival table title");
        QVERIFY2(
            !bytes.contains("证据附录"),
            "en-US audit KM PDF must not leak Chinese evidence appendix heading");
        QVERIFY2(
            !bytes.contains("可靠性分析"),
            "en-US audit KM PDF must not leak Chinese page title");

        QVERIFY(QFileInfo::exists(exported.audit_json_path));
        QFile audit_file(exported.audit_json_path);
        QVERIFY(audit_file.open(QIODevice::ReadOnly));
        const QJsonArray evidence_refs =
            QJsonDocument::fromJson(audit_file.readAll())
                .object()
                .value(QStringLiteral("evidence"))
                .toObject()
                .value(QStringLiteral("evidence"))
                .toArray();
        QVERIFY2(
            evidence_refs.size() >= 1,
            "audit JSON must carry evidence refs for long-table KM export");
        bool saw_label_text_id = false;
        for (const QJsonValue& value : evidence_refs) {
            if (!value.toObject().value(QStringLiteral("label_text_id")).toString().isEmpty()) {
                saw_label_text_id = true;
                break;
            }
        }
        QVERIFY2(
            saw_label_text_id,
            "audit JSON evidence refs must include label_text_id for traceability");
    }

    void pdf_reliability_km_and_weibull_cross_template_table_visibility_and_en_us_locale()
    {
        // Phase 5 REL-2/REL-3: KM + Weibull customer hides statistic tables; engineer/audit keep them.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto assert_cross_template = [&](const DataTable& table,
                                         const OutputPage& page,
                                         const char* page_title_en,
                                         const char* table_marker_en,
                                         const char* customer_pdf_name,
                                         const char* engineer_pdf_name,
                                         const char* audit_pdf_name) {
            auto build_localized = [&](ReportTemplateKind kind) {
                auto profile = make_report_profile(kind);
                profile.locale.language_tag = "en-US";
                ReportDocument document =
                    build_report_document(table, {page}, profile, options);
                document = localize_report_document(document).document;
                return document;
            };

            const ReportDocument customer_doc =
                build_localized(ReportTemplateKind::customer);
            const ReportDocument engineer_doc =
                build_localized(ReportTemplateKind::engineer);
            const ReportDocument audit_doc =
                build_localized(ReportTemplateKind::audit);
            QVERIFY(customer_doc.pages[0].visible_tables.empty());
            QVERIFY(!engineer_doc.pages[0].visible_tables.empty());
            QVERIFY(!audit_doc.pages[0].visible_tables.empty());
            QCOMPARE(
                customer_doc.provenance.facts_hash,
                engineer_doc.provenance.facts_hash);
            QCOMPARE(
                engineer_doc.provenance.facts_hash,
                audit_doc.provenance.facts_hash);

            auto export_bytes = [&](const ReportDocument& document,
                                    const QString& filename) -> QByteArray {
                QTemporaryDir directory;
                QVERIFY2(directory.isValid(), "temp dir required for PDF export");
                const QString pdf_path = directory.filePath(filename);
                ReportExportManifest manifest = build_export_manifest(
                    document, make_report_export_paths(filename.toStdString()));
                manifest.consistency_status = "ok";
                const auto exported = export_report_package(pdf_path, document, manifest);
                QVERIFY2(exported.ok, qPrintable(exported.error_message));
                QFile pdf_file(exported.pdf_path);
                QVERIFY(pdf_file.open(QIODevice::ReadOnly));
                return pdf_file.readAll();
            };

            const QByteArray customer_bytes = export_bytes(
                customer_doc, QString::fromUtf8(customer_pdf_name));
            const QByteArray engineer_bytes = export_bytes(
                engineer_doc, QString::fromUtf8(engineer_pdf_name));
            const QByteArray audit_bytes = export_bytes(
                audit_doc, QString::fromUtf8(audit_pdf_name));

            QVERIFY2(
                customer_bytes.contains(page_title_en),
                "customer PDF should still render localized page title");
            QVERIFY2(
                !customer_bytes.contains(table_marker_en),
                "customer PDF must omit reliability statistic table rows");

            for (const QByteArray& bytes : {engineer_bytes, audit_bytes}) {
                QVERIFY2(
                    bytes.contains(page_title_en),
                    "engineer/audit PDF should render localized page title");
                QVERIFY2(
                    bytes.contains(table_marker_en),
                    "engineer/audit PDF should render localized reliability table rows");
                QVERIFY2(
                    !bytes.contains("可靠性分析"),
                    "en-US reliability PDF must not leak Chinese page title");
            }

            QVERIFY2(
                audit_bytes.contains("Evidence appendix"),
                "audit PDF should include full evidence appendix heading");
            QVERIFY2(
                !engineer_bytes.contains("Evidence appendix"),
                "engineer PDF should not include audit-only evidence appendix");
        };

        DataTable km_table;
        km_table.columns = {"time", "censor_type", "mode"};
        km_table.rows = {
            {"10", "exact", "wear"},
            {"15", "right", ""},
            {"20", "failure", "wear"},
            {"25", "censored", ""},
            {"30", "exact", "wear"}};
        AnalysisConfiguration km_cfg;
        km_cfg.reliability.time_column = 0;
        km_cfg.reliability.censoring_type_column = 1;
        km_cfg.reliability.failure_mode_column = 2;
        km_cfg.reliability.model = "kaplan_meier";
        km_cfg.reliability.time_unit = "hours";
        OutputPage km_page = AnalysisService::reliability(km_table, km_cfg);
        InterpretationService::enrich(km_page);
        QVERIFY(km_page.facts.reliability.has_value());

        assert_cross_template(
            km_table,
            km_page,
            "Reliability analysis",
            "Kaplan-Meier survival table",
            "reliability_km_customer_en.pdf",
            "reliability_km_engineer_en.pdf",
            "reliability_km_audit_en.pdf");

        DataTable weibull_table;
        weibull_table.columns = {"time", "event"};
        weibull_table.rows = {
            {"10", "1"}, {"20", "1"}, {"30", "1"}, {"40", "1"}, {"50", "1"},
            {"25", "0"}, {"35", "0"}};
        AnalysisConfiguration weibull_cfg;
        weibull_cfg.reliability.time_column = 0;
        weibull_cfg.reliability.event_column = 1;
        weibull_cfg.reliability.model = "weibull";
        weibull_cfg.reliability.time_unit = "hours";
        OutputPage weibull_page =
            AnalysisService::reliability(weibull_table, weibull_cfg);
        InterpretationService::enrich(weibull_page);
        QVERIFY(weibull_page.facts.reliability.has_value());

        assert_cross_template(
            weibull_table,
            weibull_page,
            "Reliability analysis",
            "Weibull parameters",
            "reliability_weibull_customer_en.pdf",
            "reliability_weibull_engineer_en.pdf",
            "reliability_weibull_audit_en.pdf");

        AnalysisConfiguration lognormal_cfg = weibull_cfg;
        lognormal_cfg.reliability.model = "lognormal";
        OutputPage lognormal_page =
            AnalysisService::reliability(weibull_table, lognormal_cfg);
        InterpretationService::enrich(lognormal_page);
        QVERIFY(lognormal_page.facts.reliability.has_value());

        assert_cross_template(
            weibull_table,
            lognormal_page,
            "Reliability analysis",
            "Lognormal parameters",
            "reliability_lognormal_customer_en.pdf",
            "reliability_lognormal_engineer_en.pdf",
            "reliability_lognormal_audit_en.pdf");
    }

    void pdf_doe_ccd_and_bbd_design_localize_to_en_us_without_chinese_leak()
    {
        // Phase 4 DOE-1/2/3: CCD + BBD design pages → en-US engineer PDF byte scan.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto export_localized_pdf = [&](const DataTable& table,
                                        const OutputPage& page,
                                        const QString& filename) -> QByteArray {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest = build_export_manifest(
                document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        DataTable empty_table;
        AnalysisConfiguration ccd_cfg;
        ccd_cfg.chart_type = "doe_ccd";
        ccd_cfg.analysis_name = "Central composite design";
        auto& ccd = ccd_cfg.response_surface_design;
        ccd.design_kind = "ccd";
        ccd.ccd_variant = "ccf";
        ccd.factor_ids = {"A", "B"};
        ccd.factor_names = {"Temperature", "Pressure"};
        ccd.factor_units = {"C", "kPa"};
        ccd.low_levels = {60.0, 100.0};
        ccd.high_levels = {80.0, 140.0};
        ccd.centers = {70.0, 120.0};
        ccd.center_point_count = 1;
        ccd.randomize = true;
        ccd.random_seed = 42;
        OutputPage ccd_page =
            AnalysisService::doe_response_surface_design(empty_table, ccd_cfg);
        QVERIFY(ccd_page.facts.design_generation.has_value());
        QCOMPARE(
            QString::fromStdString(ccd_page.facts.design_generation->evidence_type),
            QStringLiteral("formula_reference"));

        const QByteArray ccd_bytes = export_localized_pdf(
            empty_table, ccd_page, QStringLiteral("doe_ccd_en.pdf"));
        QVERIFY2(
            ccd_bytes.contains("Central composite design (CCD)"),
            "en-US CCD PDF should render localized page title");
        QVERIFY2(
            ccd_bytes.contains("Design information"),
            "en-US CCD PDF should render localized design info table");
        QVERIFY2(
            ccd_bytes.contains("Design matrix"),
            "en-US CCD PDF should render localized design matrix table");
        QVERIFY2(
            ccd_bytes.contains("Factor definitions"),
            "en-US CCD PDF should render localized factor definitions table");
        QVERIFY2(
            ccd_bytes.contains("Factor count ="),
            "en-US CCD PDF should render localized factor count caption");
        QVERIFY2(
            !ccd_bytes.contains("中心复合设计"),
            "en-US CCD PDF must not leak Chinese page title");
        QVERIFY2(
            !ccd_bytes.contains("设计矩阵"),
            "en-US CCD PDF must not leak Chinese design matrix title");

        AnalysisConfiguration bbd_cfg;
        bbd_cfg.chart_type = "doe_bbd";
        auto& bbd = bbd_cfg.response_surface_design;
        bbd.design_kind = "bbd";
        bbd.factor_ids = {"X1", "X2", "X3"};
        bbd.factor_names = {"X1", "X2", "X3"};
        bbd.low_levels = {-1.0, -1.0, -1.0};
        bbd.high_levels = {1.0, 1.0, 1.0};
        bbd.centers = {0.0, 0.0, 0.0};
        bbd.center_point_count = 1;
        bbd.randomize = true;
        bbd.random_seed = 7;
        OutputPage bbd_page =
            AnalysisService::doe_response_surface_design(empty_table, bbd_cfg);
        QVERIFY(bbd_page.facts.design_generation.has_value());
        QCOMPARE(
            QString::fromStdString(bbd_page.facts.design_generation->design_kind),
            QStringLiteral("bbd"));

        const QByteArray bbd_bytes = export_localized_pdf(
            empty_table, bbd_page, QStringLiteral("doe_bbd_en.pdf"));
        QVERIFY2(
            bbd_bytes.contains("Box–Behnken design"),
            "en-US BBD PDF should render localized page title");
        QVERIFY2(
            bbd_bytes.contains("BBD omits corners"),
            "en-US BBD PDF should render localized BBD boundary honesty diagnostic");
        QVERIFY2(
            bbd_bytes.contains("Design matrix"),
            "en-US BBD PDF should render localized design matrix table");
        QVERIFY2(
            !bbd_bytes.contains("Box–Behnken 设计"),
            "en-US BBD PDF must not leak Chinese page title");
        QVERIFY2(
            !bbd_bytes.contains("不包含所有因素同时处于极端水平"),
            "en-US BBD PDF must not leak Chinese BBD honesty diagnostic");
    }

    void pdf_doe_ccd_and_bbd_cross_template_table_visibility_and_en_us_locale()
    {
        // Phase 4 DOE-1/2/3: customer hides design statistic tables; engineer/audit keep them.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        DataTable empty_table;

        auto build_ccd_page = [&]() {
            AnalysisConfiguration ccd_cfg;
            ccd_cfg.chart_type = "doe_ccd";
            auto& ccd = ccd_cfg.response_surface_design;
            ccd.design_kind = "ccd";
            ccd.ccd_variant = "ccf";
            ccd.factor_ids = {"A", "B"};
            ccd.factor_names = {"Temperature", "Pressure"};
            ccd.low_levels = {60.0, 100.0};
            ccd.high_levels = {80.0, 140.0};
            ccd.centers = {70.0, 120.0};
            ccd.center_point_count = 1;
            ccd.randomize = true;
            ccd.random_seed = 42;
            OutputPage page =
                AnalysisService::doe_response_surface_design(empty_table, ccd_cfg);
            QVERIFY(page.facts.design_generation.has_value());
            return page;
        };

        auto build_bbd_page = [&]() {
            AnalysisConfiguration bbd_cfg;
            bbd_cfg.chart_type = "doe_bbd";
            auto& bbd = bbd_cfg.response_surface_design;
            bbd.design_kind = "bbd";
            bbd.factor_ids = {"X1", "X2", "X3"};
            bbd.factor_names = {"X1", "X2", "X3"};
            bbd.low_levels = {-1.0, -1.0, -1.0};
            bbd.high_levels = {1.0, 1.0, 1.0};
            bbd.centers = {0.0, 0.0, 0.0};
            bbd.center_point_count = 1;
            bbd.randomize = true;
            bbd.random_seed = 7;
            OutputPage page =
                AnalysisService::doe_response_surface_design(empty_table, bbd_cfg);
            QVERIFY(page.facts.design_generation.has_value());
            return page;
        };

        auto assert_cross_template = [&](const OutputPage& page,
                                         const char* page_title_en,
                                         const char* customer_pdf_name,
                                         const char* engineer_pdf_name,
                                         const char* audit_pdf_name) {
            auto build_localized = [&](ReportTemplateKind kind) {
                auto profile = make_report_profile(kind);
                profile.locale.language_tag = "en-US";
                ReportDocument document =
                    build_report_document(empty_table, {page}, profile, options);
                document = localize_report_document(document).document;
                return document;
            };

            const ReportDocument customer_doc =
                build_localized(ReportTemplateKind::customer);
            const ReportDocument engineer_doc =
                build_localized(ReportTemplateKind::engineer);
            const ReportDocument audit_doc =
                build_localized(ReportTemplateKind::audit);
            QVERIFY(customer_doc.pages[0].visible_tables.empty());
            QVERIFY(!engineer_doc.pages[0].visible_tables.empty());
            QVERIFY(!audit_doc.pages[0].visible_tables.empty());
            QCOMPARE(
                customer_doc.provenance.facts_hash,
                engineer_doc.provenance.facts_hash);
            QCOMPARE(
                engineer_doc.provenance.facts_hash,
                audit_doc.provenance.facts_hash);

            auto export_bytes = [&](const ReportDocument& document,
                                    const QString& filename) -> QByteArray {
                QTemporaryDir directory;
                QVERIFY2(directory.isValid(), "temp dir required for PDF export");
                const QString pdf_path = directory.filePath(filename);
                ReportExportManifest manifest = build_export_manifest(
                    document, make_report_export_paths(filename.toStdString()));
                manifest.consistency_status = "ok";
                const auto exported = export_report_package(pdf_path, document, manifest);
                QVERIFY2(exported.ok, qPrintable(exported.error_message));
                QFile pdf_file(exported.pdf_path);
                QVERIFY(pdf_file.open(QIODevice::ReadOnly));
                return pdf_file.readAll();
            };

            const QByteArray customer_bytes = export_bytes(
                customer_doc, QString::fromUtf8(customer_pdf_name));
            const QByteArray engineer_bytes = export_bytes(
                engineer_doc, QString::fromUtf8(engineer_pdf_name));
            const QByteArray audit_bytes = export_bytes(
                audit_doc, QString::fromUtf8(audit_pdf_name));

            QVERIFY2(
                customer_bytes.contains(page_title_en),
                "customer PDF should still render localized page title");
            QVERIFY2(
                !customer_bytes.contains("Design matrix"),
                "customer PDF must omit DOE design statistic tables");
            QVERIFY2(
                !customer_bytes.contains("Design information"),
                "customer PDF must omit DOE design information table");

            for (const QByteArray& bytes : {engineer_bytes, audit_bytes}) {
                QVERIFY2(
                    bytes.contains(page_title_en),
                    "engineer/audit PDF should render localized page title");
                QVERIFY2(
                    bytes.contains("Design matrix"),
                    "engineer/audit PDF should render localized design matrix table");
                QVERIFY2(
                    !bytes.contains("设计矩阵"),
                    "en-US DOE PDF must not leak Chinese design matrix title");
            }

            QVERIFY2(
                audit_bytes.contains("Evidence appendix"),
                "audit PDF should include full evidence appendix heading");
            QVERIFY2(
                !engineer_bytes.contains("Evidence appendix"),
                "engineer PDF should not include audit-only evidence appendix");
        };

        assert_cross_template(
            build_ccd_page(),
            "Central composite design (CCD)",
            "doe_ccd_customer_en.pdf",
            "doe_ccd_engineer_en.pdf",
            "doe_ccd_audit_en.pdf");
        assert_cross_template(
            build_bbd_page(),
            "Box–Behnken design",
            "doe_bbd_customer_en.pdf",
            "doe_bbd_engineer_en.pdf",
            "doe_bbd_audit_en.pdf");
    }

    void pdf_warranty_strata_three_templates_manifest_facts_hash_match_en_us()
    {
        // Phase 3 S4 pre-filter: customer/engineer/audit exported manifests share facts_hash.
        DataTable table;
        table.columns = {"time", "censor_type", "mode", "exposure"};
        table.rows = {
            {"10", "exact", "wear", "10"},
            {"12", "exact", "wear", "10"},
            {"15", "right", "wear", "20"},
            {"8", "exact", "early", "5"},
            {"30", "right", "", "5"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.reliability_at_warranty = 0.95;
        configuration.reliability.censoring_type_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.exposure_column = 3;
        configuration.reliability.model = "weibull";
        OutputPage page =
            AnalysisService::reliability_warranty(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.warranty.has_value());
        QVERIFY(page.facts.warranty->strata.size() >= 2);

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        std::string reference_facts_hash;
        for (ReportTemplateKind kind :
             {ReportTemplateKind::customer,
              ReportTemplateKind::engineer,
              ReportTemplateKind::audit}) {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            document = localize_report_document(document).document;
            QVERIFY(!document.pages.empty());

            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for manifest export");
            const QString pdf_path = directory.filePath(
                QStringLiteral("warranty_strata_%1_en.pdf")
                    .arg(QString::fromStdString(profile.profile_id)));
            ReportExportManifest manifest = build_export_manifest(
                document,
                make_report_export_paths(pdf_path.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QVERIFY(QFileInfo::exists(exported.manifest_path));

            QFile manifest_file(exported.manifest_path);
            QVERIFY(manifest_file.open(QIODevice::ReadOnly));
            const ReportExportManifest loaded = report_export_manifest_from_json(
                QJsonDocument::fromJson(manifest_file.readAll()).object());
            QCOMPARE(loaded.consistency_status, std::string("ok"));
            QCOMPARE(loaded.locale_language_tag, std::string("en-US"));
            QVERIFY(!loaded.facts_hash.empty());
            if (reference_facts_hash.empty()) {
                reference_facts_hash = loaded.facts_hash;
            } else {
                QCOMPARE(loaded.facts_hash, reference_facts_hash);
            }
            QCOMPARE(loaded.facts_hash, document.provenance.facts_hash);
        }
    }

    void pdf_doe_ccd_k4_long_design_matrix_spans_pages_en_us()
    {
        // Phase 3 S2 path B pre-filter: CCD k=4 + many center points → multi-page engineer PDF.
        // Manual cross-page header repeat still required (phase3-cross-page-pdf-manual-acceptance §5 S2).
        DataTable empty_table;
        AnalysisConfiguration configuration;
        configuration.chart_type = "doe_ccd";
        auto& ccd = configuration.response_surface_design;
        ccd.design_kind = "ccd";
        ccd.ccd_variant = "ccf";
        ccd.factor_ids = {"F1", "F2", "F3", "F4"};
        ccd.factor_names = {"F1", "F2", "F3", "F4"};
        ccd.low_levels = {10.0, 20.0, 30.0, 40.0};
        ccd.high_levels = {20.0, 30.0, 40.0, 50.0};
        ccd.centers = {15.0, 25.0, 35.0, 45.0};
        ccd.center_point_count = 30;
        ccd.randomize = true;
        ccd.random_seed = 11;

        OutputPage page =
            AnalysisService::doe_response_surface_design(empty_table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.design_generation.has_value());
        QCOMPARE(
            page.facts.design_generation->factor_count,
            static_cast<std::size_t>(4));
        QVERIFY2(
            page.facts.design_generation->run_count >= 50,
            "CCD k=4 with 30 center points must produce ≥50 design runs");

        std::size_t design_matrix_rows = 0;
        for (const auto& statistic_table : page.tables) {
            if (statistic_table.title == "设计矩阵") {
                design_matrix_rows = statistic_table.rows.size();
                break;
            }
        }
        QVERIFY2(
            design_matrix_rows >= 50,
            "long CCD fixture must expose ≥50 design-matrix table rows");

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        auto build_localized = [&](const char* language_tag) {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(empty_table, {page}, profile, options);
            document = localize_report_document(document).document;
            return document;
        };

        auto export_bytes = [&](const ReportDocument& document,
                                const QString& filename) -> QByteArray {
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for PDF export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest = build_export_manifest(
                document, make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            return pdf_file.readAll();
        };

        const ReportDocument doc_en = build_localized("en-US");
        QVERIFY(!doc_en.pages.empty());
        QVERIFY(!doc_en.pages[0].visible_tables.empty());

        const QByteArray en_bytes =
            export_bytes(doc_en, QStringLiteral("doe_ccd_k4_long_en.pdf"));
        QVERIFY2(
            en_bytes.size() > 2500,
            "long DOE design matrix PDF should contain substantial drawn content");
        QVERIFY2(
            en_bytes.count("/Type /Page") >= 2,
            "long DOE design matrix should span multiple PDF pages");
        QVERIFY2(
            en_bytes.contains("Design matrix"),
            "en-US long DOE PDF should render localized design matrix title");
        QVERIFY2(
            en_bytes.contains("Central composite design (CCD)"),
            "en-US long DOE PDF should render localized page title");
        QVERIFY2(
            !en_bytes.contains("设计矩阵"),
            "en-US long DOE PDF must not leak Chinese design matrix title");
        QVERIFY2(
            !en_bytes.contains("中心复合设计"),
            "en-US long DOE PDF must not leak Chinese page title");

        const ReportDocument doc_zh = build_localized("zh-CN");
        QVERIFY(!doc_zh.pages.empty());
        QVERIFY(!doc_zh.pages[0].visible_tables.empty());
        QCOMPARE(doc_en.provenance.facts_hash, doc_zh.provenance.facts_hash);
        QCOMPARE(
            doc_zh.pages[0].source_page.title,
            std::string("中心复合设计 (CCD)"));
        assert_visible_layer_no_english_catalog_leaks(
            doc_zh.pages[0],
            "DOE CCD k=4 long zh-CN",
            {"Central composite design (CCD)",
             "Design matrix",
             "Design information",
             "Evidence appendix"});

        const QByteArray zh_bytes =
            export_bytes(doc_zh, QStringLiteral("doe_ccd_k4_long_zh.pdf"));
        QVERIFY2(
            zh_bytes.count("/Type /Page") >= 2,
            "long zh-CN DOE design matrix should span multiple PDF pages");
        QVERIFY2(
            zh_bytes.contains("设计矩阵"),
            "zh-CN long DOE PDF should render localized design matrix title");
        QVERIFY2(
            zh_bytes.contains("中心复合设计"),
            "zh-CN long DOE PDF should render localized page title");
        QVERIFY2(
            !zh_bytes.contains("Design matrix"),
            "zh-CN long DOE PDF must not leak English design matrix title");
        QVERIFY2(
            !zh_bytes.contains("Central composite design (CCD)"),
            "zh-CN long DOE PDF must not leak English page title");
    }

    void audit_json_doe_design_generation_carries_label_text_ids()
    {
        // Phase 4 EvidenceBundle: DOE design gates + formula ref in audit PDF + JSON.
        DataTable empty_table;
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";
        qputenv("DATALAB_VERAPDF", QByteArray());
        qputenv("DATALAB_PAC", QByteArray());

        struct AuditExportArtifacts {
            QJsonArray evidence_refs;
            QByteArray pdf_bytes;
            ReportDocument document;
        };

        auto export_audit = [&](const OutputPage& page,
                                const QString& filename) -> AuditExportArtifacts {
            AuditExportArtifacts artifacts;
            auto profile = make_report_profile(ReportTemplateKind::audit);
            profile.locale.language_tag = "en-US";
            artifacts.document =
                build_report_document(empty_table, {page}, profile, options);
            artifacts.document = localize_report_document(artifacts.document).document;
            QVERIFY(!artifacts.document.pages.empty());
            QTemporaryDir directory;
            QVERIFY2(directory.isValid(), "temp dir required for audit export");
            const QString pdf_path = directory.filePath(filename);
            ReportExportManifest manifest = build_export_manifest(
                artifacts.document,
                make_report_export_paths(filename.toStdString()));
            manifest.consistency_status = "ok";
            const auto exported = export_report_package(pdf_path, artifacts.document, manifest);
            QVERIFY2(exported.ok, qPrintable(exported.error_message));
            QVERIFY(QFileInfo::exists(exported.audit_json_path));
            QFile audit_file(exported.audit_json_path);
            QVERIFY(audit_file.open(QIODevice::ReadOnly));
            artifacts.evidence_refs =
                QJsonDocument::fromJson(audit_file.readAll())
                    .object()
                    .value(QStringLiteral("evidence"))
                    .toObject()
                    .value(QStringLiteral("evidence"))
                    .toArray();
            QFile pdf_file(exported.pdf_path);
            QVERIFY(pdf_file.open(QIODevice::ReadOnly));
            artifacts.pdf_bytes = pdf_file.readAll();
            return artifacts;
        };

        AnalysisConfiguration ccd_cfg;
        ccd_cfg.chart_type = "doe_ccd";
        auto& ccd = ccd_cfg.response_surface_design;
        ccd.design_kind = "ccd";
        ccd.ccd_variant = "ccf";
        ccd.factor_ids = {"A", "B"};
        ccd.factor_names = {"Temperature", "Pressure"};
        ccd.low_levels = {60.0, 100.0};
        ccd.high_levels = {80.0, 140.0};
        ccd.centers = {70.0, 120.0};
        ccd.center_point_count = 1;
        ccd.randomize = true;
        ccd.random_seed = 42;
        const OutputPage ccd_page =
            AnalysisService::doe_response_surface_design(empty_table, ccd_cfg);
        QVERIFY(ccd_page.facts.design_generation.has_value());

        const AuditExportArtifacts ccd_export =
            export_audit(ccd_page, QStringLiteral("doe_ccd_audit.pdf"));
        bool saw_ccd_gate_evidence = false;
        bool saw_ccd_formula_evidence = false;
        for (const auto& ref : ccd_export.document.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:design_formula_reference_only")
                    != std::string::npos) {
                saw_ccd_gate_evidence = true;
                QCOMPARE(
                    ref.label_text_id,
                    std::string("evidence.design_formula_reference_only"));
                QCOMPARE(ref.diagnostic_code, std::string("design_formula_reference_only"));
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
            }
            if (ref.evidence_id.find(":formula:design_generation") != std::string::npos) {
                saw_ccd_formula_evidence = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.formula_reference"));
            }
        }
        QVERIFY2(
            saw_ccd_gate_evidence,
            "audit evidence bundle must include CCD design_formula limiting gate ref");
        QVERIFY2(
            saw_ccd_formula_evidence,
            "audit evidence bundle must include CCD design_generation formula ref");

        QVERIFY2(
            ccd_export.pdf_bytes.size() > 800,
            "CCD audit PDF should contain drawn content");
        QVERIFY2(
            ccd_export.pdf_bytes.contains("Central composite design (CCD)"),
            "en-US CCD audit PDF should render localized page title");
        QVERIFY2(
            ccd_export.pdf_bytes.contains(
                "DOE design generation is formula_reference only"),
            "audit PDF evidence appendix should render localized design_formula gate label");
        QVERIFY2(
            ccd_export.pdf_bytes.contains("Formula reference (not vendor_oracle)"),
            "audit PDF evidence appendix should render localized formula ref label");
        QVERIFY2(
            !ccd_export.pdf_bytes.contains("中心复合设计"),
            "en-US CCD audit PDF must not leak Chinese page title");
        QVERIFY2(
            !ccd_export.pdf_bytes.contains("DOE 设计生成仅为 formula_reference"),
            "en-US CCD audit PDF must not leak Chinese design_formula gate label");

        bool saw_design_gate = false;
        bool saw_design_formula = false;
        for (const QJsonValue& value : ccd_export.evidence_refs) {
            const QJsonObject ref = value.toObject();
            const std::string label_text_id =
                ref.value(QStringLiteral("label_text_id")).toString().toStdString();
            const std::string evidence_id =
                ref.value(QStringLiteral("evidence_id")).toString().toStdString();
            if (label_text_id == "evidence.design_formula_reference_only") {
                saw_design_gate = true;
                QCOMPARE(
                    ref.value(QStringLiteral("diagnostic_code")).toString(),
                    QStringLiteral("design_formula_reference_only"));
                const std::string label =
                    resolve_report_text(label_text_id, "en-US").text;
                QVERIFY2(
                    label.find("formula_reference only") != std::string::npos,
                    label.c_str());
                QVERIFY2(label.find("vendor_oracle") != std::string::npos, label.c_str());
            }
            if (evidence_id.find(":formula:design_generation") != std::string::npos) {
                saw_design_formula = true;
                QCOMPARE(label_text_id, std::string("evidence.formula_reference"));
            }
        }
        QVERIFY2(saw_design_gate, "CCD audit JSON must include design_formula gate");
        QVERIFY2(
            saw_design_formula,
            "CCD audit JSON must include design_generation formula ref");

        AnalysisConfiguration bbd_cfg;
        bbd_cfg.chart_type = "doe_bbd";
        auto& bbd = bbd_cfg.response_surface_design;
        bbd.design_kind = "bbd";
        bbd.factor_ids = {"X1", "X2", "X3"};
        bbd.low_levels = {-1.0, -1.0, -1.0};
        bbd.high_levels = {1.0, 1.0, 1.0};
        bbd.centers = {0.0, 0.0, 0.0};
        bbd.center_point_count = 1;
        bbd.randomize = true;
        bbd.random_seed = 7;
        const OutputPage bbd_page =
            AnalysisService::doe_response_surface_design(empty_table, bbd_cfg);
        QVERIFY(bbd_page.facts.design_generation.has_value());

        const AuditExportArtifacts bbd_export =
            export_audit(bbd_page, QStringLiteral("doe_bbd_audit.pdf"));
        bool saw_bbd_gate_evidence = false;
        for (const auto& ref : bbd_export.document.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:bbd_no_corners") == std::string::npos) {
                continue;
            }
            saw_bbd_gate_evidence = true;
            QCOMPARE(ref.label_text_id, std::string("evidence.bbd_no_corners"));
            QCOMPARE(ref.diagnostic_code, std::string("bbd_no_corners"));
            QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
        }
        QVERIFY2(
            saw_bbd_gate_evidence,
            "audit evidence bundle must include BBD bbd_no_corners limiting gate ref");

        QVERIFY2(
            bbd_export.pdf_bytes.size() > 800,
            "BBD audit PDF should contain drawn content");
        QVERIFY2(
            bbd_export.pdf_bytes.contains("Box–Behnken design"),
            "en-US BBD audit PDF should render localized page title");
        QVERIFY2(
            bbd_export.pdf_bytes.contains("BBD has no corner points"),
            "audit PDF evidence appendix should render localized BBD gate label");
        QVERIFY2(
            bbd_export.pdf_bytes.contains(
                "DOE design generation is formula_reference only"),
            "audit PDF evidence appendix should render localized design_formula gate label");
        QVERIFY2(
            !bbd_export.pdf_bytes.contains("Box–Behnken 设计"),
            "en-US BBD audit PDF must not leak Chinese page title");
        QVERIFY2(
            !bbd_export.pdf_bytes.contains("BBD 无角点"),
            "en-US BBD audit PDF must not leak Chinese BBD gate label");

        bool saw_bbd_gate = false;
        for (const QJsonValue& value : bbd_export.evidence_refs) {
            const QJsonObject ref = value.toObject();
            if (ref.value(QStringLiteral("label_text_id")).toString()
                    != QStringLiteral("evidence.bbd_no_corners")) {
                continue;
            }
            saw_bbd_gate = true;
            QCOMPARE(
                ref.value(QStringLiteral("diagnostic_code")).toString(),
                QStringLiteral("bbd_no_corners"));
            const std::string label = resolve_report_text(
                "evidence.bbd_no_corners", "en-US").text;
            QVERIFY2(
                label.find("BBD has no corner points") != std::string::npos,
                label.c_str());
            QVERIFY2(
                label.find("domain-wide") != std::string::npos
                    || label.find("domain wide") != std::string::npos,
                label.c_str());
        }
        QVERIFY2(saw_bbd_gate, "BBD audit JSON must include bbd_no_corners gate");
    }

    void representative_vertical_slice_reports_localize_without_cross_language_leak()
    {
        // Phase 3: thirteen representative algorithm slices → en-US/zh-CN visible layer mixing guard.
        // Engineer profile only; companion three-template × bilingual guards (customer/engineer/audit):
        //   CCD/BBD/RSM LOF, KM/Weibull/Lognormal, warranty summary/strata/exposure,
        //   Graph scatter/bar/hexbin/density faceted, normal/nonnormal/Johnson/Box-Cox — see representative_*_three_report_profiles_* tests.
        // Companion pdf_*_cross_template_* (13/13) + S7 pdf_pinlength_capability_unicode_columns_*.
        // Scans document chrome (titles/headers/diagnostics/interp), not user worksheet values.
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        auto localize_slice = [&](const DataTable& table,
                                  const OutputPage& page,
                                  const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        DataTable empty_table;
        AnalysisConfiguration ccd_cfg;
        ccd_cfg.chart_type = "doe_ccd";
        ccd_cfg.response_surface_design.design_kind = "ccd";
        ccd_cfg.response_surface_design.ccd_variant = "ccf";
        ccd_cfg.response_surface_design.factor_ids = {"A", "B"};
        ccd_cfg.response_surface_design.low_levels = {60.0, 100.0};
        ccd_cfg.response_surface_design.high_levels = {80.0, 140.0};
        ccd_cfg.response_surface_design.centers = {70.0, 120.0};
        ccd_cfg.response_surface_design.center_point_count = 1;
        const OutputPage ccd_page =
            AnalysisService::doe_response_surface_design(empty_table, ccd_cfg);
        const ReportDocument ccd_en =
            localize_slice(empty_table, ccd_page, "en-US");
        QVERIFY(!ccd_en.pages.empty());
        assert_visible_layer_no_cjk(ccd_en.pages[0], "DOE CCD");
        assert_visible_layer_excludes_substrings(
            ccd_en.pages[0],
            "DOE CCD",
            {"中心复合设计", "DOE 设计生成仅为"});
        const ReportDocument ccd_zh =
            localize_slice(empty_table, ccd_page, "zh-CN");
        QVERIFY(!ccd_zh.pages.empty());
        assert_visible_layer_no_english_catalog_leaks(
            ccd_zh.pages[0],
            "DOE CCD",
            {"Central composite design (CCD)",
             "Formula reference (not vendor_oracle)",
             "DOE design generation is formula_reference only"});

        AnalysisConfiguration bbd_cfg;
        bbd_cfg.chart_type = "doe_bbd";
        auto& bbd = bbd_cfg.response_surface_design;
        bbd.design_kind = "bbd";
        bbd.factor_ids = {"X1", "X2", "X3"};
        bbd.low_levels = {-1.0, -1.0, -1.0};
        bbd.high_levels = {1.0, 1.0, 1.0};
        bbd.centers = {0.0, 0.0, 0.0};
        bbd.center_point_count = 1;
        bbd.randomize = true;
        bbd.random_seed = 7;
        const OutputPage bbd_page =
            AnalysisService::doe_response_surface_design(empty_table, bbd_cfg);
        QVERIFY(bbd_page.facts.design_generation.has_value());
        const ReportDocument bbd_en =
            localize_slice(empty_table, bbd_page, "en-US");
        QVERIFY(!bbd_en.pages.empty());
        QCOMPARE(
            bbd_en.pages[0].source_page.title,
            std::string("Box–Behnken design"));
        assert_visible_layer_no_cjk(bbd_en.pages[0], "DOE BBD");
        assert_visible_layer_excludes_substrings(
            bbd_en.pages[0],
            "DOE BBD",
            {"Box–Behnken 设计", "不包含所有因素", "BBD 无角点", "设计矩阵"});
        bool saw_bbd_no_corners_en = false;
        for (const auto& diagnostic : bbd_en.pages[0].visible_diagnostics) {
            if (diagnostic.code == "bbd_no_corners") {
                saw_bbd_no_corners_en = true;
                QVERIFY2(
                    diagnostic.message.find("BBD omits corners") != std::string::npos
                        || diagnostic.message.find("design-space boundary")
                            != std::string::npos,
                    "en-US BBD no-corners diagnostic must localize to English");
            }
        }
        QVERIFY2(
            saw_bbd_no_corners_en,
            "en-US BBD visible layer should include bbd_no_corners honesty diagnostic");
        const ReportDocument bbd_zh =
            localize_slice(empty_table, bbd_page, "zh-CN");
        QVERIFY(!bbd_zh.pages.empty());
        QCOMPARE(
            bbd_zh.pages[0].source_page.title,
            std::string("Box–Behnken 设计"));
        assert_visible_layer_no_english_catalog_leaks(
            bbd_zh.pages[0],
            "DOE BBD",
            {"Box–Behnken design",
             "BBD omits corners",
             "BBD has no corner points",
             "Design matrix"});

        DataTable graph_table;
        graph_table.columns = {"Time", "X", "Y", "Group"};
        graph_table.rows = {
            {"1", "1", "2", "A"},
            {"2", "2", "3", "A"},
            {"3", "3", "1", "B"},
            {"4", "4", "0", "B"},
        };
        AnalysisConfiguration scatter_cfg;
        scatter_cfg.graph.graph_kind = "scatter";
        scatter_cfg.graph.x_column = 1;
        scatter_cfg.graph.y_column = 2;
        scatter_cfg.graph.facet_column = 3;
        scatter_cfg.graph.facet_max_panels = 2;
        scatter_cfg.hidden_rows = {1};
        const OutputPage scatter_page = GraphService::run(graph_table, scatter_cfg);
        const ReportDocument scatter_en =
            localize_slice(graph_table, scatter_page, "en-US");
        QVERIFY(!scatter_en.pages.empty());
        assert_visible_layer_no_cjk(scatter_en.pages[0], "Graph scatter faceted");
        assert_visible_layer_excludes_substrings(
            scatter_en.pages[0],
            "Graph scatter faceted",
            {"显示 N", "分析 N", "分面 ="});
        QVERIFY2(
            scatter_en.pages[0].source_page.parameter_summary.find("Display N =")
                != std::string::npos,
            "13-slice scatter guard should carry localized display N with hidden row");
        const ReportDocument scatter_zh =
            localize_slice(graph_table, scatter_page, "zh-CN");
        QVERIFY(!scatter_zh.pages.empty());
        QCOMPARE(
            scatter_zh.pages[0].source_page.title,
            std::string("散点图（分面）"));
        assert_visible_layer_no_english_catalog_leaks(
            scatter_zh.pages[0],
            "Graph scatter faceted",
            {"Display N =", "Analysis N =", "Scatterplot (faceted)", "Facet ="});

        DataTable km_table;
        km_table.columns = {"time", "censor_type", "mode"};
        km_table.rows = {
            {"10", "exact", "wear"},
            {"15", "right", ""},
            {"20", "failure", "wear"},
            {"25", "censored", ""},
            {"30", "exact", "wear"}};
        AnalysisConfiguration km_cfg;
        km_cfg.reliability.time_column = 0;
        km_cfg.reliability.censoring_type_column = 1;
        km_cfg.reliability.failure_mode_column = 2;
        km_cfg.reliability.model = "kaplan_meier";
        km_cfg.reliability.time_unit = "hours";
        OutputPage km_page = AnalysisService::reliability(km_table, km_cfg);
        InterpretationService::enrich(km_page);
        const ReportDocument km_en = localize_slice(km_table, km_page, "en-US");
        QVERIFY(!km_en.pages.empty());
        assert_visible_layer_no_cjk(km_en.pages[0], "Reliability KM");
        assert_visible_layer_excludes_substrings(
            km_en.pages[0],
            "Reliability KM",
            {"生存表", "删失类型", "可靠性分析", "Kaplan-Meier 生存"});
        bool saw_km_survival_table_en = false;
        for (const auto& table_view : km_en.pages[0].visible_tables) {
            if (table_view.title.find("Kaplan-Meier survival table") != std::string::npos) {
                saw_km_survival_table_en = true;
            }
        }
        QVERIFY2(
            saw_km_survival_table_en,
            "en-US KM visible layer should expose localized survival table title");
        const ReportDocument km_zh = localize_slice(km_table, km_page, "zh-CN");
        QVERIFY(!km_zh.pages.empty());
        QCOMPARE(
            km_zh.pages[0].source_page.title,
            std::string("可靠性分析"));
        assert_visible_layer_no_english_catalog_leaks(
            km_zh.pages[0],
            "Reliability KM",
            {"Kaplan-Meier survival table", "Lifetime column =", "Reliability analysis"});

        AnalysisConfiguration warranty_cfg;
        warranty_cfg.chart_type = "reliability_warranty";
        warranty_cfg.reliability.warranty_time = 1000.0;
        warranty_cfg.reliability.time_unit = "hours";
        warranty_cfg.reliability.exposure = 1000.0;
        warranty_cfg.reliability.reliability_at_warranty = 0.95;
        warranty_cfg.reliability.model = "weibull";
        warranty_cfg.reliability.warranty_observed_failures = 2;
        warranty_cfg.reliability.warranty_censored_count = 1;
        warranty_cfg.reliability.warranty_valid_count = 10;
        const OutputPage warranty_page =
            AnalysisService::reliability_warranty(empty_table, warranty_cfg);
        const ReportDocument warranty_en =
            localize_slice(empty_table, warranty_page, "en-US");
        QVERIFY(!warranty_en.pages.empty());
        assert_visible_layer_no_cjk(warranty_en.pages[0], "Warranty summary");
        assert_visible_layer_excludes_substrings(
            warranty_en.pages[0],
            "Warranty summary",
            {"保修摘要", "暴露量", "claims/1000 = 1000"});
        bool saw_warranty_table_en = false;
        for (const auto& table_view : warranty_en.pages[0].visible_tables) {
            if (table_view.title.find("Warranty summary") != std::string::npos) {
                saw_warranty_table_en = true;
            }
        }
        QVERIFY2(
            saw_warranty_table_en,
            "en-US warranty visible layer should expose localized summary table title");
        const ReportDocument warranty_zh =
            localize_slice(empty_table, warranty_page, "zh-CN");
        QVERIFY(!warranty_zh.pages.empty());
        QCOMPARE(
            warranty_zh.pages[0].source_page.title,
            std::string("保修摘要"));
        assert_visible_layer_no_english_catalog_leaks(
            warranty_zh.pages[0],
            "Warranty summary",
            {"Warranty summary", "Claims per 1000", "Exposure ="});

        DataTable warranty_strata_table;
        warranty_strata_table.columns = {"time", "censor_type", "mode", "exposure"};
        warranty_strata_table.rows = {
            {"10", "exact", "wear", "10"},
            {"12", "exact", "wear", "10"},
            {"15", "right", "wear", "20"},
            {"8", "exact", "early", "5"},
            {"30", "right", "", "5"}};
        AnalysisConfiguration warranty_strata_cfg;
        warranty_strata_cfg.chart_type = "reliability_warranty";
        warranty_strata_cfg.reliability.warranty_time = 1000.0;
        warranty_strata_cfg.reliability.time_unit = "hours";
        warranty_strata_cfg.reliability.reliability_at_warranty = 0.95;
        warranty_strata_cfg.reliability.censoring_type_column = 1;
        warranty_strata_cfg.reliability.failure_mode_column = 2;
        warranty_strata_cfg.reliability.exposure_column = 3;
        warranty_strata_cfg.reliability.model = "weibull";
        const OutputPage warranty_strata_page =
            AnalysisService::reliability_warranty(warranty_strata_table, warranty_strata_cfg);
        QVERIFY(warranty_strata_page.facts.warranty.has_value());
        QCOMPARE(
            QString::fromStdString(warranty_strata_page.facts.warranty->stratum_kind),
            QStringLiteral("failure_mode"));
        QVERIFY(warranty_strata_page.facts.warranty->strata.size() >= 2);
        const ReportDocument warranty_strata_en =
            localize_slice(warranty_strata_table, warranty_strata_page, "en-US");
        QVERIFY(!warranty_strata_en.pages.empty());
        assert_visible_layer_no_cjk(warranty_strata_en.pages[0], "Warranty strata");
        assert_visible_layer_excludes_substrings(
            warranty_strata_en.pages[0],
            "Warranty strata",
            {"保修摘要", "失效模式分母追溯", "暴露量"});
        bool saw_stratum_table_en = false;
        bool saw_warranty_summary_en = false;
        for (const auto& table_view : warranty_strata_en.pages[0].visible_tables) {
            if (table_view.title.find("Failure-mode denominator trace") != std::string::npos) {
                saw_stratum_table_en = true;
            }
            if (table_view.title.find("Warranty summary") != std::string::npos) {
                saw_warranty_summary_en = true;
            }
        }
        QVERIFY2(
            saw_stratum_table_en,
            "en-US warranty strata visible layer should expose localized stratum table title");
        QVERIFY2(
            saw_warranty_summary_en,
            "en-US warranty strata visible layer should expose localized summary table title");
        const ReportDocument warranty_strata_zh =
            localize_slice(warranty_strata_table, warranty_strata_page, "zh-CN");
        QVERIFY(!warranty_strata_zh.pages.empty());
        QCOMPARE(
            warranty_strata_zh.pages[0].source_page.title,
            std::string("保修摘要"));
        bool saw_stratum_table_zh = false;
        for (const auto& table_view : warranty_strata_zh.pages[0].visible_tables) {
            if (table_view.title.find("失效模式分母追溯") != std::string::npos) {
                saw_stratum_table_zh = true;
            }
        }
        QVERIFY2(
            saw_stratum_table_zh,
            "zh-CN warranty strata visible layer should expose Chinese stratum table title");
        assert_visible_layer_no_english_catalog_leaks(
            warranty_strata_zh.pages[0],
            "Warranty strata",
            {"Failure-mode denominator trace",
             "Warranty summary",
             "Claims per 1000",
             "Failure modes (observed exact failures)"});

        DataTable box_cox_table;
        box_cox_table.columns = {"Y"};
        box_cox_table.rows = {{"1.0"}, {"2.0"}, {"4.0"}, {"8.0"}, {"16.0"}};
        AnalysisConfiguration box_cox_cfg;
        box_cox_cfg.chart_type = "box_cox";
        box_cox_cfg.variable_columns = {0};
        OutputPage box_cox_page = AnalysisService::box_cox(box_cox_table, box_cox_cfg);
        InterpretationService::enrich(box_cox_page);
        const ReportDocument box_cox_en =
            localize_slice(box_cox_table, box_cox_page, "en-US");
        QVERIFY(!box_cox_en.pages.empty());
        assert_visible_layer_no_cjk(box_cox_en.pages[0], "Box-Cox");
        const ReportDocument box_cox_zh =
            localize_slice(box_cox_table, box_cox_page, "zh-CN");
        QVERIFY(!box_cox_zh.pages.empty());
        QCOMPARE(
            box_cox_zh.pages[0].source_page.title,
            std::string("Box-Cox 变换"));
        assert_visible_layer_no_english_catalog_leaks(box_cox_zh.pages[0], "Box-Cox");

        AnalysisConfiguration box_cox_bad_cfg = box_cox_cfg;
        box_cox_bad_cfg.specifications.lower = -1.0;
        box_cox_bad_cfg.specifications.upper = 10.0;
        OutputPage box_cox_bad_page =
            AnalysisService::box_cox(box_cox_table, box_cox_bad_cfg);
        const ReportDocument box_cox_bad_en =
            localize_slice(box_cox_table, box_cox_bad_page, "en-US");
        QVERIFY(!box_cox_bad_en.pages.empty());
        assert_visible_layer_no_cjk(box_cox_bad_en.pages[0], "Box-Cox invalid spec");
        bool saw_invalid_spec_en = false;
        for (const auto& diagnostic : box_cox_bad_en.pages[0].visible_diagnostics) {
            if (diagnostic.code == "box_cox_invalid_spec_limit") {
                saw_invalid_spec_en = true;
                QVERIFY2(
                    diagnostic.message.find("Lower spec limit") != std::string::npos,
                    "en-US invalid spec diagnostic must localize");
            }
        }
        QVERIFY2(
            saw_invalid_spec_en,
            "Box-Cox invalid spec slice must surface localized gate diagnostic");
        for (const auto& table_view : box_cox_bad_en.pages[0].visible_tables) {
            QVERIFY2(
                table_view.title.find("Capability after transform") == std::string::npos,
                "invalid spec gate must omit capability table from visible layer");
        }

        std::vector<double> johnson_values;
        for (int i = 1; i <= 40; ++i) {
            johnson_values.push_back(std::exp(0.05 * static_cast<double>(i))
                + 0.1 * static_cast<double>(i % 3));
        }
        DataTable johnson_table;
        johnson_table.columns = {"x"};
        for (double value : johnson_values) {
            johnson_table.rows.push_back({std::to_string(value)});
        }
        AnalysisConfiguration johnson_cfg;
        johnson_cfg.capability_method = "johnson";
        johnson_cfg.variable_columns = {0};
        johnson_cfg.specifications.lower = johnson_values.front();
        johnson_cfg.specifications.upper = johnson_values.back() * 1.2;
        OutputPage johnson_page = AnalysisService::capability(johnson_table, johnson_cfg);
        InterpretationService::enrich(johnson_page);
        QVERIFY(johnson_page.facts.capability.has_value());
        QCOMPARE(johnson_page.facts.capability->pass_fail_judgment_allowed, false);
        const ReportDocument johnson_en =
            localize_slice(johnson_table, johnson_page, "en-US");
        QVERIFY(!johnson_en.pages.empty());
        assert_visible_layer_no_cjk(johnson_en.pages[0], "Johnson capability");
        assert_visible_layer_excludes_substrings(
            johnson_en.pages[0],
            "Johnson capability",
            {"Johnson 变换过程能力", "不得输出过程合格判定", "研究/预览", "Johnson 能力门禁"});
        bool saw_johnson_gate_en = false;
        for (const auto& diagnostic : johnson_en.pages[0].visible_diagnostics) {
            if (diagnostic.code == "johnson_capability_gated") {
                saw_johnson_gate_en = true;
                QVERIFY2(
                    diagnostic.message.find("research/preview") != std::string::npos,
                    "en-US Johnson gate diagnostic must localize to English");
            }
        }
        QVERIFY2(
            saw_johnson_gate_en,
            "en-US Johnson visible layer should include gated capability diagnostic");
        const ReportDocument johnson_zh =
            localize_slice(johnson_table, johnson_page, "zh-CN");
        QVERIFY(!johnson_zh.pages.empty());
        QCOMPARE(
            johnson_zh.pages[0].source_page.title,
            std::string("Johnson 变换过程能力"));
        assert_visible_layer_no_english_catalog_leaks(
            johnson_zh.pages[0],
            "Johnson capability",
            {"Johnson-transform process capability",
             "Johnson capability is research/preview only",
             "Johnson capability gate",
             "research/preview only"});

        DataTable nonnormal_table;
        nonnormal_table.columns = {"x"};
        for (int i = 1; i <= 30; ++i) {
            nonnormal_table.rows.push_back({std::to_string(0.5 * static_cast<double>(i) + 1.0)});
        }
        AnalysisConfiguration nonnormal_cfg;
        nonnormal_cfg.capability_method = "non_normal";
        nonnormal_cfg.nonnormal_distribution = "weibull";
        nonnormal_cfg.variable_columns = {0};
        nonnormal_cfg.specifications.lower = 1.0;
        nonnormal_cfg.specifications.upper = 20.0;
        OutputPage nonnormal_page =
            AnalysisService::capability(nonnormal_table, nonnormal_cfg);
        InterpretationService::enrich(nonnormal_page);
        QVERIFY(nonnormal_page.facts.capability.has_value());
        QCOMPARE(nonnormal_page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(
            QString::fromStdString(nonnormal_page.facts.capability->method),
            QStringLiteral("non_normal"));
        const ReportDocument nonnormal_en =
            localize_slice(nonnormal_table, nonnormal_page, "en-US");
        QVERIFY(!nonnormal_en.pages.empty());
        QCOMPARE(
            nonnormal_en.pages[0].source_page.title,
            std::string("Nonnormal process capability"));
        assert_visible_layer_no_cjk(nonnormal_en.pages[0], "Nonnormal capability");
        assert_visible_layer_excludes_substrings(
            nonnormal_en.pages[0],
            "Nonnormal capability",
            {"非正态过程能力", "变量:", "非正态能力使用拟合", "不得自动开放合格判定",
             "过程合格"});
        bool saw_nonnormal_zscore_en = false;
        bool saw_stability_gate_en = false;
        for (const auto& diagnostic : nonnormal_en.pages[0].visible_diagnostics) {
            if (diagnostic.code == "nonnormal_z_score_formula_reference") {
                saw_nonnormal_zscore_en = true;
                QVERIFY2(
                    diagnostic.message.find("Nonnormal capability computes Pp/Ppk")
                        != std::string::npos
                        || diagnostic.message.find("formula reference") != std::string::npos,
                    "en-US nonnormal Z-score diagnostic must localize to English");
            }
            if (diagnostic.code == "capability_stability_prerequisite"
                || diagnostic.code == "capability_stability_screen_clear_not_verified"
                || diagnostic.code == "capability_stability_screen_signals"
                || diagnostic.code == "assumption_not_verified") {
                saw_stability_gate_en = true;
            }
        }
        QVERIFY2(
            saw_nonnormal_zscore_en,
            "en-US nonnormal visible layer should include Z-score formula_reference diagnostic");
        QVERIFY2(
            saw_stability_gate_en,
            "en-US nonnormal visible layer should include stability/assumption gate diagnostic");
        const ReportDocument nonnormal_zh =
            localize_slice(nonnormal_table, nonnormal_page, "zh-CN");
        QVERIFY(!nonnormal_zh.pages.empty());
        QCOMPARE(
            nonnormal_zh.pages[0].source_page.title,
            std::string("非正态过程能力"));
        assert_visible_layer_no_english_catalog_leaks(
            nonnormal_zh.pages[0],
            "Nonnormal capability",
            {"Nonnormal process capability",
             "Nonnormal capability computes Pp/Ppk",
             "process pass/fail judgment is blocked",
             "Capability stability prerequisite",
             "formula reference, not a Minitab export"});

        DataTable normal_cap_table;
        normal_cap_table.columns = {"x"};
        for (int i = 0; i < 25; ++i) {
            normal_cap_table.rows.push_back(
                {std::to_string(10.0 + ((i % 5) - 2) * 0.1)});
        }
        AnalysisConfiguration normal_cap_cfg;
        normal_cap_cfg.capability_method = "normal";
        normal_cap_cfg.variable_columns = {0};
        normal_cap_cfg.specifications.lower = 9.0;
        normal_cap_cfg.specifications.upper = 11.0;
        OutputPage normal_cap_page =
            AnalysisService::capability(normal_cap_table, normal_cap_cfg);
        InterpretationService::enrich(normal_cap_page);
        QVERIFY(normal_cap_page.facts.capability.has_value());
        QCOMPARE(normal_cap_page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(
            QString::fromStdString(normal_cap_page.facts.capability->gate_status),
            QStringLiteral("stability_unverified"));
        const ReportDocument normal_cap_en =
            localize_slice(normal_cap_table, normal_cap_page, "en-US");
        QVERIFY(!normal_cap_en.pages.empty());
        QCOMPARE(
            normal_cap_en.pages[0].source_page.title,
            std::string("Normal process capability analysis"));
        assert_visible_layer_no_cjk(normal_cap_en.pages[0], "Normal capability");
        assert_visible_layer_excludes_substrings(
            normal_cap_en.pages[0],
            "Normal capability",
            {"正态过程能力分析", "变量:", "稳定性前置", "禁止过程合格判定",
             "正态能力未满足"});
        bool saw_pass_fail_block_en = false;
        bool saw_stability_prereq_en = false;
        for (const auto& diagnostic : normal_cap_en.pages[0].visible_diagnostics) {
            if (diagnostic.code
                    == "capability_pass_fail_blocked_by_stability_prerequisite") {
                saw_pass_fail_block_en = true;
                QVERIFY2(
                    diagnostic.message.find("process pass/fail judgment is blocked")
                        != std::string::npos,
                    "en-US normal capability block diagnostic must localize to English");
            }
            if (diagnostic.code == "capability_stability_prerequisite"
                || diagnostic.code == "capability_stability_screen_clear_not_verified"
                || diagnostic.code == "capability_stability_screen_signals") {
                saw_stability_prereq_en = true;
            }
        }
        QVERIFY2(
            saw_pass_fail_block_en,
            "en-US normal capability visible layer should include pass/fail block diagnostic");
        QVERIFY2(
            saw_stability_prereq_en,
            "en-US normal capability visible layer should include stability prerequisite diagnostic");
        const ReportDocument normal_cap_zh =
            localize_slice(normal_cap_table, normal_cap_page, "zh-CN");
        QVERIFY(!normal_cap_zh.pages.empty());
        QCOMPARE(
            normal_cap_zh.pages[0].source_page.title,
            std::string("正态过程能力分析"));
        assert_visible_layer_no_english_catalog_leaks(
            normal_cap_zh.pages[0],
            "Normal capability",
            {"Normal process capability analysis",
             "process pass/fail judgment is blocked",
             "Capability stability prerequisite",
             "pass_fail_judgment_allowed=false"});

        const std::vector<std::vector<double>> rsm_coded = {
            {-1.0, -1.0}, {1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0},
            {-1.0, 0.0}, {1.0, 0.0}, {0.0, -1.0}, {0.0, 1.0},
            {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}};
        std::vector<double> rsm_response;
        rsm_response.reserve(rsm_coded.size());
        for (std::size_t i = 0; i < rsm_coded.size(); ++i) {
            const double x1 = rsm_coded[i][0];
            const double x2 = rsm_coded[i][1];
            double y = 10.0 + 3.0 * x1 - 2.0 * x2 + 1.5 * x1 * x2
                + 0.8 * x1 * x1 - 0.4 * x2 * x2;
            if (std::fabs(x1) < 1.0e-9 && std::fabs(x2) < 1.0e-9) {
                y += (i == 8) ? 0.4 : ((i == 9) ? -0.2 : -0.1);
            }
            rsm_response.push_back(y);
        }
        DataTable rsm_table;
        rsm_table.name = "rsm_lof";
        rsm_table.columns = {"Y", "A", "B"};
        for (std::size_t i = 0; i < rsm_coded.size(); ++i) {
            rsm_table.rows.push_back({
                std::to_string(rsm_response[i]),
                std::to_string(rsm_coded[i][0]),
                std::to_string(rsm_coded[i][1])});
        }
        AnalysisConfiguration rsm_cfg;
        rsm_cfg.chart_type = "rsm_response";
        rsm_cfg.variable_columns = {0, 1, 2};
        OutputPage rsm_page = AnalysisService::rsm_response(rsm_table, rsm_cfg);
        InterpretationService::enrich(rsm_page);
        QVERIFY(rsm_page.facts.rsm.has_value());
        QVERIFY(rsm_page.facts.rsm->lack_of_fit_available);
        const ReportDocument rsm_en = localize_slice(rsm_table, rsm_page, "en-US");
        QVERIFY(!rsm_en.pages.empty());
        QCOMPARE(
            rsm_en.pages[0].source_page.title,
            std::string("Response surface analysis"));
        assert_visible_layer_no_cjk(rsm_en.pages[0], "RSM response");
        assert_visible_layer_excludes_substrings(
            rsm_en.pages[0],
            "RSM response",
            {"响应曲面分析", "失拟", "纯误差", "RSM 失拟"});
        bool saw_rsm_lof_en = false;
        for (const auto& diagnostic : rsm_en.pages[0].visible_diagnostics) {
            if (diagnostic.code == "rsm_lof_formula_reference") {
                saw_rsm_lof_en = true;
                QVERIFY2(
                    diagnostic.message.find("RSM lack-of-fit") != std::string::npos
                        || diagnostic.message.find("formula_reference") != std::string::npos,
                    "en-US RSM LOF diagnostic must localize to English");
            }
        }
        QVERIFY2(
            saw_rsm_lof_en,
            "en-US RSM visible layer should include LOF formula_reference diagnostic");
        const ReportDocument rsm_zh =
            localize_slice(rsm_table, rsm_page, "zh-CN");
        QVERIFY(!rsm_zh.pages.empty());
        QCOMPARE(
            rsm_zh.pages[0].source_page.title,
            std::string("响应曲面分析"));
        assert_visible_layer_no_english_catalog_leaks(
            rsm_zh.pages[0],
            "RSM response",
            {"Response surface analysis",
             "RSM lack-of-fit ANOVA",
             "Lack of fit",
             "formula_reference only"});

        DataTable life_table;
        life_table.columns = {"time", "event"};
        life_table.rows = {
            {"10", "1"}, {"20", "1"}, {"30", "1"}, {"40", "1"}, {"50", "1"},
            {"25", "0"}, {"35", "0"}};
        AnalysisConfiguration weibull_cfg;
        weibull_cfg.reliability.time_column = 0;
        weibull_cfg.reliability.event_column = 1;
        weibull_cfg.reliability.model = "weibull";
        weibull_cfg.reliability.time_unit = "hours";
        OutputPage weibull_page =
            AnalysisService::reliability(life_table, weibull_cfg);
        InterpretationService::enrich(weibull_page);
        QVERIFY(weibull_page.facts.reliability.has_value());
        const ReportDocument weibull_en =
            localize_slice(life_table, weibull_page, "en-US");
        QVERIFY(!weibull_en.pages.empty());
        assert_visible_layer_no_cjk(weibull_en.pages[0], "Reliability Weibull");
        assert_visible_layer_excludes_substrings(
            weibull_en.pages[0],
            "Reliability Weibull",
            {"Weibull 参数", "百分位寿命", "可靠性分析", "当前为二参数 Weibull"});
        bool saw_weibull_params_en = false;
        bool saw_percentile_en = false;
        for (const auto& table_view : weibull_en.pages[0].visible_tables) {
            if (table_view.title.find("Weibull parameters") != std::string::npos) {
                saw_weibull_params_en = true;
            }
            if (table_view.title.find("Percentile life") != std::string::npos) {
                saw_percentile_en = true;
            }
        }
        QVERIFY2(
            saw_weibull_params_en,
            "en-US Weibull visible layer should expose localized parameter table title");
        QVERIFY2(
            saw_percentile_en,
            "en-US Weibull visible layer should expose localized percentile table title");
        const ReportDocument weibull_zh =
            localize_slice(life_table, weibull_page, "zh-CN");
        QVERIFY(!weibull_zh.pages.empty());
        QCOMPARE(
            weibull_zh.pages[0].source_page.title,
            std::string("可靠性分析"));
        assert_visible_layer_no_english_catalog_leaks(
            weibull_zh.pages[0],
            "Reliability Weibull",
            {"Weibull parameters",
             "Percentile life",
             "Using 2-parameter Weibull",
             "Reliability analysis"});

        AnalysisConfiguration lognormal_cfg = weibull_cfg;
        lognormal_cfg.reliability.model = "lognormal";
        OutputPage lognormal_page =
            AnalysisService::reliability(life_table, lognormal_cfg);
        InterpretationService::enrich(lognormal_page);
        QVERIFY(lognormal_page.facts.reliability.has_value());
        const ReportDocument lognormal_en =
            localize_slice(life_table, lognormal_page, "en-US");
        QVERIFY(!lognormal_en.pages.empty());
        assert_visible_layer_no_cjk(lognormal_en.pages[0], "Reliability Lognormal");
        assert_visible_layer_excludes_substrings(
            lognormal_en.pages[0],
            "Reliability Lognormal",
            {"Lognormal 参数", "百分位寿命", "可靠性分析", "当前为二参数对数正态"});
        bool saw_lognormal_params_en = false;
        for (const auto& table_view : lognormal_en.pages[0].visible_tables) {
            if (table_view.title.find("Lognormal parameters") != std::string::npos) {
                saw_lognormal_params_en = true;
            }
        }
        QVERIFY2(
            saw_lognormal_params_en,
            "en-US Lognormal visible layer should expose localized parameter table title");
        const ReportDocument lognormal_zh =
            localize_slice(life_table, lognormal_page, "zh-CN");
        QVERIFY(!lognormal_zh.pages.empty());
        QCOMPARE(
            lognormal_zh.pages[0].source_page.title,
            std::string("可靠性分析"));
        assert_visible_layer_no_english_catalog_leaks(
            lognormal_zh.pages[0],
            "Reliability Lognormal",
            {"Lognormal parameters",
             "Percentile life",
             "Using 2-parameter lognormal",
             "Reliability analysis"});
    }

    void representative_warranty_strata_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 3 + Phase 0 profiles: customer/engineer/audit × en-US/zh-CN visible-layer guards.
        DataTable table;
        table.columns = {"time", "censor_type", "mode", "exposure"};
        table.rows = {
            {"10", "exact", "wear", "10"},
            {"12", "exact", "wear", "10"},
            {"15", "right", "wear", "20"},
            {"8", "exact", "early", "5"},
            {"30", "right", "", "5"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.reliability_at_warranty = 0.95;
        configuration.reliability.censoring_type_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.exposure_column = 3;
        configuration.reliability.model = "weibull";
        const OutputPage page =
            AnalysisService::reliability_warranty(table, configuration);
        QVERIFY(page.facts.warranty.has_value());

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QVERIFY(!customer_en.pages.empty());
        QVERIFY(!engineer_en.pages.empty());
        QVERIFY(!audit_en.pages.empty());
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_en_profile = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"保修摘要", "失效模式分母追溯", "暴露量"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Warranty summary"));
        };
        assert_en_profile(customer_en, "customer en-US");
        assert_en_profile(engineer_en, "engineer en-US");
        assert_en_profile(audit_en, "audit en-US");
        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        bool engineer_stratum_en = false;
        bool audit_stratum_en = false;
        for (const auto& table_view : engineer_en.pages[0].visible_tables) {
            if (table_view.title.find("Failure-mode denominator trace") != std::string::npos) {
                engineer_stratum_en = true;
            }
        }
        for (const auto& table_view : audit_en.pages[0].visible_tables) {
            if (table_view.title.find("Failure-mode denominator trace") != std::string::npos) {
                audit_stratum_en = true;
            }
        }
        QVERIFY2(engineer_stratum_en, "engineer en-US should expose localized stratum table");
        QVERIFY2(audit_stratum_en, "audit en-US should expose localized stratum table");
        QVERIFY(!audit_en.pages[0].visible_evidence.empty());

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_zh_profile = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("保修摘要"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Warranty summary",
                 "Failure-mode denominator trace",
                 "Claims per 1000",
                 "Evidence appendix"});
        };
        assert_zh_profile(customer_zh, "customer zh-CN");
        assert_zh_profile(engineer_zh, "engineer zh-CN");
        assert_zh_profile(audit_zh, "audit zh-CN");
        bool engineer_stratum_zh = false;
        for (const auto& table_view : engineer_zh.pages[0].visible_tables) {
            if (table_view.title.find("失效模式分母追溯") != std::string::npos) {
                engineer_stratum_zh = true;
            }
        }
        QVERIFY2(engineer_stratum_zh, "engineer zh-CN should expose Chinese stratum table");
    }

    void representative_johnson_capability_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 0 + Phase 6: Johnson gated capability × customer/engineer/audit × en/zh guards.
        std::vector<double> values;
        for (int i = 1; i <= 40; ++i) {
            values.push_back(std::exp(0.05 * static_cast<double>(i))
                + 0.1 * static_cast<double>(i % 3));
        }
        DataTable table;
        table.columns = {"x"};
        for (double value : values) {
            table.rows.push_back({std::to_string(value)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "johnson";
        configuration.variable_columns = {0};
        configuration.specifications.lower = values.front();
        configuration.specifications.upper = values.back() * 1.2;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_johnson_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"Johnson 变换过程能力", "研究/预览", "不得输出过程合格判定",
                 "Johnson 能力门禁"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Johnson-transform process capability"));
        };
        assert_johnson_en(customer_en, "customer en-US");
        assert_johnson_en(engineer_en, "engineer en-US");
        assert_johnson_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        const auto has_gate_diagnostic = [](const datalab::domain::ReportPageView& page) {
            for (const auto& diagnostic : page.visible_diagnostics) {
                if (diagnostic.code == "johnson_capability_gated") {
                    return diagnostic.message.find("research/preview") != std::string::npos;
                }
            }
            return false;
        };
        QVERIFY2(
            has_gate_diagnostic(customer_en.pages[0]),
            "customer en-US should surface Johnson gate as key risk diagnostic");
        QVERIFY2(
            has_gate_diagnostic(engineer_en.pages[0]),
            "engineer en-US should surface Johnson gate diagnostic");
        QVERIFY2(
            has_gate_diagnostic(audit_en.pages[0]),
            "audit en-US should surface Johnson gate diagnostic");

        bool saw_johnson_evidence = false;
        for (const auto& ref : audit_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:johnson") != std::string::npos) {
                saw_johnson_evidence = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.johnson_capability_gated"));
            }
        }
        QVERIFY2(
            saw_johnson_evidence,
            "audit en-US visible_evidence must include Johnson limiting gate ref");
        bool engineer_johnson_evidence = false;
        for (const auto& ref : engineer_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:johnson") != std::string::npos) {
                engineer_johnson_evidence = true;
            }
        }
        QVERIFY2(
            engineer_johnson_evidence,
            "engineer en-US visible_evidence may include limiting gate refs without appendix");
        bool customer_johnson_evidence = false;
        for (const auto& ref : customer_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:johnson") != std::string::npos) {
                customer_johnson_evidence = true;
            }
        }
        QVERIFY2(
            customer_johnson_evidence,
            "customer en-US keeps limiting Johnson gate evidence ref");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_johnson_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Johnson 变换过程能力"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Johnson-transform process capability",
                 "Johnson capability is research/preview only",
                 "Johnson capability gate",
                 "research/preview only",
                 "Evidence appendix"});
        };
        assert_johnson_zh(customer_zh, "customer zh-CN");
        assert_johnson_zh(engineer_zh, "engineer zh-CN");
        assert_johnson_zh(audit_zh, "audit zh-CN");
    }

    void representative_rsm_lof_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 4 + Phase 0: RSM LOF formula_reference gate × customer/engineer/audit × en/zh guards.
        const std::vector<std::vector<double>> coded = {
            {-1.0, -1.0}, {1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0},
            {-1.0, 0.0}, {1.0, 0.0}, {0.0, -1.0}, {0.0, 1.0},
            {0.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}};
        std::vector<double> response;
        response.reserve(coded.size());
        for (std::size_t i = 0; i < coded.size(); ++i) {
            const double x1 = coded[i][0];
            const double x2 = coded[i][1];
            double y = 10.0 + 3.0 * x1 - 2.0 * x2 + 1.5 * x1 * x2
                + 0.8 * x1 * x1 - 0.4 * x2 * x2;
            if (std::fabs(x1) < 1.0e-9 && std::fabs(x2) < 1.0e-9) {
                y += (i == 8) ? 0.4 : ((i == 9) ? -0.2 : -0.1);
            }
            response.push_back(y);
        }
        DataTable table;
        table.name = "rsm_lof";
        table.columns = {"Y", "A", "B"};
        for (std::size_t i = 0; i < coded.size(); ++i) {
            table.rows.push_back({
                std::to_string(response[i]),
                std::to_string(coded[i][0]),
                std::to_string(coded[i][1])});
        }
        AnalysisConfiguration configuration;
        configuration.chart_type = "rsm_response";
        configuration.variable_columns = {0, 1, 2};
        OutputPage page = AnalysisService::rsm_response(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.rsm.has_value());
        QVERIFY(page.facts.rsm->lack_of_fit_available);

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_rsm_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"响应曲面分析", "失拟 ANOVA", "纯误差", "不是 vendor_oracle"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Response surface analysis"));
        };
        assert_rsm_en(customer_en, "customer en-US");
        assert_rsm_en(engineer_en, "engineer en-US");
        assert_rsm_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        const auto has_lof_diagnostic = [](const datalab::domain::ReportPageView& page) {
            for (const auto& diagnostic : page.visible_diagnostics) {
                if (diagnostic.code == "rsm_lof_formula_reference") {
                    return diagnostic.message.find("formula_reference") != std::string::npos
                        || diagnostic.message.find("Lack-of-fit") != std::string::npos;
                }
            }
            return false;
        };
        QVERIFY2(
            !has_lof_diagnostic(customer_en.pages[0]),
            "customer en-US should not surface info-level LOF diagnostic dumps");
        QVERIFY2(
            has_lof_diagnostic(engineer_en.pages[0]),
            "engineer en-US should surface LOF formula_reference diagnostic");
        QVERIFY2(
            has_lof_diagnostic(audit_en.pages[0]),
            "audit en-US should surface LOF formula_reference diagnostic");

        bool audit_lof_evidence = false;
        for (const auto& ref : audit_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:rsm_lof_formula_reference") != std::string::npos) {
                audit_lof_evidence = true;
                QCOMPARE(
                    ref.label_text_id,
                    std::string("evidence.rsm_lof_formula_reference"));
            }
        }
        QVERIFY2(
            audit_lof_evidence,
            "audit en-US visible_evidence must include RSM LOF limiting gate ref");
        bool engineer_lof_evidence = false;
        for (const auto& ref : engineer_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:rsm_lof_formula_reference") != std::string::npos) {
                engineer_lof_evidence = true;
            }
        }
        QVERIFY2(
            engineer_lof_evidence,
            "engineer en-US visible_evidence may include limiting RSM LOF gate ref");
        bool customer_lof_evidence = false;
        for (const auto& ref : customer_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:rsm_lof_formula_reference") != std::string::npos) {
                customer_lof_evidence = true;
                QCOMPARE(
                    ref.label_text_id,
                    std::string("evidence.rsm_lof_formula_reference"));
            }
        }
        QVERIFY2(
            customer_lof_evidence,
            "customer en-US keeps limiting RSM LOF gate evidence ref");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_rsm_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("响应曲面分析"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Response surface analysis",
                 "RSM lack-of-fit ANOVA",
                 "Lack-of-fit ANOVA",
                 "formula_reference only",
                 "Evidence appendix"});
        };
        assert_rsm_zh(customer_zh, "customer zh-CN");
        assert_rsm_zh(engineer_zh, "engineer zh-CN");
        assert_rsm_zh(audit_zh, "audit zh-CN");
    }

    void representative_doe_ccd_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 4 + Phase 0: DOE CCD design_formula gate × customer/engineer/audit × en/zh guards.
        DataTable empty_table;
        AnalysisConfiguration ccd_cfg;
        ccd_cfg.chart_type = "doe_ccd";
        auto& ccd = ccd_cfg.response_surface_design;
        ccd.design_kind = "ccd";
        ccd.ccd_variant = "ccf";
        ccd.factor_ids = {"A", "B"};
        ccd.factor_names = {"Temperature", "Pressure"};
        ccd.low_levels = {60.0, 100.0};
        ccd.high_levels = {80.0, 140.0};
        ccd.centers = {70.0, 120.0};
        ccd.center_point_count = 1;
        ccd.randomize = true;
        ccd.random_seed = 42;
        const OutputPage page =
            AnalysisService::doe_response_surface_design(empty_table, ccd_cfg);
        QVERIFY(page.facts.design_generation.has_value());
        QCOMPARE(
            QString::fromStdString(page.facts.design_generation->evidence_type),
            QStringLiteral("formula_reference"));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(empty_table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_ccd_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"中心复合设计", "DOE 设计生成仅为", "设计矩阵", "不是 vendor_oracle"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Central composite design (CCD)"));
        };
        assert_ccd_en(customer_en, "customer en-US");
        assert_ccd_en(engineer_en, "engineer en-US");
        assert_ccd_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        bool engineer_design_matrix = false;
        for (const auto& table_view : engineer_en.pages[0].visible_tables) {
            if (table_view.title.find("Design matrix") != std::string::npos) {
                engineer_design_matrix = true;
            }
        }
        QVERIFY2(
            engineer_design_matrix,
            "engineer en-US should expose localized design matrix table");

        const auto has_design_gate_evidence = [](const datalab::domain::ReportPageView& page) {
            for (const auto& ref : page.visible_evidence) {
                if (ref.evidence_id.find(":gate:design_formula_reference_only")
                        != std::string::npos) {
                    return ref.label_text_id == "evidence.design_formula_reference_only";
                }
            }
            return false;
        };
        QVERIFY2(
            has_design_gate_evidence(customer_en.pages[0]),
            "customer en-US keeps limiting design_formula gate evidence ref");
        QVERIFY2(
            has_design_gate_evidence(engineer_en.pages[0]),
            "engineer en-US keeps limiting design_formula gate evidence ref");
        QVERIFY2(
            has_design_gate_evidence(audit_en.pages[0]),
            "audit en-US keeps limiting design_formula gate evidence ref");

        bool audit_formula_evidence = false;
        for (const auto& ref : audit_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":formula:design_generation") != std::string::npos) {
                audit_formula_evidence = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.formula_reference"));
            }
        }
        QVERIFY2(
            audit_formula_evidence,
            "audit en-US visible_evidence should include design_generation formula ref");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_ccd_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("中心复合设计 (CCD)"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Central composite design (CCD)",
                 "DOE design generation is formula_reference only",
                 "Formula reference (not vendor_oracle)",
                 "Design matrix",
                 "Evidence appendix"});
        };
        assert_ccd_zh(customer_zh, "customer zh-CN");
        assert_ccd_zh(engineer_zh, "engineer zh-CN");
        assert_ccd_zh(audit_zh, "audit zh-CN");
        bool engineer_design_matrix_zh = false;
        for (const auto& table_view : engineer_zh.pages[0].visible_tables) {
            if (table_view.title.find("设计矩阵") != std::string::npos) {
                engineer_design_matrix_zh = true;
            }
        }
        QVERIFY2(
            engineer_design_matrix_zh,
            "engineer zh-CN should expose Chinese design matrix table title");
    }

    void representative_doe_bbd_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 4 + Phase 0: DOE BBD design_formula + bbd_no_corners gates × customer/engineer/audit × en/zh.
        DataTable empty_table;
        AnalysisConfiguration bbd_cfg;
        bbd_cfg.chart_type = "doe_bbd";
        auto& bbd = bbd_cfg.response_surface_design;
        bbd.design_kind = "bbd";
        bbd.factor_ids = {"X1", "X2", "X3"};
        bbd.factor_names = {"X1", "X2", "X3"};
        bbd.low_levels = {-1.0, -1.0, -1.0};
        bbd.high_levels = {1.0, 1.0, 1.0};
        bbd.centers = {0.0, 0.0, 0.0};
        bbd.center_point_count = 1;
        bbd.randomize = true;
        bbd.random_seed = 7;
        const OutputPage page =
            AnalysisService::doe_response_surface_design(empty_table, bbd_cfg);
        QVERIFY(page.facts.design_generation.has_value());
        QCOMPARE(
            QString::fromStdString(page.facts.design_generation->design_kind),
            QStringLiteral("bbd"));
        QCOMPARE(
            QString::fromStdString(page.facts.design_generation->evidence_type),
            QStringLiteral("formula_reference"));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(empty_table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_bbd_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"Box–Behnken 设计", "不包含所有因素", "BBD 无角点", "设计矩阵",
                 "不是 vendor_oracle"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Box–Behnken design"));
        };
        assert_bbd_en(customer_en, "customer en-US");
        assert_bbd_en(engineer_en, "engineer en-US");
        assert_bbd_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        bool engineer_design_matrix = false;
        for (const auto& table_view : engineer_en.pages[0].visible_tables) {
            if (table_view.title.find("Design matrix") != std::string::npos) {
                engineer_design_matrix = true;
            }
        }
        QVERIFY2(
            engineer_design_matrix,
            "engineer en-US should expose localized design matrix table");

        const auto has_bbd_corners_diagnostic = [](const datalab::domain::ReportPageView& page) {
            for (const auto& diagnostic : page.visible_diagnostics) {
                if (diagnostic.code == "bbd_no_corners") {
                    return diagnostic.message.find("BBD omits corners") != std::string::npos
                        || diagnostic.message.find("design-space boundary")
                            != std::string::npos;
                }
            }
            return false;
        };
        QVERIFY2(
            !has_bbd_corners_diagnostic(customer_en.pages[0]),
            "customer en-US should not surface info-level bbd_no_corners diagnostic dump");
        QVERIFY2(
            has_bbd_corners_diagnostic(engineer_en.pages[0]),
            "engineer en-US should surface bbd_no_corners honesty diagnostic");
        QVERIFY2(
            has_bbd_corners_diagnostic(audit_en.pages[0]),
            "audit en-US should surface bbd_no_corners honesty diagnostic");

        const auto has_gate_evidence = [](const datalab::domain::ReportPageView& page,
                                          const char* gate_suffix,
                                          const char* label_text_id) {
            for (const auto& ref : page.visible_evidence) {
                const std::string needle = std::string(":gate:") + gate_suffix;
                if (ref.evidence_id.find(needle) != std::string::npos) {
                    return ref.label_text_id == label_text_id;
                }
            }
            return false;
        };
        QVERIFY2(
            has_gate_evidence(
                customer_en.pages[0],
                "design_formula_reference_only",
                "evidence.design_formula_reference_only"),
            "customer en-US keeps limiting design_formula gate evidence ref");
        QVERIFY2(
            has_gate_evidence(
                engineer_en.pages[0],
                "design_formula_reference_only",
                "evidence.design_formula_reference_only"),
            "engineer en-US keeps limiting design_formula gate evidence ref");
        QVERIFY2(
            has_gate_evidence(
                audit_en.pages[0],
                "design_formula_reference_only",
                "evidence.design_formula_reference_only"),
            "audit en-US keeps limiting design_formula gate evidence ref");
        QVERIFY2(
            has_gate_evidence(
                customer_en.pages[0],
                "bbd_no_corners",
                "evidence.bbd_no_corners"),
            "customer en-US keeps limiting bbd_no_corners gate evidence ref");
        QVERIFY2(
            has_gate_evidence(
                engineer_en.pages[0],
                "bbd_no_corners",
                "evidence.bbd_no_corners"),
            "engineer en-US keeps limiting bbd_no_corners gate evidence ref");
        QVERIFY2(
            has_gate_evidence(
                audit_en.pages[0],
                "bbd_no_corners",
                "evidence.bbd_no_corners"),
            "audit en-US keeps limiting bbd_no_corners gate evidence ref");

        bool audit_formula_evidence = false;
        for (const auto& ref : audit_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":formula:design_generation") != std::string::npos) {
                audit_formula_evidence = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.formula_reference"));
            }
        }
        QVERIFY2(
            audit_formula_evidence,
            "audit en-US visible_evidence should include design_generation formula ref");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_bbd_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Box–Behnken 设计"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Box–Behnken design",
                 "BBD omits corners",
                 "BBD has no corner points",
                 "DOE design generation is formula_reference only",
                 "Design matrix",
                 "Evidence appendix"});
        };
        assert_bbd_zh(customer_zh, "customer zh-CN");
        assert_bbd_zh(engineer_zh, "engineer zh-CN");
        assert_bbd_zh(audit_zh, "audit zh-CN");
        bool engineer_design_matrix_zh = false;
        for (const auto& table_view : engineer_zh.pages[0].visible_tables) {
            if (table_view.title.find("设计矩阵") != std::string::npos) {
                engineer_design_matrix_zh = true;
            }
        }
        QVERIFY2(
            engineer_design_matrix_zh,
            "engineer zh-CN should expose Chinese design matrix table title");
    }

    void representative_nonnormal_capability_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 6 + Phase 0: non-normal capability stability gate × customer/engineer/audit × en/zh.
        DataTable table;
        table.columns = {"x"};
        for (int i = 1; i <= 30; ++i) {
            table.rows.push_back({std::to_string(0.5 * static_cast<double>(i) + 1.0)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "non_normal";
        configuration.nonnormal_distribution = "weibull";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 1.0;
        configuration.specifications.upper = 20.0;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(
            QString::fromStdString(page.facts.capability->method),
            QStringLiteral("non_normal"));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_nonnormal_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"非正态过程能力", "不得自动开放合格判定", "过程合格", "变量:"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Nonnormal process capability"));
        };
        assert_nonnormal_en(customer_en, "customer en-US");
        assert_nonnormal_en(engineer_en, "engineer en-US");
        assert_nonnormal_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        const auto has_nonnormal_risk_diagnostics =
            [](const datalab::domain::ReportPageView& page) {
                bool zscore = false;
                bool stability = false;
                for (const auto& diagnostic : page.visible_diagnostics) {
                    if (diagnostic.code == "nonnormal_z_score_formula_reference") {
                        zscore = diagnostic.message.find("Nonnormal capability") != std::string::npos
                            || diagnostic.message.find("formula reference") != std::string::npos;
                    }
                    if (diagnostic.code == "capability_stability_prerequisite"
                        || diagnostic.code == "capability_stability_screen_clear_not_verified"
                        || diagnostic.code == "capability_stability_screen_signals"
                        || diagnostic.code == "assumption_not_verified") {
                        stability = true;
                    }
                }
                return zscore && stability;
            };
        QVERIFY2(
            has_nonnormal_risk_diagnostics(customer_en.pages[0]),
            "customer en-US should surface nonnormal Z-score + stability risk diagnostics");
        QVERIFY2(
            has_nonnormal_risk_diagnostics(engineer_en.pages[0]),
            "engineer en-US should surface nonnormal Z-score + stability risk diagnostics");
        QVERIFY2(
            has_nonnormal_risk_diagnostics(audit_en.pages[0]),
            "audit en-US should surface nonnormal Z-score + stability risk diagnostics");

        bool audit_pass_fail_gate = false;
        for (const auto& ref : audit_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:capability_pass_fail_blocked")
                    != std::string::npos) {
                audit_pass_fail_gate = true;
                QCOMPARE(
                    ref.label_text_id,
                    std::string("evidence.capability_stability_prerequisite"));
            }
        }
        QVERIFY2(
            audit_pass_fail_gate,
            "audit en-US visible_evidence must include capability pass/fail blocked gate ref");
        bool customer_pass_fail_gate = false;
        for (const auto& ref : customer_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:capability_pass_fail_blocked")
                    != std::string::npos) {
                customer_pass_fail_gate = true;
            }
        }
        QVERIFY2(
            customer_pass_fail_gate,
            "customer en-US keeps limiting capability pass/fail blocked gate evidence ref");
        bool engineer_pass_fail_gate = false;
        for (const auto& ref : engineer_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:capability_pass_fail_blocked")
                    != std::string::npos) {
                engineer_pass_fail_gate = true;
            }
        }
        QVERIFY2(
            engineer_pass_fail_gate,
            "engineer en-US keeps limiting capability pass/fail blocked gate evidence ref");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_nonnormal_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("非正态过程能力"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Nonnormal process capability",
                 "Nonnormal capability computes Pp/Ppk",
                 "process pass/fail judgment is blocked",
                 "Capability stability prerequisite",
                 "formula reference, not a Minitab export",
                 "Evidence appendix"});
        };
        assert_nonnormal_zh(customer_zh, "customer zh-CN");
        assert_nonnormal_zh(engineer_zh, "engineer zh-CN");
        assert_nonnormal_zh(audit_zh, "audit zh-CN");
    }

    void representative_normal_capability_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 6 + Phase 0: normal capability stability gate × customer/engineer/audit × en/zh.
        DataTable table;
        table.columns = {"x"};
        for (int i = 0; i < 25; ++i) {
            table.rows.push_back(
                {std::to_string(10.0 + ((i % 5) - 2) * 0.1)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "normal";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 9.0;
        configuration.specifications.upper = 11.0;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(
            QString::fromStdString(page.facts.capability->gate_status),
            QStringLiteral("stability_unverified"));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_normal_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"正态过程能力分析", "稳定性前置", "禁止过程合格判定",
                 "正态能力未满足", "变量:"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Normal process capability analysis"));
        };
        assert_normal_en(customer_en, "customer en-US");
        assert_normal_en(engineer_en, "engineer en-US");
        assert_normal_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        const auto has_normal_stability_diagnostics =
            [](const datalab::domain::ReportPageView& page) {
                bool pass_fail_block = false;
                bool stability = false;
                for (const auto& diagnostic : page.visible_diagnostics) {
                    if (diagnostic.code
                            == "capability_pass_fail_blocked_by_stability_prerequisite") {
                        pass_fail_block =
                            diagnostic.message.find("process pass/fail judgment is blocked")
                                != std::string::npos;
                    }
                    if (diagnostic.code == "capability_stability_prerequisite"
                        || diagnostic.code == "capability_stability_screen_clear_not_verified"
                        || diagnostic.code == "capability_stability_screen_signals"
                        || diagnostic.code == "assumption_not_verified") {
                        stability = true;
                    }
                }
                return pass_fail_block && stability;
            };
        QVERIFY2(
            has_normal_stability_diagnostics(customer_en.pages[0]),
            "customer en-US should surface pass/fail block + stability risk diagnostics");
        QVERIFY2(
            has_normal_stability_diagnostics(engineer_en.pages[0]),
            "engineer en-US should surface pass/fail block + stability risk diagnostics");
        QVERIFY2(
            has_normal_stability_diagnostics(audit_en.pages[0]),
            "audit en-US should surface pass/fail block + stability risk diagnostics");

        const auto has_pass_fail_gate = [](const datalab::domain::ReportPageView& page) {
            for (const auto& ref : page.visible_evidence) {
                if (ref.evidence_id.find(":gate:capability_pass_fail_blocked")
                        != std::string::npos) {
                    return ref.label_text_id == "evidence.capability_stability_prerequisite";
                }
            }
            return false;
        };
        QVERIFY2(
            has_pass_fail_gate(customer_en.pages[0]),
            "customer en-US keeps limiting capability pass/fail blocked gate evidence ref");
        QVERIFY2(
            has_pass_fail_gate(engineer_en.pages[0]),
            "engineer en-US keeps limiting capability pass/fail blocked gate evidence ref");
        QVERIFY2(
            has_pass_fail_gate(audit_en.pages[0]),
            "audit en-US keeps limiting capability pass/fail blocked gate evidence ref");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_normal_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("正态过程能力分析"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Normal process capability analysis",
                 "process pass/fail judgment is blocked",
                 "Capability stability prerequisite",
                 "pass_fail_judgment_allowed=false",
                 "Evidence appendix"});
        };
        assert_normal_zh(customer_zh, "customer zh-CN");
        assert_normal_zh(engineer_zh, "engineer zh-CN");
        assert_normal_zh(audit_zh, "audit zh-CN");
    }

    void representative_box_cox_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 6 CAP-NN-2: Box-Cox assumption honesty × customer/engineer/audit × en/zh.
        DataTable table;
        table.columns = {"Y"};
        table.rows = {{"1.0"}, {"2.0"}, {"4.0"}, {"8.0"}, {"16.0"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "box_cox";
        configuration.variable_columns = {0};
        configuration.specifications.lower = 0.5;
        configuration.specifications.upper = 20.0;
        OutputPage page = AnalysisService::box_cox(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.box_cox.has_value());
        QCOMPARE(
            QString::fromStdString(page.facts.box_cox->assumption_status),
            QStringLiteral("not_verified"));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_box_cox_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"Box-Cox 变换", "变换后过程能力", "过程合格判定", "概率图只是诊断"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Box-Cox transformation"));
            QCOMPARE(
                doc.pages[0].source_page.facts.box_cox->assumption_status,
                std::string("not_verified"));
        };
        assert_box_cox_en(customer_en, "customer en-US");
        assert_box_cox_en(engineer_en, "engineer en-US");
        assert_box_cox_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        const auto has_pass_fail_honesty_bullet =
            [](const datalab::domain::ReportPageView& page) {
                for (const auto& section : page.visible_interpretation) {
                    for (const auto& bullet : section.bullets) {
                        if (bullet.find("process pass/fail decision") != std::string::npos
                            || bullet.find("not a process pass/fail decision")
                                != std::string::npos) {
                            return true;
                        }
                    }
                }
                return false;
            };
        QVERIFY2(
            has_pass_fail_honesty_bullet(customer_en.pages[0]),
            "customer en-US should surface localized Box-Cox pass/fail honesty bullet");
        QVERIFY2(
            has_pass_fail_honesty_bullet(engineer_en.pages[0]),
            "engineer en-US should surface localized Box-Cox pass/fail honesty bullet");
        QVERIFY2(
            has_pass_fail_honesty_bullet(audit_en.pages[0]),
            "audit en-US should surface localized Box-Cox pass/fail honesty bullet");

        bool engineer_capability_table = false;
        for (const auto& table_view : engineer_en.pages[0].visible_tables) {
            if (table_view.title == "Capability after transform") {
                engineer_capability_table = true;
            }
        }
        QVERIFY2(
            engineer_capability_table,
            "engineer en-US should expose localized post-transform capability table");
        bool customer_capability_table = false;
        for (const auto& table_view : customer_en.pages[0].visible_tables) {
            if (table_view.title.find("Capability") != std::string::npos) {
                customer_capability_table = true;
            }
        }
        QVERIFY2(
            !customer_capability_table,
            "customer en-US must not expose capability statistic tables");

        const auto has_box_cox_not_pass_fail_gate =
            [](const datalab::domain::ReportPageView& page_view) {
                for (const auto& ref : page_view.visible_evidence) {
                    if (ref.evidence_id.find(":gate:box_cox_not_pass_fail")
                            != std::string::npos) {
                        return ref.label_text_id == "evidence.box_cox_not_pass_fail";
                    }
                }
                return false;
            };
        QVERIFY2(
            has_box_cox_not_pass_fail_gate(customer_en.pages[0]),
            "customer en-US keeps limiting Box-Cox not-pass/fail gate evidence ref");
        QVERIFY2(
            has_box_cox_not_pass_fail_gate(engineer_en.pages[0]),
            "engineer en-US keeps limiting Box-Cox not-pass/fail gate evidence ref");
        QVERIFY2(
            has_box_cox_not_pass_fail_gate(audit_en.pages[0]),
            "audit en-US keeps limiting Box-Cox not-pass/fail gate evidence ref");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_box_cox_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Box-Cox 变换"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Box-Cox transformation",
                 "Capability after transform",
                 "process pass/fail decision",
                 "The probability plot is diagnostic only",
                 "Evidence appendix"});
        };
        assert_box_cox_zh(customer_zh, "customer zh-CN");
        assert_box_cox_zh(engineer_zh, "engineer zh-CN");
        assert_box_cox_zh(audit_zh, "audit zh-CN");
        bool engineer_capability_table_zh = false;
        for (const auto& table_view : engineer_zh.pages[0].visible_tables) {
            if (table_view.title.find("变换后过程能力") != std::string::npos) {
                engineer_capability_table_zh = true;
            }
        }
        QVERIFY2(
            engineer_capability_table_zh,
            "engineer zh-CN should expose Chinese post-transform capability table title");

        // Invalid spec limit gate: all templates omit capability table; en-US gate bullet + diagnostic.
        AnalysisConfiguration bad_cfg = configuration;
        bad_cfg.specifications.lower = -1.0;
        bad_cfg.specifications.upper = 10.0;
        OutputPage bad_page = AnalysisService::box_cox(table, bad_cfg);
        InterpretationService::enrich(bad_page);
        const auto localize_bad = [&](ReportTemplateKind kind,
                                      const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {bad_page}, profile, options);
            return localize_report_document(document).document;
        };
        const ReportDocument bad_customer_en =
            localize_bad(ReportTemplateKind::customer, "en-US");
        const ReportDocument bad_engineer_en =
            localize_bad(ReportTemplateKind::engineer, "en-US");
        const ReportDocument bad_audit_en =
            localize_bad(ReportTemplateKind::audit, "en-US");
        for (const ReportDocument* doc :
             {&bad_customer_en, &bad_engineer_en, &bad_audit_en}) {
            assert_visible_layer_no_cjk(doc->pages[0], "Box-Cox invalid spec gate");
            for (const auto& table_view : doc->pages[0].visible_tables) {
                QVERIFY2(
                    table_view.title.find("Capability") == std::string::npos,
                    "invalid spec gate must omit capability tables");
            }
        }
        const auto has_gate_bullet = [](const datalab::domain::ReportPageView& page_view) {
            for (const auto& section : page_view.visible_interpretation) {
                for (const auto& bullet : section.bullets) {
                    if (bullet.find("post-transform capability table was skipped")
                            != std::string::npos) {
                        return true;
                    }
                }
            }
            return false;
        };
        QVERIFY2(
            has_gate_bullet(bad_customer_en.pages[0]),
            "customer en-US invalid-spec must surface gate limitation bullet");
        bool bad_customer_diag = false;
        for (const auto& diagnostic : bad_customer_en.pages[0].visible_diagnostics) {
            if (diagnostic.code == "box_cox_invalid_spec_limit") {
                bad_customer_diag = true;
                QVERIFY2(
                    diagnostic.message.find("Lower spec limit") != std::string::npos,
                    "customer invalid-spec diagnostic must localize");
            }
        }
        QVERIFY2(
            bad_customer_diag,
            "customer en-US invalid-spec must include risk diagnostic");

        bool bad_audit_spec_gate = false;
        bool bad_audit_not_pass_fail = false;
        for (const auto& ref : bad_audit_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:box_cox_spec_limit") != std::string::npos) {
                bad_audit_spec_gate = true;
            }
            if (ref.evidence_id.find(":gate:box_cox_not_pass_fail") != std::string::npos) {
                bad_audit_not_pass_fail = true;
            }
        }
        QVERIFY2(
            bad_audit_spec_gate,
            "audit invalid-spec must include Box-Cox spec-limit gate evidence ref");
        QVERIFY2(
            bad_audit_not_pass_fail,
            "audit invalid-spec must include Box-Cox not-pass/fail gate evidence ref");

        const ReportDocument bad_customer_zh =
            localize_bad(ReportTemplateKind::customer, "zh-CN");
        bool bad_customer_zh_bullet = false;
        for (const auto& section : bad_customer_zh.pages[0].visible_interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("已跳过变换后过程能力表") != std::string::npos) {
                    bad_customer_zh_bullet = true;
                }
            }
        }
        QVERIFY2(
            bad_customer_zh_bullet,
            "customer zh-CN invalid-spec must retain Chinese gate limitation bullet");

        AnalysisConfiguration inverted_cfg = configuration;
        inverted_cfg.specifications.lower = 10.0;
        inverted_cfg.specifications.upper = 2.0;
        OutputPage inverted_page = AnalysisService::box_cox(table, inverted_cfg);
        InterpretationService::enrich(inverted_page);
        const ReportDocument inverted_engineer_en = [&]() {
            auto profile = make_report_profile(ReportTemplateKind::engineer);
            profile.locale.language_tag = "en-US";
            ReportDocument document =
                build_report_document(table, {inverted_page}, profile, options);
            return localize_report_document(document).document;
        }();
        bool inverted_order_diag = false;
        for (const auto& diagnostic : inverted_engineer_en.pages[0].visible_diagnostics) {
            if (diagnostic.code == "box_cox_spec_limits_order") {
                inverted_order_diag = true;
                QVERIFY2(
                    diagnostic.message.find("out of order") != std::string::npos,
                    "inverted spec diagnostic must localize to English");
            }
        }
        QVERIFY2(
            inverted_order_diag,
            "inverted LSL/USL must surface box_cox_spec_limits_order diagnostic");
        QVERIFY2(
            has_gate_bullet(inverted_engineer_en.pages[0]),
            "inverted spec must surface gate limitation bullet");
        for (const auto& table_view : inverted_engineer_en.pages[0].visible_tables) {
            QVERIFY2(
                table_view.title.find("Capability") == std::string::npos,
                "inverted spec gate must omit capability tables");
        }
    }

    void representative_reliability_km_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 5 REL-2: Kaplan-Meier × customer/engineer/audit × en/zh visible-layer guards.
        DataTable table;
        table.columns = {"time", "censor_type", "mode"};
        table.rows = {
            {"10", "exact", "wear"},
            {"15", "right", ""},
            {"20", "failure", "wear"},
            {"25", "censored", ""},
            {"30", "exact", "wear"}};
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.censoring_type_column = 1;
        configuration.reliability.failure_mode_column = 2;
        configuration.reliability.model = "kaplan_meier";
        configuration.reliability.time_unit = "hours";
        OutputPage page = AnalysisService::reliability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.reliability.has_value());
        QCOMPARE(
            QString::fromStdString(page.facts.reliability->distribution),
            QStringLiteral("kaplan_meier"));
        QCOMPARE(
            QString::fromStdString(page.facts.reliability->evidence_type),
            QStringLiteral("formula_reference"));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_km_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"生存表", "删失类型", "可靠性分析", "Kaplan-Meier 生存"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Reliability analysis"));
        };
        assert_km_en(customer_en, "customer en-US");
        assert_km_en(engineer_en, "engineer en-US");
        assert_km_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        bool engineer_survival_table = false;
        for (const auto& table_view : engineer_en.pages[0].visible_tables) {
            if (table_view.title.find("Kaplan-Meier survival table") != std::string::npos) {
                engineer_survival_table = true;
            }
        }
        QVERIFY2(
            engineer_survival_table,
            "engineer en-US should expose localized KM survival table title");

        bool audit_formula_evidence = false;
        for (const auto& ref : audit_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":formula:reliability") != std::string::npos) {
                audit_formula_evidence = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.formula_reference"));
            }
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
        }
        QVERIFY2(
            audit_formula_evidence,
            "audit en-US visible_evidence should include reliability formula ref");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_km_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("可靠性分析"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Reliability analysis",
                 "Kaplan-Meier survival table",
                 "Lifetime column =",
                 "Evidence appendix"});
        };
        assert_km_zh(customer_zh, "customer zh-CN");
        assert_km_zh(engineer_zh, "engineer zh-CN");
        assert_km_zh(audit_zh, "audit zh-CN");
        bool engineer_survival_table_zh = false;
        for (const auto& table_view : engineer_zh.pages[0].visible_tables) {
            if (table_view.title.find("Kaplan-Meier 生存表") != std::string::npos) {
                engineer_survival_table_zh = true;
            }
        }
        QVERIFY2(
            engineer_survival_table_zh,
            "engineer zh-CN should expose Chinese KM survival table title");
    }

    void representative_reliability_weibull_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 5 REL-3: Weibull × customer/engineer/audit × en/zh visible-layer guards.
        DataTable table;
        table.columns = {"time", "event"};
        table.rows = {
            {"10", "1"}, {"20", "1"}, {"30", "1"}, {"40", "1"}, {"50", "1"},
            {"25", "0"}, {"35", "0"}};
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.event_column = 1;
        configuration.reliability.model = "weibull";
        configuration.reliability.time_unit = "hours";
        OutputPage page = AnalysisService::reliability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.reliability.has_value());
        QCOMPARE(
            QString::fromStdString(page.facts.reliability->evidence_type),
            QStringLiteral("formula_reference"));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_weibull_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"可靠性分析", "Weibull 参数", "百分位寿命", "二参数 Weibull"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Reliability analysis"));
        };
        assert_weibull_en(customer_en, "customer en-US");
        assert_weibull_en(engineer_en, "engineer en-US");
        assert_weibull_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        const auto has_two_param_weibull_diagnostic =
            [](const datalab::domain::ReportPageView& page) {
                for (const auto& diagnostic : page.visible_diagnostics) {
                    if (diagnostic.code == "two_param_weibull") {
                        return diagnostic.message.find("2-parameter Weibull") != std::string::npos
                            || diagnostic.message.find("Using 2-parameter Weibull")
                                != std::string::npos;
                    }
                }
                return false;
            };
        QVERIFY2(
            !has_two_param_weibull_diagnostic(customer_en.pages[0]),
            "customer en-US should not surface info-level two_param_weibull diagnostic dump");
        QVERIFY2(
            has_two_param_weibull_diagnostic(engineer_en.pages[0]),
            "engineer en-US should surface localized two_param_weibull diagnostic");
        QVERIFY2(
            has_two_param_weibull_diagnostic(audit_en.pages[0]),
            "audit en-US should surface localized two_param_weibull diagnostic");

        bool engineer_weibull_params = false;
        bool engineer_percentile = false;
        for (const auto& table_view : engineer_en.pages[0].visible_tables) {
            if (table_view.title.find("Weibull parameters") != std::string::npos) {
                engineer_weibull_params = true;
            }
            if (table_view.title.find("Percentile life") != std::string::npos) {
                engineer_percentile = true;
            }
        }
        QVERIFY2(engineer_weibull_params, "engineer en-US should expose Weibull parameters table");
        QVERIFY2(engineer_percentile, "engineer en-US should expose percentile life table");

        bool audit_formula_evidence = false;
        for (const auto& ref : audit_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":formula:reliability") != std::string::npos) {
                audit_formula_evidence = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.formula_reference"));
            }
        }
        QVERIFY2(
            audit_formula_evidence,
            "audit en-US visible_evidence should include reliability formula ref");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_weibull_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("可靠性分析"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Reliability analysis",
                 "Weibull parameters",
                 "Percentile life",
                 "Using 2-parameter Weibull",
                 "Evidence appendix"});
        };
        assert_weibull_zh(customer_zh, "customer zh-CN");
        assert_weibull_zh(engineer_zh, "engineer zh-CN");
        assert_weibull_zh(audit_zh, "audit zh-CN");
    }

    void representative_reliability_lognormal_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 5 REL-3: Lognormal × customer/engineer/audit × en/zh visible-layer guards.
        DataTable table;
        table.columns = {"time", "event"};
        table.rows = {
            {"10", "1"}, {"20", "1"}, {"30", "1"}, {"40", "1"}, {"50", "1"},
            {"25", "0"}, {"35", "0"}};
        AnalysisConfiguration configuration;
        configuration.reliability.time_column = 0;
        configuration.reliability.event_column = 1;
        configuration.reliability.model = "lognormal";
        configuration.reliability.time_unit = "hours";
        OutputPage page = AnalysisService::reliability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.reliability.has_value());
        QCOMPARE(
            QString::fromStdString(page.facts.reliability->evidence_type),
            QStringLiteral("formula_reference"));

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_lognormal_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"可靠性分析", "Lognormal 参数", "百分位寿命", "二参数对数正态"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Reliability analysis"));
        };
        assert_lognormal_en(customer_en, "customer en-US");
        assert_lognormal_en(engineer_en, "engineer en-US");
        assert_lognormal_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        const auto has_two_param_lognormal_diagnostic =
            [](const datalab::domain::ReportPageView& page) {
                for (const auto& diagnostic : page.visible_diagnostics) {
                    if (diagnostic.code == "two_param_lognormal") {
                        return diagnostic.message.find("2-parameter lognormal") != std::string::npos
                            || diagnostic.message.find("Using 2-parameter lognormal")
                                != std::string::npos;
                    }
                }
                return false;
            };
        QVERIFY2(
            !has_two_param_lognormal_diagnostic(customer_en.pages[0]),
            "customer en-US should not surface info-level two_param_lognormal diagnostic dump");
        QVERIFY2(
            has_two_param_lognormal_diagnostic(engineer_en.pages[0]),
            "engineer en-US should surface localized two_param_lognormal diagnostic");
        QVERIFY2(
            has_two_param_lognormal_diagnostic(audit_en.pages[0]),
            "audit en-US should surface localized two_param_lognormal diagnostic");

        bool engineer_lognormal_params = false;
        for (const auto& table_view : engineer_en.pages[0].visible_tables) {
            if (table_view.title.find("Lognormal parameters") != std::string::npos) {
                engineer_lognormal_params = true;
            }
        }
        QVERIFY2(
            engineer_lognormal_params,
            "engineer en-US should expose Lognormal parameters table");

        bool audit_formula_evidence = false;
        for (const auto& ref : audit_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":formula:reliability") != std::string::npos) {
                audit_formula_evidence = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.formula_reference"));
            }
        }
        QVERIFY2(
            audit_formula_evidence,
            "audit en-US visible_evidence should include reliability formula ref");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_lognormal_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("可靠性分析"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Reliability analysis",
                 "Lognormal parameters",
                 "Percentile life",
                 "Using 2-parameter lognormal",
                 "Evidence appendix"});
        };
        assert_lognormal_zh(customer_zh, "customer zh-CN");
        assert_lognormal_zh(engineer_zh, "engineer zh-CN");
        assert_lognormal_zh(audit_zh, "audit zh-CN");
    }

    void representative_graph_scatter_faceted_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 7 + Phase 0: Graph Builder faceted scatter × customer/engineer/audit × en/zh + hidden honesty.
        DataTable table;
        table.columns = {"Time", "X", "Y", "Group"};
        table.rows = {
            {"1", "1", "2", "A"},
            {"2", "2", "3", "A"},
            {"3", "3", "1", "B"},
            {"4", "4", "0", "B"}};
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "scatter";
        configuration.graph.x_column = 1;
        configuration.graph.y_column = 2;
        configuration.graph.facet_column = 3;
        configuration.graph.facet_max_panels = 2;
        configuration.hidden_rows = {1};
        OutputPage page = GraphService::run(table, configuration);
        QVERIFY(page.facts.eda.has_value());
        QVERIFY(page.facts.eda->facet_enabled);
        QVERIFY(page.plots.size() >= 2);
        QVERIFY(page.facts.eda->hidden_excluded_distinct);
        QCOMPARE(page.facts.eda->hidden_count, std::size_t{1});

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_scatter_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"散点图（分面）", "显示 N", "分析 N", "分面 ="});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Scatterplot (faceted)"));
        };
        assert_scatter_en(customer_en, "customer en-US");
        assert_scatter_en(engineer_en, "engineer en-US");
        assert_scatter_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        QVERIFY2(
            customer_en.pages[0].visible_plots.size() == 1,
            "customer en-US should truncate faceted scatter to max_plots=1");
        QVERIFY2(
            engineer_en.pages[0].visible_plots.size() >= 2,
            "engineer en-US should expose multiple faceted scatter panels");
        QCOMPARE(
            engineer_en.pages[0].visible_plots.size(),
            audit_en.pages[0].visible_plots.size());

        QVERIFY(!customer_en.pages[0].show_parameter_summary);
        QVERIFY(engineer_en.pages[0].show_parameter_summary);
        QVERIFY2(
            engineer_en.pages[0].source_page.parameter_summary.find("Facet =")
                != std::string::npos,
            "engineer source page should carry localized facet parameter summary");
        QVERIFY2(
            engineer_en.pages[0].source_page.parameter_summary.find("Display N =")
                != std::string::npos,
            "engineer source page should carry localized display N");
        QVERIFY2(
            engineer_en.pages[0].source_page.parameter_summary.find("Analysis N =")
                != std::string::npos,
            "engineer source page should carry localized analysis N");
        QVERIFY2(
            engineer_en.pages[0].source_page.parameter_summary.find("hidden = 1")
                != std::string::npos,
            "engineer source page should carry localized hidden count");

        bool engineer_visibility_diag = false;
        for (const auto& diagnostic : engineer_en.pages[0].visible_diagnostics) {
            if (diagnostic.code == "row_visibility_contract") {
                engineer_visibility_diag = true;
                QVERIFY2(
                    diagnostic.message.find("hidden") != std::string::npos,
                    "engineer en-US must localize row visibility contract diagnostic");
            }
        }
        QVERIFY2(
            engineer_visibility_diag,
            "engineer en-US should surface localized row visibility contract diagnostic");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_scatter_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("散点图（分面）"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Scatterplot (faceted)",
                 "Display N =",
                 "Analysis N =",
                 "Facet =",
                 "Evidence appendix"});
        };
        assert_scatter_zh(customer_zh, "customer zh-CN");
        assert_scatter_zh(engineer_zh, "engineer zh-CN");
        assert_scatter_zh(audit_zh, "audit zh-CN");
    }

    void representative_graph_bar_faceted_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 7: Graph Builder faceted bar × customer/engineer/audit × en/zh + hidden/excluded honesty.
        DataTable table;
        table.columns = {"x", "cat", "facet"};
        table.rows = {
            {"1.0", "A", "P1"},
            {"2.0", "B", "P1"},
            {"3.0", "A", "P2"},
            {"4.0", "B", "P2"},
            {"5.0", "A", "P3"},
            {"6.0", "B", "P3"},
        };
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "bar";
        configuration.graph.x_column = 1;
        configuration.graph.facet_column = 2;
        configuration.graph.facet_max_panels = 2;
        configuration.hidden_rows = {1};
        OutputPage page = GraphService::run(table, configuration);
        QVERIFY(page.facts.eda.has_value());
        QVERIFY(page.facts.eda->hidden_excluded_distinct);
        QVERIFY(page.plots.size() >= 2);

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");

        const auto assert_bar_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"条形图（分面）", "条形图显示计数省略", "类别 =", "分面 ="});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Bar chart (faceted)"));
        };
        assert_bar_en(customer_en, "customer en-US");
        assert_bar_en(engineer_en, "engineer en-US");
        assert_bar_en(audit_en, "audit en-US");

        QVERIFY2(
            customer_en.pages[0].visible_plots.size() == 1,
            "customer en-US should truncate faceted bar to max_plots=1");
        QVERIFY2(
            engineer_en.pages[0].visible_plots.size() >= 2,
            "engineer en-US should expose multiple faceted bar panels");
        QVERIFY(!customer_en.pages[0].show_parameter_summary);
        QVERIFY(engineer_en.pages[0].show_parameter_summary);

        bool engineer_bar_diag = false;
        for (const auto& diagnostic : engineer_en.pages[0].visible_diagnostics) {
            if (diagnostic.code == "bar_hidden_excluded_distinct") {
                engineer_bar_diag = true;
                QVERIFY2(
                    diagnostic.message.find("omit hidden") != std::string::npos,
                    "engineer en-US must localize bar hidden/excluded diagnostic");
            }
        }
        QVERIFY2(
            engineer_bar_diag,
            "engineer en-US should surface localized bar visibility diagnostic");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        QCOMPARE(
            customer_zh.pages[0].source_page.title,
            std::string("条形图（分面）"));
    }

    void representative_graph_hexbin_faceted_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 7: Graph Builder faceted hexbin × customer/engineer/audit × en/zh.
        DataTable table;
        table.columns = {"y", "cat", "facet", "x"};
        table.rows = {
            {"10", "A", "P1", "1"},
            {"12", "B", "P1", "2"},
            {"14", "A", "P2", "3"},
            {"16", "B", "P2", "4"},
            {"18", "A", "P3", "5"},
            {"20", "B", "P3", "6"},
            {"22", "A", "P4", "7"},
        };
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "hexbin";
        configuration.graph.x_column = 3;
        configuration.graph.y_column = 0;
        configuration.graph.facet_column = 2;
        configuration.graph.facet_max_panels = 2;
        configuration.excluded_rows = {6};
        configuration.hidden_rows = {1};
        OutputPage page = GraphService::run(table, configuration);
        QVERIFY(page.facts.eda.has_value());
        QVERIFY(page.facts.eda->facet_enabled);
        QVERIFY(page.plots.size() >= 2);
        QVERIFY(page.facts.eda->hidden_excluded_distinct);
        QCOMPARE(page.facts.eda->hidden_count, std::size_t{1});
        QCOMPARE(page.facts.eda->excluded_count, std::size_t{1});
        bool saw_rect_bins = false;
        for (const auto& diagnostic : page.diagnostics) {
            if (diagnostic.code == "hexbin_rectangular_bins") {
                saw_rect_bins = true;
            }
        }
        QVERIFY2(saw_rect_bins, "hexbin fixture must emit rectangular-bin diagnostic");

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_hexbin_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"Hexbin（分面）", "使用矩形二维分箱", "分面 =", "显示 N", "分析 N"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Hexbin (faceted)"));
        };
        assert_hexbin_en(customer_en, "customer en-US");
        assert_hexbin_en(engineer_en, "engineer en-US");
        assert_hexbin_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        QVERIFY2(
            customer_en.pages[0].visible_plots.size() == 1,
            "customer en-US should truncate faceted hexbin to max_plots=1");
        QVERIFY2(
            engineer_en.pages[0].visible_plots.size() >= 2,
            "engineer en-US should expose multiple faceted hexbin panels");
        QCOMPARE(
            engineer_en.pages[0].visible_plots.size(),
            audit_en.pages[0].visible_plots.size());

        QVERIFY(!customer_en.pages[0].show_parameter_summary);
        QVERIFY(engineer_en.pages[0].show_parameter_summary);
        QVERIFY2(
            engineer_en.pages[0].source_page.parameter_summary.find("Facet =")
                != std::string::npos,
            "engineer source page should carry localized facet parameter summary");

        const auto has_hexbin_gate = [](const datalab::domain::ReportPageView& page) {
            for (const auto& ref : page.visible_evidence) {
                if (ref.evidence_id.find(":gate:hexbin_rectangular_bins") != std::string::npos) {
                    return true;
                }
            }
            return false;
        };
        QVERIFY2(
            has_hexbin_gate(customer_en.pages[0]),
            "customer en-US keeps hexbin rectangular-bin limiting gate ref");
        QVERIFY2(
            has_hexbin_gate(audit_en.pages[0]),
            "audit en-US keeps hexbin rectangular-bin limiting gate ref");

        bool engineer_rect_diag = false;
        for (const auto& diagnostic : engineer_en.pages[0].visible_diagnostics) {
            if (diagnostic.code == "hexbin_rectangular_bins") {
                engineer_rect_diag = true;
                QVERIFY2(
                    diagnostic.message.find("rectangular 2D bins") != std::string::npos,
                    "engineer en-US must localize hexbin honesty diagnostic");
            }
        }
        QVERIFY2(
            engineer_rect_diag,
            "engineer en-US should surface localized rectangular-bin diagnostic");
        QVERIFY2(
            engineer_en.pages[0].source_page.parameter_summary.find("Display N =")
                != std::string::npos,
            "engineer source page should carry localized display N");
        QVERIFY2(
            engineer_en.pages[0].source_page.parameter_summary.find("hidden = 1")
                != std::string::npos,
            "engineer source page should carry localized hidden count");
        QVERIFY2(
            engineer_en.pages[0].source_page.parameter_summary.find("excluded = 1")
                != std::string::npos,
            "engineer source page should carry localized excluded count");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_hexbin_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Hexbin（分面）"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Hexbin (faceted)",
                 "Display N =",
                 "Analysis N =",
                 "Facet =",
                 "Using rectangular 2D bins",
                 "Evidence appendix"});
        };
        assert_hexbin_zh(customer_zh, "customer zh-CN");
        assert_hexbin_zh(engineer_zh, "engineer zh-CN");
        assert_hexbin_zh(audit_zh, "audit zh-CN");
    }

    void representative_graph_density_faceted_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 7: Graph Builder faceted density × customer/engineer/audit × en/zh + hidden honesty.
        DataTable table;
        table.columns = {"x", "cat", "facet"};
        table.rows = {
            {"1.0", "A", "P1"},
            {"2.0", "B", "P1"},
            {"3.0", "A", "P2"},
            {"4.0", "B", "P2"},
            {"5.0", "A", "P3"},
            {"6.0", "B", "P3"},
        };
        AnalysisConfiguration configuration;
        configuration.graph.graph_kind = "density";
        configuration.graph.x_column = 0;
        configuration.graph.facet_column = 2;
        configuration.graph.facet_max_panels = 2;
        configuration.hidden_rows = {1};
        OutputPage page = GraphService::run(table, configuration);
        QVERIFY(page.facts.eda.has_value());
        QVERIFY(page.plots.size() >= 2);
        QVERIFY(page.facts.eda->hidden_excluded_distinct);
        QCOMPARE(page.facts.eda->hidden_count, std::size_t{1});

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");

        const auto assert_density_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"密度图（分面）", "密度曲线是连续", "分面 =", "显示 N", "分析 N"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Density plot (faceted)"));
        };
        assert_density_en(customer_en, "customer en-US");
        assert_density_en(engineer_en, "engineer en-US");
        assert_density_en(audit_en, "audit en-US");

        QVERIFY2(
            customer_en.pages[0].visible_plots.size() == 1,
            "customer en-US should truncate faceted density to max_plots=1");
        QVERIFY2(
            engineer_en.pages[0].visible_plots.size() >= 2,
            "engineer en-US should expose multiple faceted density panels");

        const auto has_density_gate = [](const datalab::domain::ReportPageView& page) {
            for (const auto& ref : page.visible_evidence) {
                if (ref.evidence_id.find(":gate:density_curve_not_discrete_marks")
                        != std::string::npos) {
                    return true;
                }
            }
            return false;
        };
        QVERIFY2(
            has_density_gate(customer_en.pages[0]),
            "customer en-US keeps density curve honesty limiting gate ref");
        QVERIFY2(
            has_density_gate(audit_en.pages[0]),
            "audit en-US keeps density curve honesty limiting gate ref");

        bool engineer_density_diag = false;
        for (const auto& diagnostic : engineer_en.pages[0].visible_diagnostics) {
            if (diagnostic.code == "density_curve_not_discrete_marks") {
                engineer_density_diag = true;
                QVERIFY2(
                    diagnostic.message.find("continuous KDE grids") != std::string::npos,
                    "engineer en-US must localize density honesty diagnostic");
            }
        }
        QVERIFY2(
            engineer_density_diag,
            "engineer en-US should surface localized density honesty diagnostic");
        QVERIFY2(
            engineer_en.pages[0].source_page.parameter_summary.find("Display N =")
                != std::string::npos,
            "engineer source page should carry localized display N");
        QVERIFY2(
            engineer_en.pages[0].source_page.parameter_summary.find("hidden = 1")
                != std::string::npos,
            "engineer source page should carry localized hidden count");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        QCOMPARE(
            customer_zh.pages[0].source_page.title,
            std::string("密度图（分面）"));
    }

    void representative_warranty_summary_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 5 + Phase 0: scalar warranty summary × customer/engineer/audit × en/zh guards.
        DataTable table;
        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.exposure = 1000.0;
        configuration.reliability.reliability_at_warranty = 0.95;
        configuration.reliability.model = "weibull";
        configuration.reliability.warranty_observed_failures = 2;
        configuration.reliability.warranty_censored_count = 1;
        configuration.reliability.warranty_valid_count = 10;
        OutputPage page = AnalysisService::reliability_warranty(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.warranty.has_value());
        QVERIFY2(
            page.facts.warranty->claims_per_1000 > 0.0,
            "warranty summary fixture must carry claims_per_1000");

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_warranty_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"保修摘要", "暴露量", "失效模式分母追溯", "claims/1000 = 1000"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Warranty summary"));
        };
        assert_warranty_en(customer_en, "customer en-US");
        assert_warranty_en(engineer_en, "engineer en-US");
        assert_warranty_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(!engineer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        bool engineer_summary_table = false;
        bool engineer_stratum_table = false;
        for (const auto& table_view : engineer_en.pages[0].visible_tables) {
            if (table_view.title.find("Warranty summary") != std::string::npos) {
                engineer_summary_table = true;
            }
            if (table_view.title.find("Failure-mode denominator trace") != std::string::npos) {
                engineer_stratum_table = true;
            }
        }
        QVERIFY2(
            engineer_summary_table,
            "engineer en-US should expose localized warranty summary table");
        QVERIFY2(
            !engineer_stratum_table,
            "scalar warranty summary must not expose stratum denominator table");

        const auto has_warranty_gate = [](const datalab::domain::ReportPageView& page,
                                          const char* gate_suffix) {
            for (const auto& ref : page.visible_evidence) {
                if (ref.evidence_id.find(std::string(":gate:") + gate_suffix)
                        != std::string::npos) {
                    return true;
                }
            }
            return false;
        };
        QVERIFY2(
            has_warranty_gate(customer_en.pages[0], "warranty_not_legal_promise"),
            "customer en-US keeps warranty_not_legal_promise limiting gate ref");
        QVERIFY2(
            has_warranty_gate(
                customer_en.pages[0],
                "warranty_prediction_not_observation"),
            "customer en-US keeps warranty_prediction_not_observation limiting gate ref");
        QVERIFY2(
            has_warranty_gate(audit_en.pages[0], "warranty_not_legal_promise"),
            "audit en-US keeps warranty_not_legal_promise limiting gate ref");
        QVERIFY2(
            has_warranty_gate(
                audit_en.pages[0],
                "warranty_prediction_not_observation"),
            "audit en-US keeps warranty_prediction_not_observation limiting gate ref");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        const ReportDocument audit_zh =
            localize_profile(ReportTemplateKind::audit, "zh-CN");
        const auto assert_warranty_zh = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("保修摘要"));
            assert_visible_layer_no_english_catalog_leaks(
                doc.pages[0],
                label,
                {"Warranty summary",
                 "Claims per 1000",
                 "Exposure =",
                 "Failure-mode denominator trace",
                 "Evidence appendix"});
        };
        assert_warranty_zh(customer_zh, "customer zh-CN");
        assert_warranty_zh(engineer_zh, "engineer zh-CN");
        assert_warranty_zh(audit_zh, "audit zh-CN");
        bool engineer_summary_table_zh = false;
        for (const auto& table_view : engineer_zh.pages[0].visible_tables) {
            if (table_view.title.find("保修摘要") != std::string::npos) {
                engineer_summary_table_zh = true;
            }
        }
        QVERIFY2(
            engineer_summary_table_zh,
            "engineer zh-CN should expose Chinese warranty summary table title");
    }

    void representative_johnson_spec_outside_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 6 CAP-NN-3: Johnson spec-outside gate × customer/engineer/audit × en-US guards.
        std::vector<double> values;
        for (int i = 1; i <= 40; ++i) {
            values.push_back(std::exp(0.05 * static_cast<double>(i))
                + 0.1 * static_cast<double>(i % 3));
        }
        DataTable table;
        table.columns = {"x"};
        for (double value : values) {
            table.rows.push_back({std::to_string(value)});
        }
        AnalysisConfiguration configuration;
        configuration.capability_method = "johnson";
        configuration.variable_columns = {0};
        configuration.specifications.lower = -1000.0;
        OutputPage page = AnalysisService::capability(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.capability.has_value());
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_spec_outside_en = [&](const ReportDocument& doc,
                                                const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"Johnson 变换过程能力", "规格限落在 Johnson", "整体能力",
                 "Overall Capability"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Johnson-transform process capability"));
            bool saw_spec_outside = false;
            for (const auto& diagnostic : doc.pages[0].visible_diagnostics) {
                if (diagnostic.code == "johnson_spec_outside_support") {
                    saw_spec_outside = true;
                    QVERIFY2(
                        diagnostic.message.find("outside the Johnson transform support")
                            != std::string::npos,
                        label);
                }
            }
            QVERIFY2(
                saw_spec_outside,
                "en-US profile must surface localized spec-outside diagnostic");
            for (const auto& table_view : doc.pages[0].visible_tables) {
                QVERIFY2(
                    table_view.title.find("Overall Capability") == std::string::npos,
                    label);
            }
        };
        assert_spec_outside_en(customer_en, "customer en-US");
        assert_spec_outside_en(engineer_en, "engineer en-US");
        assert_spec_outside_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        const auto has_spec_limit_gate = [](const datalab::domain::ReportPageView& page) {
            for (const auto& ref : page.visible_evidence) {
                if (ref.evidence_id.find(":gate:johnson_spec_limit") != std::string::npos) {
                    return true;
                }
            }
            return false;
        };
        QVERIFY2(
            has_spec_limit_gate(customer_en.pages[0]),
            "customer en-US keeps johnson_spec_limit limiting gate ref");
        QVERIFY2(
            has_spec_limit_gate(audit_en.pages[0]),
            "audit en-US keeps johnson_spec_limit limiting gate ref");
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);
    }

    void representative_warranty_exposure_gate_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 5 REL exposure gate × customer/engineer/audit × en-US visible-layer guards.
        DataTable invalid_table;
        invalid_table.columns = {"time", "censor_type", "exposure"};
        invalid_table.rows = {
            {"10", "exact", "1.5"},
            {"15", "right", "-1"},
            {"20", "exact", "1.0"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.exposure = 9999.0;
        configuration.reliability.reliability_at_warranty = 0.98;
        configuration.reliability.exposure_column = 2;
        configuration.reliability.model = "weibull";
        OutputPage page =
            AnalysisService::reliability_warranty(invalid_table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(!page.diagnostics.empty());
        QCOMPARE(
            QString::fromStdString(page.diagnostics.front().code),
            QStringLiteral("invalid_exposure_value"));
        QVERIFY(!page.facts.warranty.has_value());

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(invalid_table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_exposure_gate_en = [&](const ReportDocument& doc,
                                                 const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"暴露量", "静默补齐", "Claims per 1000"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Warranty summary"));
            bool saw_invalid_exposure = false;
            for (const auto& diagnostic : doc.pages[0].visible_diagnostics) {
                if (diagnostic.code == "invalid_exposure_value") {
                    saw_invalid_exposure = true;
                    QVERIFY2(
                        diagnostic.message.find("not silently imputed")
                            != std::string::npos,
                        label);
                }
            }
            QVERIFY2(
                saw_invalid_exposure,
                "en-US profile must surface localized invalid exposure diagnostic");
            bool saw_limitation = false;
            for (const auto& section : doc.pages[0].visible_interpretation) {
                for (const auto& bullet : section.bullets) {
                    if (bullet.find("warranty summary metrics were skipped")
                            != std::string::npos) {
                        saw_limitation = true;
                    }
                }
            }
            QVERIFY2(
                saw_limitation,
                "en-US profile must surface localized warranty exposure gate limitation");
            QVERIFY(doc.pages[0].visible_tables.empty());
        };
        assert_exposure_gate_en(customer_en, "customer en-US");
        assert_exposure_gate_en(engineer_en, "engineer en-US");
        assert_exposure_gate_en(audit_en, "audit en-US");

        bool saw_exposure_gate = false;
        for (const auto& ref : audit_en.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:warranty_exposure") != std::string::npos) {
                saw_exposure_gate = true;
                QCOMPARE(ref.label_text_id, std::string("evidence.warranty_exposure_gate"));
                QCOMPARE(ref.diagnostic_code, std::string("invalid_exposure_value"));
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
            }
        }
        QVERIFY2(
            saw_exposure_gate,
            "audit en-US must include warranty exposure limiting gate ref");
        QVERIFY(!customer_en.pages[0].show_evidence_appendix);
        QVERIFY(audit_en.pages[0].show_evidence_appendix);

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        QCOMPARE(
            customer_zh.pages[0].source_page.title,
            std::string("保修摘要"));
        bool saw_zh_invalid = false;
        for (const auto& diagnostic : customer_zh.pages[0].visible_diagnostics) {
            if (diagnostic.code == "invalid_exposure_value") {
                saw_zh_invalid = true;
                QVERIFY2(
                    diagnostic.message.find("静默补齐") != std::string::npos,
                    "zh-CN must localize invalid exposure diagnostic");
            }
        }
        QVERIFY2(
            saw_zh_invalid,
            "zh-CN customer must surface localized invalid exposure diagnostic");
    }

    void representative_warranty_exposure_column_override_three_report_profiles_localize_without_cross_language_leak()
    {
        // Phase 5 REL: exposure column overrides scalar × customer/engineer/audit × en/zh.
        DataTable table;
        table.columns = {"time", "censor_type", "exposure"};
        table.rows = {
            {"10", "exact", "1.5"},
            {"15", "right", "2.5"},
            {"20", "exact", "1.0"}};
        AnalysisConfiguration configuration;
        configuration.chart_type = "reliability_warranty";
        configuration.reliability.warranty_time = 1000.0;
        configuration.reliability.time_unit = "hours";
        configuration.reliability.exposure = 9999.0;
        configuration.reliability.reliability_at_warranty = 0.98;
        configuration.reliability.exposure_column = 2;
        configuration.reliability.model = "weibull";
        OutputPage page =
            AnalysisService::reliability_warranty(table, configuration);
        InterpretationService::enrich(page);
        QVERIFY(page.facts.warranty.has_value());
        QCOMPARE(
            QString::fromStdString(page.facts.warranty->exposure_source),
            QStringLiteral("column_sum"));
        QVERIFY(qAbs(page.facts.warranty->exposure - 5.0) < 1e-9);

        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-22T12:00:00Z";
        options.software_version = "DataLab";

        const auto localize_profile = [&](ReportTemplateKind kind,
                                          const char* language_tag) -> ReportDocument {
            auto profile = make_report_profile(kind);
            profile.locale.language_tag = language_tag;
            ReportDocument document =
                build_report_document(table, {page}, profile, options);
            return localize_report_document(document).document;
        };

        const ReportDocument customer_en =
            localize_profile(ReportTemplateKind::customer, "en-US");
        const ReportDocument engineer_en =
            localize_profile(ReportTemplateKind::engineer, "en-US");
        const ReportDocument audit_en =
            localize_profile(ReportTemplateKind::audit, "en-US");
        QCOMPARE(
            customer_en.provenance.facts_hash,
            engineer_en.provenance.facts_hash);
        QCOMPARE(engineer_en.provenance.facts_hash, audit_en.provenance.facts_hash);

        const auto assert_override_en = [&](const ReportDocument& doc, const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"保修摘要", "标量被忽略", "同时提供了暴露量列"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Warranty summary"));
            bool saw_override = false;
            for (const auto& diagnostic : doc.pages[0].visible_diagnostics) {
                if (diagnostic.code == "warranty_exposure_column_overrides_scalar") {
                    saw_override = true;
                    QVERIFY2(
                        diagnostic.message.find("column sum and ignores the scalar")
                            != std::string::npos,
                        label);
                }
            }
            QVERIFY2(
                saw_override,
                "en-US profile must surface localized exposure override diagnostic");
        };
        const auto assert_customer_override_en = [&](const ReportDocument& doc,
                                                     const char* label) {
            QVERIFY(!doc.pages.empty());
            assert_visible_layer_no_cjk(doc.pages[0], label);
            assert_visible_layer_excludes_substrings(
                doc.pages[0],
                label,
                {"保修摘要", "标量被忽略", "同时提供了暴露量列"});
            QCOMPARE(
                doc.pages[0].source_page.title,
                std::string("Warranty summary"));
            for (const auto& diagnostic : doc.pages[0].visible_diagnostics) {
                QVERIFY2(
                    diagnostic.code != "warranty_exposure_column_overrides_scalar",
                    "customer en-US must not surface info-level override diagnostic");
            }
        };
        assert_customer_override_en(customer_en, "customer en-US");
        assert_override_en(engineer_en, "engineer en-US");
        assert_override_en(audit_en, "audit en-US");

        QVERIFY(customer_en.pages[0].visible_tables.empty());
        QVERIFY(!engineer_en.pages[0].visible_tables.empty());
        QVERIFY(!audit_en.pages[0].visible_tables.empty());

        const auto has_warranty_gate = [](const datalab::domain::ReportPageView& page,
                                          const char* gate_suffix) {
            for (const auto& ref : page.visible_evidence) {
                if (ref.evidence_id.find(std::string(":gate:") + gate_suffix)
                        != std::string::npos) {
                    return true;
                }
            }
            return false;
        };
        QVERIFY2(
            has_warranty_gate(customer_en.pages[0], "warranty_not_legal_promise"),
            "customer en-US keeps warranty_not_legal_promise limiting gate ref");

        const ReportDocument customer_zh =
            localize_profile(ReportTemplateKind::customer, "zh-CN");
        QCOMPARE(
            customer_zh.pages[0].source_page.title,
            std::string("保修摘要"));
        for (const auto& diagnostic : customer_zh.pages[0].visible_diagnostics) {
            QVERIFY2(
                diagnostic.code != "warranty_exposure_column_overrides_scalar",
                "customer zh-CN must not surface info-level override diagnostic");
        }

        const ReportDocument engineer_zh =
            localize_profile(ReportTemplateKind::engineer, "zh-CN");
        bool saw_zh_override = false;
        for (const auto& diagnostic : engineer_zh.pages[0].visible_diagnostics) {
            if (diagnostic.code == "warranty_exposure_column_overrides_scalar") {
                saw_zh_override = true;
                QVERIFY2(
                    diagnostic.message.find("标量被忽略") != std::string::npos,
                    "zh-CN engineer must localize exposure override diagnostic");
            }
        }
        QVERIFY2(
            saw_zh_override,
            "zh-CN engineer must surface localized exposure override diagnostic");
    }
};

QTEST_MAIN(ReportExportPhase2Test)
#include "report_export_phase2_test.moc"
