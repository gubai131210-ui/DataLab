#include "ui/life_data_lognormal_dialog.h"

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

QComboBox* make_column_combo(const QStringList& labels, QWidget* parent)
{
    auto* combo = new QComboBox(parent);
    for (int i = 0; i < labels.size(); ++i) {
        combo->addItem(labels[i], i);
    }
    return combo;
}

}  // namespace

LifeDataLognormalDialog::LifeDataLognormalDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("寿命数据 Lognormal"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* p1_body = new QWidget(stack_);
    auto* p1_layout = new QHBoxLayout(p1_body);
    auto* p1_left = new QWidget(p1_body);
    auto* p1_form = new QFormLayout(p1_left);
    time_combo_ = make_column_combo(column_labels, p1_left);
    event_combo_ = make_column_combo(column_labels, p1_left);
    p1_form->addRow(QStringLiteral("时间列"), time_combo_);
    p1_form->addRow(QStringLiteral("删失列 (1=失败)"), event_combo_);
    covariate_list_ = new QListWidget(p1_body);
    covariate_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int i = 0; i < column_labels.size(); ++i) {
        auto* item = new QListWidgetItem(column_labels[i], covariate_list_);
        item->setData(Qt::UserRole, i);
    }
    p1_layout->addWidget(p1_left, 1);
    p1_layout->addWidget(covariate_list_, 1);
    stack_->addWidget(make_titled_page(QStringLiteral("时间、删失与协变量"), p1_body, stack_));

    options_note_ = new QPlainTextEdit(stack_);
    options_note_->setReadOnly(true);
    options_note_->setPlainText(QStringLiteral(
        "Lognormal 分布：log(Y) ~ Normal(μ, σ²)。\n"
        "μ = β₀ + Σβ_k x_k；右删失 MLE。\n"
        "百分位：1, 5, 50, 95, 99。"));
    stack_->addWidget(make_titled_page(QStringLiteral("Lognormal 选项"), options_note_, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "Newton-Raphson MLE on log scale。\n"
        "失败：完整密度；删失：生存函数。\n"
        "time>0 complete-case；保留 source_row。"));
    stack_->addWidget(make_titled_page(QStringLiteral("MLE 方法"), method_note_, stack_));

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

    connect(back_button_, &QPushButton::clicked, this, &LifeDataLognormalDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &LifeDataLognormalDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &LifeDataLognormalDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::LifeDataLognormalConfiguration LifeDataLognormalDialog::configuration() const
{
    datalab::domain::LifeDataLognormalConfiguration cfg;
    cfg.time_column = static_cast<std::size_t>(time_combo_->currentData().toInt());
    cfg.event_column = static_cast<std::size_t>(event_combo_->currentData().toInt());
    for (int i = 0; i < covariate_list_->count(); ++i) {
        if (covariate_list_->item(i)->isSelected()) {
            cfg.covariate_columns.push_back(
                static_cast<std::size_t>(covariate_list_->item(i)->data(Qt::UserRole).toInt()));
        }
    }
    if (cfg.covariate_columns.size() > 2) {
        cfg.covariate_columns.resize(2);
    }
    return cfg;
}

bool LifeDataLognormalDialog::validate_columns(QString* error) const
{
    if (time_combo_->currentData().toInt() < 0 || event_combo_->currentData().toInt() < 0) {
        if (error != nullptr) {
            *error = QStringLiteral("请选择时间与删失列。");
        }
        return false;
    }
  if (configuration().covariate_columns.size() > 2) {
        if (error != nullptr) {
            *error = QStringLiteral("最多 2 个协变量。");
        }
        return false;
    }
    return true;
}

void LifeDataLognormalDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("时间=%1 删失=%2 协变量=%3")
            .arg(cfg.time_column.value_or(0))
            .arg(cfg.event_column.value_or(0))
            .arg(cfg.covariate_columns.size()));
}

void LifeDataLognormalDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void LifeDataLognormalDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void LifeDataLognormalDialog::on_next()
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

void LifeDataLognormalDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}

#include "life_data_lognormal_dialog.moc"
