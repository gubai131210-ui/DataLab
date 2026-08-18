#include "chart_renderer.h"

#include "reporting/chart_coordinate_mapper.h"
#include "reporting/chart_geometry.h"
#include "domain/statistics/normal_distribution.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPolygonF>
#include <QtCore/QHash>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace {
QVector<double> finite_values(const std::vector<double>& values)
{
    QVector<double> result;
    for (const double value : values) {
        if (std::isfinite(value)) {
            result.append(value);
        }
    }
    return result;
}

Qt::PenStyle qt_line_style(const ChartLineStyle style)
{
    switch (style) {
    case ChartLineStyle::Dash: return Qt::DashLine;
    case ChartLineStyle::Dot: return Qt::DotLine;
    case ChartLineStyle::DashDot: return Qt::DashDotLine;
    case ChartLineStyle::Solid:
    default: return Qt::SolidLine;
    }
}

QColor series_color(const ChartSeries& series)
{
    const QColor configured(series.style.color);
    return configured.isValid() ? configured : QColor("#455a64");
}

void draw_point(
    QPainter& painter, const QPointF& point, const ChartPointStyle style, const double size)
{
    const double safe_size = std::clamp(size, 1.0, 12.0);
    switch (style) {
    case ChartPointStyle::Square:
        painter.drawRect(QRectF(point.x() - safe_size, point.y() - safe_size,
                                safe_size * 2.0, safe_size * 2.0));
        break;
    case ChartPointStyle::Triangle: {
        QPolygonF triangle;
        triangle << QPointF(point.x(), point.y() - safe_size)
                 << QPointF(point.x() + safe_size, point.y() + safe_size)
                 << QPointF(point.x() - safe_size, point.y() + safe_size);
        painter.drawPolygon(triangle);
        break;
    }
    case ChartPointStyle::Cross:
        painter.drawLine(QPointF(point.x() - safe_size, point.y() - safe_size),
                         QPointF(point.x() + safe_size, point.y() + safe_size));
        painter.drawLine(QPointF(point.x() - safe_size, point.y() + safe_size),
                         QPointF(point.x() + safe_size, point.y() - safe_size));
        break;
    case ChartPointStyle::Circle:
        painter.drawEllipse(point, safe_size, safe_size);
        break;
    case ChartPointStyle::None:
    default:
        break;
    }
}

struct ChartThemeColors {
    QColor background;
    QColor text;
    QColor muted_text;
    QColor grid;
    QColor axis;
};

ChartThemeColors theme_colors(const ChartModel& model)
{
    ChartThemeColors colors;
    const QString preset = model.theme_preset.toLower();
    if (preset == QStringLiteral("dark")) {
        colors.background = QColor(QStringLiteral("#1e1e1e"));
        colors.text = QColor(QStringLiteral("#e0e0e0"));
        colors.muted_text = QColor(QStringLiteral("#9e9e9e"));
        colors.grid = QColor(QStringLiteral("#3a3a3a"));
        colors.axis = QColor(QStringLiteral("#bdbdbd"));
        return colors;
    }
    if (preset == QStringLiteral("print")) {
        colors.background = QColor(QStringLiteral("#ffffff"));
        colors.text = QColor(QStringLiteral("#000000"));
        colors.muted_text = QColor(QStringLiteral("#424242"));
        colors.grid = QColor(QStringLiteral("#d0d0d0"));
        colors.axis = QColor(QStringLiteral("#000000"));
        return colors;
    }
    colors.background = QColor(QStringLiteral("#ffffff"));
    colors.text = QColor(QStringLiteral("#263238"));
    colors.muted_text = QColor(QStringLiteral("#5d6872"));
    colors.grid = QColor(QStringLiteral("#e3e7eb"));
    colors.axis = QColor(QStringLiteral("#263238"));
    const QColor custom_grid(model.grid_color);
    if (custom_grid.isValid()
        && model.grid_color.compare(QStringLiteral("#e3e7eb"), Qt::CaseInsensitive) != 0) {
        colors.grid = custom_grid;
    }
    return colors;
}

QRectF plot_rect(const QRectF& area)
{
    return chart_geometry::plot_rect(area, ChartKind::Control);
}

QRectF pareto_plot_rect(const QRectF& area)
{
    return chart_geometry::plot_rect(area, ChartKind::Pareto);
}

void apply_custom_y_range(double& y_min, double& y_max, const ChartModel& model)
{
    const bool custom_min = model.y_min.has_value() && std::isfinite(*model.y_min);
    const bool custom_max = model.y_max.has_value() && std::isfinite(*model.y_max);
    if (custom_min) {
        y_min = *model.y_min;
    }
    if (custom_max) {
        y_max = *model.y_max;
    }
    if (custom_min && custom_max && !(y_min < y_max)) {
        const double mid = 0.5 * (y_min + y_max);
        y_min = mid - 1.0;
        y_max = mid + 1.0;
    }
}

void apply_custom_x_range(double& x_min, double& x_max, const ChartModel& model)
{
    const bool custom_min = model.x_min.has_value() && std::isfinite(*model.x_min);
    const bool custom_max = model.x_max.has_value() && std::isfinite(*model.x_max);
    if (custom_min) {
        x_min = *model.x_min;
    }
    if (custom_max) {
        x_max = *model.x_max;
    }
    if (custom_min && custom_max && !(x_min < x_max)) {
        const double mid = 0.5 * (x_min + x_max);
        x_min = mid - 1.0;
        x_max = mid + 1.0;
    }
}

void fill_data_region(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    if (model.data_region_fill.isEmpty()) {
        return;
    }
    const QColor fill(model.data_region_fill);
    if (!fill.isValid() || fill.alpha() == 0) {
        return;
    }
    const QRectF plot = model.kind == ChartKind::Pareto
        ? pareto_plot_rect(area) : plot_rect(area);
    painter.fillRect(plot, fill);
}

void draw_title_and_axes(
    QPainter& painter,
    const QRectF& area,
    const QRectF& plot,
    const ChartModel& model)
{
    painter.setPen(theme_colors(model).text);
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), model.title_font_size, QFont::Bold));
    painter.drawText(QRectF(area.left(), area.top(), area.width(), 26.0),
                     Qt::AlignCenter, model.title);
    if (!model.subtitle.isEmpty()) {
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"),
                               std::max(6, model.axis_font_size - 1)));
        painter.drawText(QRectF(area.left(), area.top() + 24.0, area.width(), 18.0),
                         Qt::AlignCenter, model.subtitle);
    }
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), model.axis_font_size));
    painter.drawText(QRectF(plot.left(), area.bottom() - 34.0, plot.width(), 20.0),
                     Qt::AlignCenter, model.x_axis_title);
    painter.save();
    painter.translate(area.left() + 14.0, plot.center().y());
    painter.rotate(-90.0);
    painter.drawText(QRectF(-plot.height() / 2.0, -10.0, plot.height(), 20.0),
                     Qt::AlignCenter, model.y_axis_title);
    painter.restore();
}

void render_control(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = plot_rect(area);
    const ChartSeries* first_series = nullptr;
    for (const ChartSeries& series : model.series) {
        if (!series.values.empty() || !series.lower.empty() || !series.upper.empty()) {
            first_series = &series;
            break;
        }
    }
    if (model.values.empty() && first_series == nullptr) {
        painter.setPen(theme_colors(model).muted_text);
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }

    QVector<double> all_values = finite_values(model.values);
    for (const double value : finite_values(model.lower)) {
        all_values.append(value);
    }
    for (const double value : finite_values(model.upper)) {
        all_values.append(value);
    }
    for (const double value : finite_values(model.center)) {
        all_values.append(value);
    }
    for (const ChartSeries& series : model.series) {
        for (const double value : finite_values(series.values)) {
            all_values.append(value);
        }
        for (const double value : finite_values(series.lower)) {
            all_values.append(value);
        }
        for (const double value : finite_values(series.upper)) {
            all_values.append(value);
        }
    }
    if (all_values.isEmpty()) {
        return;
    }
    auto [minimum_it, maximum_it] = std::minmax_element(all_values.cbegin(), all_values.cend());
    double minimum = *minimum_it;
    double maximum = *maximum_it;
    if (qFuzzyCompare(minimum, maximum)) {
        minimum -= 1.0;
        maximum += 1.0;
    } else {
        const double padding = (maximum - minimum) * 0.08;
        minimum -= padding;
        maximum += padding;
    }
    apply_custom_y_range(minimum, maximum, model);

    const auto x_at = [&](std::size_t index) {
        return model.x_values.size() == model.values.size()
            ? model.x_values[index] : static_cast<double>(index);
    };
    const std::vector<double>& base_values = model.values.empty()
        ? (!first_series->values.empty()
               ? first_series->values
               : (!first_series->lower.empty() ? first_series->lower : first_series->upper))
        : model.values;
    const auto series_x_at = [](const ChartSeries& series, std::size_t index) {
        return index < series.x_values.size()
            ? series.x_values[index] : static_cast<double>(index);
    };
    double x_min = model.values.empty() ? series_x_at(*first_series, 0) : x_at(0);
    double x_max = x_min;
    if (!model.values.empty() && model.x_values.size() == model.values.size()) {
        const auto x_range = std::minmax_element(
            model.x_values.cbegin(), model.x_values.cend());
        x_min = *x_range.first;
        x_max = *x_range.second;
    } else {
        x_max = model.values.empty()
            ? series_x_at(*first_series, base_values.size() - 1)
            : x_at(model.values.size() - 1);
    }
    for (const ChartSeries& series : model.series) {
        if (series.values.empty()) {
            continue;
        }
        double series_min = series_x_at(series, 0);
        double series_max = series_min;
        if (series.x_values.size() == series.values.size()) {
            const auto range = std::minmax_element(
                series.x_values.cbegin(), series.x_values.cend());
            series_min = *range.first;
            series_max = *range.second;
        } else {
            series_max = series_x_at(series, series.values.size() - 1);
        }
        x_min = std::min(x_min, series_min);
        x_max = std::max(x_max, series_max);
    }
    apply_custom_x_range(x_min, x_max, model);
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(x_min, std::max(x_min + 1.0, x_max),
                          minimum, maximum);
    mapper.zoom(model.view.zoom_factor, plot.center());
    mapper.pan(model.view.pan_offset);

    painter.setPen(QPen(theme_colors(model).grid, 1.0));
    for (int tick = 0; tick <= 5; ++tick) {
        const double value = minimum + (maximum - minimum) * tick / 5.0;
        const QPointF point = mapper.to_pixel(x_min, value);
        if (model.show_grid) {
            painter.drawLine(QPointF(plot.left(), point.y()), QPointF(plot.right(), point.y()));
        }
        painter.setPen(theme_colors(model).muted_text);
        painter.drawText(QRectF(area.left(), point.y() - 10.0, 52.0, 20.0),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(value, 'g', 5));
        painter.setPen(QPen(theme_colors(model).grid, 1.0));
    }
    painter.setPen(QPen(theme_colors(model).axis, 1.2));
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());

    const auto draw_right_label = [&](const std::vector<double>& line,
                                      const QString& prefix,
                                      const QColor& color,
                                      Qt::PenStyle style) {
        if (line.empty()) {
            return;
        }
        const double value = line.front();
        const QPointF point = mapper.to_pixel(x_min, value);
        painter.setPen(QPen(color, 1.0, style));
        painter.drawLine(QPointF(plot.right(), point.y()),
                         QPointF(plot.right() + 8.0, point.y()));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));
        painter.drawText(QRectF(plot.right() + 10.0, point.y() - 9.0, area.right() - plot.right() - 12.0, 18.0),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         prefix + QStringLiteral("=") + QString::number(value, 'g', 6));
    };

    const auto draw_series = [&](const std::vector<double>& series, const QPen& pen) {
        if (series.empty()) {
            return;
        }
        painter.setPen(pen);
        const std::size_t count = std::min(series.size(), model.values.size());
        for (std::size_t i = 1; i < count; ++i) {
            if (!std::isfinite(series[i - 1]) || !std::isfinite(series[i])) {
                continue;
            }
            painter.drawLine(mapper.to_pixel(x_at(i - 1), series[i - 1]),
                             mapper.to_pixel(x_at(i), series[i]));
        }
    };
    const auto draw_extra_series = [&](const ChartSeries& series) {
        if (!series.style.visible) {
            return;
        }
        const std::size_t count = std::max({
            series.values.size(), series.lower.size(), series.upper.size()});
        if (count == 0) {
            return;
        }
        const QColor color = series_color(series);
        if (series.role == ChartSeriesRole::ConfidenceBand
            && series.lower.size() == count && series.upper.size() == count) {
            QPainterPath band;
            bool started = false;
            for (std::size_t i = 0; i < count; ++i) {
                if (!std::isfinite(series.lower[i]) || !std::isfinite(series_x_at(series, i))) {
                    continue;
                }
                const QPointF point = mapper.to_pixel(
                    series_x_at(series, i), series.lower[i]);
                if (!started) {
                    band.moveTo(point);
                    started = true;
                } else {
                    band.lineTo(point);
                }
            }
            for (std::size_t i = count; i-- > 0;) {
                if (!std::isfinite(series.upper[i]) || !std::isfinite(series_x_at(series, i))) {
                    continue;
                }
                band.lineTo(mapper.to_pixel(series_x_at(series, i), series.upper[i]));
            }
            if (started) {
                band.closeSubpath();
                painter.fillPath(band, QColor(144, 202, 249, 75));
            }
        }
        painter.setPen(QPen(color, series.style.line_width,
                            qt_line_style(series.style.line_style)));
        for (std::size_t i = 1; i < count; ++i) {
            if (i < series.values.size() && i - 1 < series.values.size()
                && std::isfinite(series.values[i - 1]) && std::isfinite(series.values[i])) {
                painter.drawLine(
                    mapper.to_pixel(series_x_at(series, i - 1), series.values[i - 1]),
                    mapper.to_pixel(series_x_at(series, i), series.values[i]));
            }
        }
        if (series.show_points) {
            painter.setBrush(color);
            painter.setPen(Qt::NoPen);
            for (std::size_t i = 0; i < series.values.size(); ++i) {
                if (std::isfinite(series.values[i])) {
                    const QPointF point = mapper.to_pixel(
                        series_x_at(series, i), series.values[i]);
                    draw_point(painter, point, series.style.point_style,
                               series.style.point_size);
                }
            }
        }
    };
    if (model.lower_style.visible) {
        draw_series(model.lower, QPen(
            QColor(model.lower_style.color), model.lower_style.line_width,
            qt_line_style(model.lower_style.line_style)));
    }
    if (model.upper_style.visible) {
        draw_series(model.upper, QPen(
            QColor(model.upper_style.color), model.upper_style.line_width,
            qt_line_style(model.upper_style.line_style)));
    }
    if (model.center_style.visible) {
        draw_series(model.center, QPen(
            QColor(model.center_style.color), model.center_style.line_width,
            qt_line_style(model.center_style.line_style)));
    }
    if (model.value_style.visible) {
        draw_series(model.values, QPen(
            QColor(model.value_style.color), model.value_style.line_width,
            qt_line_style(model.value_style.line_style)));
    }
    for (const ChartSeries& series : model.series) {
        draw_extra_series(series);
    }
    if (model.upper_style.visible) {
        draw_right_label(model.upper, model.upper_style.label,
                         QColor(model.upper_style.color),
                         qt_line_style(model.upper_style.line_style));
    }
    if (model.center_style.visible) {
        draw_right_label(model.center, model.center_style.label,
                         QColor(model.center_style.color),
                         qt_line_style(model.center_style.line_style));
    }
    if (model.lower_style.visible) {
        draw_right_label(model.lower, model.lower_style.label,
                         QColor(model.lower_style.color),
                         qt_line_style(model.lower_style.line_style));
    }

    painter.setPen(Qt::NoPen);
    for (std::size_t i = 0; i < model.values.size(); ++i) {
        const bool selected = std::find(model.view.selected_points.cbegin(),
                                        model.view.selected_points.cend(), i)
            != model.view.selected_points.cend();
        const auto failed_test_number = [&]() -> int {
            for (std::size_t test = 0; test < model.special_cause_points.size(); ++test) {
                if (std::find(model.special_cause_points[test].cbegin(),
                              model.special_cause_points[test].cend(), i)
                    != model.special_cause_points[test].cend()) {
                    return static_cast<int>(test + 1);
                }
            }
            return 0;
        };
        const bool out_of_control = i < model.lower.size() && i < model.upper.size()
            && (model.values[i] < model.lower[i] || model.values[i] > model.upper[i]);
        const int special_cause_test = failed_test_number();
        const bool special_cause = out_of_control || special_cause_test > 0;
        const QColor value_color(model.value_style.color);
        painter.setBrush(selected ? QColor("#ff9800")
                                   : (special_cause ? QColor("#d32f2f") : value_color));
        if (!std::isfinite(model.values[i])) {
            continue;
        }
        const QPointF point = mapper.to_pixel(x_at(i), model.values[i]);
        if (model.view.hovered_point.has_value() && *model.view.hovered_point == i) {
            painter.setPen(QPen(QColor("#0d47a1"), 2.0));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(point, 8.0, 8.0);
            painter.setBrush(selected ? QColor("#ff9800")
                                       : (special_cause ? QColor("#d32f2f") : value_color));
        }
        draw_point(painter, point, model.value_style.point_style,
                   model.value_style.point_size);
        if (special_cause) {
            painter.setPen(QColor("#d32f2f"));
            painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 7));
            const std::size_t row = i < model.source_rows.size() ? model.source_rows[i] + 1 : i + 1;
            const QString label = special_cause_test > 0
                ? QStringLiteral("%1 (%2)").arg(static_cast<qulonglong>(row))
                    .arg(special_cause_test)
                : QString::number(static_cast<qulonglong>(row));
            painter.drawText(point + QPointF(4.0, -4.0), label);
            painter.setPen(Qt::NoPen);
        }
    }

    draw_title_and_axes(painter, area, plot, model);
    if (!model.show_legend) {
        return;
    }
    const QRectF legend(area.right() - 260.0, area.top() + 28.0, 245.0, 18.0);
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), model.legend_font_size));
    double legend_x = legend.left();
    const auto draw_legend_item = [&](const QString& label, const QColor& color) {
        if (label.isEmpty() || legend_x >= legend.right()) {
            return;
        }
        painter.setPen(color);
        painter.drawText(QRectF(legend_x, legend.top(), legend.right() - legend_x, 18.0),
                         Qt::AlignLeft, QStringLiteral("— ") + label);
        legend_x += 76.0;
    };
    if (model.value_style.visible) {
        draw_legend_item(QStringLiteral("观测值"), QColor(model.value_style.color));
    }
    if (model.center_style.visible) {
        draw_legend_item(model.center_style.label, QColor(model.center_style.color));
    }
    if (model.lower_style.visible || model.upper_style.visible) {
        draw_legend_item(QStringLiteral("控制限"), QColor(model.lower_style.color));
    }
    double legend_y = legend.bottom() + 2.0;
    for (const ChartSeries& series : model.series) {
        if (series.label.isEmpty() || !series.style.visible) {
            continue;
        }
        painter.setPen(series_color(series));
        painter.drawText(QRectF(legend.left(), legend_y, legend.width(), 16.0),
                         Qt::AlignLeft, QStringLiteral("— ") + series.label);
        legend_y += 15.0;
    }
}

void render_scatter(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = plot_rect(area);
    const std::size_t count = std::min(model.x_values.size(), model.values.size());
    if (count == 0) {
        return;
    }
    const auto x_range = std::minmax_element(
        model.x_values.cbegin(), model.x_values.cbegin() + count);
    const auto y_range = std::minmax_element(
        model.values.cbegin(), model.values.cbegin() + count);
    double x_min = *x_range.first;
    double x_max = *x_range.second;
    double y_min = *y_range.first;
    double y_max = *y_range.second;
    for (const ChartSeries& series : model.series) {
        const std::size_t series_count = series.values.size();
        const std::size_t interval_count = std::max(series.lower.size(), series.upper.size());
        if (series_count == 0 && interval_count == 0) {
            continue;
        }
        if (series_count > 0) {
            const auto series_y_range = std::minmax_element(
                series.values.cbegin(), series.values.cend());
            y_min = std::min(y_min, *series_y_range.first);
            y_max = std::max(y_max, *series_y_range.second);
        }
        if (series.lower.size() == interval_count && interval_count > 0) {
            const auto range = std::minmax_element(
                series.lower.cbegin(), series.lower.cend());
            y_min = std::min(y_min, *range.first);
            y_max = std::max(y_max, *range.second);
        }
        if (series.upper.size() == interval_count && interval_count > 0) {
            const auto range = std::minmax_element(
                series.upper.cbegin(), series.upper.cend());
            y_min = std::min(y_min, *range.first);
            y_max = std::max(y_max, *range.second);
        }
        if (series.x_values.size() == series_count) {
            const auto series_x_range = std::minmax_element(
                series.x_values.cbegin(), series.x_values.cend());
            x_min = std::min(x_min, *series_x_range.first);
            x_max = std::max(x_max, *series_x_range.second);
        }
    }
    if (x_min == x_max) {
        x_min -= 1.0;
        x_max += 1.0;
    }
    if (y_min == y_max) {
        y_min -= 1.0;
        y_max += 1.0;
    }
    const double x_padding = 0.05 * (x_max - x_min);
    const double y_padding = 0.05 * (y_max - y_min);
    x_min -= x_padding;
    x_max += x_padding;
    y_min -= y_padding;
    y_max += y_padding;
    apply_custom_x_range(x_min, x_max, model);
    apply_custom_y_range(y_min, y_max, model);
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(x_min, x_max, y_min, y_max);
    mapper.zoom(model.view.zoom_factor, plot.center());
    mapper.pan(model.view.pan_offset);
    painter.setPen(theme_colors(model).grid);
    if (model.show_grid) {
        for (int tick = 0; tick <= 5; ++tick) {
            const double fraction = static_cast<double>(tick) / 5.0;
            const double x = plot.left() + fraction * plot.width();
            const double y = plot.bottom() - fraction * plot.height();
            painter.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
            painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        }
    }
    painter.setPen(theme_colors(model).axis);
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    painter.setBrush(QColor("#1565c0"));
    painter.setPen(Qt::NoPen);
    for (std::size_t index = 0; index < count; ++index) {
        if (!std::isfinite(model.x_values[index])
            || !std::isfinite(model.values[index])) {
            continue;
        }
        const QPointF point = mapper.to_pixel(model.x_values[index], model.values[index]);
        const bool selected = std::find(model.view.selected_points.cbegin(),
                                        model.view.selected_points.cend(), index)
            != model.view.selected_points.cend();
        if (selected || (model.view.hovered_point.has_value()
                         && *model.view.hovered_point == index)) {
            painter.setBrush(selected ? QColor("#ff9800") : QColor("#1565c0"));
            painter.drawEllipse(point, selected ? 5.0 : 4.5, selected ? 5.0 : 4.5);
        }
        if (model.view.hovered_point.has_value() && *model.view.hovered_point == index) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor("#0d47a1"), 2.0));
            painter.drawEllipse(point, 8.0, 8.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(selected ? QColor("#ff9800") : QColor("#1565c0"));
        }
        painter.drawEllipse(point, 3.5, 3.5);
    }
    for (const ChartSeries& series : model.series) {
        if (!series.style.visible) {
            continue;
        }
        const std::size_t count = std::max({
            series.values.size(), series.lower.size(), series.upper.size()});
        if (count == 0) {
            continue;
        }
        const auto x_at = [&](std::size_t index) {
            return index < series.x_values.size()
                ? series.x_values[index] : static_cast<double>(index);
        };
        const QColor color = series_color(series);
        painter.setPen(QPen(color, series.style.line_width,
                            qt_line_style(series.style.line_style)));
        for (std::size_t index = 1; index < series.values.size(); ++index) {
            if (std::isfinite(series.values[index - 1])
                && std::isfinite(series.values[index])) {
                painter.drawLine(mapper.to_pixel(x_at(index - 1), series.values[index - 1]),
                                 mapper.to_pixel(x_at(index), series.values[index]));
            }
        }
        if (series.role == ChartSeriesRole::ConfidenceBand
            && series.lower.size() == count && series.upper.size() == count) {
            QPainterPath band;
            for (std::size_t index = 0; index < series.lower.size(); ++index) {
                const QPointF point = mapper.to_pixel(x_at(index), series.lower[index]);
                index == 0 ? band.moveTo(point) : band.lineTo(point);
            }
            for (std::size_t index = series.upper.size(); index-- > 0;) {
                band.lineTo(mapper.to_pixel(x_at(index), series.upper[index]));
            }
            band.closeSubpath();
            painter.fillPath(band, QColor(144, 202, 249, 75));
        }
    }
    painter.setPen(theme_colors(model).text);
    painter.drawText(QRectF(plot.left(), area.top() + 8.0, plot.width(), 24.0),
                     Qt::AlignCenter, model.title);
    painter.drawText(QRectF(plot.left(), plot.bottom() + 8.0, plot.width(), 24.0),
                     Qt::AlignCenter, model.x_axis_title);
    painter.save();
    painter.translate(area.left() + 16.0, plot.center().y());
    painter.rotate(-90.0);
    painter.drawText(QRectF(-plot.height() / 2.0, -16.0, plot.height(), 24.0),
                     Qt::AlignCenter, model.y_axis_title);
    painter.restore();
}

void render_probability(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = plot_rect(area);
    if (model.values.empty() || model.values.size() != model.x_values.size()
        || plot.width() <= 1.0 || plot.height() <= 1.0) {
        painter.setPen(theme_colors(model).muted_text);
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    const auto [x_min_it, x_max_it] =
        std::minmax_element(model.x_values.cbegin(), model.x_values.cend());
    const auto [y_min_it, y_max_it] =
        std::minmax_element(model.values.cbegin(), model.values.cend());
    double x_min = *x_min_it;
    double x_max = *x_max_it;
    double y_min = *y_min_it;
    double y_max = *y_max_it;
    if (qFuzzyCompare(x_min, x_max)) {
        x_min -= 1.0;
        x_max += 1.0;
    }
    if (qFuzzyCompare(y_min, y_max)) {
        y_min -= 1.0;
        y_max += 1.0;
    }
    const double x_padding = (x_max - x_min) * 0.08;
    const double y_padding = (y_max - y_min) * 0.08;
    x_min -= x_padding;
    x_max += x_padding;
    y_min -= y_padding;
    y_max += y_padding;
    apply_custom_x_range(x_min, x_max, model);
    apply_custom_y_range(y_min, y_max, model);
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(x_min, x_max, y_min, y_max);
    mapper.zoom(model.view.zoom_factor, plot.center());
    mapper.pan(model.view.pan_offset);
    painter.setPen(QPen(theme_colors(model).grid, 1.0));
    for (int tick = 0; tick <= 5; ++tick) {
        const double x = x_min + (x_max - x_min) * tick / 5.0;
        const double y = y_min + (y_max - y_min) * tick / 5.0;
        const QPointF x_point = mapper.to_pixel(x, y_min);
        const QPointF y_point = mapper.to_pixel(x_min, y);
        if (model.show_grid) {
            painter.drawLine(QPointF(x_point.x(), plot.top()),
                             QPointF(x_point.x(), plot.bottom()));
            painter.drawLine(QPointF(plot.left(), y_point.y()),
                             QPointF(plot.right(), y_point.y()));
        }
        painter.setPen(theme_colors(model).muted_text);
        painter.drawText(QRectF(x_point.x() - 30.0, plot.bottom() + 2.0, 60.0, 18.0),
                         Qt::AlignCenter, QString::number(x, 'g', 3));
        painter.drawText(QRectF(area.left(), y_point.y() - 9.0, 52.0, 18.0),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(y, 'g', 5));
        painter.setPen(QPen(theme_colors(model).grid, 1.0));
    }
    painter.setPen(QPen(theme_colors(model).axis, 1.2));
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());

    double mean_x = 0.0;
    double mean_y = 0.0;
    for (std::size_t index = 0; index < model.values.size(); ++index) {
        mean_x += model.x_values[index];
        mean_y += model.values[index];
    }
    mean_x /= static_cast<double>(model.values.size());
    mean_y /= static_cast<double>(model.values.size());
    double covariance = 0.0;
    double variance_x = 0.0;
    for (std::size_t index = 0; index < model.values.size(); ++index) {
        const double dx = model.x_values[index] - mean_x;
        covariance += dx * (model.values[index] - mean_y);
        variance_x += dx * dx;
    }
    const double slope = variance_x > 0.0 ? covariance / variance_x : 0.0;
    const double intercept = mean_y - slope * mean_x;
    if (model.lower.size() == model.x_values.size()
        && model.upper.size() == model.x_values.size()
        && !model.x_values.empty()) {
        QPainterPath band;
        band.moveTo(mapper.to_pixel(model.x_values.front(), model.upper.front()));
        for (std::size_t index = 0; index < model.x_values.size(); ++index) {
            band.lineTo(mapper.to_pixel(model.x_values[index], model.upper[index]));
        }
        for (std::size_t index = model.x_values.size(); index-- > 0; ) {
            band.lineTo(mapper.to_pixel(model.x_values[index], model.lower[index]));
        }
        band.closeSubpath();
        painter.fillPath(band, QColor(198, 40, 40, 40));
    }
    if (model.center.size() == model.x_values.size() && model.center.size() >= 2) {
        painter.setPen(QPen(QColor("#c62828"), 1.4));
        painter.drawLine(mapper.to_pixel(model.x_values.front(), model.center.front()),
                         mapper.to_pixel(model.x_values.back(), model.center.back()));
    } else {
        painter.setPen(QPen(QColor("#c62828"), 1.4));
        painter.drawLine(mapper.to_pixel(x_min, intercept + slope * x_min),
                         mapper.to_pixel(x_max, intercept + slope * x_max));
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#1565c0"));
    for (std::size_t index = 0; index < model.values.size(); ++index) {
        const QPointF point = mapper.to_pixel(model.x_values[index], model.values[index]);
        const bool selected = std::find(model.view.selected_points.cbegin(),
                                        model.view.selected_points.cend(), index)
            != model.view.selected_points.cend();
        const bool hovered = model.view.hovered_point.has_value()
            && *model.view.hovered_point == index;
        painter.setBrush(selected ? QColor("#ff9800") : QColor("#1565c0"));
        painter.drawEllipse(point, selected ? 5.0 : 3.5, selected ? 5.0 : 3.5);
        if (hovered) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor("#0d47a1"), 2.0));
            painter.drawEllipse(point, 8.0, 8.0);
            painter.setPen(Qt::NoPen);
        }
    }
    draw_title_and_axes(painter, area, plot, model);
}

void render_histogram(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = plot_rect(area);
    if (model.histogram_counts.empty() || model.histogram_edges.size() < 2) {
        painter.setPen(theme_colors(model).muted_text);
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    const double x_min = model.histogram_edges.front();
    const double x_max = model.histogram_edges.back();
    double y_max = *std::max_element(model.histogram_counts.begin(), model.histogram_counts.end());
    if (y_max <= 0.0) {
        y_max = 1.0;
    }
    y_max *= 1.15;
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(x_min, x_max, 0.0, y_max);
    mapper.zoom(model.view.zoom_factor, plot.center());
    mapper.pan(model.view.pan_offset);

    for (int tick = 0; tick <= 5; ++tick) {
        const double value = y_max * static_cast<double>(tick) / 5.0;
        const QPointF point = mapper.to_pixel(x_min, value);
        if (model.show_grid) {
            painter.setPen(QPen(theme_colors(model).grid, 1.0));
            painter.drawLine(QPointF(plot.left(), point.y()), QPointF(plot.right(), point.y()));
        }
        painter.setPen(theme_colors(model).muted_text);
        painter.drawText(QRectF(area.left(), point.y() - 9.0, 52.0, 18.0),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(value, 'g', 5));
    }
    painter.setPen(QPen(theme_colors(model).axis, 1.2));
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());

    for (std::size_t index = 0; index < model.histogram_counts.size(); ++index) {
        const double left = model.histogram_edges[index];
        const double right = model.histogram_edges[index + 1];
        const QPointF top_left = mapper.to_pixel(left, model.histogram_counts[index]);
        const QPointF bottom_right = mapper.to_pixel(right, 0.0);
        const bool selected = std::find(model.view.selected_points.cbegin(),
                                        model.view.selected_points.cend(), index)
            != model.view.selected_points.cend();
        const bool hovered = model.view.hovered_point.has_value()
            && *model.view.hovered_point == index;
        painter.fillRect(QRectF(top_left, bottom_right),
                         selected ? QColor("#ffcc80") : QColor("#90caf9"));
        painter.setPen(selected || hovered ? QColor("#0d47a1") : QColor("#1565c0"));
        painter.setPen(QPen(painter.pen().color(), selected || hovered ? 2.0 : 1.0));
        painter.drawRect(QRectF(top_left, bottom_right));
    }

    const auto draw_spec = [&](std::optional<double> value, const QColor& color, const QString& label) {
        if (!value.has_value()) {
            return;
        }
        painter.setPen(QPen(color, 1.4, Qt::DashLine));
        const QPointF top = mapper.to_pixel(*value, y_max);
        const QPointF bottom = mapper.to_pixel(*value, 0.0);
        painter.drawLine(top, bottom);
        painter.drawText(top + QPointF(3.0, 12.0), label);
    };
    draw_spec(model.lsl, QColor("#d32f2f"), QStringLiteral("LSL"));
    draw_spec(model.usl, QColor("#d32f2f"), QStringLiteral("USL"));
    draw_spec(model.target, QColor("#6a1b9a"), QStringLiteral("Target"));

    if (model.process_mean.has_value() && model.within_sigma.has_value() && *model.within_sigma > 0.0) {
        painter.setPen(QPen(QColor("#455a64"), 1.2, Qt::DashLine));
        QPainterPath within;
        for (int step = 0; step <= 80; ++step) {
            const double x = x_min + (x_max - x_min) * step / 80.0;
            const double density = datalab::domain::statistics::normal_pdf(
                x, *model.process_mean, *model.within_sigma);
            const double y = density * (x_max - x_min) / static_cast<double>(model.histogram_counts.size())
                * static_cast<double>(model.values.size());
            const QPointF point = mapper.to_pixel(x, y);
            if (step == 0) {
                within.moveTo(point);
            } else {
                within.lineTo(point);
            }
        }
        painter.drawPath(within);
    }
    if (model.process_mean.has_value() && model.overall_sigma.has_value() && *model.overall_sigma > 0.0) {
        painter.setPen(QPen(QColor("#c62828"), 1.4));
        QPainterPath overall;
        for (int step = 0; step <= 80; ++step) {
            const double x = x_min + (x_max - x_min) * step / 80.0;
            const double density = datalab::domain::statistics::normal_pdf(
                x, *model.process_mean, *model.overall_sigma);
            const double y = density * (x_max - x_min) / static_cast<double>(model.histogram_counts.size())
                * static_cast<double>(model.values.size());
            const QPointF point = mapper.to_pixel(x, y);
            if (step == 0) {
                overall.moveTo(point);
            } else {
                overall.lineTo(point);
            }
        }
        painter.drawPath(overall);
    }
    draw_title_and_axes(painter, area, plot, model);
}

void render_boxplot(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = plot_rect(area);
    const std::size_t box_count = std::min({
        model.box_min.size(), model.box_q1.size(), model.box_median.size(),
        model.box_q3.size(), model.box_max.size()});
    if (box_count == 0) {
        painter.setPen(theme_colors(model).muted_text);
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    QVector<double> all_values;
    for (const auto& series : {model.box_min, model.box_max, model.box_q1, model.box_q3}) {
        for (const double value : series) {
            all_values.append(value);
        }
    }
    for (const ChartSeries& series : model.series) {
        for (const double value : series.values) {
            all_values.append(value);
        }
    }
    auto [minimum_it, maximum_it] = std::minmax_element(all_values.cbegin(), all_values.cend());
    double minimum = *minimum_it;
    double maximum = *maximum_it;
    const double padding = std::max(0.08 * (maximum - minimum), 1.0e-6);
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(-0.5, static_cast<double>(box_count) - 0.5,
                          minimum - padding, maximum + padding);
    painter.setPen(QPen(theme_colors(model).axis, 1.2));
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());

    const double box_width = std::min(40.0, plot.width() / static_cast<double>(box_count) * 0.4);
    for (std::size_t index = 0; index < box_count; ++index) {
        const double x = static_cast<double>(index);
        const QPointF max_p = mapper.to_pixel(x, model.box_max[index]);
        const QPointF min_p = mapper.to_pixel(x, model.box_min[index]);
        const QPointF q3 = mapper.to_pixel(x, model.box_q3[index]);
        const QPointF q1 = mapper.to_pixel(x, model.box_q1[index]);
        const QPointF med = mapper.to_pixel(x, model.box_median[index]);
        const bool selected = std::find(model.view.selected_points.cbegin(),
                                        model.view.selected_points.cend(), index)
            != model.view.selected_points.cend();
        const bool hovered = model.view.hovered_point.has_value()
            && *model.view.hovered_point == index;
        const QColor box_color = selected ? QColor("#ffcc80") : QColor("#bbdefb");
        painter.setPen(QPen(selected || hovered ? QColor("#0d47a1")
                                                : QColor("#1565c0"),
                            selected || hovered ? 2.0 : 1.2));
        painter.drawLine(max_p, q3);
        painter.drawLine(min_p, q1);
        painter.drawLine(QPointF(max_p.x() - 8.0, max_p.y()), QPointF(max_p.x() + 8.0, max_p.y()));
        painter.drawLine(QPointF(min_p.x() - 8.0, min_p.y()), QPointF(min_p.x() + 8.0, min_p.y()));
        painter.fillRect(QRectF(QPointF(q3.x() - box_width / 2.0, q3.y()),
                                QPointF(q1.x() + box_width / 2.0, q1.y())),
                         box_color);
        painter.drawRect(QRectF(QPointF(q3.x() - box_width / 2.0, q3.y()),
                                QPointF(q1.x() + box_width / 2.0, q1.y())));
        painter.setPen(QPen(QColor("#0d47a1"), 2.0));
        painter.drawLine(QPointF(med.x() - box_width / 2.0, med.y()),
                         QPointF(med.x() + box_width / 2.0, med.y()));
        if (hovered) {
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(QRectF(QPointF(q3.x() - box_width / 2.0 - 4.0, q3.y() - 4.0),
                                    QPointF(q1.x() + box_width / 2.0 + 4.0, q1.y() + 4.0)));
        }
        if (index < static_cast<std::size_t>(model.box_labels.size())) {
            painter.setPen(QColor("#455a64"));
            painter.drawText(QRectF(min_p.x() - 40.0, plot.bottom() + 4.0, 80.0, 18.0),
                             Qt::AlignCenter, model.box_labels[index]);
        }
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#c62828"));
    for (const ChartSeries& series : model.series) {
        const std::size_t count = std::min(series.x_values.size(), series.values.size());
        for (std::size_t index = 0; index < count; ++index) {
            painter.drawEllipse(mapper.to_pixel(series.x_values[index], series.values[index]),
                                3.0, 3.0);
        }
    }
    draw_title_and_axes(painter, area, plot, model);
}

void render_pareto(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = pareto_plot_rect(area);
    if (model.category_values.empty() || plot.width() <= 1.0 || plot.height() <= 1.0) {
        painter.setPen(theme_colors(model).muted_text);
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    const double maximum_count =
        *std::max_element(model.category_values.begin(), model.category_values.end());
    const double total_count = std::accumulate(
        model.category_values.begin(), model.category_values.end(), 0.0);
    // Minitab-style dual axis: left scale tops out at total count so the first
    // cumulative point sits on the first bar top (count/total == first cum%).
    const double scale_top = std::max({1.0, maximum_count, total_count});
    const double tick_step = std::max(1.0, std::ceil(scale_top / 5.0));
    const double y_max = tick_step * 5.0;
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(-0.5, static_cast<double>(model.category_values.size()) - 0.5, 0.0, y_max);

    painter.setPen(theme_colors(model).text);
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 11, QFont::Bold));
    painter.drawText(QRectF(area.left(), area.top(), area.width(), 26.0),
                     Qt::AlignCenter, model.title);
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));

    for (int tick = 0; tick <= 5; ++tick) {
        const double value = tick_step * static_cast<double>(tick);
        const QPointF point = mapper.to_pixel(-0.5, value);
        if (model.show_grid) {
            painter.setPen(QPen(theme_colors(model).grid, 1.0));
            painter.drawLine(QPointF(plot.left(), point.y()), QPointF(plot.right(), point.y()));
        }
        painter.setPen(theme_colors(model).muted_text);
        painter.drawText(QRectF(area.left() + 18.0, point.y() - 10.0, 42.0, 20.0),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(static_cast<qint64>(std::llround(value))));
    }

    painter.setPen(QPen(theme_colors(model).axis, 1.2));
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.drawLine(plot.bottomRight(), plot.topRight());
    painter.setPen(theme_colors(model).text);
    painter.save();
    painter.translate(area.left() + 12.0, plot.center().y());
    painter.rotate(-90.0);
    painter.drawText(QRectF(-plot.height() / 2.0, -10.0, plot.height(), 20.0),
                     Qt::AlignCenter, QStringLiteral("计数"));
    painter.restore();
    painter.save();
    painter.translate(area.right() - 12.0, plot.center().y());
    painter.rotate(90.0);
    painter.drawText(QRectF(-plot.height() / 2.0, -10.0, plot.height(), 20.0),
                     Qt::AlignCenter, QStringLiteral("百分比"));
    painter.restore();

    const double bar_width = std::min(36.0, plot.width() / static_cast<double>(model.category_values.size()) * 0.6);
    for (std::size_t index = 0; index < model.category_values.size(); ++index) {
        const QPointF top = mapper.to_pixel(static_cast<double>(index), model.category_values[index]);
        const QPointF bottom = mapper.to_pixel(static_cast<double>(index), 0.0);
        painter.fillRect(QRectF(QPointF(top.x() - bar_width / 2.0, top.y()),
                                QPointF(bottom.x() + bar_width / 2.0, bottom.y())),
                         QColor("#64b5f6"));
        painter.setPen(QColor("#1565c0"));
        painter.drawRect(QRectF(QPointF(top.x() - bar_width / 2.0, top.y()),
                                QPointF(bottom.x() + bar_width / 2.0, bottom.y())));
        if (index < static_cast<std::size_t>(model.categories.size())) {
            painter.setPen(QColor("#455a64"));
            painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));
            painter.save();
            // Anchor under the bar; rotate so text runs down-right and stays in the label band.
            painter.translate(bottom.x() + 4.0, plot.bottom() + 4.0);
            painter.rotate(45.0);
            painter.drawText(QRectF(0.0, -8.0, 96.0, 16.0),
                             Qt::AlignLeft | Qt::AlignVCenter, model.categories[index]);
            painter.restore();
        }
    }

    if (!model.cumulative_percent.empty()) {
        // Minitab places cum-% markers on the right edge of each bar, not the center.
        const auto cumulative_point = [&](std::size_t index) -> QPointF {
            const QPointF center = mapper.to_pixel(
                static_cast<double>(index),
                model.cumulative_percent[index] / 100.0 * y_max);
            return QPointF(center.x() + bar_width * 0.5, center.y());
        };
        painter.setPen(QPen(QColor("#c62828"), 1.8));
        painter.setBrush(QColor("#c62828"));
        for (std::size_t index = 0; index < model.cumulative_percent.size(); ++index) {
            const QPointF point = cumulative_point(index);
            if (index > 0) {
                painter.drawLine(cumulative_point(index - 1), point);
            }
            painter.drawEllipse(point, 3.5, 3.5);
        }
    }

    painter.setPen(QColor("#c62828"));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));
    for (int tick = 0; tick <= 100; tick += 20) {
        const QPointF point = mapper.to_pixel(
            static_cast<double>(model.category_values.size()) - 0.5,
            static_cast<double>(tick) / 100.0 * y_max);
        painter.drawText(
            QRectF(plot.right() + 6.0, point.y() - 8.0, 42.0, 16.0),
            Qt::AlignLeft | Qt::AlignVCenter,
            QStringLiteral("%1%").arg(tick));
    }

    // Stats rows sit below the slanted-label band to avoid collisions.
    constexpr double kLabelBand = 78.0;
    constexpr double kStatsTop = kLabelBand + 8.0;
    constexpr double kRowHeight = 16.0;
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));
    const QStringList row_labels = {
        QStringLiteral("Count"),
        QStringLiteral("百分比"),
        QStringLiteral("累积 %")};
    for (int row = 0; row < row_labels.size(); ++row) {
        painter.setPen(theme_colors(model).text);
        painter.drawText(
            QRectF(area.left() + 2.0, plot.bottom() + kStatsTop + row * kRowHeight, 58.0, kRowHeight),
            Qt::AlignRight | Qt::AlignVCenter, row_labels[row]);
    }
    for (std::size_t index = 0; index < model.category_values.size(); ++index) {
        const QPointF point = mapper.to_pixel(static_cast<double>(index), 0.0);
        const double percent = total_count > 0.0
            ? 100.0 * model.category_values[index] / total_count
            : 0.0;
        const double cumulative = index < model.cumulative_percent.size()
            ? model.cumulative_percent[index] : 0.0;
        painter.setPen(theme_colors(model).text);
        painter.drawText(QRectF(point.x() - 30.0, plot.bottom() + kStatsTop, 60.0, kRowHeight),
                         Qt::AlignCenter,
                         QString::number(model.category_values[index], 'f', 0));
        painter.drawText(QRectF(point.x() - 30.0, plot.bottom() + kStatsTop + kRowHeight, 60.0, kRowHeight),
                         Qt::AlignCenter, QString::number(percent, 'f', 1));
        painter.drawText(QRectF(point.x() - 30.0, plot.bottom() + kStatsTop + 2.0 * kRowHeight, 60.0, kRowHeight),
                         Qt::AlignCenter, QString::number(cumulative, 'f', 1));
    }
}

void render_interval(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = plot_rect(area);
    const std::size_t count = std::min({
        model.values.size(), model.interval_lower.size(), model.interval_upper.size()});
    if (count == 0) {
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    double minimum = *std::min_element(model.interval_lower.cbegin(),
                                       model.interval_lower.cbegin() + count);
    double maximum = *std::max_element(model.interval_upper.cbegin(),
                                       model.interval_upper.cbegin() + count);
    if (qFuzzyCompare(minimum, maximum)) {
        minimum -= 1.0;
        maximum += 1.0;
    }
    const double padding = (maximum - minimum) * 0.08;
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(-0.5, static_cast<double>(count) - 0.5,
                          minimum - padding, maximum + padding);
    painter.setPen(QPen(theme_colors(model).grid, 1.0));
    if (model.show_grid) {
        for (int tick = 0; tick <= 5; ++tick) {
            const double value = minimum - padding
                + (maximum - minimum + 2.0 * padding) * tick / 5.0;
            const double y = mapper.to_pixel(0.0, value).y();
            painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
            painter.setPen(theme_colors(model).muted_text);
            painter.drawText(QRectF(area.left(), y - 9.0, 52.0, 18.0),
                             Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(value, 'g', 5));
            painter.setPen(QPen(theme_colors(model).grid, 1.0));
        }
    }
    painter.setPen(QPen(theme_colors(model).axis, 1.2));
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.setBrush(QColor("#1565c0"));
    for (std::size_t index = 0; index < count; ++index) {
        const QPointF point = mapper.to_pixel(static_cast<double>(index), model.values[index]);
        const QPointF lower = mapper.to_pixel(
            static_cast<double>(index), model.interval_lower[index]);
        const QPointF upper = mapper.to_pixel(
            static_cast<double>(index), model.interval_upper[index]);
        painter.setPen(QPen(QColor("#455a64"), 1.4));
        painter.drawLine(lower, upper);
        painter.drawLine(lower + QPointF(-5.0, 0.0), lower + QPointF(5.0, 0.0));
        painter.drawLine(upper + QPointF(-5.0, 0.0), upper + QPointF(5.0, 0.0));
        painter.setPen(Qt::NoPen);
        const bool selected = std::find(model.view.selected_points.cbegin(),
                                        model.view.selected_points.cend(), index)
            != model.view.selected_points.cend();
        const bool hovered = model.view.hovered_point.has_value()
            && *model.view.hovered_point == index;
        painter.setBrush(selected ? QColor("#ff9800") : QColor("#1565c0"));
        painter.drawEllipse(point, selected ? 6.0 : 4.0, selected ? 6.0 : 4.0);
        if (hovered) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor("#0d47a1"), 2.0));
            painter.drawEllipse(point, 9.0, 9.0);
        }
        if (index < static_cast<std::size_t>(model.categories.size())) {
            painter.setPen(QColor("#455a64"));
            painter.drawText(QRectF(point.x() - 45.0, plot.bottom() + 4.0, 90.0, 18.0),
                             Qt::AlignCenter, model.categories[index]);
        }
    }
    draw_title_and_axes(painter, area, plot, model);
}

void render_bubble(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = plot_rect(area);
    const std::size_t count = std::min({
        model.x_values.size(), model.values.size(), model.bubble_sizes.size()});
    if (count == 0) {
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    const auto x_range = std::minmax_element(model.x_values.cbegin(),
                                             model.x_values.cbegin() + count);
    const auto y_range = std::minmax_element(model.values.cbegin(),
                                             model.values.cbegin() + count);
    double x_min = *x_range.first;
    double x_max = *x_range.second;
    double y_min = *y_range.first;
    double y_max = *y_range.second;
    if (qFuzzyCompare(x_min, x_max)) { x_min -= 1.0; x_max += 1.0; }
    if (qFuzzyCompare(y_min, y_max)) { y_min -= 1.0; y_max += 1.0; }
    const double x_padding = (x_max - x_min) * 0.05;
    const double y_padding = (y_max - y_min) * 0.05;
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(x_min - x_padding, x_max + x_padding,
                          y_min - y_padding, y_max + y_padding);
    painter.setPen(QPen(theme_colors(model).axis, 1.2));
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    const auto size_range = std::minmax_element(
        model.bubble_sizes.cbegin(), model.bubble_sizes.cbegin() + count);
    const double size_min = *size_range.first;
    const double size_max = *size_range.second;
    const auto group_color = [](const QString& group) {
        const uint hash = qHash(group);
        return QColor::fromHsv(static_cast<int>(hash % 360U), 150, 210, 180);
    };
    for (std::size_t index = 0; index < count; ++index) {
        const double fraction = size_max > size_min
            ? (model.bubble_sizes[index] - size_min) / (size_max - size_min) : 0.5;
        const double radius = 4.0 + std::sqrt(std::clamp(fraction, 0.0, 1.0)) * 18.0;
        const QPointF point = mapper.to_pixel(model.x_values[index], model.values[index]);
        const QString group = index < static_cast<std::size_t>(model.point_groups.size())
            ? model.point_groups[index] : QStringLiteral("全部");
        painter.setPen(QPen(group_color(group).darker(130), 1.0));
        painter.setBrush(group_color(group));
        painter.drawEllipse(point, radius, radius);
        if (index < static_cast<std::size_t>(model.point_labels.size())
            && !model.point_labels[index].isEmpty()) {
            painter.setPen(theme_colors(model).text);
            painter.drawText(point + QPointF(radius + 2.0, 0.0), model.point_labels[index]);
        }
    }
    draw_title_and_axes(painter, area, plot, model);
}

void render_correlation(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const std::size_t count = model.matrix_values.size();
    if (count == 0 || model.matrix_labels.size() < count) {
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    const double cell = std::min(82.0, std::max(42.0,
        std::min(area.width(), area.height()) / static_cast<double>(count + 1)));
    const QPointF origin(area.left() + 120.0, area.top() + 52.0);
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));
    for (std::size_t row = 0; row < count; ++row) {
        painter.setPen(theme_colors(model).text);
        painter.drawText(QRectF(origin.x() - 116.0,
                                 origin.y() + static_cast<double>(row) * cell,
                                 110.0, cell),
                         Qt::AlignRight | Qt::AlignVCenter, model.matrix_labels[row]);
        painter.drawText(QRectF(origin.x() + static_cast<double>(row) * cell,
                                 origin.y() - 38.0, cell, 34.0),
                         Qt::AlignCenter, model.matrix_labels[row]);
        for (std::size_t column = 0; column < count; ++column) {
            const double value = row < model.matrix_values.size()
                && column < model.matrix_values[row].size()
                ? model.matrix_values[row][column] : 0.0;
            const double intensity = std::clamp(std::abs(value), 0.0, 1.0);
            const QColor color = value >= 0.0
                ? QColor(33, 113, 181, static_cast<int>(45.0 + intensity * 190.0))
                : QColor(203, 50, 52, static_cast<int>(45.0 + intensity * 190.0));
            const QRectF cell_rect(origin.x() + static_cast<double>(column) * cell,
                                   origin.y() + static_cast<double>(row) * cell,
                                   cell - 2.0, cell - 2.0);
            painter.fillRect(cell_rect, color);
            painter.setPen(theme_colors(model).text);
            painter.drawText(cell_rect, Qt::AlignCenter,
                             QString::number(value, 'f', 2));
        }
    }
    painter.setPen(theme_colors(model).text);
    painter.drawText(QRectF(area.left(), area.top() + 8.0, area.width(), 24.0),
                     Qt::AlignCenter, model.title);
}

QColor scale_color(double value, double minimum, double maximum)
{
    const double span = std::max(1.0e-12, maximum - minimum);
    const double t = std::clamp((value - minimum) / span, 0.0, 1.0);
    return QColor::fromHsv(static_cast<int>(std::lround(220.0 - t * 220.0)), 170, 230);
}

QColor group_color(const QString& group)
{
    const uint hash = qHash(group.isEmpty() ? QStringLiteral("全部") : group);
    return QColor::fromHsv(static_cast<int>(hash % 360U), 150, 210, 200);
}

void render_ecdf(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = plot_rect(area);
    if (model.x_values.empty() || model.values.empty()) {
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    const std::size_t count = std::min(model.x_values.size(), model.values.size());
    double x_min = *std::min_element(model.x_values.cbegin(), model.x_values.cbegin() + count);
    double x_max = *std::max_element(model.x_values.cbegin(), model.x_values.cbegin() + count);
    if (qFuzzyCompare(x_min, x_max)) {
        x_min -= 1.0;
        x_max += 1.0;
    }
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(x_min, x_max, 0.0, 1.05);
    painter.setPen(QPen(theme_colors(model).axis, 1.2));
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.setPen(QPen(QColor(model.value_style.color), model.value_style.line_width));
    QPainterPath path;
    path.moveTo(mapper.to_pixel(model.x_values.front(), 0.0));
    double previous = 0.0;
    for (std::size_t index = 0; index < count; ++index) {
        const QPointF low = mapper.to_pixel(model.x_values[index], previous);
        const QPointF high = mapper.to_pixel(model.x_values[index], model.values[index]);
        path.lineTo(low);
        path.lineTo(high);
        previous = model.values[index];
    }
    path.lineTo(mapper.to_pixel(x_max, previous));
    painter.drawPath(path);
    draw_title_and_axes(painter, area, plot, model);
}

void render_matrix(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const std::size_t count = model.matrix_labels.size();
    if (count == 0 || model.matrix_values.size() < count) {
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    const double cell = std::min(area.width(), area.height()) / static_cast<double>(count + 0.4);
    const QPointF origin(area.left() + 36.0, area.top() + 40.0);
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));
    for (std::size_t row = 0; row < count; ++row) {
        for (std::size_t column = 0; column < count; ++column) {
            const QRectF cell_rect(origin.x() + static_cast<double>(column) * cell,
                                   origin.y() + static_cast<double>(row) * cell,
                                   cell - 4.0, cell - 4.0);
            painter.fillRect(cell_rect, QColor("#f7fafc"));
            painter.setPen(QColor("#90a4ae"));
            painter.drawRect(cell_rect);
            if (row == column) {
                painter.setPen(theme_colors(model).text);
                painter.drawText(cell_rect, Qt::AlignCenter, model.matrix_labels[row]);
                continue;
            }
            const auto& xs = model.matrix_values[column];
            const auto& ys = model.matrix_values[row];
            const std::size_t n = std::min(xs.size(), ys.size());
            if (n == 0) {
                continue;
            }
            const auto x_range = std::minmax_element(xs.cbegin(), xs.cend());
            const auto y_range = std::minmax_element(ys.cbegin(), ys.cend());
            ChartCoordinateMapper mapper(cell_rect.adjusted(4.0, 4.0, -4.0, -4.0));
            mapper.set_data_range(*x_range.first, *x_range.second == *x_range.first
                                      ? *x_range.first + 1.0 : *x_range.second,
                                  *y_range.first, *y_range.second == *y_range.first
                                      ? *y_range.first + 1.0 : *y_range.second);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#1565c0"));
            for (std::size_t index = 0; index < n; ++index) {
                painter.drawEllipse(mapper.to_pixel(xs[index], ys[index]), 1.8, 1.8);
            }
        }
    }
    painter.setPen(theme_colors(model).text);
    painter.drawText(QRectF(area.left(), area.top() + 6.0, area.width(), 24.0),
                     Qt::AlignCenter, model.title);
}

void render_marginal(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF scatter_area = area.adjusted(58.0, 72.0, -78.0, -48.0);
    const QRectF x_hist(scatter_area.left(), area.top() + 36.0,
                        scatter_area.width(), scatter_area.top() - area.top() - 40.0);
    const QRectF y_hist(scatter_area.right() + 8.0, scatter_area.top(),
                        58.0, scatter_area.height());
    ChartModel scatter = model;
    scatter.kind = ChartKind::Scatter;
    scatter.title.clear();
    scatter.subtitle.clear();
    render_scatter(painter, scatter_area.adjusted(-58.0, -42.0, 96.0, 48.0), scatter);
    auto draw_hist = [&](const QRectF& rect, const std::vector<double>& edges,
                         const std::vector<double>& counts, bool vertical) {
        if (edges.size() < 2 || counts.empty()) {
            return;
        }
        const double maximum = std::max(1.0, *std::max_element(counts.cbegin(), counts.cend()));
        for (std::size_t index = 0; index < counts.size(); ++index) {
            const double start = (edges[index] - edges.front())
                / std::max(1.0e-12, edges.back() - edges.front());
            const double end = (edges[index + 1] - edges.front())
                / std::max(1.0e-12, edges.back() - edges.front());
            const double height = counts[index] / maximum;
            QRectF bar;
            if (vertical) {
                bar = QRectF(rect.left(),
                             rect.bottom() - end * rect.height(),
                             height * rect.width(),
                             (end - start) * rect.height());
            } else {
                bar = QRectF(rect.left() + start * rect.width(),
                             rect.bottom() - height * rect.height(),
                             (end - start) * rect.width(),
                             height * rect.height());
            }
            painter.fillRect(bar, QColor("#90caf9"));
            painter.setPen(QColor("#1565c0"));
            painter.drawRect(bar);
        }
    };
    draw_hist(x_hist, model.histogram_edges, model.histogram_counts, false);
    draw_hist(y_hist, model.histogram_edges_y, model.histogram_counts_y, true);
    painter.setPen(theme_colors(model).text);
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 11, QFont::Bold));
    painter.drawText(QRectF(area.left(), area.top(), area.width(), 26.0),
                     Qt::AlignCenter, model.title);
}

void render_parallel(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = plot_rect(area);
    const std::size_t axes = model.matrix_labels.size();
    if (axes < 2 || model.matrix_values.empty()) {
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    painter.setPen(QPen(theme_colors(model).axis, 1.2));
    for (std::size_t axis = 0; axis < axes; ++axis) {
        const double x = plot.left()
            + plot.width() * static_cast<double>(axis) / static_cast<double>(axes - 1);
        painter.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        painter.drawText(QRectF(x - 40.0, plot.bottom() + 4.0, 80.0, 18.0),
                         Qt::AlignCenter, model.matrix_labels[axis]);
    }
    for (std::size_t row = 0; row < model.matrix_values.size(); ++row) {
        const QString group = row < static_cast<std::size_t>(model.point_groups.size())
            ? model.point_groups[row] : QStringLiteral("全部");
        QColor color = group_color(group);
        const bool selected = std::find(model.view.selected_points.cbegin(),
                                        model.view.selected_points.cend(), row)
            != model.view.selected_points.cend();
        const bool hovered = model.view.hovered_point.has_value()
            && *model.view.hovered_point == row;
        color.setAlpha(selected || hovered ? 230 : 140);
        painter.setPen(QPen(color, selected || hovered ? 3.0 : 1.2));
        QPainterPath path;
        bool started = false;
        for (std::size_t axis = 0; axis < axes && axis < model.matrix_values[row].size(); ++axis) {
            const double value = model.matrix_values[row][axis];
            if (!std::isfinite(value)) {
                started = false;
                continue;
            }
            const double minimum = axis < model.lower.size() ? model.lower[axis] : 0.0;
            const double maximum = axis < model.upper.size() ? model.upper[axis] : 1.0;
            const double t = (value - minimum) / std::max(1.0e-12, maximum - minimum);
            const double x = plot.left()
                + plot.width() * static_cast<double>(axis) / static_cast<double>(axes - 1);
            const QPointF point(x, plot.bottom() - std::clamp(t, 0.0, 1.0) * plot.height());
            if (!started) {
                path.moveTo(point);
                started = true;
            } else {
                path.lineTo(point);
            }
        }
        painter.drawPath(path);
    }
    draw_title_and_axes(painter, area, plot, model);
}

void render_heatmap(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const std::vector<QString>& columns = model.matrix_labels;
    const std::vector<QString>& rows = model.categories.empty() ? model.matrix_labels : model.categories;
    if (columns.empty() || rows.empty() || model.matrix_values.empty()) {
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    const double minimum = model.color_min.value_or(-1.0);
    const double maximum = model.color_max.value_or(1.0);
    const double cell_w = std::min(72.0, (area.width() - 140.0) / static_cast<double>(columns.size()));
    const double cell_h = std::min(48.0, (area.height() - 90.0) / static_cast<double>(rows.size()));
    const QPointF origin(area.left() + 110.0, area.top() + 52.0);
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));
    for (std::size_t row = 0; row < rows.size(); ++row) {
        painter.setPen(theme_colors(model).text);
        painter.drawText(QRectF(origin.x() - 104.0, origin.y() + static_cast<double>(row) * cell_h,
                                100.0, cell_h),
                         Qt::AlignRight | Qt::AlignVCenter, rows[row]);
        for (std::size_t column = 0; column < columns.size(); ++column) {
            const double value = row < model.matrix_values.size()
                && column < model.matrix_values[row].size()
                ? model.matrix_values[row][column] : 0.0;
            const QRectF cell_rect(origin.x() + static_cast<double>(column) * cell_w,
                                   origin.y() + static_cast<double>(row) * cell_h,
                                   cell_w - 2.0, cell_h - 2.0);
            painter.fillRect(cell_rect, scale_color(value, minimum, maximum));
            painter.setPen(theme_colors(model).text);
            painter.drawText(cell_rect, Qt::AlignCenter, QString::number(value, 'f', 2));
            if (row == 0) {
                painter.drawText(QRectF(cell_rect.left(), origin.y() - 34.0, cell_w, 30.0),
                                 Qt::AlignCenter, columns[column]);
            }
        }
    }
    painter.setPen(theme_colors(model).text);
    painter.drawText(QRectF(area.left(), area.top() + 8.0, area.width(), 24.0),
                     Qt::AlignCenter, model.title);
}

void render_time_series(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    ChartModel line = model;
    line.kind = ChartKind::Scatter;
    line.value_style.point_style = ChartPointStyle::Circle;
    render_scatter(painter, area, line);
    if (model.x_values.size() < 2 || model.values.size() < 2) {
        return;
    }
    const QRectF plot = plot_rect(area);
    const std::size_t count = std::min(model.x_values.size(), model.values.size());
    auto x_range = std::minmax_element(model.x_values.cbegin(), model.x_values.cbegin() + count);
    auto y_range = std::minmax_element(model.values.cbegin(), model.values.cbegin() + count);
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(*x_range.first, *x_range.second, *y_range.first, *y_range.second);
    mapper.zoom(model.view.zoom_factor, plot.center());
    mapper.pan(model.view.pan_offset);
    if (model.value_style.line_width > 0.0) {
        painter.setPen(QPen(QColor(model.value_style.color), model.value_style.line_width));
        for (std::size_t index = 1; index < count; ++index) {
            painter.drawLine(mapper.to_pixel(model.x_values[index - 1], model.values[index - 1]),
                             mapper.to_pixel(model.x_values[index], model.values[index]));
        }
    }
}

void render_area(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = plot_rect(area);
    const std::size_t count = std::min(model.x_values.size(), model.values.size());
    if (count < 2) {
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    auto x_range = std::minmax_element(model.x_values.cbegin(), model.x_values.cbegin() + count);
    auto y_range = std::minmax_element(model.values.cbegin(), model.values.cbegin() + count);
    const double y_min = std::min(0.0, *y_range.first);
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(*x_range.first, *x_range.second, y_min, *y_range.second);
    QPainterPath path;
    path.moveTo(mapper.to_pixel(model.x_values.front(), y_min));
    for (std::size_t index = 0; index < count; ++index) {
        path.lineTo(mapper.to_pixel(model.x_values[index], model.values[index]));
    }
    path.lineTo(mapper.to_pixel(model.x_values.back(), y_min));
    path.closeSubpath();
    QColor fill(model.value_style.color);
    fill.setAlpha(90);
    painter.fillPath(path, fill);
    render_time_series(painter, area, model);
}

void render_contour(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = plot_rect(area);
    if (model.contour_x.size() < 2 || model.contour_y.size() < 2
        || model.matrix_values.size() < model.contour_y.size()) {
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    const double x_min = model.contour_x.front();
    const double x_max = model.contour_x.back();
    const double y_min = model.contour_y.front();
    const double y_max = model.contour_y.back();
    const double c_min = model.color_min.value_or(0.0);
    const double c_max = model.color_max.value_or(1.0);
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(x_min, x_max, y_min, y_max);
    for (std::size_t row = 0; row + 1 < model.contour_y.size(); ++row) {
        for (std::size_t column = 0; column + 1 < model.contour_x.size(); ++column) {
            const double value = model.matrix_values[row][column];
            const QPointF top_left = mapper.to_pixel(model.contour_x[column], model.contour_y[row + 1]);
            const QPointF bottom_right = mapper.to_pixel(model.contour_x[column + 1], model.contour_y[row]);
            painter.fillRect(QRectF(top_left, bottom_right).normalized(),
                             scale_color(value, c_min, c_max));
        }
    }
    painter.setPen(QPen(theme_colors(model).axis, 1.2));
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    draw_title_and_axes(painter, area, plot, model);
}

void render_pie(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    if (model.category_values.empty()) {
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    const double total = std::accumulate(
        model.category_values.cbegin(), model.category_values.cend(), 0.0);
    const double size = std::min(area.width(), area.height()) * 0.55;
    const QRectF pie(area.center().x() - size / 2.0, area.center().y() - size / 2.2, size, size);
    double angle = 90.0 * 16.0;
    for (std::size_t index = 0; index < model.category_values.size(); ++index) {
        const double span = total <= 0.0
            ? 0.0
            : -model.category_values[index] / total * 360.0 * 16.0;
        painter.setBrush(group_color(index < static_cast<std::size_t>(model.categories.size())
            ? model.categories[index] : QString::number(static_cast<int>(index))));
        painter.setPen(QPen(QColor("#ffffff"), 1.2));
        painter.drawPie(pie, static_cast<int>(angle), static_cast<int>(span));
        angle += span;
    }
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));
    double legend_y = area.bottom() - 24.0 * static_cast<double>(model.categories.size()) - 12.0;
    for (std::size_t index = 0; index < model.categories.size(); ++index) {
        const QString percent = index < model.cumulative_percent.size()
            ? QString::number(model.cumulative_percent[index], 'f', 1) + QStringLiteral("%")
            : QString();
        painter.setBrush(group_color(model.categories[index]));
        painter.setPen(Qt::NoPen);
        painter.drawRect(QRectF(area.left() + 24.0, legend_y, 12.0, 12.0));
        painter.setPen(theme_colors(model).text);
        painter.drawText(QRectF(area.left() + 42.0, legend_y - 2.0, area.width() - 60.0, 18.0),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         model.categories[index] + QStringLiteral("  ") + percent);
        legend_y += 20.0;
    }
    painter.setPen(theme_colors(model).text);
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 11, QFont::Bold));
    painter.drawText(QRectF(area.left(), area.top() + 8.0, area.width(), 24.0),
                     Qt::AlignCenter, model.title);
}
}  // namespace

void ChartRenderer::render(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    painter.save();
    painter.fillRect(area, theme_colors(model).background);
    fill_data_region(painter, area, model);
    painter.setRenderHint(QPainter::Antialiasing, true);
    switch (model.kind) {
    case ChartKind::Histogram:
        render_histogram(painter, area, model);
        break;
    case ChartKind::BoxPlot:
        render_boxplot(painter, area, model);
        break;
    case ChartKind::Pareto:
        render_pareto(painter, area, model);
        break;
    case ChartKind::Probability:
        render_probability(painter, area, model);
        break;
    case ChartKind::Scatter:
        render_scatter(painter, area, model);
        break;
    case ChartKind::Interval:
        render_interval(painter, area, model);
        break;
    case ChartKind::Correlation:
        render_correlation(painter, area, model);
        break;
    case ChartKind::Bubble:
        render_bubble(painter, area, model);
        break;
    case ChartKind::Ecdf:
        render_ecdf(painter, area, model);
        break;
    case ChartKind::Matrix:
        render_matrix(painter, area, model);
        break;
    case ChartKind::Marginal:
        render_marginal(painter, area, model);
        break;
    case ChartKind::Parallel:
        render_parallel(painter, area, model);
        break;
    case ChartKind::Heatmap:
        render_heatmap(painter, area, model);
        break;
    case ChartKind::TimeSeries:
        render_time_series(painter, area, model);
        break;
    case ChartKind::Area:
        render_area(painter, area, model);
        break;
    case ChartKind::Contour:
        render_contour(painter, area, model);
        break;
    case ChartKind::Pie:
        render_pie(painter, area, model);
        break;
    case ChartKind::Control:
    default:
        render_control(painter, area, model);
        break;
    }
    painter.restore();
}

QPixmap ChartRenderer::render_to_pixmap(
    const ChartModel& model,
    const QSize& size,
    qreal device_pixel_ratio)
{
    const qreal safe_ratio = device_pixel_ratio > 0.0 ? device_pixel_ratio : 1.0;
    const int pixel_width = std::max(1, static_cast<int>(std::lround(size.width() * safe_ratio)));
    const int pixel_height = std::max(1, static_cast<int>(std::lround(size.height() * safe_ratio)));
    QPixmap pixmap(pixel_width, pixel_height);
    pixmap.setDevicePixelRatio(safe_ratio);
    pixmap.fill(theme_colors(model).background);
    QPainter painter(&pixmap);
    render(painter, QRectF(0.0, 0.0, size.width(), size.height()), model);
    return pixmap;
}
