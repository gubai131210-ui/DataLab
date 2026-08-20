#include "ui/command_registry.h"
#include "ui/mainwindow.h"
#include "ui/worksheet_model.h"
#include "ui/worksheet_view.h"

#include <QAction>
#include <QItemSelectionModel>
#include <QUndoStack>
#include <QtTest/QtTest>

class MainWindowEditCommandsTest final : public QObject {
    Q_OBJECT

private slots:
    void clearCellsCreatesSingleUndoEntry();
};

void MainWindowEditCommandsTest::clearCellsCreatesSingleUndoEntry()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto* model = window.findChild<WorksheetModel*>();
    auto* view = window.findChild<WorksheetView*>();
    auto* commands = window.findChild<CommandRegistry*>();
    auto* undo_stack = window.findChild<QUndoStack*>();
    QVERIFY(model != nullptr);
    QVERIFY(view != nullptr);
    QVERIFY(commands != nullptr);
    QVERIFY(undo_stack != nullptr);

    datalab::domain::DataTable table;
    table.name = "worksheet";
    table.columns = {"A"};
    table.rows = {{"value"}};
    model->set_table(table);

    const QModelIndex index = model->index(0, 0);
    QVERIFY(index.isValid());
    view->setCurrentIndex(index);
    view->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);

    QAction* clear_action = commands->get(QStringLiteral("clear_cells"));
    QVERIFY(clear_action != nullptr);
    clear_action->trigger();

    QCOMPARE(model->data(index, Qt::DisplayRole).toString(), QString());
    QCOMPARE(model->table().rows[0][0], std::string{});
    QCOMPARE(undo_stack->count(), 1);
    QVERIFY(undo_stack->canUndo());
    QCOMPARE(undo_stack->undoText(), QStringLiteral("清除单元格"));

    undo_stack->undo();
    QCOMPARE(model->data(index, Qt::DisplayRole).toString(), QStringLiteral("value"));
    QCOMPARE(model->table().rows[0][0], std::string{"value"});
}

QTEST_MAIN(MainWindowEditCommandsTest)

#include "mainwindow_edit_commands_test.moc"
