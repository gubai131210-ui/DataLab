#include "ui/mixture_design_dialog.h"

#include "domain/statistics/mixture_design.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

namespace {

QWidget* make_mixture_page(const QString& title, QWidget* body, QWidget* parent)
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

MixtureDesignDialog::MixtureDesignDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Mixture 设计"));
    setMinimumSize(720, 520);
    resize(860, 600);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    auto* options_body = new QWidget(stack_);
    auto* options_layout = new QVBoxLayout(options_body);
    options_layout->addWidget(new QLabel(QStringLiteral("分量数 q（3 或 4）"), options_body));
    q_spin_ = new QSpinBox(options_body);
    q_spin_->setRange(3, 4);
    q_spin_->setValue(3);
    options_layout->addWidget(q_spin_);
    options_layout->addWidget(new QLabel(QStringLiteral("分量名（逗号分隔，可空）"), options_body));
    names_edit_ = new QLineEdit(QStringLiteral("x1,x2,x3"), options_body);
    options_layout->addWidget(names_edit_);
    randomize_check_ = new QCheckBox(QStringLiteral("随机化 RunOrder"), options_body);
    options_layout->addWidget(randomize_check_);
    options_layout->addWidget(new QLabel(QStringLiteral("随机种子"), options_body));
    seed_spin_ = new QSpinBox(options_body);
    seed_spin_->setRange(1, 1000000);
    seed_spin_->setValue(1);
    options_layout->addWidget(seed_spin_);
    options_layout->addStretch(1);
    stack_->addWidget(make_mixture_page(QStringLiteral("选项"), options_body, stack_));

    method_note_ = new QPlainTextEdit(stack_);
    method_note_->setReadOnly(true);
    method_note_->setPlainText(QStringLiteral(
        "Simplex-lattice {q, m=2}（NIST / formula_reference）：\n\n"
        "各分量取 {0, 1/2, 1} 且 Σ x_i = 1。\n"
        "点集 = q 个顶点 + C(q,2) 个边中点。\n"
        "总运行 N = q(q+1)/2（q=3→6，q=4→10）。\n\n"
        "本命令仅设计生成与写表；Scheffé 分析留给后续波次。"));
    stack_->addWidget(make_mixture_page(QStringLiteral("方法说明"), method_note_, stack_));

    matrix_preview_ = new QTableWidget(0, 2, stack_);
    matrix_preview_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    matrix_preview_->horizontalHeader()->setStretchLastSection(true);
    stack_->addWidget(make_mixture_page(QStringLiteral("设计矩阵预览"), matrix_preview_, stack_));

    confirm_ = new QPlainTextEdit(stack_);
    confirm_->setReadOnly(true);
    stack_->addWidget(make_mixture_page(QStringLiteral("写入确认"), confirm_, stack_));

    root->addWidget(stack_, 1);
    auto* nav = new QHBoxLayout();
    back_button_ = new QPushButton(QStringLiteral("上一步"), this);
    next_button_ = new QPushButton(QStringLiteral("下一步"), this);
    run_button_ = new QPushButton(QStringLiteral("生成并写入"), this);
    auto* cancel = new QPushButton(QStringLiteral("取消"), this);
    nav->addWidget(back_button_);
    nav->addStretch(1);
    nav->addWidget(next_button_);
    nav->addWidget(run_button_);
    nav->addWidget(cancel);
    root->addLayout(nav);

    connect(back_button_, &QPushButton::clicked, this, &MixtureDesignDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &MixtureDesignDialog::on_next);
    connect(run_button_, &QPushButton::clicked, this, &MixtureDesignDialog::on_accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    update_nav();
}

datalab::domain::MixtureDesignConfiguration MixtureDesignDialog::configuration() const
{
    datalab::domain::MixtureDesignConfiguration cfg;
    cfg.component_count = static_cast<std::size_t>(q_spin_->value());
    const QStringList names =
        names_edit_->text().split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString& name : names) {
        cfg.component_names.push_back(name.trimmed().toStdString());
    }
    cfg.randomize = randomize_check_->isChecked();
    cfg.random_seed = static_cast<std::uint64_t>(seed_spin_->value());
    return cfg;
}

bool MixtureDesignDialog::validate_options(QString* error) const
{
    const int q = q_spin_->value();
    if (q < 3 || q > 4) {
        if (error != nullptr) {
            *error = QStringLiteral("q 仅支持 3 或 4。");
        }
        return false;
    }
    return true;
}

void MixtureDesignDialog::rebuild_matrix_preview()
{
    const auto cfg = configuration();
    datalab::domain::statistics::MixtureDesignOptions options;
    options.component_count = cfg.component_count;
    options.component_names = cfg.component_names;
    options.randomize = cfg.randomize;
    options.random_seed = cfg.random_seed;
    const auto design =
        datalab::domain::statistics::generate_mixture_simplex_lattice(options);

    QStringList headers;
    headers << QStringLiteral("StdOrder") << QStringLiteral("RunOrder");
    for (const auto& name : design.component_names) {
        headers << QString::fromStdString(name);
    }
    matrix_preview_->clear();
    matrix_preview_->setColumnCount(headers.size());
    matrix_preview_->setHorizontalHeaderLabels(headers);
    matrix_preview_->setRowCount(static_cast<int>(design.runs.size()));
    for (int r = 0; r < static_cast<int>(design.runs.size()); ++r) {
        const auto& run = design.runs[static_cast<std::size_t>(r)];
        matrix_preview_->setItem(
            r, 0, new QTableWidgetItem(QString::number(run.standard_order)));
        matrix_preview_->setItem(
            r, 1, new QTableWidgetItem(QString::number(run.run_order)));
        for (int c = 0; c < static_cast<int>(run.proportions.size()); ++c) {
            matrix_preview_->setItem(
                r, c + 2,
                new QTableWidgetItem(QString::number(run.proportions[static_cast<std::size_t>(c)])));
        }
    }
}

void MixtureDesignDialog::rebuild_confirm()
{
    const auto cfg = configuration();
    const std::size_t n = cfg.component_count * (cfg.component_count + 1) / 2;
    confirm_->setPlainText(QStringLiteral(
        "将生成 simplex-lattice 设计并写入工作表：\n"
        "q = %1，N = %2，degree m = 2。\n"
        "列：分量比例 + RunOrder + 空 Response。\n"
        "新工作表会清空旧 excluded_rows / hidden_rows（保护 A→B）。\n"
        "本命令不做 Mixture 回归分析。")
                               .arg(cfg.component_count)
                               .arg(n));
}

void MixtureDesignDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
    run_button_->setVisible(index + 1 == stack_->count());
    if (index == 2) {
        rebuild_matrix_preview();
    }
    if (index == 3) {
        rebuild_confirm();
    }
}

void MixtureDesignDialog::on_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_nav();
    }
}

void MixtureDesignDialog::on_next()
{
    if (stack_->currentIndex() == 0) {
        QString error;
        if (!validate_options(&error)) {
            QMessageBox::warning(this, QStringLiteral("选项无效"), error);
            return;
        }
    }
    if (stack_->currentIndex() + 1 < stack_->count()) {
        stack_->setCurrentIndex(stack_->currentIndex() + 1);
        update_nav();
    }
}

void MixtureDesignDialog::on_accept()
{
    QString error;
    if (!validate_options(&error)) {
        QMessageBox::warning(this, QStringLiteral("选项无效"), error);
        return;
    }
    accepted_valid_ = true;
    accept();
}
