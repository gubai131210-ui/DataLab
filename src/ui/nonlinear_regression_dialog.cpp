#include "ui/nonlinear_regression_dialog.h"

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

QComboBox* make_column_combo(const QStringList& labels, QWidget* parent)
{
    auto* combo = new QComboBox(parent);
    for (int i = 0; i < labels.size(); ++i) {
        combo->addItem(labels[i], i);
    }
    return combo;
}

}  // namespace

NonlinearRegressionDialog::NonlinearRegressionDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("非线性回归"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* p1_body = new QWidget(stack_);
    auto* p1_form = new QFormLayout(p1_body);
    response_combo_ = make_column_combo(column_labels, p1_body);
    predictor_combo_ = make_column_combo(column_labels, p1_body);
    p1_form->addRow(QStringLiteral("响应列"), response_combo_);
    p1_form->addRow(QStringLiteral("预测列"), predictor_combo_);
    stack_->addWidget(make_titled_page(QStringLiteral("响应与预测"), p1_body, stack_));

    auto* p2_body = new QWidget(stack_);
    auto* p2_form = new QFormLayout(p2_body);
    model_combo_ = new QComboBox(p2_body);
    model_combo_->addItem(QStringLiteral("Growth"), QStringLiteral("growth"));
    model_combo_->addItem(QStringLiteral("Decay"), QStringLiteral("decay"));
    model_combo_->addItem(QStringLiteral("Logistic Saturation"), QStringLiteral("logistic_saturation"));
    model_combo_->addItem(QStringLiteral("Michaelis-Menten"), QStringLiteral("michaelis_menten"));
    model_combo_->addItem(QStringLiteral("Power"), QStringLiteral("power"));
    start_a_spin_ = new QDoubleSpinBox(p2_body);
    start_b_spin_ = new QDoubleSpinBox(p2_body);
    start_a_spin_->setRange(-1.0e6, 1.0e6);
    start_b_spin_->setRange(-1.0e6, 1.0e6);
    start_a_spin_->setValue(10.0);
    start_b_spin_->setValue(1.0);
    p2_form->addRow(QStringLiteral("模型"), model_combo_);
    p2_form->addRow(QStringLiteral("初值 a"), start_a_spin_);
    p2_form->addRow(QStringLiteral("初值 b"), start_b_spin_);
    stack_->addWidget(make_titled_page(QStringLiteral("模型与初值"), p2_body, stack_));

    auto* p3_body = new QWidget(stack_);
    auto* p3_form = new QFormLayout(p3_body);
    algorithm_combo_ = new QComboBox(p3_body);
    algorithm_combo_->addItem(QStringLiteral("Gauss-Newton"), QStringLiteral("gn"));
    algorithm_combo_->addItem(QStringLiteral("Levenberg-Marquardt"), QStringLiteral("lm"));
    max_iter_spin_ = new QSpinBox(p3_body);
    max_iter_spin_->setRange(10, 1000);
    max_iter_spin_->setValue(100);
    tolerance_spin_ = new QDoubleSpinBox(p3_body);
    tolerance_spin_->setDecimals(8);
    tolerance_spin_->setRange(1.0e-12, 1.0);
    tolerance_spin_->setValue(1.0e-6);
    p3_form->addRow(QStringLiteral("算法"), algorithm_combo_);
    p3_form->addRow(QStringLiteral("最大迭代"), max_iter_spin_);
    p3_form->addRow(QStringLiteral("容差"), tolerance_spin_);
    stack_->addWidget(make_titled_page(QStringLiteral("GN/LM 与收敛"), p3_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "GN: δ=(J'J)⁻¹J'r\n"
        "LM: (J'J+λD)δ=J'r\n"
        "与线性回归对话框独立。"));
    stack_->addWidget(make_titled_page(QStringLiteral("方法说明"), method_note_, stack_));

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

    connect(back_button_, &QPushButton::clicked, this, &NonlinearRegressionDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &NonlinearRegressionDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &NonlinearRegressionDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::NonlinearRegressionConfiguration NonlinearRegressionDialog::configuration() const
{
    datalab::domain::NonlinearRegressionConfiguration cfg;
    cfg.response_column = static_cast<std::size_t>(response_combo_->currentData().toInt());
    cfg.predictor_column = static_cast<std::size_t>(predictor_combo_->currentData().toInt());
    cfg.model_id = model_combo_->currentData().toString().toStdString();
    cfg.algorithm = algorithm_combo_->currentData().toString().toStdString();
    cfg.starting_values = {start_a_spin_->value(), start_b_spin_->value()};
    cfg.max_iterations = static_cast<std::size_t>(max_iter_spin_->value());
    cfg.tolerance = tolerance_spin_->value();
    cfg.lm_lambda = 0.01;
    return cfg;
}

bool NonlinearRegressionDialog::validate_columns(QString* error) const
{
    if (response_combo_->currentData().toInt() < 0 || predictor_combo_->currentData().toInt() < 0) {
        if (error != nullptr) {
            *error = QStringLiteral("请选择响应列与预测列。");
        }
        return false;
    }
    return true;
}

void NonlinearRegressionDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("Y=%1 X=%2 模型=%3 算法=%4")
            .arg(cfg.response_column.value_or(0))
            .arg(cfg.predictor_column.value_or(0))
            .arg(QString::fromStdString(cfg.model_id))
            .arg(QString::fromStdString(cfg.algorithm)));
}

void NonlinearRegressionDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void NonlinearRegressionDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void NonlinearRegressionDialog::on_next()
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

void NonlinearRegressionDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}

#include "nonlinear_regression_dialog.moc"
