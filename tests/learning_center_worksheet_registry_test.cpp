#include "application/learning/worksheet_registry.h"

#include <QtTest>

using datalab::application::learning::WorksheetRegistry;

class LearningCenterWorksheetRegistryTest final : public QObject {
    Q_OBJECT

private slots:
    void saveSkipsEmptyTable();
    void activateFallsBackToLiveTable();
    void importNewDoesNotRemoveExisting();
};

void LearningCenterWorksheetRegistryTest::saveSkipsEmptyTable()
{
    WorksheetRegistry registry;
    datalab::domain::DataTable empty;
    registry.save_current(empty);
    QCOMPARE(registry.worksheets().size(), 0U);
    QCOMPARE(registry.active_name(), std::string());
}

void LearningCenterWorksheetRegistryTest::activateFallsBackToLiveTable()
{
    WorksheetRegistry registry;
    datalab::domain::DataTable live;
    live.name = "sheet_a";
    live.columns = {"x"};
    live.rows = {{"1"}};

    const auto activated = registry.activate("sheet_a", live);
    QVERIFY(activated.has_value());
    QCOMPARE(activated->rows.size(), 1U);
    QCOMPARE(registry.worksheets().size(), 1U);
}

void LearningCenterWorksheetRegistryTest::importNewDoesNotRemoveExisting()
{
    WorksheetRegistry registry;

    datalab::domain::DataTable first;
    first.name = "first";
    first.columns = {"a"};
    first.rows = {{"1"}};
    registry.save_current(first);

    datalab::domain::DataTable second;
    second.columns = {"b"};
    second.rows = {{"2"}, {"3"}};
    registry.import_new("demo_second", second);

    QCOMPARE(registry.worksheets().size(), 2U);
    QVERIFY(registry.worksheets().find("first") != registry.worksheets().end());
    QVERIFY(registry.worksheets().find("demo_second") != registry.worksheets().end());
    QCOMPARE(registry.worksheets().at("first").rows.size(), 1U);
    QCOMPARE(registry.worksheets().at("demo_second").rows.size(), 2U);
}

QTEST_MAIN(LearningCenterWorksheetRegistryTest)
#include "learning_center_worksheet_registry_test.moc"
