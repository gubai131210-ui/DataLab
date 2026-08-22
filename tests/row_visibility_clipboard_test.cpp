#include "ui/row_visibility_clipboard.h"

#include <QApplication>
#include <QtTest/QtTest>

class RowVisibilityClipboardTest final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void footnoteEmptyWhenNoVisibility();
    void footnoteIncludesHiddenAndExcluded();
    void composedPixmapAddsHeight();
    void tsvFootnoteUsesCommentLines();
    void cleanupTestCase();

private:
    int argc_ = 1;
    char argv0_[2] = {'t', '\0'};
    char* argv_[1] = {argv0_};
    QApplication* app_ = nullptr;
};

void RowVisibilityClipboardTest::initTestCase()
{
    app_ = new QApplication(argc_, argv_);
}

void RowVisibilityClipboardTest::cleanupTestCase()
{
    delete app_;
    app_ = nullptr;
}

void RowVisibilityClipboardTest::footnoteEmptyWhenNoVisibility()
{
    QCOMPARE(datalab::ui::row_visibility_footnote(0, 0, 0, 0), QString());
}

void RowVisibilityClipboardTest::footnoteIncludesHiddenAndExcluded()
{
    const QString footnote = datalab::ui::row_visibility_footnote(2, 3, 40, 45);
    QVERIFY(footnote.contains(QStringLiteral("排除 2")));
    QVERIFY(footnote.contains(QStringLiteral("隐藏 3")));
    QVERIFY(footnote.contains(QStringLiteral("分析 N = 40")));
    QVERIFY(footnote.contains(QStringLiteral("显示 N = 45")));
    QVERIFY(footnote.contains(QStringLiteral("hidden")));
}

void RowVisibilityClipboardTest::composedPixmapAddsHeight()
{
    QPixmap chart(320, 160);
    chart.fill(Qt::white);
    const QString footnote = datalab::ui::row_visibility_footnote(1, 0, 10, 10);
    const QPixmap composed = datalab::ui::compose_chart_pixmap_with_footnote(chart, footnote);
    QVERIFY(!composed.isNull());
    QVERIFY(composed.height() > chart.height());
}

void RowVisibilityClipboardTest::tsvFootnoteUsesCommentLines()
{
    const QString footnote = datalab::ui::row_visibility_footnote(2, 1, 20, 22);
    const QString tsv = datalab::ui::append_clipboard_footnote_comments(
        QStringLiteral("x\ty\n1\t2"), footnote);
    QVERIFY(tsv.startsWith(QStringLiteral("x\ty")));
    QVERIFY(tsv.contains(QStringLiteral("# 行可见性契约")));
    QVERIFY(tsv.contains(QStringLiteral("排除 2")));
}

QTEST_MAIN(RowVisibilityClipboardTest)
#include "row_visibility_clipboard_test.moc"
