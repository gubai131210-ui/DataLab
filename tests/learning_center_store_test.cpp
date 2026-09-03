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
    void listsTenDatasets();
    void loadsSmtDatasetRowsAndColumns();
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
}

void LearningCenterStoreTest::listsTenDatasets()
{
    QString error;
    const auto datasets = LearningDatasetStore::list_datasets(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(static_cast<int>(datasets.size()), 10);
}

void LearningCenterStoreTest::loadsSmtDatasetRowsAndColumns()
{
    QString error;
    const auto table = LearningDatasetStore::load_dataset(QStringLiteral("smt_paste_height"), &error);
    QVERIFY2(table.has_value(), qPrintable(error));
    QCOMPARE(static_cast<int>(table->columns.size()), 5);
    QCOMPARE(static_cast<int>(table->rows.size()), 80);
    QCOMPARE(table->columns.front(), "锡膏高度_um");
}

void LearningCenterStoreTest::loadsAllTutorials()
{
    QString error;
    const auto tutorials = LearningTutorialCatalog::load_all(&error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(static_cast<int>(tutorials.size()), 184);
}

void LearningCenterStoreTest::noConnectionLeakAfterOperations()
{
    const int before = QSqlDatabase::connectionNames().size();
    QString error;
    (void)LearningDatasetStore::list_datasets(&error);
    (void)LearningDatasetStore::load_dataset(QStringLiteral("paired_rework"), &error);
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
        QStringLiteral("paired_rework"), &error);
    QVERIFY2(imported.has_value(), qPrintable(error));
    registry.import_new("demo_paired_rework", *imported);

    QCOMPARE(registry.worksheets().size(), 2U);
    QVERIFY(registry.worksheets().find("user_data.csv") != registry.worksheets().end());
    QVERIFY(registry.worksheets().find("demo_paired_rework") != registry.worksheets().end());
    QCOMPARE(registry.active_name(), std::string("demo_paired_rework"));

    const auto restored = registry.activate("user_data.csv", registry.worksheets().at("demo_paired_rework"));
    QVERIFY(restored.has_value());
    QCOMPARE(restored->name, "user_data.csv");
    QCOMPARE(restored->rows.size(), 2U);
}

QTEST_MAIN(LearningCenterStoreTest)
#include "learning_center_store_test.moc"
