#include "application/analysis_intent.h"
#include "domain/quality_types.h"
#include "ui/analysis_commands.h"

#include <QtTest/QtTest>

class AnalysisIntentTest final : public QObject {
    Q_OBJECT

private slots:
    void collectsRoleAndTextValues();
    void commandBuilderDoesNotRequireDialog();
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

QTEST_MAIN(AnalysisIntentTest)
#include "analysis_intent_test.moc"
