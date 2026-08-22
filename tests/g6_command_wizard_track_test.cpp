#include "application/command_recommendation_engine.h"
#include "ui/analysis_commands.h"
#include "ui/command_wizard_dialog.h"

#include <QtTest>

#include <QSignalSpy>
#include <QString>

#include <set>
#include <string>
#include <vector>

using datalab::application::CommandWizardIntent;
using datalab::application::RecommendResult;
using datalab::application::recommend;
using datalab::domain::ColumnType;

namespace {

bool contains_id(const RecommendResult& result, const char* id)
{
    for (const auto& item : result.recommendations) {
        if (item.command_id == id) {
            return true;
        }
    }
    return false;
}

bool all_findable(const RecommendResult& result, QString* orphan)
{
    for (const auto& item : result.recommendations) {
        const auto* command =
            analysis_commands::find(QString::fromStdString(item.command_id));
        if (command == nullptr) {
            if (orphan != nullptr) {
                *orphan = QString::fromStdString(item.command_id);
            }
            return false;
        }
    }
    return true;
}

}  // namespace

class G6CommandWizardTrackTest final : public QObject {
    Q_OBJECT

private slots:
    // G6_ENGINE T01–T15 style coverage
    void t01_oneNumericDescribe();
    void t02_oneNumericControlChart();
    void t03_oneNumericCapability();
    void t04_twoNumericCompare();
    void t05_twoNumericAssociate();
    void t06_numericPlusCategorical();
    void t07_threePlusNumericAssociate();
    void t08_categoricalOnly();
    void t09_reliabilityIntent();
    void t10_graphIntent();
    void t11_emptySelectionHint();
    void t12_allUnknownHint();
    void t13_topNCap();
    void t14_intentFiltersStrongly();
    void t15_allRecommendedIdsFindable();

    // UI smoke：可构造对话框 + openAnalysisRequested 信号
    void uiSmokeConstructAndSignal();
};

void G6CommandWizardTrackTest::t01_oneNumericDescribe()
{
    const RecommendResult result =
        recommend({ColumnType::numeric}, CommandWizardIntent::describe);
    QVERIFY(contains_id(result, "descriptive"));
    QVERIFY(contains_id(result, "histogram"));
    QVERIFY(contains_id(result, "normality_test"));
    QVERIFY(contains_id(result, "boxplot"));
    QString orphan;
    QVERIFY2(all_findable(result, &orphan), qPrintable(orphan));
}

void G6CommandWizardTrackTest::t02_oneNumericControlChart()
{
    const RecommendResult result =
        recommend({ColumnType::numeric}, CommandWizardIntent::control_chart);
    QVERIFY(contains_id(result, "imr"));
    QCOMPARE(result.recommendations.size(), static_cast<std::size_t>(1));
    QString orphan;
    QVERIFY2(all_findable(result, &orphan), qPrintable(orphan));
}

void G6CommandWizardTrackTest::t03_oneNumericCapability()
{
    const RecommendResult result =
        recommend({ColumnType::numeric}, CommandWizardIntent::capability);
    QVERIFY(contains_id(result, "capability"));
    QVERIFY(contains_id(result, "nonnormal_capability"));
    QString orphan;
    QVERIFY2(all_findable(result, &orphan), qPrintable(orphan));
}

void G6CommandWizardTrackTest::t04_twoNumericCompare()
{
    const RecommendResult result = recommend(
        {ColumnType::numeric, ColumnType::numeric}, CommandWizardIntent::compare);
    QVERIFY(contains_id(result, "two_sample_t"));
    QVERIFY(contains_id(result, "mann_whitney"));
    QVERIFY(contains_id(result, "variance_test"));
    QString orphan;
    QVERIFY2(all_findable(result, &orphan), qPrintable(orphan));
}

void G6CommandWizardTrackTest::t05_twoNumericAssociate()
{
    const RecommendResult result = recommend(
        {ColumnType::numeric, ColumnType::numeric}, CommandWizardIntent::associate);
    QVERIFY(contains_id(result, "correlation"));
    QVERIFY(contains_id(result, "regression"));
    QVERIFY(contains_id(result, "scatter_plot"));
    QString orphan;
    QVERIFY2(all_findable(result, &orphan), qPrintable(orphan));
}

void G6CommandWizardTrackTest::t06_numericPlusCategorical()
{
    const RecommendResult result = recommend(
        {ColumnType::numeric, ColumnType::categorical}, CommandWizardIntent::any);
    QVERIFY(contains_id(result, "one_way_anova"));
    QVERIFY(contains_id(result, "boxplot"));
    QVERIFY(contains_id(result, "kruskal_wallis"));
    QString orphan;
    QVERIFY2(all_findable(result, &orphan), qPrintable(orphan));
}

void G6CommandWizardTrackTest::t07_threePlusNumericAssociate()
{
    const RecommendResult result = recommend(
        {ColumnType::numeric, ColumnType::numeric, ColumnType::numeric},
        CommandWizardIntent::associate);
    QVERIFY(contains_id(result, "regression"));
    QVERIFY(contains_id(result, "pca"));
    QVERIFY(contains_id(result, "correlation"));
    QString orphan;
    QVERIFY2(all_findable(result, &orphan), qPrintable(orphan));
}

void G6CommandWizardTrackTest::t08_categoricalOnly()
{
    const RecommendResult result =
        recommend({ColumnType::categorical}, CommandWizardIntent::any);
    QVERIFY(contains_id(result, "pareto"));
    QVERIFY(contains_id(result, "chi_square"));
    QString orphan;
    QVERIFY2(all_findable(result, &orphan), qPrintable(orphan));
}

void G6CommandWizardTrackTest::t09_reliabilityIntent()
{
    const RecommendResult result =
        recommend({ColumnType::numeric}, CommandWizardIntent::reliability);
    QVERIFY(contains_id(result, "reliability"));
    QVERIFY(contains_id(result, "cox_regression"));
    QString orphan;
    QVERIFY2(all_findable(result, &orphan), qPrintable(orphan));
}

void G6CommandWizardTrackTest::t10_graphIntent()
{
    const RecommendResult one =
        recommend({ColumnType::numeric}, CommandWizardIntent::graph);
    QVERIFY(contains_id(one, "histogram"));
    QVERIFY(contains_id(one, "boxplot"));

    const RecommendResult two = recommend(
        {ColumnType::numeric, ColumnType::numeric}, CommandWizardIntent::graph);
    QVERIFY(contains_id(two, "scatter_plot"));

    const RecommendResult timed =
        recommend({ColumnType::time, ColumnType::numeric}, CommandWizardIntent::graph);
    QVERIFY(contains_id(timed, "time_series_plot"));
    QString orphan;
    QVERIFY2(all_findable(one, &orphan), qPrintable(orphan));
    QVERIFY2(all_findable(two, &orphan), qPrintable(orphan));
    QVERIFY2(all_findable(timed, &orphan), qPrintable(orphan));
}

void G6CommandWizardTrackTest::t11_emptySelectionHint()
{
    const RecommendResult result = recommend({}, CommandWizardIntent::any);
    QVERIFY(result.recommendations.empty());
    QVERIFY(result.hint_key.has_value());
    QCOMPARE(QString::fromStdString(*result.hint_key), QStringLiteral("hint.select_columns"));
}

void G6CommandWizardTrackTest::t12_allUnknownHint()
{
    const RecommendResult result =
        recommend({ColumnType::unknown, ColumnType::unknown}, CommandWizardIntent::any);
    QVERIFY(result.recommendations.empty());
    QVERIFY(result.hint_key.has_value());
    QCOMPARE(QString::fromStdString(*result.hint_key),
             QStringLiteral("hint.check_column_types"));
}

void G6CommandWizardTrackTest::t13_topNCap()
{
    // any + 2 numeric yields compare+associate；强制 top_n=2 截断。
    const RecommendResult result = recommend(
        {ColumnType::numeric, ColumnType::numeric}, CommandWizardIntent::any, 2);
    QVERIFY(result.recommendations.size() <= 2);
    QVERIFY(result.recommendations.size() <= 8);
    QString orphan;
    QVERIFY2(all_findable(result, &orphan), qPrintable(orphan));
}

void G6CommandWizardTrackTest::t14_intentFiltersStrongly()
{
    const RecommendResult result =
        recommend({ColumnType::numeric}, CommandWizardIntent::control_chart);
    for (const auto& item : result.recommendations) {
        QCOMPARE(QString::fromStdString(item.command_id), QStringLiteral("imr"));
    }
}

void G6CommandWizardTrackTest::t15_allRecommendedIdsFindable()
{
    const std::vector<std::vector<ColumnType>> cases = {
        {ColumnType::numeric},
        {ColumnType::numeric, ColumnType::numeric},
        {ColumnType::numeric, ColumnType::categorical},
        {ColumnType::numeric, ColumnType::numeric, ColumnType::numeric},
        {ColumnType::categorical},
        {ColumnType::time, ColumnType::numeric},
    };
    const CommandWizardIntent intents[] = {
        CommandWizardIntent::any,
        CommandWizardIntent::describe,
        CommandWizardIntent::compare,
        CommandWizardIntent::associate,
        CommandWizardIntent::control_chart,
        CommandWizardIntent::capability,
        CommandWizardIntent::reliability,
        CommandWizardIntent::graph,
    };
    std::set<std::string> seen;
    for (const auto& types : cases) {
        for (const CommandWizardIntent intent : intents) {
            const RecommendResult result = recommend(types, intent);
            QString orphan;
            QVERIFY2(all_findable(result, &orphan), qPrintable(orphan));
            for (const auto& item : result.recommendations) {
                seen.insert(item.command_id);
            }
        }
    }
    QVERIFY(seen.count("descriptive") > 0);
    QVERIFY(seen.count("imr") > 0);
    QVERIFY(seen.count("scatter_plot") > 0);
}

void G6CommandWizardTrackTest::uiSmokeConstructAndSignal()
{
    datalab::ui::CommandWizardDialog dialog(
        {QStringLiteral("C1"), QStringLiteral("C2")},
        {ColumnType::numeric, ColumnType::numeric});
    QSignalSpy spy(&dialog, &datalab::ui::CommandWizardDialog::openAnalysisRequested);
    QVERIFY(spy.isValid());
    // openAnalysisRequested：构造后信号可 spy；确认路径由用户交互触发（骨架不模拟点击）。
    QCOMPARE(spy.count(), 0);
    QVERIFY(dialog.objectName() == QStringLiteral("commandWizardDialog"));
}

QTEST_MAIN(G6CommandWizardTrackTest)
#include "g6_command_wizard_track_test.moc"
