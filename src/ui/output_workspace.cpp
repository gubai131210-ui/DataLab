#include "ui/output_workspace.h"

#include "ui/analysis_chart_widget.h"
#include "ui/page_renderer.h"
#include "reporting/chart_adapter.h"

#include <QFrame>
#include <QIcon>
#include <QInputDialog>
#include <QLineEdit>
#include <QMouseEvent>
#include <QScrollArea>
#include <QTabBar>

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
    const std::size_t page_index = pages_.size() - 1;
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    page_renderer::PageRenderOptions options;
    options.interactive_charts = true;
    options.include_method = true;
    options.on_display_properties_changed =
        [this, page_index](std::size_t plot_index, const ChartModel& model) {
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
        };
    options.on_rows_selected = [this](const std::vector<std::size_t>& rows) {
        emit rows_selected(rows);
    };
    scroll->setWidget(page_renderer::build_page_widget(page, scroll, options));
    const int index = addTab(
        scroll,
        QIcon(page_renderer::icon_resource(page.id)),
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
