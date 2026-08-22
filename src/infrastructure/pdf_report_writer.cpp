#include "infrastructure/pdf_report_writer.h"

#include "domain/report_text_catalog.h"
#include "infrastructure/report_layout_cursor.h"
#include "infrastructure/report_painter.h"
#include "reporting/chart_adapter.h"
#include "reporting/chart_renderer.h"

#include <QDateTime>
#include <QFontDatabase>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QPdfWriter>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <vector>

namespace datalab::infrastructure {
namespace {

QString tr_report(
    const std::string& text_id,
    const std::string& language_tag,
    std::vector<domain::MissingTranslation>* missing)
{
    return QString::fromStdString(
        domain::resolve_report_text(text_id, language_tag, missing).text);
}

QString template_display_name(
    domain::ReportTemplateKind kind,
    const std::string& language_tag,
    std::vector<domain::MissingTranslation>* missing)
{
    return QString::fromStdString(
        domain::localize_template_kind(kind, language_tag, missing));
}

QString evidence_level_text(
    const domain::ReportProfile& profile,
    const std::string& language_tag,
    std::vector<domain::MissingTranslation>* missing)
{
    if (profile.include_full_evidence_appendix) {
        return tr_report("report.evidence.audit", language_tag, missing);
    }
    if (profile.include_rule_evidence) {
        return tr_report("report.evidence.engineer", language_tag, missing);
    }
    return tr_report("report.evidence.customer", language_tag, missing);
}

QString display_cell_text(const std::string& raw)
{
    if (raw.empty()) {
        return QStringLiteral("—");
    }
    bool ok = false;
    const double value = QString::fromStdString(raw).toDouble(&ok);
    if (!ok) {
        return QString::fromStdString(raw);
    }
    if (std::isnan(value)) {
        return QStringLiteral("NaN");
    }
    if (std::isinf(value)) {
        return value > 0.0 ? QStringLiteral("+∞") : QStringLiteral("-∞");
    }
    return QString::fromStdString(raw);
}

QString apply_one_arg(const QString& pattern, const QString& arg)
{
    QString out = pattern;
    out.replace(QStringLiteral("%1"), arg);
    return out;
}

QString format_evidence_appendix_line(
    const domain::EvidenceRef& ref,
    const std::string& language_tag,
    std::vector<domain::MissingTranslation>* missing,
    bool include_row_count)
{
    return QString::fromStdString(domain::format_evidence_ref_display_line(
        ref, language_tag, include_row_count, missing));
}

bool write_with_document(
    const QString& file_path,
    const domain::ReportDocument& document,
    QString* error_message)
{
    std::vector<domain::MissingTranslation> missing;
    const std::string lang = document.profile.locale.language_tag;

    QPdfWriter writer(file_path);
    writer.setResolution(72);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    const QString report_title = tr_report("report.title", lang, &missing)
        + (document.provenance.report_id.empty()
               ? QString()
               : QStringLiteral(" [")
                   + QString::fromStdString(document.provenance.report_id)
                   + QLatin1Char(']'));
    writer.setTitle(report_title);
    writer.setCreator(QString::fromStdString(
        document.software_version.empty() ? "DataLab" : document.software_version));
    // Qt QPdfWriter does not make DataLab PDF/A or PDF/UA compliant by setting metadata.
    QPainter painter(&writer);
    if (!painter.isActive()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("无法创建 PDF 报告。");
        }
        return false;
    }

    const QString family = QFontDatabase().families().contains(QStringLiteral("Microsoft YaHei"))
        ? QStringLiteral("Microsoft YaHei")
        : QStringLiteral("SimSun");
    const QRectF page_rect(90.0, 100.0, writer.width() - 180.0, writer.height() - 190.0);
    ReportLayoutCursor cursor(page_rect, 8.0);
    const QFont body_font(family, 10);
    const QFont heading_font(family, 13, QFont::Bold);
    const QFont title_font(family, 18, QFont::Bold);

    const auto draw_header = [&] {
        painter.setFont(QFont(family, 8));
        painter.setPen(QColor("#68737d"));
        painter.drawText(
            QRectF(90.0, 48.0, writer.width() - 180.0, 36.0),
            Qt::TextWordWrap,
            QStringLiteral("report_id=%1  template=%2@%3  locale=%4  facts_hash=%5")
                .arg(QString::fromStdString(document.provenance.report_id))
                .arg(QString::fromStdString(
                    document.profile.profile_id.empty()
                        ? domain::report_template_kind_id(document.profile.template_kind)
                        : document.profile.profile_id))
                .arg(QString::fromStdString(document.profile.template_version))
                .arg(QString::fromStdString(document.profile.locale.language_tag))
                .arg(QString::fromStdString(document.provenance.facts_hash)));
    };
    const auto draw_footer = [&] {
        painter.setFont(QFont(family, 8));
        painter.setPen(QColor("#68737d"));
        painter.drawText(
            QRectF(90.0, writer.height() - 65.0, writer.width() - 180.0, 20.0),
            Qt::AlignCenter,
            tr_report("report.footer", lang, &missing)
                + QStringLiteral(" · ")
                + template_display_name(document.profile.template_kind, lang, &missing)
                + QStringLiteral(" · ")
                + apply_one_arg(
                      tr_report("report.page", lang, &missing),
                      QString::number(cursor.page_number()))
                + QStringLiteral(" · PDF/A=not_validated · PDF/UA=unsupported"));
    };
    const auto new_page = [&] {
        draw_footer();
        writer.newPage();
        cursor.new_page();
        draw_header();
    };
    const auto ensure_space = [&](double height) {
        if (cursor.needs_page_break(height)) {
            new_page();
        }
    };
    const auto draw_wrapped = [&](const QString& text, const QFont& font, double indent = 0.0) {
        const double height = cursor.measure_text(text, font);
        ensure_space(height);
        painter.setFont(font);
        painter.setPen(Qt::black);
        painter.drawText(
            QRectF(cursor.x() + indent, cursor.y(), page_rect.width() - indent, height),
            Qt::TextWordWrap,
            text);
        cursor.advance(height);
    };

    draw_header();
    draw_wrapped(tr_report("report.title", lang, &missing), title_font);
    draw_wrapped(
        tr_report("report.template", lang, &missing) + QStringLiteral("：")
            + template_display_name(document.profile.template_kind, lang, &missing)
            + QStringLiteral("    ")
            + tr_report("report.language", lang, &missing) + QStringLiteral("：")
            + QString::fromStdString(document.profile.locale.language_tag)
            + QStringLiteral("    ")
            + tr_report("report.evidence_level", lang, &missing) + QStringLiteral("：")
            + evidence_level_text(document.profile, lang, &missing),
        body_font);
    draw_wrapped(
        tr_report("report.generated_at", lang, &missing) + QStringLiteral("：")
            + QString::fromStdString(domain::format_report_datetime_utc(
                  document.provenance.generated_at_utc.empty()
                      ? QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString()
                      : document.provenance.generated_at_utc,
                  document.profile.locale)),
        body_font);
    draw_wrapped(
        tr_report("report.source", lang, &missing) + QStringLiteral("：")
            + QString::fromStdString(
                document.provenance.source_path.empty() ? document.provenance.source_dataset_id
                                                        : document.provenance.source_path),
        body_font);
    draw_wrapped(
        tr_report("report.source_kind", lang, &missing) + QStringLiteral("：")
            + QString::fromStdString(document.provenance.source_kind)
            + QStringLiteral("    N=")
            + QString::number(static_cast<qulonglong>(document.provenance.row_count_n))
            + QStringLiteral("    ")
            + tr_report("report.filter", lang, &missing) + QStringLiteral("：")
            + (document.provenance.filter_summary.empty()
                   ? tr_report("report.filter.none", lang, &missing)
                   : QString::fromStdString(document.provenance.filter_summary)),
        body_font);
    draw_wrapped(
        tr_report("report.software_version", lang, &missing) + QStringLiteral("：")
            + QString::fromStdString(document.provenance.software_version)
            + QStringLiteral("    ")
            + tr_report("report.algorithm_version", lang, &missing) + QStringLiteral("：")
            + (document.provenance.algorithm_version.empty()
                   ? tr_report("report.unrecorded", lang, &missing)
                   : QString::fromStdString(document.provenance.algorithm_version)),
        body_font);

    if (document.profile.include_import_plan) {
        draw_wrapped(
            tr_report("report.import_plan", lang, &missing) + QStringLiteral("：")
                + (document.provenance.import_plan_summary.empty()
                       ? tr_report("report.import_plan.worksheet_fallback", lang, &missing)
                       : QString::fromStdString(document.provenance.import_plan_summary)),
            body_font);
    }
    if (document.profile.include_source_hashes) {
        draw_wrapped(
            tr_report("report.input_hash", lang, &missing) + QStringLiteral("：")
                + QString::fromStdString(document.provenance.input_snapshot_hash),
            body_font);
        draw_wrapped(
            tr_report("report.facts_hash", lang, &missing) + QStringLiteral("：")
                + QString::fromStdString(document.provenance.facts_hash),
            body_font);
    }
    draw_wrapped(tr_report("report.pdfa_banner", lang, &missing), body_font);

    if (!missing.empty()) {
        draw_wrapped(tr_report("diag.missing_translation", lang, nullptr), body_font);
    }

    if (document.pages.empty()) {
        draw_wrapped(tr_report("report.no_results", lang, &missing), body_font);
    }

    for (const domain::ReportPageView& page_view : document.pages) {
        const domain::OutputPage& page = page_view.source_page;
        ensure_space(80.0);
        draw_wrapped(QString::fromStdString(page.title), heading_font);
        if (page_view.show_parameter_summary) {
            draw_wrapped(
                QString::fromStdString(page.method_name + "    " + page.parameter_summary),
                body_font);
        } else {
            draw_wrapped(QString::fromStdString(page.method_name), body_font);
        }
        if (page_view.show_method_metadata) {
            draw_wrapped(
                tr_report("report.method_metadata", lang, &missing) + QStringLiteral("：")
                    + QString::fromStdString(page.method_metadata.algorithm) + QStringLiteral(" @ ")
                    + QString::fromStdString(page.method_metadata.version),
                body_font);
        }

        for (const auto& section : page_view.visible_interpretation) {
            draw_wrapped(
                QStringLiteral("【") + QString::fromStdString(section.heading) + QStringLiteral("】"),
                QFont(family, 11, QFont::Bold));
            for (const auto& bullet : section.bullets) {
                draw_wrapped(QStringLiteral("• ") + QString::fromStdString(bullet), body_font, 8.0);
            }
        }

        for (const auto& table_block : page_view.visible_tables) {
            draw_wrapped(QString::fromStdString(table_block.title), QFont(family, 11, QFont::Bold));
            const QFont header_font(family, 8, QFont::Bold);
            const QFont cell_font(family, 8);
            QFontMetrics cell_metrics(cell_font);
            const std::vector<double> column_widths = ReportPainter::measure_column_widths(
                table_block.headers, table_block.rows, header_font, cell_font, page_rect.width());
            const auto row_height_for = [&](const std::vector<std::string>& row) {
                double height = 24.0;
                for (std::size_t column = 0; column < table_block.headers.size(); ++column) {
                    const QString text = column < row.size()
                        ? QString::fromStdString(row[column])
                        : QString();
                    const QRect bounds = cell_metrics.boundingRect(
                        QRect(0, 0, static_cast<int>(column_widths[column] - 8.0), 1000),
                        Qt::TextWordWrap,
                        text);
                    height = std::max(height, bounds.height() + 8.0);
                }
                return std::min(height, 72.0);
            };
            ensure_space(32.0);
            const auto paint_header_row = [&] {
                ensure_space(32.0);
                painter.setFont(header_font);
                double x = page_rect.left();
                for (std::size_t column = 0; column < table_block.headers.size(); ++column) {
                    const QRectF cell(x, cursor.y(), column_widths[column], 32.0);
                    painter.fillRect(cell, QColor("#e8f0f7"));
                    painter.drawRect(cell);
                    painter.drawText(
                        cell.adjusted(4, 4, -4, -4),
                        Qt::TextWordWrap | Qt::AlignVCenter,
                        QString::fromStdString(table_block.headers[column]));
                    x += column_widths[column];
                }
                cursor.advance(32.0);
                painter.setFont(cell_font);
            };
            ReportPainter::paint_paginated_table(
                table_block.rows.size(),
                32.0,
                [&](std::size_t row_index) {
                    return row_height_for(table_block.rows[row_index]);
                },
                [&](double needed) { return cursor.needs_page_break(needed); },
                [&] { new_page(); },
                paint_header_row,
                [&](std::size_t row_index) {
                    const auto& row = table_block.rows[row_index];
                    const double row_height = row_height_for(row);
                    ensure_space(row_height);
                    double x = page_rect.left();
                    for (std::size_t column = 0; column < table_block.headers.size(); ++column) {
                        const QRectF cell(x, cursor.y(), column_widths[column], row_height);
                        painter.drawRect(cell);
                        const QString text = column < row.size()
                            ? display_cell_text(row[column])
                            : QStringLiteral("—");
                        painter.drawText(
                            cell.adjusted(4, 4, -4, -4),
                            Qt::TextWordWrap | Qt::AlignVCenter,
                            text);
                        x += column_widths[column];
                    }
                    cursor.advance(row_height);
                });
        }
        if (page_view.truncated_table_row_count > 0) {
            draw_wrapped(
                apply_one_arg(
                    tr_report("report.table_truncated", lang, &missing),
                    QString::number(static_cast<qulonglong>(page_view.truncated_table_row_count))),
                body_font);
        }

        for (const auto& diagnostic : page_view.visible_diagnostics) {
            draw_wrapped(
                QStringLiteral("• ") + QString::fromStdString(diagnostic.message), body_font, 8.0);
        }

        if (document.profile.include_rule_evidence) {
            for (const domain::RuleEvidence& rule : page_view.visible_rules) {
                QStringList row_ids;
                for (domain::RowId row : rule.related_rows) {
                    row_ids.append(QString::number(static_cast<qulonglong>(row)));
                }
                draw_wrapped(
                    tr_report("report.rule_line", lang, &missing) + QStringLiteral(" ")
                        + QString::fromStdString(rule.id.empty() ? rule.name : rule.id)
                        + QStringLiteral(" [") + QString::fromStdString(rule.status)
                        + QStringLiteral("] ")
                        + tr_report("report.threshold", lang, &missing) + QStringLiteral("=")
                        + QString::fromStdString(rule.threshold) + QStringLiteral(" ")
                        + tr_report("report.anomaly_rows", lang, &missing) + QStringLiteral("=")
                        + (row_ids.isEmpty() ? tr_report("report.no_row_id", lang, &missing)
                                             : row_ids.join(QLatin1Char(',')))
                        + QStringLiteral("：") + QString::fromStdString(rule.message),
                    body_font,
                    8.0);
            }
        }

        if (document.profile.include_formula_references && !page_view.show_evidence_appendix) {
            // Engineer summary: formula refs only (not full evidence dump).
            bool drew_heading = false;
            for (const domain::EvidenceRef& ref : page_view.visible_evidence) {
                if (ref.kind != domain::EvidenceKind::formula_reference) {
                    continue;
                }
                if (!drew_heading) {
                    draw_wrapped(
                        tr_report("report.formula_references", lang, &missing),
                        QFont(family, 11, QFont::Bold));
                    drew_heading = true;
                }
                draw_wrapped(
                    format_evidence_appendix_line(ref, lang, &missing, false),
                    body_font,
                    8.0);
            }
        }

        if (page_view.show_evidence_appendix) {
            draw_wrapped(
                tr_report("report.evidence_appendix", lang, &missing),
                QFont(family, 11, QFont::Bold));
            for (const domain::EvidenceRef& ref : page_view.visible_evidence) {
                draw_wrapped(
                    format_evidence_appendix_line(ref, lang, &missing, true),
                    body_font,
                    8.0);
            }
            if (page_view.truncated_evidence_count > 0) {
                draw_wrapped(
                    apply_one_arg(
                        tr_report("report.evidence_truncated", lang, &missing),
                        QString::number(
                            static_cast<qulonglong>(page_view.truncated_evidence_count))),
                    body_font);
            }
        }

        for (const auto& plot : page_view.visible_plots) {
            const QRectF chart_bounds(page_rect.left(), cursor.y(), page_rect.width(), 340.0);
            const QRectF chart_rect = ReportPainter::contain(chart_bounds, 16.0 / 9.0);
            ensure_space(chart_rect.height() + 18.0);
            ChartModel chart_model = chart_model_from_plot(plot);
            chart_model.language_tag = lang;
            ChartRenderer::render(painter, chart_rect, chart_model);
            cursor.advance(chart_rect.height() + 18.0);
        }
    }

    draw_footer();
    painter.end();
    return true;
}

}  // namespace

bool PdfReportWriter::write(
    const QString& file_path,
    const domain::DataTable& table,
    const std::vector<domain::OutputPage>& pages,
    QString* error_message)
{
    domain::ReportProfile profile =
        domain::make_report_profile(domain::ReportTemplateKind::engineer);
    domain::ReportDocument document;
    document.schema_version = 1;
    document.profile = profile;
    document.software_version = "DataLab";
    document.provenance.source_path = table.source_path;
    document.provenance.source_dataset_id = table.import_metadata.dataset_id.empty()
        ? table.name
        : table.import_metadata.dataset_id;
    document.provenance.source_kind = "worksheet_snapshot";
    document.provenance.row_count_n = table.rows.size();
    document.provenance.column_count = table.columns.size();
    document.provenance.filter_summary = table.import_metadata.filter_summary;
    document.provenance.generated_at_utc =
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
    document.provenance.software_version = "DataLab";

    for (const domain::OutputPage& page : pages) {
        domain::ReportPageView view;
        view.source_page = page;
        view.visible_interpretation = page.interpretation;
        view.visible_tables = page.tables;
        view.visible_plots = page.plots;
        view.visible_diagnostics = page.diagnostics;
        view.show_parameter_summary = true;
        view.show_method_metadata = true;
        document.pages.push_back(std::move(view));
    }
    return write_with_document(file_path, document, error_message);
}

bool PdfReportWriter::write_document(
    const QString& file_path,
    const domain::ReportDocument& document,
    QString* error_message)
{
    return write_with_document(file_path, document, error_message);
}

}  // namespace datalab::infrastructure
