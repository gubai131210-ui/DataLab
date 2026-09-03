#include "domain/quality_types.h"
#include "ui/worksheet_model.h"

#include <QtTest/QtTest>

class WorksheetModelTest final : public QObject {
    Q_OBJECT

private slots:
    void emptyGridShowsDefaultSlackWithoutMaterializingColumns();
    void editsAndReplacesCells();
    void clearsCellsBeyondCurrentRows();
    void editingLastDefaultColumnGrowsDisplayWithoutExtraStoredColumns();
    void growAtEdgeAddsSlackUntilMinitabColumnCap();
    void removeColumnsRestoresDisplaySlack();
    void importedWideTableKeepsSlackBeyondUsedColumns();
    void tabStyleGrowAtLastEmptyColumnDoesNotStoreColumns();
};

void WorksheetModelTest::emptyGridShowsDefaultSlackWithoutMaterializingColumns()
{
    WorksheetModel model;
    QCOMPARE(model.columnCount(), WorksheetModel::kDefaultColumns);
    QCOMPARE(model.rowCount(), WorksheetModel::kDefaultRows);
    QVERIFY(model.table().columns.empty());
    QVERIFY(model.table().rows.empty());
    QCOMPARE(model.headerData(19, Qt::Horizontal, Qt::DisplayRole).toString(),
             QStringLiteral("C20\n"));
}

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
    QCOMPARE(model.data(model.index(1, 0), Qt::DisplayRole).toString(), QString());
    QCOMPARE(model.data(model.index(1, 1), Qt::DisplayRole).toString(), QStringLiteral("*"));
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
    QCOMPARE(model.data(model.index(5, 2), Qt::DisplayRole).toString(), QString());
    QVERIFY(model.table().rows.size() >= 6);
    QVERIFY(model.table().columns.size() >= 4);
    QCOMPARE(model.table().rows[5][2], std::string{});
}

void WorksheetModelTest::editingLastDefaultColumnGrowsDisplayWithoutExtraStoredColumns()
{
    WorksheetModel model;
    QVERIFY(model.setData(model.index(0, WorksheetModel::kDefaultColumns - 1),
                           QStringLiteral("edge")));
    QCOMPARE(static_cast<int>(model.table().columns.size()), WorksheetModel::kDefaultColumns);
    QVERIFY(model.columnCount() > WorksheetModel::kDefaultColumns);
    QVERIFY(model.columnCount() >= WorksheetModel::kDefaultColumns + WorksheetModel::kColumnSlack);
    QVERIFY(model.index(0, WorksheetModel::kDefaultColumns).isValid());
    QCOMPARE(model.data(model.index(0, WorksheetModel::kDefaultColumns), Qt::DisplayRole).toString(),
             QString());
}

void WorksheetModelTest::growAtEdgeAddsSlackUntilMinitabColumnCap()
{
    WorksheetModel model;
    datalab::domain::DataTable table;
    table.columns.assign(static_cast<std::size_t>(WorksheetModel::kMaxColumns - 10), "");
    model.set_table(table);
    QCOMPARE(model.columnCount(), WorksheetModel::kMaxColumns);

    QVERIFY(!model.grow_if_at_edge(0, WorksheetModel::kMaxColumns - 1));
    QCOMPARE(model.columnCount(), WorksheetModel::kMaxColumns);
    QCOMPARE(static_cast<int>(model.table().columns.size()), WorksheetModel::kMaxColumns - 10);
}

void WorksheetModelTest::removeColumnsRestoresDisplaySlack()
{
    WorksheetModel model;
    datalab::domain::DataTable table;
    table.name = "worksheet";
    table.columns = {"A", "B", "C"};
    table.rows = {{"1", "2", "3"}};
    model.set_table(table);

    QVERIFY(model.remove_columns({1}));
    QCOMPARE(model.table().columns.size(), 2);
    QVERIFY(model.columnCount() >= WorksheetModel::kDefaultColumns);
    QVERIFY(model.index(0, WorksheetModel::kDefaultColumns - 1).isValid());
}

void WorksheetModelTest::importedWideTableKeepsSlackBeyondUsedColumns()
{
    WorksheetModel model;
    datalab::domain::DataTable table;
    table.columns.assign(25, "x");
    table.rows = {std::vector<std::string>(25, "1")};
    model.set_table(table);

    QCOMPARE(static_cast<int>(model.table().columns.size()), 25);
    QCOMPARE(model.columnCount(), 25 + WorksheetModel::kColumnSlack);
    QVERIFY(model.index(0, 25).isValid());
}

void WorksheetModelTest::tabStyleGrowAtLastEmptyColumnDoesNotStoreColumns()
{
    WorksheetModel model;
    QVERIFY(model.grow_if_at_edge(0, WorksheetModel::kDefaultColumns - 1));
    QCOMPARE(model.columnCount(),
             WorksheetModel::kDefaultColumns + WorksheetModel::kColumnSlack);
    QVERIFY(model.table().columns.empty());
    QVERIFY(model.grow_if_at_edge(WorksheetModel::kDefaultRows - 1, 0));
    QCOMPARE(model.rowCount(), WorksheetModel::kDefaultRows + WorksheetModel::kRowSlack);
    QVERIFY(model.table().rows.empty());
}

QTEST_APPLESS_MAIN(WorksheetModelTest)

#include "worksheet_model_test.moc"
