#include "ui/output_workspace.h"

#include "ui/analysis_chart_widget.h"
#include "ui/page_renderer.h"
#include "reporting/chart_adapter.h"

#include <QFrame>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTabBar>

OutputWorkspace::OutputWorkspace(QWidget* parent)
    : QTabWidget(parent)
{
    setDocumentMode(true);
    setTabPosition(QTabWidget::North);
    setElideMode(Qt::ElideRight);
    setTabsClosable(true);
    setStyleSheet(QStringLiteral(
        "QTabWidget::pane { border: 1px solid #d7e3e6; background: #ffffff;"
        " border-radius: 7px; }"
        "QTabBar { background: #e8f0f2; qproperty-drawBase: 0; }"
        "QTabBar::tab { background: transparent; color: #647b84; padding: 9px 16px;"
        " margin: 3px 2px 0 0; border: 0; border-radius: 6px 6px 0 0; }"
        "QTabBar::tab:hover { background: #dceced; color: #2d6971; }"
        "QTabBar::tab:selected { background: #ffffff; color: #147d85;"
        " font-weight: 600; border-top: 2px solid #35a6aa; }"));
    tabBar()->installEventFilter(this);
    connect(this, &QTabWidget::tabCloseRequested, this, &OutputWorkspace::close_page_at);

    empty_label_ = new QLabel(this);
    empty_label_->setAlignment(Qt::AlignCenter);
    empty_label_->setWordWrap(true);
    empty_label_->setText(QStringLiteral(
        "导入数据后，从“统计 / 图形 / 质量工具”选择方法。"));
    empty_label_->setStyleSheet(QStringLiteral(
        "background: #ffffff; color: #647b84; font-size: 14px;"
        " border: 1px dashed #d7e3e6; border-radius: 7px; padding: 24px;"));
    update_empty_state();
}

void OutputWorkspace::resizeEvent(QResizeEvent* event)
{
    QTabWidget::resizeEvent(event);
    update_empty_state();
}

void OutputWorkspace::update_empty_state()
{
    const bool empty = pages_.empty();
    if (tabBar() != nullptr) {
        tabBar()->setVisible(!empty);
    }
    if (empty_label_ == nullptr) {
        return;
    }
    empty_label_->setVisible(empty);
    if (empty) {
        empty_label_->setGeometry(rect().adjusted(8, 8, -8, -8));
        empty_label_->raise();
    }
}

void OutputWorkspace::close_page_at(int index)
{
    if (index < 0 || index >= static_cast<int>(pages_.size())) {
        return;
    }
    const QString id = QString::fromStdString(pages_[static_cast<std::size_t>(index)].id);
    QWidget* page = widget(index);
    removeTab(index);
    pages_.erase(pages_.begin() + index);
    if (page != nullptr) {
        page->deleteLater();
    }
    update_empty_state();
    emit page_closed(id);
    emit pages_changed();
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
    const std::string page_id = page.id;
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    page_renderer::PageRenderOptions options;
    options.interactive_charts = true;
    options.include_method = true;
    options.on_display_properties_changed =
        [this, page_id](std::size_t plot_index, const ChartModel& model) {
            for (auto& stored : pages_) {
                if (stored.id != page_id) {
                    continue;
                }
                if (plot_index < stored.plots.size()) {
                    stored.plots[plot_index] = plot_from_chart_model(model);
                }
                return;
            }
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
    update_empty_state();
    emit pages_changed();
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
    while (count() > 0) {
        QWidget* page = widget(0);
        removeTab(0);
        if (page != nullptr) {
            page->deleteLater();
        }
    }
    pages_.clear();
    update_empty_state();
    emit pages_changed();
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
