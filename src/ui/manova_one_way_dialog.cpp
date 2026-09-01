#include "ui/manova_one_way_dialog.h"

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

ManovaOneWayDialog::ManovaOneWayDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("单因子 MANOVA"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* columns_body = new QWidget(stack_);
    auto* columns_layout = new QHBoxLayout(columns_body);
    response_list_ = new QListWidget(columns_body);
    response_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int i = 0; i < column_labels.size(); ++i) {
        auto* item = new QListWidgetItem(column_labels[i], response_list_);
        item->setData(Qt::UserRole, i);
    }
    auto* right = new QWidget(columns_body);
    auto* right_form = new QFormLayout(right);
    factor_combo_ = new QComboBox(right);
    for (int i = 0; i < column_labels.size(); ++i) {
        factor_combo_->addItem(column_labels[i], i);
    }
    right_form->addRow(QStringLiteral("因子列"), factor_combo_);
    columns_layout->addWidget(response_list_, 1);
    columns_layout->addWidget(right, 1);
    stack_->addWidget(make_titled_page(QStringLiteral("多响应与因子"), columns_body, stack_));

    auto* tests_body = new QWidget(stack_);
    auto* tests_layout = new QVBoxLayout(tests_body);
    wilks_check_ = new QCheckBox(QStringLiteral("Wilks' Lambda"), tests_body);
    pillai_check_ = new QCheckBox(QStringLiteral("Pillai's Trace"), tests_body);
    lh_check_ = new QCheckBox(QStringLiteral("Lawley-Hotelling"), tests_body);
    roy_check_ = new QCheckBox(QStringLiteral("Roy's Largest Root"), tests_body);
    wilks_check_->setChecked(true);
    pillai_check_->setChecked(true);
    lh_check_->setChecked(true);
    roy_check_->setChecked(true);
    tests_layout->addWidget(wilks_check_);
    tests_layout->addWidget(pillai_check_);
    tests_layout->addWidget(lh_check_);
    tests_layout->addWidget(roy_check_);
    tests_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("检验选项"), tests_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "单因子 MANOVA（formula_reference）：\n\n"
        "H/E SSCP；λ_i 为 E^{-1}H 特征值。\n"
        "Wilks Λ = Π 1/(1+λ_i)；Pillai/LH/Roy F 近似。\n"
        "2～4 响应；complete-case；保留 source_row。"));
    stack_->addWidget(make_titled_page(QStringLiteral("SSCP 方法"), method_note_, stack_));

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

    connect(back_button_, &QPushButton::clicked, this, &ManovaOneWayDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &ManovaOneWayDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &ManovaOneWayDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::ManovaOneWayConfiguration ManovaOneWayDialog::configuration() const
{
    datalab::domain::ManovaOneWayConfiguration cfg;
    for (int i = 0; i < response_list_->count(); ++i) {
        if (response_list_->item(i)->isSelected()) {
            cfg.response_columns.push_back(
                static_cast<std::size_t>(response_list_->item(i)->data(Qt::UserRole).toInt()));
        }
    }
    cfg.factor_column = static_cast<std::size_t>(factor_combo_->currentData().toInt());
    cfg.wilks = wilks_check_->isChecked();
    cfg.pillai = pillai_check_->isChecked();
    cfg.lawley_hotelling = lh_check_->isChecked();
    cfg.roy = roy_check_->isChecked();
    return cfg;
}

bool ManovaOneWayDialog::validate_columns(QString* error) const
{
    const auto cfg = configuration();
    if (cfg.response_columns.size() < 2 || cfg.response_columns.size() > 4) {
        if (error != nullptr) {
            *error = QStringLiteral("请选择 2～4 个响应列。");
        }
        return false;
    }
    if (std::find(cfg.response_columns.cbegin(), cfg.response_columns.cend(),
                  cfg.factor_column)
        != cfg.response_columns.cend()) {
        if (error != nullptr) {
            *error = QStringLiteral("因子列不能与响应列相同。");
        }
        return false;
    }
    if (!cfg.wilks && !cfg.pillai && !cfg.lawley_hotelling && !cfg.roy) {
        if (error != nullptr) {
            *error = QStringLiteral("至少选择一种 MANOVA 检验。");
        }
        return false;
    }
    return true;
}

void ManovaOneWayDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("响应数=%1 因子=%2\nWilks=%3 Pillai=%4 LH=%5 Roy=%6\n\n"
                       "将输出 MANOVA 检验表与组均值向量。")
            .arg(cfg.response_columns.size())
            .arg(cfg.factor_column.value_or(0))
            .arg(cfg.wilks ? 1 : 0)
            .arg(cfg.pillai ? 1 : 0)
            .arg(cfg.lawley_hotelling ? 1 : 0)
            .arg(cfg.roy ? 1 : 0));
}

void ManovaOneWayDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void ManovaOneWayDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void ManovaOneWayDialog::on_next()
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

void ManovaOneWayDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
