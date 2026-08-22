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

    static domain::OutputPage outlier_test(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage correlation(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage one_sample_t(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage one_sample_z(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage two_sample_t(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage one_sample_equivalence(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage two_sample_equivalence(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage two_sample_equivalence_ratio(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage paired_equivalence(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage one_proportion_equivalence(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage two_proportion_equivalence(
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

    static domain::OutputPage one_proportion(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage one_poisson_rate(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage two_poisson_rate(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage chi_square(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage cross_tabulation(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage chi_square_gof(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage box_cox(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage gage_rr(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage emp_crossed(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage expanded_gage_rr(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage msa_type1(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage reliability(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage reliability_warranty(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage t_power(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
    static domain::OutputPage acf_pacf(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage mann_whitney(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage wilcoxon_signed_rank(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage sign_test(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage runs_test(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage fisher_exact(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage mcnemar(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage cochran_q(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage mood_median(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage kruskal_wallis(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage friedman(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage ewma(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage hotelling_t2(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage mewma(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage generalized_variance(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage cusum(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage zone_chart(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage z_mr(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage moving_average(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage g_chart(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage t_chart(
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

    static domain::OutputPage rsm_response(
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

    static domain::OutputPage kmeans(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage cart_tree(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage adf_test(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage poisson_regression(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage isolation_forest(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage bootstrap_mean(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage bootstrap_two_sample(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage probit_reliability(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage cluster_observations(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage ordinal_logistic(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage nominal_logistic(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage discriminant(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage ccf(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage correlogram(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage stepwise_regression(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage best_subsets_regression(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage km_interval(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage doe_plackett_burman(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage doe_response_surface_design(
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

    static domain::OutputPage imr_rs(
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

    static domain::OutputPage batch_capability(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage nonparametric_capability(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage cox_regression(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage accelerated_life(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage binomial_capability(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage poisson_capability(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage multi_vari(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage variability_chart(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage tolerance_intervals(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage capability_sixpack(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage histogram(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage eda_4plot(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage boxplot(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage pareto(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage run_chart(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage cause_and_effect(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage acceptance_sampling(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage anom(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);

    static domain::OutputPage poisson_gof(
        const domain::DataTable& table,
        const domain::AnalysisConfiguration& configuration);
};

}  // namespace datalab::application
