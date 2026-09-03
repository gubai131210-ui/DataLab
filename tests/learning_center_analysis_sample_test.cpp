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
    QSKIP("Wave-0 仅金标 imr_spi_shift；cat_shift_line 待 Wave-3");
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
    const auto loaded = load_dataset("imr_spi_shift");
    QVERIFY(loaded.has_value());
    const DataTable& table = *loaded;
    const auto measurement = find_column(table, "锡膏高度_um");
    QVERIFY(measurement.has_value());

    AnalysisConfiguration configuration;
    configuration.variable_columns = {*measurement};
    const OutputPage page = AnalysisService::histogram(table, configuration);
    assert_has_output(page);
    QVERIFY(!page.plots.empty());
}

void LearningCenterAnalysisSampleTest::qualityToolsMenuParetoOnDefectTail()
{
    QSKIP("Wave-0 仅金标 imr_spi_shift；pareto_defect_tail 待 Wave-2");
}

void LearningCenterAnalysisSampleTest::qualityToolsMenuBetweenWithinOnLaterWave()
{
    QSKIP("Wave-0 仅金标 imr_spi_shift；cap_between_within 待 Wave-2");
}

QTEST_MAIN(LearningCenterAnalysisSampleTest)
#include "learning_center_analysis_sample_test.moc"
