#include "ui/analysis_chart_widget.h"
#include "ui/graph_properties_dialog.h"

#include "reporting/chart_coordinate_mapper.h"
#include "reporting/chart_geometry.h"
#include "reporting/chart_interaction.h"
#include "reporting/chart_renderer.h"

#include <QContextMenuEvent>
#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QFileDialog>
#include <QGuiApplication>
#include <QImage>
#include <QIODevice>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLineF>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QStringList>
#include <QToolTip>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace {

chart_interaction::Element element_for_hit(const ChartModel& model, const std::size_t index)
{
    using chart_interaction::ElementKind;
    switch (model.kind) {
    case ChartKind::Histogram:
    case ChartKind::Pareto:
        return {ElementKind::Bar, index};
    case ChartKind::BoxPlot:
        return {ElementKind::Box, index};
    case ChartKind::Correlation:
    case ChartKind::Heatmap:
    case ChartKind::Matrix:
        return {ElementKind::Cell, index};
    case ChartKind::Pie:
        return {ElementKind::Slice, index};
    case ChartKind::Parallel:
        return {ElementKind::ParallelObservation, index};
    case ChartKind::Contour:
        return {ElementKind::ContourCell, index};
    case ChartKind::Control:
    case ChartKind::Probability:
    case ChartKind::Scatter:
    case ChartKind::Interval:
    case ChartKind::Bubble:
    case ChartKind::Ecdf:
    case ChartKind::Marginal:
    case ChartKind::TimeSeries:
    case ChartKind::Area:
    default:
        return {ElementKind::Point, index};
    }
}

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
    model_.values = values;
    model_.center = center;
    model_.lower = lower;
    model_.upper = upper;
    model_.source_rows.resize(values.size());
    std::iota(model_.source_rows.begin(), model_.source_rows.end(), 0);
    model_.view.zoom_factor = 1.0;
    model_.view.pan_offset = {};
    update();
}

void AnalysisChartWidget::set_model(const ChartModel& model)
{
    model_ = model;
    model_.view.zoom_factor = 1.0;
    model_.view.pan_offset = {};
    update();
}

void AnalysisChartWidget::set_source_rows(const std::vector<std::size_t>& rows)
{
    model_.source_rows = rows;
    if (model_.source_rows.size() != model_.values.size()) {
        model_.source_rows.resize(model_.values.size());
        std::iota(model_.source_rows.begin(), model_.source_rows.end(), 0);
    }
}

void AnalysisChartWidget::set_selected_source_rows(const std::vector<std::size_t>& rows)
{
    model_.view.selected_points.clear();
    for (std::size_t index = 0; index < model_.source_rows.size(); ++index) {
        if (std::find(rows.begin(), rows.end(), model_.source_rows[index]) != rows.end()) {
            model_.view.selected_points.push_back(index);
        }
    }
    update();
}

void AnalysisChartWidget::set_editor_enabled(const bool enabled)
{
    editor_enabled_ = enabled;
}

void AnalysisChartWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    ChartRenderer::render(painter, rect(), model_);
    if (selecting_) {
        const QRect selection = QRect(selection_start_, selection_end_).normalized();
        painter.setPen(QPen(QColor("#1976d2"), 1.0, Qt::DashLine));
        painter.setBrush(QColor(25, 118, 210, 35));
        painter.drawRect(selection);
    }
}

void AnalysisChartWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (panning_) {
        tooltip_timer_.stop();
        QToolTip::hideText();
        const QPoint current = event->position().toPoint();
        model_.view.pan_offset += current - last_mouse_position_;
        last_mouse_position_ = current;
        update();
        return;
    }
    if (selecting_) {
        tooltip_timer_.stop();
        QToolTip::hideText();
        selection_end_ = event->position().toPoint();
        update();
        return;
    }
    const auto point = hit_test(event->position().toPoint());
    if (point.has_value()) {
        const std::size_t index = *point;
        model_.view.hovered_point = index;
        emit element_selected(chart_interaction::element_path(
            model_, element_for_hit(model_, index)));
        QString text;
        if (model_.kind == ChartKind::Parallel) {
            text = chart_interaction::tooltip_text(
                model_, {chart_interaction::HitKind::ParallelObservation, index});
        } else if (model_.kind == ChartKind::Contour) {
            text = chart_interaction::tooltip_text(
                model_, {chart_interaction::HitKind::ContourCell, index});
        } else if (model_.kind == ChartKind::Control && index < model_.values.size()) {
            const QString source = index < model_.source_rows.size()
                ? QString::number(static_cast<qulonglong>(model_.source_rows[index] + 1))
                : QStringLiteral("-");
            text = QStringLiteral("点 %1  原始行 %2\n观测值: %3")
                .arg(static_cast<qulonglong>(index + 1))
                .arg(source)
                .arg(model_.values[index], 0, 'g', 8);
            QStringList tests;
            if (index < model_.triggered_tests.size()) {
                for (const int test : model_.triggered_tests[index]) {
                    tests.push_back(QStringLiteral("Test %1").arg(test));
                }
            } else {
                for (std::size_t test = 0; test < model_.special_cause_points.size(); ++test) {
                    if (std::find(model_.special_cause_points[test].cbegin(),
                                  model_.special_cause_points[test].cend(), index)
                        != model_.special_cause_points[test].cend()) {
                        tests.push_back(QStringLiteral("Test %1").arg(static_cast<int>(test + 1)));
                    }
                }
            }
            if (!tests.isEmpty()) {
                text += QStringLiteral("\n失败测试: ") + tests.join(QStringLiteral(", "));
            }
            if (model_.sigma_z > 0.0) {
                text += QStringLiteral("\nSigma Z: ")
                    + QString::number(model_.sigma_z, 'g', 6);
            }
            if (index < model_.signal_direction.size()
                && model_.signal_direction[index] != 0) {
                text += QStringLiteral("\n信号方向: ")
                    + (model_.signal_direction[index] > 0
                           ? QStringLiteral("上侧")
                           : QStringLiteral("下侧"));
            }
        } else if (model_.kind == ChartKind::Histogram
                   && index + 1 < model_.histogram_edges.size()) {
            const double total = std::accumulate(
                model_.histogram_counts.cbegin(), model_.histogram_counts.cend(), 0.0);
            const double count = index < model_.histogram_counts.size()
                ? model_.histogram_counts[index] : 0.0;
            text = QStringLiteral("区间 [%1, %2)\n频数: %3")
                .arg(model_.histogram_edges[index], 0, 'g', 6)
                .arg(model_.histogram_edges[index + 1], 0, 'g', 6)
                .arg(count);
            if (total > 0.0) {
                text += QStringLiteral("\n百分比: ")
                    + QString::number(count / total * 100.0, 'f', 2) + QStringLiteral("%");
            }
        } else if (model_.kind == ChartKind::Pareto && index < model_.categories.size()) {
            text = QStringLiteral("%1\n数量: %2")
                .arg(model_.categories[index])
                .arg(index < model_.category_values.size() ? model_.category_values[index] : 0.0);
            if (index < model_.cumulative_percent.size()) {
                text += QStringLiteral("\n累计百分比: ")
                    + QString::number(model_.cumulative_percent[index], 'f', 2)
                    + QStringLiteral("%");
            }
        } else if (model_.kind == ChartKind::BoxPlot && index < model_.box_labels.size()) {
            text = QStringLiteral("%1\n中位数: %2")
                .arg(model_.box_labels[index])
                .arg(index < model_.box_median.size() ? model_.box_median[index] : 0.0);
        } else if ((model_.kind == ChartKind::Scatter || model_.kind == ChartKind::Bubble
                    || model_.kind == ChartKind::Marginal)
                   && index < model_.values.size()
                   && index < model_.x_values.size()) {
            text = QStringLiteral("点 %1\nX: %2\nY: %3")
                .arg(static_cast<qulonglong>(index + 1))
                .arg(model_.x_values[index], 0, 'g', 8)
                .arg(model_.values[index], 0, 'g', 8);
            if (index < model_.source_rows.size()) {
                text += QStringLiteral("\n原始行: ")
                    + QString::number(static_cast<qulonglong>(
                        model_.source_rows[index] + 1));
            }
            if (model_.kind == ChartKind::Bubble && index < model_.bubble_sizes.size()) {
                text += QStringLiteral("\n气泡大小: ")
                    + QString::number(model_.bubble_sizes[index], 'g', 8);
            }
            if (index < model_.point_labels.size() && !model_.point_labels[index].isEmpty()) {
                text += QStringLiteral("\n标签: ") + model_.point_labels[index];
            }
            if (index < model_.point_groups.size() && !model_.point_groups[index].isEmpty()) {
                text += QStringLiteral("\n分组: ") + model_.point_groups[index];
            }
        } else if (model_.kind == ChartKind::Interval
                   && index < model_.categories.size()
                   && index < model_.values.size()) {
            text = QStringLiteral("%1\n均值: %2")
                .arg(model_.categories[index])
                .arg(model_.values[index], 0, 'g', 8);
            if (index < model_.interval_lower.size()
                && index < model_.interval_upper.size()) {
                text += QStringLiteral("\n区间: [%1, %2]")
                    .arg(model_.interval_lower[index], 0, 'g', 8)
                    .arg(model_.interval_upper[index], 0, 'g', 8);
            }
        } else if ((model_.kind == ChartKind::Ecdf || model_.kind == ChartKind::TimeSeries
                    || model_.kind == ChartKind::Area)
                   && index < model_.values.size()
                   && index < model_.x_values.size()) {
            text = QStringLiteral("X: %1\nY: %2")
                .arg(model_.x_values[index], 0, 'g', 8)
                .arg(model_.values[index], 0, 'g', 8);
            if (index < model_.source_rows.size()) {
                text += QStringLiteral("\n原始行: ")
                    + QString::number(static_cast<qulonglong>(
                        model_.source_rows[index] + 1));
            }
        } else if ((model_.kind == ChartKind::Correlation || model_.kind == ChartKind::Heatmap
                    || model_.kind == ChartKind::Matrix)
                   && !model_.matrix_values.empty()) {
            const std::size_t n_cols = model_.matrix_labels.empty()
                ? model_.matrix_values.front().size()
                : static_cast<std::size_t>(model_.matrix_labels.size());
            const std::size_t columns = std::max<std::size_t>(n_cols, 1);
            const std::size_t row = index / columns;
            const std::size_t column = index % columns;
            QString row_label = QString::number(static_cast<int>(row));
            QString column_label = QString::number(static_cast<int>(column));
            if (model_.kind == ChartKind::Heatmap && !model_.categories.empty()
                && row < static_cast<std::size_t>(model_.categories.size())) {
                row_label = model_.categories[row];
            } else if (row < static_cast<std::size_t>(model_.matrix_labels.size())) {
                row_label = model_.matrix_labels[row];
            }
            if (column < static_cast<std::size_t>(model_.matrix_labels.size())) {
                column_label = model_.matrix_labels[column];
            }
            if (model_.kind == ChartKind::Matrix) {
                text = QStringLiteral("%1 × %2").arg(row_label, column_label);
            } else {
                const double value = row < model_.matrix_values.size()
                    && column < model_.matrix_values[row].size()
                    ? model_.matrix_values[row][column] : 0.0;
                text = QStringLiteral("%1 × %2\n值: %3").arg(row_label, column_label)
                    .arg(value, 0, 'g', 8);
            }
        } else if (model_.kind == ChartKind::Pie && index < model_.categories.size()) {
            text = model_.categories[index];
            if (index < model_.category_values.size()) {
                text += QStringLiteral("\n计数: ")
                    + QString::number(model_.category_values[index], 'g', 8);
            }
            if (index < model_.cumulative_percent.size()) {
                text += QStringLiteral("\n百分比: ")
                    + QString::number(model_.cumulative_percent[index], 'f', 2)
                    + QStringLiteral("%");
            }
        } else if (model_.kind == ChartKind::Probability
                   && index < model_.values.size()
                   && index < model_.x_values.size()) {
            text = QStringLiteral("点 %1\n观测值: %2\n理论分位数: %3")
                .arg(static_cast<qulonglong>(index + 1))
                .arg(model_.values[index], 0, 'g', 8)
                .arg(model_.x_values[index], 0, 'g', 8);
        }
        if (text.isEmpty()) {
            tooltip_timer_.stop();
            pending_tooltip_.clear();
            QToolTip::hideText();
        } else {
            pending_tooltip_ = text;
            pending_tooltip_position_ = event->globalPosition().toPoint();
            tooltip_timer_.start();
        }
    } else {
        tooltip_timer_.stop();
        pending_tooltip_.clear();
        model_.view.hovered_point.reset();
        QToolTip::hideText();
    }
    update();
}

void AnalysisChartWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton
        || (event->button() == Qt::LeftButton && space_pressed_)) {
        panning_ = true;
        last_mouse_position_ = event->position().toPoint();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        selection_start_ = event->position().toPoint();
        selection_end_ = selection_start_;
        selecting_ = true;
        const auto point = hit_test(selection_start_);
        if (point.has_value()) {
            if (!(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier))) {
                model_.view.selected_points.clear();
            }
            model_.view.selected_points.push_back(*point);
            std::sort(model_.view.selected_points.begin(), model_.view.selected_points.end());
            model_.view.selected_points.erase(
                std::unique(model_.view.selected_points.begin(), model_.view.selected_points.end()),
                model_.view.selected_points.end());
            emit_selected_rows();
            emit element_selected(chart_interaction::element_path(
                model_, element_for_hit(model_, *point)));
            update();
        }
    }
}

void AnalysisChartWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        panning_ = false;
    } else if (event->button() == Qt::LeftButton) {
        if ((selection_end_ - selection_start_).manhattanLength() > 8
            && !model_.values.empty()) {
            const QRectF plot = chart_geometry::plot_rect(rect(), model_.kind);
            const auto x_at = [&](std::size_t index) {
                return model_.x_values.size() == model_.values.size()
                    ? model_.x_values[index] : static_cast<double>(index);
            };
            double y_min = *std::min_element(model_.values.cbegin(), model_.values.cend());
            double y_max = *std::max_element(model_.values.cbegin(), model_.values.cend());
            if (qFuzzyCompare(y_min, y_max)) {
                y_min -= 1.0;
                y_max += 1.0;
            }
            const double y_padding = (y_max - y_min) * 0.08;
            double x_min = x_at(0);
            double x_max = std::max(x_at(0) + 1.0, x_at(model_.values.size() - 1));
            if (model_.x_min.has_value() && std::isfinite(*model_.x_min)) {
                x_min = *model_.x_min;
            }
            if (model_.x_max.has_value() && std::isfinite(*model_.x_max)) {
                x_max = *model_.x_max;
            }
            if (model_.y_min.has_value() && std::isfinite(*model_.y_min)) {
                y_min = *model_.y_min;
            } else {
                y_min -= y_padding;
            }
            if (model_.y_max.has_value() && std::isfinite(*model_.y_max)) {
                y_max = *model_.y_max;
            } else {
                y_max += y_padding;
            }
            ChartCoordinateMapper mapper(plot);
            mapper.set_data_range(x_min, std::max(x_min + 1.0, x_max), y_min, y_max);
            mapper.zoom(model_.view.zoom_factor, plot.center());
            mapper.pan(model_.view.pan_offset);
            if (event->modifiers() & Qt::ShiftModifier) {
                const QPointF first = mapper.to_data(selection_start_);
                const QPointF second = mapper.to_data(selection_end_);
                model_.x_min = std::min(first.x(), second.x());
                model_.x_max = std::max(first.x(), second.x());
                model_.y_min = std::min(first.y(), second.y());
                model_.y_max = std::max(first.y(), second.y());
                if (!(*model_.x_min < *model_.x_max)) {
                    const double mid = 0.5 * (*model_.x_min + *model_.x_max);
                    model_.x_min = mid - 1.0;
                    model_.x_max = mid + 1.0;
                }
                if (!(*model_.y_min < *model_.y_max)) {
                    const double mid = 0.5 * (*model_.y_min + *model_.y_max);
                    model_.y_min = mid - 1.0;
                    model_.y_max = mid + 1.0;
                }
                model_.view.zoom_factor = 1.0;
                model_.view.pan_offset = {};
                emit display_properties_changed(model_);
                update();
            } else {
            const auto to_index = [&](int x) {
                const double data_x =
                    mapper.to_data(QPointF(static_cast<double>(x), 0.0)).x();
                const double clamped = std::clamp(
                    data_x, 0.0, static_cast<double>(model_.values.size() - 1));
                return static_cast<std::size_t>(std::llround(clamped));
            };
            const std::size_t first = std::min(to_index(selection_start_.x()),
                                               to_index(selection_end_.x()));
            const std::size_t last = std::max(to_index(selection_start_.x()),
                                              to_index(selection_end_.x()));
            model_.view.selected_points.clear();
            for (std::size_t index = first; index <= last; ++index) {
                model_.view.selected_points.push_back(index);
            }
            emit_selected_rows();
            update();
            }
        }
        selecting_ = false;
    }
}

void AnalysisChartWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Copy)) {
        copy_to_clipboard();
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
    if (event->modifiers() & Qt::ControlModifier) {
        model_.view.zoom_factor *= event->angleDelta().y() > 0 ? 1.2 : (1.0 / 1.2);
        model_.view.zoom_factor = std::clamp(model_.view.zoom_factor, 0.25, 20.0);
        update();
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}

void AnalysisChartWidget::contextMenuEvent(QContextMenuEvent* event)
{
    event->accept();
    QMenu menu(this);
    QAction* element_edit_action = nullptr;
    const auto element = hit_test(event->pos());
    if (element.has_value()) {
        const chart_interaction::Element selected_element =
            element_for_hit(model_, *element);
        emit element_selected(chart_interaction::element_path(model_, selected_element));
        element_edit_action = menu.addAction(QStringLiteral("编辑当前对象…"));
    }
    QAction* edit_action = menu.addAction(QStringLiteral("编辑图形…"));
    QAction* save_action = menu.addAction(QStringLiteral("保存图形为 PNG…"));
    QAction* copy_action = menu.addAction(QStringLiteral("复制图形"));
    menu.addSeparator();
    QAction* fit_action = menu.addAction(QStringLiteral("适合窗口"));
    QAction* clear_action = menu.addAction(QStringLiteral("清除刷选"));
    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == edit_action || chosen == element_edit_action) {
        edit_graph();
    } else if (chosen == save_action) {
        save_graph();
    } else if (chosen == copy_action) {
        copy_to_clipboard();
    } else if (chosen == fit_action) {
        fit_to_window();
        update();
    } else if (chosen == clear_action) {
        model_.view.selected_points.clear();
        model_.view.hovered_point.reset();
        tooltip_timer_.stop();
        pending_tooltip_.clear();
        QToolTip::hideText();
        emit_selected_rows();
        update();
    }
}

void AnalysisChartWidget::edit_graph()
{
    if (editor_enabled_) {
        emit edit_requested();
        return;
    }
    GraphPropertiesDialog dialog(model_, this);
    if (dialog.exec() == QDialog::Accepted) {
        model_ = dialog.model();
        emit display_properties_changed(model_);
        update();
    }
}

void AnalysisChartWidget::save_graph()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("保存图形"),
        QString(),
        QStringLiteral("PNG 图片 (*.png)"));
    if (!path.isEmpty()) {
        const QPixmap pixmap = ChartRenderer::render_to_pixmap(
            model_, size(), devicePixelRatioF());
        pixmap.save(path, "PNG");
    }
}

void AnalysisChartWidget::copy_to_clipboard()
{
    const QPixmap pixmap = ChartRenderer::render_to_pixmap(
        model_, size(), devicePixelRatioF());
    if (pixmap.isNull()) {
        return;
    }
    const QImage image = pixmap.toImage();
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    auto* mime = new QMimeData();
    mime->setImageData(image);
    if (!png.isEmpty()) {
        mime->setData(QStringLiteral("image/png"), png);
    }
    QApplication::clipboard()->setMimeData(mime);
}

void AnalysisChartWidget::copy_graph()
{
    copy_to_clipboard();
}

void AnalysisChartWidget::fit_to_window()
{
    model_.view.zoom_factor = 1.0;
    model_.view.pan_offset = {};
    model_.x_min.reset();
    model_.x_max.reset();
    model_.y_min.reset();
    model_.y_max.reset();
    emit display_properties_changed(model_);
    update();
}

void AnalysisChartWidget::emit_selected_rows()
{
    std::vector<std::size_t> rows;
    rows.reserve(model_.view.selected_points.size());
    for (const std::size_t point : model_.view.selected_points) {
        if (point < model_.source_rows.size()) {
            rows.push_back(model_.source_rows[point]);
        }
    }
    emit rows_selected(rows);
}

std::optional<std::size_t> AnalysisChartWidget::hit_test(const QPoint& position) const
{
    const QRectF plot = chart_geometry::plot_rect(rect(), model_.kind);
    if (model_.kind == ChartKind::Histogram && model_.histogram_edges.size() >= 2) {
        const double maximum = *std::max_element(
            model_.histogram_counts.cbegin(), model_.histogram_counts.cend());
        ChartCoordinateMapper mapper(plot);
        mapper.set_data_range(model_.histogram_edges.front(), model_.histogram_edges.back(),
                              0.0, std::max(1.0, maximum));
        mapper.zoom(model_.view.zoom_factor, plot.center());
        const double x = mapper.to_data(position).x();
        for (std::size_t index = 0; index + 1 < model_.histogram_edges.size(); ++index) {
            if (x >= model_.histogram_edges[index] && x < model_.histogram_edges[index + 1]) {
                return index;
            }
        }
        return std::nullopt;
    }
    if (model_.kind == ChartKind::Pareto && !model_.category_values.empty()) {
        const double maximum = *std::max_element(
            model_.category_values.cbegin(), model_.category_values.cend());
        ChartCoordinateMapper mapper(plot);
        mapper.set_data_range(-0.5, static_cast<double>(model_.category_values.size()) - 0.5,
                              0.0, std::max(1.0, maximum));
        mapper.zoom(model_.view.zoom_factor, plot.center());
        const QPointF data = mapper.to_data(position);
        const auto index = static_cast<long long>(std::llround(data.x()));
        if (index >= 0 && index < static_cast<long long>(model_.category_values.size())) {
            return static_cast<std::size_t>(index);
        }
        return std::nullopt;
    }
    if (model_.kind == ChartKind::BoxPlot && !model_.box_median.empty()) {
        const double minimum = *std::min_element(model_.box_min.cbegin(), model_.box_min.cend());
        const double maximum = *std::max_element(model_.box_max.cbegin(), model_.box_max.cend());
        ChartCoordinateMapper mapper(plot);
        mapper.set_data_range(-0.5, static_cast<double>(model_.box_median.size()) - 0.5,
                              minimum, maximum);
        mapper.zoom(model_.view.zoom_factor, plot.center());
        const auto index = static_cast<long long>(std::llround(mapper.to_data(position).x()));
        if (index >= 0 && index < static_cast<long long>(model_.box_median.size())) {
            return static_cast<std::size_t>(index);
        }
        return std::nullopt;
    }
    if (model_.kind == ChartKind::Parallel && !model_.matrix_values.empty()) {
        const std::size_t axes = model_.matrix_labels.size();
        if (axes < 2) {
            return std::nullopt;
        }
        const QRectF parallel_plot = chart_geometry::plot_rect(rect(), model_.kind);
        std::optional<std::size_t> result;
        double distance = 14.0;
        for (std::size_t row = 0; row < model_.matrix_values.size(); ++row) {
            double row_distance = std::numeric_limits<double>::max();
            for (std::size_t axis = 0;
                 axis < axes && axis < model_.matrix_values[row].size(); ++axis) {
                const double minimum = axis < model_.lower.size() ? model_.lower[axis] : 0.0;
                const double maximum = axis < model_.upper.size() ? model_.upper[axis] : 1.0;
                const double value = model_.matrix_values[row][axis];
                if (!std::isfinite(value) || maximum <= minimum) {
                    continue;
                }
                const double fraction = std::clamp(
                    (value - minimum) / (maximum - minimum), 0.0, 1.0);
                const double x = parallel_plot.left()
                    + parallel_plot.width() * static_cast<double>(axis)
                        / static_cast<double>(axes - 1);
                row_distance = std::min(
                    row_distance, QLineF(QPointF(x, parallel_plot.bottom()
                        - fraction * parallel_plot.height()), position).length());
            }
            if (row_distance < distance) {
                distance = row_distance;
                result = row;
            }
        }
        return result;
    }
    if (model_.kind == ChartKind::Contour
        && model_.contour_x.size() >= 2 && model_.contour_y.size() >= 2
        && model_.matrix_values.size() >= model_.contour_y.size()) {
        ChartCoordinateMapper mapper(plot);
        mapper.set_data_range(model_.contour_x.front(), model_.contour_x.back(),
                              model_.contour_y.front(), model_.contour_y.back());
        const QPointF data = mapper.to_data(position);
        const auto column = static_cast<long long>(
            std::upper_bound(model_.contour_x.cbegin(), model_.contour_x.cend(), data.x())
            - model_.contour_x.cbegin()) - 1;
        const auto row = static_cast<long long>(
            std::upper_bound(model_.contour_y.cbegin(), model_.contour_y.cend(), data.y())
            - model_.contour_y.cbegin()) - 1;
        const long long columns = static_cast<long long>(model_.contour_x.size() - 1);
        const long long rows = static_cast<long long>(model_.contour_y.size() - 1);
        if (row >= 0 && row < rows && column >= 0 && column < columns
            && static_cast<std::size_t>(row) < model_.matrix_values.size()
            && static_cast<std::size_t>(column) < model_.matrix_values[row].size()) {
            return static_cast<std::size_t>(row) * static_cast<std::size_t>(columns)
                + static_cast<std::size_t>(column);
        }
        return std::nullopt;
    }
    if ((model_.kind == ChartKind::Correlation || model_.kind == ChartKind::Heatmap
         || model_.kind == ChartKind::Matrix)
        && !model_.matrix_values.empty()) {
        const QRectF area = rect();
        const std::vector<QString>& columns = model_.matrix_labels;
        const std::vector<QString>& rows = model_.categories.empty()
            ? model_.matrix_labels : model_.categories;
        if (model_.kind == ChartKind::Matrix) {
            const std::size_t count = model_.matrix_labels.size();
            if (count == 0) {
                return std::nullopt;
            }
            const double cell = std::min(area.width(), area.height())
                / static_cast<double>(count + 0.4);
            const QPointF origin(area.left() + 36.0, area.top() + 40.0);
            const int column = static_cast<int>((position.x() - origin.x()) / cell);
            const int row = static_cast<int>((position.y() - origin.y()) / cell);
            if (row >= 0 && column >= 0
                && row < static_cast<int>(count) && column < static_cast<int>(count)) {
                return static_cast<std::size_t>(row) * count + static_cast<std::size_t>(column);
            }
            return std::nullopt;
        }
        if (columns.empty() || rows.empty()) {
            return std::nullopt;
        }
        const double cell_w = std::min(
            72.0, (area.width() - 140.0) / static_cast<double>(columns.size()));
        const double cell_h = std::min(
            48.0, (area.height() - 90.0) / static_cast<double>(rows.size()));
        const QPointF origin(area.left() + 110.0, area.top() + 52.0);
        const int column = static_cast<int>((position.x() - origin.x()) / cell_w);
        const int row = static_cast<int>((position.y() - origin.y()) / cell_h);
        if (row >= 0 && column >= 0
            && row < static_cast<int>(rows.size())
            && column < static_cast<int>(columns.size())) {
            return static_cast<std::size_t>(row) * columns.size()
                + static_cast<std::size_t>(column);
        }
        return std::nullopt;
    }
    if (model_.kind == ChartKind::Pie && !model_.category_values.empty()) {
        const QRectF area = rect();
        const double size = std::min(area.width(), area.height()) * 0.55;
        const QRectF pie(area.center().x() - size / 2.0,
                         area.center().y() - size / 2.2, size, size);
        const QPointF center = pie.center();
        const double dx = static_cast<double>(position.x()) - center.x();
        const double dy = static_cast<double>(position.y()) - center.y();
        if (dx * dx + dy * dy > (size / 2.0) * (size / 2.0)) {
            return std::nullopt;
        }
        const double total = std::accumulate(
            model_.category_values.cbegin(), model_.category_values.cend(), 0.0);
        if (total <= 0.0) {
            return std::nullopt;
        }
        double angle = std::atan2(-dy, dx) * 180.0 / 3.14159265358979323846;
        double from_start = 90.0 - angle;
        if (from_start < 0.0) {
            from_start += 360.0;
        }
        double accumulated = 0.0;
        for (std::size_t index = 0; index < model_.category_values.size(); ++index) {
            accumulated += 360.0 * model_.category_values[index] / total;
            if (from_start <= accumulated) {
                return index;
            }
        }
        return model_.category_values.size() - 1;
    }
    if (model_.values.empty()) {
        return std::nullopt;
    }
    double minimum = *std::min_element(model_.values.cbegin(), model_.values.cend());
    double maximum = *std::max_element(model_.values.cbegin(), model_.values.cend());
    if (qFuzzyCompare(minimum, maximum)) {
        minimum -= 1.0;
        maximum += 1.0;
    }
    const double padding = (maximum - minimum) * 0.08;
    const auto x_at = [&](std::size_t index) {
        return model_.x_values.size() == model_.values.size()
            ? model_.x_values[index] : static_cast<double>(index);
    };
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(x_at(0), std::max(x_at(0) + 1.0, x_at(model_.values.size() - 1)),
                          minimum - padding, maximum + padding);
    mapper.zoom(model_.view.zoom_factor, plot.center());
    mapper.pan(model_.view.pan_offset);
    std::optional<std::size_t> result;
    double distance = 28.0;
    for (std::size_t i = 0; i < model_.values.size(); ++i) {
        const QPointF point = mapper.to_pixel(x_at(i), model_.values[i]);
        if (std::abs(point.x() - position.x()) > 28.0) {
            continue;
        }
        const double current = QLineF(point, position).length();
        if (current < distance) {
            distance = current;
            result = i;
        }
    }
    return result;
}
