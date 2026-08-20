#include "application/analysis_intent.h"
#include "domain/quality_types.h"
#include "ui/analysis_commands.h"

#include <QtTest/QtTest>

class AnalysisIntentTest final : public QObject {
    Q_OBJECT

private slots:
    void collectsRoleAndTextValues();
    void commandBuilderDoesNotRequireDialog();
    void appliesIndependentResponseObjectives();
    void fallsBackToSharedObjectiveForSingleResponse();
    void appliesToleranceIntervalCommand();
    void appliesBinomialCapabilityCommand();
    void appliesOneProportionCommand();
    void appliesOnePoissonRateCommand();
    void appliesTwoPoissonRateCommand();
    void appliesOneSampleEquivalenceCommand();
    void appliesImrRsCommand();
    void appliesOutlierTestCommand();
};

void AnalysisIntentTest::collectsRoleAndTextValues()
{
    datalab::application::AnalysisIntent intent;
    intent.roles["variables"] = {2, 5};
    intent.inputs["confidence"] = "95";

    QCOMPARE(intent.first_role_index("variables"), 2);
    const std::vector<int> indices = intent.role_indices("variables");
    QCOMPARE(indices.size(), std::size_t{2});
    QCOMPARE(indices[1], 5);
    QCOMPARE(intent.line_number("confidence"), std::optional<double>(95.0));
}

void AnalysisIntentTest::commandBuilderDoesNotRequireDialog()
{
    const analysis_commands::AnalysisCommand* command =
        analysis_commands::find(QStringLiteral("one_sample_t"));
    QVERIFY(command != nullptr);

    datalab::domain::AnalysisConfiguration configuration;
    datalab::application::AnalysisIntent intent;
    intent.roles["variables"] = {3};
    intent.inputs["hypothesis_mean"] = "10";
    intent.inputs["confidence"] = "95";
    intent.inputs["alternative"] = "two_sided";

    const analysis_commands::AnalysisApplyResult result =
        command->apply(configuration, intent);
    QVERIFY(result.valid);
    QCOMPARE(configuration.selection.measurement_column, std::size_t{3});
    QCOMPARE(configuration.inference.hypothesis_mean, std::optional<double>(10.0));
}

void AnalysisIntentTest::appliesIndependentResponseObjectives()
{
    const analysis_commands::AnalysisCommand* command =
        analysis_commands::find(QStringLiteral("response_optimization"));
    QVERIFY(command != nullptr);

    datalab::domain::AnalysisConfiguration configuration;
    datalab::application::AnalysisIntent intent;
    intent.roles["response"] = {2, 3};
    intent.roles["factor_columns"] = {0, 1};
    intent.inputs["goal"] = "maximize";
    intent.inputs["weight"] = "1";
    intent.inputs["objectives"] =
        R"([{"goal":"maximize","weight":1.5},{"goal":"minimize","weight":0.5}])";

    const analysis_commands::AnalysisApplyResult result =
        command->apply(configuration, intent);
    QVERIFY(result.valid);
    QCOMPARE(configuration.doe.response_columns.size(), std::size_t{2});
    QCOMPARE(configuration.doe.optimization_objectives.size(), std::size_t{2});
    QCOMPARE(configuration.doe.optimization_objectives[0].goal, std::string{"maximize"});
    QCOMPARE(configuration.doe.optimization_objectives[0].weight, 1.5);
    QCOMPARE(configuration.doe.optimization_objectives[1].goal, std::string{"minimize"});
    QCOMPARE(configuration.doe.optimization_objectives[1].weight, 0.5);
}

void AnalysisIntentTest::fallsBackToSharedObjectiveForSingleResponse()
{
    const analysis_commands::AnalysisCommand* command =
        analysis_commands::find(QStringLiteral("response_optimization"));
    QVERIFY(command != nullptr);

    datalab::domain::AnalysisConfiguration configuration;
    datalab::application::AnalysisIntent intent;
    intent.roles["response"] = {2};
    intent.roles["factor_columns"] = {0, 1};
    intent.inputs["goal"] = "minimize";
    intent.inputs["weight"] = "2";
    intent.inputs["lower"] = "1";
    intent.inputs["upper"] = "9";

    const analysis_commands::AnalysisApplyResult result =
        command->apply(configuration, intent);
    QVERIFY(result.valid);
    QCOMPARE(configuration.doe.optimization_objectives.size(), std::size_t{1});
    QCOMPARE(configuration.doe.optimization_objectives[0].goal, std::string{"minimize"});
    QCOMPARE(configuration.doe.optimization_objectives[0].weight, 2.0);
    QCOMPARE(configuration.doe.optimization_objectives[0].lower, std::optional<double>(1.0));
    QCOMPARE(configuration.doe.optimization_objectives[0].upper, std::optional<double>(9.0));
    QCOMPARE(configuration.doe.optimization_goal, std::string{"minimize"});
}

void AnalysisIntentTest::appliesBinomialCapabilityCommand()
{
    const analysis_commands::AnalysisCommand* command =
        analysis_commands::find(QStringLiteral("binomial_capability"));
    QVERIFY(command != nullptr);

    datalab::domain::AnalysisConfiguration configuration;
    datalab::application::AnalysisIntent intent;
    intent.roles["defectives"] = {0};
    intent.roles["inspected"] = {1};
    intent.inputs["target"] = "0.02";

    const analysis_commands::AnalysisApplyResult result =
        command->apply(configuration, intent);
    QVERIFY(result.valid);
    QCOMPARE(configuration.chart_type, std::string{"binomial_capability"});
    QCOMPARE(configuration.capability_method, std::string{"binomial"});
    QCOMPARE(configuration.selection.defect_count_column, std::optional<std::size_t>(0));
    QCOMPARE(configuration.selection.inspected_count_column, std::optional<std::size_t>(1));
    QCOMPARE(configuration.specifications.target, std::optional<double>(0.02));
}

void AnalysisIntentTest::appliesToleranceIntervalCommand()
{
    const analysis_commands::AnalysisCommand* command =
        analysis_commands::find(QStringLiteral("tolerance_intervals"));
    QVERIFY(command != nullptr);

    datalab::domain::AnalysisConfiguration configuration;
    datalab::application::AnalysisIntent intent;
    intent.roles["measurement"] = {1};
    intent.inputs["coverage"] = "95";
    intent.inputs["confidence"] = "99";
    intent.inputs["alternative"] = "two_sided";

    const analysis_commands::AnalysisApplyResult result =
        command->apply(configuration, intent);
    QVERIFY(result.valid);
    QCOMPARE(configuration.chart_type, std::string{"tolerance_intervals"});
    QCOMPARE(configuration.selection.measurement_column, std::size_t{1});
    QCOMPARE(configuration.inference.coverage_proportion, std::optional<double>(0.95));
    QCOMPARE(configuration.inference.confidence_level, 0.99);
    QCOMPARE(configuration.inference.alternative, std::string{"two_sided"});
}

void AnalysisIntentTest::appliesOneProportionCommand()
{
    const analysis_commands::AnalysisCommand* command =
        analysis_commands::find(QStringLiteral("one_proportion"));
    QVERIFY(command != nullptr);

    datalab::domain::AnalysisConfiguration configuration;
    datalab::application::AnalysisIntent intent;
    intent.roles["events"] = {0};
    intent.roles["trials"] = {1};
    intent.inputs["hypothesis_mean"] = "0.05";
    intent.inputs["confidence"] = "95";
    intent.inputs["alternative"] = "two_sided";
    intent.inputs["method"] = "exact";

    const analysis_commands::AnalysisApplyResult result =
        command->apply(configuration, intent);
    QVERIFY(result.valid);
    QCOMPARE(configuration.chart_type, std::string{"one_proportion"});
    QCOMPARE(configuration.inference.first_events_column, std::optional<std::size_t>(0));
    QCOMPARE(configuration.inference.first_trials_column, std::optional<std::size_t>(1));
    QCOMPARE(configuration.inference.hypothesis_mean, std::optional<double>(0.05));
    QCOMPARE(configuration.inference.proportion_method, std::string{"exact"});
}

void AnalysisIntentTest::appliesOnePoissonRateCommand()
{
    const analysis_commands::AnalysisCommand* command =
        analysis_commands::find(QStringLiteral("one_poisson_rate"));
    QVERIFY(command != nullptr);

    datalab::domain::AnalysisConfiguration configuration;
    datalab::application::AnalysisIntent intent;
    intent.roles["defects"] = {0};
    intent.roles["length"] = {1};
    intent.inputs["hypothesis_mean"] = "0.05";
    intent.inputs["confidence"] = "95";
    intent.inputs["alternative"] = "two_sided";
    intent.inputs["method"] = "exact";

    const analysis_commands::AnalysisApplyResult result =
        command->apply(configuration, intent);
    QVERIFY(result.valid);
    QCOMPARE(configuration.chart_type, std::string{"one_poisson_rate"});
    QCOMPARE(configuration.inference.first_events_column, std::optional<std::size_t>(0));
    QCOMPARE(configuration.inference.first_trials_column, std::optional<std::size_t>(1));
    QCOMPARE(configuration.inference.hypothesis_mean, std::optional<double>(0.05));
    QCOMPARE(configuration.inference.proportion_method, std::string{"exact"});
}

void AnalysisIntentTest::appliesTwoPoissonRateCommand()
{
    const analysis_commands::AnalysisCommand* command =
        analysis_commands::find(QStringLiteral("two_poisson_rate"));
    QVERIFY(command != nullptr);

    datalab::domain::AnalysisConfiguration configuration;
    datalab::application::AnalysisIntent intent;
    intent.roles["first_events"] = {0};
    intent.roles["first_trials"] = {1};
    intent.roles["second_events"] = {2};
    intent.roles["second_trials"] = {3};
    intent.inputs["confidence"] = "95";
    intent.inputs["alternative"] = "two_sided";
    intent.inputs["method"] = "normal";

    const analysis_commands::AnalysisApplyResult result =
        command->apply(configuration, intent);
    QVERIFY(result.valid);
    QCOMPARE(configuration.chart_type, std::string{"two_poisson_rate"});
    QCOMPARE(configuration.inference.first_events_column, std::optional<std::size_t>(0));
    QCOMPARE(configuration.inference.first_trials_column, std::optional<std::size_t>(1));
    QCOMPARE(configuration.inference.second_events_column, std::optional<std::size_t>(2));
    QCOMPARE(configuration.inference.second_trials_column, std::optional<std::size_t>(3));
    QCOMPARE(configuration.inference.proportion_method, std::string{"normal"});
}

void AnalysisIntentTest::appliesOneSampleEquivalenceCommand()
{
    const analysis_commands::AnalysisCommand* command =
        analysis_commands::find(QStringLiteral("one_sample_equivalence"));
    QVERIFY(command != nullptr);

    datalab::domain::AnalysisConfiguration configuration;
    datalab::application::AnalysisIntent intent;
    intent.roles["variables"] = {2};
    intent.inputs["hypothesis_mean"] = "0";
    intent.inputs["lower"] = "-1.5";
    intent.inputs["upper"] = "1.5";
    intent.inputs["confidence"] = "95";

    const analysis_commands::AnalysisApplyResult result =
        command->apply(configuration, intent);
    QVERIFY(result.valid);
    QCOMPARE(configuration.chart_type, std::string{"one_sample_equivalence"});
    QCOMPARE(configuration.selection.measurement_column, std::size_t{2});
    QCOMPARE(configuration.inference.equivalence_lower, std::optional<double>(-1.5));
    QCOMPARE(configuration.inference.equivalence_upper, std::optional<double>(1.5));
}

void AnalysisIntentTest::appliesImrRsCommand()
{
    const analysis_commands::AnalysisCommand* command =
        analysis_commands::find(QStringLiteral("imr_rs"));
    QVERIFY(command != nullptr);

    datalab::domain::AnalysisConfiguration configuration;
    datalab::application::AnalysisIntent intent;
    intent.roles["variables"] = {0};
    intent.inputs["subgroup_size"] = "5";

    const analysis_commands::AnalysisApplyResult result =
        command->apply(configuration, intent);
    QVERIFY(result.valid);
    QCOMPARE(configuration.chart_type, std::string{"imr_rs"});
    QCOMPARE(configuration.selection.measurement_column, std::size_t{0});
    QCOMPARE(configuration.control.subgroup_size, std::optional<int>(5));
}

void AnalysisIntentTest::appliesOutlierTestCommand()
{
    const analysis_commands::AnalysisCommand* command =
        analysis_commands::find(QStringLiteral("outlier_test"));
    QVERIFY(command != nullptr);

    datalab::domain::AnalysisConfiguration configuration;
    datalab::application::AnalysisIntent intent;
    intent.roles["variables"] = {2};
    intent.inputs["confidence"] = "95";
    intent.inputs["alternative"] = "two_sided";

    const analysis_commands::AnalysisApplyResult result =
        command->apply(configuration, intent);
    QVERIFY(result.valid);
    QCOMPARE(configuration.chart_type, std::string{"outlier_test"});
    QCOMPARE(configuration.selection.measurement_column, std::size_t{2});
    QCOMPARE(configuration.inference.confidence_level, 0.95);
    QCOMPARE(configuration.inference.alternative, std::string{"two_sided"});
}

QTEST_MAIN(AnalysisIntentTest)
#include "analysis_intent_test.moc"
