#include "mainwindow.h"

#include "application/analysis_service.h"
#include "application/interpretation_service.h"
#include "application/report_assembly_service.h"
#include "application/report_export_service.h"
#include "application/report_localization.h"
#include "infrastructure/data_import_service.h"
#include "infrastructure/pdf_report_writer.h"
#include "infrastructure/project_repository.h"
#include "infrastructure/report_export_writer.h"
#include "infrastructure/report_serialization.h"
#include "ui/analysis_commands.h"
#include "ui/analysis_setup_dialog.h"
#include "ui/analysis_chart_widget.h"
#include "ui/command_registry.h"
#include "ui/output_workspace.h"
#include "ui/project_navigator.h"
#include "ui/worksheet_model.h"
#include "ui/worksheet_sort_filter_proxy.h"
#include "ui/worksheet_view.h"
#include "ui/report_preview_dialog.h"
#include "ui/report_template_dialog.h"
#include "ui/algorithm_help_dialog.h"
#include "ui/formula_registry_dialog.h"
#include "ui/database_import_wizard.h"
#include "ui/app_ui_tr.h"

#include <QAction>
#include <QAbstractItemDelegate>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QDockWidget>
#include <QEvent>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QJsonDocument>
#include <QHash>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QToolBar>
#include <QWidget>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QUndoCommand>
#include <QUndoStack>
#include <QVBoxLayout>

#include <algorithm>
#include <optional>
#include <set>
#include <utility>
#include <vector>
namespace {

AnalysisChartWidget* chart_widget_from_focus()
{
    QWidget* widget = QApplication::focusWidget();
    while (widget != nullptr) {
        if (auto* chart = qobject_cast<AnalysisChartWidget*>(widget)) {
            return chart;
        }
        widget = widget->parentWidget();
    }
    return nullptr;
}

QTableView* output_table_view_from_focus(OutputWorkspace* workspace)
{
    if (workspace == nullptr) {
        return nullptr;
    }
    QWidget* widget = QApplication::focusWidget();
    while (widget != nullptr && widget != workspace) {
        if (auto* table = qobject_cast<QTableView*>(widget)) {
            if (workspace->isAncestorOf(table)) {
                return table;
            }
        }
        widget = widget->parentWidget();
    }
    return nullptr;
}

bool trigger_output_table_copy(QTableView* table_view)
{
    if (table_view == nullptr) {
        return false;
    }
    static const QString k_copy_table_action = QStringLiteral("复制表格（TSV）");
    for (QAction* action : table_view->actions()) {
        if (action->text() == k_copy_table_action) {
            action->trigger();
            return true;
        }
    }
    return false;
}

AnalysisChartWidget* chart_widget_on_page(QWidget* page, OutputWorkspace* workspace = nullptr)
{
    if (workspace != nullptr && page == workspace->currentWidget()) {
        if (AnalysisChartWidget* preferred = workspace->chart_for_copy()) {
            return preferred;
        }
    }
    if (page == nullptr) {
        return nullptr;
    }
    if (AnalysisChartWidget* focused = chart_widget_from_focus()) {
        if (page->isAncestorOf(focused)) {
            return focused;
        }
    }
    const auto charts = page->findChildren<AnalysisChartWidget*>();
    return charts.isEmpty() ? nullptr : charts.front();
}

bool data_tables_equal(
    const datalab::domain::DataTable& left,
    const datalab::domain::DataTable& right)
{
    return left.name == right.name
        && left.source_path == right.source_path
        && left.columns == right.columns
        && left.rows == right.rows
        && left.import_warnings == right.import_warnings
        && left.import_metadata.schema_version == right.import_metadata.schema_version
        && left.import_metadata.sheet_name == right.import_metadata.sheet_name
        && left.import_metadata.sheet_index == right.import_metadata.sheet_index
        && left.import_metadata.original_row_count == right.import_metadata.original_row_count
        && left.import_metadata.column_count == right.import_metadata.column_count
        && left.import_metadata.dataset_id == right.import_metadata.dataset_id
        && left.import_metadata.imported_at == right.import_metadata.imported_at
        && left.import_metadata.warnings == right.import_metadata.warnings
        && left.row_ids == right.row_ids
        && left.column_types == right.column_types
        && left.cell_states == right.cell_states;
}

QString primary_analysis_menu(const analysis_commands::AnalysisCommand& command)
{
    if (command.id == QStringLiteral("pareto")) {
        return QStringLiteral("质量工具");
    }
    if (command.id == QStringLiteral("doe_factorial")
        || command.id == QStringLiteral("doe_response")
        || command.id == QStringLiteral("response_optimization")
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
        || id == QStringLiteral("doe_response")
        || id == QStringLiteral("response_optimization")) {
        return QStringLiteral("DOE");
    }
    if (id == QStringLiteral("descriptive")
        || id == QStringLiteral("normality_test")
        || id == QStringLiteral("outlier_test")
        || id == QStringLiteral("one_sample_t")
        || id == QStringLiteral("one_sample_z")
        || id == QStringLiteral("two_sample_t")
        || id == QStringLiteral("paired_t")
        || id == QStringLiteral("correlation")
        || id == QStringLiteral("one_proportion")
        || id == QStringLiteral("one_poisson_rate")
        || id == QStringLiteral("two_poisson_rate")
        || id == QStringLiteral("one_sample_equivalence")
        || id == QStringLiteral("paired_equivalence")
        || id == QStringLiteral("two_sample_equivalence")
        || id == QStringLiteral("two_sample_equivalence_ratio")
        || id == QStringLiteral("one_proportion_equivalence")
        || id == QStringLiteral("two_proportion_equivalence")) {
        return QStringLiteral("基础统计");
    }
    if (id == QStringLiteral("one_way_anova")
        || id == QStringLiteral("two_factor_anova")
        || id == QStringLiteral("anom")) {
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
        || id == QStringLiteral("chi_square_gof")
        || id == QStringLiteral("poisson_gof")
        || id == QStringLiteral("fisher_exact")
        || id == QStringLiteral("variance_test")
        || id == QStringLiteral("mann_whitney")
        || id == QStringLiteral("wilcoxon_signed_rank")
        || id == QStringLiteral("sign_test")
        || id == QStringLiteral("runs_test")
        || id == QStringLiteral("mcnemar")
        || id == QStringLiteral("cochran_q")
        || id == QStringLiteral("mood_median")
        || id == QStringLiteral("kruskal_wallis")
        || id == QStringLiteral("friedman")) {
        return QStringLiteral("假设检验");
    }
    if (id == QStringLiteral("cross_tabulation")) {
        return QStringLiteral("表格");
    }
    if (id == QStringLiteral("capability")
        || id == QStringLiteral("nonnormal_capability")
        || id == QStringLiteral("capability_sixpack")
        || id == QStringLiteral("box_cox")
        || id == QStringLiteral("gage_rr")
        || id == QStringLiteral("msa_type1")
        || id == QStringLiteral("nested_gage_rr")
        || id == QStringLiteral("attribute_agreement")
        || id == QStringLiteral("pareto")
        || id == QStringLiteral("run_chart")
        || id == QStringLiteral("cause_and_effect")
        || id == QStringLiteral("multi_vari")
        || id == QStringLiteral("variability_chart")
        || id == QStringLiteral("acceptance_sampling")
        || id == QStringLiteral("tolerance_intervals")) {
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
        "QTableView QLineEdit { min-height: 0; padding: 0 4px; margin: 0;"
        " border: 1px solid #35a6aa; border-radius: 0; }"
        "QTableView QLineEdit:focus { border: 1px solid #35a6aa; }"
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

    connect_action(QStringLiteral("new"), datalab::ui::ui_tr("新建项目"), &MainWindow::new_project);
    connect_action(QStringLiteral("open"), datalab::ui::ui_tr("打开项目"), &MainWindow::open_project);
    connect_action(QStringLiteral("save"), datalab::ui::ui_tr("保存项目"), &MainWindow::save_project);
    connect_action(QStringLiteral("export_pdf"), datalab::ui::ui_tr("导出 PDF"), &MainWindow::export_pdf);
    commands_->add(QStringLiteral("exit"), datalab::ui::ui_tr("退出"));
    connect(commands_->get(QStringLiteral("exit")), &QAction::triggered, this, &QWidget::close);

    QAction* undo_action = commands_->add(QStringLiteral("undo"), datalab::ui::ui_tr("撤销"), false);
    QAction* redo_action = commands_->add(QStringLiteral("redo"), datalab::ui::ui_tr("重做"), false);
    connect(undo_action, &QAction::triggered, undo_stack_, &QUndoStack::undo);
    connect(redo_action, &QAction::triggered, undo_stack_, &QUndoStack::redo);
    connect(undo_stack_, &QUndoStack::canUndoChanged, undo_action, &QAction::setEnabled);
    connect(undo_stack_, &QUndoStack::canRedoChanged, redo_action, &QAction::setEnabled);

    connect_action(QStringLiteral("import"), datalab::ui::ui_tr("导入数据"), &MainWindow::import_data);
    connect_action(QStringLiteral("import_database"), datalab::ui::ui_tr("数据库导入…"),
                   &MainWindow::import_database);
    QAction* copy_action = commands_->add(QStringLiteral("copy"), datalab::ui::ui_tr("复制"));
    copy_action->setShortcut(QKeySequence::Copy);
    copy_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(copy_action, &QAction::triggered, this, &MainWindow::copy_selection);
    QAction* copy_chart_action = commands_->add(QStringLiteral("copy_chart"), datalab::ui::ui_tr("复制图形"));
    copy_chart_action->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    copy_chart_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(copy_chart_action, &QAction::triggered, this, &MainWindow::copy_chart);
    QAction* cut_action = commands_->add(QStringLiteral("cut"), datalab::ui::ui_tr("剪切"));
    cut_action->setShortcut(QKeySequence::Cut);
    cut_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(cut_action, &QAction::triggered, this, &MainWindow::cut_selection);
    QAction* clear_action = commands_->add(QStringLiteral("clear_cells"), datalab::ui::ui_tr("清除单元格"));
    clear_action->setShortcut(QKeySequence::Delete);
    clear_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(clear_action, &QAction::triggered, this, &MainWindow::clear_selection);
    QAction* paste_action = commands_->add(QStringLiteral("paste"), datalab::ui::ui_tr("粘贴"));
    paste_action->setShortcut(QKeySequence::Paste);
    paste_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(paste_action, &QAction::triggered, this, &MainWindow::paste_clipboard);
    connect_action(QStringLiteral("exclude"), datalab::ui::ui_tr("排除选中行"), &MainWindow::exclude_selected_row);
    connect_action(QStringLiteral("clear_exclude"), datalab::ui::ui_tr("清除排除标记"), &MainWindow::clear_exclusions);
    connect_action(QStringLiteral("hide_row"), datalab::ui::ui_tr("隐藏选中行（仅显示）"), &MainWindow::hide_selected_row);
    connect_action(QStringLiteral("clear_hide"), datalab::ui::ui_tr("清除隐藏标记"), &MainWindow::clear_hidden_rows);

    set_icon(QStringLiteral("new"), QStringLiteral("new"));
    set_icon(QStringLiteral("open"), QStringLiteral("open"));
    set_icon(QStringLiteral("save"), QStringLiteral("save"));
    set_icon(QStringLiteral("import"), QStringLiteral("import-data"));
    set_icon(QStringLiteral("undo"), QStringLiteral("undo"));
    set_icon(QStringLiteral("redo"), QStringLiteral("redo"));
    set_icon(QStringLiteral("export_pdf"), QStringLiteral("export-pdf"));
    // 分析命令：注册动作 + 连接通用 run_from_spec + 由命令表提供图标。
    for (const auto& command : analysis_commands::all()) {
        QAction* action = commands_->add(command.id, datalab::ui::ui_tr(command.menu_label));
        connect(action, &QAction::triggered, this, [this, id = command.id] {
            run_from_spec(id);
        });
        set_icon(command.id, command.icon_file);
    }

    auto* file_menu = menuBar()->addMenu(datalab::ui::ui_tr("文件"));
    commands_->add_to_menu(file_menu, QStringLiteral("new"));
    commands_->add_to_menu(file_menu, QStringLiteral("open"));
    commands_->add_to_menu(file_menu, QStringLiteral("save"));
    file_menu->addSeparator();
    commands_->add_to_menu(file_menu, QStringLiteral("export_pdf"));
    file_menu->addSeparator();
    commands_->add_to_menu(file_menu, QStringLiteral("exit"));

    auto* edit_menu = menuBar()->addMenu(datalab::ui::ui_tr("编辑"));
    commands_->add_to_menu(edit_menu, QStringLiteral("undo"));
    commands_->add_to_menu(edit_menu, QStringLiteral("redo"));
    edit_menu->addSeparator();
    commands_->add_to_menu(edit_menu, QStringLiteral("copy"));
    commands_->add_to_menu(edit_menu, QStringLiteral("copy_chart"));
    commands_->add_to_menu(edit_menu, QStringLiteral("cut"));
    commands_->add_to_menu(edit_menu, QStringLiteral("paste"));
    commands_->add_to_menu(edit_menu, QStringLiteral("clear_cells"));

    auto* data_menu = menuBar()->addMenu(datalab::ui::ui_tr("数据"));
    commands_->add_to_menu(data_menu, QStringLiteral("import"));
    commands_->add_to_menu(data_menu, QStringLiteral("import_database"));
    commands_->add_to_menu(data_menu, QStringLiteral("exclude"));
    commands_->add_to_menu(data_menu, QStringLiteral("clear_exclude"));
    commands_->add_to_menu(data_menu, QStringLiteral("hide_row"));
    commands_->add_to_menu(data_menu, QStringLiteral("clear_hide"));

    // 分析菜单：按 Minitab 风格的顶层菜单和子菜单生成。
    // Hash keys stay zh_CN source strings so language does not split menus.
    QHash<QString, QMenu*> analysis_menus;
    QHash<QString, QMenu*> analysis_submenus;
    for (const auto& command : analysis_commands::all()) {
        const QString top_menu_src = primary_analysis_menu(command);
        QMenu* menu = analysis_menus.value(top_menu_src, nullptr);
        if (menu == nullptr) {
            menu = menuBar()->addMenu(datalab::ui::ui_tr(top_menu_src));
            analysis_menus.insert(top_menu_src, menu);
        }
        const QString group_src = analysis_menu_group(command);
        QMenu* target = menu;
        if (!group_src.isEmpty()) {
            const QString submenu_key = top_menu_src + QStringLiteral("/") + group_src;
            target = analysis_submenus.value(submenu_key, nullptr);
            if (target == nullptr) {
                target = menu->addMenu(datalab::ui::ui_tr(group_src));
                analysis_submenus.insert(submenu_key, target);
            }
        }
        if (command.separator_before) {
            target->addSeparator();
        }
        commands_->add_to_menu(target, command.id);
    }
    menuBar()->addMenu(datalab::ui::ui_tr("查看"));

    auto* help_menu = menuBar()->addMenu(datalab::ui::ui_tr("帮助"));
    help_menu->addAction(datalab::ui::ui_tr("算法、公式与参考资料"), this, [this] {
        if (algorithm_help_dialog_ == nullptr) {
            algorithm_help_dialog_ = new AlgorithmHelpDialog(this);
            algorithm_help_dialog_->setAttribute(Qt::WA_DeleteOnClose);
            connect(algorithm_help_dialog_, &QObject::destroyed, this, [this] {
                algorithm_help_dialog_ = nullptr;
            });
            connect(
                algorithm_help_dialog_,
                &AlgorithmHelpDialog::open_in_formula_registry,
                this,
                [this](const QString& id) {
                    if (formula_registry_dialog_ == nullptr) {
                        formula_registry_dialog_ = new FormulaRegistryDialog(this);
                        formula_registry_dialog_->setAttribute(Qt::WA_DeleteOnClose);
                        connect(formula_registry_dialog_, &QObject::destroyed, this, [this] {
                            formula_registry_dialog_ = nullptr;
                        });
                    }
                    formula_registry_dialog_->select_entry(id);
                    formula_registry_dialog_->show();
                    formula_registry_dialog_->raise();
                    formula_registry_dialog_->activateWindow();
                });
        }
        algorithm_help_dialog_->show();
        algorithm_help_dialog_->raise();
        algorithm_help_dialog_->activateWindow();
    });
    help_menu->addAction(datalab::ui::ui_tr("公式注册表"), this, [this] {
        if (formula_registry_dialog_ == nullptr) {
            formula_registry_dialog_ = new FormulaRegistryDialog(this);
            formula_registry_dialog_->setAttribute(Qt::WA_DeleteOnClose);
            connect(formula_registry_dialog_, &QObject::destroyed, this, [this] {
                formula_registry_dialog_ = nullptr;
            });
        }
        formula_registry_dialog_->show();
        formula_registry_dialog_->raise();
        formula_registry_dialog_->activateWindow();
    });
    help_menu->addAction(datalab::ui::ui_tr("关于 DataLab"), this, [this] {
        QMessageBox::about(
            this,
            datalab::ui::ui_tr("关于 DataLab"),
            datalab::ui::ui_tr(
                "DataLab 品质工作站：按 Minitab 方式选择数据、计算并输出质量分析报告。"));
    });
    help_menu->addAction(datalab::ui::ui_tr("统计口径"), this, [this] {
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
    auto* context = new QWidget(context_dock_);
    auto* context_layout = new QVBoxLayout(context);
    context_layout->setContentsMargins(16, 16, 16, 16);
    context_layout->setSpacing(12);
    auto* context_heading = new QLabel(QStringLiteral("数据集状态"), context);
    context_heading->setObjectName(QStringLiteral("pane_title"));
    context_status_ = new QLabel(QStringLiteral("尚未导入数据"), context);
    context_status_->setWordWrap(true);
    context_detail_ = new QLabel(
        QStringLiteral("导入 CSV 或 Excel 后，这里显示当前工作表的行数、列数和排除状态。"),
        context);
    context_detail_->setWordWrap(true);
    context_detail_->setObjectName(QStringLiteral("pane_subtitle"));
    context_next_ = new QLabel(
        QStringLiteral("下一步\n导入数据后，从“统计 / 图形 / 质量工具”菜单选择方法。"), context);
    context_next_->setWordWrap(true);
    context_next_->setStyleSheet(QStringLiteral(
        "background:#ffffff; color:#49636d; padding:11px;"
        " border:1px solid #d7e3e6; border-radius:7px;"));
    context_layout->addWidget(context_heading);
    context_layout->addWidget(context_status_);
    context_layout->addWidget(context_detail_);
    context_layout->addWidget(context_next_);
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
    worksheet_sort_proxy_ = new datalab::ui::WorksheetSortFilterProxyModel(this);
    worksheet_sort_proxy_->setSourceModel(worksheet_model_);
    connect(worksheet_model_, &WorksheetModel::table_changed, this,
            [this](const datalab::domain::DataTable& table) {
                table_ = table;
                std::set<int> sortable;
                for (int column = 0; column < static_cast<int>(table.columns.size()); ++column) {
                    sortable.insert(column);
                }
                worksheet_sort_proxy_->set_sortable_columns(sortable);
                refresh_context_dock();
            });
    connect(worksheet_model_, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex&, const QModelIndex&, const QList<int>&) {
                if (suppress_table_edit_undo_) {
                    return;
                }
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
    data_table_ = new WorksheetView(central);
    data_table_->setModel(worksheet_sort_proxy_);
    data_table_->setSortingEnabled(true);
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
    auto* hide_column_action = new QAction(QStringLiteral("隐藏当前列"), data_table_);
    connect(hide_column_action, &QAction::triggered, this, [this]() {
        const QModelIndex current = data_table_->currentIndex();
        if (!current.isValid()) {
            return;
        }
        const int column = worksheet_sort_proxy_->mapToSource(current).column();
        data_table_->setColumnHidden(current.column(), true);
        auto flags = worksheet_model_->column_hidden_flags();
        if (static_cast<std::size_t>(column) >= flags.size()) {
            flags.resize(static_cast<std::size_t>(column) + 1, false);
        }
        flags[static_cast<std::size_t>(column)] = true;
        worksheet_model_->set_column_hidden_flags(flags);
        statusBar()->showMessage(QStringLiteral("已隐藏列 C%1（不改变导入列顺序）。").arg(column + 1));
    });
    data_table_->addAction(hide_column_action);
    auto* show_columns_action = new QAction(QStringLiteral("显示全部列"), data_table_);
    connect(show_columns_action, &QAction::triggered, this, [this]() {
        for (int column = 0; column < worksheet_sort_proxy_->columnCount(); ++column) {
            data_table_->setColumnHidden(column, false);
        }
        worksheet_model_->set_column_hidden_flags(
            std::vector<bool>(worksheet_model_->table().columns.size(), false));
        statusBar()->showMessage(QStringLiteral("已显示全部列。"));
    });
    data_table_->addAction(show_columns_action);
    connect(static_cast<WorksheetView*>(data_table_), &WorksheetView::active_cell_changed,
            this, [this](const QModelIndex& index) {
                if (index.isValid() && cell_address_ != nullptr) {
                    const QModelIndex source = worksheet_sort_proxy_->mapToSource(index);
                    const QVariant row_id =
                        worksheet_model_->data(source, datalab::ui::RowIdRole);
                    cell_address_->setText(
                        QStringLiteral("C%1 · 源行 %2 · RowId %3")
                            .arg(source.column() + 1)
                            .arg(source.row() + 1)
                            .arg(row_id.isValid() ? row_id.toString() : QStringLiteral("-")));
                }
            });
    output_workspace_ = new OutputWorkspace(central);
    output_workspace_->installEventFilter(this);
    connect(output_workspace_, &OutputWorkspace::copy_chart_requested, this, &MainWindow::copy_chart);
    connect(output_workspace_, &OutputWorkspace::rows_selected, this,
            [this](const std::vector<std::size_t>& rows) {
                if (data_table_ == nullptr || data_table_->selectionModel() == nullptr
                    || worksheet_model_ == nullptr || worksheet_sort_proxy_ == nullptr) {
                    return;
                }
                data_table_->selectionModel()->clearSelection();
                for (const std::size_t row : rows) {
                    const QModelIndex source =
                        worksheet_model_->index(static_cast<int>(row), 0);
                    const QModelIndex proxy = worksheet_sort_proxy_->mapFromSource(source);
                    if (proxy.isValid()) {
                        data_table_->selectionModel()->select(
                            proxy, QItemSelectionModel::Select | QItemSelectionModel::Rows);
                    }
                }
            });
    connect(data_table_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this] {
                std::vector<std::size_t> rows;
                if (data_table_ != nullptr && data_table_->selectionModel() != nullptr
                    && worksheet_sort_proxy_ != nullptr) {
                    for (const QModelIndex& index :
                         data_table_->selectionModel()->selectedRows()) {
                        const QModelIndex source = worksheet_sort_proxy_->mapToSource(index);
                        if (source.isValid()) {
                            rows.push_back(static_cast<std::size_t>(source.row()));
                        }
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
    connect(output_workspace_, &OutputWorkspace::page_closed,
            navigator_, &ProjectNavigator::remove_analysis);
    connect(output_workspace_, &OutputWorkspace::pages_changed,
            this, &MainWindow::refresh_context_dock);

    auto* output_pane = new QFrame(central);
    output_pane->setObjectName(QStringLiteral("pane_card"));
    auto* output_layout = new QVBoxLayout(output_pane);
    output_layout->setContentsMargins(14, 10, 14, 14);
    output_layout->setSpacing(6);
    auto* output_heading = new QHBoxLayout();
    auto* output_title = new QLabel(QStringLiteral("分析输出"), output_pane);
    output_title->setObjectName(QStringLiteral("pane_title"));
    auto* output_subtitle = new QLabel(
        QStringLiteral("统计表、图形和诊断结果"), output_pane);
    output_subtitle->setObjectName(QStringLiteral("pane_subtitle"));
    output_heading->addWidget(output_title);
    output_heading->addSpacing(10);
    output_heading->addWidget(output_subtitle);
    output_heading->addStretch(1);
    output_layout->addLayout(output_heading);
    output_layout->addWidget(output_workspace_, 1);

    auto* worksheet_pane = new QFrame(central);
    worksheet_pane->setObjectName(QStringLiteral("pane_card"));
    auto* worksheet_layout = new QVBoxLayout(worksheet_pane);
    worksheet_layout->setContentsMargins(14, 10, 14, 14);
    worksheet_layout->setSpacing(6);
    auto* worksheet_heading = new QHBoxLayout();
    auto* worksheet_title = new QLabel(QStringLiteral("活动工作表"), worksheet_pane);
    worksheet_title->setObjectName(QStringLiteral("pane_title"));
    auto* worksheet_subtitle = new QLabel(
        QStringLiteral("编辑数据，分析当前工作表"), worksheet_pane);
    worksheet_subtitle->setObjectName(QStringLiteral("pane_subtitle"));
    cell_address_ = new QLabel(QStringLiteral("C1 · 1"), worksheet_pane);
    cell_address_->setMinimumWidth(64);
    cell_address_->setObjectName(QStringLiteral("pane_subtitle"));
    worksheet_heading->addWidget(worksheet_title);
    worksheet_heading->addSpacing(10);
    worksheet_heading->addWidget(worksheet_subtitle);
    worksheet_heading->addStretch(1);
    worksheet_heading->addWidget(cell_address_);
    worksheet_layout->addLayout(worksheet_heading);
    worksheet_filter_edit_ = new QLineEdit(worksheet_pane);
    worksheet_filter_edit_->setPlaceholderText(
        QStringLiteral("筛选工作表（白名单列本地过滤，不改动原始行映射）"));
    connect(worksheet_filter_edit_, &QLineEdit::textChanged, this,
            [this](const QString& text) {
                if (worksheet_sort_proxy_ != nullptr) {
                    worksheet_sort_proxy_->set_text_filter(text);
                }
            });
    worksheet_layout->addWidget(worksheet_filter_edit_);
    worksheet_layout->addWidget(data_table_, 1);
    output_pane->setMinimumHeight(280);
    worksheet_pane->setMinimumHeight(280);

    auto* workspace = new QSplitter(Qt::Vertical, central);
    workspace->setObjectName(QStringLiteral("main_workspace"));
    workspace->addWidget(output_pane);
    workspace->addWidget(worksheet_pane);
    workspace->setStretchFactor(0, 1);
    workspace->setStretchFactor(1, 1);
    workspace->setChildrenCollapsible(false);
    workspace->setSizes({400, 380});
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
    refresh_context_dock();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == output_workspace_ && event->type() == QEvent::KeyPress) {
        auto* key_event = static_cast<QKeyEvent*>(event);
        if (key_event->matches(QKeySequence::Copy)) {
            if (QTableView* table = output_table_view_from_focus(output_workspace_)) {
                if (trigger_output_table_copy(table)) {
                    statusBar()->showMessage(QStringLiteral("已复制表格到剪贴板。"));
                    return true;
                }
            }
            if (AnalysisChartWidget* chart = chart_widget_from_focus()) {
                statusBar()->showMessage(chart->copy_to_clipboard()
                        ? QStringLiteral("已复制图形到剪贴板。")
                        : QStringLiteral("当前图形暂时无法复制。"));
                return true;
            }
            if (AnalysisChartWidget* chart =
                    chart_widget_on_page(output_workspace_->currentWidget(), output_workspace_)) {
                statusBar()->showMessage(chart->copy_to_clipboard()
                        ? QStringLiteral("已复制图形到剪贴板。")
                        : QStringLiteral("当前图形暂时无法复制。"));
                return true;
            }
        }
    }
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
    if (data_tables_equal(before, after)) {
        return;
    }
    undo_stack_->push(
        new TableChangeCommand(worksheet_model_, before, after, text, already_applied));
}

void MainWindow::copy_selection()
{
    const QWidget* focus = QApplication::focusWidget();
    const bool worksheet_focused = data_table_ != nullptr
        && (focus == data_table_ || data_table_->isAncestorOf(focus));
    if (!worksheet_focused) {
        if (output_workspace_ != nullptr) {
            if (QTableView* table = output_table_view_from_focus(output_workspace_)) {
                if (trigger_output_table_copy(table)) {
                    statusBar()->showMessage(QStringLiteral("已复制表格到剪贴板。"));
                    return;
                }
            }
        }
        if (AnalysisChartWidget* chart = chart_widget_from_focus()) {
            statusBar()->showMessage(chart->copy_to_clipboard()
                    ? QStringLiteral("已复制图形到剪贴板。")
                    : QStringLiteral("当前图形暂时无法复制。"));
            return;
        }
        if (output_workspace_ != nullptr) {
            if (AnalysisChartWidget* chart =
                    chart_widget_on_page(output_workspace_->currentWidget(), output_workspace_)) {
                statusBar()->showMessage(chart->copy_to_clipboard()
                        ? QStringLiteral("已复制图形到剪贴板。")
                        : QStringLiteral("当前图形暂时无法复制。"));
                return;
            }
        }
    }
    if (data_table_ == nullptr || data_table_->selectionModel() == nullptr) {
        return;
    }
    const QModelIndexList indexes = data_table_->selectionModel()->selectedIndexes();
    if (indexes.isEmpty()) {
        return;
    }
    QModelIndexList source_indexes;
    source_indexes.reserve(indexes.size());
    for (const QModelIndex& index : indexes) {
        source_indexes.push_back(
            worksheet_sort_proxy_ != nullptr
                ? worksheet_sort_proxy_->mapToSource(index)
                : index);
    }
    const QString text = worksheet_model_->selection_tsv(source_indexes, true);
    QApplication::clipboard()->setText(text);
    statusBar()->showMessage(QStringLiteral("已复制选区（含表头）。"));
}

void MainWindow::copy_chart()
{
    AnalysisChartWidget* chart = chart_widget_from_focus();
    if (chart == nullptr && output_workspace_ != nullptr) {
        chart = chart_widget_on_page(output_workspace_->currentWidget(), output_workspace_);
    }
    if (chart == nullptr) {
        statusBar()->showMessage(QStringLiteral("当前输出页没有可复制的图形。"));
        return;
    }
    statusBar()->showMessage(chart->copy_to_clipboard()
            ? QStringLiteral("已复制图形到剪贴板。")
            : QStringLiteral("当前图形暂时无法复制。"));
}

void MainWindow::cut_selection()
{
    if (data_table_ == nullptr || data_table_->selectionModel() == nullptr
        || data_table_->selectionModel()->selectedIndexes().isEmpty()) {
        return;
    }
    copy_selection();
    const datalab::domain::DataTable before = table_;
    const QModelIndexList indexes = data_table_->selectionModel()->selectedIndexes();
    QModelIndexList source_indexes;
    for (const QModelIndex& index : indexes) {
        source_indexes.push_back(worksheet_sort_proxy_->mapToSource(index));
    }
    suppress_table_edit_undo_ = true;
    if (!worksheet_model_->clear_cells(source_indexes)) {
        suppress_table_edit_undo_ = false;
        return;
    }
    suppress_table_edit_undo_ = false;
    table_ = worksheet_model_->table();
    push_table_change(before, table_, QStringLiteral("剪切单元格"));
}

void MainWindow::clear_selection()
{
    if (data_table_ == nullptr || data_table_->selectionModel() == nullptr) {
        return;
    }
    auto* view = static_cast<WorksheetView*>(data_table_);
    if (view->is_editing()) {
        view->commit_editing();
    }
    const QModelIndexList selected = data_table_->selectionModel()->selectedIndexes();
    QModelIndexList indexes = selected;
    if (indexes.isEmpty() && data_table_->currentIndex().isValid()) {
        indexes.push_back(data_table_->currentIndex());
    }
    if (indexes.isEmpty()) {
        return;
    }
    QModelIndexList source_indexes;
    for (const QModelIndex& index : indexes) {
        source_indexes.push_back(worksheet_sort_proxy_->mapToSource(index));
    }
    const datalab::domain::DataTable before = table_;
    suppress_table_edit_undo_ = true;
    if (!worksheet_model_->clear_cells(source_indexes)) {
        suppress_table_edit_undo_ = false;
        statusBar()->showMessage(QStringLiteral("所选单元格已为空。"));
        return;
    }
    suppress_table_edit_undo_ = false;
    table_ = worksheet_model_->table();
    push_table_change(before, table_, QStringLiteral("清除单元格"));
    statusBar()->showMessage(QStringLiteral("已清除 %1 个单元格。").arg(indexes.size()));
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
    if (!confirm_discard_output()) {
        return;
    }
    table_ = {};
    cleaning_operations_.clear();
    undo_stack_->clear();
    worksheet_model_->set_table(table_);
    worksheet_model_->set_excluded_rows({});
    worksheet_model_->set_hidden_rows({});
    output_workspace_->clear_pages();
    output_workspace_->set_selected_source_rows({});
    navigator_->clear_contents();
    navigator_->set_project_name(QStringLiteral("DataLab 项目"));
    refresh_context_dock();
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
    undo_stack_->clear();
    output_workspace_->clear_pages();
    output_workspace_->set_selected_source_rows({});
    navigator_->clear_contents();
    navigator_->set_project_name(QString::fromStdString(table_.name.empty() ? "DataLab 项目" : table_.name));
    if (!table_.name.empty()) {
        navigator_->add_worksheet(QString::fromStdString(table_.name));
    }
    for (const auto& page : pages) {
        output_workspace_->add_page(page);
        navigator_->add_analysis(QString::fromStdString(page.id), QString::fromStdString(page.title));
    }
    refresh_context_dock();
    statusBar()->showMessage(QStringLiteral("项目已打开。"));
}

void MainWindow::import_database()
{
    datalab::ui::DatabaseImportWizard wizard(this);
    if (wizard.exec() != QDialog::Accepted) {
        return;
    }
    const auto imported = wizard.imported_table();
    if (!imported.has_value()) {
        return;
    }
    if (!confirm_discard_output()) {
        return;
    }
    cleaning_operations_.clear();
    undo_stack_->clear();
    output_workspace_->clear_pages();
    output_workspace_->set_selected_source_rows({});
    navigator_->clear_contents();
    table_ = *imported;
    display_table();
    navigator_->add_worksheet(QString::fromStdString(table_.name));
    statusBar()->showMessage(
        QStringLiteral("已从数据库导入 %1 行：%2。")
            .arg(static_cast<qulonglong>(table_.rows.size()))
            .arg(QString::fromStdString(table_.name)));
}

void MainWindow::import_data()
{
    const QString file_path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("导入数据"),
        QString(),
        QStringLiteral("数据文件 (*.csv *.txt *.xlsx);;所有文件 (*.*)"));
    if (file_path.isEmpty()) {
        return;
    }
    if (!confirm_discard_output()) {
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
        undo_stack_->clear();
        output_workspace_->clear_pages();
        output_workspace_->set_selected_source_rows({});
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
    worksheet_model_->set_hidden_rows(hidden_rows());
    data_table_->resizeColumnsToContents();
    refresh_context_dock();
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
    configuration.hidden_rows = hidden_rows();
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

std::vector<std::size_t> MainWindow::hidden_rows() const
{
    std::vector<std::size_t> rows;
    for (const auto& operation : cleaning_operations_) {
        if (operation.operation != "hide_row") {
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
    if (enriched_page.worksheet_export.has_value()) {
        const bool censoring_sheet =
            enriched_page.worksheet_export->name == "censoring_observations";
        const QString title = censoring_sheet
            ? QStringLiteral("写入删失状态工作表")
            : QStringLiteral("写入设计工作表");
        const QString body = censoring_sheet
            ? QStringLiteral(
                  "已生成逐观测删失状态工作表（%1 行，含 censoring_type）。"
                  "是否替换当前活动工作表以便审计/再分析？")
                  .arg(static_cast<qulonglong>(
                      enriched_page.worksheet_export->rows.size()))
            : QStringLiteral(
                  "已生成设计矩阵工作表（%1 行）。是否替换当前活动工作表以便录入 Response？")
                  .arg(static_cast<qulonglong>(
                      enriched_page.worksheet_export->rows.size()));
        const auto answer = QMessageBox::question(
            this,
            title,
            body,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (answer == QMessageBox::Yes) {
            const datalab::domain::DataTable before = table_;
            table_ = *enriched_page.worksheet_export;
            worksheet_model_->set_table(table_);
            worksheet_model_->set_excluded_rows({});
            worksheet_model_->set_hidden_rows({});
            push_table_change(
                before,
                table_,
                censoring_sheet ? QStringLiteral("删失状态写入工作表")
                                : QStringLiteral("DOE 设计写入工作表"),
                true);
            navigator_->add_worksheet(QString::fromStdString(table_.name));
            refresh_context_dock();
        }
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
        command->dialog_title.isEmpty()
            ? datalab::ui::ui_tr(command->menu_label)
            : datalab::ui::ui_tr(command->dialog_title),
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

void MainWindow::hide_selected_row()
{
    const QModelIndexList selected_rows = data_table_->selectionModel()->selectedRows();
    if (selected_rows.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择行"),
                                 QStringLiteral("请先在工作表中选择要隐藏的行（仅影响显示，不排除分析）。"));
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
    after.push_back({"hide_row", "用户隐藏选中行（仅显示；分析仍纳入）。", rows});
    undo_stack_->push(new CleaningChangeCommand(
        [this](const std::vector<datalab::domain::CleaningOperation>& operations) {
            restore_cleaning_operations(operations);
        },
        before, std::move(after), QStringLiteral("隐藏行")));
    statusBar()->showMessage(
        QStringLiteral("已标记隐藏 %1 行（仅显示；分析仍纳入）。")
            .arg(static_cast<qulonglong>(rows.size())));
}

void MainWindow::clear_hidden_rows()
{
    const std::vector<datalab::domain::CleaningOperation> before = cleaning_operations_;
    std::vector<datalab::domain::CleaningOperation> after = before;
    after.erase(
        std::remove_if(after.begin(), after.end(),
                       [](const datalab::domain::CleaningOperation& operation) {
                           return operation.operation == "hide_row";
                       }),
        after.end());
    undo_stack_->push(new CleaningChangeCommand(
        [this](const std::vector<datalab::domain::CleaningOperation>& operations) {
            restore_cleaning_operations(operations);
        },
        before, std::move(after), QStringLiteral("清除隐藏标记")));
    statusBar()->showMessage(QStringLiteral("已清除隐藏标记，原始数据未修改。"));
}

void MainWindow::restore_cleaning_operations(
    const std::vector<datalab::domain::CleaningOperation>& operations)
{
    cleaning_operations_ = operations;
    worksheet_model_->set_excluded_rows(excluded_rows());
    worksheet_model_->set_hidden_rows(hidden_rows());
    refresh_context_dock();
}

void MainWindow::refresh_context_dock()
{
    if (context_status_ == nullptr || context_detail_ == nullptr || context_next_ == nullptr) {
        return;
    }
    const bool has_data = !table_.columns.empty();
    const std::size_t excluded = excluded_rows().size();
    const std::size_t hidden = hidden_rows().size();
    if (!has_data) {
        context_status_->setText(QStringLiteral("尚未导入数据"));
        context_status_->setStyleSheet(QStringLiteral(
            "background: #fff8e6; color: #8a5a00; padding: 12px;"
            " border: 1px solid #ead7a0; border-radius: 7px;"));
        context_detail_->setText(QStringLiteral(
            "导入 CSV 或 Excel 后，这里显示当前工作表的行数、列数、排除与隐藏状态。"));
        context_next_->setText(QStringLiteral(
            "下一步\n导入数据后，从“统计 / 图形 / 质量工具”菜单选择方法。"));
        return;
    }
    context_status_->setText(QStringLiteral("已准备分析\n%1 行 · %2 列")
        .arg(static_cast<qulonglong>(table_.rows.size()))
        .arg(static_cast<qulonglong>(table_.columns.size())));
    context_status_->setStyleSheet(QStringLiteral(
        "background: #e8f6f2; color: #18794e; padding: 12px;"
        " border: 1px solid #c7e9dc; border-radius: 7px;"));
    const QString sheet_name = table_.name.empty()
        ? QStringLiteral("未命名工作表")
        : QString::fromStdString(table_.name);
    context_detail_->setText(
        QStringLiteral("当前工作表「%1」。已排除 %2 行；已隐藏 %3 行（仅显示，分析仍纳入）。")
            .arg(sheet_name)
            .arg(static_cast<qulonglong>(excluded))
            .arg(static_cast<qulonglong>(hidden)));
    if (output_workspace_ != nullptr && output_workspace_->has_pages()) {
        context_next_->setText(QStringLiteral(
            "下一步\n查看分析输出，或从“文件”导出 PDF 报告。"));
    } else {
        context_next_->setText(QStringLiteral(
            "下一步\n从“统计 / 图形 / 质量工具”选择分析方法。"));
    }
}

bool MainWindow::confirm_discard_output()
{
    if (output_workspace_ == nullptr || !output_workspace_->has_pages()) {
        return true;
    }
    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("丢弃分析结果"),
        QStringLiteral("当前项目已有分析输出。继续将丢弃这些结果。是否继续？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    return answer == QMessageBox::Yes;
}

void MainWindow::export_pdf()
{
    if (!output_workspace_->has_pages()) {
        QMessageBox::information(this, QStringLiteral("没有分析"), QStringLiteral("请先运行分析。"));
        return;
    }

    ReportTemplateDialog template_dialog(this);
    if (template_dialog.exec() != QDialog::Accepted) {
        return;
    }

    datalab::application::ReportAssemblyOptions options;
    options.software_version = "DataLab";
    options.generated_at_utc =
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
    const datalab::domain::ReportDocument assembled =
        datalab::application::build_report_document(
            table_,
            output_workspace_->pages(),
            template_dialog.selected_profile(),
            options);
    const datalab::application::ReportLocalizationResult localized =
        datalab::application::localize_report_document(assembled);
    const datalab::domain::ReportDocument& document = localized.document;

    ReportPreviewDialog preview(document, this);
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

    const datalab::application::ReportExportPaths paths =
        datalab::application::make_report_export_paths(
            QFileInfo(file_path).fileName().toStdString());
    datalab::domain::ReportExportManifest manifest =
        datalab::application::build_export_manifest(document, paths);
    std::string mismatch;
    if (!datalab::application::manifest_matches_document(manifest, document, &mismatch)) {
        QMessageBox::critical(
            this,
            QStringLiteral("导出失败"),
            QStringLiteral("报告与 manifest 一致性校验失败：%1")
                .arg(QString::fromStdString(mismatch)));
        return;
    }
    manifest.consistency_status = "ok";

    const datalab::infrastructure::ReportExportResult exported =
        datalab::infrastructure::export_report_package(
            file_path,
            document,
            manifest,
            [&document](datalab::domain::ReportExportManifest& mutable_manifest,
                        const QString& temp_pdf) {
                const datalab::domain::ExternalPdfaValidatorResult external =
                    datalab::infrastructure::run_optional_verapdf(temp_pdf);
                auto assessment = datalab::application::merge_external_pdfa_validator_result(
                    datalab::application::assess_pdf_export_pipeline(),
                    external);
                const datalab::domain::ExternalPdfaValidatorResult pac =
                    datalab::infrastructure::run_optional_pac(temp_pdf);
                assessment = datalab::application::merge_optional_pac_result(
                    std::move(assessment), pac);
                datalab::application::apply_pdf_compliance_assessment(
                    mutable_manifest, assessment);
                std::string post_reason;
                if (!datalab::application::manifest_matches_document(
                        mutable_manifest, document, &post_reason)) {
                    // Force honest fallback rather than writing a forbidden claim.
                    datalab::application::apply_pdf_compliance_assessment(
                        mutable_manifest,
                        datalab::application::assess_pdf_export_pipeline());
                    mutable_manifest.validator_notes +=
                        " Post-validator manifest rejected (" + post_reason
                        + "); reset to not_validated/unsupported.";
                }
            });
    if (!exported.ok) {
        QMessageBox::critical(
            this,
            QStringLiteral("导出失败"),
            exported.error_message.isEmpty() ? QStringLiteral("导出失败。")
                                             : exported.error_message);
        return;
    }

    navigator_->add_report(QStringLiteral("PDF 报告"));
    statusBar()->showMessage(
        QStringLiteral("PDF / audit JSON / manifest 已导出（PDF/A=%1，PDF/UA=%2）。")
            .arg(QString::fromStdString(
                datalab::domain::pdf_compliance_status_id(exported.manifest.pdfa_status)))
            .arg(QString::fromStdString(
                datalab::domain::pdf_compliance_status_id(exported.manifest.pdfua_status))));
}
