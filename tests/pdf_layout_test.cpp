#include "infrastructure/report_layout_cursor.h"
#include "infrastructure/pdf_report_writer.h"

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class PdfLayoutTest final : public QObject {
    Q_OBJECT

private slots:
    void wrapsLongTextAndAdvances();
    void startsNewPageWhenSpaceRunsOut();
    void writesMultiPageReportWithChart();
};

void PdfLayoutTest::wrapsLongTextAndAdvances()
{
    ReportLayoutCursor cursor(QRectF(40.0, 40.0, 500.0, 700.0), 12.0);
    const double before = cursor.y();
    const double height = cursor.measure_text(
        QStringLiteral("这是一段用于验证 PDF 中文换行和排版的长文本，不能与标题或统计表重叠。"),
        QFont(QStringLiteral("Microsoft YaHei"), 10));

    QVERIFY(height > 12.0);
    cursor.advance(height);
    QVERIFY(cursor.y() > before);
}

void PdfLayoutTest::startsNewPageWhenSpaceRunsOut()
{
    ReportLayoutCursor cursor(QRectF(40.0, 40.0, 500.0, 100.0), 12.0);
    cursor.advance(60.0);
    QVERIFY(cursor.needs_page_break(60.0));
    cursor.new_page();
    QCOMPARE(cursor.page_number(), 2);
    QVERIFY(qFuzzyCompare(cursor.y(), 40.0));
}

void PdfLayoutTest::writesMultiPageReportWithChart()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    datalab::domain::DataTable table;
    table.source_path = "D:/公司/质量项目/非常长的数据源路径/PistonRingDiameter.csv";
    table.columns = {"Diameter"};
    table.rows = {{"74.00"}, {"74.01"}, {"73.99"}};

    datalab::domain::OutputPage page;
    page.title = "I-MR 控制图";
    page.method_name = "I-MR";
    page.parameter_summary = "变量: Diameter";
    page.diagnostics.push_back(
        {datalab::domain::DiagnosticMessage::Severity::warning,
         "TEST",
         "这是一条用于验证多页 PDF 换行和布局的很长诊断信息。"});
    datalab::domain::PlotSpec plot;
    plot.kind = datalab::domain::PlotKind::control;
    plot.title = "I 图";
    plot.x_axis_title = "观测序号";
    plot.y_axis_title = "测量值";
    plot.values = {74.00, 74.01, 73.99};
    plot.center = {74.00, 74.00, 74.00};
    plot.lower = {73.95, 73.95, 73.95};
    plot.upper = {74.05, 74.05, 74.05};
    page.plots.push_back(plot);

    const QString path = directory.filePath(QStringLiteral("report.pdf"));
    QString error;
    QVERIFY(datalab::infrastructure::PdfReportWriter::write(
        path, table, std::vector<datalab::domain::OutputPage>{page}, &error));
    QVERIFY2(QFileInfo::exists(path), qPrintable(error));
    QVERIFY(QFileInfo(path).size() > 1000);
}

QTEST_MAIN(PdfLayoutTest)

#include "pdf_layout_test.moc"
