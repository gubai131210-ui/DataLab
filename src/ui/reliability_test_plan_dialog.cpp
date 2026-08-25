#include "ui/reliability_test_plan_dialog.h"

#include "domain/statistics/reliability_test_plan.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

namespace {

QWidget* make_rtp_page(const QString& title, QWidget* body, QWidget* parent)
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

ReliabilityTestPlanDialog::ReliabilityTestPlanDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("可靠性试验计划"));
    setMinimumSize(700, 520);
    resize(780, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* inputs_body = new QWidget(stack_);
    auto* inputs_layout = new QVBoxLayout(inputs_body);
    auto add_spin = [&](const QString& label, QDoubleSpinBox** out, double value,
                        double min_v, double max_v, int decimals) {
        inputs_layout->addWidget(new QLabel(label, inputs_body));
        *out = new QDoubleSpinBox(inputs_body);
        (*out)->setDecimals(decimals);
        (*out)->setRange(min_v, max_v);
        (*out)->setValue(value);
        inputs_layout->addWidget(*out);
    };
    add_spin(QStringLiteral("Weibull 形状 β（假设）"), &beta_spin_, 1.0, 1e-6, 100.0, 6);
    add_spin(QStringLiteral("目标可靠度 R（0–1）"), &r_spin_, 0.9, 1e-9, 1.0 - 1e-9, 6);
    add_spin(QStringLiteral("置信水平 CL（0–1）"), &cl_spin_, 0.9, 1e-9, 1.0 - 1e-9, 6);
    add_spin(QStringLiteral("试验时长 T0"), &t0_spin_, 1.0, 1e-9, 1e12, 6);
    add_spin(QStringLiteral("任务时长 tm"), &tm_spin_, 1.0, 1e-9, 1e12, 6);
    inputs_layout->addWidget(new QLabel(QStringLiteral("允许失效数 r"), inputs_body));
    allowed_spin_ = new QSpinBox(inputs_body);
    allowed_spin_->setRange(0, 50);
    allowed_spin_->setValue(0);
    inputs_layout->addWidget(allowed_spin_);
    inputs_layout->addStretch(1);
    stack_->addWidget(make_rtp_page(QStringLiteral("输入"), inputs_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "Weibull 演示型试验计划（formula_reference）：\n\n"
        "δ = (T0/tm)^β\n"
        "零失效：n = ceil( ln(1−CL) / (δ ln R) )\n"
        "允许 r 次失效：在 R_test = R^δ 下搜索最小 n 使二项累积 ≤ 1−CL。\n\n"
        "β 为假设，非数据估计；结果页与输入页分离。"));
    stack_->addWidget(make_rtp_page(QStringLiteral("方法说明"), method_note_, stack_));

    results_ = new QPlainTextEdit(stack_);
    results_->setReadOnly(true);
    stack_->addWidget(make_rtp_page(QStringLiteral("结果预览"), results_, stack_));

    root->addWidget(stack_, 1);
    auto* nav = new QHBoxLayout();
    back_button_ = new QPushButton(QStringLiteral("上一步"), this);
    next_button_ = new QPushButton(QStringLiteral("下一步"), this);
    run_button_ = new QPushButton(QStringLiteral("生成计划"), this);
    auto* cancel = new QPushButton(QStringLiteral("取消"), this);
    nav->addWidget(back_button_);
    nav->addStretch(1);
    nav->addWidget(next_button_);
    nav->addWidget(run_button_);
    nav->addWidget(cancel);
    root->addLayout(nav);

    connect(back_button_, &QPushButton::clicked, this, &ReliabilityTestPlanDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &ReliabilityTestPlanDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &ReliabilityTestPlanDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::ReliabilityTestPlanConfiguration
ReliabilityTestPlanDialog::configuration() const
{
    datalab::domain::ReliabilityTestPlanConfiguration cfg;
    cfg.shape_beta = beta_spin_->value();
    cfg.target_reliability = r_spin_->value();
    cfg.confidence_level = cl_spin_->value();
    cfg.test_time = t0_spin_->value();
    cfg.mission_time = tm_spin_->value();
    cfg.allowed_failures = static_cast<std::size_t>(allowed_spin_->value());
    return cfg;
}

bool ReliabilityTestPlanDialog::validate_inputs(QString* error) const
{
    if (!(r_spin_->value() > 0.0) || !(r_spin_->value() < 1.0)) {
        if (error != nullptr) {
            *error = QStringLiteral("R 必须满足 0 < R < 1。");
        }
        return false;
    }
    if (!(cl_spin_->value() > 0.0) || !(cl_spin_->value() < 1.0)) {
        if (error != nullptr) {
            *error = QStringLiteral("CL 必须满足 0 < CL < 1。");
        }
        return false;
    }
    return true;
}

void ReliabilityTestPlanDialog::rebuild_results()
{
    const auto cfg = configuration();
    datalab::domain::statistics::ReliabilityTestPlanOptions options;
    options.shape_beta = cfg.shape_beta;
    options.target_reliability = cfg.target_reliability;
    options.confidence_level = cfg.confidence_level;
    options.test_time = cfg.test_time;
    options.mission_time = cfg.mission_time;
    options.allowed_failures = cfg.allowed_failures;
    const auto plan =
        datalab::domain::statistics::plan_reliability_demonstration(options);

    QString text;
    if (plan.sample_size.has_value()) {
        text += QStringLiteral("预览样本量 n = %1\n").arg(*plan.sample_size);
    } else {
        text += QStringLiteral("预览：无法计算 n（请检查输入）。\n");
    }
    text += QStringLiteral("允许失效 r = %1\n").arg(cfg.allowed_failures);
    text += QStringLiteral("δ = %1\n").arg(plan.time_ratio_delta, 0, 'g', 6);
    text += QStringLiteral("\n假设摘要：β 为工程假设；演示型计划不宣称寿命已达标。\n");
    text += QStringLiteral("确认后写入正式输出页（Test Plan + Assumptions）。");
    results_->setPlainText(text);
}

void ReliabilityTestPlanDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == 2) {
        rebuild_results();
    }
}

void ReliabilityTestPlanDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void ReliabilityTestPlanDialog::on_next()
{
    if (stack_->currentIndex() == 0) {
        QString error;
        if (!validate_inputs(&error)) {
            QMessageBox::warning(this, QStringLiteral("输入无效"), error);
            return;
        }
    }
    if (stack_->currentIndex() + 1 < stack_->count()) {
        stack_->setCurrentIndex(stack_->currentIndex() + 1);
        update_nav();
    }
}

void ReliabilityTestPlanDialog::on_accept()
{
    QString error;
    if (!validate_inputs(&error)) {
        QMessageBox::warning(this, QStringLiteral("输入无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
