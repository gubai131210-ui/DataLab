#include "ui/nhpp_repairable_dialog.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

namespace {

QWidget* make_nhpp_page(const QString& title, QWidget* body, QWidget* parent)
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

NhppRepairableDialog::NhppRepairableDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("可修复系统 NHPP"));
    setMinimumSize(700, 500);
    resize(780, 560);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* data_body = new QWidget(stack_);
    auto* data_layout = new QVBoxLayout(data_body);
    data_layout->addWidget(new QLabel(QStringLiteral("累积失效时间列"), data_body));
    time_combo_ = new QComboBox(data_body);
    for (int i = 0; i < column_labels.size(); ++i) {
        time_combo_->addItem(column_labels[i], i);
    }
    data_layout->addWidget(time_combo_);
    use_custom_t_ = new QCheckBox(QStringLiteral("指定截尾时间 T（否则取最大 ti）"), data_body);
    data_layout->addWidget(use_custom_t_);
    t_spin_ = new QDoubleSpinBox(data_body);
    t_spin_->setDecimals(6);
    t_spin_->setRange(1e-9, 1e12);
    t_spin_->setValue(100.0);
    t_spin_->setEnabled(false);
    data_layout->addWidget(t_spin_);
    connect(use_custom_t_, &QCheckBox::toggled, t_spin_, &QWidget::setEnabled);
    duane_check_ = new QCheckBox(QStringLiteral("输出 Duane 图（趋势参考）"), data_body);
    duane_check_->setChecked(true);
    data_layout->addWidget(duane_check_);
    data_layout->addStretch(1);
    stack_->addWidget(make_nhpp_page(QStringLiteral("数据"), data_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "Crow–AMSAA 幂律 NHPP（NIST APR，formula_reference）：\n\n"
        "λ(t) = λ β t^{β−1}，M(t) = λ t^β\n"
        "MLE：β̂ = n / Σ ln(T/ti)，λ̂ = n / T^{β̂}\n\n"
        "Duane 图仅作累积 MTBF 趋势参考，禁止解释为「ROCOF 合格 / 已证明稳定」。"));
    stack_->addWidget(make_nhpp_page(QStringLiteral("方法"), method_note_, stack_));

    results_note_ = new QPlainTextEdit(stack_);
    results_note_->setReadOnly(true);
    stack_->addWidget(make_nhpp_page(QStringLiteral("结果说明"), results_note_, stack_));

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

    connect(back_button_, &QPushButton::clicked, this, &NhppRepairableDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &NhppRepairableDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &NhppRepairableDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::NhppRepairableConfiguration NhppRepairableDialog::configuration() const
{
    datalab::domain::NhppRepairableConfiguration cfg;
    if (time_combo_->currentIndex() >= 0) {
        cfg.time_column = static_cast<std::size_t>(time_combo_->currentData().toInt());
    }
    if (use_custom_t_->isChecked()) {
        cfg.truncation_time = t_spin_->value();
    }
    cfg.include_duane_plot = duane_check_->isChecked();
    return cfg;
}

bool NhppRepairableDialog::validate_data(QString* error) const
{
    if (time_combo_->count() == 0 || time_combo_->currentIndex() < 0) {
        if (error != nullptr) {
            *error = QStringLiteral("请选择时间列。");
        }
        return false;
    }
    return true;
}

void NhppRepairableDialog::rebuild_results_note()
{
    results_note_->setPlainText(QStringLiteral(
        "将输出：\n"
        "• Observation Summary（n、T）\n"
        "• Parameter Estimates（β、λ）\n"
        "• Intensity / Mean function 表\n"
        "%1"
        "\n解释层只陈述参数与曲线，不宣称 ROCOF 合格或过程已稳定。")
                                    .arg(duane_check_->isChecked()
                                             ? QStringLiteral("• 可选 Duane 图\n")
                                             : QString()));
}

void NhppRepairableDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == 2) {
        rebuild_results_note();
    }
}

void NhppRepairableDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void NhppRepairableDialog::on_next()
{
    if (stack_->currentIndex() == 0) {
        QString error;
        if (!validate_data(&error)) {
            QMessageBox::warning(this, QStringLiteral("数据不足"), error);
            return;
        }
    }
    if (stack_->currentIndex() + 1 < stack_->count()) {
        stack_->setCurrentIndex(stack_->currentIndex() + 1);
        update_nav();
    }
}

void NhppRepairableDialog::on_accept()
{
    QString error;
    if (!validate_data(&error)) {
        QMessageBox::warning(this, QStringLiteral("数据不足"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
