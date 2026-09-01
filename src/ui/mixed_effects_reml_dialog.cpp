#include "ui/mixed_effects_reml_dialog.h"

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

MixedEffectsRemlDialog::MixedEffectsRemlDialog(
    const QStringList& column_labels, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("混合效应 REML"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* p1_body = new QWidget(stack_);
    auto* p1_form = new QFormLayout(p1_body);
    response_combo_ = make_column_combo(column_labels, p1_body, false);
    random_combo_ = make_column_combo(column_labels, p1_body, false);
    p1_form->addRow(QStringLiteral("响应列"), response_combo_);
    p1_form->addRow(QStringLiteral("随机因子"), random_combo_);
    stack_->addWidget(make_titled_page(QStringLiteral("响应与随机因子"), p1_body, stack_));

    auto* p2_body = new QWidget(stack_);
    auto* p2_form = new QFormLayout(p2_body);
    fixed_a_combo_ = make_column_combo(column_labels, p2_body, true);
    fixed_b_combo_ = make_column_combo(column_labels, p2_body, true);
    covariate_combo_ = make_column_combo(column_labels, p2_body, true);
    p2_form->addRow(QStringLiteral("固定因子 A"), fixed_a_combo_);
    p2_form->addRow(QStringLiteral("固定因子 B"), fixed_b_combo_);
    p2_form->addRow(QStringLiteral("协变量"), covariate_combo_);
    stack_->addWidget(make_titled_page(QStringLiteral("固定因子与协变量"), p2_body, stack_));

    auto* p3_body = new QWidget(stack_);
    auto* p3_form = new QFormLayout(p3_body);
    method_combo_ = new QComboBox(p3_body);
    method_combo_->addItem(QStringLiteral("Newton REML"), QStringLiteral("newton"));
    method_combo_->addItem(QStringLiteral("EM REML"), QStringLiteral("em"));
    p3_form->addRow(QStringLiteral("REML 方法"), method_combo_);
    stack_->addWidget(make_titled_page(QStringLiteral("REML 方法"), p3_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "y = Xβ + Zμ + ε；V = σ²I + σ_u²ZZ'。\n"
        "REML 最大化 restricted log-likelihood；固定效应 BLUE。"));
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

    connect(back_button_, &QPushButton::clicked, this, &MixedEffectsRemlDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &MixedEffectsRemlDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &MixedEffectsRemlDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::MixedEffectsRemlConfiguration MixedEffectsRemlDialog::configuration() const
{
    datalab::domain::MixedEffectsRemlConfiguration cfg;
    cfg.response_column = static_cast<std::size_t>(response_combo_->currentData().toInt());
    cfg.random_factor_column = static_cast<std::size_t>(random_combo_->currentData().toInt());
    if (fixed_a_combo_->currentData().toInt() >= 0) {
        cfg.fixed_factor_a_column = static_cast<std::size_t>(fixed_a_combo_->currentData().toInt());
    }
    if (fixed_b_combo_->currentData().toInt() >= 0) {
        cfg.fixed_factor_b_column = static_cast<std::size_t>(fixed_b_combo_->currentData().toInt());
    }
    if (covariate_combo_->currentData().toInt() >= 0) {
        cfg.covariate_column = static_cast<std::size_t>(covariate_combo_->currentData().toInt());
    }
    cfg.reml_method = method_combo_->currentData().toString().toStdString();
    return cfg;
}

bool MixedEffectsRemlDialog::validate_columns(QString* error) const
{
    if (response_combo_->currentData().toInt() < 0 || random_combo_->currentData().toInt() < 0) {
        if (error != nullptr) {
            *error = QStringLiteral("请选择响应列与随机因子列。");
        }
        return false;
    }
    return true;
}

void MixedEffectsRemlDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("响应=%1 随机=%2 固定A=%3 REML=%4")
            .arg(cfg.response_column.value_or(0))
            .arg(cfg.random_factor_column.value_or(0))
            .arg(cfg.fixed_factor_a_column.has_value()
                     ? QString::number(*cfg.fixed_factor_a_column) : "-")
            .arg(QString::fromStdString(cfg.reml_method)));
}

void MixedEffectsRemlDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void MixedEffectsRemlDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void MixedEffectsRemlDialog::on_next()
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

void MixedEffectsRemlDialog::on_accept()
{
    QString error;
    if (!validate_columns(&error)) {
        QMessageBox::warning(this, QStringLiteral("列无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}

#include "mixed_effects_reml_dialog.moc"
