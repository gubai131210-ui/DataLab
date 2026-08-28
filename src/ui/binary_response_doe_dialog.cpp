#include "ui/binary_response_doe_dialog.h"

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

BinaryResponseDoeDialog::BinaryResponseDoeDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("二值响应 DOE"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* layout_body = new QWidget(stack_);
    auto* layout_form = new QFormLayout(layout_body);
    factor_list_ = new QListWidget(layout_body);
    factor_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int i = 0; i < column_labels.size(); ++i) {
        auto* item = new QListWidgetItem(column_labels[i], factor_list_);
        item->setData(Qt::UserRole, i);
    }
    layout_form->addRow(QStringLiteral("因子列（多选）"), factor_list_);
    events_trials_radio_ = new QRadioButton(QStringLiteral("events + trials 两列"), layout_body);
    binary_radio_ = new QRadioButton(QStringLiteral("单列 0/1"), layout_body);
    events_trials_radio_->setChecked(true);
    events_combo_ = new QComboBox(layout_body);
    trials_combo_ = new QComboBox(layout_body);
    binary_combo_ = new QComboBox(layout_body);
    for (int i = 0; i < column_labels.size(); ++i) {
        events_combo_->addItem(column_labels[i], i);
        trials_combo_->addItem(column_labels[i], i);
        binary_combo_->addItem(column_labels[i], i);
    }
    layout_form->addRow(events_trials_radio_);
    layout_form->addRow(QStringLiteral("Events"), events_combo_);
    layout_form->addRow(QStringLiteral("Trials"), trials_combo_);
    layout_form->addRow(binary_radio_);
    layout_form->addRow(QStringLiteral("Binary 0/1"), binary_combo_);
    stack_->addWidget(make_titled_page(QStringLiteral("因子与响应"), layout_body, stack_));

    auto* model_body = new QWidget(stack_);
    auto* model_layout = new QVBoxLayout(model_body);
    interaction_check_ = new QCheckBox(QStringLiteral("包含 A×B 交互（前两因子）"), model_body);
    interaction_check_->setChecked(true);
    model_layout->addWidget(interaction_check_);
    model_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("模型"), model_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "Logit IRWLS（formula_reference）：\n\n"
        "η = Xβ；p = logit⁻¹(η)\n"
        "OR = exp(β)\n"
        "events/trials：展开为二项观测；须 0 ≤ events ≤ trials。\n\n"
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

    connect(back_button_, &QPushButton::clicked, this, &BinaryResponseDoeDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &BinaryResponseDoeDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &BinaryResponseDoeDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::BinaryResponseDoeConfiguration BinaryResponseDoeDialog::configuration() const
{
    datalab::domain::BinaryResponseDoeConfiguration cfg;
    for (QListWidgetItem* item : factor_list_->selectedItems()) {
        cfg.factor_columns.push_back(static_cast<std::size_t>(item->data(Qt::UserRole).toInt()));
    }
    cfg.use_events_trials = events_trials_radio_->isChecked();
    if (cfg.use_events_trials) {
        cfg.events_column = static_cast<std::size_t>(events_combo_->currentData().toInt());
        cfg.trials_column = static_cast<std::size_t>(trials_combo_->currentData().toInt());
    } else {
        cfg.binary_column = static_cast<std::size_t>(binary_combo_->currentData().toInt());
    }
    cfg.include_ab_interaction = interaction_check_->isChecked();
    return cfg;
}

bool BinaryResponseDoeDialog::validate_input(QString* error) const
{
    if (factor_list_->selectedItems().size() < 1) {
        if (error != nullptr) {
            *error = QStringLiteral("请至少选择一个因子列。");
        }
        return false;
    }
    return true;
}

void BinaryResponseDoeDialog::rebuild_preview()
{
    const auto cfg = configuration();
    QString text;
    text += QStringLiteral("因子数：%1\n").arg(cfg.factor_columns.size());
    text += cfg.use_events_trials
                ? QStringLiteral("响应：events/trials\n")
                : QStringLiteral("响应：0/1 单列\n");
    text += QStringLiteral("A×B 交互：%1\n\n")
                .arg(cfg.include_ab_interaction ? QStringLiteral("是")
                                                : QStringLiteral("否"));
    text += QStringLiteral("将输出 Coefficients、Odds Ratios、Goodness-of-Fit。");
    preview_->setPlainText(text);
}

void BinaryResponseDoeDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void BinaryResponseDoeDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void BinaryResponseDoeDialog::on_next()
{
    if (stack_->currentIndex() == 0) {
        QString error;
        if (!validate_input(&error)) {
            QMessageBox::warning(this, QStringLiteral("输入无效"), error);
            return;
        }
    }
    if (stack_->currentIndex() + 1 < stack_->count()) {
        stack_->setCurrentIndex(stack_->currentIndex() + 1);
        update_nav();
    }
}

void BinaryResponseDoeDialog::on_accept()
{
    QString error;
    if (!validate_input(&error)) {
        QMessageBox::warning(this, QStringLiteral("输入无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
