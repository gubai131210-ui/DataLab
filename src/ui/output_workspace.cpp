#include "ui/output_workspace.h"

#include "ui/analysis_chart_widget.h"
#include "ui/page_renderer.h"
#include "reporting/chart_adapter.h"

#include <QApplication>
#include <QFrame>
#include <QIcon>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollArea>
#include <QShortcut>
#include <QTabBar>
#include <QTableView>
#include <QVBoxLayout>
#include <QFocusEvent>
#include <utility>

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
    connect(this, &QTabWidget::currentChanged, this, [this] {
        refresh_source_link_labels(active_source_fingerprint());
    });

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

OutputWorkspace::~OutputWorkspace()
{
    callback_lifetime_.reset();
    // QTabWidget may change the current tab while destroying its child pages.
    // Its callbacks must stop before our page/context members are destroyed.
    disconnect(this, nullptr, this, nullptr);
    source_fingerprint_provider_ = {};
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
    if (last_focused_chart_ != nullptr && page != nullptr && page->isAncestorOf(last_focused_chart_)) {
        last_focused_chart_ = nullptr;
    }
    removeTab(index);
    pages_.erase(pages_.begin() + index);
    refresh_source_link_labels(active_source_fingerprint());
    if (page != nullptr) {
        page->deleteLater();
    }
    update_empty_state();
    emit page_closed(id);
    emit pages_changed();
}

bool OutputWorkspace::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::FocusIn || event->type() == QEvent::MouseButtonPress) {
        auto* widget = qobject_cast<QWidget*>(watched);
        if (widget != nullptr) {
            AnalysisChartWidget* chart = qobject_cast<AnalysisChartWidget*>(widget);
            if (chart == nullptr) {
                for (QWidget* parent = widget->parentWidget(); parent != nullptr;
                     parent = parent->parentWidget()) {
                    if ((chart = qobject_cast<AnalysisChartWidget*>(parent)) != nullptr) {
                        break;
                    }
                }
            }
            if (chart != nullptr) {
                last_focused_chart_ = chart;
            }
        }
    }
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
    const std::weak_ptr<char> lifetime = callback_lifetime_;
    auto* scroll = new QWidget(this);
    auto* page_layout = new QVBoxLayout(scroll);
    page_layout->setContentsMargins(0, 0, 0, 0);
    auto* source_label = new QLabel(scroll);
    source_label->setObjectName(QStringLiteral("source_link_status"));
    source_label->setWordWrap(true);
    source_label->setTextFormat(Qt::PlainText);
    page_layout->addWidget(source_label);
    page_renderer::PageRenderOptions options;
    options.interactive_charts = true;
    options.include_method = true;
    options.on_display_properties_changed =
        [this, scroll, lifetime](std::size_t plot_index, const ChartModel& model) {
            if (lifetime.expired()) return;
            const int index = indexOf(scroll);
            if (index >= 0 && static_cast<std::size_t>(index) < pages_.size()
                && plot_index < pages_[index].plots.size()) {
                pages_[index].plots[plot_index] = plot_from_chart_model(model);
            }
        };
    options.on_rows_selected = [this, scroll, lifetime](const std::vector<std::size_t>& rows) {
        if (lifetime.expired()) return;
        const int index = indexOf(scroll);
        if (index < 0 || static_cast<std::size_t>(index) >= pages_.size()) {
            return;
        }
        const std::string fingerprint = active_source_fingerprint();
        refresh_source_link_labels(fingerprint);
        if (page_matches_source(index, fingerprint)) {
            emit rows_selected(rows);
        } else if (const auto* label = scroll->findChild<QLabel*>(
                       QStringLiteral("source_link_status"))) {
            emit source_link_unavailable(label->text());
        }
    };
    options.on_formula_substitution = [this, lifetime](const datalab::domain::OutputPage& p) {
        if (lifetime.expired()) return;
        emit formula_substitution_requested(p);
    };
    page_layout->addWidget(page_renderer::build_page_widget(page, scroll, options));
    for (AnalysisChartWidget* chart : scroll->findChildren<AnalysisChartWidget*>()) {
        chart->installEventFilter(this);
        if (QWidget* surface = chart->findChild<QWidget*>(QStringLiteral("chart_surface"))) {
            surface->installEventFilter(this);
        }
    }
    auto* copy_chart_shortcut = new QShortcut(QKeySequence::Copy, scroll);
    copy_chart_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(copy_chart_shortcut, &QShortcut::activated, scroll, [this, scroll, lifetime]() {
        if (lifetime.expired()) return;
        QWidget* focus = QApplication::focusWidget();
        if (focus != nullptr) {
            if (qobject_cast<QTableView*>(focus) != nullptr) {
                return;
            }
            for (QWidget* widget = focus; widget != nullptr; widget = widget->parentWidget()) {
                if (qobject_cast<AnalysisChartWidget*>(widget) != nullptr) {
                    return;
                }
            }
        }
        if (focus == nullptr || scroll->isAncestorOf(focus) || focus == scroll) {
            emit copy_chart_requested();
        }
    });
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
    last_focused_chart_ = nullptr;
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
    const std::string fingerprint = active_source_fingerprint();
    refresh_source_link_labels(fingerprint);
    for (int index = 0; index < count(); ++index) {
        const bool compatible = page_matches_source(index, fingerprint);
        for (AnalysisChartWidget* chart : widget(index)->findChildren<AnalysisChartWidget*>()) {
            if (compatible) chart->set_selected_source_rows(rows);
            else chart->set_selected_source_rows({});
        }
    }
}

void OutputWorkspace::set_source_fingerprint_provider(std::function<std::string()> provider)
{
    source_fingerprint_provider_ = std::move(provider);
    invalidate_source_selection();
    refresh_source_link_labels(active_source_fingerprint());
}

void OutputWorkspace::invalidate_source_selection()
{
    if (source_context_dirty_) return;
    source_context_dirty_ = true;
    for (AnalysisChartWidget* chart : findChildren<AnalysisChartWidget*>()) {
        chart->set_selected_source_rows({});
    }
    for (int index = 0; index < count(); ++index) {
        if (auto* label = widget(index)->findChild<QLabel*>(QStringLiteral("source_link_status"))) {
            label->setText(QStringLiteral("工作表已更新，行联动将在操作时核对数据来源。"));
        }
    }
}

std::string OutputWorkspace::active_source_fingerprint() const
{
    return source_fingerprint_provider_ ? source_fingerprint_provider_() : std::string{};
}

bool OutputWorkspace::page_matches_source(std::size_t index, const std::string& fingerprint) const
{
    return index < pages_.size() && !fingerprint.empty()
        && pages_[index].source_data_fingerprint == fingerprint;
}

void OutputWorkspace::refresh_source_link_labels(const std::string& fingerprint)
{
    source_context_dirty_ = false;
    for (int index = 0; index < count() && static_cast<std::size_t>(index) < pages_.size(); ++index) {
        auto* label = widget(index)->findChild<QLabel*>(QStringLiteral("source_link_status"));
        if (label == nullptr) continue;
        const auto& page = pages_[index];
        const QString source_name = page.source_worksheet_name.empty()
            ? QStringLiteral("未命名工作表") : QString::fromStdString(page.source_worksheet_name);
        const bool compatible = page_matches_source(index, fingerprint);
        QString message;
        if (compatible) {
            message = QStringLiteral("来源工作表：%1 · 行联动可用").arg(source_name);
        } else if (page.source_data_fingerprint.empty()) {
            message = QStringLiteral("此结果未记录数据来源，工作表行联动暂不可用。");
        } else {
            message = QStringLiteral("来源工作表：%1 · 当前工作表或数据已改变，行联动已暂停。")
                          .arg(source_name);
        }
        if (label->text() != message) label->setText(message);
        const QString style = compatible
            ? QStringLiteral("color:#3b6970; background:#eef7f7; padding:6px 10px;")
            : QStringLiteral("color:#80632c; background:#fff7e7; padding:6px 10px;");
        if (label->styleSheet() != style) label->setStyleSheet(style);
    }
}

AnalysisChartWidget* OutputWorkspace::chart_for_copy() const
{
    QWidget* page = currentWidget();
    if (page == nullptr) {
        return nullptr;
    }
    if (last_focused_chart_ != nullptr && page->isAncestorOf(last_focused_chart_)) {
        return last_focused_chart_;
    }
    const auto charts = page->findChildren<AnalysisChartWidget*>();
    return charts.isEmpty() ? nullptr : charts.front();
}
