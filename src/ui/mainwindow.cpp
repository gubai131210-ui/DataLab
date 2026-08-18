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
#include <QFrame>
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
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QUndoCommand>
#include <QUndoStack>
#include <QVBoxLayout>

#include <algorithm>
#include <optional>
#include <utility>

namespace {

QString primary_analysis_menu(const analysis_commands::AnalysisCommand& command)
{
    if (command.id == QStringLiteral("pareto")) {
        return QStringLiteral("质量工具");
    }
    if (command.id == QStringLiteral("doe_factorial")
        || command.id == QStringLiteral("doe_response")
        || command.menu_path == QStringLiteral("控制图")) {
        return QStringLiteral("统计");
    }
    return command.menu_path;
}

QString analysis_menu_group(const analysis_commands::AnalysisCommand& command)
{
    const QString id = command.id;
    if (command.menu_path == QStringLiteral("控制图")) {
        return QStringLiteral("控制图");
    }
    if (id == QStringLiteral("doe_factorial")
        || id == QStringLiteral("doe_response")) {
        return QStringLiteral("DOE");
    }
    if (id == QStringLiteral("descriptive")
        || id == QStringLiteral("normality_test")
        || id == QStringLiteral("one_sample_t")
        || id == QStringLiteral("two_sample_t")
        || id == QStringLiteral("paired_t")
        || id == QStringLiteral("correlation")) {
        return QStringLiteral("基础统计");
    }
    if (id == QStringLiteral("one_way_anova")
        || id == QStringLiteral("two_factor_anova")) {
        return QStringLiteral("ANOVA");
    }
    if (id == QStringLiteral("regression")
        || id == QStringLiteral("logistic_regression")) {
        return QStringLiteral("回归");
    }
    if (id == QStringLiteral("time_series_smoothing")
        || id == QStringLiteral("time_series_decomposition")
        || id == QStringLiteral("seasonal_forecasting")
        || id == QStringLiteral("arima")) {
        return QStringLiteral("时间序列");
    }
    if (id == QStringLiteral("pca")) {
        return QStringLiteral("多变量");
    }
    if (id == QStringLiteral("reliability")) {
        return QStringLiteral("可靠性");
    }
    if (id == QStringLiteral("t_power")) {
        return QStringLiteral("功效与样本量");
    }
    if (id == QStringLiteral("two_proportions")
        || id == QStringLiteral("chi_square")
        || id == QStringLiteral("variance_test")
        || id == QStringLiteral("mann_whitney")
        || id == QStringLiteral("wilcoxon_signed_rank")
        || id == QStringLiteral("kruskal_wallis")) {
        return QStringLiteral("假设检验");
    }
    if (id == QStringLiteral("capability")
        || id == QStringLiteral("capability_sixpack")
        || id == QStringLiteral("box_cox")
        || id == QStringLiteral("gage_rr")
        || id == QStringLiteral("msa_type1")
        || id == QStringLiteral("nested_gage_rr")
        || id == QStringLiteral("attribute_agreement")
        || id == QStringLiteral("pareto")) {
        return QStringLiteral("质量工具");
    }
    if (command.menu_path == QStringLiteral("图形")) {
        return QStringLiteral("探索性图形");
    }
    return {};
}

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

// 清洗操作（行排除/清除排除）undo：保存前后 cleaning_operations_ 快照，
// undo/redo 经调用方提供的应用器重放（MainWindow 内构造 lambda 访问私有状态）。
class CleaningChangeCommand final : public QUndoCommand {
public:
    CleaningChangeCommand(
        std::function<void(const std::vector<datalab::domain::CleaningOperation>&)> apply,
        std::vector<datalab::domain::CleaningOperation> before,
        std::vector<datalab::domain::CleaningOperation> after,
        const QString& text)
        : apply_(std::move(apply))
        , before_(std::move(before))
        , after_(std::move(after))
    {
        setText(text);
    }

    void undo() override
    {
        apply_(before_);
    }

    void redo() override
    {
        apply_(after_);
    }

private:
    std::function<void(const std::vector<datalab::domain::CleaningOperation>&)> apply_;
    std::vector<datalab::domain::CleaningOperation> before_;
    std::vector<datalab::domain::CleaningOperation> after_;
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
        "QMainWindow { background: #eef3f5; color: #243b44; }"
        "QMenuBar { background: #ffffff; color: #243b44; spacing: 4px; padding: 5px 10px;"
        " border-bottom: 1px solid #d9e3e6; }"
        "QMenuBar::item { padding: 7px 11px; border-radius: 5px; }"
        "QMenuBar::item:selected { background: #e2f2f1; color: #147d85; }"
        "QMenu { background: #ffffff; color: #243b44; border: 1px solid #d5e1e5; padding: 6px; }"
        "QMenu::item { padding: 8px 30px 8px 12px; border-radius: 5px; }"
        "QMenu::item:selected { background: #e2f2f1; color: #147d85; }"
        "QToolBar { background: #ffffff; border: 0; border-bottom: 1px solid #d9e3e6;"
        " spacing: 6px; padding: 7px 12px; }"
        "QToolButton { color: #49636d; border: 0; border-radius: 6px; padding: 7px; }"
        "QToolButton:hover { background: #e2f2f1; color: #147d85; }"
        "QToolButton:pressed { background: #cfe9e8; }"
        "QDockWidget { color: #243b44; font-weight: 600; }"
        "QDockWidget::title { background: #e3edf0; padding: 10px 13px; border: 0;"
        " border-bottom: 1px solid #d2dfe3; }"
        "QLineEdit { background: #ffffff; border: 1px solid #c8d8dd; border-radius: 6px;"
        " padding: 7px 10px; color: #243b44; min-height: 30px; }"
        "QLineEdit:focus { border: 1px solid #35a6aa; }"
        "QTableView { background: #ffffff; alternate-background-color: #f7fafb;"
        " border: 1px solid #d5e1e5; gridline-color: #e5edef; }"
        "QHeaderView::section { background: #e8f0f2; color: #38525c;"
        " border: 0; border-right: 1px solid #d5e1e5; padding: 7px 9px; }"
        "QStatusBar { background: #e3edf0; color: #49636d; border-top: 1px solid #d2dfe3; }"
        "QSplitter::handle { background: #d3e0e4; }"
        "QSplitter::handle:hover { background: #72c1c0; }"
        "QFrame#pane_card { background: #ffffff; border: 1px solid #d7e3e6;"
        " border-radius: 8px; }"
        "QLabel#pane_title { color: #29434e; font-size: 14px; font-weight: 700; }"
        "QLabel#pane_subtitle { color: #71858d; font-size: 11px; }"));
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

    // 分析菜单：按 Minitab 风格的顶层菜单和子菜单生成。
    QHash<QString, QMenu*> analysis_menus;
    QHash<QString, QMenu*> analysis_submenus;
    for (const auto& command : analysis_commands::all()) {
        const QString top_menu = primary_analysis_menu(command);
        QMenu* menu = analysis_menus.value(top_menu, nullptr);
        if (menu == nullptr) {
            menu = menuBar()->addMenu(top_menu);
            analysis_menus.insert(top_menu, menu);
        }
        const QString group = analysis_menu_group(command);
        QMenu* target = menu;
        if (!group.isEmpty()) {
            const QString submenu_key = top_menu + QStringLiteral("/") + group;
            target = analysis_submenus.value(submenu_key, nullptr);
            if (target == nullptr) {
                target = menu->addMenu(group);
                analysis_submenus.insert(submenu_key, target);
            }
        }
        if (command.separator_before) {
            target->addSeparator();
        }
        commands_->add_to_menu(target, command.id);
    }
    menuBar()->addMenu(QStringLiteral("查看"));

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
    QMenu* view_menu = nullptr;
    for (QAction* action : menuBar()->actions()) {
        if (action->text() == QStringLiteral("查看")) {
            view_menu = action->menu();
            break;
        }
    }
    navigator_dock_ = new QDockWidget(QStringLiteral("项目导航器"), this);
    navigator_dock_->setAllowedAreas(Qt::LeftDockWidgetArea);
    navigator_dock_->setMinimumWidth(220);
    navigator_dock_->setMaximumWidth(300);
    navigator_ = new ProjectNavigator(navigator_dock_);
    navigator_dock_->setWidget(navigator_);
    addDockWidget(Qt::LeftDockWidgetArea, navigator_dock_);
    if (view_menu != nullptr) {
        view_menu->addAction(navigator_dock_->toggleViewAction());
    }
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
    context_layout->setContentsMargins(16, 16, 16, 16);
    context_layout->setSpacing(12);
    auto* context_heading = new QLabel(QStringLiteral("数据集状态"), context);
    context_heading->setObjectName(QStringLiteral("pane_title"));
    auto* context_status = new QLabel(QStringLiteral("尚未导入数据"), context);
    context_status->setWordWrap(true);
    context_status->setStyleSheet(QStringLiteral(
        "background: #e8f6f2; color: #18794e; padding: 12px;"
        " border: 1px solid #c7e9dc; border-radius: 7px;"));
    auto* context_detail = new QLabel(
        QStringLiteral("导入 CSV 或 Excel 后，这里显示当前工作表的行数、列数和排除状态。"),
        context);
    context_detail->setWordWrap(true);
    context_detail->setObjectName(QStringLiteral("pane_subtitle"));
    auto* context_next = new QLabel(
        QStringLiteral("下一步\n导入数据后，从“分析”菜单选择统计方法。"), context);
    context_next->setWordWrap(true);
    context_next->setStyleSheet(QStringLiteral(
        "background:#ffffff; color:#49636d; padding:11px;"
        " border:1px solid #d7e3e6; border-radius:7px;"));
    context_layout->addWidget(context_heading);
    context_layout->addWidget(context_status);
    context_layout->addWidget(context_detail);
    context_layout->addWidget(context_next);
    context_layout->addStretch(1);
    context_dock_->setWidget(context);
    addDockWidget(Qt::RightDockWidgetArea, context_dock_);
    if (view_menu != nullptr) {
        view_menu->addAction(context_dock_->toggleViewAction());
    }

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

    auto make_pane = [central](const QString& title, const QString& subtitle,
                               QWidget* content) {
        auto* pane = new QFrame(central);
        pane->setObjectName(QStringLiteral("pane_card"));
        auto* pane_layout = new QVBoxLayout(pane);
        pane_layout->setContentsMargins(14, 10, 14, 14);
        pane_layout->setSpacing(6);
        auto* heading = new QHBoxLayout();
        auto* title_label = new QLabel(title, pane);
        title_label->setObjectName(QStringLiteral("pane_title"));
        auto* subtitle_label = new QLabel(subtitle, pane);
        subtitle_label->setObjectName(QStringLiteral("pane_subtitle"));
        heading->addWidget(title_label);
        heading->addSpacing(10);
        heading->addWidget(subtitle_label);
        heading->addStretch(1);
        pane_layout->addLayout(heading);
        pane_layout->addWidget(content, 1);
        return pane;
    };
    auto* output_pane = make_pane(
        QStringLiteral("分析输出"),
        QStringLiteral("统计表、图形和诊断结果"),
        output_workspace_);
    auto* worksheet_pane = make_pane(
        QStringLiteral("活动工作表"),
        QStringLiteral("编辑数据，分析当前工作表"),
        data_table_);
    auto* workspace = new QSplitter(Qt::Vertical, central);
    workspace->setObjectName(QStringLiteral("main_workspace"));
    workspace->addWidget(output_pane);
    workspace->addWidget(worksheet_pane);
    workspace->setStretchFactor(0, 2);
    workspace->setStretchFactor(1, 1);
    workspace->setSizes({460, 320});
    layout->addWidget(workspace);
    if (view_menu != nullptr) {
        QAction* worksheet_action = view_menu->addAction(QStringLiteral("活动工作表"));
        worksheet_action->setCheckable(true);
        worksheet_action->setChecked(true);
        connect(worksheet_action, &QAction::toggled, worksheet_pane, &QWidget::setVisible);
        view_menu->addSeparator();
    }
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

    if (import_in_progress_) {
        statusBar()->showMessage(QStringLiteral("已有数据导入任务正在执行。"));
        return;
    }

    import_in_progress_ = true;
    statusBar()->showMessage(QStringLiteral("正在导入数据……"));
    using ImportResult = std::pair<std::optional<datalab::domain::DataTable>, QString>;
    auto* watcher = new QFutureWatcher<ImportResult>(this);
    connect(watcher, &QFutureWatcher<ImportResult>::finished, this, [this, watcher] {
        import_in_progress_ = false;
        const ImportResult result = watcher->result();
        watcher->deleteLater();
        if (!result.first.has_value()) {
            QMessageBox::critical(this, QStringLiteral("导入失败"), result.second);
            return;
        }
        cleaning_operations_.clear();
        output_workspace_->clear_pages();
        navigator_->clear_contents();
        table_ = *result.first;
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
    });
    watcher->setFuture(QtConcurrent::run([file_path] {
        QString error_message;
        const auto imported = datalab::infrastructure::DataImportService::import_file(
            file_path, &error_message);
        return ImportResult{imported, error_message};
    }));
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
        QStringLiteral(":/icons/%1.svg").arg(command->icon_file),
        table_.column_types);
    for (const auto& role : command->roles) {
        dialog.add_role(role);
    }
    for (const auto& input : command->inputs) {
        dialog.add_input(input);
    }
    datalab::domain::AnalysisConfiguration accepted_configuration;
    datalab::application::AnalysisIntent accepted_intent;
    dialog.set_accept_validator([&dialog, &accepted_configuration, &accepted_intent,
                                 this, command, id](QString* error_title,
                                                    QString* error_message) {
        datalab::application::AnalysisIntent intent;
        intent.command_id = id.toStdString();
        for (const auto& role : command->roles) {
            std::vector<std::size_t>& selected = intent.roles[role.id.toStdString()];
            for (const int index : dialog.role_indices(role.id)) {
                if (index >= 0) {
                    selected.push_back(static_cast<std::size_t>(index));
                }
            }
        }
        for (const auto& input : command->inputs) {
            intent.inputs[input.id.toStdString()] =
                dialog.line_text(input.id).toStdString();
        }

        datalab::domain::AnalysisConfiguration configuration = base_configuration();
        const analysis_commands::AnalysisApplyResult result =
            command->apply(configuration, intent);
        if (!result.valid) {
            if (error_title != nullptr) {
                *error_title = result.error_title;
            }
            if (error_message != nullptr) {
                *error_message = result.error_message;
            }
            if (!result.field_id.isEmpty()) {
                dialog.set_field_error(result.field_id, result.error_message);
            }
            return false;
        }
        accepted_configuration = std::move(configuration);
        accepted_intent = std::move(intent);
        return true;
    });
    if (dialog.exec() == QDialog::Accepted) {
        publish_page(command->run(table_, accepted_configuration));
    }
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
    const std::vector<datalab::domain::CleaningOperation> before = cleaning_operations_;
    std::vector<datalab::domain::CleaningOperation> after = before;
    after.push_back({"exclude_row", "用户从分析中排除选中行。", rows});
    // push() 立即调用 redo() 重放 after，纳入 undo 栈后可撤销。
    undo_stack_->push(new CleaningChangeCommand(
        [this](const std::vector<datalab::domain::CleaningOperation>& operations) {
            restore_cleaning_operations(operations);
        },
        before, std::move(after), QStringLiteral("排除行")));
    statusBar()->showMessage(
        QStringLiteral("已标记排除 %1 行，原始数据未修改。")
            .arg(static_cast<qulonglong>(rows.size())));
}

void MainWindow::clear_exclusions()
{
    const std::vector<datalab::domain::CleaningOperation> before = cleaning_operations_;
    std::vector<datalab::domain::CleaningOperation> after = before;
    after.erase(
        std::remove_if(after.begin(), after.end(),
                       [](const datalab::domain::CleaningOperation& operation) {
                           return operation.operation == "exclude_row";
                       }),
        after.end());
    undo_stack_->push(new CleaningChangeCommand(
        [this](const std::vector<datalab::domain::CleaningOperation>& operations) {
            restore_cleaning_operations(operations);
        },
        before, std::move(after), QStringLiteral("清除排除标记")));
    statusBar()->showMessage(QStringLiteral("已清除排除标记，原始数据未修改。"));
}

void MainWindow::restore_cleaning_operations(
    const std::vector<datalab::domain::CleaningOperation>& operations)
{
    cleaning_operations_ = operations;
    worksheet_model_->set_excluded_rows(excluded_rows());
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
