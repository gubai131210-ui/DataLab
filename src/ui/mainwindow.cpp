#include "mainwindow.h"

#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "infrastructure/data_import_service.h"
#include "infrastructure/pdf_report_writer.h"
#include "infrastructure/project_repository.h"
#include "ui/analysis_commands.h"
#include "ui/analysis_setup_dialog.h"
#include "ui/command_registry.h"
#include "ui/output_workspace.h"
#include "ui/project_navigator.h"
#include "ui/worksheet_model.h"
#include "ui/worksheet_view.h"
#include "ui/report_preview_dialog.h"

#include <QAction>
#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDockWidget>
#include <QEvent>
#include <QFileDialog>
#include <QFile>
#include <QHash>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QToolBar>
#include <QUndoCommand>
#include <QUndoStack>
#include <QVBoxLayout>

#include <algorithm>

namespace {

class TableChangeCommand final : public QUndoCommand {
public:
    TableChangeCommand(
        WorksheetModel* model,
        const datalab::domain::DataTable& before,
        const datalab::domain::DataTable& after,
        const QString& text,
        bool already_applied)
        : model_(model)
        , before_(before)
        , after_(after)
        , already_applied_(already_applied)
    {
        setText(text);
    }

    void undo() override
    {
        model_->replace_table(before_);
    }

    void redo() override
    {
        // QUndoStack::push() calls redo() immediately. If the model already has
        // the new data (cell edit), skip the first redo to avoid beginResetModel
        // wiping the editor / selection mid-update.
        if (already_applied_) {
            already_applied_ = false;
            return;
        }
        model_->replace_table(after_);
    }

private:
    WorksheetModel* model_;
    datalab::domain::DataTable before_;
    datalab::domain::DataTable after_;
    bool already_applied_ = false;
};

}  // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("DataLab 品质工作站"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/app-mark.svg")));
    resize(1440, 900);
    setMinimumSize(1080, 680);
    setStyleSheet(QStringLiteral(
        "QMainWindow { background: #f4f7f9; color: #29434e; }"
        "QMenuBar { background: #ffffff; color: #29434e; spacing: 6px; padding: 4px 8px; }"
        "QMenuBar::item { padding: 6px 10px; border-radius: 4px; }"
        "QMenuBar::item:selected { background: #e3f3f4; color: #147d85; }"
        "QMenu { background: #ffffff; color: #29434e; border: 1px solid #d6e1e5; padding: 5px; }"
        "QMenu::item { padding: 7px 28px 7px 12px; border-radius: 4px; }"
        "QMenu::item:selected { background: #e3f3f4; color: #147d85; }"
        "QToolBar { background: #ffffff; border: 0; border-bottom: 1px solid #d6e1e5; spacing: 5px; padding: 6px 10px; }"
        "QToolButton { color: #49636d; border: 0; border-radius: 5px; padding: 6px; }"
        "QToolButton:hover { background: #eaf6f6; color: #147d85; }"
        "QToolButton:pressed { background: #d8eeee; }"
        "QDockWidget { color: #29434e; font-weight: 600; }"
        "QDockWidget::title { background: #e7f0f3; padding: 9px 12px; border-bottom: 1px solid #d6e1e5; }"
        "QLineEdit { background: #ffffff; border: 1px solid #cbd9de; border-radius: 5px; padding: 6px 9px; color: #29434e; }"
        "QLineEdit:focus { border: 1px solid #42aeb4; }"
        "QStatusBar { background: #e7f0f3; color: #49636d; border-top: 1px solid #d6e1e5; }"
        "QSplitter::handle { background: #d6e1e5; }"));
    commands_ = new CommandRegistry(this);
    undo_stack_ = new QUndoStack(this);
    create_commands();
    create_layout();
    statusBar()->showMessage(QStringLiteral("就绪。请导入数据。"));
}

MainWindow::~MainWindow() = default;

void MainWindow::create_commands()
{
    auto connect_action = [this](const QString& id, const QString& text, void (MainWindow::*slot)(), bool enabled = true) {
        QAction* action = commands_->add(id, text, enabled);
        connect(action, &QAction::triggered, this, slot);
        return action;
    };
    const auto set_icon = [this](const QString& id, const QString& file) {
        if (QAction* action = commands_->get(id)) {
            const QString resource = QStringLiteral(":/icons/%1.svg").arg(file);
            action->setIcon(QIcon(resource));
            action->setIconVisibleInMenu(true);
            action->setToolTip(action->text());
            action->setStatusTip(action->text());
            Q_ASSERT(QFile::exists(resource));
            Q_ASSERT(!action->icon().isNull());
        }
    };

    connect_action(QStringLiteral("new"), QStringLiteral("新建项目"), &MainWindow::new_project);
    connect_action(QStringLiteral("open"), QStringLiteral("打开项目"), &MainWindow::open_project);
    connect_action(QStringLiteral("save"), QStringLiteral("保存项目"), &MainWindow::save_project);
    connect_action(QStringLiteral("export_pdf"), QStringLiteral("导出 PDF"), &MainWindow::export_pdf);
    commands_->add(QStringLiteral("exit"), QStringLiteral("退出"));
    connect(commands_->get(QStringLiteral("exit")), &QAction::triggered, this, &QWidget::close);

    QAction* undo_action = commands_->add(QStringLiteral("undo"), QStringLiteral("撤销"), false);
    QAction* redo_action = commands_->add(QStringLiteral("redo"), QStringLiteral("重做"), false);
    connect(undo_action, &QAction::triggered, undo_stack_, &QUndoStack::undo);
    connect(redo_action, &QAction::triggered, undo_stack_, &QUndoStack::redo);
    connect(undo_stack_, &QUndoStack::canUndoChanged, undo_action, &QAction::setEnabled);
    connect(undo_stack_, &QUndoStack::canRedoChanged, redo_action, &QAction::setEnabled);

    connect_action(QStringLiteral("import"), QStringLiteral("导入数据"), &MainWindow::import_data);
    QAction* copy_action = commands_->add(QStringLiteral("copy"), QStringLiteral("复制"));
    copy_action->setShortcut(QKeySequence::Copy);
    copy_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(copy_action, &QAction::triggered, this, &MainWindow::copy_selection);
    QAction* cut_action = commands_->add(QStringLiteral("cut"), QStringLiteral("剪切"));
    cut_action->setShortcut(QKeySequence::Cut);
    cut_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(cut_action, &QAction::triggered, this, &MainWindow::cut_selection);
    QAction* clear_action = commands_->add(QStringLiteral("clear_cells"), QStringLiteral("清除单元格"));
    clear_action->setShortcut(QKeySequence::Delete);
    clear_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(clear_action, &QAction::triggered, this, &MainWindow::clear_selection);
    QAction* paste_action = commands_->add(QStringLiteral("paste"), QStringLiteral("粘贴"));
    paste_action->setShortcut(QKeySequence::Paste);
    paste_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(paste_action, &QAction::triggered, this, &MainWindow::paste_clipboard);
    connect_action(QStringLiteral("exclude"), QStringLiteral("排除选中行"), &MainWindow::exclude_selected_row);
    connect_action(QStringLiteral("clear_exclude"), QStringLiteral("清除排除标记"), &MainWindow::clear_exclusions);

    set_icon(QStringLiteral("new"), QStringLiteral("new"));
    set_icon(QStringLiteral("open"), QStringLiteral("open"));
    set_icon(QStringLiteral("save"), QStringLiteral("save"));
    set_icon(QStringLiteral("import"), QStringLiteral("import-data"));
    set_icon(QStringLiteral("undo"), QStringLiteral("undo"));
    set_icon(QStringLiteral("redo"), QStringLiteral("redo"));
    set_icon(QStringLiteral("export_pdf"), QStringLiteral("export-pdf"));
    // 分析命令：注册动作 + 连接通用 run_from_spec + 由命令表提供图标。
    for (const auto& command : analysis_commands::all()) {
        QAction* action = commands_->add(command.id, command.menu_label);
        connect(action, &QAction::triggered, this, [this, id = command.id] {
            run_from_spec(id);
        });
        set_icon(command.id, command.icon_file);
    }

    commands_->add(QStringLiteral("msa"), QStringLiteral("测量系统分析（后续版本）"), false);
    commands_->add(QStringLiteral("doe"), QStringLiteral("试验设计（后续版本）"), false);

    auto* file_menu = menuBar()->addMenu(QStringLiteral("文件"));
    commands_->add_to_menu(file_menu, QStringLiteral("new"));
    commands_->add_to_menu(file_menu, QStringLiteral("open"));
    commands_->add_to_menu(file_menu, QStringLiteral("save"));
    file_menu->addSeparator();
    commands_->add_to_menu(file_menu, QStringLiteral("export_pdf"));
    file_menu->addSeparator();
    commands_->add_to_menu(file_menu, QStringLiteral("exit"));

    auto* edit_menu = menuBar()->addMenu(QStringLiteral("编辑"));
    commands_->add_to_menu(edit_menu, QStringLiteral("undo"));
    commands_->add_to_menu(edit_menu, QStringLiteral("redo"));
    edit_menu->addSeparator();
    commands_->add_to_menu(edit_menu, QStringLiteral("copy"));
    commands_->add_to_menu(edit_menu, QStringLiteral("cut"));
    commands_->add_to_menu(edit_menu, QStringLiteral("paste"));
    commands_->add_to_menu(edit_menu, QStringLiteral("clear_cells"));

    auto* data_menu = menuBar()->addMenu(QStringLiteral("数据"));
    commands_->add_to_menu(data_menu, QStringLiteral("import"));
    commands_->add_to_menu(data_menu, QStringLiteral("exclude"));
    commands_->add_to_menu(data_menu, QStringLiteral("clear_exclude"));

    // 分析菜单：按命令表 menu_path 分组生成（表顺序即菜单项顺序，
    // separator_before 重现原手工菜单的分隔线）。
    QHash<QString, QMenu*> analysis_menus;
    for (const auto& command : analysis_commands::all()) {
        QMenu* menu = analysis_menus.value(command.menu_path, nullptr);
        if (menu == nullptr) {
            menu = menuBar()->addMenu(command.menu_path);
            analysis_menus.insert(command.menu_path, menu);
        }
        if (command.separator_before) {
            menu->addSeparator();
        }
        commands_->add_to_menu(menu, command.id);
    }
    commands_->add_to_menu(
        analysis_menus.value(QStringLiteral("质量工具")), QStringLiteral("doe"));

    auto* view_menu = menuBar()->addMenu(QStringLiteral("查看"));
    auto* navigator_action = view_menu->addAction(QStringLiteral("项目导航器"));
    connect(navigator_action, &QAction::triggered, this, [this] {
        if (navigator_dock_ != nullptr) {
            navigator_dock_->setVisible(!navigator_dock_->isVisible());
        }
    });

    auto* help_menu = menuBar()->addMenu(QStringLiteral("帮助"));
    help_menu->addAction(QStringLiteral("关于 DataLab"), this, [this] {
        QMessageBox::about(
            this,
            QStringLiteral("关于 DataLab"),
            QStringLiteral("DataLab 品质工作站：按 Minitab 方式选择数据、计算并输出质量分析报告。"));
    });
    help_menu->addAction(QStringLiteral("统计口径"), this, [this] {
        QMessageBox::information(
            this,
            QStringLiteral("统计口径"),
            QStringLiteral("I-MR 的组内 σ 使用 MR̄/d2；过程能力 Cp/Cpk 使用同一 within σ，Pp/Ppk 使用样本标准差。详见 docs/statistical-methodology.md。"));
    });

    auto* toolbar = addToolBar(QStringLiteral("标准工具栏"));
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(20, 20));
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->setToolTip(QStringLiteral("常用操作"));
    toolbar->addAction(commands_->get(QStringLiteral("import")));
    toolbar->addAction(commands_->get(QStringLiteral("save")));
    toolbar->addSeparator();
    toolbar->addAction(commands_->get(QStringLiteral("undo")));
    toolbar->addAction(commands_->get(QStringLiteral("redo")));
    toolbar->addSeparator();
    toolbar->addAction(commands_->get(QStringLiteral("descriptive")));
    toolbar->addAction(commands_->get(QStringLiteral("imr")));
    toolbar->addAction(commands_->get(QStringLiteral("xbar_s")));
    toolbar->addAction(commands_->get(QStringLiteral("capability")));
    toolbar->addSeparator();
    toolbar->addAction(commands_->get(QStringLiteral("export_pdf")));
}

void MainWindow::create_layout()
{
    navigator_dock_ = new QDockWidget(QStringLiteral("项目导航器"), this);
    navigator_dock_->setAllowedAreas(Qt::LeftDockWidgetArea);
    navigator_dock_->setMinimumWidth(220);
    navigator_dock_->setMaximumWidth(300);
    navigator_ = new ProjectNavigator(navigator_dock_);
    navigator_dock_->setWidget(navigator_);
    addDockWidget(Qt::LeftDockWidgetArea, navigator_dock_);
    connect(navigator_, &ProjectNavigator::analysis_activated, this, [this](const QString& id) {
        if (output_workspace_ != nullptr) {
            output_workspace_->show_page(id);
        }
    });

    context_dock_ = new QDockWidget(QStringLiteral("上下文信息"), this);
    context_dock_->setAllowedAreas(Qt::RightDockWidgetArea);
    context_dock_->setMinimumWidth(220);
    context_dock_->setMaximumWidth(300);
    auto* context = new QWidget(context_dock_);
    auto* context_layout = new QVBoxLayout(context);
    context_layout->setContentsMargins(14, 14, 14, 14);
    context_layout->setSpacing(10);
    auto* context_heading = new QLabel(QStringLiteral("数据集状态"), context);
    context_heading->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 600; color: #29434e;"));
    auto* context_status = new QLabel(QStringLiteral("尚未导入数据"), context);
    context_status->setWordWrap(true);
    context_status->setStyleSheet(QStringLiteral(
        "background: #e8f6f2; color: #18794e; padding: 10px; border-radius: 6px;"));
    auto* context_detail = new QLabel(
        QStringLiteral("导入 CSV 或 Excel 后，这里显示当前工作表的行数、列数和排除状态。"),
        context);
    context_detail->setWordWrap(true);
    context_detail->setStyleSheet(QStringLiteral("color: #647b84; line-height: 1.4;"));
    context_layout->addWidget(context_heading);
    context_layout->addWidget(context_status);
    context_layout->addWidget(context_detail);
    context_layout->addStretch(1);
    context_dock_->setWidget(context);
    addDockWidget(Qt::RightDockWidgetArea, context_dock_);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(12, 10, 12, 12);
    layout->setSpacing(8);
    worksheet_model_ = new WorksheetModel(this);
    connect(worksheet_model_, &WorksheetModel::table_changed, this,
            [this, context_status](const datalab::domain::DataTable& table) {
                table_ = table;
                context_status->setText(QStringLiteral("已准备分析\n%1 行 · %2 列")
                    .arg(static_cast<qulonglong>(table.rows.size()))
                    .arg(static_cast<qulonglong>(table.columns.size())));
            });
    connect(worksheet_model_, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex&, const QModelIndex&, const QList<int>&) {
                const datalab::domain::DataTable before = table_;
                const datalab::domain::DataTable after = worksheet_model_->table();
                push_table_change(before, after, QStringLiteral("编辑单元格"), true);
            });
    connect(worksheet_model_, &QAbstractItemModel::headerDataChanged, this,
            [this](Qt::Orientation, int, int) {
                const datalab::domain::DataTable before = table_;
                const datalab::domain::DataTable after = worksheet_model_->table();
                push_table_change(before, after, QStringLiteral("编辑列名"), true);
            });
    auto* formula_layout = new QHBoxLayout();
    auto* cell_name = new QLabel(QStringLiteral("单元格"), central);
    cell_name->setMinimumWidth(52);
    formula_bar_ = new QLineEdit(central);
    formula_bar_->setPlaceholderText(QStringLiteral("选择单元格后在此编辑"));
    formula_layout->addWidget(cell_name);
    formula_layout->addWidget(formula_bar_, 1);
    layout->addLayout(formula_layout);

    data_table_ = new WorksheetView(central);
    data_table_->setModel(worksheet_model_);
    data_table_->setEditTriggers(
        QAbstractItemView::DoubleClicked
        | QAbstractItemView::EditKeyPressed
        | QAbstractItemView::AnyKeyPressed);
    data_table_->installEventFilter(this);
    data_table_->setContextMenuPolicy(Qt::ActionsContextMenu);
    data_table_->addAction(commands_->get(QStringLiteral("copy")));
    data_table_->addAction(commands_->get(QStringLiteral("cut")));
    data_table_->addAction(commands_->get(QStringLiteral("paste")));
    data_table_->addAction(commands_->get(QStringLiteral("clear_cells")));
    connect(static_cast<WorksheetView*>(data_table_), &WorksheetView::active_cell_changed,
            this, [this](const QModelIndex& index) {
                if (formula_bar_ != nullptr && index.isValid()) {
                    formula_bar_->setText(index.data(Qt::DisplayRole).toString().replace(
                        QStringLiteral("*"), QString()));
                }
            });
    connect(formula_bar_, &QLineEdit::editingFinished, this, [this] {
        const QModelIndex index = data_table_->currentIndex();
        if (index.isValid()) {
            worksheet_model_->setData(index, formula_bar_->text());
        }
    });
    output_workspace_ = new OutputWorkspace(central);
    connect(output_workspace_, &OutputWorkspace::rows_selected, this,
            [this](const std::vector<std::size_t>& rows) {
                if (data_table_ == nullptr || data_table_->selectionModel() == nullptr) {
                    return;
                }
                data_table_->selectionModel()->clearSelection();
                for (const std::size_t row : rows) {
                    const QModelIndex index = worksheet_model_->index(static_cast<int>(row), 0);
                    if (index.isValid()) {
                        data_table_->selectionModel()->select(
                            index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
                    }
                }
            });
    connect(data_table_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] {
                std::vector<std::size_t> rows;
                if (data_table_ != nullptr && data_table_->selectionModel() != nullptr) {
                    for (const QModelIndex& index : data_table_->selectionModel()->selectedRows()) {
                        rows.push_back(static_cast<std::size_t>(index.row()));
                    }
                }
                if (output_workspace_ != nullptr) {
                    output_workspace_->set_selected_source_rows(rows);
                }
            });
    connect(output_workspace_, &OutputWorkspace::page_title_changed, this,
            [this](const QString& id, const QString& title) {
                navigator_->rename_analysis(id, title);
            });

    auto* workspace = new QSplitter(Qt::Vertical, central);
    workspace->addWidget(output_workspace_);
    workspace->addWidget(data_table_);
    workspace->setStretchFactor(0, 2);
    workspace->setStretchFactor(1, 1);
    workspace->setSizes({460, 320});
    layout->addWidget(workspace);
    setCentralWidget(central);
    setDockNestingEnabled(true);
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == data_table_ && event->type() == QEvent::KeyPress) {
        if (static_cast<WorksheetView*>(data_table_)->is_editing()) {
            return QMainWindow::eventFilter(watched, event);
        }
        auto* key_event = static_cast<QKeyEvent*>(event);
        if (key_event->matches(QKeySequence::Paste)) {
            paste_clipboard();
            return true;
        }
        if (key_event->matches(QKeySequence::Copy)) {
            copy_selection();
            return true;
        }
        if (key_event->matches(QKeySequence::Cut)) {
            cut_selection();
            return true;
        }
        if (key_event->key() == Qt::Key_Delete || key_event->key() == Qt::Key_Backspace) {
            clear_selection();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::push_table_change(
    const datalab::domain::DataTable& before,
    const datalab::domain::DataTable& after,
    const QString& text,
    bool already_applied)
{
    if (before.rows == after.rows && before.columns == after.columns) {
        return;
    }
    undo_stack_->push(
        new TableChangeCommand(worksheet_model_, before, after, text, already_applied));
}

void MainWindow::copy_selection()
{
    if (data_table_ == nullptr || data_table_->selectionModel() == nullptr) {
        return;
    }
    const QModelIndexList indexes = data_table_->selectionModel()->selectedIndexes();
    if (indexes.isEmpty()) {
        return;
    }
    int min_row = indexes.front().row();
    int max_row = min_row;
    int min_column = indexes.front().column();
    int max_column = min_column;
    for (const QModelIndex& index : indexes) {
        min_row = std::min(min_row, index.row());
        max_row = std::max(max_row, index.row());
        min_column = std::min(min_column, index.column());
        max_column = std::max(max_column, index.column());
    }
    QStringList lines;
    for (int row = min_row; row <= max_row; ++row) {
        QStringList cells;
        for (int column = min_column; column <= max_column; ++column) {
            cells.push_back(worksheet_model_->data(
                worksheet_model_->index(row, column), Qt::DisplayRole).toString());
        }
        lines.push_back(cells.join(QLatin1Char('\t')));
    }
    QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
    statusBar()->showMessage(QStringLiteral("已复制 %1 行 × %2 列。")
                                 .arg(max_row - min_row + 1)
                                 .arg(max_column - min_column + 1));
}

void MainWindow::cut_selection()
{
    if (data_table_ == nullptr || data_table_->selectionModel() == nullptr
        || data_table_->selectionModel()->selectedIndexes().isEmpty()) {
        return;
    }
    copy_selection();
    const datalab::domain::DataTable before = table_;
    datalab::domain::DataTable after = before;
    for (const QModelIndex& index : data_table_->selectionModel()->selectedIndexes()) {
        if (index.row() < static_cast<int>(after.rows.size())
            && index.column() < static_cast<int>(after.rows[index.row()].size())) {
            after.rows[static_cast<std::size_t>(index.row())]
                      [static_cast<std::size_t>(index.column())].clear();
        }
    }
    push_table_change(before, after, QStringLiteral("剪切单元格"));
}

void MainWindow::clear_selection()
{
    if (data_table_ == nullptr || data_table_->selectionModel() == nullptr) {
        return;
    }
    if (static_cast<WorksheetView*>(data_table_)->is_editing()) {
        return;
    }
    const QModelIndexList indexes = data_table_->selectionModel()->selectedIndexes();
    if (indexes.isEmpty()) {
        return;
    }
    const datalab::domain::DataTable before = table_;
    datalab::domain::DataTable after = before;
    for (const QModelIndex& index : indexes) {
        if (index.row() < static_cast<int>(after.rows.size())
            && index.column() < static_cast<int>(after.rows[index.row()].size())) {
            after.rows[static_cast<std::size_t>(index.row())]
                      [static_cast<std::size_t>(index.column())].clear();
        }
    }
    push_table_change(before, after, QStringLiteral("清除单元格"));
}

void MainWindow::paste_clipboard()
{
    if (data_table_ == nullptr || data_table_->currentIndex().isValid() == false) {
        return;
    }
    const QString text = QApplication::clipboard()->text();
    if (text.isEmpty()) {
        return;
    }
    const QModelIndex start = data_table_->currentIndex();
    QString normalized = text;
    while (normalized.endsWith(QLatin1Char('\n'))
           || normalized.endsWith(QLatin1Char('\r'))) {
        normalized.chop(1);
    }
    if (normalized.isEmpty()) {
        return;
    }
    const QChar delimiter = normalized.contains(QLatin1Char('\t'))
        ? QLatin1Char('\t')
        : QLatin1Char(',');
    const QStringList lines = normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"))
        .replace(QLatin1Char('\r'), QLatin1Char('\n'))
        .split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    const datalab::domain::DataTable before = table_;
    datalab::domain::DataTable after = before;
    for (int row_offset = 0; row_offset < lines.size(); ++row_offset) {
        const QStringList cells = lines[row_offset].split(delimiter, Qt::KeepEmptyParts);
        for (int column_offset = 0; column_offset < cells.size(); ++column_offset) {
            const int row = start.row() + row_offset;
            const int column = start.column() + column_offset;
            if (row < 0 || column < 0) {
                continue;
            }
            if (row >= static_cast<int>(after.rows.size())) {
                after.rows.resize(static_cast<std::size_t>(row + 1));
            }
            if (column >= static_cast<int>(after.columns.size())) {
                after.columns.resize(static_cast<std::size_t>(column + 1));
            }
            if (column >= static_cast<int>(after.rows[static_cast<std::size_t>(row)].size())) {
                after.rows[static_cast<std::size_t>(row)].resize(
                    static_cast<std::size_t>(column + 1));
            }
            after.rows[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] =
                cells[column_offset].toStdString();
        }
    }
    push_table_change(before, after, QStringLiteral("粘贴单元格"));
    statusBar()->showMessage(QStringLiteral("已从剪贴板粘贴 %1 行数据。").arg(lines.size()));
}

void MainWindow::new_project()
{
    table_ = {};
    cleaning_operations_.clear();
    worksheet_model_->set_table(table_);
    worksheet_model_->set_excluded_rows({});
    output_workspace_->clear_pages();
    navigator_->clear_contents();
    navigator_->set_project_name(QStringLiteral("DataLab 项目"));
    statusBar()->showMessage(QStringLiteral("已新建项目。"));
}

void MainWindow::open_project()
{
    const QString file_path = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开 DataLab 项目"), QString(),
        QStringLiteral("DataLab 项目 (*.dlab)"));
    if (file_path.isEmpty()) {
        return;
    }
    QString error_message;
    std::vector<datalab::domain::OutputPage> pages;
    datalab::infrastructure::ProjectRepository repository;
    if (!repository.load(file_path, &table_, &cleaning_operations_, &pages, &error_message)) {
        QMessageBox::critical(this, QStringLiteral("打开失败"), error_message);
        return;
    }
    display_table();
    output_workspace_->clear_pages();
    navigator_->clear_contents();
    navigator_->set_project_name(QString::fromStdString(table_.name.empty() ? "DataLab 项目" : table_.name));
    if (!table_.name.empty()) {
        navigator_->add_worksheet(QString::fromStdString(table_.name));
    }
    for (const auto& page : pages) {
        output_workspace_->add_page(page);
        navigator_->add_analysis(QString::fromStdString(page.id), QString::fromStdString(page.title));
    }
    statusBar()->showMessage(QStringLiteral("项目已打开。"));
}

void MainWindow::import_data()
{
    const QString file_path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("导入数据"),
        QString(),
        QStringLiteral("数据文件 (*.csv *.txt *.xlsx *.xls);;所有文件 (*.*)"));
    if (file_path.isEmpty()) {
        return;
    }

    QString error_message;
    const auto imported = datalab::infrastructure::DataImportService::import_file(
        file_path, &error_message);
    if (!imported.has_value()) {
        QMessageBox::critical(this, QStringLiteral("导入失败"), error_message);
        return;
    }

    table_ = *imported;
    display_table();
    navigator_->add_worksheet(QString::fromStdString(table_.name));
    QString import_summary = QStringLiteral("已导入 %1 行数据：%2。")
        .arg(static_cast<qulonglong>(table_.rows.size()))
        .arg(QString::fromStdString(table_.name));
    if (!table_.import_warnings.empty()) {
        import_summary += QStringLiteral(" 检测到 %1 个导入警告，请检查列名或行字段数。")
            .arg(static_cast<qulonglong>(table_.import_warnings.size()));
    }
    statusBar()->showMessage(import_summary);
}

void MainWindow::display_table()
{
    worksheet_model_->set_table(table_);
    worksheet_model_->set_excluded_rows(excluded_rows());
    data_table_->resizeColumnsToContents();
}

void MainWindow::save_project()
{
    if (table_.columns.empty()) {
        QMessageBox::information(this, QStringLiteral("没有数据"), QStringLiteral("请先导入数据。"));
        return;
    }
    const QString file_path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("保存 DataLab 项目"),
        QString(),
        QStringLiteral("DataLab 项目 (*.dlab)"));
    if (file_path.isEmpty()) {
        return;
    }

    QString error_message;
    const datalab::infrastructure::ProjectRepository repository;
    if (!repository.save(file_path, table_, cleaning_operations_, output_workspace_->pages(), &error_message)) {
        QMessageBox::critical(this, QStringLiteral("保存失败"), error_message);
        return;
    }
    statusBar()->showMessage(QStringLiteral("项目已保存。"));
}

bool MainWindow::ensure_data()
{
    if (table_.columns.empty()) {
        QMessageBox::information(this, QStringLiteral("没有数据"), QStringLiteral("请先导入数据。"));
        return false;
    }
    return true;
}

QStringList MainWindow::column_labels() const
{
    return worksheet_model_->column_labels();
}

datalab::domain::AnalysisConfiguration MainWindow::base_configuration() const
{
    datalab::domain::AnalysisConfiguration configuration;
    configuration.excluded_rows = excluded_rows();
    return configuration;
}

std::vector<std::size_t> MainWindow::excluded_rows() const
{
    std::vector<std::size_t> rows;
    for (const auto& operation : cleaning_operations_) {
        if (operation.operation != "exclude_row") {
            continue;
        }
        for (const std::size_t row : operation.affected_rows) {
            if (std::find(rows.begin(), rows.end(), row) == rows.end()) {
                rows.push_back(row);
            }
        }
    }
    return rows;
}

void MainWindow::publish_page(const datalab::domain::OutputPage& page)
{
    datalab::domain::OutputPage enriched_page = page;
    datalab::application::InterpretationService::enrich(enriched_page);
    if (!enriched_page.diagnostics.empty()
        && enriched_page.diagnostics.front().severity == datalab::domain::DiagnosticMessage::Severity::error
        && enriched_page.tables.empty() && enriched_page.plots.empty()) {
        QMessageBox::warning(this, QStringLiteral("分析未完成"),
                             QString::fromStdString(enriched_page.diagnostics.front().message));
        return;
    }
    output_workspace_->add_page(enriched_page);
    navigator_->add_analysis(QString::fromStdString(enriched_page.id), QString::fromStdString(enriched_page.title));
    statusBar()->showMessage(QStringLiteral("分析完成：") + QString::fromStdString(enriched_page.title));
}

void MainWindow::run_from_spec(const QString& id)
{
    const analysis_commands::AnalysisCommand* command = analysis_commands::find(id);
    if (command == nullptr) {
        return;
    }
    if (command->requires_data && !ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(
        command->dialog_title,
        column_labels(),
        this,
        QStringLiteral(":/icons/%1.svg").arg(command->icon_file));
    for (const auto& role : command->roles) {
        dialog.add_role(role.id, role.label, role.multi, role.optional);
    }
    for (const auto& input : command->inputs) {
        dialog.add_line_edit(input.id, input.label, input.placeholder);
    }
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    const analysis_commands::AnalysisApplyResult result = command->apply(configuration, dialog);
    if (!result.valid) {
        if (!result.error_title.isEmpty()) {
            QMessageBox::information(this, result.error_title, result.error_message);
        }
        return;
    }
    publish_page(command->run(table_, configuration));
}

void MainWindow::exclude_selected_row()
{
    const QModelIndexList selected_rows = data_table_->selectionModel()->selectedRows();
    if (selected_rows.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择行"), QStringLiteral("请先在工作表中选择要排除的行。"));
        return;
    }
    std::vector<std::size_t> rows;
    for (const QModelIndex& index : selected_rows) {
        const std::size_t row = static_cast<std::size_t>(index.row());
        if (std::find(rows.begin(), rows.end(), row) == rows.end()) {
            rows.push_back(row);
        }
    }
    cleaning_operations_.push_back({"exclude_row", "用户从分析中排除选中行。", rows});
    worksheet_model_->set_excluded_rows(excluded_rows());
    statusBar()->showMessage(
        QStringLiteral("已标记排除 %1 行，原始数据未修改。")
            .arg(static_cast<qulonglong>(rows.size())));
}

void MainWindow::clear_exclusions()
{
    cleaning_operations_.erase(
        std::remove_if(cleaning_operations_.begin(), cleaning_operations_.end(),
                       [](const datalab::domain::CleaningOperation& operation) {
                           return operation.operation == "exclude_row";
                       }),
        cleaning_operations_.end());
    worksheet_model_->set_excluded_rows({});
    statusBar()->showMessage(QStringLiteral("已清除排除标记，原始数据未修改。"));
}

void MainWindow::export_pdf()
{
    if (!output_workspace_->has_pages()) {
        QMessageBox::information(this, QStringLiteral("没有分析"), QStringLiteral("请先运行分析。"));
        return;
    }
    ReportPreviewDialog preview(output_workspace_->pages(), this);
    if (preview.exec() != QDialog::Accepted) {
        return;
    }
    const QString file_path = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出 PDF 报告"),
        QString(),
        QStringLiteral("PDF 文件 (*.pdf)"));
    if (file_path.isEmpty()) {
        return;
    }
    QString error_message;
    if (!datalab::infrastructure::PdfReportWriter::write(
            file_path, table_, output_workspace_->pages(), &error_message)) {
        QMessageBox::critical(this, QStringLiteral("导出失败"), error_message);
        return;
    }
    navigator_->add_report(QStringLiteral("PDF 报告"));
    statusBar()->showMessage(QStringLiteral("PDF 报告已导出。"));
}
