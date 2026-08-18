#include "domain/quality_types.h"
#include "ui/worksheet_model.h"

#include <QtTest/QtTest>

class WorksheetModelTest final : public QObject {
    Q_OBJECT

private slots:
    void editsAndReplacesCells();
    void clearsCellsBeyondCurrentRows();
};

void WorksheetModelTest::editsAndReplacesCells()
{
    WorksheetModel model;
    datalab::domain::DataTable table;
    table.name = "worksheet";
    table.columns = {"A", "B"};
    table.rows = {{"1", "2"}, {"3", "*"}};
    model.set_table(table);

    QVERIFY(model.setData(model.index(0, 1), QStringLiteral("4")));
    QCOMPARE(model.data(model.index(0, 1), Qt::DisplayRole).toString(), QStringLiteral("4"));
    QCOMPARE(model.table().rows[0][1], std::string{"4"});

    datalab::domain::DataTable replacement = model.table();
    replacement.rows[1][0].clear();
    model.replace_table(replacement);
    QCOMPARE(model.data(model.index(1, 0), Qt::DisplayRole).toString(), QStringLiteral("*"));
}

void WorksheetModelTest::clearsCellsBeyondCurrentRows()
{
    WorksheetModel model;
    datalab::domain::DataTable table;
    table.name = "worksheet";
    table.columns = {"A"};
    table.rows = {{"value"}};
    model.set_table(table);

    QModelIndexList indexes;
    indexes.push_back(model.index(5, 2));
    indexes.push_back(model.index(5, 3));
    QVERIFY(model.clear_cells(indexes));
    QCOMPARE(model.data(model.index(5, 2), Qt::DisplayRole).toString(), QStringLiteral("*"));
    QVERIFY(model.table().rows.size() >= 6);
    QVERIFY(model.table().columns.size() >= 4);
    QCOMPARE(model.table().rows[5][2], std::string{});
}

QTEST_APPLESS_MAIN(WorksheetModelTest)

#include "worksheet_model_test.moc"
