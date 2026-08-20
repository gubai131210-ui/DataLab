#include "infrastructure/data_import_service.h"
#include "infrastructure/excel_table_importer.h"

#include "domain/column_extract.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QtTest/QtTest>

class ExcelTableImporterTest final : public QObject {
    Q_OBJECT

private slots:
    void importsFirstWorksheetFromFixture();
    void importsThroughService();
    void rejectsMissingFile();
    void rejectsLegacyXlsExtension();
    void rejectsInvalidZipPayload();
    void validatesImportedContract();
};

QString fixture_path()
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("../tests/fixtures/import/basic_contract.xlsx"));
}

QString source_fixture_path()
{
#ifdef DATALAB_SOURCE_DIR
    return QDir(QStringLiteral(DATALAB_SOURCE_DIR))
        .filePath(QStringLiteral("tests/fixtures/import/basic_contract.xlsx"));
#else
    return fixture_path();
#endif
}

void ExcelTableImporterTest::importsFirstWorksheetFromFixture()
{
    const QString path = source_fixture_path();
    if (!QFile::exists(path)) {
        QSKIP("Excel fixture is unavailable.");
    }

    QString error;
    const auto table = datalab::infrastructure::ExcelTableImporter::import_file(path, &error);
    QVERIFY2(table.has_value(), qPrintable(error));
    QCOMPARE(table->import_metadata.sheet_name, std::string("数据表"));
    QCOMPARE(table->import_metadata.sheet_index, std::size_t{0});
    QCOMPARE(table->columns, std::vector<std::string>({"日期", "数值", "分组"}));
    QCOMPARE(table->rows.size(), std::size_t{3});
    QCOMPARE(table->rows[0][0], std::string("2026-01-01"));
    QCOMPARE(table->rows[0][1], std::string("1.5"));
    QCOMPARE(table->rows[0][2], std::string("A"));
    QCOMPARE(table->rows[1][0], std::string("2026-01-01"));
    QCOMPARE(table->rows[1][1], std::string(""));
    QCOMPARE(table->rows[1][2], std::string("B"));
    QCOMPARE(table->rows[2][0], std::string("inline"));
    QCOMPARE(table->rows[2][1], std::string("True"));
    QCOMPARE(table->rows[2][2], std::string(""));
}

void ExcelTableImporterTest::importsThroughService()
{
    const QString path = source_fixture_path();
    if (!QFile::exists(path)) {
        QSKIP("Excel fixture is unavailable.");
    }

    QString error;
    const auto table = datalab::infrastructure::DataImportService::import_file(path, &error);
    QVERIFY2(table.has_value(), qPrintable(error));
    QVERIFY(!table->source_path.empty());
    QVERIFY(!table->import_metadata.imported_at.empty());
}

void ExcelTableImporterTest::rejectsMissingFile()
{
    QString error;
    const auto table = datalab::infrastructure::ExcelTableImporter::import_file(
        QStringLiteral("missing_workbook.xlsx"), &error);
    QVERIFY(!table.has_value());
    QVERIFY(!error.isEmpty());
}

void ExcelTableImporterTest::rejectsLegacyXlsExtension()
{
    QString error;
    const auto table = datalab::infrastructure::DataImportService::import_file(
        QStringLiteral("legacy_workbook.xls"), &error);
    QVERIFY(!table.has_value());
    QVERIFY(error.contains(QStringLiteral(".xls")));
}

void ExcelTableImporterTest::rejectsInvalidZipPayload()
{
    QTemporaryFile file;
    file.setAutoRemove(true);
    QVERIFY(file.open());
    QCOMPARE(file.write("not-a-zip"), qint64(9));
    QVERIFY(file.flush());

    QString error;
    const auto table = datalab::infrastructure::ExcelTableImporter::import_file(
        file.fileName(), &error);
    QVERIFY(!table.has_value());
    QVERIFY(!error.isEmpty());
}

void ExcelTableImporterTest::validatesImportedContract()
{
    const QString path = source_fixture_path();
    if (!QFile::exists(path)) {
        QSKIP("Excel fixture is unavailable.");
    }

    QString error;
    const auto table = datalab::infrastructure::DataImportService::import_file(path, &error);
    QVERIFY2(table.has_value(), qPrintable(error));
    QVERIFY(datalab::domain::validate_data_table_contract(*table).empty());
    QCOMPARE(table->row_ids.size(), table->rows.size());
    QCOMPARE(table->column_types.size(), table->columns.size());
}

QTEST_MAIN(ExcelTableImporterTest)
#include "excel_table_importer_test.moc"
