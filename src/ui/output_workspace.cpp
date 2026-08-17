#include "ui/output_workspace.h"

#include "ui/analysis_chart_widget.h"
#include "ui/chart_adapter.h"

#include <QLabel>
#include <QScrollArea>
#include <QTableWidget>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QGridLayout>
#include <QFrame>
#include <QFile>
#include <QInputDialog>
#include <QIcon>
#include <QLineEdit>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace {

QString analysis_icon_resource(const std::string& analysis_id)
{
    const QString candidate =
        QStringLiteral(":/icons/%1.svg").arg(QString::fromStdString(analysis_id));
    return QFile::exists(candidate) ? candidate : QStringLiteral(":/icons/report.svg");
}

}  // namespace

OutputWorkspace::OutputWorkspace(QWidget* parent)
    : QTabWidget(parent)
{
    setDocumentMode(true);
    setTabPosition(QTabWidget::North);
    setElideMode(Qt::ElideRight);
    setStyleSheet(QStringLiteral(
        "QTabWidget::pane { border: 1px solid #d6e1e5; background: #ffffff; }"
        "QTabBar::tab { background: #eaf2f5; color: #647b84; padding: 9px 16px;"
        " border: 0; border-right: 1px solid #d6e1e5; }"
        "QTabBar::tab:selected { background: #ffffff; color: #147d85;"
        " border-top: 2px solid #42aeb4; }"));
    tabBar()->installEventFilter(this);
}

bool OutputWorkspace::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == tabBar() && event->type() == QEvent::MouseButtonDblClick) {
        auto* mouse_event = static_cast<QMouseEvent*>(event);
        const int index = tabBar()->tabAt(mouse_event->position().toPoint());
        if (index >= 0 && index < static_cast<int>(pages_.size())) {
            bool accepted = false;
            const QString title = QInputDialog::getText(
                this,
                QStringLiteral("编辑输出页标题"),
                QStringLiteral("标题："),
                QLineEdit::Normal,
                tabText(index),
                &accepted);
            if (accepted && !title.trimmed().isEmpty()) {
                const QString id = QString::fromStdString(pages_[static_cast<std::size_t>(index)].id);
                pages_[static_cast<std::size_t>(index)].title = title.trimmed().toStdString();
                setTabText(index, title.trimmed());
                emit page_title_changed(id, title.trimmed());
            }
            return true;
        }
    }
    return QTabWidget::eventFilter(watched, event);
}

void OutputWorkspace::add_page(const datalab::domain::OutputPage& page)
{
    pages_.push_back(page);
    const int index = addTab(
        build_page_widget(page, pages_.size() - 1),
        QIcon(analysis_icon_resource(page.id)),
        QString::fromStdString(page.title));
    setTabToolTip(index, QString::fromStdString(page.id));
    setCurrentIndex(index);
}

void OutputWorkspace::show_page(const QString& id)
{
    for (int index = 0; index < count(); ++index) {
        if (tabToolTip(index) == id) {
            setCurrentIndex(index);
            return;
        }
    }
}

void OutputWorkspace::clear_pages()
{
    clear();
    pages_.clear();
}

std::vector<datalab::domain::OutputPage> OutputWorkspace::pages() const
{
    return pages_;
}

datalab::domain::OutputPage OutputWorkspace::current_page() const
{
    if (pages_.empty() || currentIndex() < 0
        || currentIndex() >= static_cast<int>(pages_.size())) {
        return {};
    }
    return pages_[static_cast<std::size_t>(currentIndex())];
}

bool OutputWorkspace::has_pages() const
{
    return !pages_.empty();
}

void OutputWorkspace::set_selected_source_rows(const std::vector<std::size_t>& rows)
{
    const auto charts = findChildren<AnalysisChartWidget*>();
    for (AnalysisChartWidget* chart : charts) {
        chart->set_selected_source_rows(rows);
    }
}

QWidget* OutputWorkspace::build_page_widget(
    const datalab::domain::OutputPage& page, std::size_t page_index)
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* container = new QWidget(scroll);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(22, 18, 22, 24);
    layout->setSpacing(14);

    auto* title_row = new QHBoxLayout();
    auto* title_icon = new QLabel(container);
    title_icon->setPixmap(QIcon(analysis_icon_resource(page.id)).pixmap(28, 28));
    title_row->addWidget(title_icon);
    auto* title = new QLabel(QString::fromStdString(page.title), container);
    title->setStyleSheet(QStringLiteral(
        "font-size: 21px; font-weight: 700; color: #20343d; padding: 2px 0 0;"));
    title_row->addWidget(title);
    title_row->addStretch();
    layout->addLayout(title_row);
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
        grid->setSelectionMode(QAbstractItemView::NoSelection);
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
                column, new QTableWidgetItem(QString::fromStdString(table.headers[static_cast<std::size_t>(column)])));
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
        connect(chart, &AnalysisChartWidget::rows_selected, this, &OutputWorkspace::rows_selected);
        connect(chart, &AnalysisChartWidget::display_properties_changed, this,
                [this, page_index, plot_index](const ChartModel& model) {
                    if (page_index >= pages_.size()
                        || plot_index >= pages_[page_index].plots.size()) {
                        return;
                    }
                    auto& target = pages_[page_index].plots[plot_index];
                    target.title = model.title.toStdString();
                    target.subtitle = model.subtitle.toStdString();
                    target.show_grid = model.show_grid;
                    target.show_legend = model.show_legend;
                    target.line_width = model.line_width;
                });
        if (plot_grid != nullptr) {
            plot_grid->addWidget(chart, static_cast<int>(plot_index / 2),
                                 static_cast<int>(plot_index % 2));
        } else {
            layout->addWidget(chart);
        }
    }
    layout->addStretch();
    scroll->setWidget(container);
    return scroll;
}
