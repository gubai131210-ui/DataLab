#include "ui/formula_registry_dialog.h"

#include <QLineEdit>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QtTest/QtTest>

namespace {

bool registry_tree_contains_entry(QTreeWidget* tree, const QString& entry_id)
{
    if (tree == nullptr) {
        return false;
    }
    for (int category_index = 0; category_index < tree->topLevelItemCount(); ++category_index) {
        QTreeWidgetItem* category_item = tree->topLevelItem(category_index);
        for (int entry_index = 0; entry_index < category_item->childCount(); ++entry_index) {
            if (category_item->child(entry_index)->data(0, Qt::UserRole).toString() == entry_id) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace

class FormulaRegistryDialogTest final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void dialogBuildsTreeAndShowsFormulaDetail();
    void searchFiltersById();
    void searchFiltersByReferenceUrl();
    void searchFiltersByResearchMdPath();
    void searchFiltersByFormulaSymbol();
    void cleanupTestCase();

private:
    int argc_ = 1;
    char argv0_[2] = {'t', '\0'};
    char* argv_[1] = {argv0_};
    QApplication* app_ = nullptr;
};

void FormulaRegistryDialogTest::initTestCase()
{
    app_ = new QApplication(argc_, argv_);
}

void FormulaRegistryDialogTest::cleanupTestCase()
{
    delete app_;
    app_ = nullptr;
}

void FormulaRegistryDialogTest::dialogBuildsTreeAndShowsFormulaDetail()
{
    FormulaRegistryDialog dialog;
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));

    auto* tree = dialog.findChild<QTreeWidget*>();
    auto* browser = dialog.findChild<QTextBrowser*>(QStringLiteral("formulaRegistryDetailBrowser"));
    QVERIFY(tree != nullptr);
    QVERIFY(browser != nullptr);
    QVERIFY(tree->topLevelItemCount() > 0);

    dialog.select_entry(QStringLiteral("capability"));
    const QString html = browser->toHtml();
    QVERIFY(!html.isEmpty());
    QVERIFY(html.contains(QStringLiteral("capability")));
    QVERIFY(html.contains(QStringLiteral("formula_reference"))
            || html.contains(QStringLiteral("Cp"))
            || html.contains(QStringLiteral("公式")));
}

void FormulaRegistryDialogTest::searchFiltersById()
{
    FormulaRegistryDialog dialog;
    auto* search = dialog.findChild<QLineEdit*>();
    auto* tree = dialog.findChild<QTreeWidget*>();
    QVERIFY(search != nullptr);
    QVERIFY(tree != nullptr);

    const int full_count = tree->topLevelItemCount();
    search->setText(QStringLiteral("zzzz_no_such_command"));
    QCOMPARE(tree->topLevelItemCount(), 0);

    search->clear();
    QVERIFY(tree->topLevelItemCount() >= full_count);
}

void FormulaRegistryDialogTest::searchFiltersByReferenceUrl()
{
    FormulaRegistryDialog dialog;
    auto* search = dialog.findChild<QLineEdit*>();
    auto* tree = dialog.findChild<QTreeWidget*>();
    QVERIFY(search != nullptr);
    QVERIFY(tree != nullptr);

    search->setText(QStringLiteral("minitab.com/en-us/minitab/help-and-how-to/quality-and-process-improvement/capability-analysis"));
    QVERIFY(registry_tree_contains_entry(tree, QStringLiteral("capability")));
}

void FormulaRegistryDialogTest::searchFiltersByResearchMdPath()
{
    FormulaRegistryDialog dialog;
    auto* search = dialog.findChild<QLineEdit*>();
    auto* tree = dialog.findChild<QTreeWidget*>();
    QVERIFY(search != nullptr);
    QVERIFY(tree != nullptr);

    search->setText(QStringLiteral("docs/statistical-methodology.md"));
    QVERIFY(registry_tree_contains_entry(tree, QStringLiteral("capability")));
}

void FormulaRegistryDialogTest::searchFiltersByFormulaSymbol()
{
    FormulaRegistryDialog dialog;
    auto* search = dialog.findChild<QLineEdit*>();
    auto* tree = dialog.findChild<QTreeWidget*>();
    QVERIFY(search != nullptr);
    QVERIFY(tree != nullptr);

    search->setText(QStringLiteral("Cpk"));
    QVERIFY(registry_tree_contains_entry(tree, QStringLiteral("capability")));
}

QTEST_MAIN(FormulaRegistryDialogTest)
#include "formula_registry_dialog_test.moc"
