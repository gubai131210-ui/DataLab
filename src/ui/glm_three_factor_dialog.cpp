#include "ui/glm_three_factor_dialog.h"

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

GlmThreeFactorDialog::GlmThreeFactorDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("三因子 GLM"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* columns_body = new QWidget(stack_);
    auto* form = new QFormLayout(columns_body);
    response_combo_ = new QComboBox(columns_body);
    factor_a_combo_ = new QComboBox(columns_body);
    factor_b_combo_ = new QComboBox(columns_body);
    factor_c_combo_ = new QComboBox(columns_body);
    for (int i = 0; i < column_labels.size(); ++i) {
        response_combo_->addItem(column_labels[i], i);
        factor_a_combo_->addItem(column_labels[i], i);
        factor_b_combo_->addItem(column_labels[i], i);
        factor_c_combo_->addItem(column_labels[i], i);
    }
    form->addRow(QStringLiteral("响应列"), response_combo_);
    form->addRow(QStringLiteral("因子 A"), factor_a_combo_);
    form->addRow(QStringLiteral("因子 B"), factor_b_combo_);
    form->addRow(QStringLiteral("因子 C"), factor_c_combo_);
    stack_->addWidget(make_titled_page(QStringLiteral("列选择"), columns_body, stack_));

    auto* model_body = new QWidget(stack_);
    auto* model_layout = new QVBoxLayout(model_body);
    ab_check_ = new QCheckBox(QStringLiteral("包含 A×B 交互"), model_body);
    ac_check_ = new QCheckBox(QStringLiteral("包含 A×C 交互"), model_body);
    bc_check_ = new QCheckBox(QStringLiteral("包含 B×C 交互"), model_body);
    ab_check_->setChecked(true);
    ac_check_->setChecked(true);
    bc_check_->setChecked(true);
    model_layout->addWidget(ab_check_);
    model_layout->addWidget(ac_check_);
    model_layout->addWidget(bc_check_);
    model_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("模型"), model_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "三因子不平衡 GLM（formula_reference）：\n\n"
        "Y = Xβ + ε；Type III Adj SS\n"
        "Fitted Means：回归预测按水平平均。\n"
        "不含 ABC 三阶交互。\n\n"
        "complete-case；保留 source_row。"));
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

    connect(back_button_, &QPushButton::clicked, this, &GlmThreeFactorDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &GlmThreeFactorDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &GlmThreeFactorDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::GlmThreeFactorConfiguration GlmThreeFactorDialog::configuration() const
{
    datalab::domain::GlmThreeFactorConfiguration cfg;
    cfg.response_column = static_cast<std::size_t>(
        response_combo_->currentData().toInt());
    cfg.factor_a_column = static_cast<std::size_t>(
        factor_a_combo_->currentData().toInt());
    cfg.factor_b_column = static_cast<std::size_t>(
        factor_b_combo_->currentData().toInt());
    cfg.factor_c_column = static_cast<std::size_t>(
        factor_c_combo_->currentData().toInt());
    cfg.include_ab_interaction = ab_check_->isChecked();
    cfg.include_ac_interaction = ac_check_->isChecked();
    cfg.include_bc_interaction = bc_check_->isChecked();
    return cfg;
}

bool GlmThreeFactorDialog::validate_columns(QString* error) const
{
    const int resp = response_combo_->currentData().toInt();
    const int fa = factor_a_combo_->currentData().toInt();
    const int fb = factor_b_combo_->currentData().toInt();
    const int fc = factor_c_combo_->currentData().toInt();
    if (resp == fa || resp == fb || resp == fc || fa == fb || fa == fc || fb == fc) {
        if (error != nullptr) {
            *error = QStringLiteral("响应与三个因子列必须互不相同。");
        }
        return false;
    }
    return true;
}

void GlmThreeFactorDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("响应：%1\nA/B/C：%2 / %3 / %4\n"
                       "交互 AB=%5 AC=%6 BC=%7\n\n"
                       "将输出 Type III ANOVA 与 Fitted Means。")
            .arg(cfg.response_column.value_or(0))
            .arg(cfg.factor_a_column.value_or(0))
            .arg(cfg.factor_b_column.value_or(0))
            .arg(cfg.factor_c_column.value_or(0))
            .arg(cfg.include_ab_interaction ? 1 : 0)
            .arg(cfg.include_ac_interaction ? 1 : 0)
            .arg(cfg.include_bc_interaction ? 1 : 0));
}

void GlmThreeFactorDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void GlmThreeFactorDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void GlmThreeFactorDialog::on_next()
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

void GlmThreeFactorDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
