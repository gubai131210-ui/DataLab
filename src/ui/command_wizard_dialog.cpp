#include "ui/command_wizard_dialog.h"

#include "application/command_recommendation_engine.h"
#include "ui/analysis_commands.h"
#include "ui/app_ui_tr.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace datalab::ui {
namespace {

QString reason_display(const std::string& reason_key)
{
    static const QHash<QString, QString> k_reasons = {
        {QStringLiteral("reason.univariate_describe"), QStringLiteral("单变量描述与分布")},
        {QStringLiteral("reason.imr"), QStringLiteral("个体计量控制图")},
        {QStringLiteral("reason.capability"),
         QStringLiteral("过程能力（规格在对话框中填写）")},
        {QStringLiteral("reason.two_sample_compare"), QStringLiteral("两组数值比较")},
        {QStringLiteral("reason.associate"), QStringLiteral("相关、回归或散点")},
        {QStringLiteral("reason.factor_group"), QStringLiteral("因子分组比较")},
        {QStringLiteral("reason.multivariate"), QStringLiteral("多变量关联")},
        {QStringLiteral("reason.categorical"), QStringLiteral("属性/分类分析")},
        {QStringLiteral("reason.reliability"),
         QStringLiteral("生存/可靠性（请核对删失编码）")},
        {QStringLiteral("reason.graph_explore"), QStringLiteral("探索图形")},
    };
    const QString key = QString::fromStdString(reason_key);
    const QString source = k_reasons.value(key, key);
    return ui_tr(source);
}

QString hint_display(const std::string& hint_key)
{
    static const QHash<QString, QString> k_hints = {
        {QStringLiteral("hint.select_columns"), QStringLiteral("请先选择至少一列")},
        {QStringLiteral("hint.check_column_types"),
         QStringLiteral("请核对列类型后再推荐")},
    };
    const QString key = QString::fromStdString(hint_key);
    const QString source = k_hints.value(key, key);
    return ui_tr(source);
}

QString column_type_label(datalab::domain::ColumnType type)
{
    switch (type) {
    case datalab::domain::ColumnType::numeric:
        return ui_tr(QStringLiteral("数值"));
    case datalab::domain::ColumnType::categorical:
        return ui_tr(QStringLiteral("分类"));
    case datalab::domain::ColumnType::time:
        return ui_tr(QStringLiteral("时间"));
    case datalab::domain::ColumnType::unknown:
    default:
        return ui_tr(QStringLiteral("未知"));
    }
}

datalab::application::CommandWizardIntent intent_from_combo(int index)
{
    using Intent = datalab::application::CommandWizardIntent;
    switch (index) {
    case 1:
        return Intent::describe;
    case 2:
        return Intent::compare;
    case 3:
        return Intent::associate;
    case 4:
        return Intent::control_chart;
    case 5:
        return Intent::capability;
    case 6:
        return Intent::reliability;
    case 7:
        return Intent::graph;
    case 0:
    default:
        return Intent::any;
    }
}

}  // namespace

CommandWizardDialog::CommandWizardDialog(
    const QStringList& column_names,
    const std::vector<datalab::domain::ColumnType>& column_types,
    QWidget* parent)
    : QDialog(parent)
    , column_names_(column_names)
    , column_types_(column_types)
{
    setObjectName(QStringLiteral("commandWizardDialog"));
    setWindowTitle(ui_tr(QStringLiteral("命令向导")));
    setMinimumSize(640, 480);
    build_pages();
    update_navigation();
}

void CommandWizardDialog::build_pages()
{
    auto* root = new QVBoxLayout(this);
    page_title_ = new QLabel(this);
    page_title_->setObjectName(QStringLiteral("commandWizardPageTitle"));
    page_title_->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 600;"));
    root->addWidget(page_title_);

    stack_ = new QStackedWidget(this);
    stack_->setObjectName(QStringLiteral("commandWizardStack"));

    // Step 0: columns
    auto* columns_page = new QWidget(stack_);
    auto* columns_layout = new QVBoxLayout(columns_page);
    columns_layout->addWidget(
        new QLabel(ui_tr(QStringLiteral("勾选要分析的列（按列类型推荐）")), columns_page));
    column_list_ = new QListWidget(columns_page);
    column_list_->setObjectName(QStringLiteral("commandWizardColumnList"));
    column_list_->setSelectionMode(QAbstractItemView::NoSelection);
    const int n = std::min(column_names_.size(), static_cast<int>(column_types_.size()));
    for (int i = 0; i < n; ++i) {
        const QString label = QStringLiteral("%1  [%2]")
                                  .arg(column_names_.at(i), column_type_label(column_types_[static_cast<std::size_t>(i)]));
        auto* item = new QListWidgetItem(label, column_list_);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setData(Qt::UserRole, i);
    }
    columns_layout->addWidget(column_list_);
    stack_->addWidget(columns_page);

    // Step 1: intent
    auto* intent_page = new QWidget(stack_);
    auto* intent_layout = new QVBoxLayout(intent_page);
    intent_layout->addWidget(
        new QLabel(ui_tr(QStringLiteral("选择分析意图（可选）")), intent_page));
    intent_combo_ = new QComboBox(intent_page);
    intent_combo_->setObjectName(QStringLiteral("commandWizardIntentCombo"));
    intent_combo_->addItem(ui_tr(QStringLiteral("不限（任意）")));
    intent_combo_->addItem(ui_tr(QStringLiteral("描述")));
    intent_combo_->addItem(ui_tr(QStringLiteral("比较")));
    intent_combo_->addItem(ui_tr(QStringLiteral("关联")));
    intent_combo_->addItem(ui_tr(QStringLiteral("控制图")));
    intent_combo_->addItem(ui_tr(QStringLiteral("过程能力")));
    intent_combo_->addItem(ui_tr(QStringLiteral("可靠性")));
    intent_combo_->addItem(ui_tr(QStringLiteral("图形探索")));
    intent_layout->addWidget(intent_combo_);
    intent_layout->addStretch();
    stack_->addWidget(intent_page);

    // Step 2: recommendations
    auto* recommend_page = new QWidget(stack_);
    auto* recommend_layout = new QVBoxLayout(recommend_page);
    recommend_layout->addWidget(
        new QLabel(ui_tr(QStringLiteral("推荐命令（点选一项后打开设置）")), recommend_page));
    recommendation_list_ = new QListWidget(recommend_page);
    recommendation_list_->setObjectName(QStringLiteral("commandWizardRecommendationList"));
    recommend_layout->addWidget(recommendation_list_);
    stack_->addWidget(recommend_page);

    root->addWidget(stack_, 1);

    status_label_ = new QLabel(this);
    status_label_->setObjectName(QStringLiteral("commandWizardStatus"));
    status_label_->setWordWrap(true);
    root->addWidget(status_label_);

    auto* nav = new QHBoxLayout();
    back_button_ = new QPushButton(ui_tr(QStringLiteral("上一步")), this);
    back_button_->setObjectName(QStringLiteral("commandWizardBack"));
    next_button_ = new QPushButton(ui_tr(QStringLiteral("下一步")), this);
    next_button_->setObjectName(QStringLiteral("commandWizardNext"));
    open_button_ = new QPushButton(ui_tr(QStringLiteral("打开分析设置")), this);
    open_button_->setObjectName(QStringLiteral("commandWizardOpen"));
    cancel_button_ = new QPushButton(ui_tr(QStringLiteral("取消")), this);
    cancel_button_->setObjectName(QStringLiteral("commandWizardCancel"));
    nav->addWidget(back_button_);
    nav->addStretch();
    nav->addWidget(cancel_button_);
    nav->addWidget(next_button_);
    nav->addWidget(open_button_);
    root->addLayout(nav);

    connect(back_button_, &QPushButton::clicked, this, &CommandWizardDialog::go_back);
    connect(next_button_, &QPushButton::clicked, this, &CommandWizardDialog::go_next);
    connect(open_button_, &QPushButton::clicked, this, &CommandWizardDialog::open_selected_command);
    connect(cancel_button_, &QPushButton::clicked, this, &QDialog::reject);
    connect(recommendation_list_, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem*) { open_selected_command(); });
}

void CommandWizardDialog::update_navigation()
{
    const int index = stack_->currentIndex();
    const int last = stack_->count() - 1;
    back_button_->setEnabled(index > 0);
    next_button_->setVisible(index < last);
    open_button_->setVisible(index == last);
    open_button_->setEnabled(index == last && recommendation_list_->currentItem() != nullptr);

    if (index == 0) {
        page_title_->setText(ui_tr(QStringLiteral("步骤 1：选择列")));
    } else if (index == 1) {
        page_title_->setText(ui_tr(QStringLiteral("步骤 2：选择意图")));
    } else {
        page_title_->setText(ui_tr(QStringLiteral("步骤 3：查看推荐")));
    }
}

void CommandWizardDialog::go_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_navigation();
    }
}

void CommandWizardDialog::go_next()
{
    if (stack_->currentIndex() >= stack_->count() - 1) {
        return;
    }
    const int next = stack_->currentIndex() + 1;
    stack_->setCurrentIndex(next);
    if (next == stack_->count() - 1) {
        refresh_recommendations();
    }
    update_navigation();
}

std::vector<datalab::domain::ColumnType> CommandWizardDialog::selected_column_types() const
{
    std::vector<datalab::domain::ColumnType> selected;
    for (int row = 0; row < column_list_->count(); ++row) {
        QListWidgetItem* item = column_list_->item(row);
        if (item == nullptr || item->checkState() != Qt::Checked) {
            continue;
        }
        const int index = item->data(Qt::UserRole).toInt();
        if (index < 0 || static_cast<std::size_t>(index) >= column_types_.size()) {
            continue;
        }
        selected.push_back(column_types_[static_cast<std::size_t>(index)]);
    }
    return selected;
}

void CommandWizardDialog::refresh_recommendations()
{
    recommendation_list_->clear();
    status_label_->clear();

    const auto intent = intent_from_combo(intent_combo_->currentIndex());
    const auto types = selected_column_types();
    // G6_ENGINE call site（Wizard 只读推荐，不跑分析）
    const datalab::application::RecommendResult result =
        datalab::application::recommend(types, intent);

    if (result.hint_key.has_value()) {
        status_label_->setText(hint_display(*result.hint_key));
    }

    for (const auto& item : result.recommendations) {
        const analysis_commands::AnalysisCommand* command =
            analysis_commands::find(QString::fromStdString(item.command_id));
        if (command == nullptr) {
            continue;
        }
        const QString path = ui_tr(command->menu_path) + QStringLiteral(" > ")
            + ui_tr(command->menu_group);
        const QString text = QStringLiteral("%1\n%2\n%3")
                                 .arg(ui_tr(command->menu_label), path,
                                      reason_display(item.reason_key));
        auto* row = new QListWidgetItem(text, recommendation_list_);
        row->setData(Qt::UserRole, QString::fromStdString(item.command_id));
    }

    if (recommendation_list_->count() > 0) {
        recommendation_list_->setCurrentRow(0);
    }
    connect(recommendation_list_, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem*, QListWidgetItem*) { update_navigation(); },
            Qt::UniqueConnection);
    update_navigation();
}

QString CommandWizardDialog::selected_command_id() const
{
    QListWidgetItem* item = recommendation_list_->currentItem();
    if (item == nullptr) {
        return {};
    }
    return item->data(Qt::UserRole).toString();
}

void CommandWizardDialog::open_selected_command()
{
    const QString command_id = selected_command_id();
    if (command_id.isEmpty()) {
        status_label_->setText(ui_tr(QStringLiteral("请先选择一条推荐命令")));
        return;
    }
    // 先发信号再 accept，便于 MainWindow 在 exec 返回后 run_from_spec（避免双模态）。
    emit openAnalysisRequested(command_id);
    accept();
}

}  // namespace datalab::ui
