#include "ui/factor_analysis_dialog.h"

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

FactorAnalysisDialog::FactorAnalysisDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("因子分析"));
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
    stack_->addWidget(make_titled_page(QStringLiteral("变量选择"), variable_list_, stack_));

    auto* extract_body = new QWidget(stack_);
    auto* extract_layout = new QVBoxLayout(extract_body);
    kaiser_check_ = new QCheckBox(QStringLiteral("Kaiser 准则 (λ>1)"), extract_body);
    kaiser_check_->setChecked(true);
    extract_layout->addWidget(kaiser_check_);
    extract_layout->addWidget(new QLabel(QStringLiteral("或指定因子数（0=自动）"), extract_body));
    factor_count_spin_ = new QSpinBox(extract_body);
    factor_count_spin_->setRange(0, 20);
    factor_count_spin_->setValue(0);
    extract_layout->addWidget(factor_count_spin_);
    varimax_check_ = new QCheckBox(QStringLiteral("Varimax 正交旋转"), extract_body);
    extract_layout->addWidget(varimax_check_);
    extract_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("提取/旋转"), extract_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "探索性因子分析（主成分提取，formula_reference）：\n\n"
        "相关阵 R 特征分解；载荷 L = √λ · 特征向量\n"
        "% Var = λ_i / Σλ\n"
        "可选 Varimax 旋转。\n\n"
        "与 pca 命令区分：本命令无 Hotelling T²。"));
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

    connect(back_button_, &QPushButton::clicked, this, &FactorAnalysisDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &FactorAnalysisDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &FactorAnalysisDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::FactorAnalysisConfiguration FactorAnalysisDialog::configuration() const
{
    datalab::domain::FactorAnalysisConfiguration cfg;
    for (QListWidgetItem* item : variable_list_->selectedItems()) {
        cfg.variable_columns.push_back(
            static_cast<std::size_t>(item->data(Qt::UserRole).toInt()));
    }
    cfg.factor_count = static_cast<std::size_t>(factor_count_spin_->value());
    cfg.use_kaiser_rule = kaiser_check_->isChecked();
    cfg.varimax_rotation = varimax_check_->isChecked();
    return cfg;
}

bool FactorAnalysisDialog::validate_columns(QString* error) const
{
    if (variable_list_->selectedItems().size() < 3) {
        if (error != nullptr) {
            *error = QStringLiteral("请至少选择三个数值变量。");
        }
        return false;
    }
    return true;
}

void FactorAnalysisDialog::rebuild_preview()
{
    const auto cfg = configuration();
    QString text;
    text += QStringLiteral("变量数：%1\n").arg(cfg.variable_columns.size());
    text += QStringLiteral("因子数：%1（0=Kaiser）\n")
                .arg(cfg.factor_count);
    text += QStringLiteral("Kaiser：%1    Varimax：%2\n\n")
                .arg(cfg.use_kaiser_rule ? QStringLiteral("是")
                                         : QStringLiteral("否"))
                .arg(cfg.varimax_rotation ? QStringLiteral("是")
                                          : QStringLiteral("否"));
    text += QStringLiteral("将输出 Loadings、% Var、Communalities、Scree 图。");
    preview_->setPlainText(text);
}

void FactorAnalysisDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void FactorAnalysisDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void FactorAnalysisDialog::on_next()
{
    if (stack_->currentIndex() == 0) {
        QString error;
        if (!validate_columns(&error)) {
            QMessageBox::warning(this, QStringLiteral("变量不足"), error);
            return;
        }
    }
    if (stack_->currentIndex() + 1 < stack_->count()) {
        stack_->setCurrentIndex(stack_->currentIndex() + 1);
        update_nav();
    }
}

void FactorAnalysisDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("变量不足"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
