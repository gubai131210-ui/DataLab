#include "chart_renderer.h"

#include "reporting/chart_coordinate_mapper.h"
#include "domain/statistics/normal_distribution.h"

#include <QPainter>
#include <QPainterPath>

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

QRectF plot_rect(const QRectF& area)
{
    return area.adjusted(58.0, 42.0, -96.0, -48.0);
}

QRectF pareto_plot_rect(const QRectF& area)
{
    // Extra bottom room: slanted category labels (~70px) + gap + 3 stats rows.
    return area.adjusted(64.0, 42.0, -88.0, -178.0);
}

void draw_title_and_axes(
    QPainter& painter,
    const QRectF& area,
    const QRectF& plot,
    const ChartModel& model)
{
    painter.setPen(QColor("#263238"));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 11, QFont::Bold));
    painter.drawText(QRectF(area.left(), area.top(), area.width(), 26.0),
                     Qt::AlignCenter, model.title);
    if (!model.subtitle.isEmpty()) {
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));
        painter.drawText(QRectF(area.left(), area.top() + 24.0, area.width(), 18.0),
                         Qt::AlignCenter, model.subtitle);
    }
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
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
        painter.setPen(QColor("#46515c"));
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
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(x_min, std::max(x_min + 1.0, x_max),
                          minimum, maximum);
    mapper.zoom(model.zoom_factor, plot.center());
    mapper.pan(model.pan_offset);

    painter.setPen(QPen(QColor("#e3e7eb"), 1.0));
    for (int tick = 0; tick <= 5; ++tick) {
        const double value = minimum + (maximum - minimum) * tick / 5.0;
        const QPointF point = mapper.to_pixel(x_min, value);
        if (model.show_grid) {
            painter.drawLine(QPointF(plot.left(), point.y()), QPointF(plot.right(), point.y()));
        }
        painter.setPen(QColor("#5d6872"));
        painter.drawText(QRectF(area.left(), point.y() - 10.0, 52.0, 20.0),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(value, 'g', 5));
        painter.setPen(QPen(QColor("#e3e7eb"), 1.0));
    }
    painter.setPen(QPen(QColor("#263238"), 1.2));
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
    const auto series_color = [](ChartSeriesRole role) {
        switch (role) {
        case ChartSeriesRole::Actual: return QColor("#1565c0");
        case ChartSeriesRole::Fitted: return QColor("#2e7d32");
        case ChartSeriesRole::Forecast: return QColor("#ef6c00");
        case ChartSeriesRole::InteractionFirst: return QColor("#1565c0");
        case ChartSeriesRole::InteractionSecond: return QColor("#c62828");
        case ChartSeriesRole::Trend: return QColor("#6a1b9a");
        case ChartSeriesRole::Seasonal: return QColor("#00838f");
        case ChartSeriesRole::Remainder: return QColor("#5d4037");
        case ChartSeriesRole::ConfidenceBand: return QColor("#90caf9");
        case ChartSeriesRole::Component:
        case ChartSeriesRole::Generic:
        default: return QColor("#455a64");
        }
    };
    const auto draw_extra_series = [&](const ChartSeries& series) {
        const std::size_t count = std::max({
            series.values.size(), series.lower.size(), series.upper.size()});
        if (count == 0) {
            return;
        }
        const QColor color = series_color(series.role);
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
        painter.setPen(QPen(color, series.line_width,
                            series.role == ChartSeriesRole::Forecast
                                ? Qt::DashLine : Qt::SolidLine));
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
                    painter.drawEllipse(
                        mapper.to_pixel(series_x_at(series, i), series.values[i]), 3.0, 3.0);
                }
            }
        }
    };
    draw_series(model.lower, QPen(QColor("#d32f2f"), 1.0, Qt::DashLine));
    draw_series(model.upper, QPen(QColor("#d32f2f"), 1.0, Qt::DashLine));
    draw_series(model.center, QPen(QColor("#2e7d32"), 1.2, Qt::DashLine));
    draw_series(model.values, QPen(QColor("#1565c0"), model.line_width));
    for (const ChartSeries& series : model.series) {
        draw_extra_series(series);
    }
    draw_right_label(model.upper, QStringLiteral("UCL"), QColor("#d32f2f"), Qt::DashLine);
    draw_right_label(model.center, model.center_label, QColor("#2e7d32"), Qt::DashLine);
    draw_right_label(model.lower, QStringLiteral("LCL"), QColor("#d32f2f"), Qt::DashLine);

    painter.setPen(Qt::NoPen);
    for (std::size_t i = 0; i < model.values.size(); ++i) {
        const bool selected = std::find(model.selected_points.cbegin(),
                                        model.selected_points.cend(), i)
            != model.selected_points.cend();
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
        painter.setBrush(selected ? QColor("#ff9800")
                                   : (special_cause ? QColor("#d32f2f") : QColor("#1565c0")));
        if (!std::isfinite(model.values[i])) {
            continue;
        }
        const QPointF point = mapper.to_pixel(x_at(i), model.values[i]);
        if (model.hovered_point.has_value() && *model.hovered_point == i) {
            painter.setPen(QPen(QColor("#0d47a1"), 2.0));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(point, 8.0, 8.0);
            painter.setBrush(selected ? QColor("#ff9800")
                                       : (special_cause ? QColor("#d32f2f") : QColor("#1565c0")));
        }
        painter.drawEllipse(point, 3.5, 3.5);
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
    painter.setPen(QColor("#1565c0"));
    painter.drawText(legend, Qt::AlignLeft, QStringLiteral("● 观测值   "));
    painter.setPen(QColor("#2e7d32"));
    painter.drawText(legend.adjusted(72.0, 0.0, 0.0, 0.0), Qt::AlignLeft, QStringLiteral("— 中心线"));
    painter.setPen(QColor("#d32f2f"));
    painter.drawText(legend.adjusted(148.0, 0.0, 0.0, 0.0), Qt::AlignLeft, QStringLiteral("·· 控制限"));
    double legend_y = legend.bottom() + 2.0;
    for (const ChartSeries& series : model.series) {
        if (series.label.isEmpty()) {
            continue;
        }
        painter.setPen(series_color(series.role));
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
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(x_min, x_max, y_min, y_max);
    mapper.zoom(model.zoom_factor, plot.center());
    mapper.pan(model.pan_offset);
    painter.setPen(QColor("#cfd8dc"));
    if (model.show_grid) {
        for (int tick = 0; tick <= 5; ++tick) {
            const double fraction = static_cast<double>(tick) / 5.0;
            const double x = plot.left() + fraction * plot.width();
            const double y = plot.bottom() - fraction * plot.height();
            painter.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
            painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        }
    }
    painter.setPen(QColor("#37474f"));
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
        painter.drawEllipse(point, 3.5, 3.5);
    }
    const auto series_color = [](ChartSeriesRole role) {
        switch (role) {
        case ChartSeriesRole::Actual: return QColor("#1565c0");
        case ChartSeriesRole::Fitted: return QColor("#2e7d32");
        case ChartSeriesRole::Forecast: return QColor("#ef6c00");
        case ChartSeriesRole::InteractionFirst: return QColor("#1565c0");
        case ChartSeriesRole::InteractionSecond: return QColor("#c62828");
        case ChartSeriesRole::Trend: return QColor("#6a1b9a");
        case ChartSeriesRole::Seasonal: return QColor("#00838f");
        case ChartSeriesRole::Remainder: return QColor("#5d4037");
        default: return QColor("#455a64");
        }
    };
    for (const ChartSeries& series : model.series) {
        const std::size_t count = std::max({
            series.values.size(), series.lower.size(), series.upper.size()});
        if (count == 0) {
            continue;
        }
        const auto x_at = [&](std::size_t index) {
            return index < series.x_values.size()
                ? series.x_values[index] : static_cast<double>(index);
        };
        const QColor color = series_color(series.role);
        painter.setPen(QPen(color, series.line_width,
                            series.role == ChartSeriesRole::Forecast
                                ? Qt::DashLine : Qt::SolidLine));
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
    painter.setPen(QColor("#263238"));
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
        painter.setPen(QColor("#46515c"));
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
    ChartCoordinateMapper mapper(plot);
    mapper.set_data_range(x_min, x_max, y_min, y_max);
    mapper.zoom(model.zoom_factor, plot.center());
    mapper.pan(model.pan_offset);
    painter.setPen(QPen(QColor("#e3e7eb"), 1.0));
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
        painter.setPen(QColor("#5d6872"));
        painter.drawText(QRectF(x_point.x() - 30.0, plot.bottom() + 2.0, 60.0, 18.0),
                         Qt::AlignCenter, QString::number(x, 'g', 3));
        painter.drawText(QRectF(area.left(), y_point.y() - 9.0, 52.0, 18.0),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(y, 'g', 5));
        painter.setPen(QPen(QColor("#e3e7eb"), 1.0));
    }
    painter.setPen(QPen(QColor("#263238"), 1.2));
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
    painter.setPen(QPen(QColor("#c62828"), 1.4));
    painter.drawLine(mapper.to_pixel(x_min, intercept + slope * x_min),
                     mapper.to_pixel(x_max, intercept + slope * x_max));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#1565c0"));
    for (std::size_t index = 0; index < model.values.size(); ++index) {
        painter.drawEllipse(mapper.to_pixel(model.x_values[index], model.values[index]), 3.5, 3.5);
    }
    draw_title_and_axes(painter, area, plot, model);
}

void render_histogram(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = plot_rect(area);
    if (model.histogram_counts.empty() || model.histogram_edges.size() < 2) {
        painter.setPen(QColor("#46515c"));
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
    mapper.zoom(model.zoom_factor, plot.center());
    mapper.pan(model.pan_offset);

    for (int tick = 0; tick <= 5; ++tick) {
        const double value = y_max * static_cast<double>(tick) / 5.0;
        const QPointF point = mapper.to_pixel(x_min, value);
        if (model.show_grid) {
            painter.setPen(QPen(QColor("#e3e7eb"), 1.0));
            painter.drawLine(QPointF(plot.left(), point.y()), QPointF(plot.right(), point.y()));
        }
        painter.setPen(QColor("#5d6872"));
        painter.drawText(QRectF(area.left(), point.y() - 9.0, 52.0, 18.0),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(value, 'g', 5));
    }
    painter.setPen(QPen(QColor("#263238"), 1.2));
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());

    for (std::size_t index = 0; index < model.histogram_counts.size(); ++index) {
        const double left = model.histogram_edges[index];
        const double right = model.histogram_edges[index + 1];
        const QPointF top_left = mapper.to_pixel(left, model.histogram_counts[index]);
        const QPointF bottom_right = mapper.to_pixel(right, 0.0);
        painter.fillRect(QRectF(top_left, bottom_right), QColor("#90caf9"));
        painter.setPen(QColor("#1565c0"));
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
        painter.setPen(QColor("#46515c"));
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("没有可显示的数据"));
        return;
    }
    QVector<double> all_values;
    for (const auto& series : {model.box_min, model.box_max}) {
        for (const double value : series) {
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
    painter.setPen(QPen(QColor("#263238"), 1.2));
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
        painter.setPen(QPen(QColor("#1565c0"), 1.2));
        painter.drawLine(max_p, q3);
        painter.drawLine(min_p, q1);
        painter.drawLine(QPointF(max_p.x() - 8.0, max_p.y()), QPointF(max_p.x() + 8.0, max_p.y()));
        painter.drawLine(QPointF(min_p.x() - 8.0, min_p.y()), QPointF(min_p.x() + 8.0, min_p.y()));
        painter.fillRect(QRectF(QPointF(q3.x() - box_width / 2.0, q3.y()),
                                QPointF(q1.x() + box_width / 2.0, q1.y())),
                         QColor("#bbdefb"));
        painter.drawRect(QRectF(QPointF(q3.x() - box_width / 2.0, q3.y()),
                                QPointF(q1.x() + box_width / 2.0, q1.y())));
        painter.setPen(QPen(QColor("#0d47a1"), 2.0));
        painter.drawLine(QPointF(med.x() - box_width / 2.0, med.y()),
                         QPointF(med.x() + box_width / 2.0, med.y()));
        if (index < static_cast<std::size_t>(model.box_labels.size())) {
            painter.setPen(QColor("#455a64"));
            painter.drawText(QRectF(min_p.x() - 40.0, plot.bottom() + 4.0, 80.0, 18.0),
                             Qt::AlignCenter, model.box_labels[index]);
        }
    }
    draw_title_and_axes(painter, area, plot, model);
}

void render_pareto(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    const QRectF plot = pareto_plot_rect(area);
    if (model.category_values.empty() || plot.width() <= 1.0 || plot.height() <= 1.0) {
        painter.setPen(QColor("#46515c"));
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

    painter.setPen(QColor("#263238"));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 11, QFont::Bold));
    painter.drawText(QRectF(area.left(), area.top(), area.width(), 26.0),
                     Qt::AlignCenter, model.title);
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));

    for (int tick = 0; tick <= 5; ++tick) {
        const double value = tick_step * static_cast<double>(tick);
        const QPointF point = mapper.to_pixel(-0.5, value);
        if (model.show_grid) {
            painter.setPen(QPen(QColor("#e3e7eb"), 1.0));
            painter.drawLine(QPointF(plot.left(), point.y()), QPointF(plot.right(), point.y()));
        }
        painter.setPen(QColor("#5d6872"));
        painter.drawText(QRectF(area.left() + 18.0, point.y() - 10.0, 42.0, 20.0),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(static_cast<qint64>(std::llround(value))));
    }

    painter.setPen(QPen(QColor("#263238"), 1.2));
    painter.drawLine(plot.bottomLeft(), plot.topLeft());
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.drawLine(plot.bottomRight(), plot.topRight());
    painter.setPen(QColor("#263238"));
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
        painter.setPen(QColor("#263238"));
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
        painter.setPen(QColor("#263238"));
        painter.drawText(QRectF(point.x() - 30.0, plot.bottom() + kStatsTop, 60.0, kRowHeight),
                         Qt::AlignCenter,
                         QString::number(model.category_values[index], 'f', 0));
        painter.drawText(QRectF(point.x() - 30.0, plot.bottom() + kStatsTop + kRowHeight, 60.0, kRowHeight),
                         Qt::AlignCenter, QString::number(percent, 'f', 1));
        painter.drawText(QRectF(point.x() - 30.0, plot.bottom() + kStatsTop + 2.0 * kRowHeight, 60.0, kRowHeight),
                         Qt::AlignCenter, QString::number(cumulative, 'f', 1));
    }
}
}  // namespace

void ChartRenderer::render(QPainter& painter, const QRectF& area, const ChartModel& model)
{
    painter.save();
    painter.fillRect(area, Qt::white);
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
    case ChartKind::Control:
    default:
        render_control(painter, area, model);
        break;
    }
    painter.restore();
}
