#include "infrastructure/csv_importer.h"

#include <QtTest/QtTest>

class MinitabFixtureTest final : public QObject {
    Q_OBJECT

private slots:
    void readsOfficialFixtureRowsAndColumns();
};

void MinitabFixtureTest::readsOfficialFixtureRowsAndColumns()
{
    const struct Fixture {
        const char* file_name;
        std::size_t rows;
        std::size_t columns;
    } fixtures[] = {
        {"CrankshaftMovement.csv", 125, 2},
        {"CamshaftLength.csv", 100, 4},
        {"PistonRingDiameter.csv", 125, 1},
        {"PinLength.csv", 100, 2},
        {"UnansweredCalls.csv", 21, 2},
        {"CableWires.csv", 100, 2},
    };

    for (const Fixture& fixture : fixtures) {
        QString error;
        const QString path = QStringLiteral(
            DATALAB_SOURCE_DIR "/tests/fixtures/minitab/converted/") + fixture.file_name;
        const auto table = datalab::infrastructure::CsvImporter::import_file(path, &error);
        QVERIFY2(table.has_value(), qPrintable(error));
        QCOMPARE(table->rows.size(), fixture.rows);
        QCOMPARE(table->columns.size(), fixture.columns);
    }
}

QTEST_APPLESS_MAIN(MinitabFixtureTest)

#include "minitab_fixture_test.moc"
