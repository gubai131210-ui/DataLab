#include "ui/split_plot_analyze_dialog.h"

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

SplitPlotAnalyzeDialog::SplitPlotAnalyzeDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("裂区析因分析"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* columns_body = new QWidget(stack_);
    auto* form = new QFormLayout(columns_body);
    response_combo_ = new QComboBox(columns_body);
    htc_combo_ = new QComboBox(columns_body);
    etc_a_combo_ = new QComboBox(columns_body);
    etc_b_combo_ = new QComboBox(columns_body);
    wp_combo_ = new QComboBox(columns_body);
    for (int i = 0; i < column_labels.size(); ++i) {
        response_combo_->addItem(column_labels[i], i);
        htc_combo_->addItem(column_labels[i], i);
        etc_a_combo_->addItem(column_labels[i], i);
        etc_b_combo_->addItem(column_labels[i], i);
        wp_combo_->addItem(column_labels[i], i);
    }
    etc_b_combo_->addItem(QStringLiteral("（无第二易改因子）"), -1);
    form->addRow(QStringLiteral("响应列"), response_combo_);
    form->addRow(QStringLiteral("难改因子"), htc_combo_);
    form->addRow(QStringLiteral("易改因子 A"), etc_a_combo_);
    form->addRow(QStringLiteral("易改因子 B（可选）"), etc_b_combo_);
    form->addRow(QStringLiteral("Whole Plot 指示"), wp_combo_);
    stack_->addWidget(make_titled_page(QStringLiteral("因子与 WP"), columns_body, stack_));

    auto* model_body = new QWidget(stack_);
    auto* model_layout = new QVBoxLayout(model_body);
    htc_etc_check_ = new QCheckBox(QStringLiteral("包含难改×易改交互"), model_body);
    etc_interaction_check_ = new QCheckBox(QStringLiteral("包含易改因子交互"), model_body);
    htc_etc_check_->setChecked(true);
    etc_interaction_check_->setChecked(true);
    model_layout->addWidget(htc_etc_check_);
    model_layout->addWidget(etc_interaction_check_);
    model_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("模型"), model_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "裂区双误差 ANOVA（formula_reference）：\n\n"
        "F_HTC = MS_term / MS_WP_Error\n"
        "F_ETC = MS_term / MS_SP_Error\n"
        "WP Residual = ŷ_full − ŷ_fixed-only\n\n"
        "complete-case；保留 source_row。"));
    stack_->addWidget(make_titled_page(QStringLiteral("双误差方法"), method_note_, stack_));

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

    connect(back_button_, &QPushButton::clicked, this, &SplitPlotAnalyzeDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &SplitPlotAnalyzeDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &SplitPlotAnalyzeDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::SplitPlotAnalyzeConfiguration SplitPlotAnalyzeDialog::configuration() const
{
    datalab::domain::SplitPlotAnalyzeConfiguration cfg;
    cfg.response_column = static_cast<std::size_t>(response_combo_->currentData().toInt());
    cfg.htc_factor_column = static_cast<std::size_t>(htc_combo_->currentData().toInt());
    cfg.etc_factor_a_column = static_cast<std::size_t>(etc_a_combo_->currentData().toInt());
    const int etc_b = etc_b_combo_->currentData().toInt();
    if (etc_b >= 0) {
        cfg.etc_factor_b_column = static_cast<std::size_t>(etc_b);
    }
    cfg.whole_plot_column = static_cast<std::size_t>(wp_combo_->currentData().toInt());
    cfg.include_htc_etc_interaction = htc_etc_check_->isChecked();
    cfg.include_etc_interaction = etc_interaction_check_->isChecked();
    return cfg;
}

bool SplitPlotAnalyzeDialog::validate_columns(QString* error) const
{
    const int y = response_combo_->currentData().toInt();
    const int htc = htc_combo_->currentData().toInt();
    const int etc_a = etc_a_combo_->currentData().toInt();
    const int etc_b = etc_b_combo_->currentData().toInt();
    const int wp = wp_combo_->currentData().toInt();
    if (y == htc || y == etc_a || y == wp || htc == etc_a || htc == wp || etc_a == wp) {
        if (error != nullptr) {
            *error = QStringLiteral("响应、难改/易改因子与 WP 列须互不相同。");
        }
        return false;
    }
    if (etc_b >= 0 && (y == etc_b || htc == etc_b || etc_a == etc_b || wp == etc_b)) {
        if (error != nullptr) {
            *error = QStringLiteral("第二易改因子列须与其他列不同。");
        }
        return false;
    }
    return true;
}

void SplitPlotAnalyzeDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("响应=%1 难改=%2 易改A=%3 WP=%4\n\n将输出 WP/SP 双误差 ANOVA。")
            .arg(cfg.response_column.value_or(0))
            .arg(cfg.htc_factor_column.value_or(0))
            .arg(cfg.etc_factor_a_column.value_or(0))
            .arg(cfg.whole_plot_column.value_or(0)));
}

void SplitPlotAnalyzeDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void SplitPlotAnalyzeDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void SplitPlotAnalyzeDialog::on_next()
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

void SplitPlotAnalyzeDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
