#include "ui/analysis_chart_widget.h"
#include "ui/row_visibility_clipboard.h"
#include "ui/graph_properties_dialog.h"
#include "ui/graph_properties_panel.h"

#include "domain/statistics/special_cause_rule_catalog.h"
#include "reporting/chart_geometry.h"
#include "reporting/chart_interaction.h"
#include "reporting/chart_plot_context.h"
#include "reporting/chart_renderer.h"

#include <QContextMenuEvent>
#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QEvent>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QIODevice>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLayout>
#include <QLineEdit>
#include <QLineF>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPixmap>
#include <QScreen>
#include <QStringList>
#include <QToolTip>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace {

constexpr std::size_t kMaxChartUndoHistory = 32;



QPoint clamp_tooltip_position(const QPoint position, const QString& text)
{
    QScreen* screen = QGuiApplication::screenAt(position);
    if (screen == nullptr) {
        return position + QPoint(12, 18);
    }
    const QRect bounds = screen->availableGeometry();
    const int estimated_width = std::min(
        360, std::max(180, static_cast<int>(text.size()) * 7));
    const int estimated_height = std::min(
        180, 28 + static_cast<int>(text.count(QLatin1Char('\n'))) * 20);
    QPoint result = position + QPoint(12, 18);
    if (result.x() + estimated_width > bounds.right()) {
        result.setX(position.x() - estimated_width - 12);
    }
    if (result.y() + estimated_height > bounds.bottom()) {
        result.setY(position.y() - estimated_height - 12);
    }
    return result;
}

}  // namespace

AnalysisChartWidget::AnalysisChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(320);
    setAutoFillBackground(true);
    setMouseTracking(true);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setFocusPolicy(Qt::StrongFocus);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    surface_ = new QWidget(this);
    surface_->setObjectName(QStringLiteral("chart_surface"));
    surface_->setMouseTracking(true);
    surface_->setFocusPolicy(Qt::StrongFocus);
    surface_->setContextMenuPolicy(Qt::DefaultContextMenu);
    surface_->setAttribute(Qt::WA_OpaquePaintEvent);
    surface_->installEventFilter(this);
    layout->addWidget(surface_, 1);
    tooltip_timer_.setSingleShot(true);
    tooltip_timer_.setInterval(180);
    connect(&tooltip_timer_, &QTimer::timeout, this, [this]() {
        if (!pending_tooltip_.isEmpty()) {
            QToolTip::showText(
                clamp_tooltip_position(pending_tooltip_position_, pending_tooltip_),
                pending_tooltip_, this);
        }
    });
}

void AnalysisChartWidget::set_data(
    const std::vector<double>& values,
    const std::vector<double>& center,
    const std::vector<double>& lower,
    const std::vector<double>& upper)
{
    ChartModel model;
    model.values = values;
    model.center = center;
    model.lower = lower;
    model.upper = upper;
    set_model(model);
}

void AnalysisChartWidget::set_model(const ChartModel& model)
{
    model_ = model;
    undo_history_.clear();
    redo_history_.clear();
    clear_hover();
    panning_ = selecting_ = space_pressed_ = false;
    pressed_hit_.reset();
    navigation_before_drag_.reset();
    if (panel_ != nullptr && panel_->isVisible()) {
        panel_->set_model(model_);
    }
    request_chart_update();
}

void AnalysisChartWidget::apply_model_change(const ChartModel& model, const bool record_undo)
{
    if (record_undo && !applying_history_) {
        undo_history_.push_back(model_);
        if (undo_history_.size() > kMaxChartUndoHistory) {
            undo_history_.erase(undo_history_.begin());
        }
        redo_history_.clear();
    }
    model_ = model;
    clear_hover();
    if (panel_ != nullptr && panel_->isVisible()) {
        panel_->refresh_model(model_);
    }
    emit display_properties_changed(model_);
    request_chart_update();
}

void AnalysisChartWidget::undo_chart_edit()
{
    if (undo_history_.empty()) return;
    HistoryEntry previous = std::move(undo_history_.back());
    undo_history_.pop_back();
    applying_history_ = true;
    if (const auto* navigation = std::get_if<NavigationState>(&previous)) {
        redo_history_.push_back(navigation_state());
        restore_navigation(*navigation);
    } else {
        redo_history_.push_back(model_);
        apply_model_change(std::get<ChartModel>(previous), false);
    }
    applying_history_ = false;
    if (redo_history_.size() > kMaxChartUndoHistory) redo_history_.erase(redo_history_.begin());
}

void AnalysisChartWidget::redo_chart_edit()
{
    if (redo_history_.empty()) return;
    HistoryEntry next = std::move(redo_history_.back());
    redo_history_.pop_back();
    applying_history_ = true;
    if (const auto* navigation = std::get_if<NavigationState>(&next)) {
        undo_history_.push_back(navigation_state());
        restore_navigation(*navigation);
    } else {
        undo_history_.push_back(model_);
        apply_model_change(std::get<ChartModel>(next), false);
    }
    applying_history_ = false;
    if (undo_history_.size() > kMaxChartUndoHistory) undo_history_.erase(undo_history_.begin());
}

AnalysisChartWidget::NavigationState AnalysisChartWidget::navigation_state() const
{
    return {model_.view.zoom_factor, model_.view.pan_offset, model_.view.pan_reference_size, model_.x_min, model_.x_max, model_.y_min, model_.y_max};
}

void AnalysisChartWidget::restore_navigation(const NavigationState& state)
{
    model_.view.zoom_factor = state.zoom; model_.view.pan_offset = state.pan;
    model_.view.pan_reference_size = state.pan_reference_size;
    model_.x_min = state.x_min; model_.x_max = state.x_max;
    model_.y_min = state.y_min; model_.y_max = state.y_max;
    clear_hover();
    if (panel_ != nullptr && panel_->isVisible()) panel_->refresh_model(model_);
    emit display_properties_changed(model_);
    request_chart_update();
}

void AnalysisChartWidget::record_navigation_change(const NavigationState& before)
{
    const auto after = navigation_state();
    if (before.zoom == after.zoom && before.pan == after.pan && before.pan_reference_size == after.pan_reference_size && before.x_min == after.x_min
        && before.x_max == after.x_max && before.y_min == after.y_min && before.y_max == after.y_max) return;
    if (!applying_history_) {
        undo_history_.push_back(before);
        if (undo_history_.size() > kMaxChartUndoHistory) undo_history_.erase(undo_history_.begin());
        redo_history_.clear();
    }
    clear_hover();
    if (panel_ != nullptr && panel_->isVisible()) panel_->refresh_model(model_);
    emit display_properties_changed(model_);
    request_chart_update();
}

void AnalysisChartWidget::clear_hover()
{
    tooltip_timer_.stop(); pending_tooltip_.clear(); hovered_hit_.reset();
    model_.view.hovered_point.reset();
    QToolTip::hideText();
}

void AnalysisChartWidget::set_source_rows(const std::vector<std::size_t>& rows)
{
    model_.source_rows = rows;
}

void AnalysisChartWidget::set_selected_source_rows(const std::vector<std::size_t>& rows)
{
    model_.view.selected_points =
        chart_interaction::resolve_mark_indices_for_source_rows(model_, rows);
    request_chart_update();
}

std::vector<std::size_t> AnalysisChartWidget::selected_source_rows() const
{
    return chart_interaction::resolve_selected_source_rows(model_, model_.view.selected_points);
}

void AnalysisChartWidget::set_row_visibility_summary(
    const std::size_t excluded_count,
    const std::size_t hidden_count,
    const std::size_t analysis_n,
    const std::size_t display_n)
{
    excluded_count_ = excluded_count;
    hidden_count_ = hidden_count;
    analysis_n_ = analysis_n;
    display_n_ = display_n;
    if (panel_ != nullptr) {
        panel_->set_row_visibility_summary(
            excluded_count_, hidden_count_, analysis_n_, display_n_);
    }
}

bool AnalysisChartWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != surface_) {
        return QWidget::eventFilter(watched, event);
    }
    switch (event->type()) {
    case QEvent::Paint: {
        QPainter painter(surface_);
        ChartRenderer::render(painter, surface_->rect(), model_);
        if (selecting_) {
            const QRect selection = QRect(selection_start_, selection_end_).normalized();
            painter.setPen(QPen(QColor("#1976d2"), 1.0, Qt::DashLine));
            painter.setBrush(QColor(25, 118, 210, 35));
            painter.drawRect(selection);
        }
        return true;
    }
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseButtonPress:
        surface_->setFocus(Qt::MouseFocusReason);
        mousePressEvent(static_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast<QMouseEvent*>(event));
        return true;
    case QEvent::KeyPress:
        keyPressEvent(static_cast<QKeyEvent*>(event));
        return event->isAccepted();
    case QEvent::KeyRelease:
        keyReleaseEvent(static_cast<QKeyEvent*>(event));
        return event->isAccepted();
    case QEvent::Wheel:
        wheelEvent(static_cast<QWheelEvent*>(event));
        return event->isAccepted();
    case QEvent::Leave:
        clear_hover();
        request_chart_update();
        return false;
    case QEvent::FocusOut:
    case QEvent::Hide:
        clear_hover();
        if (panning_ && navigation_before_drag_.has_value()) record_navigation_change(*navigation_before_drag_);
        panning_ = selecting_ = space_pressed_ = false;
        navigation_before_drag_.reset();
        pressed_hit_.reset();
        request_chart_update();
        return false;
    case QEvent::ContextMenu:
        surface_->setFocus(Qt::MouseFocusReason);
        contextMenuEvent(static_cast<QContextMenuEvent*>(event));
        return true;
    default:
        return false;
    }
}

void AnalysisChartWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.fillRect(rect(), palette().window());
}

void AnalysisChartWidget::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint position = event->position().toPoint();
    if (panning_) {
        clear_hover();
        const auto context = build_chart_plot_context(model_, plot_bounds());
        const auto plot = context.valid ? context.plot : chart_plot_area(plot_bounds(), model_.kind);
        chart_set_pan_offset(model_, plot, chart_pan_offset(model_, plot) + position - last_mouse_position_);
        last_mouse_position_ = position;
        request_chart_update();
        return;
    }
    if (selecting_) {
        clear_hover(); selection_end_ = position; request_chart_update(); return;
    }
    const auto hit = hit_test(position);
    if (!hit.has_value()) { clear_hover(); request_chart_update(); return; }
    const bool changed = !hovered_hit_.has_value() || hovered_hit_->kind != hit->kind
        || hovered_hit_->index != hit->index || hovered_hit_->series_index != hit->series_index
        || hovered_hit_->matrix_row != hit->matrix_row || hovered_hit_->matrix_column != hit->matrix_column;
    hovered_hit_ = hit;
    if (hit->kind != chart_interaction::HitKind::ReferenceLine) model_.view.hovered_point = hit->index;
    else model_.view.hovered_point.reset();
    const QString message = chart_interaction::tooltip_text(model_, *hit);
    pending_tooltip_position_ = event->globalPosition().toPoint();
    if (changed || message != pending_tooltip_) {
        tooltip_timer_.stop(); QToolTip::hideText();
        pending_tooltip_ = message;
        if (!message.isEmpty()) tooltip_timer_.start();
    }
    request_chart_update();
}

void AnalysisChartWidget::mousePressEvent(QMouseEvent* event)
{
    const QPoint position = event->position().toPoint();
    if (event->button() == Qt::MiddleButton || (event->button() == Qt::LeftButton && space_pressed_)) {
        const auto context = build_chart_plot_context(model_, plot_bounds());
        const bool grid = model_.kind == ChartKind::Matrix || model_.kind == ChartKind::Correlation || model_.kind == ChartKind::Heatmap;
        if (chart_interaction::supports_navigation(model_.kind)
            && ((context.valid && context.plot.contains(position)) || (grid && chart_plot_area(plot_bounds(), model_.kind).contains(position)))) {
            clear_hover(); panning_ = true; last_mouse_position_ = position;
            navigation_before_drag_ = navigation_state();
        }
        return;
    }
    if (event->button() != Qt::LeftButton) return;
    clear_hover();
    selection_start_ = selection_end_ = position;
    selection_before_drag_ = model_.view.selected_points;
    selection_modifiers_ = event->modifiers();
    pressed_hit_ = hit_test(position);
    selecting_ = true;
}

void AnalysisChartWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (panning_ && (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton)) {
        panning_ = false;
        if (navigation_before_drag_.has_value()) record_navigation_change(*navigation_before_drag_);
        navigation_before_drag_.reset();
        return;
    }
    if (event->button() != Qt::LeftButton || !selecting_) return;
    selecting_ = false;
    selection_end_ = event->position().toPoint();
    const bool dragged = (selection_end_ - selection_start_).manhattanLength() > 8;
    if (dragged && (selection_modifiers_ & Qt::ShiftModifier)) {
        const auto before = navigation_state();
        if (chart_interaction::zoom_to_rectangle(model_, plot_bounds(), QRectF(selection_start_, selection_end_))) record_navigation_change(before);
    } else {
        const bool additive = selection_modifiers_ & (Qt::ControlModifier | Qt::ShiftModifier);
        std::vector<std::size_t> selected = additive ? selection_before_drag_ : std::vector<std::size_t>{};
        std::optional<chart_interaction::Hit> hit;
        if (dragged) {
            const auto brushed = chart_interaction::select_rectangle(model_, plot_bounds(), QRectF(selection_start_, selection_end_));
            selected.insert(selected.end(), brushed.begin(), brushed.end());
        } else {
            hit = hit_test(selection_end_);
            if (!hit.has_value()) hit = pressed_hit_;
            if (hit.has_value() && hit->kind == chart_interaction::HitKind::ReferenceLine) {
                emit element_selected(chart_interaction::element_path(model_, chart_interaction::element_for_hit(*hit)));
                pressed_hit_.reset(); request_chart_update(); return;
            }
            if (hit.has_value()) {
                const auto found = std::find(selected.begin(), selected.end(), hit->index);
                if ((selection_modifiers_ & Qt::ControlModifier) && found != selected.end()) {
                    const auto hit_rows = chart_interaction::resolve_selected_source_rows(model_, {hit->index});
                    const auto linked_marks = hit_rows.empty() ? std::vector<std::size_t>{}
                        : chart_interaction::resolve_mark_indices_for_source_rows(model_, hit_rows);
                    selected.erase(std::remove_if(selected.begin(), selected.end(), [&](std::size_t index) {
                        return index == hit->index || (!hit_rows.empty()
                            && std::binary_search(linked_marks.begin(), linked_marks.end(), index)
                            && chart_interaction::resolve_selected_source_rows(model_, {index}) == hit_rows);
                    }), selected.end());
                } else selected.push_back(hit->index);
                emit element_selected(chart_interaction::element_path(model_, chart_interaction::element_for_hit(*hit)));
                if (hit->series_index.has_value() && panel_ != nullptr && panel_->isVisible()) panel_->set_selected_series(*hit->series_index);
            }
        }
        std::sort(selected.begin(), selected.end());
        selected.erase(std::unique(selected.begin(), selected.end()), selected.end());
        model_.view.selected_points = std::move(selected);
        emit_selected_rows();
    }
    pressed_hit_.reset(); request_chart_update();
}

void AnalysisChartWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        if (panning_ && navigation_before_drag_.has_value()) restore_navigation(*navigation_before_drag_);
        panning_ = selecting_ = space_pressed_ = false;
        navigation_before_drag_.reset();
        pressed_hit_.reset();
        clear_hover();
        request_chart_update();
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::Copy)) {
        copy_to_clipboard();
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::Undo)) {
        undo_chart_edit();
        event->accept();
        return;
    }
    if (event->matches(QKeySequence::Redo)) {
        redo_chart_edit();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Space) {
        space_pressed_ = true;
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void AnalysisChartWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space) {
        space_pressed_ = false;
        event->accept();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void AnalysisChartWidget::wheelEvent(QWheelEvent* event)
{
    event->ignore();
    if (!(event->modifiers() & Qt::ControlModifier) || event->angleDelta().y() == 0) return;
    const auto before = navigation_state();
    const double factor = std::pow(1.2, static_cast<double>(event->angleDelta().y()) / 120.0);
    if (chart_interaction::zoom_at(model_, plot_bounds(), event->position(), factor)) {
        record_navigation_change(before); event->accept();
    }
}

void AnalysisChartWidget::contextMenuEvent(QContextMenuEvent* event)
{
    event->accept();
    const auto hit = hit_test(event->pos());
    clear_hover();
    QMenu menu(this);
    QAction* element_edit_action = hit.has_value() ? menu.addAction(QStringLiteral("编辑当前对象…")) : nullptr;
    QAction* edit_action = menu.addAction(QStringLiteral("编辑图形…"));
    QAction* full_action = menu.addAction(QStringLiteral("完整属性…"));
    QAction* save_action = menu.addAction(QStringLiteral("保存图形为 PNG…"));
    QAction* copy_action = menu.addAction(QStringLiteral("复制图形"));
    menu.addSeparator();
    QAction* fit_action = menu.addAction(QStringLiteral("适合窗口"));
    QAction* clear_action = menu.addAction(QStringLiteral("清除刷选"));
    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == nullptr) return;
    if (chosen == edit_action || (element_edit_action != nullptr && chosen == element_edit_action)) {
        enter_edit_mode(hit.has_value() ? chart_interaction::element_path(model_, chart_interaction::element_for_hit(*hit)) : QString());
        if (hit.has_value() && hit->series_index.has_value() && panel_ != nullptr) panel_->set_selected_series(*hit->series_index);
    } else if (chosen == full_action) open_full_editor();
    else if (chosen == save_action) save_graph();
    else if (chosen == copy_action) copy_to_clipboard();
    else if (chosen == fit_action) fit_to_window();
    else if (chosen == clear_action) {
        model_.view.selected_points.clear(); clear_hover(); emit_selected_rows(); request_chart_update();
    }
}

void AnalysisChartWidget::edit_graph()
{
    enter_edit_mode(QString());
}

void AnalysisChartWidget::open_full_editor()
{
    GraphPropertiesDialog dialog(model_, this);
    if (dialog.exec() == QDialog::Accepted) {
        apply_model_change(dialog.model(), true);
    }
}

void AnalysisChartWidget::enter_edit_mode(const QString& path)
{
    if (layout() == nullptr) {
        return;
    }
    if (panel_ == nullptr) {
        panel_ = new GraphPropertiesPanel(model_, this);
        layout()->addWidget(panel_);
        connect(panel_, &GraphPropertiesPanel::model_changed, this,
                [this](const ChartModel& model) {
                    apply_model_change(model, true);
                });
        connect(panel_, &GraphPropertiesPanel::close_requested, this,
                [this]() { leave_edit_mode(); });
        connect(this, &AnalysisChartWidget::element_selected, this,
                [this](const QString& selected) {
                    if (panel_ != nullptr && panel_->isVisible()) {
                        panel_->set_selected_path(selected);
                    }
                });
        panel_->set_model(model_);
        panel_->set_row_visibility_summary(
            excluded_count_, hidden_count_, analysis_n_, display_n_);
    } else {
        panel_->set_model(model_);
        panel_->set_row_visibility_summary(
            excluded_count_, hidden_count_, analysis_n_, display_n_);
    }
    panel_->show();
    if (!path.isEmpty()) {
        panel_->set_selected_path(path);
    }
    request_chart_update();
}

void AnalysisChartWidget::leave_edit_mode()
{
    if (panel_ != nullptr) {
        panel_->hide();
    }
    request_chart_update();
}

void AnalysisChartWidget::request_chart_update()
{
    if (surface_ != nullptr) {
        surface_->update();
    }
    update();
}

QRect AnalysisChartWidget::plot_bounds() const
{
    if (surface_ != nullptr) {
        return surface_->rect();
    }
    return rect();
}

QSize AnalysisChartWidget::chart_render_size() const
{
    QSize size;
    if (surface_ != nullptr && surface_->size().isValid() && !surface_->size().isEmpty()) {
        size = surface_->size();
    } else {
        size = this->size();
    }
    if (!size.isValid() || size.isEmpty()) {
        return QSize(640, 480);
    }
    return size;
}

QPixmap AnalysisChartWidget::render_chart_pixmap() const
{
    return ChartRenderer::render_to_pixmap(model_, chart_render_size(), devicePixelRatioF());
}

void AnalysisChartWidget::save_graph()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("保存图形"),
        QString(),
        QStringLiteral("PNG 图片 (*.png)"));
    if (!path.isEmpty()) {
        const QPixmap pixmap = render_chart_pixmap();
        pixmap.save(path, "PNG");
    }
}

bool AnalysisChartWidget::copy_to_clipboard()
{
    const QPixmap chart = render_chart_pixmap();
    if (chart.isNull()) {
        return false;
    }
    const QString footnote = datalab::ui::row_visibility_footnote(
        excluded_count_, hidden_count_, analysis_n_, display_n_);
    const QPixmap pixmap = datalab::ui::compose_chart_pixmap_with_footnote(chart, footnote);
    if (pixmap.isNull()) {
        return false;
    }
    const QImage image = pixmap.toImage();
    QByteArray png;
    QBuffer png_buffer(&png);
    png_buffer.open(QIODevice::WriteOnly);
    image.save(&png_buffer, "PNG");
    QByteArray bmp;
    QBuffer bmp_buffer(&bmp);
    bmp_buffer.open(QIODevice::WriteOnly);
    image.save(&bmp_buffer, "BMP");
    auto* mime = new QMimeData();
    mime->setImageData(image);
    if (!png.isEmpty()) {
        mime->setData(QStringLiteral("image/png"), png);
    }
    if (!bmp.isEmpty()) {
        mime->setData(QStringLiteral("image/bmp"), bmp);
    }
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setMimeData(mime);
    return true;
}

void AnalysisChartWidget::copy_graph()
{
    copy_to_clipboard();
}

void AnalysisChartWidget::fit_to_window()
{
    const auto before = navigation_state();
    model_.view.zoom_factor = 1.0; model_.view.pan_offset = {};
    model_.view.pan_reference_size = {};
    model_.x_min.reset(); model_.x_max.reset(); model_.y_min.reset(); model_.y_max.reset();
    record_navigation_change(before);
}

void AnalysisChartWidget::emit_selected_rows()
{
    emit rows_selected(selected_source_rows());
}

std::optional<chart_interaction::Hit> AnalysisChartWidget::hit_test(const QPoint& position) const
{
    return chart_interaction::hit_test(model_, plot_bounds(), position);
}
