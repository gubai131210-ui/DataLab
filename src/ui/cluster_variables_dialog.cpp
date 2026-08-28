#include "ui/cluster_variables_dialog.h"

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

ClusterVariablesDialog::ClusterVariablesDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("变量聚类"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* select_body = new QWidget(stack_);
    auto* select_layout = new QVBoxLayout(select_body);
    variable_list_ = new QListWidget(select_body);
    variable_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int i = 0; i < column_labels.size(); ++i) {
        auto* item = new QListWidgetItem(column_labels[i], variable_list_);
        item->setData(Qt::UserRole, i);
    }
    select_layout->addWidget(variable_list_);
    stack_->addWidget(make_titled_page(QStringLiteral("变量选择"), select_body, stack_));

    auto* distance_body = new QWidget(stack_);
    auto* distance_form = new QFormLayout(distance_body);
    linkage_combo_ = new QComboBox(distance_body);
    linkage_combo_->addItem(QStringLiteral("Complete"), QStringLiteral("complete"));
    linkage_combo_->addItem(QStringLiteral("Average"), QStringLiteral("average"));
    linkage_combo_->addItem(QStringLiteral("Single"), QStringLiteral("single"));
    absolute_corr_check_ = new QCheckBox(QStringLiteral("距离 d = 1 − |ρ|"), distance_body);
    absolute_corr_check_->setChecked(true);
    distance_form->addRow(QStringLiteral("连结方法"), linkage_combo_);
    distance_form->addRow(absolute_corr_check_);
    stack_->addWidget(make_titled_page(QStringLiteral("距离与连结"), distance_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "变量聚类（formula_reference）：\n\n"
        "Pearson ρ_ij；d_ij = 1 − |ρ_ij|\n"
        "相似度 s_ij = 100(1 − d_ij/d_max)\n"
        "合并步数 = p − 1（p 为变量数）。\n\n"
        "dendrogram 在输出区；非 cluster_observations。"));
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

    connect(back_button_, &QPushButton::clicked, this, &ClusterVariablesDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &ClusterVariablesDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &ClusterVariablesDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::ClusterVariablesConfiguration ClusterVariablesDialog::configuration() const
{
    datalab::domain::ClusterVariablesConfiguration cfg;
    for (QListWidgetItem* item : variable_list_->selectedItems()) {
        cfg.variable_columns.push_back(static_cast<std::size_t>(item->data(Qt::UserRole).toInt()));
    }
    cfg.linkage = linkage_combo_->currentData().toString().toStdString();
    cfg.use_absolute_correlation = absolute_corr_check_->isChecked();
    return cfg;
}

bool ClusterVariablesDialog::validate_input(QString* error) const
{
    if (variable_list_->selectedItems().size() < 3) {
        if (error != nullptr) {
            *error = QStringLiteral("请至少选择三个数值变量。");
        }
        return false;
    }
    return true;
}

void ClusterVariablesDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("变量数：%1\n连结：%2\n绝对相关距离：%3\n\n"
                       "将输出 Amalgamation Steps 与 Dendrogram。")
            .arg(cfg.variable_columns.size())
            .arg(QString::fromStdString(cfg.linkage))
            .arg(cfg.use_absolute_correlation ? QStringLiteral("是")
                                            : QStringLiteral("否")));
}

void ClusterVariablesDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void ClusterVariablesDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void ClusterVariablesDialog::on_next()
{
    if (stack_->currentIndex() == 0) {
        QString error;
        if (!validate_input(&error)) {
            QMessageBox::warning(this, QStringLiteral("选择无效"), error);
            return;
        }
    }
    if (stack_->currentIndex() + 1 < stack_->count()) {
        stack_->setCurrentIndex(stack_->currentIndex() + 1);
        update_nav();
    }
}

void ClusterVariablesDialog::on_accept()
{
    QString error;
    if (!validate_input(&error)) {
        QMessageBox::warning(this, QStringLiteral("选择无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
