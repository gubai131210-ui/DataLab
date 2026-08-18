#include "ui/analysis_chart_widget.h"
#include "ui/graph_properties_dialog.h"

#include "reporting/chart_coordinate_mapper.h"
#include "reporting/chart_geometry.h"
#include "reporting/chart_renderer.h"

#include <QContextMenuEvent>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLineF>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QStringList>
#include <QToolTip>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <numeric>

AnalysisChartWidget::AnalysisChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(320);
    setAutoFillBackground(true);
    setMouseTracking(true);
    setContextMenuPolicy(Qt::DefaultContextMenu);
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
    fit_to_window();
    update();
}

void AnalysisChartWidget::set_model(const ChartModel& model)
{
    model_ = model;
    fit_to_window();
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
        const QPoint current = event->position().toPoint();
        model_.view.pan_offset += current - last_mouse_position_;
        last_mouse_position_ = current;
        update();
        return;
    }
    if (selecting_) {
        selection_end_ = event->position().toPoint();
        update();
        return;
    }
    const auto point = hit_test(event->position().toPoint());
    if (point.has_value()) {
        const std::size_t index = *point;
        model_.view.hovered_point = index;
        QString text;
        if (model_.kind == ChartKind::Control && index < model_.values.size()) {
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
        } else if ((model_.kind == ChartKind::Scatter || model_.kind == ChartKind::Bubble)
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
        QToolTip::showText(event->globalPosition().toPoint(), text, this);
    } else {
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
            // 与 hit_test 相同的映射（数据范围 + zoom + pan），框选在缩放/平移下不错位。
            const QRectF plot = chart_geometry::plot_rect(rect(), model_.kind);
            const auto x_at = [&](std::size_t index) {
                return model_.x_values.size() == model_.values.size()
                    ? model_.x_values[index] : static_cast<double>(index);
            };
            ChartCoordinateMapper mapper(plot);
            mapper.set_data_range(
                x_at(0), std::max(x_at(0) + 1.0, x_at(model_.values.size() - 1)),
                0.0, 1.0);
            mapper.zoom(model_.view.zoom_factor, plot.center());
            mapper.pan(model_.view.pan_offset);
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
        selecting_ = false;
    }
}

void AnalysisChartWidget::keyPressEvent(QKeyEvent* event)
{
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
    QMenu menu(this);
    QAction* edit_action = menu.addAction(QStringLiteral("编辑图形…"));
    QAction* save_action = menu.addAction(QStringLiteral("保存图形为 PNG…"));
    QAction* copy_action = menu.addAction(QStringLiteral("复制图形"));
    menu.addSeparator();
    QAction* fit_action = menu.addAction(QStringLiteral("适合窗口"));
    QAction* clear_action = menu.addAction(QStringLiteral("清除刷选"));
    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == edit_action) {
        edit_graph();
    } else if (chosen == save_action) {
        save_graph();
    } else if (chosen == copy_action) {
        copy_graph();
    } else if (chosen == fit_action) {
        fit_to_window();
        update();
    } else if (chosen == clear_action) {
        model_.view.selected_points.clear();
        emit_selected_rows();
        update();
    }
}

void AnalysisChartWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton
        && event->position().y() < 50.0) {
        edit_graph();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void AnalysisChartWidget::edit_graph()
{
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
        grab().save(path, "PNG");
    }
}

void AnalysisChartWidget::copy_graph()
{
    QApplication::clipboard()->setPixmap(grab());
}

void AnalysisChartWidget::fit_to_window()
{
    model_.view.zoom_factor = 1.0;
    model_.view.pan_offset = {};
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
