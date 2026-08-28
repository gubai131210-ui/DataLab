#include "ui/life_data_regression_dialog.h"

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

LifeDataRegressionDialog::LifeDataRegressionDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("寿命数据回归"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* data_body = new QWidget(stack_);
    auto* data_form = new QFormLayout(data_body);
    time_combo_ = new QComboBox(data_body);
    censor_combo_ = new QComboBox(data_body);
    covariate_list_ = new QListWidget(data_body);
    covariate_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int i = 0; i < column_labels.size(); ++i) {
        time_combo_->addItem(column_labels[i], i);
        censor_combo_->addItem(column_labels[i], i);
        auto* item = new QListWidgetItem(column_labels[i], covariate_list_);
        item->setData(Qt::UserRole, i);
    }
    data_form->addRow(QStringLiteral("时间列"), time_combo_);
    data_form->addRow(QStringLiteral("删失指示（1=失败）"), censor_combo_);
    data_form->addRow(QStringLiteral("协变量（1～2 列）"), covariate_list_);
    stack_->addWidget(make_titled_page(QStringLiteral("时间、删失与协变量"), data_body, stack_));

    auto* dist_body = new QWidget(stack_);
    auto* dist_form = new QFormLayout(dist_body);
    distribution_combo_ = new QComboBox(dist_body);
    distribution_combo_->addItem(QStringLiteral("Weibull"), QStringLiteral("weibull"));
    percentile_1_check_ = new QCheckBox(QStringLiteral("输出 1% 百分位"), dist_body);
    percentile_5_check_ = new QCheckBox(QStringLiteral("输出 5% 百分位"), dist_body);
    percentile_1_check_->setChecked(true);
    percentile_5_check_->setChecked(true);
    dist_form->addRow(QStringLiteral("分布"), distribution_combo_);
    dist_form->addRow(percentile_1_check_);
    dist_form->addRow(percentile_5_check_);
    stack_->addWidget(make_titled_page(QStringLiteral("分布与百分位"), dist_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "Weibull 寿命回归 MLE（formula_reference）：\n\n"
        "log Y_p = β₀ + Σβ_k x_k + σ Φ⁻¹(p)\n"
        "右删失：失败 log f(t)，删失 −F(t)\n"
        "Newton-Raphson MLE；非 accelerated_life。\n\n"
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

    connect(back_button_, &QPushButton::clicked, this, &LifeDataRegressionDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &LifeDataRegressionDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &LifeDataRegressionDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::LifeDataRegressionConfiguration LifeDataRegressionDialog::configuration() const
{
    datalab::domain::LifeDataRegressionConfiguration cfg;
    cfg.time_column = static_cast<std::size_t>(time_combo_->currentData().toInt());
    cfg.censor_column = static_cast<std::size_t>(censor_combo_->currentData().toInt());
    for (QListWidgetItem* item : covariate_list_->selectedItems()) {
        cfg.covariate_columns.push_back(
            static_cast<std::size_t>(item->data(Qt::UserRole).toInt()));
    }
    cfg.distribution = distribution_combo_->currentData().toString().toStdString();
    cfg.percentile_levels.clear();
    if (percentile_1_check_->isChecked()) {
        cfg.percentile_levels.push_back(1.0);
    }
    if (percentile_5_check_->isChecked()) {
        cfg.percentile_levels.push_back(5.0);
    }
    return cfg;
}

bool LifeDataRegressionDialog::validate_input(QString* error) const
{
    const int selected = covariate_list_->selectedItems().size();
    if (selected < 1 || selected > 2) {
        if (error != nullptr) {
            *error = QStringLiteral("请选择 1～2 个协变量列。");
        }
        return false;
    }
    return true;
}

void LifeDataRegressionDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("时间列：%1    删失列：%2\n协变量数：%3\n分布：%4\n\n"
                       "将输出 Regression Table 与 Percentile Table。")
            .arg(cfg.time_column.value_or(0))
            .arg(cfg.censor_column.value_or(0))
            .arg(cfg.covariate_columns.size())
            .arg(QString::fromStdString(cfg.distribution)));
}

void LifeDataRegressionDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void LifeDataRegressionDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void LifeDataRegressionDialog::on_next()
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

void LifeDataRegressionDialog::on_accept()
{
    QString error;
    if (!validate_input(&error)) {
        QMessageBox::warning(this, QStringLiteral("输入无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
