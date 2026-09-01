#include "ui/mixture_process_variable_dialog.h"

#include <algorithm>
#include <QAbstractItemView>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

namespace {

QWidget* make_titled_page(const QString& title, QWidget* body, QWidget* parent)
{
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    auto* heading = new QLabel(title, page);
    heading->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 700;"));
    layout->addWidget(heading);
    layout->addWidget(body, 1);
    return page;
}

}  // namespace

MixtureProcessVariableDialog::MixtureProcessVariableDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Mixture + 过程变量"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* columns_body = new QWidget(stack_);
    auto* columns_layout = new QHBoxLayout(columns_body);
    component_list_ = new QListWidget(columns_body);
    component_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    auto* right = new QWidget(columns_body);
    auto* right_form = new QFormLayout(right);
    response_combo_ = new QComboBox(right);
    process_combo_ = new QComboBox(right);
    for (int i = 0; i < column_labels.size(); ++i) {
        auto* item = new QListWidgetItem(column_labels[i], component_list_);
        item->setData(Qt::UserRole, i);
        response_combo_->addItem(column_labels[i], i);
        process_combo_->addItem(column_labels[i], i);
    }
    right_form->addRow(QStringLiteral("响应列"), response_combo_);
    right_form->addRow(QStringLiteral("过程变量"), process_combo_);
    columns_layout->addWidget(component_list_, 1);
    columns_layout->addWidget(right, 1);
    stack_->addWidget(make_titled_page(QStringLiteral("组分与过程"), columns_body, stack_));

    auto* model_body = new QWidget(stack_);
    auto* model_layout = new QVBoxLayout(model_body);
    order_combo_ = new QComboBox(model_body);
    order_combo_->addItem(QStringLiteral("线性 Scheffé"), QStringLiteral("linear"));
    order_combo_->addItem(QStringLiteral("二次 (+ x_i x_j)"), QStringLiteral("quadratic"));
    interaction_check_ = new QCheckBox(QStringLiteral("包含组分×过程交互"), model_body);
    interaction_check_->setChecked(true);
    model_layout->addWidget(new QLabel(QStringLiteral("组分阶"), model_body));
    model_layout->addWidget(order_combo_);
    model_layout->addWidget(interaction_check_);
    model_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("模型"), model_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "Scheffé + 过程变量（无截距，formula_reference）：\n\n"
        "Y = Σ b_i x_i + Σ b_{ij} x_i x_j + γ X_1 + Σ δ_i x_i X_1\n"
        "约束 Σ x_i ≈ 1（容差 0.05）。\n\n"
        "独立于纯组分 Mixture 分析命令对话框。"));
    stack_->addWidget(make_titled_page(QStringLiteral("Scheffé 方法"), method_note_, stack_));

    preview_ = new QPlainTextEdit(stack_);
    preview_->setReadOnly(true);
    stack_->addWidget(make_titled_page(QStringLiteral("预览"), preview_, stack_));

    root->addWidget(stack_, 1);
    auto* nav = new QHBoxLayout();
    back_button_ = new QPushButton(QStringLiteral("上一步"), this);
    next_button_ = new QPushButton(QStringLiteral("下一步"), this);
    run_button_ = new QPushButton(QStringLiteral("运行"), this);
    auto* cancel = new QPushButton(QStringLiteral("取消"), this);
    nav->addWidget(back_button_);
    nav->addStretch(1);
    nav->addWidget(next_button_);
    nav->addWidget(run_button_);
    nav->addWidget(cancel);
    root->addLayout(nav);

    connect(back_button_, &QPushButton::clicked, this, &MixtureProcessVariableDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &MixtureProcessVariableDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &MixtureProcessVariableDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::MixtureProcessVariableConfiguration
MixtureProcessVariableDialog::configuration() const
{
    datalab::domain::MixtureProcessVariableConfiguration cfg;
    for (int i = 0; i < component_list_->count(); ++i) {
        if (component_list_->item(i)->isSelected()) {
            cfg.component_columns.push_back(
                static_cast<std::size_t>(component_list_->item(i)->data(Qt::UserRole).toInt()));
        }
    }
    cfg.response_column = static_cast<std::size_t>(response_combo_->currentData().toInt());
    cfg.process_column = static_cast<std::size_t>(process_combo_->currentData().toInt());
    cfg.component_order = order_combo_->currentData().toString().toStdString();
    cfg.include_component_process_interaction = interaction_check_->isChecked();
    return cfg;
}

bool MixtureProcessVariableDialog::validate_columns(QString* error) const
{
    const auto cfg = configuration();
    if (cfg.component_columns.size() < 2 || cfg.component_columns.size() > 4) {
        if (error != nullptr) {
            *error = QStringLiteral("请选择 2～4 个组分列。");
        }
        return false;
    }
    const int y = static_cast<int>(*cfg.response_column);
    const int x = static_cast<int>(*cfg.process_column);
    if (std::find(cfg.component_columns.cbegin(), cfg.component_columns.cend(),
                  cfg.response_column)
            != cfg.component_columns.cend()
        || std::find(cfg.component_columns.cbegin(), cfg.component_columns.cend(),
                     cfg.process_column)
            != cfg.component_columns.cend()
        || y == x) {
        if (error != nullptr) {
            *error = QStringLiteral("响应、过程变量与组分列须互不相同。");
        }
        return false;
    }
    return true;
}

void MixtureProcessVariableDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("组分数=%1 响应=%2 过程=%3 阶=%4 交互=%5\n\n"
                       "将输出 Scheffé 系数与 ANOVA。")
            .arg(cfg.component_columns.size())
            .arg(cfg.response_column.value_or(0))
            .arg(cfg.process_column.value_or(0))
            .arg(QString::fromStdString(cfg.component_order))
            .arg(cfg.include_component_process_interaction ? 1 : 0));
}

void MixtureProcessVariableDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void MixtureProcessVariableDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void MixtureProcessVariableDialog::on_next()
{
    if (stack_->currentIndex() == 0) {
        QString error;
        if (!validate_columns(&error)) {
            QMessageBox::warning(this, QStringLiteral("列无效"), error);
            return;
        }
    }
    if (stack_->currentIndex() + 1 < stack_->count()) {
        stack_->setCurrentIndex(stack_->currentIndex() + 1);
        update_nav();
    }
}

void MixtureProcessVariableDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
