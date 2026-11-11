#include "reporting/chart_series_contract.h"

namespace datalab::reporting {
namespace {

bool same_coordinate(double first, double second)
{
    return first == second || (std::isnan(first) && std::isnan(second));
}

} // namespace

bool chart_primary_mirrors_series(const ChartModel& model)
{
    if (model.kind != ChartKind::TimeSeries || model.values.empty() || model.series.empty()) {
        return false;
    }
    std::size_t count = 0;
    for (const ChartSeries& series : model.series) {
        if (!series.values.empty() && series.role != ChartSeriesRole::Generic
            && series.role != ChartSeriesRole::Actual) return false;
        if (series.values.size() > model.values.size() - count) return false;
        count += series.values.size();
    }
    if (count != model.values.size()) return false;
    std::size_t offset = 0;
    for (const ChartSeries& series : model.series) {
        for (std::size_t index = 0; index < series.values.size(); ++index) {
            const double primary_x = offset + index < model.x_values.size()
                ? model.x_values[offset + index] : static_cast<double>(offset + index);
            const double series_x = index < series.x_values.size()
                ? series.x_values[index] : static_cast<double>(index);
            if (!same_coordinate(model.values[offset + index], series.values[index])
                || !same_coordinate(primary_x, series_x)) return false;
        }
        offset += series.values.size();
    }
    return true;
}

} // namespace datalab::reporting
