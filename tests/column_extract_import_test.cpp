#include "domain/column_extract.h"
#include "infrastructure/csv_importer.h"

#include <QTemporaryFile>
#include <QtTest/QtTest>

class ColumnExtractImportTest final : public QObject {
    Q_OBJECT

private slots:
    void rejectsTrailingCharactersInNumericCells();
    void preservesRowsForQuotedCsvFields();
};

void ColumnExtractImportTest::rejectsTrailingCharactersInNumericCells()
{
    datalab::domain::DataTable table;
    table.columns = {"Value"};
    table.rows = {{"12abc"}, {" 13.5 "}, {"NA"}};

    const auto result = datalab::domain::extract_numeric_column(table, 0, {});

    QCOMPARE(result.values.size(), std::size_t{1});
    QCOMPARE(result.source_rows.front(), std::size_t{1});
    QCOMPARE(result.missing_count, std::size_t{2});
    QCOMPARE(result.invalid_count, std::size_t{1});
    QVERIFY(result.column_valid);
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

QTEST_APPLESS_MAIN(ColumnExtractImportTest)

#include "column_extract_import_test.moc"
