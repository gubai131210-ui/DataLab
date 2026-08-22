#pragma once

#include "domain/quality_types.h"

namespace datalab::application {

class GraphService final {
public:
    static domain::OutputPage run(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage scatter(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage interval(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage correlation(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage bubble(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage probability(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage ecdf(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage matrix(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage marginal(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage parallel(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage heatmap(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage time_series(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage area(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage contour(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage pie(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage density(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage hexbin(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage violin(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage bar(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
};

}  // namespace datalab::application
