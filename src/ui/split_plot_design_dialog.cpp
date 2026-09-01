#include "ui/split_plot_design_dialog.h"

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

SplitPlotDesignDialog::SplitPlotDesignDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("2 水平裂区设计"));
    setMinimumSize(720, 520);
    resize(820, 580);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* p1_body = new QWidget(stack_);
    auto* p1_form = new QFormLayout(p1_body);
    factor_count_spin_ = new QSpinBox(p1_body);
    factor_count_spin_->setRange(2, 4);
    factor_count_spin_->setValue(3);
    factor_name_1_ = new QLineEdit(QStringLiteral("A"), p1_body);
    factor_name_2_ = new QLineEdit(QStringLiteral("B"), p1_body);
    factor_name_3_ = new QLineEdit(QStringLiteral("C"), p1_body);
    factor_name_4_ = new QLineEdit(QStringLiteral("D"), p1_body);
    p1_form->addRow(QStringLiteral("因子数"), factor_count_spin_);
    p1_form->addRow(QStringLiteral("因子 1"), factor_name_1_);
    p1_form->addRow(QStringLiteral("因子 2"), factor_name_2_);
    p1_form->addRow(QStringLiteral("因子 3"), factor_name_3_);
    p1_form->addRow(QStringLiteral("因子 4"), factor_name_4_);
    stack_->addWidget(make_titled_page(QStringLiteral("因子数与名称"), p1_body, stack_));

    auto* p2_body = new QWidget(stack_);
    auto* p2_form = new QFormLayout(p2_body);
    htc_combo_ = new QComboBox(p2_body);
    htc_combo_->addItem(QStringLiteral("因子 1 (A)"), 0);
    htc_combo_->addItem(QStringLiteral("因子 2 (B)"), 1);
    htc_combo_->addItem(QStringLiteral("因子 3 (C)"), 2);
    htc_combo_->addItem(QStringLiteral("因子 4 (D)"), 3);
    p2_form->addRow(QStringLiteral("难改因子 (HTC)"), htc_combo_);
    stack_->addWidget(make_titled_page(QStringLiteral("HTC 与设计"), p2_body, stack_));

    auto* p3_body = new QWidget(stack_);
    auto* p3_form = new QFormLayout(p3_body);
    replicate_spin_ = new QSpinBox(p3_body);
    replicate_spin_->setRange(1, 10);
    replicate_spin_->setValue(1);
    randomize_check_ = new QCheckBox(QStringLiteral("随机化运行顺序"), p3_body);
    randomize_check_->setChecked(true);
    p3_form->addRow(QStringLiteral("Whole-plot 复制"), replicate_spin_);
    p3_form->addRow(randomize_check_);
    stack_->addWidget(make_titled_page(QStringLiteral("复制与随机化"), p3_body, stack_));

    design_note_ = new QPlainTextEdit(stack_);
    design_note_->setReadOnly(true);
    design_note_->setPlainText(QStringLiteral(
        "输出含 Whole plot 列；可接 split_plot_analyze 分析。\n"
        "与 split_plot_analyze 对话框独立。"));
    stack_->addWidget(make_titled_page(QStringLiteral("设计说明"), design_note_, stack_));

    preview_ = new QPlainTextEdit(stack_);
    preview_->setReadOnly(true);
    stack_->addWidget(make_titled_page(QStringLiteral("预览矩阵"), preview_, stack_));

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

    connect(back_button_, &QPushButton::clicked, this, &SplitPlotDesignDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &SplitPlotDesignDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &SplitPlotDesignDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::SplitPlotDesignConfiguration SplitPlotDesignDialog::configuration() const
{
    datalab::domain::SplitPlotDesignConfiguration cfg;
    const int count = factor_count_spin_->value();
    QLineEdit* names[] = {factor_name_1_, factor_name_2_, factor_name_3_, factor_name_4_};
    for (int i = 0; i < count; ++i) {
        cfg.factor_names.push_back(names[i]->text().toStdString());
        cfg.low_levels.push_back("-");
        cfg.high_levels.push_back("+");
    }
    cfg.htc_factor_index = static_cast<std::size_t>(htc_combo_->currentData().toInt());
    if (cfg.htc_factor_index >= cfg.factor_names.size()) {
        cfg.htc_factor_index = 0;
    }
    cfg.whole_plot_replicates = static_cast<std::size_t>(replicate_spin_->value());
    cfg.randomize = randomize_check_->isChecked();
    return cfg;
}

bool SplitPlotDesignDialog::validate_input(QString* error) const
{
    const int count = factor_count_spin_->value();
    QLineEdit* names[] = {factor_name_1_, factor_name_2_, factor_name_3_, factor_name_4_};
    for (int i = 0; i < count; ++i) {
        if (names[i]->text().trimmed().isEmpty()) {
            if (error != nullptr) {
                *error = QStringLiteral("因子名称不能为空。");
            }
            return false;
        }
    }
    if (htc_combo_->currentData().toInt() >= count) {
        if (error != nullptr) {
            *error = QStringLiteral("难改因子索引超出因子数。");
        }
        return false;
    }
    return true;
}

void SplitPlotDesignDialog::rebuild_preview()
{
    const auto cfg = configuration();
    preview_->setPlainText(
        QStringLiteral("因子=%1 HTC=%2 复制=%3 随机=%4")
            .arg(cfg.factor_names.size())
            .arg(cfg.htc_factor_index)
            .arg(cfg.whole_plot_replicates)
            .arg(cfg.randomize ? "是" : "否"));
}

void SplitPlotDesignDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == stack_->count() - 1) {
        rebuild_preview();
    }
}

void SplitPlotDesignDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void SplitPlotDesignDialog::on_next()
{
    if (stack_->currentIndex() == 0 || stack_->currentIndex() == 1) {
        QString error;
        if (!validate_input(&error)) {
            QMessageBox::warning(this, QStringLiteral("输入无效"), error);
            return;
        }
    }
    if (stack_->currentIndex() + 1 < stack_->count()) {
        stack_->setCurrentIndex(stack_->currentIndex() + 1);
        update_nav();
    }
}

void SplitPlotDesignDialog::on_accept()
{
    QString error;
    if (!validate_input(&error)) {
        QMessageBox::warning(this, QStringLiteral("输入无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}

#include "split_plot_design_dialog.moc"
