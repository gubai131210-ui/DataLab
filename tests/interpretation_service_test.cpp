#include "application/interpretation_service.h"

#include <QtTest/QtTest>

#include <algorithm>

class InterpretationServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void usesStructuredCapabilityFacts();
    void usesStructuredSpcFacts();
    void usesStructuredRegressionFacts();
    void usesStructuredAnovaFacts();
    void usesStructuredGageFacts();
    void usesStructuredReliabilityFacts();
    void usesStructuredDescriptiveAndAdvancedFacts();
    void usesChiSquareFactsWithoutCausalClaim();
    void usesStructuredForecastFactsForTimeSeriesMethods();
    void usesStructuredDoeFactsForResponseOptimization();
    void usesToleranceFactsWithoutCapabilityClaim();
    void usesBinomialCapabilityFactsWithoutPassClaim();
    void usesProportionFactsWithoutPassClaim();
    void usesBoxCoxFactsWithoutNormalityClaim();
    void usesPoissonRateFactsWithoutPassClaim();
    void usesEquivalenceFactsWithoutPassClaim();
    void usesDoeResidualFactsWithoutNormalityClaim();
};

void InterpretationServiceTest::usesStructuredCapabilityFacts()
{
    datalab::domain::OutputPage page;
    page.method_name = "Capability";
    page.facts.capability = datalab::domain::CapabilityFacts{1.10, 1.42};

    datalab::application::InterpretationService::enrich(page);

    QVERIFY(!page.interpretation.empty());
    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(conclusion->bullets[0].find("Cpk = 1.100000") != std::string::npos);
    QVERIFY(conclusion->bullets[1].find("Ppk = 1.420000") != std::string::npos);
}

void InterpretationServiceTest::usesStructuredSpcFacts()
{
    datalab::domain::OutputPage page;
    page.method_name = "Xbar-R Chart";
    page.facts.spc = datalab::domain::SpcFacts{3, 1.25};

    datalab::application::InterpretationService::enrich(page);

    QVERIFY(!page.interpretation.empty());
    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(conclusion->bullets[0].find("3 个控制图超限点") != std::string::npos);
    QVERIFY(conclusion->bullets[1].find("Sigma Z = 1.250000") != std::string::npos);
}

void InterpretationServiceTest::usesStructuredRegressionFacts()
{
    datalab::domain::OutputPage page;
    page.method_name = "Linear Regression";
    datalab::domain::RegressionFacts facts;
    facts.r_squared = 0.82;
    facts.influential_count = 2;
    facts.max_vif = 6.5;
    facts.assumption_status = "evidence_against";
    page.facts.regression = facts;

    datalab::application::InterpretationService::enrich(page);

    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(conclusion->bullets.front().find("R²") != std::string::npos);
    const auto limitations = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "限制与数据质量"; });
    QVERIFY(limitations != page.interpretation.cend());
    QVERIFY(std::any_of(
        limitations->bullets.cbegin(), limitations->bullets.cend(),
        [](const std::string& bullet) {
            return bullet.find("异常观测表") != std::string::npos
                && bullet.find("不会自动删除") != std::string::npos;
        }));
}

void InterpretationServiceTest::usesStructuredAnovaFacts()
{
    datalab::domain::OutputPage page;
    page.method_name = "One-Way ANOVA";
    datalab::domain::AnovaFacts facts;
    facts.significant_terms = {"组间"};
    facts.family_confidence_level = 0.95;
    facts.tukey_significant_pairs = 1;
    page.facts.anova = facts;

    datalab::application::InterpretationService::enrich(page);

    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(conclusion->bullets.front().find("组间") != std::string::npos);
}

void InterpretationServiceTest::usesStructuredGageFacts()
{
    datalab::domain::OutputPage page;
    page.method_name = "Crossed Gage R&R (ANOVA)";
    datalab::domain::MsaFacts facts;
    facts.ndc = 3.0;
    facts.ndc_available = true;
    facts.gage_percent_study_variation = 28.0;
    facts.negative_variance_truncated = true;
    page.facts.msa = facts;

    datalab::application::InterpretationService::enrich(page);

    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(conclusion->bullets.front().find("ndc") != std::string::npos);
}

void InterpretationServiceTest::usesStructuredReliabilityFacts()
{
    datalab::domain::OutputPage page;
    page.method_name = "Kaplan-Meier";
    datalab::domain::ReliabilityFacts facts;
    facts.valid_count = 10;
    facts.failure_count = 6;
    facts.censored_count = 4;
    facts.identifiable = true;
    page.facts.reliability = facts;

    datalab::application::InterpretationService::enrich(page);

    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(conclusion->bullets.front().find("删失") != std::string::npos);
}

void InterpretationServiceTest::usesStructuredDescriptiveAndAdvancedFacts()
{
    datalab::domain::OutputPage descriptive;
    descriptive.method_name = "Display Descriptive Statistics";
    descriptive.facts.descriptive = datalab::domain::DescriptiveFacts{12, 2, 10.5, 1.2};
    datalab::application::InterpretationService::enrich(descriptive);
    const auto desc_conclusion = std::find_if(
        descriptive.interpretation.cbegin(), descriptive.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(desc_conclusion != descriptive.interpretation.cend());
    QVERIFY(desc_conclusion->bullets.front().find("有效观测 N = 12") != std::string::npos);

    datalab::domain::OutputPage logistic;
    logistic.method_name = "Binary Logistic Regression";
    datalab::domain::LogisticFacts logistic_facts;
    logistic_facts.converged = false;
    logistic.facts.logistic = logistic_facts;
    datalab::application::InterpretationService::enrich(logistic);
    const auto logistic_conclusion = std::find_if(
        logistic.interpretation.cbegin(), logistic.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(logistic_conclusion != logistic.interpretation.cend());
    QVERIFY(logistic_conclusion->bullets.front().find("未收敛") != std::string::npos);

    datalab::domain::OutputPage logistic_hl;
    logistic_hl.method_name = "Binary Logistic Regression";
    datalab::domain::LogisticFacts hl_facts;
    hl_facts.converged = true;
    hl_facts.hosmer_lemeshow_status = "computed";
    hl_facts.hosmer_lemeshow_p = 0.02;
    logistic_hl.facts.logistic = hl_facts;
    datalab::application::InterpretationService::enrich(logistic_hl);
    const auto hl_conclusion = std::find_if(
        logistic_hl.interpretation.cbegin(), logistic_hl.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(hl_conclusion != logistic_hl.interpretation.cend());
    QVERIFY(hl_conclusion->bullets.front().find("拒绝拟合不足") != std::string::npos);
    QVERIFY(hl_conclusion->bullets.front().find("已充分") == std::string::npos);

    datalab::domain::OutputPage pca;
    pca.method_name = "Principal Component Analysis";
    pca.facts.pca = datalab::domain::PcaFacts{"standardized", 2, 1};
    datalab::application::InterpretationService::enrich(pca);
    const auto pca_conclusion = std::find_if(
        pca.interpretation.cbegin(), pca.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(pca_conclusion != pca.interpretation.cend());
    QVERIFY(pca_conclusion->bullets.front().find("T²/Q") != std::string::npos);
    QVERIFY(pca_conclusion->bullets.front().find("过程合格") == std::string::npos);

    datalab::domain::OutputPage nonparametric;
    nonparametric.method_name = "Mann-Whitney U";
    datalab::domain::NonparametricFacts np_facts;
    np_facts.method = "mann_whitney";
    np_facts.p_value = 0.12;
    np_facts.approximation = "normal";
    np_facts.small_sample_warning = true;
    nonparametric.facts.nonparametric = np_facts;
    datalab::application::InterpretationService::enrich(nonparametric);
    const auto np_conclusion = std::find_if(
        nonparametric.interpretation.cbegin(), nonparametric.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(np_conclusion != nonparametric.interpretation.cend());
    QVERIFY(np_conclusion->bullets.front().find("已证明") == std::string::npos);
    QVERIFY(np_conclusion->bullets.front().find("不能证明") != std::string::npos);

    datalab::domain::OutputPage variance;
    variance.method_name = "Variance Test";
    datalab::domain::VarianceFacts variance_facts;
    variance_facts.method = "Levene";
    variance_facts.p_value = 0.20;
    variance_facts.group_count = 2;
    variance.facts.variance = variance_facts;
    datalab::application::InterpretationService::enrich(variance);
    const auto variance_conclusion = std::find_if(
        variance.interpretation.cbegin(), variance.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(variance_conclusion != variance.interpretation.cend());
    QVERIFY(variance_conclusion->bullets.front().find("不能证明方差相等") != std::string::npos);
}

void InterpretationServiceTest::usesChiSquareFactsWithoutCausalClaim()
{
    datalab::domain::OutputPage page;
    page.method_name = "Chi-Square Association";
    datalab::domain::ChiSquareFacts facts;
    facts.statistic = 4.2;
    facts.p_value = 0.04;
    facts.expected_count_warning = true;
    page.facts.chi_square = facts;
    datalab::application::InterpretationService::enrich(page);
    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(conclusion->bullets.front().find("不能证明因果关系") != std::string::npos);
    QVERIFY(conclusion->bullets.front().find("已证明") == std::string::npos);
    QVERIFY(conclusion->bullets.front().find("显著相关") == std::string::npos);
    const auto limitations = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "限制与数据质量"; });
    QVERIFY(limitations != page.interpretation.cend());
    QVERIFY(limitations->bullets.front().find("期望频数") != std::string::npos);
}

void InterpretationServiceTest::usesStructuredForecastFactsForTimeSeriesMethods()
{
    datalab::domain::OutputPage decomposition;
    decomposition.method_name = "Time Series Decomposition";
    decomposition.configuration.time_series.decomposition_seasonal_period = 4;
    decomposition.configuration.time_series.decomposition_model = "additive";
    decomposition.facts.forecast = datalab::domain::ForecastFacts{12.5, std::nullopt,
                                                                  std::nullopt, std::nullopt};
    datalab::application::InterpretationService::enrich(decomposition);
    const auto decomposition_conclusion = std::find_if(
        decomposition.interpretation.cbegin(), decomposition.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(decomposition_conclusion != decomposition.interpretation.cend());
    QVERIFY(decomposition_conclusion->bullets.front().find("MAPE = 12.500000")
            != std::string::npos);
    const auto decomposition_advice = std::find_if(
        decomposition.interpretation.cbegin(), decomposition.interpretation.cend(),
        [](const auto& section) { return section.heading == "工程建议"; });
    QVERIFY(decomposition_advice != decomposition.interpretation.cend());
    QVERIFY(decomposition_advice->bullets.front().find("加法分解") != std::string::npos);

    datalab::domain::OutputPage smoothing;
    smoothing.method_name = "Double Exponential Smoothing";
    smoothing.facts.forecast = datalab::domain::ForecastFacts{8.0, std::nullopt,
                                                               std::nullopt, std::nullopt};
    datalab::application::InterpretationService::enrich(smoothing);
    const auto smoothing_conclusion = std::find_if(
        smoothing.interpretation.cbegin(), smoothing.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(smoothing_conclusion != smoothing.interpretation.cend());
    QVERIFY(smoothing_conclusion->bullets.front().find("MAPE = 8.000000")
            != std::string::npos);
}

void InterpretationServiceTest::usesStructuredDoeFactsForResponseOptimization()
{
    datalab::domain::OutputPage page;
    page.method_name = "Response Optimization";
    page.configuration.inference.confidence_level = 0.95;
    datalab::domain::DoeFacts facts;
    facts.multi_response = true;
    facts.response_count = 2;
    facts.response_names = {"Y", "Y2"};
    facts.best_overall_desirability = 0.92;
    facts.has_p_value = true;
    facts.significant_terms = {"A", "B"};
    facts.prediction_interval_available = false;
    page.facts.doe = facts;

    datalab::application::InterpretationService::enrich(page);

    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(std::any_of(
        conclusion->bullets.cbegin(), conclusion->bullets.cend(),
        [](const std::string& bullet) {
            return bullet.find("多响应 Desirability") != std::string::npos;
        }));
    QVERIFY(std::any_of(
        conclusion->bullets.cbegin(), conclusion->bullets.cend(),
        [](const std::string& bullet) {
            return bullet.find("总体 Desirability = 0.920000") != std::string::npos;
        }));
    const auto limitations = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "限制与数据质量"; });
    QVERIFY(limitations != page.interpretation.cend());
    QVERIFY(std::any_of(
        limitations->bullets.cbegin(), limitations->bullets.cend(),
        [](const std::string& bullet) {
            return bullet.find("置信/预测区间不可用") != std::string::npos;
        }));
}

void InterpretationServiceTest::usesToleranceFactsWithoutCapabilityClaim()
{
    datalab::domain::OutputPage page;
    page.method_name = "Tolerance Intervals";
    datalab::domain::ToleranceFacts facts;
    facts.method = "howe_two_sided";
    facts.coverage = 0.95;
    facts.confidence_level = 0.95;
    facts.lower = 1.2;
    facts.upper = 8.4;
    facts.assumption_status = "not_verified";
    page.facts.tolerance = facts;
    datalab::application::InterpretationService::enrich(page);
    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(conclusion->bullets.front().find("howe_two_sided") != std::string::npos);
    QVERIFY(conclusion->bullets.front().find("not_verified") != std::string::npos);
    QVERIFY(conclusion->bullets.front().find("合格") == std::string::npos);
    QVERIFY(conclusion->bullets.front().find("规格已覆盖") == std::string::npos);
}

void InterpretationServiceTest::usesBinomialCapabilityFactsWithoutPassClaim()
{
    datalab::domain::OutputPage page;
    page.method_name = "Binomial Capability";
    datalab::domain::CapabilityFacts facts;
    facts.method = "binomial";
    facts.average_p = 0.04;
    facts.percent_defective = 4.0;
    facts.process_z = 1.75;
    facts.assumption_status = "not_verified";
    page.facts.capability = facts;
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(!page.interpretation.empty());
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
            QVERIFY(bullet.find("Cpk") == std::string::npos);
        }
    }
}

void InterpretationServiceTest::usesProportionFactsWithoutPassClaim()
{
    datalab::domain::OutputPage page;
    page.method_name = "1 Proportion";
    datalab::domain::ProportionFacts facts;
    facts.method = "exact";
    facts.proportion = 0.2;
    facts.hypothesized = 0.5;
    facts.p_value = 0.109375;
    facts.assumption_status = "not_verified";
    page.facts.proportion = facts;
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(!page.interpretation.empty());
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
        }
    }
}

void InterpretationServiceTest::usesBoxCoxFactsWithoutNormalityClaim()
{
    datalab::domain::OutputPage page;
    page.method_name = "Box-Cox Transformation";
    datalab::domain::BoxCoxFacts facts;
    facts.lambda = 0.0;
    facts.n = 4;
    facts.missing_count = 1;
    facts.transformed_standard_deviation = 0.5;
    facts.assumption_status = "not_verified";
    page.facts.box_cox = facts;
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(!page.interpretation.empty());
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
            QVERIFY(bullet.find("已正态") == std::string::npos);
            QVERIFY(bullet.find("过程合格") == std::string::npos);
        }
    }
}

void InterpretationServiceTest::usesPoissonRateFactsWithoutPassClaim()
{
    datalab::domain::OutputPage page;
    page.method_name = "1-Sample Poisson Rate";
    datalab::domain::PoissonRateFacts facts;
    facts.kind = "one_sample";
    facts.method = "exact";
    facts.rate = 0.2;
    facts.hypothesized = 0.5;
    facts.p_value = 0.124672;
    facts.assumption_status = "not_verified";
    page.facts.poisson_rate = facts;
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(!page.interpretation.empty());
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
        }
    }
}

void InterpretationServiceTest::usesEquivalenceFactsWithoutPassClaim()
{
    datalab::domain::OutputPage page;
    page.method_name = "1-Sample Equivalence Test";
    datalab::domain::EquivalenceFacts facts;
    facts.kind = "one_sample";
    facts.lower = -1.0;
    facts.upper = 1.0;
    facts.ci_lower = -0.4;
    facts.ci_upper = 0.4;
    facts.within_limits = false;
    facts.assumption_status = "not_verified";
    page.facts.equivalence = facts;
    datalab::application::InterpretationService::enrich(page);
    QVERIFY(!page.interpretation.empty());
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("合格") == std::string::npos);
            QVERIFY(bullet.find("已证明等价") == std::string::npos
                    || bullet.find("不能写成已证明等价") != std::string::npos);
        }
    }
}

void InterpretationServiceTest::usesDoeResidualFactsWithoutNormalityClaim()
{
    datalab::domain::OutputPage page;
    page.method_name = "2-Level Factorial Response Analysis";
    datalab::domain::DoeFacts facts;
    facts.residual_count = 8;
    page.facts.doe = facts;
    datalab::application::InterpretationService::enrich(page);
    bool mentions_histogram = false;
    for (const auto& section : page.interpretation) {
        for (const auto& bullet : section.bullets) {
            QVERIFY(bullet.find("残差已正态") == std::string::npos);
            QVERIFY(bullet.find("模型合格") == std::string::npos);
            if (bullet.find("直方图") != std::string::npos) {
                mentions_histogram = true;
            }
        }
    }
    QVERIFY(mentions_histogram);
}

QTEST_MAIN(InterpretationServiceTest)
#include "interpretation_service_test.moc"
