#include "ui/page_renderer.h"

#include "ui/analysis_chart_widget.h"
#include "reporting/chart_adapter.h"

#include <QFile>
#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QObject>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTextStream>
#include <QVBoxLayout>

namespace page_renderer {
namespace {

QString table_text(const datalab::domain::StatisticTable& table, QChar separator)
{
    QStringList lines;
    auto encode = [separator](const std::string& value) {
        QString text = QString::fromStdString(value);
        if (text.contains(separator) || text.contains('"') || text.contains('\n')) {
            text.replace('"', QStringLiteral("\"\""));
            text = QStringLiteral("\"") + text + QStringLiteral("\"");
        }
        return text;
    };
    QStringList headers;
    for (const auto& header : table.headers) {
        headers.push_back(encode(header));
    }
    lines.push_back(headers.join(separator));
    for (const auto& row : table.rows) {
        QStringList cells;
        for (std::size_t column = 0; column < table.headers.size(); ++column) {
            cells.push_back(encode(column < row.size() ? row[column] : std::string()));
        }
        lines.push_back(cells.join(separator));
    }
    return lines.join(QLatin1Char('\n'));
}

}  // namespace

QString icon_resource(const std::string& analysis_id)
{
    const QString candidate =
        QStringLiteral(":/icons/%1.svg").arg(QString::fromStdString(analysis_id));
    return QFile::exists(candidate) ? candidate : QStringLiteral(":/icons/report.svg");
}

QWidget* build_page_widget(
    const datalab::domain::OutputPage& page,
    QWidget* parent,
    const PageRenderOptions& options)
{
    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(22, 18, 22, 24);
    layout->setSpacing(14);

    auto* title_row = new QHBoxLayout();
    auto* title_icon = new QLabel(container);
    title_icon->setPixmap(QIcon(icon_resource(page.id)).pixmap(28, 28));
    title_row->addWidget(title_icon);
    auto* title = new QLabel(QString::fromStdString(page.title), container);
    title->setStyleSheet(QStringLiteral(
        "font-size: 21px; font-weight: 700; color: #20343d; padding: 2px 0 0;"));
    title_row->addWidget(title);
    title_row->addStretch();
    layout->addLayout(title_row);

    if (options.include_method) {
        auto* method_card = new QFrame(container);
        method_card->setStyleSheet(QStringLiteral(
            "QFrame { background:#f3f8f9; border:1px solid #d8e7e9; border-radius:8px; }"
            "QLabel { color:#526a73; padding:2px 0; }"));
        auto* method_layout = new QVBoxLayout(method_card);
        method_layout->setContentsMargins(12, 9, 12, 9);
        auto* method = new QLabel(
            QStringLiteral("<b>%1</b><br/>%2")
                .arg(QString::fromStdString(page.method_name),
                     QString::fromStdString(page.parameter_summary)),
            method_card);
        method->setTextFormat(Qt::RichText);
        method->setWordWrap(true);
        method_layout->addWidget(method);
        layout->addWidget(method_card);
    } else if (!page.parameter_summary.empty()) {
        auto* summary = new QLabel(
            QString::fromStdString(page.parameter_summary), container);
        summary->setWordWrap(true);
        summary->setStyleSheet(QStringLiteral(
            "background:#f3f8f9; color:#526a73; border:1px solid #d8e7e9;"
            " border-radius:8px; padding:10px;"));
        layout->addWidget(summary);
    }

    for (const auto& section : page.interpretation) {
        auto* card = new QLabel(container);
        QString text = QStringLiteral("【%1】\n").arg(QString::fromStdString(section.heading));
        for (const auto& bullet : section.bullets) {
            text += QStringLiteral("• ") + QString::fromStdString(bullet) + QLatin1Char('\n');
        }
        card->setText(text.trimmed());
        card->setWordWrap(true);
        const QString color = section.severity == datalab::domain::DiagnosticMessage::Severity::error
            ? QStringLiteral("#fff0f0")
            : section.severity == datalab::domain::DiagnosticMessage::Severity::warning
                ? QStringLiteral("#fff8e6") : QStringLiteral("#eaf8f2");
        card->setStyleSheet(QStringLiteral(
            "background:%1; color:#49636d; padding:11px 13px; border:1px solid #d6e1e5;"
            " border-radius:8px;").arg(color));
        layout->addWidget(card);
    }

    for (const auto& table : page.tables) {
        auto* caption = new QLabel(QString::fromStdString(table.title), container);
        caption->setStyleSheet(QStringLiteral("font-weight: 600; margin-top: 8px;"));
        layout->addWidget(caption);
        auto* grid = new QTableWidget(
            static_cast<int>(table.rows.size()),
            static_cast<int>(table.headers.size()),
            container);
        grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
        grid->setSelectionMode(QAbstractItemView::ContiguousSelection);
        grid->setSelectionBehavior(QAbstractItemView::SelectItems);
        grid->setContextMenuPolicy(Qt::ActionsContextMenu);
        auto* copy_table = new QAction(QStringLiteral("复制表格（TSV）"), grid);
        QObject::connect(copy_table, &QAction::triggered, grid, [table] {
            QApplication::clipboard()->setText(table_text(table, QLatin1Char('\t')));
        });
        grid->addAction(copy_table);
        auto* export_table = new QAction(QStringLiteral("导出表格（CSV）"), grid);
        QObject::connect(export_table, &QAction::triggered, grid, [table, grid] {
            const QString path = QFileDialog::getSaveFileName(
                grid, QStringLiteral("导出统计表"),
                QString::fromStdString(table.title) + QStringLiteral(".csv"),
                QStringLiteral("CSV 文件 (*.csv)"));
            if (path.isEmpty()) {
                return;
            }
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream.setEncoding(QStringConverter::Utf8);
                stream << table_text(table, QLatin1Char(','));
            }
        });
        grid->addAction(export_table);
        grid->setWordWrap(true);
        grid->setShowGrid(false);
        grid->setMinimumHeight(42);
        grid->verticalHeader()->setDefaultSectionSize(28);
        grid->verticalHeader()->setVisible(false);
        grid->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        grid->setAlternatingRowColors(true);
        grid->setStyleSheet(QStringLiteral(
            "QTableWidget { background:#ffffff; alternate-background-color:#f6fafb;"
            " gridline-color:#dbe6e9; border:1px solid #d6e1e5; }"
            "QHeaderView::section { background:#e7f0f3; color:#29434e;"
            " border:0; border-right:1px solid #d6e1e5; padding:7px; font-weight:600; }"));
        grid->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        grid->horizontalHeader()->setMinimumSectionSize(64);
        grid->horizontalHeader()->setMaximumSectionSize(220);
        grid->horizontalHeader()->setStretchLastSection(false);
        for (int column = 0; column < static_cast<int>(table.headers.size()); ++column) {
            grid->setHorizontalHeaderItem(
                column, new QTableWidgetItem(QString::fromStdString(
                    table.headers[static_cast<std::size_t>(column)])));
        }
        for (int row = 0; row < static_cast<int>(table.rows.size()); ++row) {
            for (int column = 0; column < static_cast<int>(table.headers.size()); ++column) {
                const auto& cells = table.rows[static_cast<std::size_t>(row)];
                const QString text = column < static_cast<int>(cells.size())
                    ? QString::fromStdString(cells[static_cast<std::size_t>(column)])
                    : QString();
                auto* item = new QTableWidgetItem(text);
                if (text.contains(QRegularExpression(QStringLiteral(
                        "^\\s*[+\\-]?(?:\\d|\\.)")))) {
                    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                } else {
                    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                }
                grid->setItem(row, column, item);
            }
        }
        grid->resizeColumnsToContents();
        grid->resizeRowsToContents();
        grid->setMinimumHeight(24 * static_cast<int>(table.rows.size() + 2));
        layout->addWidget(grid);
    }

    if (!page.diagnostics.empty()) {
        auto* caption = new QLabel(QStringLiteral("诊断信息"), container);
        caption->setStyleSheet(QStringLiteral("font-weight: 600;"));
        layout->addWidget(caption);
        for (const auto& diagnostic : page.diagnostics) {
            auto* diagnostic_row = new QHBoxLayout();
            auto* diagnostic_icon = new QLabel(container);
            const QString icon = diagnostic.severity
                == datalab::domain::DiagnosticMessage::Severity::error
                ? QStringLiteral("error")
                : diagnostic.severity
                    == datalab::domain::DiagnosticMessage::Severity::warning
                    ? QStringLiteral("warning") : QStringLiteral("success");
            diagnostic_icon->setPixmap(
                QIcon(QStringLiteral(":/icons/%1.svg").arg(icon)).pixmap(20, 20));
            diagnostic_row->addWidget(diagnostic_icon);
            auto* message = new QLabel(QString::fromStdString(diagnostic.message), container);
            message->setWordWrap(true);
            const QString background =
                diagnostic.severity == datalab::domain::DiagnosticMessage::Severity::error
                    ? QStringLiteral("#fff0f0")
                    : diagnostic.severity == datalab::domain::DiagnosticMessage::Severity::warning
                        ? QStringLiteral("#fff8e6") : QStringLiteral("#eef8f3");
            message->setStyleSheet(QStringLiteral(
                "background:%1; color:#4d626a; border:1px solid #d6e1e5;"
                " border-radius:6px; padding:8px 10px;").arg(background));
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
        auto* chart = new AnalysisChartWidget(container);
        chart->set_model(chart_model_from_plot(plot));
        chart->setMinimumHeight(280);
        chart->setStyleSheet(QStringLiteral(
            "background:#ffffff; border:1px solid #d6e1e5; border-radius:8px;"));
        if (options.interactive_charts) {
            if (options.on_rows_selected) {
                QObject::connect(chart, &AnalysisChartWidget::rows_selected, chart,
                                 [handler = options.on_rows_selected](
                                     const std::vector<std::size_t>& rows) {
                                     handler(rows);
                                 });
            }
            if (options.on_display_properties_changed) {
                QObject::connect(
                    chart, &AnalysisChartWidget::display_properties_changed, chart,
                    [handler = options.on_display_properties_changed,
                     plot_index](const ChartModel& model) {
                        handler(plot_index, model);
                    });
            }
        }
        if (plot_grid != nullptr) {
            plot_grid->addWidget(chart, static_cast<int>(plot_index / 2),
                                 static_cast<int>(plot_index % 2));
        } else {
            layout->addWidget(chart);
        }
    }
    layout->addStretch();
    return container;
}

}  // namespace page_renderer
