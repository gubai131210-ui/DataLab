#include "ui/report_preview_dialog.h"

#include "ui/analysis_chart_widget.h"
#include "ui/chart_adapter.h"

#include <QDialogButtonBox>
#include <QFile>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

QString analysis_icon_resource(const std::string& analysis_id)
{
    const QString candidate =
        QStringLiteral(":/icons/%1.svg").arg(QString::fromStdString(analysis_id));
    return QFile::exists(candidate) ? candidate : QStringLiteral(":/icons/report.svg");
}

}  // namespace

ReportPreviewDialog::ReportPreviewDialog(
    const std::vector<datalab::domain::OutputPage>& pages,
    QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("PDF 导出预览"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/export-pdf.svg")));
    resize(980, 720);
    auto* outer = new QVBoxLayout(this);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto* content = new QWidget(scroll);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(28, 24, 28, 28);
    layout->setSpacing(14);

    for (const auto& page : pages) {
        auto* title_row = new QHBoxLayout();
        auto* title_icon = new QLabel(content);
        title_icon->setPixmap(QIcon(analysis_icon_resource(page.id)).pixmap(24, 24));
        title_row->addWidget(title_icon);
        auto* title = new QLabel(QString::fromStdString(page.title), content);
        title->setStyleSheet(QStringLiteral(
            "font-size: 21px; font-weight: 700; color:#20343d; padding-top:4px;"));
        title_row->addWidget(title);
        title_row->addStretch();
        layout->addLayout(title_row);
        if (!page.parameter_summary.empty()) {
            auto* summary = new QLabel(QString::fromStdString(page.parameter_summary), content);
            summary->setWordWrap(true);
            summary->setStyleSheet(QStringLiteral(
                "background:#f3f8f9; color:#526a73; border:1px solid #d8e7e9;"
                " border-radius:8px; padding:10px;"));
            layout->addWidget(summary);
        }
        for (const auto& section : page.interpretation) {
            auto* interpretation = new QLabel(content);
            QString text = QStringLiteral("【%1】\n")
                .arg(QString::fromStdString(section.heading));
            for (const auto& bullet : section.bullets) {
                text += QStringLiteral("• ") + QString::fromStdString(bullet)
                    + QLatin1Char('\n');
            }
            interpretation->setText(text.trimmed());
            interpretation->setWordWrap(true);
            const QString background =
                section.severity == datalab::domain::DiagnosticMessage::Severity::error
                    ? QStringLiteral("#fff0f0")
                    : section.severity == datalab::domain::DiagnosticMessage::Severity::warning
                        ? QStringLiteral("#fff8e6") : QStringLiteral("#eef8f3");
            interpretation->setStyleSheet(QStringLiteral(
                "background:%1; color:#49636d; border:1px solid #d6e1e5;"
                " border-radius:8px; padding:10px;").arg(background));
            layout->addWidget(interpretation);
        }
        for (const auto& table : page.tables) {
            auto* caption = new QLabel(QString::fromStdString(table.title), content);
            caption->setStyleSheet(QStringLiteral(
                "font-weight: 650; color:#29434e; padding-top:6px;"));
            layout->addWidget(caption);
            auto* grid = new QTableWidget(
                static_cast<int>(table.rows.size()),
                static_cast<int>(table.headers.size()), content);
            grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
            grid->setWordWrap(true);
            grid->setShowGrid(false);
            grid->verticalHeader()->setVisible(false);
            grid->verticalHeader()->setDefaultSectionSize(28);
            grid->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
            grid->horizontalHeader()->setMinimumSectionSize(64);
            grid->horizontalHeader()->setMaximumSectionSize(220);
            grid->horizontalHeader()->setStretchLastSection(true);
            grid->setStyleSheet(QStringLiteral(
                "QTableWidget { background:#ffffff; alternate-background-color:#f6fafb;"
                " border:1px solid #d6e1e5; }"
                "QHeaderView::section { background:#e7f0f3; color:#29434e;"
                " border:0; padding:7px; font-weight:600; }"));
            for (int column = 0; column < static_cast<int>(table.headers.size()); ++column) {
                grid->setHorizontalHeaderItem(
                    column,
                    new QTableWidgetItem(QString::fromStdString(
                        table.headers[static_cast<std::size_t>(column)])));
            }
            for (int row = 0; row < static_cast<int>(table.rows.size()); ++row) {
                for (int column = 0; column < static_cast<int>(table.headers.size()); ++column) {
                    const auto& cells = table.rows[static_cast<std::size_t>(row)];
                    if (column < static_cast<int>(cells.size())) {
                        const QString text = QString::fromStdString(
                            cells[static_cast<std::size_t>(column)]);
                        auto* item = new QTableWidgetItem(text);
                        item->setTextAlignment(
                            text.contains(QRegularExpression(QStringLiteral(
                                "^\\s*[+\\-]?(?:\\d|\\.)")))
                                ? Qt::AlignRight | Qt::AlignVCenter
                                : Qt::AlignLeft | Qt::AlignVCenter);
                        grid->setItem(row, column, item);
                    }
                }
            }
            grid->resizeRowsToContents();
            layout->addWidget(grid);
        }
        if (!page.diagnostics.empty()) {
            auto* caption = new QLabel(QStringLiteral("诊断信息"), content);
            caption->setStyleSheet(QStringLiteral(
                "font-weight:650; color:#29434e; padding-top:6px;"));
            layout->addWidget(caption);
            for (const auto& diagnostic : page.diagnostics) {
                auto* diagnostic_row = new QHBoxLayout();
                auto* diagnostic_icon = new QLabel(content);
                const QString icon = diagnostic.severity
                    == datalab::domain::DiagnosticMessage::Severity::error
                    ? QStringLiteral("error")
                    : diagnostic.severity
                        == datalab::domain::DiagnosticMessage::Severity::warning
                        ? QStringLiteral("warning") : QStringLiteral("success");
                diagnostic_icon->setPixmap(
                    QIcon(QStringLiteral(":/icons/%1.svg").arg(icon)).pixmap(18, 18));
                diagnostic_row->addWidget(diagnostic_icon);
                auto* message = new QLabel(
                    QString::fromStdString(diagnostic.message), content);
                message->setWordWrap(true);
                const QString background =
                    diagnostic.severity == datalab::domain::DiagnosticMessage::Severity::error
                        ? QStringLiteral("#fff0f0")
                        : diagnostic.severity == datalab::domain::DiagnosticMessage::Severity::warning
                            ? QStringLiteral("#fff8e6") : QStringLiteral("#eef8f3");
                message->setStyleSheet(QStringLiteral(
                    "background:%1; color:#4d626a; border:1px solid #d6e1e5;"
                    " border-radius:6px; padding:8px;").arg(background));
                diagnostic_row->addWidget(message, 1);
                layout->addLayout(diagnostic_row);
            }
        }
        QGridLayout* plot_grid = nullptr;
        if (page.method_name == "Capability Sixpack") {
            plot_grid = new QGridLayout();
            layout->addLayout(plot_grid);
        }
        for (std::size_t plot_index = 0; plot_index < page.plots.size(); ++plot_index) {
            const auto& plot = page.plots[plot_index];
            auto* chart = new AnalysisChartWidget(content);
            chart->set_model(chart_model_from_plot(plot));
            chart->setMinimumHeight(280);
            if (plot_grid != nullptr) {
                plot_grid->addWidget(chart, static_cast<int>(plot_index / 2),
                                     static_cast<int>(plot_index % 2));
            } else {
                layout->addWidget(chart);
            }
        }
        auto* separator = new QLabel(QStringLiteral("— 下一页 —"), content);
        separator->setAlignment(Qt::AlignCenter);
        layout->addWidget(separator);
    }
    layout->addStretch();
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Cancel | QDialogButtonBox::Ok, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认导出"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    buttons->button(QDialogButtonBox::Ok)->setIcon(
        QIcon(QStringLiteral(":/icons/export-pdf.svg")));
    buttons->button(QDialogButtonBox::Cancel)->setIcon(
        QIcon(QStringLiteral(":/icons/error.svg")));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    outer->addWidget(buttons);
}
