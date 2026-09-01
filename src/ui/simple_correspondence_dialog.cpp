#include "ui/simple_correspondence_dialog.h"

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

SimpleCorrespondenceDialog::SimpleCorrespondenceDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("简单对应分析"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* p1_body = new QWidget(stack_);
    auto* p1_form = new QFormLayout(p1_body);
    row_combo_ = make_column_combo(column_labels, p1_body);
    col_combo_ = make_column_combo(column_labels, p1_body);
    p1_form->addRow(QStringLiteral("行变量"), row_combo_);
    p1_form->addRow(QStringLiteral("列变量"), col_combo_);
    stack_->addWidget(make_titled_page(QStringLiteral("行列变量"), p1_body, stack_));

    auto* p2_body = new QWidget(stack_);
    auto* p2_form = new QFormLayout(p2_body);
    component_spin_ = new QSpinBox(p2_body);
    component_spin_->setRange(1, 2);
    component_spin_->setValue(2);
    p2_form->addRow(QStringLiteral("组件数"), component_spin_);
    stack_->addWidget(make_titled_page(QStringLiteral("组件与图选项"), p2_body, stack_));

    auto* p3_body = new QWidget(stack_);
    auto* p3_form = new QFormLayout(p3_body);
    row_contrib_check_ = new QCheckBox(QStringLiteral("行贡献表"), p3_body);
    col_contrib_check_ = new QCheckBox(QStringLiteral("列贡献表"), p3_body);
    row_contrib_check_->setChecked(true);
    col_contrib_check_->setChecked(true);
    p3_form->addRow(row_contrib_check_);
    p3_form->addRow(col_contrib_check_);
    stack_->addWidget(make_titled_page(QStringLiteral("输出表选项"), p3_body, stack_));

    output_note_ = new QPlainTextEdit(stack_);
    output_note_->setReadOnly(true);
    output_note_->setPlainText(QStringLiteral(
        "惯性 I = χ²/n；SVD 主坐标。\n"
        "输出：Summary、Row/Column Contributions。"));
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

    connect(back_button_, &QPushButton::clicked, this, &SimpleCorrespondenceDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &SimpleCorrespondenceDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &SimpleCorrespondenceDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::SimpleCorrespondenceConfiguration SimpleCorrespondenceDialog::configuration() const
{
    datalab::domain::SimpleCorrespondenceConfiguration cfg;
    cfg.row_variable_column = static_cast<std::size_t>(row_combo_->currentData().toInt());
    cfg.column_variable_column = static_cast<std::size_t>(col_combo_->currentData().toInt());
    cfg.component_count = static_cast<std::size_t>(component_spin_->value());
    cfg.include_row_contributions = row_contrib_check_->isChecked();
    cfg.include_column_contributions = col_contrib_check_->isChecked();
    return cfg;
}

bool SimpleCorrespondenceDialog::validate_columns(QString* error) const
{
    if (row_combo_->currentData().toInt() < 0 || col_combo_->currentData().toInt() < 0) {
        if (error != nullptr) {
            *error = QStringLiteral("请选择行变量与列变量。");
        }
        return false;
    }
    if (row_combo_->currentData() == col_combo_->currentData()) {
        if (error != nullptr) {
            *error = QStringLiteral("行变量与列变量不能相同。");
        }
        return false;
    }
    return true;
}

void SimpleCorrespondenceDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("行=%1 列=%2 组件=%3")
            .arg(cfg.row_variable_column.value_or(0))
            .arg(cfg.column_variable_column.value_or(0))
            .arg(cfg.component_count));
}

void SimpleCorrespondenceDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void SimpleCorrespondenceDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void SimpleCorrespondenceDialog::on_next()
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

void SimpleCorrespondenceDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}

#include "simple_correspondence_dialog.moc"
