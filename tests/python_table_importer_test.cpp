#include "infrastructure/python_table_importer.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QtTest/QtTest>

class PythonTableImporterTest final : public QObject {
    Q_OBJECT

private slots:
    void acceptsVersionedPayload();
    void rejectsUnsupportedVersion();
};

QString create_script(const QByteArray& payload)
{
    QTemporaryFile script(QDir::tempPath() + QStringLiteral("/datalab-stub-XXXXXX.py"));
    script.setAutoRemove(false);
    if (!script.open()) {
        return {};
    }
    const QByteArray source =
        "import json,sys\n"
        "sys.stdout.write(json.dumps(" + payload + "))\n";
    script.write(source);
    script.close();
    return script.fileName();
}

void PythonTableImporterTest::acceptsVersionedPayload()
{
    const QString python = QStandardPaths::findExecutable(QStringLiteral("python"));
    if (python.isEmpty()) {
        QSKIP("Python is required for the importer seam test.");
    }
    const QString script = create_script(
        "{\"schema_version\": 1, \"name\": \"stub\", \"source_path\": \"x\", "
        "\"columns\": [\"value\"], \"rows\": [[\"1\"]]}");
    QVERIFY(!script.isEmpty());
    QString error;
    const auto table = datalab::infrastructure::PythonTableImporter::import_file(
        QStringLiteral("input.csv"), &error, python, script);
    QFile::remove(script);
    QVERIFY2(table.has_value(), qPrintable(error));
    QCOMPARE(table->name, std::string("stub"));
}

void PythonTableImporterTest::rejectsUnsupportedVersion()
{
    const QString python = QStandardPaths::findExecutable(QStringLiteral("python"));
    if (python.isEmpty()) {
        QSKIP("Python is required for the importer seam test.");
    }
    const QString script = create_script(
        "{\"schema_version\": 99, \"name\": \"stub\", \"columns\": [], \"rows\": []}");
    QVERIFY(!script.isEmpty());
    QString error;
    const auto table = datalab::infrastructure::PythonTableImporter::import_file(
        QStringLiteral("input.csv"), &error, python, script);
    QFile::remove(script);
    QVERIFY(!table.has_value());
    QVERIFY(error.contains(QStringLiteral("schema version")));
}

QTEST_MAIN(PythonTableImporterTest)
#include "python_table_importer_test.moc"
