#include "reporting/chart_interaction.h"

namespace chart_interaction {

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
        return QStringLiteral("图形 > 控制线");
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
        return text;
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

    return {};
}

}  // namespace chart_interaction
