#include "application/mes/mes_table_adapter.h"

#include <QtTest>

using datalab::application::MesTableImportRequest;
using datalab::application::materialize_mes_batch_table;

class MesTableAdapterTest final : public QObject {
    Q_OBJECT

private slots:
    void materializesBatchRowsWithContract();
    void rejectsEmptyHeaders();
    void slimWideTestValueQueryKeepsValueColumnsOnly();
    void slimWideTimeRangeKeepsRouteTimeAndTwoValues();
    void slimLongFormatDropsStatusColumns();
    void slimLongTestValueQueryKeepsRouteTime();
    void getSnInfoImportNotSlimmed();
    void slimDisabledKeepsAllColumns();
};

void MesTableAdapterTest::materializesBatchRowsWithContract() {
    MesTableImportRequest request;
    request.headers = {"输入SN", "测试参数值"};
    request.rows = {{"SN001", "1.23"}};
    request.api_name = "TestValueQuery";

    const auto result = materialize_mes_batch_table(request);
    QVERIFY(result.table.has_value());
    QCOMPARE(result.table->columns.size(), 3U);
    QCOMPARE(result.table->columns[0], "序号");
    QCOMPARE(result.table->columns[1], "输入SN");
    QCOMPARE(result.table->columns[2], "测试参数值");
    QCOMPARE(result.table->rows.size(), 1U);
    QCOMPARE(result.table->rows[0][0], "1");
    QCOMPARE(result.table->import_metadata.provider_id, "mes_batch");
    QCOMPARE(result.table->import_metadata.source_object, "TestValueQuery");
    QCOMPARE(result.table->import_metadata.filter_summary,
             std::string("TestValueQuery; slim_test_value_v1"));
    QVERIFY(result.table->row_ids.size() == 1U);
    QVERIFY(result.table->column_types.size() == 3U);
}

void MesTableAdapterTest::rejectsEmptyHeaders() {
    MesTableImportRequest request;
    request.rows = {{"SN001"}};
    request.api_name = "GetSnInfo";

    const auto result = materialize_mes_batch_table(request);
    QVERIFY(!result.table.has_value());
    QVERIFY(!result.error_message.empty());
}

void MesTableAdapterTest::slimWideTestValueQueryKeepsValueColumnsOnly() {
    MesTableImportRequest request;
    request.api_name = "TestValueQuery";
    request.headers = {
        "输入SN", "ItemA", "ItemA_上限", "ItemA_下限", "ItemA_状态"};
    request.rows = {{"SN001", "1.1", "2.0", "0.5", "成功"}};

    const auto result = materialize_mes_batch_table(request);
    QVERIFY(result.table.has_value());
    QCOMPARE(result.table->columns.size(), 3U);
    QCOMPARE(result.table->columns[0], "序号");
    QCOMPARE(result.table->columns[1], "输入SN");
    QCOMPARE(result.table->columns[2], "ItemA");
    QCOMPARE(result.table->rows[0][0], "1");
    QCOMPARE(result.table->rows[0][1], "SN001");
    QCOMPARE(result.table->rows[0][2], "1.1");
}

void MesTableAdapterTest::slimWideTimeRangeKeepsRouteTimeAndTwoValues() {
    MesTableImportRequest request;
    request.api_name = "TimeRangeTestValueQuery";
    request.headers = {
        "输入SN", "过站时间",
        "A/A", "A/A_上限", "A/A_下限", "A/A_状态",
        "B/B", "B/B_上限", "B/B_下限", "B/B_状态"};
    request.rows = {{
        "SN001", "2026-01-01 10:00:00",
        "1.0", "2.0", "0.0", "成功",
        "3.3", "4.0", "2.0", "成功"}};

    const auto result = materialize_mes_batch_table(request);
    QVERIFY(result.table.has_value());
    QCOMPARE(result.table->columns.size(), 5U);
    QCOMPARE(result.table->columns[0], "序号");
    QCOMPARE(result.table->columns[1], "输入SN");
    QCOMPARE(result.table->columns[2], "过站时间");
    QCOMPARE(result.table->columns[3], "A/A");
    QCOMPARE(result.table->columns[4], "B/B");
    QCOMPARE(result.table->rows[0][2], "2026-01-01 10:00:00");
    QCOMPARE(result.table->rows[0][3], "1.0");
    QCOMPARE(result.table->rows[0][4], "3.3");
}

void MesTableAdapterTest::slimLongFormatDropsStatusColumns() {
    MesTableImportRequest request;
    request.api_name = "TimeRangeTestValueQuery";
    request.headers = {
        "输入SN", "过站时间", "TestItem/TestSpec", "测试参数值",
        "上限", "下限", "查询状态", "错误信息"};
    request.rows = {{
        "SN001", "2026-01-01", "Voltage/High", "3.3",
        "5.0", "1.0", "成功", ""}};

    const auto result = materialize_mes_batch_table(request);
    QVERIFY(result.table.has_value());
    QCOMPARE(result.table->columns.size(), 5U);
    QCOMPARE(result.table->columns[0], "序号");
    QCOMPARE(result.table->columns[1], "输入SN");
    QCOMPARE(result.table->columns[2], "过站时间");
    QCOMPARE(result.table->columns[3], "TestItem/TestSpec");
    QCOMPARE(result.table->columns[4], "测试参数值");
    QCOMPARE(result.table->rows[0][4], "3.3");
}

void MesTableAdapterTest::slimLongTestValueQueryKeepsRouteTime() {
    MesTableImportRequest request;
    request.api_name = "TestValueQuery";
    request.headers = {
        "输入SN", "过站时间", "TestItem/TestSpec", "测试参数值",
        "上限", "下限", "查询状态", "错误信息"};
    request.rows = {{
        "SN001", "2026-01-01 10:00:00", "Voltage/High", "3.3",
        "5.0", "1.0", "成功", ""}};

    const auto result = materialize_mes_batch_table(request);
    QVERIFY(result.table.has_value());
    QCOMPARE(result.table->columns.size(), 5U);
    QCOMPARE(result.table->columns[2], "过站时间");
    QCOMPARE(result.table->rows[0][2], "2026-01-01 10:00:00");
    QCOMPARE(result.table->rows[0][4], "3.3");
}

void MesTableAdapterTest::getSnInfoImportNotSlimmed() {
    MesTableImportRequest request;
    request.api_name = "GetSnInfo";
    request.headers = {"输入内容", "SN", "查询状态", "错误信息"};
    request.rows = {{"IN001", "SN001", "成功", ""}};

    const auto result = materialize_mes_batch_table(request);
    QVERIFY(result.table.has_value());
    QCOMPARE(result.table->columns.size(), 4U);
    QCOMPARE(result.table->import_metadata.filter_summary, "GetSnInfo");
}

void MesTableAdapterTest::slimDisabledKeepsAllColumns() {
    MesTableImportRequest request;
    request.api_name = "TestValueQuery";
    request.slim_test_value_import = false;
    request.headers = {
        "输入SN", "ItemA", "ItemA_上限", "ItemA_下限", "ItemA_状态"};
    request.rows = {{"SN001", "1.1", "2.0", "0.5", "成功"}};

    const auto result = materialize_mes_batch_table(request);
    QVERIFY(result.table.has_value());
    QCOMPARE(result.table->columns.size(), 5U);
    QCOMPARE(result.table->import_metadata.filter_summary, "TestValueQuery");
}

QTEST_MAIN(MesTableAdapterTest)
#include "mes_table_adapter_test.moc"
