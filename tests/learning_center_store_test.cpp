#include "application/learning/learning_dataset_store.h"
#include "application/learning/learning_tutorial_catalog.h"
#include "application/learning/worksheet_registry.h"
#include "ui/algorithm_help_catalog.h"
#include "ui/analysis_commands.h"

#include <QSet>
#include <QSqlDatabase>

#include <QtTest>

using datalab::application::learning::LearningDatasetStore;
using datalab::application::learning::LearningTutorialCatalog;
using datalab::application::learning::WorksheetRegistry;

class LearningCenterStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void catalogVersionMatches();
    void listsPlannedDatasets();
    void loadsImrSpiShiftRowsAndColumns();
    void loadsImrGoldTutorialFields();
    void loadsAllTutorials();
    void noConnectionLeakAfterOperations();
    void tutorialIdsMatchCommandAndHelpUnion();
    void allDatasetColumnsMatchDeclaredRowCounts();
    void importPreservesExistingWorksheet();
};

void LearningCenterStoreTest::catalogVersionMatches()
{
    QString error;
    const QString version = LearningDatasetStore::catalog_version(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(version, QString::fromLatin1(LearningDatasetStore::kExpectedCatalogVersion));
    QCOMPARE(version, QStringLiteral("learning-center-v2"));
}

void LearningCenterStoreTest::listsPlannedDatasets()
{
    QString error;
    const auto datasets = LearningDatasetStore::list_datasets(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(datasets.size() >= 2);

    QSet<QString> ids;
    for (const auto& dataset : datasets) {
        ids.insert(dataset.dataset_id);
        QVERIFY2(!dataset.dataset_id.startsWith(QStringLiteral("demo_")),
                 qPrintable(dataset.dataset_id));
    }
    QVERIFY(ids.contains(QStringLiteral("imr_spi_shift")));
    QVERIFY(ids.contains(QStringLiteral("imr_spi_spike_b")));

    const QStringList banned = {
        QStringLiteral("smt_paste_height"), QStringLiteral("two_line_thickness"),
        QStringLiteral("paired_rework"), QStringLiteral("anova_cavity"),
        QStringLiteral("corr_temp_offset"), QStringLiteral("attribute_defect"),
        QStringLiteral("gage_rr_balance"), QStringLiteral("doe_factorial_demo"),
        QStringLiteral("reliability_cycles"), QStringLiteral("ts_weekly_yield"),
    };
    for (const QString& banned_id : banned) {
        QVERIFY2(!ids.contains(banned_id), qPrintable(banned_id));
    }
}

void LearningCenterStoreTest::loadsImrSpiShiftRowsAndColumns()
{
    QString error;
    const auto table = LearningDatasetStore::load_dataset(QStringLiteral("imr_spi_shift"), &error);
    QVERIFY2(table.has_value(), qPrintable(error));
    QCOMPARE(static_cast<int>(table->columns.size()), 3);
    QVERIFY(static_cast<int>(table->rows.size()) >= 30);
    QVERIFY(static_cast<int>(table->rows.size()) <= 200);
    bool has_height = false;
    for (const auto& column : table->columns) {
        if (column == "锡膏高度_um") {
            has_height = true;
        }
    }
    QVERIFY(has_height);
    QCOMPARE(table->name, std::string("demo_imr_spi_shift"));
}

void LearningCenterStoreTest::loadsImrGoldTutorialFields()
{
    QString error;
    const auto tutorial = LearningTutorialCatalog::find_by_id(QStringLiteral("imr"), &error);
    QVERIFY2(tutorial.has_value(), qPrintable(error));
    QCOMPARE(tutorial->dataset_id.value_or(QString()), QStringLiteral("imr_spi_shift"));
    QCOMPARE(tutorial->dialog_fill.value(QStringLiteral("variables")),
             QStringLiteral("锡膏高度_um"));
    QVERIFY(!tutorial->dialog_fill.contains(QStringLiteral("stage")));
    QVERIFY(tutorial->glossary.size() >= 3);
    QVERIFY(tutorial->dialog_fill_detail.size() >= 9);
    QVERIFY(tutorial->buried_signals.size() >= 2);
    QVERIFY(!tutorial->skill_mission.isEmpty());
    QVERIFY(!tutorial->prereq_quiz.isEmpty());
    QVERIFY(!tutorial->self_explain.isEmpty());
    QVERIFY(!tutorial->fade_levels.isEmpty());
    QVERIFY(!tutorial->retrieval_quiz.isEmpty());
    QVERIFY(!tutorial->misconceptions.isEmpty());

    bool has_ucl = false;
    bool has_usl = false;
    for (const auto& item : tutorial->glossary) {
        has_ucl = has_ucl || item.term.contains(QStringLiteral("UCL"));
        has_usl = has_usl || item.term.contains(QStringLiteral("USL"));
    }
    QVERIFY(has_ucl);
    QVERIFY(has_usl);

    QSet<int> buried_rows;
    for (const auto& signal : tutorial->buried_signals) {
        buried_rows.insert(signal.row);
    }
    QVERIFY(buried_rows.contains(41));
    QVERIFY(buried_rows.contains(55));
}

void LearningCenterStoreTest::loadsAllTutorials()
{
    QString error;
    const auto tutorials = LearningTutorialCatalog::load_all(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(static_cast<int>(tutorials.size()), 184);

    int with_dataset = 0;
    for (const auto& tutorial : tutorials) {
        if (tutorial.dataset_id.has_value()) {
            ++with_dataset;
            QCOMPARE(tutorial.dataset_id.value(), QStringLiteral("imr_spi_shift"));
            QCOMPARE(tutorial.command_id, QStringLiteral("imr"));
        }
    }
    QCOMPARE(with_dataset, 1);
}

void LearningCenterStoreTest::noConnectionLeakAfterOperations()
{
    const int before = QSqlDatabase::connectionNames().size();
    QString error;
    (void)LearningDatasetStore::list_datasets(&error);
    (void)LearningDatasetStore::load_dataset(QStringLiteral("imr_spi_shift"), &error);
    (void)LearningTutorialCatalog::load_all(&error);
    const int after = QSqlDatabase::connectionNames().size();
    QCOMPARE(after, before);
}

void LearningCenterStoreTest::tutorialIdsMatchCommandAndHelpUnion()
{
    QSet<QString> union_ids;
    for (const auto& command : analysis_commands::all()) {
        union_ids.insert(command.id);
    }

    const AlgorithmHelpCatalog help_catalog =
        AlgorithmHelpCatalogLoader::load_from_resource(QStringLiteral(":/help/algorithm_help.json"));
    for (const AlgorithmHelpEntry& entry : help_catalog.entries) {
        union_ids.insert(entry.id);
    }
    QCOMPARE(union_ids.size(), 184);

    QString error;
    const auto tutorials = LearningTutorialCatalog::load_all(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    QSet<QString> tutorial_ids;
    for (const auto& tutorial : tutorials) {
        QVERIFY2(!tutorial_ids.contains(tutorial.command_id),
                 qPrintable(QStringLiteral("duplicate tutorial id: %1").arg(tutorial.command_id)));
        tutorial_ids.insert(tutorial.command_id);
    }
    QCOMPARE(tutorial_ids, union_ids);
}

void LearningCenterStoreTest::allDatasetColumnsMatchDeclaredRowCounts()
{
    QString error;
    const auto datasets = LearningDatasetStore::list_datasets(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));

    for (const auto& summary : datasets) {
        const auto table = LearningDatasetStore::load_dataset(summary.dataset_id, &error);
        QVERIFY2(table.has_value(), qPrintable(error));
        QCOMPARE(static_cast<int>(table->rows.size()), summary.row_count);
        QVERIFY(!table->columns.empty());
        for (const auto& row : table->rows) {
            QCOMPARE(static_cast<int>(row.size()), static_cast<int>(table->columns.size()));
        }
    }
}

void LearningCenterStoreTest::importPreservesExistingWorksheet()
{
    WorksheetRegistry registry;

    datalab::domain::DataTable existing;
    existing.name = "user_data.csv";
    existing.columns = {"A"};
    existing.rows = {{"1"}, {"2"}};
    registry.save_current(existing);
    QCOMPARE(registry.worksheets().size(), 1U);

    QString error;
    const auto imported = LearningDatasetStore::load_dataset(
        QStringLiteral("imr_spi_shift"), &error);
    QVERIFY2(imported.has_value(), qPrintable(error));
    registry.import_new("demo_imr_spi_shift", *imported);

    QCOMPARE(registry.worksheets().size(), 2U);
    QVERIFY(registry.worksheets().find("user_data.csv") != registry.worksheets().end());
    QVERIFY(registry.worksheets().find("demo_imr_spi_shift") != registry.worksheets().end());
    QCOMPARE(registry.active_name(), std::string("demo_imr_spi_shift"));

    const auto restored = registry.activate("user_data.csv", registry.worksheets().at("demo_imr_spi_shift"));
    QVERIFY(restored.has_value());
    QCOMPARE(restored->name, "user_data.csv");
    QCOMPARE(restored->rows.size(), 2U);
}

QTEST_MAIN(LearningCenterStoreTest)
#include "learning_center_store_test.moc"
