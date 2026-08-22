#include "ui/report_preview_dialog.h"

#include "domain/report_text_catalog.h"
#include "ui/page_renderer.h"

#include <QDialogButtonBox>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStringList>
#include <QVBoxLayout>

namespace {

QString tr_doc(const datalab::domain::ReportDocument& document, const char* id)
{
    return QString::fromStdString(
        datalab::domain::resolve_report_text(id, document.profile.locale.language_tag).text);
}

QString template_label(const datalab::domain::ReportDocument& document)
{
    return QString::fromStdString(datalab::domain::localize_template_kind(
        document.profile.template_kind, document.profile.locale.language_tag));
}

QString evidence_level_label(const datalab::domain::ReportDocument& document)
{
    if (document.profile.include_full_evidence_appendix) {
        return tr_doc(document, "report.evidence.audit");
    }
    if (document.profile.include_rule_evidence) {
        return tr_doc(document, "report.evidence.engineer");
    }
    return tr_doc(document, "report.evidence.customer");
}

void populate_preview_layout(
    QVBoxLayout* layout,
    QWidget* content,
    const datalab::domain::ReportDocument& document)
{
    auto* meta = new QLabel(
        tr_doc(document, "report.template") + QStringLiteral("：") + template_label(document)
            + QStringLiteral("    ") + tr_doc(document, "report.language") + QStringLiteral("：")
            + QString::fromStdString(document.profile.locale.language_tag) + QStringLiteral("    ")
            + tr_doc(document, "report.evidence_level") + QStringLiteral("：")
            + evidence_level_label(document) + QStringLiteral("\n")
            + tr_doc(document, "report.source") + QStringLiteral("：")
            + QString::fromStdString(
                document.provenance.source_path.empty() ? document.provenance.source_dataset_id
                                                        : document.provenance.source_path)
            + QStringLiteral("    N=")
            + QString::number(static_cast<qulonglong>(document.provenance.row_count_n))
            + QStringLiteral("    ") + tr_doc(document, "report.filter") + QStringLiteral("：")
            + (document.provenance.filter_summary.empty()
                   ? tr_doc(document, "report.filter.none")
                   : QString::fromStdString(document.provenance.filter_summary))
            + QStringLiteral("\n")
            + tr_doc(document, "report.software_version") + QStringLiteral("：")
            + QString::fromStdString(
                document.provenance.software_version.empty() ? document.software_version
                                                              : document.provenance.software_version)
            + QStringLiteral("    ") + tr_doc(document, "report.generated_at")
            + QStringLiteral("：")
            + (document.provenance.generated_at_utc.empty()
                   ? tr_doc(document, "report.unrecorded")
                   : QString::fromStdString(document.provenance.generated_at_utc)),
        content);
    meta->setWordWrap(true);
    layout->addWidget(meta);

    for (std::size_t index = 0; index < document.pages.size(); ++index) {
        const datalab::domain::ReportPageView& view = document.pages[index];
        datalab::domain::OutputPage display = view.source_page;
        display.interpretation = view.visible_interpretation;
        display.tables = view.visible_tables;
        display.plots = view.visible_plots;
        display.diagnostics = view.visible_diagnostics;
        if (!view.show_parameter_summary) {
            display.parameter_summary.clear();
        }

        page_renderer::PageRenderOptions options;
        options.include_method = view.show_parameter_summary || view.show_method_metadata;
        options.chart_language_tag = document.profile.locale.language_tag;
        layout->addWidget(page_renderer::build_page_widget(display, content, options));

        if (view.show_evidence_appendix && !view.visible_evidence.empty()) {
            auto* evidence_label =
                new QLabel(tr_doc(document, "ui.preview.evidence_summary"), content);
            evidence_label->setStyleSheet(QStringLiteral("font-weight:600;"));
            layout->addWidget(evidence_label);
            for (const datalab::domain::EvidenceRef& ref : view.visible_evidence) {
                const std::string lang = document.profile.locale.language_tag;
                layout->addWidget(new QLabel(
                    QString::fromStdString(
                        datalab::domain::format_evidence_ref_display_line(
                            ref, lang, true, nullptr)),
                    content));
            }
        }
        if (!view.visible_rules.empty()) {
            auto* rules_label = new QLabel(tr_doc(document, "ui.preview.rule_evidence"), content);
            rules_label->setStyleSheet(QStringLiteral("font-weight:600;"));
            layout->addWidget(rules_label);
            for (const datalab::domain::RuleEvidence& rule : view.visible_rules) {
                QStringList row_ids;
                for (datalab::domain::RowId row : rule.related_rows) {
                    row_ids.append(QString::number(static_cast<qulonglong>(row)));
                }
                layout->addWidget(new QLabel(
                    QStringLiteral("%1 [%2] %3=%4 %5=%6")
                        .arg(QString::fromStdString(rule.id.empty() ? rule.name : rule.id))
                        .arg(QString::fromStdString(rule.status))
                        .arg(tr_doc(document, "report.threshold"))
                        .arg(QString::fromStdString(rule.threshold))
                        .arg(tr_doc(document, "report.anomaly_rows"))
                        .arg(row_ids.isEmpty() ? tr_doc(document, "report.no_row_id")
                                              : row_ids.join(QLatin1Char(','))),
                    content));
            }
        }

        if (index + 1 < document.pages.size()) {
            auto* separator = new QLabel(tr_doc(document, "report.next_page"), content);
            separator->setAlignment(Qt::AlignCenter);
            layout->addWidget(separator);
        }
    }
}

}  // namespace

ReportPreviewDialog::ReportPreviewDialog(
    const datalab::domain::ReportDocument& document,
    QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr_doc(document, "ui.preview.title"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/export-pdf.svg")));
    resize(980, 720);
    auto* outer = new QVBoxLayout(this);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto* content = new QWidget(scroll);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(28, 24, 28, 28);
    layout->setSpacing(14);
    populate_preview_layout(layout, content, document);
    layout->addStretch();
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Cancel | QDialogButtonBox::Ok, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr_doc(document, "ui.preview.confirm"));
    buttons->button(QDialogButtonBox::Cancel)->setText(
        tr_doc(document, "ui.template_dialog.cancel"));
    buttons->button(QDialogButtonBox::Ok)->setIcon(
        QIcon(QStringLiteral(":/icons/export-pdf.svg")));
    buttons->button(QDialogButtonBox::Cancel)->setIcon(
        QIcon(QStringLiteral(":/icons/error.svg")));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    outer->addWidget(buttons);
}

ReportPreviewDialog::ReportPreviewDialog(
    const std::vector<datalab::domain::OutputPage>& pages,
    QWidget* parent)
    : ReportPreviewDialog(
          [&pages] {
              datalab::domain::ReportDocument document;
              document.profile = datalab::domain::make_report_profile(
                  datalab::domain::ReportTemplateKind::engineer);
              document.provenance.source_kind = "worksheet_snapshot";
              for (const datalab::domain::OutputPage& page : pages) {
                  datalab::domain::ReportPageView view;
                  view.source_page = page;
                  view.visible_interpretation = page.interpretation;
                  view.visible_tables = page.tables;
                  view.visible_plots = page.plots;
                  view.visible_diagnostics = page.diagnostics;
                  view.show_parameter_summary = true;
                  view.show_method_metadata = true;
                  document.pages.push_back(std::move(view));
              }
              return document;
          }(),
          parent)
{
}
