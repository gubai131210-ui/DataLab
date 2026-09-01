#include "ui/binary_doe_probit_dialog.h"

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

BinaryDoeProbitDialog::BinaryDoeProbitDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("二值 DOE Probit/Gompit"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* p1_body = new QWidget(stack_);
    auto* p1_layout = new QHBoxLayout(p1_body);
    factor_list_ = new QListWidget(p1_body);
    factor_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int i = 0; i < column_labels.size(); ++i) {
        auto* item = new QListWidgetItem(column_labels[i], factor_list_);
        item->setData(Qt::UserRole, i);
    }
    auto* p1_right = new QWidget(p1_body);
    auto* p1_form = new QFormLayout(p1_right);
    events_combo_ = make_column_combo(column_labels, p1_right, true);
    trials_combo_ = make_column_combo(column_labels, p1_right, true);
    binary_combo_ = make_column_combo(column_labels, p1_right, true);
    p1_form->addRow(QStringLiteral("Events"), events_combo_);
    p1_form->addRow(QStringLiteral("Trials"), trials_combo_);
    p1_form->addRow(QStringLiteral("0/1 响应"), binary_combo_);
    interaction_check_ = new QCheckBox(QStringLiteral("A×B 交互"), p1_right);
    interaction_check_->setChecked(true);
    p1_form->addRow(interaction_check_);
    p1_layout->addWidget(factor_list_, 1);
    p1_layout->addWidget(p1_right, 1);
    stack_->addWidget(make_titled_page(QStringLiteral("因子与响应"), p1_body, stack_));

    auto* p2_body = new QWidget(stack_);
    auto* p2_form = new QFormLayout(p2_body);
    link_combo_ = new QComboBox(p2_body);
    link_combo_->addItem(QStringLiteral("Probit (normit)"), QStringLiteral("probit"));
    link_combo_->addItem(QStringLiteral("Gompit (cloglog)"), QStringLiteral("gompit"));
    p2_form->addRow(QStringLiteral("Link 函数"), link_combo_);
    stack_->addWidget(make_titled_page(QStringLiteral("Link 选择"), p2_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "Probit: Φ⁻¹(μ) = x'β\n"
        "Gompit: log(-log(1-μ)) = x'β\n"
        "IRWLS 迭代；complete-case；保留 source_row。"));
    stack_->addWidget(make_titled_page(QStringLiteral("IRWLS 方法"), method_note_, stack_));

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

    connect(back_button_, &QPushButton::clicked, this, &BinaryDoeProbitDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &BinaryDoeProbitDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &BinaryDoeProbitDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::BinaryDoeProbitConfiguration BinaryDoeProbitDialog::configuration() const
{
    datalab::domain::BinaryDoeProbitConfiguration cfg;
    for (int i = 0; i < factor_list_->count(); ++i) {
        if (factor_list_->item(i)->isSelected()) {
            cfg.factor_columns.push_back(
                static_cast<std::size_t>(factor_list_->item(i)->data(Qt::UserRole).toInt()));
        }
    }
    if (events_combo_->currentData().toInt() >= 0) {
        cfg.events_column = static_cast<std::size_t>(events_combo_->currentData().toInt());
    }
    if (trials_combo_->currentData().toInt() >= 0) {
        cfg.trials_column = static_cast<std::size_t>(trials_combo_->currentData().toInt());
    }
    if (binary_combo_->currentData().toInt() >= 0) {
        cfg.binary_response_column =
            static_cast<std::size_t>(binary_combo_->currentData().toInt());
    }
    cfg.link = link_combo_->currentData().toString().toStdString();
    cfg.include_ab_interaction = interaction_check_->isChecked();
    return cfg;
}

bool BinaryDoeProbitDialog::validate_columns(QString* error) const
{
    const auto cfg = configuration();
    if (cfg.factor_columns.empty()) {
        if (error != nullptr) {
            *error = QStringLiteral("请至少选择一个因子列。");
        }
        return false;
    }
    const bool et = cfg.events_column.has_value() && cfg.trials_column.has_value();
    const bool binary = cfg.binary_response_column.has_value();
    if (!et && !binary) {
        if (error != nullptr) {
            *error = QStringLiteral("需要 Events/Trials 或 0/1 响应列。");
        }
        return false;
    }
    return true;
}

void BinaryDoeProbitDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("因子=%1 link=%2 交互=%3")
            .arg(cfg.factor_columns.size())
            .arg(QString::fromStdString(cfg.link))
            .arg(cfg.include_ab_interaction ? 1 : 0));
}

void BinaryDoeProbitDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void BinaryDoeProbitDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void BinaryDoeProbitDialog::on_next()
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

void BinaryDoeProbitDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}

#include "binary_doe_probit_dialog.moc"
