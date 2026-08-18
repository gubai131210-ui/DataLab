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

QTEST_MAIN(InterpretationServiceTest)
#include "interpretation_service_test.moc"
