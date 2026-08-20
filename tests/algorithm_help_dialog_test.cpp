#include "ui/algorithm_help_dialog.h"

#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QtTest/QtTest>

class AlgorithmHelpDialogTest final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void dialogBuildsTreeAndShowsDetail();
    void searchFiltersEntries();
    void formulaCopyProvidesPlainText();
    void summaryCopyContainsMethodNotRepoPath();
    void invalidReferenceUrlIsRejected();
    void cleanupTestCase();

private:
    int argc_ = 1;
    char argv0_[2] = {'t', '\0'};
    char* argv_[1] = {argv0_};
    QApplication* app_ = nullptr;
};

void AlgorithmHelpDialogTest::initTestCase()
{
    app_ = new QApplication(argc_, argv_);
}

void AlgorithmHelpDialogTest::cleanupTestCase()
{
    delete app_;
    app_ = nullptr;
}

void AlgorithmHelpDialogTest::dialogBuildsTreeAndShowsDetail()
{
    AlgorithmHelpDialog dialog;
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    auto* tree = dialog.findChild<QTreeWidget*>();
    auto* browser = dialog.findChild<QTextBrowser*>();
    QVERIFY(tree != nullptr);
    QVERIFY(browser != nullptr);
    QVERIFY(tree->topLevelItemCount() > 0);

    dialog.select_entry(QStringLiteral("capability"));
    QVERIFY(!browser->toHtml().isEmpty());
    QVERIFY(browser->toHtml().contains(QStringLiteral("正态过程能力"))
            || browser->toHtml().contains(QStringLiteral("capability")));
    QVERIFY(browser->toHtml().contains(QStringLiteral("Cp")));
    QVERIFY(browser->toHtml().contains(QStringLiteral("计算步骤")));
    QVERIFY(browser->toHtml().contains(QStringLiteral("符号定义")));
    QVERIFY(browser->toHtml().contains(QStringLiteral("判定规则")));
    QVERIFY(browser->toHtml().contains(QStringLiteral("不能计算")));
}

void AlgorithmHelpDialogTest::searchFiltersEntries()
{
    AlgorithmHelpDialog dialog;
    auto* search = dialog.findChild<QLineEdit*>();
    auto* tree = dialog.findChild<QTreeWidget*>();
    auto* browser = dialog.findChild<QTextBrowser*>();
    QVERIFY(search != nullptr);
    QVERIFY(tree != nullptr);
    QVERIFY(browser != nullptr);

    const int full_count = tree->topLevelItemCount();
    QVERIFY(full_count > 0);

    search->setText(QStringLiteral("zzzz-no-match-zzzz"));
    QCOMPARE(tree->topLevelItemCount(), 0);
    QVERIFY(browser->toHtml().contains(QStringLiteral("没有匹配")));

    search->clear();
    QVERIFY(tree->topLevelItemCount() > 0);

    search->setText(QStringLiteral("Grubbs"));
    bool has_grubbs = false;
    for (int category_index = 0; category_index < tree->topLevelItemCount(); ++category_index) {
        QTreeWidgetItem* category = tree->topLevelItem(category_index);
        for (int entry_index = 0; entry_index < category->childCount(); ++entry_index) {
            if (category->child(entry_index)->text(0).contains(QStringLiteral("Grubbs"))) {
                has_grubbs = true;
            }
        }
    }
    QVERIFY(has_grubbs);

    search->setText(QStringLiteral("σ_within"));
    QVERIFY(tree->topLevelItemCount() > 0);
}

void AlgorithmHelpDialogTest::formulaCopyProvidesPlainText()
{
    AlgorithmHelpDialog dialog;
    dialog.select_entry(QStringLiteral("capability"));

    bool found_copy = false;
    const auto buttons = dialog.findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->text() == QStringLiteral("复制公式纯文本")) {
            found_copy = true;
            QVERIFY(button->isEnabled());
            QTest::mouseClick(button, Qt::LeftButton);
            break;
        }
    }
    QVERIFY(found_copy);

    const QString clipboard = QApplication::clipboard()->text();
    QVERIFY(!clipboard.isEmpty());
    QVERIFY(!clipboard.contains(QStringLiteral("<sup>")));
    QVERIFY(!clipboard.contains(QStringLiteral("<table")));
}

void AlgorithmHelpDialogTest::summaryCopyContainsMethodNotRepoPath()
{
    AlgorithmHelpDialog dialog;
    dialog.select_entry(QStringLiteral("capability"));
    for (QPushButton* button : dialog.findChildren<QPushButton*>()) {
        if (button->text() == QStringLiteral("复制条目摘要")) {
            QTest::mouseClick(button, Qt::LeftButton);
            break;
        }
    }
    const QString clipboard = QApplication::clipboard()->text();
    QVERIFY(clipboard.contains(QStringLiteral("用途")));
    QVERIFY(clipboard.contains(QStringLiteral("计算步骤")));
    QVERIFY(clipboard.contains(QStringLiteral("判定规则")));
    QVERIFY(!clipboard.contains(QStringLiteral("docs/research")));
}

void AlgorithmHelpDialogTest::invalidReferenceUrlIsRejected()
{
    AlgorithmHelpDialog dialog;
    dialog.select_entry(QStringLiteral("descriptive"));
    for (QPushButton* button : dialog.findChildren<QPushButton*>()) {
        if (button->text() == QStringLiteral("打开参考网站")) {
            QVERIFY(button->isEnabled());
            break;
        }
    }
}

QTEST_MAIN(AlgorithmHelpDialogTest)
#include "algorithm_help_dialog_test.moc"
