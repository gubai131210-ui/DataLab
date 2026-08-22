#include "ui/database_preview_model.h"
#include "ui/report_table_model.h"
#include "ui/worksheet_model.h"
#include "ui/worksheet_sort_filter_proxy.h"

#include <QTest>

class WorksheetInteractionTest final : public QObject {
    Q_OBJECT

private slots:
    void worksheetRolesExposeRowIdAndCellState();
    void selectionTsvIncludesHeaders();
    void sortFilterProxyFiltersWithoutLosingSourceRow();
    void previewModelFetchMoreAppendsRows();
    void reportTableModelExposesKindsAndRuleIds();
};

void WorksheetInteractionTest::worksheetRolesExposeRowIdAndCellState()
{
    WorksheetModel model;
    datalab::domain::DataTable table;
    table.columns = {"a"};
    table.rows = {{"1"}, {""}};
    table.row_ids = {10, 20};
    table.column_types = {datalab::domain::ColumnType::numeric};
    table.cell_states = {
        {datalab::domain::CellState::valid},
        {datalab::domain::CellState::missing}};
    table.import_metadata.original_row_count = 2;
    table.import_metadata.column_count = 1;
    model.set_table(table);

    QCOMPARE(model.data(model.index(0, 0), datalab::ui::RowIdRole).toULongLong(), 10ull);
    QCOMPARE(model.data(model.index(1, 0), datalab::ui::CellStateRole).toInt(),
             static_cast<int>(datalab::domain::CellState::missing));
    QVERIFY(model.data(model.index(0, 0), Qt::ToolTipRole).toString().contains(QStringLiteral("RowId")));
    QCOMPARE(model.headerData(0, Qt::Vertical, Qt::DisplayRole).toString(), QStringLiteral("10"));
    QCOMPARE(model.headerData(1, Qt::Vertical, Qt::DisplayRole).toString(), QStringLiteral("20"));
}

void WorksheetInteractionTest::selectionTsvIncludesHeaders()
{
    WorksheetModel model;
    datalab::domain::DataTable table;
    table.columns = {"x", "y"};
    table.rows = {{"1", "2"}};
    table.row_ids = {1};
    table.column_types = {
        datalab::domain::ColumnType::numeric,
        datalab::domain::ColumnType::numeric};
    table.cell_states = {
        {datalab::domain::CellState::valid, datalab::domain::CellState::valid}};
    table.import_metadata.original_row_count = 1;
    table.import_metadata.column_count = 2;
    model.set_table(table);
    const QString text = model.selection_tsv(
        {model.index(0, 0), model.index(0, 1)}, true);
    QVERIFY(text.startsWith(QStringLiteral("x\ty")));
    QVERIFY(text.contains(QStringLiteral("1\t2")));
}

void WorksheetInteractionTest::sortFilterProxyFiltersWithoutLosingSourceRow()
{
    WorksheetModel model;
    datalab::domain::DataTable table;
    table.columns = {"v"};
    table.rows = {{"alpha"}, {"beta"}, {"alpha2"}};
    table.row_ids = {1, 2, 3};
    table.column_types = {datalab::domain::ColumnType::categorical};
    table.cell_states = {
        {datalab::domain::CellState::valid},
        {datalab::domain::CellState::valid},
        {datalab::domain::CellState::valid}};
    table.import_metadata.original_row_count = 3;
    table.import_metadata.column_count = 1;
    model.set_table(table);

    datalab::ui::WorksheetSortFilterProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.set_sortable_columns({0});
    proxy.set_text_filter(QStringLiteral("alpha"));
    QCOMPARE(proxy.rowCount(), 2);
    const QModelIndex proxy_index = proxy.index(0, 0);
    const QModelIndex source = proxy.mapToSource(proxy_index);
    QCOMPARE(model.data(source, datalab::ui::RowIdRole).toULongLong(), 1ull);
}

void WorksheetInteractionTest::previewModelFetchMoreAppendsRows()
{
    datalab::ui::DatabasePreviewModel model;
    datalab::domain::DataTable first;
    first.columns = {"id"};
    first.rows = {{"1"}};
    first.row_ids = {1};
    first.column_types = {datalab::domain::ColumnType::numeric};
    first.cell_states = {{datalab::domain::CellState::valid}};
    model.set_sample(first, true);

    int loads = 0;
    datalab::domain::ImportPlan plan;
    plan.provider_id = "sqlite";
    plan.object_ref.name = "t";
    plan.selected_columns = {"id"};
    plan.order_key = "id";
    model.configure_paging(
        plan,
        1,
        [&loads](const datalab::domain::ImportPlan& page_plan, std::uint64_t, std::uint64_t) {
            ++loads;
            if (!page_plan.keyset_after.has_value()
                || page_plan.keyset_after->columns.size() != 1
                || page_plan.keyset_after->columns.front() != "id") {
                return datalab::domain::DatabaseResult<datalab::domain::DataTable>::failure(
                    "bad_keyset", "expected keyset on id");
            }
            datalab::domain::DataTable page;
            page.columns = {"id"};
            const std::string next =
                std::to_string(std::stoll(page_plan.keyset_after->after_values.front()) + 1);
            page.rows = {{next}};
            page.row_ids = {static_cast<datalab::domain::RowId>(std::stoull(next))};
            page.column_types = {datalab::domain::ColumnType::numeric};
            page.cell_states = {{datalab::domain::CellState::valid}};
            if (std::stoll(next) > 3) {
                page.rows.clear();
                page.row_ids.clear();
                page.cell_states.clear();
            }
            return datalab::domain::DatabaseResult<datalab::domain::DataTable>::success(
                std::move(page));
        },
        std::vector<std::string>{"id"});
    QVERIFY(model.canFetchMore({}));
    model.fetchMore({});
    QCOMPARE(model.rowCount(), 2);
    QVERIFY(model.using_keyset_paging());
    model.fetchMore({});
    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(loads, 2);
}

void WorksheetInteractionTest::reportTableModelExposesKindsAndRuleIds()
{
    datalab::domain::StatisticTable table;
    table.title = "t";
    table.headers = {"规则ID", "值"};
    table.column_kinds = {"rule_id", "number"};
    table.rule_ids = {"beyond_control_limit"};
    table.row_ids = {7};
    table.rows = {{"beyond_control_limit", "1.2"}};
    datalab::ui::ReportTableModel model;
    model.set_table(table);
    QCOMPARE(model.data(model.index(0, 0), datalab::ui::ReportRuleIdRole).toString(),
             QStringLiteral("beyond_control_limit"));
    QCOMPARE(model.data(model.index(0, 1), datalab::ui::ReportColumnKindRole).toString(),
             QStringLiteral("number"));
    QCOMPARE(model.data(model.index(0, 0), datalab::ui::ReportRowIdRole).toULongLong(), 7ull);
}

QTEST_MAIN(WorksheetInteractionTest)

#include "worksheet_interaction_test.moc"
