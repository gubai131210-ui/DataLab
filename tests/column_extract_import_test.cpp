#include "domain/column_extract.h"
#include "infrastructure/csv_importer.h"
#include "infrastructure/data_import_service.h"

#include <QTemporaryFile>
#include <QtTest/QtTest>

class ColumnExtractImportTest final : public QObject {
    Q_OBJECT

private slots:
    void rejectsTrailingCharactersInNumericCells();
    void preservesRowsForQuotedCsvFields();
    void importsCsvThroughService();
    void reportsMissingFileThroughService();
    void distinguishesMissingAndInvalidCells();
    void populatesImportContract();
    void preservesMultilineQuotedCsvFields();
};

void ColumnExtractImportTest::rejectsTrailingCharactersInNumericCells()
{
    datalab::domain::DataTable table;
    table.columns = {"Value"};
    table.rows = {{"12abc"}, {" 13.5 "}, {"NA"}};

    const auto result = datalab::domain::extract_numeric_column(table, 0, {});

    QCOMPARE(result.values.size(), std::size_t{1});
    QCOMPARE(result.source_rows.front(), std::size_t{1});
    QCOMPARE(result.missing_count, std::size_t{1});
    QCOMPARE(result.invalid_count, std::size_t{1});
    QVERIFY(result.column_valid);
}

void ColumnExtractImportTest::distinguishesMissingAndInvalidCells()
{
    datalab::domain::DataTable table;
    table.columns = {"Value"};
    table.rows = {{"1"}, {"NA"}, {"not-a-number"}, {""}};

    datalab::domain::populate_data_table_contract(table);
    QCOMPARE(table.cell_states[0][0], datalab::domain::CellState::valid);
    QCOMPARE(table.cell_states[1][0], datalab::domain::CellState::missing);
    QCOMPARE(table.cell_states[2][0], datalab::domain::CellState::invalid);
    QCOMPARE(table.cell_states[3][0], datalab::domain::CellState::missing);

    const auto result = datalab::domain::extract_numeric_column(table, 0, {});
    QCOMPARE(result.missing_count, std::size_t{2});
    QCOMPARE(result.invalid_count, std::size_t{1});

    datalab::domain::DataTable categorical;
    categorical.columns = {"Group"};
    categorical.rows = {{"A"}, {"B"}};
    datalab::domain::populate_data_table_contract(categorical);
    QCOMPARE(categorical.column_types[0], datalab::domain::ColumnType::categorical);
    QCOMPARE(categorical.cell_states[0][0], datalab::domain::CellState::valid);
}

void ColumnExtractImportTest::populatesImportContract()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray content("Date,Value,Group\n"
                             "2026-01-01,1.5,A\n"
                             "2026-01-02,NA,B\n");
    QCOMPARE(file.write(content), content.size());
    QVERIFY(file.flush());

    QString error;
    const auto table = datalab::infrastructure::CsvImporter::import_file(
        file.fileName(), &error);
    QVERIFY2(table.has_value(), qPrintable(error));
    QCOMPARE(table->row_ids, std::vector<datalab::domain::RowId>({0, 1}));
    QCOMPARE(table->column_types[0], datalab::domain::ColumnType::time);
    QCOMPARE(table->column_types[1], datalab::domain::ColumnType::numeric);
    QCOMPARE(table->column_types[2], datalab::domain::ColumnType::categorical);
    QCOMPARE(table->import_metadata.original_row_count, std::size_t{2});
}

void ColumnExtractImportTest::preservesMultilineQuotedCsvFields()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray content("Name,Note\n"
                             "A,\"first line\n"
                             "second line\"\n"
                             "B,\"plain\"\n");
    QCOMPARE(file.write(content), content.size());
    QVERIFY(file.flush());

    QString error;
    const auto table = datalab::infrastructure::CsvImporter::import_file(
        file.fileName(), &error);
    QVERIFY2(table.has_value(), qPrintable(error));
    QCOMPARE(table->rows.size(), std::size_t{2});
    QCOMPARE(table->rows[0][1], std::string("first line\nsecond line"));
}

void ColumnExtractImportTest::preservesRowsForQuotedCsvFields()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray content("\xEF\xBB\xBFName,Value,Note\n"
                             "A,1,\"contains,comma\"\n"
                             "B,2,\"quoted \"\"text\"\"\"\n"
                             "C,3\n");
    QCOMPARE(file.write(content), content.size());
    QVERIFY(file.flush());

    QString error;
    const auto table = datalab::infrastructure::CsvImporter::import_file(
        file.fileName(), &error);

    QVERIFY2(table.has_value(), qPrintable(error));
    QCOMPARE(table->columns.size(), std::size_t{3});
    QCOMPARE(table->columns.front(), std::string("Name"));
    QCOMPARE(table->rows.size(), std::size_t{3});
    QCOMPARE(table->rows[0][2], std::string("contains,comma"));
    QCOMPARE(table->rows[1][2], std::string("quoted \"text\""));
    QCOMPARE(table->rows[2].size(), std::size_t{3});
    QVERIFY(!table->import_warnings.empty());
}

void ColumnExtractImportTest::importsCsvThroughService()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    const QByteArray content("Part,Length\n"
                             "A,1.5\n"
                             "B,2.5\n");
    QCOMPARE(file.write(content), content.size());
    QVERIFY(file.flush());

    QString error;
    const auto table = datalab::infrastructure::DataImportService::import_file(
        file.fileName(), &error);

    QVERIFY2(table.has_value(), qPrintable(error));
    QCOMPARE(table->columns.size(), std::size_t{2});
    QCOMPARE(table->rows.size(), std::size_t{2});
    QCOMPARE(table->rows[1][1], std::string("2.5"));
}

void ColumnExtractImportTest::reportsMissingFileThroughService()
{
    QString error;
    const auto table = datalab::infrastructure::DataImportService::import_file(
        QStringLiteral("nonexistent_data_file.csv"), &error);

    QVERIFY(!table.has_value());
    QVERIFY(!error.isEmpty());
}

QTEST_APPLESS_MAIN(ColumnExtractImportTest)

#include "column_extract_import_test.moc"
