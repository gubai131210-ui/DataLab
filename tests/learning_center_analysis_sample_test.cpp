#include "application/analysis_service.h"
#include "application/learning/learning_dataset_store.h"

#include <QtTest>

#include <optional>
#include <string>

using datalab::application::AnalysisService;
using datalab::application::learning::LearningDatasetStore;
using datalab::domain::AnalysisConfiguration;
using datalab::domain::DataTable;
using datalab::domain::OutputPage;

namespace {

std::optional<std::size_t> find_column(const DataTable& table, const char* name)
{
    for (std::size_t index = 0; index < table.columns.size(); ++index) {
        if (table.columns[index] == name) {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<DataTable> load_dataset(const char* dataset_id)
{
    QString error;
    return LearningDatasetStore::load_dataset(QString::fromUtf8(dataset_id), &error);
}

void assert_has_output(const OutputPage& page)
{
    QVERIFY2(!page.tables.empty() || !page.plots.empty(),
             "analysis produced no tables or plots");
}

}  // namespace

class LearningCenterAnalysisSampleTest final : public QObject {
    Q_OBJECT

private slots:
    void statisticsMenuChiSquareOnCatShiftLine();
    void controlChartMenuImrOnSpiShift();
    void graphMenuHistogramOnImrSpiShift();
    void qualityToolsMenuParetoOnDefectTail();
    void qualityToolsMenuBetweenWithinOnLaterWave();
};

void LearningCenterAnalysisSampleTest::statisticsMenuChiSquareOnCatShiftLine()
{
    const auto loaded = load_dataset("cat_shift_line");
    QVERIFY(loaded.has_value());
    const DataTable& table = *loaded;
    const auto row_cat = find_column(table, "班次");
    const auto col_cat = find_column(table, "缺陷类型");
    QVERIFY(row_cat.has_value());
    QVERIFY(col_cat.has_value());

    AnalysisConfiguration configuration;
    configuration.inference.row_category_column = *row_cat;
    configuration.inference.column_category_column = *col_cat;
    const OutputPage page = AnalysisService::chi_square(table, configuration);
    assert_has_output(page);
    QVERIFY(page.facts.chi_square.has_value());
}

void LearningCenterAnalysisSampleTest::controlChartMenuImrOnSpiShift()
{
    const auto loaded = load_dataset("imr_spi_shift");
    QVERIFY(loaded.has_value());
    const DataTable& table = *loaded;
    const auto measurement = find_column(table, "锡膏高度_um");
    QVERIFY(measurement.has_value());

    AnalysisConfiguration configuration;
    configuration.variable_columns = {*measurement};
    const OutputPage page = AnalysisService::individuals_moving_range(table, configuration);
    assert_has_output(page);
    QVERIFY(page.facts.spc.has_value());
}

void LearningCenterAnalysisSampleTest::graphMenuHistogramOnImrSpiShift()
{
    const auto loaded = load_dataset("graph_hist_prob");
    QVERIFY(loaded.has_value());
    const DataTable& table = *loaded;
    const auto measurement = find_column(table, "厚度_um");
    QVERIFY(measurement.has_value());

    AnalysisConfiguration configuration;
    configuration.variable_columns = {*measurement};
    const OutputPage page = AnalysisService::histogram(table, configuration);
    assert_has_output(page);
    QVERIFY(!page.plots.empty());
}

void LearningCenterAnalysisSampleTest::qualityToolsMenuParetoOnDefectTail()
{
    const auto loaded = load_dataset("pareto_defect_tail");
    QVERIFY(loaded.has_value());
    const DataTable& table = *loaded;
    const auto defect = find_column(table, "缺陷类别");
    QVERIFY(defect.has_value());

    AnalysisConfiguration configuration;
    configuration.chart_type = "pareto";
    configuration.variable_columns = {*defect};
    const OutputPage page = AnalysisService::pareto(table, configuration);
    assert_has_output(page);
    QVERIFY(!page.plots.empty());
}

void LearningCenterAnalysisSampleTest::qualityToolsMenuBetweenWithinOnLaterWave()
{
    const auto loaded = load_dataset("cap_between_within");
    QVERIFY(loaded.has_value());
    const DataTable& table = *loaded;
    const auto measurement = find_column(table, "厚度_um");
    const auto subgroup = find_column(table, "子组");
    QVERIFY(measurement.has_value());
    QVERIFY(subgroup.has_value());

    AnalysisConfiguration configuration;
    configuration.variable_columns = {*measurement};
    configuration.selection.measurement_column = *measurement;
    configuration.selection.subgroup_column = *subgroup;
    configuration.specifications.lower = 95.0;
    configuration.specifications.upper = 105.0;
    configuration.specifications.target = 100.0;
    configuration.capability_method = "between_within";
    const OutputPage page =
        AnalysisService::between_within_capability(table, configuration);
    assert_has_output(page);
}

QTEST_MAIN(LearningCenterAnalysisSampleTest)
#include "learning_center_analysis_sample_test.moc"
