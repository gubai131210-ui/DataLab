#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "domain/statistics/nominal_logistic.h"
#include "domain/statistics/nonparametric_capability.h"
#include "domain/statistics/accelerated_life.h"
#include "domain/statistics/stepwise_regression.h"
#include "infrastructure/output_serialization.h"

#include <QtTest>

#include <string>
#include <vector>

using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;

class AlgorithmWave2HardeningTest final : public QObject {
    Q_OBJECT

private slots:
    void nominalLogisticUsesIrls();
    void nominalLogisticSerializationRoundTrip();
    void nominalLogisticInterpretationNoForbiddenPhrases();
    void nonparametricCapabilitySerializationRoundTrip();
    void acceleratedLifeNewtonMleDiagnostic();
    void acceleratedLifeSerializationRoundTrip();
    void stepwiseRegressionSerializationRoundTrip();
    void wave2FactsInterpretationNoForbiddenPhrases();
};

void AlgorithmWave2HardeningTest::nominalLogisticUsesIrls()
{
    std::vector<std::size_t> response = {0, 0, 1, 1, 2, 2, 0, 1, 2, 0,
                                         1, 2, 0, 1, 2, 0, 1, 2, 0, 1};
    std::vector<std::vector<double>> predictors(20, {1.0});
    for (std::size_t i = 0; i < predictors.size(); ++i) {
        predictors[i][0] = static_cast<double>(i) * 0.05;
    }
    const auto result = datalab::domain::statistics::fit_nominal_logistic(
        response, predictors, {"A", "B", "C"}, {"X"});
    bool has_irls = false;
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.message.find("IRLS") != std::string::npos) {
            has_irls = true;
        }
    }
    QVERIFY(has_irls);
    QVERIFY(result.converged);
}

void AlgorithmWave2HardeningTest::nominalLogisticSerializationRoundTrip()
{
    datalab::domain::OutputPage page;
    page.method_name = "Nominal Logistic Regression";
    datalab::domain::NominalLogisticFacts facts;
    facts.n = 20;
    facts.category_count = 3;
    facts.logit_count = 2;
    facts.converged = true;
    facts.reference_category = "C";
    facts.log_likelihood = -12.5;
    facts.aic = 35.0;
    facts.g_p_value = 0.04;
    page.facts.nominal_logistic = facts;

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.nominal_logistic.has_value());
    QCOMPARE(restored.facts.nominal_logistic->n, std::size_t{20});
    QCOMPARE(restored.facts.nominal_logistic->reference_category, std::string("C"));
    QVERIFY(restored.facts.nominal_logistic->g_p_value.has_value());
}

void AlgorithmWave2HardeningTest::nominalLogisticInterpretationNoForbiddenPhrases()
{
    datalab::domain::OutputPage page;
    page.method_name = "Nominal Logistic Regression";
    datalab::domain::NominalLogisticFacts facts;
    facts.n = 20;
    facts.category_count = 3;
    facts.logit_count = 2;
    facts.converged = true;
    facts.reference_category = "C";
    page.facts.nominal_logistic = facts;
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const std::string& bullet : section.bullets) {
            QVERIFY(bullet.find("过程合格") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
        }
    }
}

void AlgorithmWave2HardeningTest::nonparametricCapabilitySerializationRoundTrip()
{
    datalab::domain::OutputPage page;
    page.method_name = "Nonparametric Capability";
    datalab::domain::NonparametricCapabilityFacts facts;
    facts.n = 30;
    facts.cnp = 1.2;
    facts.cnpk = 1.1;
    facts.median = 10.0;
    page.facts.nonparametric_capability = facts;

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.nonparametric_capability.has_value());
    QCOMPARE(restored.facts.nonparametric_capability->n, std::size_t{30});
    QVERIFY(restored.facts.nonparametric_capability->cnpk.has_value());
}

void AlgorithmWave2HardeningTest::acceleratedLifeNewtonMleDiagnostic()
{
    std::vector<double> times = {5000, 5200, 4800, 2000, 2100, 1900, 800, 850, 820, 900};
    std::vector<bool> events = {true, true, true, true, true, true, true, true, true, true};
    std::vector<double> stress = {40, 45, 42, 60, 62, 58, 80, 82, 79, 85};
    const auto result = datalab::domain::statistics::fit_accelerated_life_weibull_arrhenius(
        times, events, stress, {}, 0.95, 25.0);
    bool has_newton = false;
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == "alt_newton_mle"
            || diagnostic.message.find("Newton") != std::string::npos) {
            has_newton = true;
        }
    }
    QVERIFY(has_newton);
    QVERIFY(result.converged);
    QVERIFY(result.shape > 0.0);
}

void AlgorithmWave2HardeningTest::acceleratedLifeSerializationRoundTrip()
{
    datalab::domain::OutputPage page;
    page.method_name = "Accelerated Life Testing";
    datalab::domain::AcceleratedLifeFacts facts;
    facts.n = 10;
    facts.failure_count = 10;
    facts.censored_count = 0;
    facts.stress_level_count = 3;
    facts.converged = true;
    facts.shape = 1.5;
    facts.use_stress_celsius = 25.0;
    facts.b10_at_use_stress = 1000.0;
    page.facts.accelerated_life = facts;

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.accelerated_life.has_value());
    QCOMPARE(restored.facts.accelerated_life->n, std::size_t{10});
    QVERIFY(restored.facts.accelerated_life->b10_at_use_stress.has_value());
}

void AlgorithmWave2HardeningTest::stepwiseRegressionSerializationRoundTrip()
{
    datalab::domain::OutputPage page;
    page.method_name = "Stepwise Regression";
    datalab::domain::StepwiseRegressionFacts facts;
    facts.n = 25;
    facts.candidate_count = 2;
    facts.selected_count = 1;
    facts.criterion = "aicc";
    facts.best_aicc = 45.0;
    page.facts.stepwise_regression = facts;

    const auto serialized = datalab::infrastructure::serialize_output_page(page);
    const auto restored = datalab::infrastructure::deserialize_output_page(serialized);
    QVERIFY(restored.facts.stepwise_regression.has_value());
    QCOMPARE(restored.facts.stepwise_regression->criterion, std::string("aicc"));
}

void AlgorithmWave2HardeningTest::wave2FactsInterpretationNoForbiddenPhrases()
{
    datalab::domain::OutputPage page;
    page.method_name = "Nonparametric Capability";
    datalab::domain::NonparametricCapabilityFacts facts;
    facts.n = 12;
    facts.cnp = 1.0;
    page.facts.nonparametric_capability = facts;
    datalab::application::InterpretationService::enrich(page);
    for (const auto& section : page.interpretation) {
        for (const std::string& bullet : section.bullets) {
            QVERIFY(bullet.find("过程合格") == std::string::npos);
            QVERIFY(bullet.find("已证明稳定") == std::string::npos);
            QVERIFY(bullet.find("批次合格") == std::string::npos);
        }
    }
}

QTEST_APPLESS_MAIN(AlgorithmWave2HardeningTest)
#include "algorithm_wave2_hardening_test.moc"
