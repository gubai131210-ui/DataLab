#include "reporting/chart_interaction.h"
#include <QStringList>
#include "domain/statistics/special_cause_rule_catalog.h"

#include <algorithm>
#include <set>
#include <cmath>
#include <numeric>

namespace chart_interaction {

Element element_for_hit(const Hit& hit)
{
    switch (hit.kind) {
    case HitKind::SeriesPoint: return {ElementKind::Series, hit.series_index.value_or(0)};
    case HitKind::Bar: return {ElementKind::Bar, hit.index};
    case HitKind::Box: return {ElementKind::Box, hit.index};
    case HitKind::Cell: return {ElementKind::Cell, hit.index};
    case HitKind::Slice: return {ElementKind::Slice, hit.index};
    case HitKind::ParallelObservation: return {ElementKind::ParallelObservation, hit.index};
    case HitKind::ContourCell: return {ElementKind::ContourCell, hit.index};
    case HitKind::ReferenceLine: return {ElementKind::ReferenceLine, hit.index};
    default: return {ElementKind::Point, hit.index};
    }
}

QString element_path(const ChartModel& model, const Element& element)
{
    switch (element.kind) {
    case ElementKind::Point:
        return QStringLiteral("图形 > 数据点 %1")
            .arg(static_cast<qulonglong>(element.index + 1));
    case ElementKind::Series:
        if (element.index < model.series.size()
            && !model.series[element.index].label.isEmpty()) {
            return QStringLiteral("图形 > 数据系列 > ") + model.series[element.index].label;
        }
        return QStringLiteral("图形 > 数据系列");
    case ElementKind::ReferenceLine:
        return QStringLiteral("图形 > 控制线 / %1")
            .arg(element.index == 0 ? QStringLiteral("CL") : element.index == 1 ? QStringLiteral("LCL") : QStringLiteral("UCL"));
    case ElementKind::DataRegion:
        return QStringLiteral("图形 > 数据区");
    case ElementKind::Cell:
        return QStringLiteral("图形 > 单元格 %1")
            .arg(static_cast<qulonglong>(element.index + 1));
    case ElementKind::Bar:
        return QStringLiteral("图形 > 柱 %1")
            .arg(static_cast<qulonglong>(element.index + 1));
    case ElementKind::Box:
        return QStringLiteral("图形 > 箱体 %1")
            .arg(static_cast<qulonglong>(element.index + 1));
    case ElementKind::Slice:
        return QStringLiteral("图形 > 扇区 %1")
            .arg(static_cast<qulonglong>(element.index + 1));
    case ElementKind::ParallelObservation:
        return QStringLiteral("图形 > 平行坐标观测 %1")
            .arg(static_cast<qulonglong>(element.index + 1));
    case ElementKind::ContourCell:
        return QStringLiteral("图形 > 等高线网格 %1")
            .arg(static_cast<qulonglong>(element.index + 1));
    }
    return QStringLiteral("图形");
}

QString tooltip_text(const ChartModel& model, const Hit& hit)
{
    const auto number = [](double value) {
        return std::isfinite(value) ? QString::number(value, 'g', 8) : QStringLiteral("不可用");
    };
    const auto rows_note = [&model, &hit](QString text) {
        const auto rows = resolve_selected_source_rows(model, {hit.index});
        if (rows.size() == 1) text += QStringLiteral("\n原始行: %1").arg(static_cast<qulonglong>(rows.front() + 1));
        if (rows.size() > 1) text += QStringLiteral("\n关联观测: %1\n示例行: %2")
            .arg(static_cast<qulonglong>(rows.size())).arg(static_cast<qulonglong>(rows.front() + 1));
        return text;
    };
    if (hit.kind == HitKind::ReferenceLine) {
        const std::vector<double>* lines[] = {&model.center, &model.lower, &model.upper};
        const QString labels[] = {model.center_label, QStringLiteral("LCL"), QStringLiteral("UCL")};
        if (hit.index >= 3 || hit.sample_index >= lines[hit.index]->size()) return {};
        return labels[hit.index] + QStringLiteral("\n值: ") + number((*lines[hit.index])[hit.sample_index]);
    }
    if (hit.kind == HitKind::SeriesPoint && hit.series_index.has_value()
        && *hit.series_index < model.series.size()) {
        const auto& series = model.series[*hit.series_index];
        if (hit.sample_index >= series.values.size()) return {};
        const double x = hit.sample_index < series.x_values.size() ? series.x_values[hit.sample_index] : static_cast<double>(hit.sample_index);
        return rows_note(QStringLiteral("系列: %1\nX: %2\nY: %3").arg(series.label, number(x), number(series.values[hit.sample_index])));
    }
    if (hit.kind == HitKind::Bar) {
        if (model.kind == ChartKind::Histogram) {
            if (hit.index >= model.histogram_counts.size() || hit.index + 1 >= model.histogram_edges.size()) return {};
            const double total = std::accumulate(model.histogram_counts.begin(), model.histogram_counts.end(), 0.0);
            QString text = QStringLiteral("区间 [%1, %2%3\n频数: %4")
                .arg(number(model.histogram_edges[hit.index]), number(model.histogram_edges[hit.index + 1]),
                     hit.index + 1 == model.histogram_counts.size() ? QStringLiteral("]") : QStringLiteral(")"), number(model.histogram_counts[hit.index]));
            if (total > 0.0) text += QStringLiteral("\n百分比: %1%").arg(number(100.0 * model.histogram_counts[hit.index] / total));
            return rows_note(text);
        }
        const auto& values = model.category_values.empty() ? model.values : model.category_values;
        QString text = hit.index < model.categories.size() ? model.categories[hit.index] : QString::number(static_cast<qulonglong>(hit.index + 1));
        if (hit.series_index.has_value() && *hit.series_index < model.series.size()) {
            const auto& series = model.series[*hit.series_index];
            if (hit.sample_index >= series.values.size()) return {};
            text += QStringLiteral("\n系列: %1\n值: %2").arg(series.label, number(series.values[hit.sample_index]));
        } else if (hit.index < values.size()) text += QStringLiteral("\n值: ") + number(values[hit.index]);
        if (model.kind == ChartKind::Pareto && hit.index < model.cumulative_percent.size())
            text += QStringLiteral("\n累计百分比: %1%").arg(number(model.cumulative_percent[hit.index]));
        return rows_note(text);
    }
    if (hit.kind == HitKind::Box && hit.index < model.box_median.size()) {
        QString text = hit.index < model.box_labels.size() ? model.box_labels[hit.index] : QStringLiteral("箱组 %1").arg(static_cast<qulonglong>(hit.index + 1));
        if (hit.index < model.box_min.size()) text += QStringLiteral("\n下须: ") + number(model.box_min[hit.index]);
        if (hit.index < model.box_q1.size()) text += QStringLiteral("\nQ1: ") + number(model.box_q1[hit.index]);
        text += QStringLiteral("\n中位数: ") + number(model.box_median[hit.index]);
        if (hit.index < model.box_q3.size()) text += QStringLiteral("\nQ3: ") + number(model.box_q3[hit.index]);
        if (hit.index < model.box_max.size()) text += QStringLiteral("\n上须: ") + number(model.box_max[hit.index]);
        return rows_note(text);
    }
    if (hit.kind == HitKind::Interval && hit.index < model.values.size()) {
        QString text = hit.index < model.categories.size() ? model.categories[hit.index] : QStringLiteral("区间 %1").arg(static_cast<qulonglong>(hit.index + 1));
        text += QStringLiteral("\n均值: ") + number(model.values[hit.index]);
        if (hit.index < model.interval_lower.size() && hit.index < model.interval_upper.size())
            text += QStringLiteral("\n区间: [%1, %2]").arg(number(model.interval_lower[hit.index]), number(model.interval_upper[hit.index]));
        return rows_note(text);
    }
    if (hit.kind == HitKind::Cell || hit.kind == HitKind::MatrixPoint || hit.kind == HitKind::SurfacePoint) {
        const auto row = hit.matrix_row, column = hit.matrix_column;
        if (hit.kind == HitKind::SurfacePoint) {
            if (row >= model.contour_y.size() || column >= model.contour_x.size() || row >= model.matrix_values.size() || column >= model.matrix_values[row].size()) return {};
            return QStringLiteral("曲面节点\nX: %1\nY: %2\nZ: %3")
                .arg(number(model.contour_x[column]), number(model.contour_y[row]), number(model.matrix_values[row][column]));
        }
        const auto& row_names = model.categories.empty() ? model.matrix_labels : model.categories;
        const QString row_name = row < row_names.size() ? row_names[row] : QString::number(static_cast<qulonglong>(row + 1));
        const QString column_name = column < model.matrix_labels.size() ? model.matrix_labels[column] : QString::number(static_cast<qulonglong>(column + 1));
        if (hit.kind == HitKind::MatrixPoint) {
            if (column >= model.matrix_values.size() || row >= model.matrix_values.size()
                || hit.index >= model.matrix_values[column].size() || hit.index >= model.matrix_values[row].size()) return {};
            return rows_note(QStringLiteral("%1 × %2\nX: %3\nY: %4").arg(column_name, row_name,
                number(model.matrix_values[column][hit.index]), number(model.matrix_values[row][hit.index])));
        }
        if (row >= model.matrix_values.size() || column >= model.matrix_values[row].size()) return {};
        QString text = QStringLiteral("%1 × %2\n值: %3").arg(row_name, column_name, number(model.matrix_values[row][column]));
        if (row < model.matrix_counts.size() && column < model.matrix_counts[row].size())
            text += QStringLiteral("\n配对 N: %1").arg(static_cast<qulonglong>(model.matrix_counts[row][column]));
        if (row < model.matrix_p_values.size() && column < model.matrix_p_values[row].size())
            text += QStringLiteral("\np: ") + number(model.matrix_p_values[row][column]);
        return rows_note(text);
    }
    if (hit.kind == HitKind::Slice && hit.index < model.category_values.size()) {
        double total = 0.0;
        for (double value : model.category_values) if (std::isfinite(value) && value > 0.0) total += value;
        const QString name = hit.index < model.categories.size() ? model.categories[hit.index] : QStringLiteral("扇区 %1").arg(static_cast<qulonglong>(hit.index + 1));
        QString text = QStringLiteral("%1\n值: %2").arg(name, number(model.category_values[hit.index]));
        if (total > 0.0) text += QStringLiteral("\n百分比: %1%").arg(number(100.0 * model.category_values[hit.index] / total));
        return rows_note(text);
    }
    if (hit.kind == HitKind::ParallelObservation
        && hit.index < model.matrix_values.size()) {
        QString text = QStringLiteral("观测 %1")
            .arg(static_cast<qulonglong>(hit.index + 1));
        if (hit.index < model.point_groups.size()
            && !model.point_groups[hit.index].isEmpty()) {
            text += QStringLiteral("\n分组: ") + model.point_groups[hit.index];
        }
        for (std::size_t axis = 0;
             axis < model.matrix_values[hit.index].size()
             && axis < static_cast<std::size_t>(model.matrix_labels.size()); ++axis) {
            text += QStringLiteral("\n%1: %2")
                .arg(model.matrix_labels[static_cast<int>(axis)])
                .arg(model.matrix_values[hit.index][axis], 0, 'g', 8);
        }
        return rows_note(text);
    }

    if (hit.kind == HitKind::ContourCell
        && model.contour_x.size() >= 2 && model.contour_y.size() >= 2) {
        const std::size_t columns = model.contour_x.size() - 1;
        const std::size_t row = hit.index / columns;
        const std::size_t column = hit.index % columns;
        if (row < model.matrix_values.size()
            && column < model.matrix_values[row].size()) {
            return QStringLiteral("网格单元 (%1, %2)\n值: %3")
                .arg(model.contour_x[column], 0, 'g', 8)
                .arg(model.contour_y[row], 0, 'g', 8)
                .arg(model.matrix_values[row][column], 0, 'g', 8);
        }
    }

    if (hit.kind == HitKind::Point && hit.index < model.values.size()) {
        const double x = hit.index < model.x_values.size() ? model.x_values[hit.index] : static_cast<double>(hit.index);
        QString text = QStringLiteral("点 %1\nX: %2\nY: %3")
            .arg(static_cast<qulonglong>(hit.index + 1)).arg(number(x), number(model.values[hit.index]));
        if (model.kind == ChartKind::Probability) text = QStringLiteral("点 %1\n理论分位数: %2\n观测值: %3")
            .arg(static_cast<qulonglong>(hit.index + 1)).arg(number(x), number(model.values[hit.index]));
        if (model.kind == ChartKind::Bubble && hit.index < model.bubble_sizes.size()) text += QStringLiteral("\n气泡大小: ") + number(model.bubble_sizes[hit.index]);
        if (hit.index < model.point_labels.size() && !model.point_labels[hit.index].isEmpty()) text += QStringLiteral("\n标签: ") + model.point_labels[hit.index];
        if (hit.index < model.point_groups.size() && !model.point_groups[hit.index].isEmpty()) text += QStringLiteral("\n分组: ") + model.point_groups[hit.index];
        if (model.kind == ChartKind::Control) {
            QStringList tests;
            if (hit.index < model.triggered_tests.size()) {
                for (int test : model.triggered_tests[hit.index]) tests << QString::fromStdString(datalab::domain::statistics::special_cause_rule_display_name(test));
            } else {
                for (std::size_t test = 0; test < model.special_cause_points.size(); ++test) {
                    const auto& points = model.special_cause_points[test];
                    if (std::find(points.begin(), points.end(), hit.index) != points.end())
                        tests << QString::fromStdString(datalab::domain::statistics::special_cause_rule_display_name(static_cast<int>(test + 1)));
                }
            }
            text += QStringLiteral("\n触发规则: ") + (tests.isEmpty() ? QStringLiteral("未触发") : tests.join(QStringLiteral(", ")));
            if (model.sigma_z > 0.0) text += QStringLiteral("\nSigma Z: ") + number(model.sigma_z);
            if (hit.index < model.signal_direction.size() && model.signal_direction[hit.index] != 0)
                text += QStringLiteral("\n信号方向: ") + (model.signal_direction[hit.index] > 0 ? QStringLiteral("上侧") : QStringLiteral("下侧"));
        }
        return rows_note(text);
    }
    return {};
}

std::vector<std::size_t> resolve_selected_source_rows(
    const ChartModel& model,
    const std::vector<std::size_t>& selected_points)
{
    std::set<std::size_t> unique;
    for (const std::size_t point : selected_points) {
        // When member_source_rows is present for this index, it is authoritative —
        // even if empty (aggregated empty cell must not fall back to row 0).
        if (point < model.member_source_rows.size()) {
            for (const std::size_t row : model.member_source_rows[point]) {
                unique.insert(row);
            }
            continue;
        }
        if (model.kind == ChartKind::Histogram) {
            if (point + 1 >= model.histogram_edges.size() || model.values.size() != model.source_rows.size()) continue;
            for (std::size_t i = 0; i < model.values.size(); ++i) {
                const double value = model.values[i];
                if (value >= model.histogram_edges[point]
                    && (value < model.histogram_edges[point + 1]
                        || (point + 2 == model.histogram_edges.size() && value == model.histogram_edges[point + 1]))) unique.insert(model.source_rows[i]);
            }
            continue;
        }
        if (model.kind == ChartKind::Density || model.kind == ChartKind::Contour
            || model.kind == ChartKind::Surface || model.kind == ChartKind::Correlation || model.kind == ChartKind::Heatmap) continue;
        const bool point_chart = model.kind == ChartKind::Control || model.kind == ChartKind::Scatter
            || model.kind == ChartKind::TimeSeries || model.kind == ChartKind::Area || model.kind == ChartKind::Marginal;
        if (point_chart && point >= model.values.size() && !model.series.empty()) {
            std::size_t offset = model.values.size();
            for (const auto& series : model.series) {
                if (point >= offset && point - offset < series.values.size()
                    && series.role == ChartSeriesRole::Actual && series.values.size() == model.source_rows.size()) unique.insert(model.source_rows[point - offset]);
                offset += series.values.size();
            }
            continue;
        }
        if (point < model.source_rows.size()) {
            unique.insert(model.source_rows[point]);
        }
    }
    return {unique.begin(), unique.end()};
}

std::vector<std::size_t> resolve_mark_indices_for_source_rows(
    const ChartModel& model,
    const std::vector<std::size_t>& source_rows)
{
    std::set<std::size_t> marks;
    const std::set<std::size_t> wanted(source_rows.begin(), source_rows.end());
    for (std::size_t index = 0; index < model.member_source_rows.size(); ++index) {
            const auto& members = model.member_source_rows[index];
            if (std::any_of(members.begin(), members.end(), [&wanted](std::size_t row) { return wanted.count(row) != 0; })) {
                marks.insert(index);
            }
    }
    if (model.kind == ChartKind::Histogram) {
        for (std::size_t index = model.member_source_rows.size(); index < model.histogram_counts.size(); ++index) {
            const auto rows = resolve_selected_source_rows(model, {index});
            if (std::any_of(rows.begin(), rows.end(), [&wanted](std::size_t row) { return wanted.count(row) != 0; })) marks.insert(index);
        }
        return {marks.begin(), marks.end()};
    }
    const auto add_if_resolved = [&](std::size_t index) {
        if (index < model.member_source_rows.size()) return;
        const auto rows = resolve_selected_source_rows(model, {index});
        if (std::any_of(rows.begin(), rows.end(), [&wanted](std::size_t row) { return wanted.count(row) != 0; })) marks.insert(index);
    };
    const bool numeric_chart = model.kind == ChartKind::Control || model.kind == ChartKind::Scatter
        || model.kind == ChartKind::TimeSeries || model.kind == ChartKind::Area || model.kind == ChartKind::Marginal;
    const std::size_t primary_count = numeric_chart && !model.series.empty() ? model.values.size() : model.source_rows.size();
    for (std::size_t index = 0; index < primary_count; ++index) add_if_resolved(index);
    const bool point_chart = model.kind == ChartKind::Control || model.kind == ChartKind::Scatter
        || model.kind == ChartKind::TimeSeries || model.kind == ChartKind::Area || model.kind == ChartKind::Marginal;
    if (point_chart) {
        std::size_t offset = model.values.size();
        for (const auto& series : model.series) {
            if (series.role == ChartSeriesRole::Actual && series.values.size() == model.source_rows.size())
                for (std::size_t index = 0; index < series.values.size(); ++index) add_if_resolved(offset + index);
            offset += series.values.size();
        }
    }
    return {marks.begin(), marks.end()};
}

}  // namespace chart_interaction
