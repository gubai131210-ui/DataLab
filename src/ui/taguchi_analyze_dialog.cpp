#include "ui/taguchi_analyze_dialog.h"

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

TaguchiAnalyzeDialog::TaguchiAnalyzeDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Taguchi 分析"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* columns_body = new QWidget(stack_);
    auto* columns_layout = new QHBoxLayout(columns_body);
    factor_list_ = new QListWidget(columns_body);
    factor_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    response_list_ = new QListWidget(columns_body);
    response_list_->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int i = 0; i < column_labels.size(); ++i) {
        auto* f_item = new QListWidgetItem(column_labels[i], factor_list_);
        f_item->setData(Qt::UserRole, i);
        auto* r_item = new QListWidgetItem(column_labels[i], response_list_);
        r_item->setData(Qt::UserRole, i);
    }
    auto* factor_wrap = new QVBoxLayout();
    factor_wrap->addWidget(new QLabel(QStringLiteral("因子列"), columns_body));
    factor_wrap->addWidget(factor_list_, 1);
    auto* response_wrap = new QVBoxLayout();
    response_wrap->addWidget(new QLabel(QStringLiteral("响应列（外阵重复）"), columns_body));
    response_wrap->addWidget(response_list_, 1);
    columns_layout->addLayout(factor_wrap, 1);
    columns_layout->addLayout(response_wrap, 1);
    stack_->addWidget(make_titled_page(QStringLiteral("列选择"), columns_body, stack_));

    auto* methods_body = new QWidget(stack_);
    auto* methods_layout = new QVBoxLayout(methods_body);
    methods_layout->addWidget(new QLabel(QStringLiteral("信噪比 (S/N) 类型"), methods_body));
    sn_combo_ = new QComboBox(methods_body);
    sn_combo_->addItem(QStringLiteral("越大越好 (larger)"), QStringLiteral("larger"));
    sn_combo_->addItem(QStringLiteral("越小越好 (smaller)"), QStringLiteral("smaller"));
    sn_combo_->addItem(QStringLiteral("望目 II (nominal)"), QStringLiteral("nominal"));
    methods_layout->addWidget(sn_combo_);
    methods_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("方法"), methods_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "S/N 公式（静态 Taguchi，formula_reference）：\n\n"
        "越大越好：S/N = -10 log10( (1/n) Σ 1/Y_i² )\n"
        "越小越好：S/N = -10 log10( (1/n) Σ Y_i² )\n"
        "望目 II：S/N = -10 log10(s²)，n≥2\n\n"
        "响应表：各因子各水平平均 Mean/S/N；Delta = max−min；Rank 按 Delta 降序。\n"
        "本对话框仅做静态分析，不宣称过程已优化。"));
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

    connect(back_button_, &QPushButton::clicked, this, &TaguchiAnalyzeDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &TaguchiAnalyzeDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &TaguchiAnalyzeDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::TaguchiAnalyzeConfiguration TaguchiAnalyzeDialog::configuration() const
{
    datalab::domain::TaguchiAnalyzeConfiguration cfg;
    for (QListWidgetItem* item : factor_list_->selectedItems()) {
        cfg.factor_columns.push_back(
            static_cast<std::size_t>(item->data(Qt::UserRole).toInt()));
    }
    for (QListWidgetItem* item : response_list_->selectedItems()) {
        cfg.response_columns.push_back(
            static_cast<std::size_t>(item->data(Qt::UserRole).toInt()));
    }
    cfg.sn_type = sn_combo_->currentData().toString().toStdString();
    return cfg;
}

bool TaguchiAnalyzeDialog::validate_columns(QString* error) const
{
    if (factor_list_->selectedItems().isEmpty()
        || response_list_->selectedItems().isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("请至少选择一个因子列与一个响应列。");
        }
        return false;
    }
    return true;
}

void TaguchiAnalyzeDialog::rebuild_preview()
{
    const auto cfg = configuration();
    QString text;
    text += QStringLiteral("因子列数：%1\n").arg(cfg.factor_columns.size());
    text += QStringLiteral("响应列数：%1\n").arg(cfg.response_columns.size());
    text += QStringLiteral("S/N 类型：%1\n\n")
                .arg(QString::fromStdString(cfg.sn_type));
    text += QStringLiteral("将输出 Means/S/N 响应表、Delta/Rank 与主效应图。\n");
    text += QStringLiteral("解释层禁止「过程已优化 / 已合格」。");
    preview_->setPlainText(text);
}

void TaguchiAnalyzeDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void TaguchiAnalyzeDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void TaguchiAnalyzeDialog::on_next()
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

void TaguchiAnalyzeDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列不足"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
