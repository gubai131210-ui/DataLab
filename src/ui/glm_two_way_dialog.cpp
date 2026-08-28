#include "ui/glm_two_way_dialog.h"

#include <QFormLayout>
#include <QHBoxLayout>
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

GlmTwoWayDialog::GlmTwoWayDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("双因子 GLM"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* columns_body = new QWidget(stack_);
    auto* form = new QFormLayout(columns_body);
    response_combo_ = new QComboBox(columns_body);
    factor_a_combo_ = new QComboBox(columns_body);
    factor_b_combo_ = new QComboBox(columns_body);
    for (int i = 0; i < column_labels.size(); ++i) {
        response_combo_->addItem(column_labels[i], i);
        factor_a_combo_->addItem(column_labels[i], i);
        factor_b_combo_->addItem(column_labels[i], i);
    }
    form->addRow(QStringLiteral("响应列"), response_combo_);
    form->addRow(QStringLiteral("因子 A"), factor_a_combo_);
    form->addRow(QStringLiteral("因子 B"), factor_b_combo_);
    stack_->addWidget(make_titled_page(QStringLiteral("列选择"), columns_body, stack_));

    auto* model_body = new QWidget(stack_);
    auto* model_layout = new QVBoxLayout(model_body);
    interaction_check_ = new QCheckBox(QStringLiteral("包含 A×B 交互"), model_body);
    interaction_check_->setChecked(true);
    model_layout->addWidget(interaction_check_);
    model_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("模型"), model_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "不平衡双因子 GLM（formula_reference）：\n\n"
        "Y = Xβ + ε；β̂ = (X'X)⁻¹X'y\n"
        "Type III Adj SS：项在其余项已在模型下的调整平方和。\n"
        "Fitted Means：按因子水平平均回归预测 ŷ（非原始单元均值）。\n\n"
        "complete-case；保留 source_row 用于残差回溯。"));
    stack_->addWidget(make_titled_page(QStringLiteral("方法"), method_note_, stack_));

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

    connect(back_button_, &QPushButton::clicked, this, &GlmTwoWayDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &GlmTwoWayDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &GlmTwoWayDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::GlmTwoWayConfiguration GlmTwoWayDialog::configuration() const
{
    datalab::domain::GlmTwoWayConfiguration cfg;
    cfg.response_column = static_cast<std::size_t>(
        response_combo_->currentData().toInt());
    cfg.factor_a_column = static_cast<std::size_t>(
        factor_a_combo_->currentData().toInt());
    cfg.factor_b_column = static_cast<std::size_t>(
        factor_b_combo_->currentData().toInt());
    cfg.include_interaction = interaction_check_->isChecked();
    return cfg;
}

bool GlmTwoWayDialog::validate_columns(QString* error) const
{
    const int resp = response_combo_->currentData().toInt();
    const int fa = factor_a_combo_->currentData().toInt();
    const int fb = factor_b_combo_->currentData().toInt();
    if (resp == fa || resp == fb || fa == fb) {
        if (error != nullptr) {
            *error = QStringLiteral("响应与两个因子列必须互不相同。");
        }
        return false;
    }
    return true;
}

void GlmTwoWayDialog::rebuild_preview()
{
    const auto cfg = configuration();
    QString text;
    text += QStringLiteral("响应列索引：%1\n")
                .arg(cfg.response_column.value_or(0));
    text += QStringLiteral("因子 A：%1    因子 B：%2\n")
                .arg(cfg.factor_a_column.value_or(0))
                .arg(cfg.factor_b_column.value_or(0));
    text += QStringLiteral("交互：%1\n\n")
                .arg(cfg.include_interaction ? QStringLiteral("是")
                                             : QStringLiteral("否"));
    text += QStringLiteral("将输出 Type III ANOVA、Fitted Means、残差诊断。");
    preview_->setPlainText(text);
}

void GlmTwoWayDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void GlmTwoWayDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void GlmTwoWayDialog::on_next()
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

void GlmTwoWayDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
