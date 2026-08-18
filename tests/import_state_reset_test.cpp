#include "domain/column_extract.h"
#include "infrastructure/csv_importer.h"
#include "infrastructure/data_import_service.h"

#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest/QtTest>

class ImportStateResetTest final : public QObject {
    Q_OBJECT

private slots:
    void datasetIdChangesWhenSourceChanges();
    void secondImportProducesDistinctContract();
    void projectRoundTripPreservesDatasetIdentity();
};

void ImportStateResetTest::datasetIdChangesWhenSourceChanges()
{
    QTemporaryFile first_file;
    QVERIFY(first_file.open());
    const QByteArray first_content("A,B\n1,2\n3,4\n");
    QCOMPARE(first_file.write(first_content), first_content.size());
    QVERIFY(first_file.flush());

    QTemporaryFile second_file;
    QVERIFY(second_file.open());
    const QByteArray second_content("X,Y,Z\n10,20,30\n");
    QCOMPARE(second_file.write(second_content), second_content.size());
    QVERIFY(second_file.flush());

    QString error;
    const auto first = datalab::infrastructure::DataImportService::import_file(
        first_file.fileName(), &error);
    QVERIFY2(first.has_value(), qPrintable(error));
    const std::string first_id = first->import_metadata.dataset_id;

    const auto second = datalab::infrastructure::DataImportService::import_file(
        second_file.fileName(), &error);
    QVERIFY2(second.has_value(), qPrintable(error));
    const std::string second_id = second->import_metadata.dataset_id;

    QVERIFY(!first_id.empty());
    QVERIFY(!second_id.empty());
    QVERIFY(first_id != second_id);
    QCOMPARE(first->columns.size(), std::size_t{2});
    QCOMPARE(second->columns.size(), std::size_t{3});
}

void ImportStateResetTest::secondImportProducesDistinctContract()
{
    QTemporaryFile first_file;
    QVERIFY(first_file.open());
    const QByteArray first_content("Value\n1\n2\n");
    QCOMPARE(first_file.write(first_content), first_content.size());
    QVERIFY(first_file.flush());

    QTemporaryFile second_file;
    QVERIFY(second_file.open());
    const QByteArray second_content("Value\n9\n");
    QCOMPARE(second_file.write(second_content), second_content.size());
    QVERIFY(second_file.flush());

    QString error;
    const auto first_import = datalab::infrastructure::CsvImporter::import_file(
        first_file.fileName(), &error);
    const auto second_import = datalab::infrastructure::CsvImporter::import_file(
        second_file.fileName(), &error);
    QVERIFY(first_import.has_value());
    QVERIFY(second_import.has_value());

    datalab::domain::DataTable first = *first_import;
    datalab::domain::DataTable second = *second_import;
    datalab::domain::populate_data_table_contract(first);
    datalab::domain::populate_data_table_contract(second);
    QVERIFY(first.import_metadata.dataset_id != second.import_metadata.dataset_id);
    QVERIFY(datalab::domain::validate_data_table_contract(first).empty());
    QVERIFY(datalab::domain::validate_data_table_contract(second).empty());
    QCOMPARE(second.rows.size(), std::size_t{1});
}

void ImportStateResetTest::projectRoundTripPreservesDatasetIdentity()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray content("Part,Length\nA,10\nB,11\n");
    QCOMPARE(file.write(content), content.size());
    QVERIFY(file.flush());

    QString error;
    const auto imported = datalab::infrastructure::DataImportService::import_file(
        file.fileName(), &error);
    QVERIFY2(imported.has_value(), qPrintable(error));
    const std::string dataset_id = imported->import_metadata.dataset_id;

    datalab::domain::DataTable reloaded = *imported;
    reloaded.name = "roundtrip";
    datalab::domain::populate_data_table_contract(reloaded);
    QCOMPARE(reloaded.import_metadata.dataset_id, dataset_id);
    QCOMPARE(reloaded.import_metadata.original_row_count, imported->rows.size());
    QCOMPARE(reloaded.import_metadata.column_count, imported->columns.size());
}

QTEST_APPLESS_MAIN(ImportStateResetTest)

#include "import_state_reset_test.moc"
