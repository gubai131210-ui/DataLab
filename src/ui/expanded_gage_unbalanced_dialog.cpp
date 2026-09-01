#include "ui/expanded_gage_unbalanced_dialog.h"

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

ExpandedGageUnbalancedDialog::ExpandedGageUnbalancedDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("不平衡 Expanded Gage R&R"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* columns_body = new QWidget(stack_);
    auto* form = new QFormLayout(columns_body);
    measurement_combo_ = new QComboBox(columns_body);
    part_combo_ = new QComboBox(columns_body);
    operator_combo_ = new QComboBox(columns_body);
    additional_combo_ = new QComboBox(columns_body);
    additional_check_ = new QCheckBox(QStringLiteral("包含附加因子"), columns_body);
    for (int i = 0; i < column_labels.size(); ++i) {
        measurement_combo_->addItem(column_labels[i], i);
        part_combo_->addItem(column_labels[i], i);
        operator_combo_->addItem(column_labels[i], i);
        additional_combo_->addItem(column_labels[i], i);
    }
    form->addRow(QStringLiteral("测量列"), measurement_combo_);
    form->addRow(QStringLiteral("Part"), part_combo_);
    form->addRow(QStringLiteral("Operator"), operator_combo_);
    form->addRow(additional_check_);
    form->addRow(QStringLiteral("附加因子"), additional_combo_);
    additional_combo_->setEnabled(false);
    connect(additional_check_, &QCheckBox::toggled, additional_combo_, &QWidget::setEnabled);
    stack_->addWidget(make_titled_page(QStringLiteral("测量与因子"), columns_body, stack_));

    auto* roles_body = new QWidget(stack_);
    auto* roles_layout = new QVBoxLayout(roles_body);
    part_random_check_ = new QCheckBox(QStringLiteral("Part 为随机效应"), roles_body);
    operator_random_check_ = new QCheckBox(QStringLiteral("Operator 为随机效应"), roles_body);
    additional_random_check_ = new QCheckBox(QStringLiteral("附加因子为随机效应"), roles_body);
    part_random_check_->setChecked(true);
    operator_random_check_->setChecked(true);
    additional_random_check_->setChecked(true);
    roles_layout->addWidget(part_random_check_);
    roles_layout->addWidget(operator_random_check_);
    roles_layout->addWidget(additional_random_check_);
    roles_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("随机/固定"), roles_body, stack_));

    auto* method_body = new QWidget(stack_);
    auto* method_form = new QFormLayout(method_body);
    tolerance_spin_ = new QDoubleSpinBox(method_body);
    tolerance_spin_->setMinimum(0.0);
    tolerance_spin_->setMaximum(1.0e9);
    tolerance_spin_->setDecimals(4);
    tolerance_spin_->setSpecialValueText(QStringLiteral("（无）"));
    method_form->addRow(QStringLiteral("公差（可选）"), tolerance_spin_);
    method_note_ = new QPlainTextEdit(method_body);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "不平衡 GLM 方差分量（formula_reference）：\n\n"
        "允许不等 Part×Operator 重复。\n"
        "VarComp 由 MS 估计；%Contribution、%Study Var、NDC。\n"
        "complete-case；保留 source_row。"));
    method_form->addRow(method_note_);
    stack_->addWidget(make_titled_page(QStringLiteral("GLM/方差分量"), method_body, stack_));

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

    connect(back_button_, &QPushButton::clicked, this, &ExpandedGageUnbalancedDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &ExpandedGageUnbalancedDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &ExpandedGageUnbalancedDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::ExpandedGageUnbalancedConfiguration
ExpandedGageUnbalancedDialog::configuration() const
{
    datalab::domain::ExpandedGageUnbalancedConfiguration cfg;
    cfg.measurement_column = static_cast<std::size_t>(
        measurement_combo_->currentData().toInt());
    cfg.part_column = static_cast<std::size_t>(part_combo_->currentData().toInt());
    cfg.operator_column = static_cast<std::size_t>(
        operator_combo_->currentData().toInt());
    cfg.include_additional_factor = additional_check_->isChecked();
    if (cfg.include_additional_factor) {
        cfg.additional_column = static_cast<std::size_t>(
            additional_combo_->currentData().toInt());
    }
    cfg.part_random = part_random_check_->isChecked();
    cfg.operator_random = operator_random_check_->isChecked();
    cfg.additional_random = additional_random_check_->isChecked();
    if (tolerance_spin_->value() > 0.0) {
        cfg.tolerance = tolerance_spin_->value();
    }
    return cfg;
}

bool ExpandedGageUnbalancedDialog::validate_columns(QString* error) const
{
    const int m = measurement_combo_->currentData().toInt();
    const int p = part_combo_->currentData().toInt();
    const int o = operator_combo_->currentData().toInt();
    const int a = additional_combo_->currentData().toInt();
    if (m == p || m == o || p == o) {
        if (error != nullptr) {
            *error = QStringLiteral("测量、Part 与 Operator 列必须互不相同。");
        }
        return false;
    }
    if (additional_check_->isChecked() && (m == a || p == a || o == a)) {
        if (error != nullptr) {
            *error = QStringLiteral("附加因子列须与其他列不同。");
        }
        return false;
    }
    return true;
}

void ExpandedGageUnbalancedDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("测量=%1 Part=%2 Operator=%3\n附加=%4\n公差=%5\n\n"
                       "将输出 VarComp、%Contribution、%Study Var、NDC。")
            .arg(cfg.measurement_column.value_or(0))
            .arg(cfg.part_column.value_or(0))
            .arg(cfg.operator_column.value_or(0))
            .arg(cfg.include_additional_factor ? 1 : 0)
            .arg(cfg.tolerance.has_value() ? *cfg.tolerance : 0.0));
}

void ExpandedGageUnbalancedDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void ExpandedGageUnbalancedDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void ExpandedGageUnbalancedDialog::on_next()
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

void ExpandedGageUnbalancedDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
