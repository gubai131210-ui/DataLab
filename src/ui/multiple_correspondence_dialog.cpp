#include "ui/multiple_correspondence_dialog.h"

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

MultipleCorrespondenceDialog::MultipleCorrespondenceDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("多重对应分析"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    variable_list_ = new QListWidget(stack_);
    variable_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int i = 0; i < column_labels.size(); ++i) {
        auto* item = new QListWidgetItem(column_labels[i], variable_list_);
        item->setData(Qt::UserRole, i);
    }
    stack_->addWidget(make_titled_page(QStringLiteral("分类变量（3～6 列）"), variable_list_, stack_));

    auto* p2_body = new QWidget(stack_);
    auto* p2_form = new QFormLayout(p2_body);
    component_spin_ = new QSpinBox(p2_body);
    component_spin_->setRange(1, 6);
    component_spin_->setValue(2);
    p2_form->addRow(QStringLiteral("组件数"), component_spin_);
    stack_->addWidget(make_titled_page(QStringLiteral("组件数"), p2_body, stack_));

    auto* p3_body = new QWidget(stack_);
    auto* p3_form = new QFormLayout(p3_body);
    col_contrib_check_ = new QCheckBox(QStringLiteral("列贡献表"), p3_body);
    col_contrib_check_->setChecked(true);
    p3_form->addRow(col_contrib_check_);
    stack_->addWidget(make_titled_page(QStringLiteral("输出表/图选项"), p3_body, stack_));

    output_note_ = new QPlainTextEdit(stack_);
    output_note_->setReadOnly(true);
    output_note_->setPlainText(QStringLiteral(
        "指示矩阵 MCA；Column Contributions。\n"
        "与简单对应分析（2 列）独立。"));
    stack_->addWidget(make_titled_page(QStringLiteral("方法说明"), output_note_, stack_));

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

    connect(back_button_, &QPushButton::clicked, this, &MultipleCorrespondenceDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &MultipleCorrespondenceDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &MultipleCorrespondenceDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::MultipleCorrespondenceConfiguration MultipleCorrespondenceDialog::configuration() const
{
    datalab::domain::MultipleCorrespondenceConfiguration cfg;
    for (auto* item : variable_list_->selectedItems()) {
        cfg.categorical_columns.push_back(static_cast<std::size_t>(item->data(Qt::UserRole).toInt()));
    }
    cfg.component_count = static_cast<std::size_t>(component_spin_->value());
    cfg.include_column_contributions = col_contrib_check_->isChecked();
    return cfg;
}

bool MultipleCorrespondenceDialog::validate_columns(QString* error) const
{
    const int count = variable_list_->selectedItems().size();
    if (count < 3 || count > 6) {
        if (error != nullptr) {
            *error = QStringLiteral("请选择 3～6 个分类变量列。");
        }
        return false;
    }
    return true;
}

void MultipleCorrespondenceDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("变量数=%1 组件=%2")
            .arg(cfg.categorical_columns.size())
            .arg(cfg.component_count));
}

void MultipleCorrespondenceDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void MultipleCorrespondenceDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void MultipleCorrespondenceDialog::on_next()
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

void MultipleCorrespondenceDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}

#include "multiple_correspondence_dialog.moc"
