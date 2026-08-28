#include "ui/analyze_variability_dialog.h"

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

AnalyzeVariabilityDialog::AnalyzeVariabilityDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Analyze Variability"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* layout_body = new QWidget(stack_);
    auto* layout_h = new QHBoxLayout(layout_body);
    factor_list_ = new QListWidget(layout_body);
    factor_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    replicate_list_ = new QListWidget(layout_body);
    replicate_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int i = 0; i < column_labels.size(); ++i) {
        auto* f_item = new QListWidgetItem(column_labels[i], factor_list_);
        f_item->setData(Qt::UserRole, i);
        auto* r_item = new QListWidgetItem(column_labels[i], replicate_list_);
        r_item->setData(Qt::UserRole, i);
    }
    auto* factor_wrap = new QVBoxLayout();
    factor_wrap->addWidget(new QLabel(QStringLiteral("因子列（2 水平）"), layout_body));
    factor_wrap->addWidget(factor_list_, 1);
    auto* rep_wrap = new QVBoxLayout();
    rep_wrap->addWidget(new QLabel(QStringLiteral("重复响应列")), layout_body);
    rep_wrap->addWidget(replicate_list_, 1);
    layout_h->addLayout(factor_wrap, 1);
    layout_h->addLayout(rep_wrap, 1);
    stack_->addWidget(make_titled_page(QStringLiteral("数据布局"), layout_body, stack_));

    auto* method_body = new QWidget(stack_);
    auto* method_layout = new QVBoxLayout(method_body);
    method_layout->addWidget(new QLabel(QStringLiteral("估计方法"), method_body));
    method_combo_ = new QComboBox(method_body);
    method_combo_->addItem(QStringLiteral("LSE（对数标准差）"), QStringLiteral("lse"));
    method_layout->addWidget(method_combo_);
    method_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("估计方法"), method_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "Analyze Variability（2 水平窄化，formula_reference）：\n\n"
        "每运行：s = 重复列样本标准差\n"
        "分散模型：ln(s) = Σ γ_j z_j（±1 编码）\n"
        "效应 = 2 × 系数\n\n"
        "重复 vs 再现：本波按列重复处理；缺重复报门禁失败。"));
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

    connect(back_button_, &QPushButton::clicked, this, &AnalyzeVariabilityDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &AnalyzeVariabilityDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &AnalyzeVariabilityDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::AnalyzeVariabilityConfiguration AnalyzeVariabilityDialog::configuration() const
{
    datalab::domain::AnalyzeVariabilityConfiguration cfg;
    for (QListWidgetItem* item : factor_list_->selectedItems()) {
        cfg.factor_columns.push_back(
            static_cast<std::size_t>(item->data(Qt::UserRole).toInt()));
    }
    for (QListWidgetItem* item : replicate_list_->selectedItems()) {
        cfg.replicate_columns.push_back(
            static_cast<std::size_t>(item->data(Qt::UserRole).toInt()));
    }
    cfg.estimation_method = method_combo_->currentData().toString().toStdString();
    return cfg;
}

bool AnalyzeVariabilityDialog::validate_columns(QString* error) const
{
    if (factor_list_->selectedItems().isEmpty()
        || replicate_list_->selectedItems().size() < 2) {
        if (error != nullptr) {
            *error = QStringLiteral("请至少选择一个因子列与两个重复响应列。");
        }
        return false;
    }
    return true;
}

void AnalyzeVariabilityDialog::rebuild_preview()
{
    const auto cfg = configuration();
    QString text;
    text += QStringLiteral("因子列数：%1\n").arg(cfg.factor_columns.size());
    text += QStringLiteral("重复列数：%1\n").arg(cfg.replicate_columns.size());
    text += QStringLiteral("方法：%1\n\n")
                .arg(QString::fromStdString(cfg.estimation_method));
    text += QStringLiteral("将输出 Std Dev by Run、分散效应与 ANOVA。");
    preview_->setPlainText(text);
}

void AnalyzeVariabilityDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void AnalyzeVariabilityDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void AnalyzeVariabilityDialog::on_next()
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

void AnalyzeVariabilityDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列不足"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
