#pragma once

#include "domain/quality_types.h"
#include "reporting/chart_model.h"

ChartModel chart_model_from_plot(const datalab::domain::PlotSpec& plot);
datalab::domain::PlotSpec plot_from_chart_model(const ChartModel& model);
