#include "ui/mixture_analyze_dialog.h"

#include <QAbstractItemView>
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

MixtureAnalyzeDialog::MixtureAnalyzeDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Mixture 分析"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* columns_body = new QWidget(stack_);
    auto* columns_layout = new QHBoxLayout(columns_body);
    component_list_ = new QListWidget(columns_body);
    component_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    response_list_ = new QListWidget(columns_body);
    response_list_->setSelectionMode(QAbstractItemView::SingleSelection);
    for (int i = 0; i < column_labels.size(); ++i) {
        auto* c_item = new QListWidgetItem(column_labels[i], component_list_);
        c_item->setData(Qt::UserRole, i);
        auto* r_item = new QListWidgetItem(column_labels[i], response_list_);
        r_item->setData(Qt::UserRole, i);
    }
    auto* comp_wrap = new QVBoxLayout();
    comp_wrap->addWidget(new QLabel(QStringLiteral("分量列 (x1..xq)"), columns_body));
    comp_wrap->addWidget(component_list_, 1);
    auto* resp_wrap = new QVBoxLayout();
    resp_wrap->addWidget(new QLabel(QStringLiteral("响应列"), columns_body));
    resp_wrap->addWidget(response_list_, 1);
    columns_layout->addLayout(comp_wrap, 1);
    columns_layout->addLayout(resp_wrap, 1);
    stack_->addWidget(make_titled_page(QStringLiteral("列选择"), columns_body, stack_));

    auto* model_body = new QWidget(stack_);
    auto* model_layout = new QVBoxLayout(model_body);
    model_layout->addWidget(new QLabel(QStringLiteral("模型阶"), model_body));
    model_combo_ = new QComboBox(model_body);
    model_combo_->addItem(QStringLiteral("线性 Scheffé"), QStringLiteral("linear"));
    model_combo_->addItem(QStringLiteral("二次 (+ x_i x_j)"), QStringLiteral("quadratic"));
    model_layout->addWidget(model_combo_);
    model_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("模型阶"), model_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "Scheffé 混料模型（无常数项，formula_reference）：\n\n"
        "线性：Y = Σ b_i x_i\n"
        "二次：Y = Σ b_i x_i + Σ_{i<j} b_{ij} x_i x_j\n"
        "OLS：b̂ = (X'X)⁻¹ X'y\n\n"
        "分量之和应≈1；偏离 1 报诊断，不自动归一化。\n"
        "独立于 mixture_design 设计生成命令。"));
    stack_->addWidget(make_titled_page(QStringLiteral("方法说明"), method_note_, stack_));

    preview_ = new QPlainTextEdit(stack_);
    preview_->setReadOnly(true);
    stack_->addWidget(make_titled_page(QStringLiteral("预览确认"), preview_, stack_));

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

    connect(back_button_, &QPushButton::clicked, this, &MixtureAnalyzeDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &MixtureAnalyzeDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &MixtureAnalyzeDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::MixtureAnalyzeConfiguration MixtureAnalyzeDialog::configuration() const
{
    datalab::domain::MixtureAnalyzeConfiguration cfg;
    for (QListWidgetItem* item : component_list_->selectedItems()) {
        cfg.component_columns.push_back(
            static_cast<std::size_t>(item->data(Qt::UserRole).toInt()));
    }
    if (QListWidgetItem* item = response_list_->currentItem()) {
        cfg.response_column = static_cast<std::size_t>(item->data(Qt::UserRole).toInt());
    }
    cfg.model_order = model_combo_->currentData().toString().toStdString();
    return cfg;
}

bool MixtureAnalyzeDialog::validate_columns(QString* error) const
{
    if (component_list_->selectedItems().size() < 2
        || response_list_->currentItem() == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("请至少选择两个分量列与一个响应列。");
        }
        return false;
    }
    return true;
}

void MixtureAnalyzeDialog::rebuild_preview()
{
    const auto cfg = configuration();
    QString text;
    text += QStringLiteral("分量列数：%1\n").arg(cfg.component_columns.size());
    text += QStringLiteral("模型阶：%1\n\n")
                .arg(QString::fromStdString(cfg.model_order));
    text += QStringLiteral("将输出 Coefficients、ANOVA、Fits/Residuals。\n");
    text += QStringLiteral("与 mixture_design 设计命令独立。");
    preview_->setPlainText(text);
}

void MixtureAnalyzeDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void MixtureAnalyzeDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void MixtureAnalyzeDialog::on_next()
{
    if (stack_->currentIndex() == 0) {
        QString error;
        if (!validate_columns(&error)) {
            QMessageBox::warning(this, QStringLiteral("列不足"), error);
            return;
        }
    }
    if (stack_->currentIndex() + 1 < stack_->count()) {
        stack_->setCurrentIndex(stack_->currentIndex() + 1);
        update_nav();
    }
}

void MixtureAnalyzeDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列不足"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
