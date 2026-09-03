#include "application/learning/learning_dataset_store.h"
#include "application/learning/learning_tutorial_catalog.h"

#include <QSqlDatabase>

#include <QtTest>

using datalab::application::learning::LearningDatasetStore;
using datalab::application::learning::LearningTutorialCatalog;

class LearningCenterStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void catalogVersionMatches();
    void listsTenDatasets();
    void loadsSmtDatasetRowsAndColumns();
    void loadsAllTutorials();
    void noConnectionLeakAfterOperations();
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

QTEST_MAIN(LearningCenterStoreTest)
#include "learning_center_store_test.moc"
