#include "ui/general_manova_dialog.h"

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

QComboBox* make_column_combo(const QStringList& labels, QWidget* parent, bool optional)
{
    auto* combo = new QComboBox(parent);
    if (optional) {
        combo->addItem(QStringLiteral("(无)"), -1);
    }
    for (int i = 0; i < labels.size(); ++i) {
        combo->addItem(labels[i], i);
    }
    return combo;
}

}  // namespace

GeneralManovaDialog::GeneralManovaDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("General MANOVA"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* p1_body = new QWidget(stack_);
    auto* p1_layout = new QHBoxLayout(p1_body);
    response_list_ = new QListWidget(p1_body);
    response_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int i = 0; i < column_labels.size(); ++i) {
        auto* item = new QListWidgetItem(column_labels[i], response_list_);
        item->setData(Qt::UserRole, i);
    }
    auto* p1_right = new QWidget(p1_body);
    auto* p1_form = new QFormLayout(p1_right);
    factor_a_combo_ = make_column_combo(column_labels, p1_right, false);
    factor_b_combo_ = make_column_combo(column_labels, p1_right, true);
    p1_form->addRow(QStringLiteral("因子 A"), factor_a_combo_);
    p1_form->addRow(QStringLiteral("因子 B（可选）"), factor_b_combo_);
    p1_layout->addWidget(response_list_, 1);
    p1_layout->addWidget(p1_right, 1);
    stack_->addWidget(make_titled_page(QStringLiteral("多响应与因子"), p1_body, stack_));

    auto* p2_body = new QWidget(stack_);
    auto* p2_form = new QFormLayout(p2_body);
    covariate_combo_ = make_column_combo(column_labels, p2_body, true);
    interaction_check_ = new QCheckBox(QStringLiteral("包含 A×B 交互"), p2_body);
    interaction_check_->setChecked(true);
    p2_form->addRow(QStringLiteral("协变量（可选）"), covariate_combo_);
    p2_form->addRow(interaction_check_);
    stack_->addWidget(make_titled_page(QStringLiteral("协变量与交互"), p2_body, stack_));

    auto* p3_body = new QWidget(stack_);
    auto* p3_layout = new QVBoxLayout(p3_body);
    wilks_check_ = new QCheckBox(QStringLiteral("Wilks' Lambda"), p3_body);
    pillai_check_ = new QCheckBox(QStringLiteral("Pillai's Trace"), p3_body);
    lh_check_ = new QCheckBox(QStringLiteral("Lawley-Hotelling"), p3_body);
    roy_check_ = new QCheckBox(QStringLiteral("Roy's Largest Root"), p3_body);
    wilks_check_->setChecked(true);
    pillai_check_->setChecked(true);
    lh_check_->setChecked(true);
    roy_check_->setChecked(true);
    p3_layout->addWidget(wilks_check_);
    p3_layout->addWidget(pillai_check_);
    p3_layout->addWidget(lh_check_);
    p3_layout->addWidget(roy_check_);
    p3_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("检验与 SSCP"), p3_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "General MANOVA Type III SSCP（formula_reference）：\n\n"
        "按效应分区 H；E 为残差 SSCP。\n"
        "λ_i 为 E^{-1}H 特征值；Wilks/Pillai/LH/Roy。"));
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

    connect(back_button_, &QPushButton::clicked, this, &GeneralManovaDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &GeneralManovaDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &GeneralManovaDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::GeneralManovaConfiguration GeneralManovaDialog::configuration() const
{
    datalab::domain::GeneralManovaConfiguration cfg;
    for (int i = 0; i < response_list_->count(); ++i) {
        if (response_list_->item(i)->isSelected()) {
            cfg.response_columns.push_back(
                static_cast<std::size_t>(response_list_->item(i)->data(Qt::UserRole).toInt()));
        }
    }
    cfg.factor_a_column = static_cast<std::size_t>(factor_a_combo_->currentData().toInt());
    if (factor_b_combo_->currentData().toInt() >= 0) {
        cfg.factor_b_column = static_cast<std::size_t>(factor_b_combo_->currentData().toInt());
    }
    if (covariate_combo_->currentData().toInt() >= 0) {
        cfg.covariate_column = static_cast<std::size_t>(covariate_combo_->currentData().toInt());
    }
    cfg.include_interaction = interaction_check_->isChecked();
    cfg.wilks = wilks_check_->isChecked();
    cfg.pillai = pillai_check_->isChecked();
    cfg.lawley_hotelling = lh_check_->isChecked();
    cfg.roy = roy_check_->isChecked();
    return cfg;
}

bool GeneralManovaDialog::validate_columns(QString* error) const
{
    const auto cfg = configuration();
    if (cfg.response_columns.size() < 2 || cfg.response_columns.size() > 4) {
        if (error != nullptr) {
            *error = QStringLiteral("请选择 2～4 个响应列。");
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

void GeneralManovaDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("响应=%1 因子A=%2 因子B=%3 协变量=%4 交互=%5")
            .arg(cfg.response_columns.size())
            .arg(cfg.factor_a_column.value_or(0))
            .arg(cfg.factor_b_column.has_value() ? QString::number(*cfg.factor_b_column) : "-")
            .arg(cfg.covariate_column.has_value() ? QString::number(*cfg.covariate_column) : "-")
            .arg(cfg.include_interaction ? 1 : 0));
}

void GeneralManovaDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void GeneralManovaDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void GeneralManovaDialog::on_next()
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

void GeneralManovaDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}

#include "general_manova_dialog.moc"
