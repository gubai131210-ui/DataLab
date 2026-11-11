#pragma once

#include "reporting/chart_model.h"

#include <QString>
#include <QPointF>
#include <QRectF>

#include <cstddef>

class QPainter;

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
    SeriesPoint,
    Bar,
    Box,
    Interval,
    Cell,
    MatrixPoint,
    Slice,
    SurfacePoint,
    ReferenceLine,
    ParallelObservation,
    ContourCell
};

struct Hit final {
    HitKind kind = HitKind::Point;
    std::size_t index = 0;
    std::optional<std::size_t> series_index;
    std::size_t sample_index = 0;
    std::size_t matrix_row = 0;
    std::size_t matrix_column = 0;
};

struct Element final {
    ElementKind kind = ElementKind::Point;
    std::size_t index = 0;
};

QString tooltip_text(const ChartModel& model, const Hit& hit);
QString element_path(const ChartModel& model, const Element& element);
Element element_for_hit(const Hit& hit);

std::optional<Hit> hit_test(const ChartModel& model, const QRectF& area, const QPointF& position);
std::vector<std::size_t> select_rectangle(
    const ChartModel& model, const QRectF& area, const QRectF& selection);
bool supports_navigation(ChartKind kind);
bool zoom_at(ChartModel& model, const QRectF& area, const QPointF& anchor, double factor);
bool zoom_to_rectangle(ChartModel& model, const QRectF& area, const QRectF& selection);
void paint_selection(QPainter& painter, const ChartModel& model, const QRectF& area);

// Resolve chart selection indices to worksheet source rows.
// Prefer member_source_rows[i] for aggregated marks; else source_rows[i].
std::vector<std::size_t> resolve_selected_source_rows(
    const ChartModel& model,
    const std::vector<std::size_t>& selected_points);

// Map chart mark indices to worksheet rows for reverse sync (worksheet → chart).
std::vector<std::size_t> resolve_mark_indices_for_source_rows(
    const ChartModel& model,
    const std::vector<std::size_t>& source_rows);

}  // namespace chart_interaction
