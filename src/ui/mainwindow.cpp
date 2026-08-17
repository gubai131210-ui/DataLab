#include "mainwindow.h"

#include "application/analysis_service.h"
#include "application/analysis_catalog.h"
#include "application/interpretation_service.h"
#include "infrastructure/csv_importer.h"
#include "infrastructure/pdf_report_writer.h"
#include "infrastructure/project_repository.h"
#include "infrastructure/python_table_importer.h"
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
    auto connect_analysis = [this](const QString& id, const QString& text) {
        QAction* action = commands_->add(id, text);
        connect(action, &QAction::triggered, this, [this, id] {
            run_analysis(id);
        });
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
    for (const auto& descriptor : datalab::application::AnalysisCatalog::all()) {
        connect_analysis(descriptor.id, descriptor.label);
    }
    const QHash<QString, QString> analysis_icons = {
        {QStringLiteral("descriptive"), QStringLiteral("descriptive")},
        {QStringLiteral("normality_test"), QStringLiteral("normality_test")},
        {QStringLiteral("correlation"), QStringLiteral("correlation")},
        {QStringLiteral("one_sample_t"), QStringLiteral("one_sample_t")},
        {QStringLiteral("two_sample_t"), QStringLiteral("two_sample_t")},
        {QStringLiteral("one_way_anova"), QStringLiteral("one_way_anova")},
        {QStringLiteral("paired_t"), QStringLiteral("paired_t")},
        {QStringLiteral("regression"), QStringLiteral("regression")},
        {QStringLiteral("two_proportions"), QStringLiteral("two_proportions")},
        {QStringLiteral("chi_square"), QStringLiteral("chi_square")},
        {QStringLiteral("mann_whitney"), QStringLiteral("mann_whitney")},
        {QStringLiteral("wilcoxon_signed_rank"), QStringLiteral("wilcoxon_signed_rank")},
        {QStringLiteral("kruskal_wallis"), QStringLiteral("kruskal_wallis")},
        {QStringLiteral("box_cox"), QStringLiteral("box_cox")},
        {QStringLiteral("gage_rr"), QStringLiteral("gage_rr")},
        {QStringLiteral("msa_type1"), QStringLiteral("gage_rr")},
        {QStringLiteral("imr"), QStringLiteral("imr")},
        {QStringLiteral("xbar_r"), QStringLiteral("xbar_r")},
        {QStringLiteral("xbar_s"), QStringLiteral("xbar_s")},
        {QStringLiteral("p_chart"), QStringLiteral("p_chart")},
        {QStringLiteral("np_chart"), QStringLiteral("np_chart")},
        {QStringLiteral("c_chart"), QStringLiteral("c_chart")},
        {QStringLiteral("u_chart"), QStringLiteral("u_chart")},
        {QStringLiteral("laney_p_chart"), QStringLiteral("laney_p_chart")},
        {QStringLiteral("laney_u_chart"), QStringLiteral("laney_u_chart")},
        {QStringLiteral("ewma"), QStringLiteral("ewma")},
        {QStringLiteral("cusum"), QStringLiteral("cusum")},
        {QStringLiteral("time_series_smoothing"), QStringLiteral("time_series_smoothing")},
        {QStringLiteral("arima"), QStringLiteral("arima")},
        {QStringLiteral("two_factor_anova"), QStringLiteral("two_factor_anova")},
        {QStringLiteral("logistic_regression"), QStringLiteral("logistic_regression")},
        {QStringLiteral("variance_test"), QStringLiteral("variance_test")},
        {QStringLiteral("time_series_decomposition"), QStringLiteral("time_series_decomposition")},
        {QStringLiteral("seasonal_forecasting"), QStringLiteral("seasonal_forecasting")},
        {QStringLiteral("pca"), QStringLiteral("pca")},
        {QStringLiteral("doe_factorial"), QStringLiteral("doe_factorial")},
        {QStringLiteral("nested_gage_rr"), QStringLiteral("nested_gage_rr")},
        {QStringLiteral("attribute_agreement"), QStringLiteral("attribute_agreement")},
        {QStringLiteral("capability"), QStringLiteral("capability")},
        {QStringLiteral("capability_sixpack"), QStringLiteral("capability_sixpack")},
        {QStringLiteral("histogram"), QStringLiteral("histogram")},
        {QStringLiteral("boxplot"), QStringLiteral("boxplot")},
        {QStringLiteral("pareto"), QStringLiteral("pareto")}};
    for (auto iterator = analysis_icons.cbegin(); iterator != analysis_icons.cend();
         ++iterator) {
        set_icon(iterator.key(), iterator.value());
    }
    for (const auto& descriptor : datalab::application::AnalysisCatalog::all()) {
        QAction* action = commands_->get(descriptor.id);
        if (action != nullptr && action->icon().isNull()) {
            set_icon(descriptor.id, QStringLiteral("data-table"));
        }
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

    auto* statistics_menu = menuBar()->addMenu(QStringLiteral("统计"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("descriptive"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("normality_test"));
    statistics_menu->addSeparator();
    commands_->add_to_menu(statistics_menu, QStringLiteral("correlation"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("one_sample_t"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("two_sample_t"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("one_way_anova"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("paired_t"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("regression"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("two_proportions"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("chi_square"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("mann_whitney"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("wilcoxon_signed_rank"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("kruskal_wallis"));

    auto* graph_menu = menuBar()->addMenu(QStringLiteral("图形"));
    commands_->add_to_menu(graph_menu, QStringLiteral("histogram"));
    commands_->add_to_menu(graph_menu, QStringLiteral("boxplot"));
    commands_->add_to_menu(graph_menu, QStringLiteral("pareto"));

    auto* control_menu = menuBar()->addMenu(QStringLiteral("控制图"));
    commands_->add_to_menu(control_menu, QStringLiteral("imr"));
    commands_->add_to_menu(control_menu, QStringLiteral("xbar_r"));
    commands_->add_to_menu(control_menu, QStringLiteral("xbar_s"));
    control_menu->addSeparator();
    commands_->add_to_menu(control_menu, QStringLiteral("p_chart"));
    commands_->add_to_menu(control_menu, QStringLiteral("np_chart"));
    commands_->add_to_menu(control_menu, QStringLiteral("c_chart"));
    commands_->add_to_menu(control_menu, QStringLiteral("u_chart"));
    control_menu->addSeparator();
    commands_->add_to_menu(control_menu, QStringLiteral("laney_p_chart"));
    commands_->add_to_menu(control_menu, QStringLiteral("laney_u_chart"));
    commands_->add_to_menu(control_menu, QStringLiteral("ewma"));
    commands_->add_to_menu(control_menu, QStringLiteral("cusum"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("time_series_smoothing"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("arima"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("two_factor_anova"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("logistic_regression"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("variance_test"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("time_series_decomposition"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("seasonal_forecasting"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("pca"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("reliability"));
    commands_->add_to_menu(statistics_menu, QStringLiteral("t_power"));

    auto* quality_menu = menuBar()->addMenu(QStringLiteral("质量工具"));
    commands_->add_to_menu(quality_menu, QStringLiteral("capability"));
    commands_->add_to_menu(quality_menu, QStringLiteral("capability_sixpack"));
    commands_->add_to_menu(quality_menu, QStringLiteral("box_cox"));
    commands_->add_to_menu(quality_menu, QStringLiteral("gage_rr"));
    commands_->add_to_menu(quality_menu, QStringLiteral("msa_type1"));
    commands_->add_to_menu(quality_menu, QStringLiteral("nested_gage_rr"));
    commands_->add_to_menu(quality_menu, QStringLiteral("attribute_agreement"));
    commands_->add_to_menu(quality_menu, QStringLiteral("doe_factorial"));
    commands_->add_to_menu(quality_menu, QStringLiteral("doe_response"));
    commands_->add_to_menu(quality_menu, QStringLiteral("doe"));

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
    const bool is_excel = file_path.endsWith(QStringLiteral(".xlsx"), Qt::CaseInsensitive)
        || file_path.endsWith(QStringLiteral(".xls"), Qt::CaseInsensitive);
    const auto imported = is_excel
        ? datalab::infrastructure::PythonTableImporter::import_file(file_path, &error_message)
        : datalab::infrastructure::CsvImporter::import_file(file_path, &error_message);
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

void MainWindow::run_analysis(const QString& id)
{
    if (id == QStringLiteral("descriptive")) {
        run_descriptive();
    } else if (id == QStringLiteral("normality_test")) {
        run_normality_test();
    } else if (id == QStringLiteral("correlation")) {
        run_correlation();
    } else if (id == QStringLiteral("one_sample_t")) {
        run_one_sample_t();
    } else if (id == QStringLiteral("two_sample_t")) {
        run_two_sample_t();
    } else if (id == QStringLiteral("one_way_anova")) {
        run_one_way_anova();
    } else if (id == QStringLiteral("paired_t")) {
        run_paired_t();
    } else if (id == QStringLiteral("regression")) {
        run_regression();
    } else if (id == QStringLiteral("two_proportions")) {
        run_two_proportions();
    } else if (id == QStringLiteral("chi_square")) {
        run_chi_square();
    } else if (id == QStringLiteral("mann_whitney")) {
        run_mann_whitney();
    } else if (id == QStringLiteral("wilcoxon_signed_rank")) {
        run_wilcoxon_signed_rank();
    } else if (id == QStringLiteral("kruskal_wallis")) {
        run_kruskal_wallis();
    } else if (id == QStringLiteral("box_cox")) {
        run_box_cox();
    } else if (id == QStringLiteral("gage_rr")) {
        run_gage_rr();
    } else if (id == QStringLiteral("msa_type1")) {
        run_msa_type1();
    } else if (id == QStringLiteral("imr")) {
        run_imr();
    } else if (id == QStringLiteral("xbar_r")) {
        run_xbar_r();
    } else if (id == QStringLiteral("xbar_s")) {
        run_xbar_s();
    } else if (id == QStringLiteral("p_chart")) {
        run_p_chart();
    } else if (id == QStringLiteral("np_chart")) {
        run_np_chart();
    } else if (id == QStringLiteral("c_chart")) {
        run_c_chart();
    } else if (id == QStringLiteral("u_chart")) {
        run_u_chart();
    } else if (id == QStringLiteral("laney_p_chart")) {
        run_laney_p_chart();
    } else if (id == QStringLiteral("laney_u_chart")) {
        run_laney_u_chart();
    } else if (id == QStringLiteral("ewma")) {
        run_ewma();
    } else if (id == QStringLiteral("cusum")) {
        run_cusum();
    } else if (id == QStringLiteral("time_series_smoothing")) {
        run_time_series_smoothing();
    } else if (id == QStringLiteral("arima")) {
        run_arima();
    } else if (id == QStringLiteral("two_factor_anova")) {
        run_two_factor_anova();
    } else if (id == QStringLiteral("logistic_regression")) {
        run_logistic_regression();
    } else if (id == QStringLiteral("variance_test")) {
        run_variance_test();
    } else if (id == QStringLiteral("time_series_decomposition")) {
        run_time_series_decomposition();
    } else if (id == QStringLiteral("seasonal_forecasting")) {
        run_seasonal_forecasting();
    } else if (id == QStringLiteral("pca")) {
        run_pca();
    } else if (id == QStringLiteral("reliability")) {
        run_reliability();
    } else if (id == QStringLiteral("t_power")) {
        run_t_power();
    } else if (id == QStringLiteral("nested_gage_rr")) {
        run_nested_gage_rr();
    } else if (id == QStringLiteral("attribute_agreement")) {
        run_attribute_agreement();
    } else if (id == QStringLiteral("doe_factorial")
               || id == QStringLiteral("doe_response")) {
        run_doe_factorial();
    } else if (id == QStringLiteral("capability")) {
        run_capability();
    } else if (id == QStringLiteral("capability_sixpack")) {
        run_capability_sixpack();
    } else if (id == QStringLiteral("histogram")) {
        run_histogram();
    } else if (id == QStringLiteral("boxplot")) {
        run_boxplot();
    } else if (id == QStringLiteral("pareto")) {
        run_pareto();
    }
}

void MainWindow::run_descriptive()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("显示描述性统计"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("变量"), true, false);
    dialog.add_role(QStringLiteral("by"), QStringLiteral("By 变量"), false, true);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "显示描述性统计";
    configuration.chart_type = "descriptive";
    for (const int index : dialog.role_indices(QStringLiteral("variables"))) {
        configuration.variable_columns.push_back(static_cast<std::size_t>(index));
    }
    if (dialog.first_role_index(QStringLiteral("by")) >= 0) {
        configuration.by_column = static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("by")));
    }
    if (configuration.variable_columns.empty()) {
        QMessageBox::information(this, QStringLiteral("未选择变量"), QStringLiteral("请从左侧选择至少一列到“变量”。"));
        return;
    }
    publish_page(datalab::application::AnalysisService::descriptive(table_, configuration));
}

void MainWindow::run_normality_test()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("正态性检验"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("变量"), false, false);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    if (column < 0) {
        QMessageBox::information(this, QStringLiteral("未选择变量"),
                                 QStringLiteral("请选择测量值列。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "正态性检验";
    configuration.chart_type = "normality_test";
    configuration.variable_columns.push_back(static_cast<std::size_t>(column));
    configuration.selection.measurement_column = static_cast<std::size_t>(column);
    publish_page(datalab::application::AnalysisService::normality_test(table_, configuration));
}

void MainWindow::run_correlation()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("相关分析"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("变量（至少两列）"), true, false);
    dialog.add_line_edit(QStringLiteral("method"), QStringLiteral("方法"),
                         QStringLiteral("pearson 或 spearman"));
    dialog.add_line_edit(QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"),
                         QStringLiteral("95"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QList<int> columns = dialog.role_indices(QStringLiteral("variables"));
    if (columns.size() < 2) {
        QMessageBox::information(this, QStringLiteral("变量不足"),
                                 QStringLiteral("请选择至少两列数值变量。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "相关分析";
    configuration.chart_type = "correlation";
    for (const int column : columns) {
        configuration.variable_columns.push_back(static_cast<std::size_t>(column));
    }
    configuration.correlation_method = dialog.line_text(QStringLiteral("method")).trimmed()
        .toLower().toStdString();
    if (configuration.correlation_method != "spearman") {
        configuration.correlation_method = "pearson";
    }
    const double confidence = dialog.line_number(QStringLiteral("confidence")).value_or(95.0);
    configuration.confidence_level = confidence > 1.0 ? confidence / 100.0 : confidence;
    publish_page(datalab::application::AnalysisService::correlation(table_, configuration));
}

void MainWindow::run_one_sample_t()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("单样本 t 检验"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("测量值"), false, false);
    dialog.add_line_edit(QStringLiteral("hypothesis_mean"), QStringLiteral("假设均值"),
                         QStringLiteral("例如 10"));
    dialog.add_line_edit(QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"),
                         QStringLiteral("95"));
    dialog.add_line_edit(QStringLiteral("alternative"), QStringLiteral("备择方向"),
                         QStringLiteral("two_sided / less / greater"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    const auto hypothesis = dialog.line_number(QStringLiteral("hypothesis_mean"));
    if (column < 0 || !hypothesis.has_value()) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("请选择测量列并输入假设均值。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "单样本 t 检验";
    configuration.chart_type = "one_sample_t";
    configuration.variable_columns.push_back(static_cast<std::size_t>(column));
    configuration.selection.measurement_column = static_cast<std::size_t>(column);
    configuration.hypothesis_mean = hypothesis;
    configuration.confidence_level = dialog.line_number(QStringLiteral("confidence")).value_or(95.0);
    if (configuration.confidence_level > 1.0) {
        configuration.confidence_level /= 100.0;
    }
    configuration.alternative = dialog.line_text(QStringLiteral("alternative")).trimmed()
        .toLower().toStdString();
    publish_page(datalab::application::AnalysisService::one_sample_t(table_, configuration));
}

void MainWindow::run_two_sample_t()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("双样本 t 检验"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("两列独立样本"), true, false);
    dialog.add_line_edit(QStringLiteral("variance"), QStringLiteral("方差方法"),
                         QStringLiteral("welch 或 pooled"));
    dialog.add_line_edit(QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"),
                         QStringLiteral("95"));
    dialog.add_line_edit(QStringLiteral("alternative"), QStringLiteral("备择方向"),
                         QStringLiteral("two_sided / less / greater"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QList<int> columns = dialog.role_indices(QStringLiteral("variables"));
    if (columns.size() != 2) {
        QMessageBox::information(this, QStringLiteral("变量数量错误"),
                                 QStringLiteral("请选择正好两列独立样本变量。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "双样本 t 检验";
    configuration.chart_type = "two_sample_t";
    configuration.variable_columns = {
        static_cast<std::size_t>(columns[0]), static_cast<std::size_t>(columns[1])};
    configuration.variance_method = dialog.line_text(QStringLiteral("variance")).trimmed()
        .toLower().toStdString();
    if (configuration.variance_method != "pooled") {
        configuration.variance_method = "welch";
    }
    configuration.confidence_level = dialog.line_number(QStringLiteral("confidence")).value_or(95.0);
    if (configuration.confidence_level > 1.0) {
        configuration.confidence_level /= 100.0;
    }
    configuration.alternative = dialog.line_text(QStringLiteral("alternative")).trimmed()
        .toLower().toStdString();
    publish_page(datalab::application::AnalysisService::two_sample_t(table_, configuration));
}

void MainWindow::run_one_way_anova()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("单因素 ANOVA"), column_labels(), this);
    dialog.add_role(QStringLiteral("response"), QStringLiteral("响应变量"), false, false);
    dialog.add_role(QStringLiteral("factor"), QStringLiteral("因子/分组列"), false, false);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int response = dialog.first_role_index(QStringLiteral("response"));
    const int factor = dialog.first_role_index(QStringLiteral("factor"));
    if (response < 0 || factor < 0) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("请选择响应变量和因子/分组列。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "单因素 ANOVA";
    configuration.chart_type = "one_way_anova";
    configuration.variable_columns.push_back(static_cast<std::size_t>(response));
    configuration.selection.measurement_column = static_cast<std::size_t>(response);
    configuration.by_column = static_cast<std::size_t>(factor);
    publish_page(datalab::application::AnalysisService::one_way_anova(table_, configuration));
}

void MainWindow::run_paired_t()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("配对 t 检验"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("配对变量（两列）"), true, false);
    dialog.add_line_edit(QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"),
                         QStringLiteral("95"));
    dialog.add_line_edit(QStringLiteral("alternative"), QStringLiteral("备择方向"),
                         QStringLiteral("two_sided / less / greater"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QList<int> columns = dialog.role_indices(QStringLiteral("variables"));
    if (columns.size() != 2) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("配对 t 检验必须选择恰好两列。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "配对 t 检验";
    configuration.chart_type = "paired_t";
    configuration.variable_columns = {
        static_cast<std::size_t>(columns[0]), static_cast<std::size_t>(columns[1])};
    const auto confidence = dialog.line_number(QStringLiteral("confidence"));
    if (confidence.has_value()) {
        configuration.confidence_level = *confidence > 1.0 ? *confidence / 100.0 : *confidence;
    }
    configuration.alternative = dialog.line_text(QStringLiteral("alternative"))
        .trimmed().toLower().toStdString();
    publish_page(datalab::application::AnalysisService::paired_t(table_, configuration));
}

void MainWindow::run_regression()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("线性回归"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"),
                    QStringLiteral("变量（第一列响应，其余为预测变量）"), true, false);
    dialog.add_line_edit(QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"),
                         QStringLiteral("95"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QList<int> columns = dialog.role_indices(QStringLiteral("variables"));
    if (columns.size() < 2) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("回归至少需要一列响应变量和一列预测变量。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "线性回归";
    configuration.chart_type = "regression";
    for (const int column : columns) {
        configuration.variable_columns.push_back(static_cast<std::size_t>(column));
    }
    const auto confidence = dialog.line_number(QStringLiteral("confidence"));
    if (confidence.has_value()) {
        configuration.confidence_level = *confidence > 1.0 ? *confidence / 100.0 : *confidence;
    }
    publish_page(datalab::application::AnalysisService::regression(table_, configuration));
}

void MainWindow::run_two_proportions()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("两比例检验"), column_labels(), this);
    dialog.add_role(QStringLiteral("first_events"), QStringLiteral("第一组事件数"), false, false);
    dialog.add_role(QStringLiteral("first_trials"), QStringLiteral("第一组试验数"), false, false);
    dialog.add_role(QStringLiteral("second_events"), QStringLiteral("第二组事件数"), false, false);
    dialog.add_role(QStringLiteral("second_trials"), QStringLiteral("第二组试验数"), false, false);
    dialog.add_line_edit(QStringLiteral("confidence"), QStringLiteral("置信水平 (%)"),
                         QStringLiteral("95"));
    dialog.add_line_edit(QStringLiteral("alternative"), QStringLiteral("备择方向"),
                         QStringLiteral("two_sided / less / greater"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int first_events = dialog.first_role_index(QStringLiteral("first_events"));
    const int first_trials = dialog.first_role_index(QStringLiteral("first_trials"));
    const int second_events = dialog.first_role_index(QStringLiteral("second_events"));
    const int second_trials = dialog.first_role_index(QStringLiteral("second_trials"));
    if (first_events < 0 || first_trials < 0 || second_events < 0 || second_trials < 0) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("请选择四个计数列。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "两比例检验";
    configuration.chart_type = "two_proportions";
    configuration.first_events_column = static_cast<std::size_t>(first_events);
    configuration.first_trials_column = static_cast<std::size_t>(first_trials);
    configuration.second_events_column = static_cast<std::size_t>(second_events);
    configuration.second_trials_column = static_cast<std::size_t>(second_trials);
    const auto confidence = dialog.line_number(QStringLiteral("confidence"));
    if (confidence.has_value()) {
        configuration.confidence_level = *confidence > 1.0 ? *confidence / 100.0 : *confidence;
    }
    configuration.alternative = dialog.line_text(QStringLiteral("alternative"))
        .trimmed().toLower().toStdString();
    publish_page(datalab::application::AnalysisService::two_proportions(table_, configuration));
}

void MainWindow::run_chi_square()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("列联表卡方"), column_labels(), this);
    dialog.add_role(QStringLiteral("row_category"), QStringLiteral("行分类列"), false, false);
    dialog.add_role(QStringLiteral("column_category"), QStringLiteral("列分类列"), false, false);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int row = dialog.first_role_index(QStringLiteral("row_category"));
    const int column = dialog.first_role_index(QStringLiteral("column_category"));
    if (row < 0 || column < 0) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("请选择行分类列和列分类列。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "列联表卡方";
    configuration.chart_type = "chi_square";
    configuration.row_category_column = static_cast<std::size_t>(row);
    configuration.column_category_column = static_cast<std::size_t>(column);
    publish_page(datalab::application::AnalysisService::chi_square(table_, configuration));
}

void MainWindow::run_mann_whitney()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("Mann-Whitney 检验"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("两列独立样本"), true, false);
    dialog.add_line_edit(QStringLiteral("alternative"), QStringLiteral("备择方向"),
                         QStringLiteral("two_sided / less / greater"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QList<int> columns = dialog.role_indices(QStringLiteral("variables"));
    if (columns.size() != 2) {
        QMessageBox::information(this, QStringLiteral("变量数量错误"), QStringLiteral("请选择正好两列独立样本。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "Mann-Whitney 检验";
    configuration.chart_type = "mann_whitney";
    configuration.variable_columns = {static_cast<std::size_t>(columns[0]),
                                      static_cast<std::size_t>(columns[1])};
    configuration.alternative = dialog.line_text(QStringLiteral("alternative")).trimmed().toLower().toStdString();
    publish_page(datalab::application::AnalysisService::mann_whitney(table_, configuration));
}

void MainWindow::run_wilcoxon_signed_rank()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("Wilcoxon 符号秩检验"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("两列配对样本"), true, false);
    dialog.add_line_edit(QStringLiteral("alternative"), QStringLiteral("备择方向"),
                         QStringLiteral("two_sided / less / greater"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const QList<int> columns = dialog.role_indices(QStringLiteral("variables"));
    if (columns.size() != 2) {
        QMessageBox::information(this, QStringLiteral("变量数量错误"), QStringLiteral("请选择正好两列配对样本。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "Wilcoxon 符号秩检验";
    configuration.chart_type = "wilcoxon_signed_rank";
    configuration.variable_columns = {static_cast<std::size_t>(columns[0]),
                                      static_cast<std::size_t>(columns[1])};
    configuration.alternative = dialog.line_text(QStringLiteral("alternative")).trimmed().toLower().toStdString();
    publish_page(datalab::application::AnalysisService::wilcoxon_signed_rank(table_, configuration));
}

void MainWindow::run_kruskal_wallis()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("Kruskal-Wallis 检验"), column_labels(), this);
    dialog.add_role(QStringLiteral("response"), QStringLiteral("测量值"), false, false);
    dialog.add_role(QStringLiteral("factor"), QStringLiteral("分组列"), false, false);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int response = dialog.first_role_index(QStringLiteral("response"));
    const int factor = dialog.first_role_index(QStringLiteral("factor"));
    if (response < 0 || factor < 0) {
        QMessageBox::information(this, QStringLiteral("参数不足"), QStringLiteral("请选择测量值和分组列。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "Kruskal-Wallis 检验";
    configuration.chart_type = "kruskal_wallis";
    configuration.variable_columns = {static_cast<std::size_t>(response)};
    configuration.by_column = static_cast<std::size_t>(factor);
    publish_page(datalab::application::AnalysisService::kruskal_wallis(table_, configuration));
}

void MainWindow::run_box_cox()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("Box-Cox 变换"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("正值变量"), false, false);
    dialog.add_line_edit(QStringLiteral("lambda"), QStringLiteral("Lambda（可选）"),
                         QStringLiteral("留空自动搜索 -5 到 5"));
    dialog.add_line_edit(QStringLiteral("lsl"), QStringLiteral("LSL（可选）"),
                         QStringLiteral("规格下限"));
    dialog.add_line_edit(QStringLiteral("usl"), QStringLiteral("USL（可选）"),
                         QStringLiteral("规格上限"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    if (column < 0) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("请选择一个正值变量。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "Box-Cox 变换";
    configuration.chart_type = "box_cox";
    configuration.variable_columns = {static_cast<std::size_t>(column)};
    configuration.hypothesis_mean = dialog.line_number(QStringLiteral("lambda"));
    configuration.specifications.lower = dialog.line_number(QStringLiteral("lsl"));
    configuration.specifications.upper = dialog.line_number(QStringLiteral("usl"));
    publish_page(datalab::application::AnalysisService::box_cox(table_, configuration));
}

void MainWindow::run_gage_rr()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("Crossed Gage R&R"), column_labels(), this);
    dialog.add_role(QStringLiteral("measurement"), QStringLiteral("测量值"), false, false);
    dialog.add_role(QStringLiteral("part"), QStringLiteral("零件"), false, false);
    dialog.add_role(QStringLiteral("operator"), QStringLiteral("操作员"), false, false);
    dialog.add_line_edit(QStringLiteral("lsl"), QStringLiteral("LSL（可选）"),
                         QStringLiteral("规格下限"));
    dialog.add_line_edit(QStringLiteral("usl"), QStringLiteral("USL（可选）"),
                         QStringLiteral("规格上限"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int measurement = dialog.first_role_index(QStringLiteral("measurement"));
    const int part = dialog.first_role_index(QStringLiteral("part"));
    const int oper = dialog.first_role_index(QStringLiteral("operator"));
    if (measurement < 0 || part < 0 || oper < 0) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("请选择测量值、零件和操作员列。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "Crossed Gage R&R";
    configuration.chart_type = "gage_rr";
    configuration.gage_measurement_column = static_cast<std::size_t>(measurement);
    configuration.gage_part_column = static_cast<std::size_t>(part);
    configuration.gage_operator_column = static_cast<std::size_t>(oper);
    configuration.specifications.lower = dialog.line_number(QStringLiteral("lsl"));
    configuration.specifications.upper = dialog.line_number(QStringLiteral("usl"));
    publish_page(datalab::application::AnalysisService::gage_rr(table_, configuration));
}

void MainWindow::run_msa_type1()
{
    if (!ensure_data()) return;
    AnalysisSetupDialog dialog(QStringLiteral("MSA Type 1 / Bias / Stability"),
                               column_labels(), this);
    dialog.add_role(QStringLiteral("measurement"), QStringLiteral("测量值"), false, false);
    dialog.add_role(QStringLiteral("reference"), QStringLiteral("参考值列（Linearity）"),
                    false, true);
    dialog.add_line_edit(QStringLiteral("mode"), QStringLiteral("模式"),
                         QStringLiteral("type1 / bias_linearity / stability"));
    dialog.add_line_edit(QStringLiteral("reference_value"), QStringLiteral("参考值（Type 1）"));
    dialog.add_line_edit(QStringLiteral("tolerance"), QStringLiteral("公差宽度（可选）"));
    if (dialog.exec() != QDialog::Accepted) return;
    const int measurement = dialog.first_role_index(QStringLiteral("measurement"));
    if (measurement < 0) {
        QMessageBox::information(this, QStringLiteral("参数不足"), QStringLiteral("请选择测量值列。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "MSA Type 1 / Bias / Stability";
    configuration.chart_type = "msa_type1";
    configuration.gage_measurement_column = static_cast<std::size_t>(measurement);
    configuration.msa_mode = dialog.line_text(QStringLiteral("mode")).trimmed().toLower().toStdString();
    if (configuration.msa_mode != "bias_linearity" && configuration.msa_mode != "stability") {
        configuration.msa_mode = "type1";
    }
    if (dialog.first_role_index(QStringLiteral("reference")) >= 0) {
        configuration.msa_reference_column =
            static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("reference")));
    }
    configuration.msa_reference_value = dialog.line_number(QStringLiteral("reference_value"));
    configuration.gage_tolerance = dialog.line_number(QStringLiteral("tolerance")).value_or(0.0);
    publish_page(datalab::application::AnalysisService::msa_type1(table_, configuration));
}

void MainWindow::run_reliability()
{
    if (!ensure_data()) return;
    AnalysisSetupDialog dialog(QStringLiteral("Reliability Analysis"),
                               column_labels(), this);
    dialog.add_role(QStringLiteral("time"), QStringLiteral("寿命/时间"), false, false);
    dialog.add_role(QStringLiteral("event"), QStringLiteral("失效指示（1=失效，0=删失）"),
                    false, false);
    dialog.add_role(QStringLiteral("group"), QStringLiteral("分组列（Log-rank，可选）"),
                    false, true);
    dialog.add_line_edit(QStringLiteral("model"), QStringLiteral("模型"),
                         QStringLiteral("kaplan_meier / weibull / exponential"));
    if (dialog.exec() != QDialog::Accepted) return;
    const int time = dialog.first_role_index(QStringLiteral("time"));
    const int event = dialog.first_role_index(QStringLiteral("event"));
    if (time < 0 || event < 0) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("请选择寿命列和失效指示列。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "Reliability Analysis";
    configuration.chart_type = "reliability";
    configuration.reliability_time_column = static_cast<std::size_t>(time);
    configuration.reliability_event_column = static_cast<std::size_t>(event);
    const int group = dialog.first_role_index(QStringLiteral("group"));
    if (group >= 0) {
        configuration.reliability_group_column = static_cast<std::size_t>(group);
    }
    configuration.reliability_model =
        dialog.line_text(QStringLiteral("model")).trimmed().toLower().toStdString();
    if (configuration.reliability_model != "weibull"
        && configuration.reliability_model != "exponential") {
        configuration.reliability_model = "kaplan_meier";
    }
    publish_page(datalab::application::AnalysisService::reliability(table_, configuration));
}

void MainWindow::run_t_power()
{
    AnalysisSetupDialog dialog(QStringLiteral("t 功效与样本量"), column_labels(), this);
    dialog.add_line_edit(QStringLiteral("mode"), QStringLiteral("模式"),
                         QStringLiteral("one_sample_sample_size"));
    dialog.add_line_edit(QStringLiteral("effect"), QStringLiteral("效应量 d"), QStringLiteral("0.5"));
    dialog.add_line_edit(QStringLiteral("target"), QStringLiteral("目标功效"), QStringLiteral("0.8"));
    dialog.add_line_edit(QStringLiteral("alpha"), QStringLiteral("显著性水平"), QStringLiteral("0.05"));
    dialog.add_line_edit(QStringLiteral("sample_size"), QStringLiteral("样本量（计算功效时）"));
    dialog.add_line_edit(QStringLiteral("groups"), QStringLiteral("ANOVA 组数"), QStringLiteral("3"));
    dialog.add_line_edit(QStringLiteral("null_proportion"), QStringLiteral("第一/假设比例"),
                         QStringLiteral("0.5"));
    dialog.add_line_edit(QStringLiteral("second_proportion"), QStringLiteral("第二/备择比例"),
                         QStringLiteral("0.7"));
    dialog.add_line_edit(QStringLiteral("variance_method"), QStringLiteral("比例方差"),
                         QStringLiteral("pooled / unpooled"));
    if (dialog.exec() != QDialog::Accepted) return;
    auto configuration = base_configuration();
    configuration.analysis_name = "t 功效与样本量";
    configuration.chart_type = "t_power";
    configuration.power_mode = dialog.line_text(QStringLiteral("mode")).trimmed().toLower().toStdString();
    if (configuration.power_mode.empty()) configuration.power_mode = "one_sample_sample_size";
    configuration.power_effect_size = dialog.line_number(QStringLiteral("effect")).value_or(0.5);
    configuration.power_target = dialog.line_number(QStringLiteral("target")).value_or(0.8);
    configuration.power_alpha = dialog.line_number(QStringLiteral("alpha")).value_or(0.05);
    configuration.power_sample_size = static_cast<std::size_t>(
        std::max(0, dialog.line_int(QStringLiteral("sample_size")).value_or(0)));
    configuration.power_group_count = static_cast<std::size_t>(
        std::max(0, dialog.line_int(QStringLiteral("groups")).value_or(3)));
    configuration.power_null_proportion =
        dialog.line_number(QStringLiteral("null_proportion")).value_or(0.5);
    configuration.power_second_proportion =
        dialog.line_number(QStringLiteral("second_proportion")).value_or(0.7);
    configuration.power_variance_method =
        dialog.line_text(QStringLiteral("variance_method")).trimmed().toLower().toStdString();
    if (configuration.power_variance_method != "unpooled") {
        configuration.power_variance_method = "pooled";
    }
    publish_page(datalab::application::AnalysisService::t_power(table_, configuration));
}

void MainWindow::run_imr()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("I-MR 控制图"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("变量"), false, false);
    dialog.add_line_edit(QStringLiteral("mr_length"), QStringLiteral("移动极差长度"), QStringLiteral("2"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "I-MR 控制图";
    configuration.chart_type = "imr";
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    if (column < 0) {
        QMessageBox::information(this, QStringLiteral("未选择变量"), QStringLiteral("请选择测量值列。"));
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(column));
    configuration.selection.measurement_column = static_cast<std::size_t>(column);
    configuration.moving_range_length = dialog.line_int(QStringLiteral("mr_length")).value_or(2);
    publish_page(datalab::application::AnalysisService::individuals_moving_range(table_, configuration));
}

void MainWindow::run_xbar_r()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("Xbar-R 控制图"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("变量"), false, false);
    dialog.add_role(QStringLiteral("subgroup"), QStringLiteral("子组列"), false, true);
    dialog.add_line_edit(QStringLiteral("subgroup_size"), QStringLiteral("子组大小"), QStringLiteral("5"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "Xbar-R 控制图";
    configuration.chart_type = "xbar_r";
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    if (column < 0) {
        QMessageBox::information(this, QStringLiteral("未选择变量"), QStringLiteral("请选择测量值列。"));
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(column));
    configuration.selection.measurement_column = static_cast<std::size_t>(column);
    if (dialog.first_role_index(QStringLiteral("subgroup")) >= 0) {
        configuration.selection.subgroup_column =
            static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("subgroup")));
    }
    configuration.subgroup_size = dialog.line_int(QStringLiteral("subgroup_size")).value_or(5);
    publish_page(datalab::application::AnalysisService::xbar_range(table_, configuration));
}

void MainWindow::run_xbar_s()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("Xbar-S 控制图"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("变量"), false, false);
    dialog.add_role(QStringLiteral("subgroup"), QStringLiteral("子组列"), false, true);
    dialog.add_line_edit(QStringLiteral("subgroup_size"), QStringLiteral("子组大小"), QStringLiteral("5"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "Xbar-S 控制图";
    configuration.chart_type = "xbar_s";
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    if (column < 0) {
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(column));
    configuration.selection.measurement_column = static_cast<std::size_t>(column);
    if (dialog.first_role_index(QStringLiteral("subgroup")) >= 0) {
        configuration.selection.subgroup_column =
            static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("subgroup")));
    }
    configuration.subgroup_size = dialog.line_int(QStringLiteral("subgroup_size")).value_or(5);
    publish_page(datalab::application::AnalysisService::xbar_s(table_, configuration));
}

void MainWindow::run_p_chart()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("P 图"), column_labels(), this);
    dialog.add_role(QStringLiteral("defectives"), QStringLiteral("不合格品数"), false, false);
    dialog.add_role(QStringLiteral("inspected"), QStringLiteral("检验数（列）"), false, true);
    dialog.add_line_edit(QStringLiteral("inspected_constant"), QStringLiteral("检验数（常数）"), QStringLiteral(""));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "P 图";
    configuration.chart_type = "p_chart";
    const int defectives = dialog.first_role_index(QStringLiteral("defectives"));
    if (defectives < 0) {
        QMessageBox::information(this, QStringLiteral("未选择变量"), QStringLiteral("请选择不合格品数列。"));
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(defectives));
    configuration.selection.defect_count_column = static_cast<std::size_t>(defectives);
    if (dialog.first_role_index(QStringLiteral("inspected")) >= 0) {
        configuration.selection.inspected_count_column =
            static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("inspected")));
    }
    if (dialog.line_int(QStringLiteral("inspected_constant")).has_value()) {
        configuration.inspected_constant =
            static_cast<std::size_t>(*dialog.line_int(QStringLiteral("inspected_constant")));
    }
    publish_page(datalab::application::AnalysisService::p_chart(table_, configuration));
}

void MainWindow::run_np_chart()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("NP 图"), column_labels(), this);
    dialog.add_role(QStringLiteral("defectives"), QStringLiteral("不合格品数"), false, false);
    dialog.add_role(QStringLiteral("inspected"), QStringLiteral("检验数列"), false, true);
    dialog.add_line_edit(QStringLiteral("inspected_constant"), QStringLiteral("检验数常数"), QStringLiteral("例如 100"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.chart_type = "np_chart";
    const int defect_column = dialog.first_role_index(QStringLiteral("defectives"));
    if (defect_column < 0) {
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(defect_column));
    configuration.selection.defect_count_column = static_cast<std::size_t>(defect_column);
    if (dialog.first_role_index(QStringLiteral("inspected")) >= 0) {
        configuration.selection.inspected_count_column =
            static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("inspected")));
    }
    if (dialog.line_int(QStringLiteral("inspected_constant")).has_value()) {
        configuration.inspected_constant =
            static_cast<std::size_t>(*dialog.line_int(QStringLiteral("inspected_constant")));
    }
    publish_page(datalab::application::AnalysisService::np_chart(table_, configuration));
}

void MainWindow::run_c_chart()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("C 图"), column_labels(), this);
    dialog.add_role(QStringLiteral("defects"), QStringLiteral("缺陷数"), false, false);
    dialog.add_line_edit(QStringLiteral("units"), QStringLiteral("每个子组单位数"), QStringLiteral("1"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.chart_type = "c_chart";
    const int defect_column = dialog.first_role_index(QStringLiteral("defects"));
    if (defect_column < 0) {
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(defect_column));
    configuration.selection.defect_count_column = static_cast<std::size_t>(defect_column);
    configuration.inspected_constant =
        static_cast<std::size_t>(dialog.line_int(QStringLiteral("units")).value_or(1));
    publish_page(datalab::application::AnalysisService::c_chart(table_, configuration));
}

void MainWindow::run_u_chart()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("U 图"), column_labels(), this);
    dialog.add_role(QStringLiteral("defects"), QStringLiteral("缺陷数"), false, false);
    dialog.add_role(QStringLiteral("units"), QStringLiteral("单位数列"), false, false);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.chart_type = "u_chart";
    const int defect_column = dialog.first_role_index(QStringLiteral("defects"));
    const int units_column = dialog.first_role_index(QStringLiteral("units"));
    if (defect_column < 0 || units_column < 0) {
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(defect_column));
    configuration.selection.defect_count_column = static_cast<std::size_t>(defect_column);
    configuration.selection.inspected_count_column = static_cast<std::size_t>(units_column);
    publish_page(datalab::application::AnalysisService::u_chart(table_, configuration));
}

void MainWindow::run_laney_p_chart()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("Laney P' 图"), column_labels(), this);
    dialog.add_role(QStringLiteral("defectives"), QStringLiteral("不合格品数"), false, false);
    dialog.add_role(QStringLiteral("inspected"), QStringLiteral("检验数列"), false, true);
    dialog.add_role(QStringLiteral("stage"), QStringLiteral("阶段列"), false, true);
    dialog.add_line_edit(QStringLiteral("inspected_constant"), QStringLiteral("检验数常数"), QStringLiteral("可选"));
    dialog.add_line_edit(QStringLiteral("tests"), QStringLiteral("特殊原因测试"), QStringLiteral("例如 1 2 3 4"));
    dialog.add_line_edit(QStringLiteral("historical_center"), QStringLiteral("历史中心线"), QStringLiteral("可选"));
    dialog.add_line_edit(QStringLiteral("historical_sigma_z"), QStringLiteral("历史 Sigma Z"), QStringLiteral("可选"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "Laney P' 图";
    configuration.chart_type = "laney_p_chart";
    const int defect_column = dialog.first_role_index(QStringLiteral("defectives"));
    if (defect_column < 0) {
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(defect_column));
    configuration.selection.defect_count_column = static_cast<std::size_t>(defect_column);
    if (dialog.first_role_index(QStringLiteral("inspected")) >= 0) {
        configuration.selection.inspected_count_column =
            static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("inspected")));
    }
    if (dialog.first_role_index(QStringLiteral("stage")) >= 0) {
        configuration.stage_column =
            static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("stage")));
    }
    if (dialog.line_int(QStringLiteral("inspected_constant")).has_value()) {
        configuration.inspected_constant = static_cast<std::size_t>(
            *dialog.line_int(QStringLiteral("inspected_constant")));
    }
    configuration.enabled_special_cause_tests.clear();
    for (const QString& token : dialog.line_text(QStringLiteral("tests")).split(
             QRegularExpression(QStringLiteral("[,;\\s]+")), Qt::SkipEmptyParts)) {
        bool ok = false;
        const int test = token.toInt(&ok);
        if (ok && test >= 1 && test <= 4) {
            configuration.enabled_special_cause_tests.push_back(test);
        }
    }
    if (configuration.enabled_special_cause_tests.empty()) {
        configuration.enabled_special_cause_tests = {1};
    }
    configuration.historical_center = dialog.line_number(QStringLiteral("historical_center"));
    configuration.historical_sigma_z = dialog.line_number(QStringLiteral("historical_sigma_z"));
    publish_page(datalab::application::AnalysisService::laney_p_chart(table_, configuration));
}

void MainWindow::run_laney_u_chart()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("Laney U' 图"), column_labels(), this);
    dialog.add_role(QStringLiteral("defects"), QStringLiteral("缺陷数"), false, false);
    dialog.add_role(QStringLiteral("units"), QStringLiteral("单位数列"), false, false);
    dialog.add_role(QStringLiteral("stage"), QStringLiteral("阶段列"), false, true);
    dialog.add_line_edit(QStringLiteral("tests"), QStringLiteral("特殊原因测试"), QStringLiteral("例如 1 2 3 4"));
    dialog.add_line_edit(QStringLiteral("historical_center"), QStringLiteral("历史中心线"), QStringLiteral("可选"));
    dialog.add_line_edit(QStringLiteral("historical_sigma_z"), QStringLiteral("历史 Sigma Z"), QStringLiteral("可选"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "Laney U' 图";
    configuration.chart_type = "laney_u_chart";
    const int defect_column = dialog.first_role_index(QStringLiteral("defects"));
    const int units_column = dialog.first_role_index(QStringLiteral("units"));
    if (defect_column < 0 || units_column < 0) {
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(defect_column));
    configuration.selection.defect_count_column = static_cast<std::size_t>(defect_column);
    configuration.selection.inspected_count_column = static_cast<std::size_t>(units_column);
    if (dialog.first_role_index(QStringLiteral("stage")) >= 0) {
        configuration.stage_column =
            static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("stage")));
    }
    configuration.enabled_special_cause_tests.clear();
    for (const QString& token : dialog.line_text(QStringLiteral("tests")).split(
             QRegularExpression(QStringLiteral("[,;\\s]+")), Qt::SkipEmptyParts)) {
        bool ok = false;
        const int test = token.toInt(&ok);
        if (ok && test >= 1 && test <= 4) {
            configuration.enabled_special_cause_tests.push_back(test);
        }
    }
    if (configuration.enabled_special_cause_tests.empty()) {
        configuration.enabled_special_cause_tests = {1};
    }
    configuration.historical_center = dialog.line_number(QStringLiteral("historical_center"));
    configuration.historical_sigma_z = dialog.line_number(QStringLiteral("historical_sigma_z"));
    publish_page(datalab::application::AnalysisService::laney_u_chart(table_, configuration));
}

void MainWindow::run_ewma()
{
    if (!ensure_data()) return;
    AnalysisSetupDialog dialog(QStringLiteral("EWMA 控制图"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("测量值"), false, false);
    dialog.add_line_edit(QStringLiteral("lambda"), QStringLiteral("Lambda"), QStringLiteral("0.2"));
    dialog.add_line_edit(QStringLiteral("limit"), QStringLiteral("控制限倍数"), QStringLiteral("3"));
    dialog.add_line_edit(QStringLiteral("historical_mean"), QStringLiteral("历史均值（可选）"), QStringLiteral("可选"));
    if (dialog.exec() != QDialog::Accepted) return;
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    if (column < 0) return;
    auto configuration = base_configuration();
    configuration.analysis_name = "EWMA 控制图";
    configuration.chart_type = "ewma";
    configuration.variable_columns = {static_cast<std::size_t>(column)};
    configuration.ewma_lambda = dialog.line_number(QStringLiteral("lambda")).value_or(0.2);
    configuration.ewma_limit_sigma = dialog.line_number(QStringLiteral("limit")).value_or(3.0);
    configuration.historical_center = dialog.line_number(QStringLiteral("historical_mean"));
    publish_page(datalab::application::AnalysisService::ewma(table_, configuration));
}

void MainWindow::run_cusum()
{
    if (!ensure_data()) return;
    AnalysisSetupDialog dialog(QStringLiteral("CUSUM 控制图"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("测量值"), false, false);
    dialog.add_line_edit(QStringLiteral("target"), QStringLiteral("目标值"), QStringLiteral("0"));
    dialog.add_line_edit(QStringLiteral("sigma"), QStringLiteral("过程 Sigma"), QStringLiteral("1"));
    dialog.add_line_edit(QStringLiteral("k"), QStringLiteral("参考值 k"), QStringLiteral("0.5"));
    dialog.add_line_edit(QStringLiteral("h"), QStringLiteral("决策间隔 h"), QStringLiteral("4"));
    if (dialog.exec() != QDialog::Accepted) return;
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    if (column < 0) return;
    auto configuration = base_configuration();
    configuration.analysis_name = "CUSUM 控制图";
    configuration.chart_type = "cusum";
    configuration.variable_columns = {static_cast<std::size_t>(column)};
    configuration.cusum_target = dialog.line_number(QStringLiteral("target")).value_or(0.0);
    configuration.cusum_sigma = dialog.line_number(QStringLiteral("sigma")).value_or(1.0);
    configuration.cusum_k = dialog.line_number(QStringLiteral("k")).value_or(0.5);
    configuration.cusum_h = dialog.line_number(QStringLiteral("h")).value_or(4.0);
    publish_page(datalab::application::AnalysisService::cusum(table_, configuration));
}

void MainWindow::run_time_series_smoothing()
{
    if (!ensure_data()) return;
    AnalysisSetupDialog dialog(QStringLiteral("时间序列平滑"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("时间序列"), false, false);
    dialog.add_line_edit(QStringLiteral("method"), QStringLiteral("方法"), QStringLiteral("single 或 double"));
    dialog.add_line_edit(QStringLiteral("alpha"), QStringLiteral("Alpha"), QStringLiteral("0.2"));
    dialog.add_line_edit(QStringLiteral("gamma"), QStringLiteral("Gamma（双指数）"), QStringLiteral("0.2"));
    dialog.add_line_edit(QStringLiteral("periods"), QStringLiteral("预测期数"), QStringLiteral("1"));
    if (dialog.exec() != QDialog::Accepted) return;
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    if (column < 0) return;
    auto configuration = base_configuration();
    configuration.analysis_name = "时间序列平滑";
    configuration.chart_type = "time_series_smoothing";
    configuration.variable_columns = {static_cast<std::size_t>(column)};
    configuration.smoothing_method = dialog.line_text(QStringLiteral("method")).trimmed().toLower().toStdString();
    if (configuration.smoothing_method != "single") configuration.smoothing_method = "double";
    configuration.smoothing_alpha = dialog.line_number(QStringLiteral("alpha")).value_or(0.2);
    configuration.smoothing_gamma = dialog.line_number(QStringLiteral("gamma")).value_or(0.2);
    configuration.forecast_periods = dialog.line_int(QStringLiteral("periods")).value_or(1);
    publish_page(datalab::application::AnalysisService::time_series_smoothing(table_, configuration));
}

void MainWindow::run_arima()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("ARIMA 基础预测"), column_labels(), this);
    dialog.add_role(QStringLiteral("time"), QStringLiteral("时间列（可选）"), false, false);
    dialog.add_role(QStringLiteral("value"), QStringLiteral("时间序列值"), false, false);
    dialog.add_line_edit(QStringLiteral("criterion"), QStringLiteral("选模准则"),
                         QStringLiteral("aicc / aic / bic"));
    dialog.add_line_edit(QStringLiteral("periods"), QStringLiteral("预测期数"),
                         QStringLiteral("3"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int value = dialog.first_role_index(QStringLiteral("value"));
    if (value < 0) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("请选择时间序列值列。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "ARIMA 基础预测";
    configuration.chart_type = "arima";
    const int time = dialog.first_role_index(QStringLiteral("time"));
    if (time >= 0) {
        configuration.arima_time_column = static_cast<std::size_t>(time);
    }
    configuration.arima_value_column = static_cast<std::size_t>(value);
    configuration.arima_selection_criterion =
        dialog.line_text(QStringLiteral("criterion")).trimmed().toLower().toStdString();
    if (configuration.arima_selection_criterion != "aic"
        && configuration.arima_selection_criterion != "bic") {
        configuration.arima_selection_criterion = "aicc";
    }
    configuration.forecast_periods =
        dialog.line_int(QStringLiteral("periods")).value_or(3);
    publish_page(datalab::application::AnalysisService::arima(table_, configuration));
}

void MainWindow::run_two_factor_anova()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("双因素 ANOVA"), column_labels(), this);
    dialog.add_role(QStringLiteral("response"), QStringLiteral("响应变量"), false, false);
    dialog.add_role(QStringLiteral("factor_a"), QStringLiteral("因子 A"), false, false);
    dialog.add_role(QStringLiteral("factor_b"), QStringLiteral("因子 B"), false, false);
    dialog.add_line_edit(QStringLiteral("encoding"), QStringLiteral("因子编码"),
                         QStringLiteral("reference / effect"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int response = dialog.first_role_index(QStringLiteral("response"));
    const int factor_a = dialog.first_role_index(QStringLiteral("factor_a"));
    const int factor_b = dialog.first_role_index(QStringLiteral("factor_b"));
    if (response < 0 || factor_a < 0 || factor_b < 0) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("请选择响应变量、因子 A 和因子 B。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "双因素 ANOVA";
    configuration.chart_type = "two_factor_anova";
    configuration.anova_response_column = static_cast<std::size_t>(response);
    configuration.anova_factor_a_column = static_cast<std::size_t>(factor_a);
    configuration.anova_factor_b_column = static_cast<std::size_t>(factor_b);
    configuration.anova_factor_encoding =
        dialog.line_text(QStringLiteral("encoding")).trimmed().toLower().toStdString();
    if (configuration.anova_factor_encoding != "effect") {
        configuration.anova_factor_encoding = "reference";
    }
    publish_page(datalab::application::AnalysisService::two_factor_anova(
        table_, configuration));
}

void MainWindow::run_logistic_regression()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("二元 Logistic 回归"), column_labels(), this);
    dialog.add_role(QStringLiteral("response"), QStringLiteral("二元响应"), false, false);
    dialog.add_role(QStringLiteral("predictors"), QStringLiteral("数值预测变量"), true, false);
    dialog.add_line_edit(QStringLiteral("event"), QStringLiteral("事件水平"),
                         QStringLiteral("0/1 数据可留空；文本数据输入事件标签"));
    dialog.add_line_edit(QStringLiteral("iterations"), QStringLiteral("最大迭代次数"),
                         QStringLiteral("100"));
    dialog.add_line_edit(QStringLiteral("tolerance"), QStringLiteral("收敛阈值"),
                         QStringLiteral("1e-8"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int response = dialog.first_role_index(QStringLiteral("response"));
    const QList<int> predictors = dialog.role_indices(QStringLiteral("predictors"));
    if (response < 0 || predictors.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("请选择一个二元响应列和至少一个预测变量。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "二元 Logistic 回归";
    configuration.chart_type = "logistic_regression";
    configuration.logistic_response_column = static_cast<std::size_t>(response);
    for (const int predictor : predictors) {
        configuration.logistic_predictor_columns.push_back(
            static_cast<std::size_t>(predictor));
    }
    configuration.logistic_event_level =
        dialog.line_text(QStringLiteral("event")).trimmed().toStdString();
    if (configuration.logistic_event_level.empty()) {
        configuration.logistic_event_level = "1";
    }
    configuration.logistic_max_iterations =
        dialog.line_int(QStringLiteral("iterations")).value_or(100);
    configuration.logistic_tolerance =
        dialog.line_number(QStringLiteral("tolerance")).value_or(1.0e-8);
    publish_page(datalab::application::AnalysisService::logistic_regression(
        table_, configuration));
}

void MainWindow::run_variance_test()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("方差检验"), column_labels(), this);
    dialog.add_role(QStringLiteral("first"), QStringLiteral("第一样本"), false, false);
    dialog.add_role(QStringLiteral("second"), QStringLiteral("第二样本（两方差）"), false, false);
    dialog.add_line_edit(QStringLiteral("hypothesis"), QStringLiteral("假设方差（一方差）"),
                         QStringLiteral("例如 1.0"));
    dialog.add_line_edit(QStringLiteral("method"), QStringLiteral("两方差方法"),
                         QStringLiteral("f / levene / brown_forsythe"));
    dialog.add_line_edit(QStringLiteral("alternative"), QStringLiteral("备择方向"),
                         QStringLiteral("two_sided / less / greater"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int first = dialog.first_role_index(QStringLiteral("first"));
    const int second = dialog.first_role_index(QStringLiteral("second"));
    if (first < 0) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("请选择第一样本列。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "方差检验";
    configuration.chart_type = "variance_test";
    configuration.variance_first_column = static_cast<std::size_t>(first);
    if (second >= 0) {
        configuration.variance_second_column = static_cast<std::size_t>(second);
    }
    configuration.hypothesized_variance =
        dialog.line_number(QStringLiteral("hypothesis"));
    configuration.variance_test_method =
        dialog.line_text(QStringLiteral("method")).trimmed().toLower().toStdString();
    if (configuration.variance_test_method != "levene"
        && configuration.variance_test_method != "brown_forsythe") {
        configuration.variance_test_method = "f";
    }
    configuration.variance_alternative =
        dialog.line_text(QStringLiteral("alternative")).trimmed().toLower().toStdString();
    if (configuration.variance_alternative != "less"
        && configuration.variance_alternative != "greater") {
        configuration.variance_alternative = "two_sided";
    }
    publish_page(datalab::application::AnalysisService::variance_test(
        table_, configuration));
}

void MainWindow::run_time_series_decomposition()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("时间序列分解"), column_labels(), this);
    dialog.add_role(QStringLiteral("time"), QStringLiteral("时间列（可选）"), false, false);
    dialog.add_role(QStringLiteral("value"), QStringLiteral("时间序列值"), false, false);
    dialog.add_line_edit(QStringLiteral("period"), QStringLiteral("季节周期"),
                         QStringLiteral("例如 4 或 12"));
    dialog.add_line_edit(QStringLiteral("model"), QStringLiteral("模型"),
                         QStringLiteral("additive / multiplicative"));
    dialog.add_line_edit(QStringLiteral("periods"), QStringLiteral("预测期数"),
                         QStringLiteral("4"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int value = dialog.first_role_index(QStringLiteral("value"));
    if (value < 0) {
        QMessageBox::information(this, QStringLiteral("参数不足"),
                                 QStringLiteral("请选择时间序列值列。"));
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "时间序列分解";
    configuration.chart_type = "time_series_decomposition";
    const int time = dialog.first_role_index(QStringLiteral("time"));
    if (time >= 0) {
        configuration.decomposition_time_column = static_cast<std::size_t>(time);
    }
    configuration.decomposition_value_column = static_cast<std::size_t>(value);
    configuration.decomposition_seasonal_period =
        dialog.line_int(QStringLiteral("period")).value_or(1);
    configuration.decomposition_model =
        dialog.line_text(QStringLiteral("model")).trimmed().toLower().toStdString();
    if (configuration.decomposition_model != "multiplicative") {
        configuration.decomposition_model = "additive";
    }
    configuration.forecast_periods =
        dialog.line_int(QStringLiteral("periods")).value_or(4);
    publish_page(datalab::application::AnalysisService::time_series_decomposition(
        table_, configuration));
}

void MainWindow::run_doe_factorial()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("2 水平全因子设计"), column_labels(), this);
    dialog.add_role(QStringLiteral("response"), QStringLiteral("响应列（可选，选择后进行响应分析）"),
                    false, true);
    dialog.add_role(QStringLiteral("factor_columns"), QStringLiteral("已导入因子列（可选，多选）"),
                    true, true);
    dialog.add_line_edit(QStringLiteral("factors"), QStringLiteral("因子名（逗号分隔）"),
                         QStringLiteral("Temperature,Pressure"));
    dialog.add_line_edit(QStringLiteral("low"), QStringLiteral("低水平（逗号分隔）"),
                         QStringLiteral("-1,-1"));
    dialog.add_line_edit(QStringLiteral("high"), QStringLiteral("高水平（逗号分隔）"),
                         QStringLiteral("1,1"));
    dialog.add_line_edit(QStringLiteral("centers"), QStringLiteral("中心点数"), QStringLiteral("0"));
    dialog.add_line_edit(QStringLiteral("blocks"), QStringLiteral("区组数"), QStringLiteral("1"));
    dialog.add_line_edit(QStringLiteral("seed"), QStringLiteral("随机种子"), QStringLiteral("0"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto split = [](const QString& text) {
        std::vector<std::string> values;
        for (const QString& value : text.split(',', Qt::SkipEmptyParts)) {
            values.push_back(value.trimmed().toStdString());
        }
        return values;
    };
    auto configuration = base_configuration();
    configuration.analysis_name = "2 水平全因子设计";
    configuration.chart_type = "doe_factorial";
    configuration.doe_factor_names = split(dialog.line_text(QStringLiteral("factors")));
    configuration.doe_low_levels = split(dialog.line_text(QStringLiteral("low")));
    configuration.doe_high_levels = split(dialog.line_text(QStringLiteral("high")));
    configuration.doe_center_point_count =
        static_cast<std::size_t>(dialog.line_int(QStringLiteral("centers")).value_or(0));
    configuration.doe_block_count =
        static_cast<std::size_t>(std::max(1, dialog.line_int(QStringLiteral("blocks")).value_or(1)));
    configuration.doe_randomize = true;
    configuration.doe_random_seed =
        static_cast<std::uint64_t>(std::max(0, dialog.line_int(QStringLiteral("seed")).value_or(0)));
    const int response_column = dialog.first_role_index(QStringLiteral("response"));
    if (response_column >= 0) {
        configuration.doe_response_column = static_cast<std::size_t>(response_column);
        const QList<int> factor_columns =
            dialog.role_indices(QStringLiteral("factor_columns"));
        for (const int column : factor_columns) {
            configuration.doe_factor_columns.push_back(static_cast<std::size_t>(column));
        }
        if (configuration.doe_factor_columns.empty()) {
            QMessageBox::information(this, QStringLiteral("因子列不足"),
                                     QStringLiteral("响应分析至少需要选择一个已导入因子列。"));
            return;
        }
    }
    publish_page(datalab::application::AnalysisService::doe_factorial(
        table_, configuration));
}

void MainWindow::run_nested_gage_rr()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("Nested Gage R&R"), column_labels(), this);
    dialog.add_role(QStringLiteral("measurement"), QStringLiteral("测量值"), false, false);
    dialog.add_role(QStringLiteral("part"), QStringLiteral("部件"), false, false);
    dialog.add_role(QStringLiteral("operator"), QStringLiteral("操作者"), false, false);
    dialog.add_line_edit(QStringLiteral("tolerance"), QStringLiteral("公差"),
                         QStringLiteral("可选"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "Nested Gage R&R";
    configuration.chart_type = "nested_gage_rr";
    configuration.nested_gage_measurement_column =
        static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("measurement")));
    configuration.nested_gage_part_column =
        static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("part")));
    configuration.nested_gage_operator_column =
        static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("operator")));
    configuration.gage_tolerance = dialog.line_number(QStringLiteral("tolerance")).value_or(0.0);
    publish_page(datalab::application::AnalysisService::nested_gage_rr(
        table_, configuration));
}

void MainWindow::run_attribute_agreement()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("属性一致性分析"), column_labels(), this);
    dialog.add_role(QStringLiteral("rating"), QStringLiteral("评级"), false, false);
    dialog.add_role(QStringLiteral("part"), QStringLiteral("部件"), false, false);
    dialog.add_role(QStringLiteral("appraiser"), QStringLiteral("评估者"), false, false);
    dialog.add_role(QStringLiteral("standard"), QStringLiteral("标准（可选）"), false, true);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "属性一致性分析";
    configuration.chart_type = "attribute_agreement";
    configuration.attribute_rating_column =
        static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("rating")));
    configuration.attribute_part_column =
        static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("part")));
    configuration.attribute_appraiser_column =
        static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("appraiser")));
    const int standard = dialog.first_role_index(QStringLiteral("standard"));
    if (standard >= 0) {
        configuration.attribute_standard_column = static_cast<std::size_t>(standard);
    }
    publish_page(datalab::application::AnalysisService::attribute_agreement(
        table_, configuration));
}

void MainWindow::run_seasonal_forecasting()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("季节性预测"), column_labels(), this);
    dialog.add_role(QStringLiteral("value"), QStringLiteral("时间序列值"), false, false);
    dialog.add_line_edit(QStringLiteral("period"), QStringLiteral("季节周期"), QStringLiteral("12"));
    dialog.add_line_edit(QStringLiteral("error"), QStringLiteral("误差模型"),
                         QStringLiteral("additive / multiplicative"));
    dialog.add_line_edit(QStringLiteral("trend"), QStringLiteral("趋势模型"),
                         QStringLiteral("additive / none / multiplicative"));
    dialog.add_line_edit(QStringLiteral("alpha"), QStringLiteral("Alpha"), QStringLiteral("0.2"));
    dialog.add_line_edit(QStringLiteral("beta"), QStringLiteral("Beta"), QStringLiteral("0.1"));
    dialog.add_line_edit(QStringLiteral("gamma"), QStringLiteral("Gamma"), QStringLiteral("0.2"));
    dialog.add_line_edit(QStringLiteral("forecast"), QStringLiteral("预测期数"), QStringLiteral("4"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "季节性预测";
    configuration.chart_type = "seasonal_forecasting";
    configuration.decomposition_value_column =
        static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("value")));
    configuration.seasonal_period = static_cast<std::size_t>(
        std::max(1, dialog.line_int(QStringLiteral("period")).value_or(12)));
    configuration.seasonal_error_model =
        dialog.line_text(QStringLiteral("error")).trimmed().toLower().toStdString();
    configuration.seasonal_trend_model =
        dialog.line_text(QStringLiteral("trend")).trimmed().toLower().toStdString();
    configuration.smoothing_alpha = dialog.line_number(QStringLiteral("alpha")).value_or(0.2);
    configuration.seasonal_beta = dialog.line_number(QStringLiteral("beta")).value_or(0.1);
    configuration.smoothing_gamma = dialog.line_number(QStringLiteral("gamma")).value_or(0.2);
    configuration.forecast_periods = std::max(1, dialog.line_int(
        QStringLiteral("forecast")).value_or(4));
    publish_page(datalab::application::AnalysisService::seasonal_forecasting(
        table_, configuration));
}

void MainWindow::run_pca()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("主成分分析"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("数值变量（可多选）"), true, false);
    dialog.add_line_edit(QStringLiteral("mode"), QStringLiteral("矩阵模式"),
                         QStringLiteral("covariance / standardized"));
    dialog.add_line_edit(QStringLiteral("components"), QStringLiteral("保留主成分数"),
                         QStringLiteral("0 = 全部"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "主成分分析";
    configuration.chart_type = "pca";
    for (const int column : dialog.role_indices(QStringLiteral("variables"))) {
        if (column >= 0) {
            configuration.pca_variable_columns.push_back(static_cast<std::size_t>(column));
        }
    }
    configuration.pca_mode =
        dialog.line_text(QStringLiteral("mode")).trimmed().toLower().toStdString();
    configuration.pca_component_count = static_cast<std::size_t>(
        std::max(0, dialog.line_int(QStringLiteral("components")).value_or(0)));
    publish_page(datalab::application::AnalysisService::pca(table_, configuration));
}

void MainWindow::run_capability()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("正态过程能力分析"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("变量"), false, false);
    dialog.add_line_edit(QStringLiteral("subgroup_size"), QStringLiteral("子组大小"), QStringLiteral("1"));
    dialog.add_line_edit(QStringLiteral("lsl"), QStringLiteral("LSL"), QStringLiteral("例如 73.95"));
    dialog.add_line_edit(QStringLiteral("usl"), QStringLiteral("USL"), QStringLiteral("例如 74.05"));
    dialog.add_line_edit(QStringLiteral("target"), QStringLiteral("Target"), QStringLiteral("可选"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "正态过程能力";
    configuration.chart_type = "capability";
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    if (column < 0) {
        QMessageBox::information(this, QStringLiteral("未选择变量"), QStringLiteral("请选择测量值列。"));
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(column));
    configuration.selection.measurement_column = static_cast<std::size_t>(column);
    configuration.subgroup_size = dialog.line_int(QStringLiteral("subgroup_size")).value_or(1);
    configuration.specifications.lower = dialog.line_number(QStringLiteral("lsl"));
    configuration.specifications.upper = dialog.line_number(QStringLiteral("usl"));
    configuration.specifications.target = dialog.line_number(QStringLiteral("target"));
    publish_page(datalab::application::AnalysisService::capability(table_, configuration));
}

void MainWindow::run_capability_sixpack()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("过程能力 Sixpack"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("变量"), false, false);
    dialog.add_line_edit(QStringLiteral("subgroup_size"), QStringLiteral("子组大小"), QStringLiteral("1"));
    dialog.add_line_edit(QStringLiteral("lsl"), QStringLiteral("LSL"), QStringLiteral("例如 73.95"));
    dialog.add_line_edit(QStringLiteral("usl"), QStringLiteral("USL"), QStringLiteral("例如 74.05"));
    dialog.add_line_edit(QStringLiteral("target"), QStringLiteral("Target"), QStringLiteral("可选"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.analysis_name = "过程能力 Sixpack";
    configuration.chart_type = "capability_sixpack";
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    if (column < 0) {
        QMessageBox::information(this, QStringLiteral("未选择变量"), QStringLiteral("请选择测量值列。"));
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(column));
    configuration.selection.measurement_column = static_cast<std::size_t>(column);
    configuration.subgroup_size = dialog.line_int(QStringLiteral("subgroup_size")).value_or(1);
    configuration.specifications.lower = dialog.line_number(QStringLiteral("lsl"));
    configuration.specifications.upper = dialog.line_number(QStringLiteral("usl"));
    configuration.specifications.target = dialog.line_number(QStringLiteral("target"));
    publish_page(datalab::application::AnalysisService::capability_sixpack(table_, configuration));
}

void MainWindow::run_histogram()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("直方图"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("变量"), false, false);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.chart_type = "histogram";
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    if (column < 0) {
        QMessageBox::information(this, QStringLiteral("未选择变量"), QStringLiteral("请选择连续变量列。"));
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(column));
    publish_page(datalab::application::AnalysisService::histogram(table_, configuration));
}

void MainWindow::run_boxplot()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("箱线图"), column_labels(), this);
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("变量"), false, false);
    dialog.add_role(QStringLiteral("by"), QStringLiteral("分类变量"), false, true);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.chart_type = "boxplot";
    const int column = dialog.first_role_index(QStringLiteral("variables"));
    if (column < 0) {
        QMessageBox::information(this, QStringLiteral("未选择变量"), QStringLiteral("请选择连续变量列。"));
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(column));
    if (dialog.first_role_index(QStringLiteral("by")) >= 0) {
        configuration.by_column = static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("by")));
    }
    publish_page(datalab::application::AnalysisService::boxplot(table_, configuration));
}

void MainWindow::run_pareto()
{
    if (!ensure_data()) {
        return;
    }
    AnalysisSetupDialog dialog(QStringLiteral("柏拉图"), column_labels(), this);
    dialog.add_role(QStringLiteral("category"), QStringLiteral("缺陷类别"), false, false);
    dialog.add_role(QStringLiteral("counts"), QStringLiteral("计数列"), false, true);
    dialog.add_line_edit(
        QStringLiteral("other_threshold"),
        QStringLiteral("Other 合并阈值（可选 %）"),
        QStringLiteral("例如 95；留空则不合并"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    auto configuration = base_configuration();
    configuration.chart_type = "pareto";
    const int column = dialog.first_role_index(QStringLiteral("category"));
    if (column < 0) {
        QMessageBox::information(this, QStringLiteral("未选择变量"), QStringLiteral("请选择缺陷类别列。"));
        return;
    }
    configuration.variable_columns.push_back(static_cast<std::size_t>(column));
    if (dialog.first_role_index(QStringLiteral("counts")) >= 0) {
        configuration.selection.defect_count_column =
            static_cast<std::size_t>(dialog.first_role_index(QStringLiteral("counts")));
    }
    const std::optional<double> threshold =
        dialog.line_number(QStringLiteral("other_threshold"));
    if (threshold.has_value() && *threshold >= 0.0 && *threshold <= 100.0) {
        configuration.pareto_other_threshold_percent = threshold;
    } else if (threshold.has_value()) {
        QMessageBox::information(
            this, QStringLiteral("参数无效"), QStringLiteral("Other 合并阈值必须在 0 到 100 之间。"));
        return;
    }
    publish_page(datalab::application::AnalysisService::pareto(table_, configuration));
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
