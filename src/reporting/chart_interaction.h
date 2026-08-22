#pragma once

#include "reporting/chart_model.h"

#include <QString>

#include <cstddef>

namespace chart_interaction {

enum class ElementKind {
    Point,
    Series,
    ReferenceLine,
    DataRegion,
    Cell,
    Bar,
    Box,
    Slice,
    ParallelObservation,
    ContourCell
};

enum class HitKind {
    Point,
    ParallelObservation,
    ContourCell
};

struct Hit final {
    HitKind kind = HitKind::Point;
    std::size_t index = 0;
};

struct Element final {
    ElementKind kind = ElementKind::Point;
    std::size_t index = 0;
};

QString tooltip_text(const ChartModel& model, const Hit& hit);
QString element_path(const ChartModel& model, const Element& element);

// Resolve chart selection indices to worksheet source rows.
// Prefer member_source_rows[i] for aggregated marks; else source_rows[i].
std::vector<std::size_t> resolve_selected_source_rows(
    const ChartModel& model,
    const std::vector<std::size_t>& selected_points);

}  // namespace chart_interaction
