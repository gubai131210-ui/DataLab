#pragma once

#include "domain/quality_types.h"

#include <string>
#include <vector>

namespace datalab::application {

class AnalysisService final {
public:
    static domain::OutputPage descriptive(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage normality_test(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage correlation(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage one_sample_t(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage two_sample_t(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage one_way_anova(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage paired_t(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage regression(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage two_proportions(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage chi_square(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage box_cox(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage gage_rr(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage msa_type1(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage reliability(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage t_power(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage mann_whitney(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage wilcoxon_signed_rank(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage kruskal_wallis(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage ewma(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage cusum(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage time_series_smoothing(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage arima(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage two_factor_anova(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage logistic_regression(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage variance_test(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage time_series_decomposition(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage doe_factorial(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage response_optimization(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage nested_gage_rr(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage attribute_agreement(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage seasonal_forecasting(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage pca(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage individuals_moving_range(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage xbar_range(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage xbar_s(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage p_chart(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage np_chart(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage c_chart(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage u_chart(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage laney_p_chart(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage laney_u_chart(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage capability(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration,
        std::vector<double>* capability_indices = nullptr);

    static domain::OutputPage distribution_identification(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage between_within_capability(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage capability_sixpack(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage histogram(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage boxplot(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage pareto(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
};

}  // namespace datalab::application
