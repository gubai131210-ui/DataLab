#include "ui/analysis_chart_widget.h"
#include "ui/graph_properties_dialog.h"

#include "reporting/chart_coordinate_mapper.h"
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
    model_.selected_points.clear();
    for (std::size_t index = 0; index < model_.source_rows.size(); ++index) {
        if (std::find(rows.begin(), rows.end(), model_.source_rows[index]) != rows.end()) {
            model_.selected_points.push_back(index);
        }
    }
    update();
}

const std::vector<std::size_t>& AnalysisChartWidget::selected_rows() const
{
    static thread_local std::vector<std::size_t> rows;
    rows.clear();
    for (const std::size_t point : model_.selected_points) {
        if (point < model_.source_rows.size()) {
            rows.push_back(model_.source_rows[point]);
        }
    }
    return rows;
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
        model_.pan_offset += current - last_mouse_position_;
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
        model_.hovered_point = index;
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
            for (std::size_t test = 0; test < model_.special_cause_points.size(); ++test) {
                if (std::find(model_.special_cause_points[test].cbegin(),
                              model_.special_cause_points[test].cend(), index)
                    != model_.special_cause_points[test].cend()) {
                    tests.push_back(QString::number(static_cast<int>(test + 1)));
                }
            }
            if (!tests.isEmpty()) {
                text += QStringLiteral("\n失败测试: ") + tests.join(QStringLiteral(", "));
            }
            if (model_.sigma_z > 0.0) {
                text += QStringLiteral("\nSigma Z: ")
                    + QString::number(model_.sigma_z, 'g', 6);
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
        } else if (model_.kind == ChartKind::Scatter
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
        } else if (model_.kind == ChartKind::Probability
                   && index < model_.values.size()
                   && index < model_.x_values.size()) {
            text = QStringLiteral("点 %1\n观测值: %2\n理论分位数: %3")
                .arg(static_cast<qulonglong>(index + 1))
                .arg(model_.x_values[index], 0, 'g', 8)
                .arg(model_.values[index], 0, 'g', 8);
        }
        QToolTip::showText(event->globalPosition().toPoint(), text, this);
    } else {
        model_.hovered_point.reset();
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
                model_.selected_points.clear();
            }
            model_.selected_points.push_back(*point);
            std::sort(model_.selected_points.begin(), model_.selected_points.end());
            model_.selected_points.erase(
                std::unique(model_.selected_points.begin(), model_.selected_points.end()),
                model_.selected_points.end());
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
            const QRectF plot = rect().adjusted(58.0, 42.0, -96.0, -48.0);
            const double denominator = std::max(1.0, plot.width());
            const auto to_index = [&](int x) {
                const double ratio = std::clamp((x - plot.left()) / denominator, 0.0, 1.0);
                return static_cast<std::size_t>(
                    std::llround(ratio * static_cast<double>(model_.values.size() - 1)));
            };
            const std::size_t first = std::min(to_index(selection_start_.x()),
                                               to_index(selection_end_.x()));
            const std::size_t last = std::max(to_index(selection_start_.x()),
                                              to_index(selection_end_.x()));
            model_.selected_points.clear();
            for (std::size_t index = first; index <= last; ++index) {
                model_.selected_points.push_back(index);
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
        model_.zoom_factor *= event->angleDelta().y() > 0 ? 1.2 : (1.0 / 1.2);
        model_.zoom_factor = std::clamp(model_.zoom_factor, 0.25, 20.0);
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
        model_.selected_points.clear();
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
    model_.zoom_factor = 1.0;
    model_.pan_offset = {};
}

void AnalysisChartWidget::emit_selected_rows()
{
    std::vector<std::size_t> rows;
    rows.reserve(model_.selected_points.size());
    for (const std::size_t point : model_.selected_points) {
        if (point < model_.source_rows.size()) {
            rows.push_back(model_.source_rows[point]);
        }
    }
    emit rows_selected(rows);
}

std::optional<std::size_t> AnalysisChartWidget::hit_test(const QPoint& position) const
{
    const QRectF plot = model_.kind == ChartKind::Pareto
        ? rect().adjusted(64.0, 42.0, -88.0, -178.0)
        : rect().adjusted(58.0, 42.0, -96.0, -48.0);
    if (model_.kind == ChartKind::Histogram && model_.histogram_edges.size() >= 2) {
        const double maximum = *std::max_element(
            model_.histogram_counts.cbegin(), model_.histogram_counts.cend());
        ChartCoordinateMapper mapper(plot);
        mapper.set_data_range(model_.histogram_edges.front(), model_.histogram_edges.back(),
                              0.0, std::max(1.0, maximum));
        mapper.zoom(model_.zoom_factor, plot.center());
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
        mapper.zoom(model_.zoom_factor, plot.center());
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
        mapper.zoom(model_.zoom_factor, plot.center());
        const auto index = static_cast<long long>(std::llround(mapper.to_data(position).x()));
        if (index >= 0 && index < static_cast<long long>(model_.box_median.size())) {
            return static_cast<std::size_t>(index);
        }
        return std::nullopt;
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
    mapper.zoom(model_.zoom_factor, plot.center());
    mapper.pan(model_.pan_offset);
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
