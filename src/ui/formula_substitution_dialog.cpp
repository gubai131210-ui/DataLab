#include "ui/formula_substitution_dialog.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
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

FormulaSubstitutionDialog::FormulaSubstitutionDialog(
    datalab::domain::OutputPage page, QWidget* parent)
    : QDialog(parent)
    , page_(std::move(page))
{
    setWindowTitle(QStringLiteral("公式代入"));
    setMinimumSize(720, 520);
    resize(860, 600);

    auto* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);

    list_ = new QListWidget(stack_);
    for (const auto& trace : page_.computation_traces) {
        const QString title = QString::fromStdString(
            trace.title.empty() ? trace.formula_id : trace.title);
        list_->addItem(title);
    }
    if (list_->count() == 0) {
        list_->addItem(QStringLiteral("（本页暂无公式代入轨迹）"));
        list_->setEnabled(false);
    } else {
        list_->setCurrentRow(0);
    }
    stack_->addWidget(make_titled_page(QStringLiteral("公式列表"), list_, stack_));

    bindings_ = new QTableWidget(0, 4, stack_);
    bindings_->setHorizontalHeaderLabels(
        {QStringLiteral("符号"), QStringLiteral("含义"), QStringLiteral("取值"),
         QStringLiteral("角色")});
    bindings_->horizontalHeader()->setStretchLastSection(true);
    bindings_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    bindings_->setSelectionBehavior(QAbstractItemView::SelectRows);
    stack_->addWidget(make_titled_page(QStringLiteral("变量取值"), bindings_, stack_));

    auto* preview_body = new QWidget(stack_);
    auto* preview_layout = new QVBoxLayout(preview_body);
    preview_ = new QPlainTextEdit(preview_body);
    preview_->setReadOnly(true);
    preview_->setMaximumHeight(72);
    steps_table_ = new QTableWidget(0, 5, preview_body);
    steps_table_->setHorizontalHeaderLabels(
        {QStringLiteral("序"), QStringLiteral("说明"), QStringLiteral("代入前"),
         QStringLiteral("代入后"), QStringLiteral("得数")});
    steps_table_->horizontalHeader()->setStretchLastSection(true);
    steps_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    steps_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    result_label_ = new QLabel(preview_body);
    result_label_->setWordWrap(true);
    preview_layout->addWidget(new QLabel(QStringLiteral("公式"), preview_body));
    preview_layout->addWidget(preview_);
    preview_layout->addWidget(new QLabel(QStringLiteral("分步求值"), preview_body));
    preview_layout->addWidget(steps_table_, 1);
    preview_layout->addWidget(result_label_);
    stack_->addWidget(make_titled_page(QStringLiteral("分步求值"), preview_body, stack_));

    auto* source_body = new QWidget(stack_);
    auto* source_layout = new QVBoxLayout(source_body);
    evidence_label_ = new QLabel(source_body);
    evidence_label_->setWordWrap(true);
    url_label_ = new QLabel(source_body);
    url_label_->setWordWrap(true);
    url_label_->setTextInteractionFlags(Qt::TextBrowserInteraction);
    url_label_->setOpenExternalLinks(true);
    open_registry_button_ = new QPushButton(QStringLiteral("在公式注册表中打开"), source_body);
    source_layout->addWidget(evidence_label_);
    source_layout->addWidget(url_label_);
    source_layout->addWidget(open_registry_button_);
    source_layout->addStretch(1);
    stack_->addWidget(make_titled_page(QStringLiteral("出处"), source_body, stack_));

    root->addWidget(stack_, 1);

    auto* nav = new QHBoxLayout();
    back_button_ = new QPushButton(QStringLiteral("上一步"), this);
    next_button_ = new QPushButton(QStringLiteral("下一步"), this);
    auto* close_button = new QPushButton(QStringLiteral("关闭"), this);
    nav->addWidget(back_button_);
    nav->addStretch(1);
    nav->addWidget(next_button_);
    nav->addWidget(close_button);
    root->addLayout(nav);

    connect(list_, &QListWidget::currentRowChanged, this, [this](int) {
        on_trace_selected();
    });
    connect(back_button_, &QPushButton::clicked, this, &FormulaSubstitutionDialog::on_back);
    connect(next_button_, &QPushButton::clicked, this, &FormulaSubstitutionDialog::on_next);
    connect(close_button, &QPushButton::clicked, this, &QDialog::accept);
    connect(open_registry_button_, &QPushButton::clicked, this,
            &FormulaSubstitutionDialog::on_open_registry);

    on_trace_selected();
    update_nav();
}

void FormulaSubstitutionDialog::on_back()
{
    const int index = stack_->currentIndex();
    if (index > 0) {
        stack_->setCurrentIndex(index - 1);
        update_nav();
    }
}

void FormulaSubstitutionDialog::on_next()
{
    const int index = stack_->currentIndex();
    if (index + 1 < stack_->count()) {
        stack_->setCurrentIndex(index + 1);
        update_nav();
    }
}

void FormulaSubstitutionDialog::on_trace_selected()
{
    rebuild_bindings();
    rebuild_preview();
    rebuild_source();
}

void FormulaSubstitutionDialog::on_open_registry()
{
    const int index = selected_trace_index();
    QString command_id = QString::fromStdString(page_.analysis_command_id);
    if (index >= 0) {
        const auto& trace = page_.computation_traces[static_cast<std::size_t>(index)];
        if (!trace.command_id.empty()) {
            command_id = QString::fromStdString(trace.command_id);
        }
    }
    emit open_in_formula_registry(command_id);
}

int FormulaSubstitutionDialog::selected_trace_index() const
{
    if (page_.computation_traces.empty()) {
        return -1;
    }
    const int row = list_->currentRow();
    if (row < 0 || row >= static_cast<int>(page_.computation_traces.size())) {
        return 0;
    }
    return row;
}

void FormulaSubstitutionDialog::rebuild_bindings()
{
    bindings_->setRowCount(0);
    const int index = selected_trace_index();
    if (index < 0) {
        return;
    }
    const auto& trace = page_.computation_traces[static_cast<std::size_t>(index)];
    bindings_->setRowCount(static_cast<int>(trace.bindings.size()));
    for (int row = 0; row < static_cast<int>(trace.bindings.size()); ++row) {
        const auto& binding = trace.bindings[static_cast<std::size_t>(row)];
        bindings_->setItem(
            row, 0, new QTableWidgetItem(QString::fromStdString(binding.symbol)));
        bindings_->setItem(
            row, 1, new QTableWidgetItem(QString::fromStdString(binding.label)));
        bindings_->setItem(
            row, 2, new QTableWidgetItem(QString::fromStdString(binding.value)));
        bindings_->setItem(
            row, 3, new QTableWidgetItem(QString::fromStdString(binding.role)));
    }
}

void FormulaSubstitutionDialog::rebuild_preview()
{
    const int index = selected_trace_index();
    if (index < 0) {
        preview_->setPlainText(QStringLiteral("无代入内容"));
        result_label_->setText({});
        steps_table_->setRowCount(0);
        return;
    }
    const auto& trace = page_.computation_traces[static_cast<std::size_t>(index)];
    preview_->setPlainText(QString::fromStdString(
        trace.plain_formula.empty() ? trace.substituted_text : trace.plain_formula));
    result_label_->setText(QStringLiteral("结果 %1 = %2")
                               .arg(QString::fromStdString(trace.result_symbol),
                                    QString::fromStdString(trace.result_value)));
    steps_table_->setRowCount(static_cast<int>(trace.steps.size()));
    for (int row = 0; row < static_cast<int>(trace.steps.size()); ++row) {
        const auto& step = trace.steps[static_cast<std::size_t>(row)];
        const int order = step.order > 0 ? step.order : row + 1;
        steps_table_->setItem(row, 0, new QTableWidgetItem(QString::number(order)));
        steps_table_->setItem(
            row, 1, new QTableWidgetItem(QString::fromStdString(step.description)));
        steps_table_->setItem(
            row, 2, new QTableWidgetItem(QString::fromStdString(step.expression_before)));
        steps_table_->setItem(
            row, 3, new QTableWidgetItem(QString::fromStdString(step.expression_after)));
        steps_table_->setItem(
            row, 4, new QTableWidgetItem(QString::fromStdString(step.value)));
    }
    steps_table_->resizeColumnsToContents();
}

void FormulaSubstitutionDialog::rebuild_source()
{
    const int index = selected_trace_index();
    if (index < 0) {
        evidence_label_->setText(QStringLiteral("evidence_type: —"));
        url_label_->setText({});
        return;
    }
    const auto& trace = page_.computation_traces[static_cast<std::size_t>(index)];
    evidence_label_->setText(
        QStringLiteral("evidence_type: %1\nformula_id: %2\ncommand_id: %3")
            .arg(QString::fromStdString(trace.evidence_type),
                 QString::fromStdString(trace.formula_id),
                 QString::fromStdString(trace.command_id)));
    const QString url = QString::fromStdString(trace.primary_url);
    if (url.isEmpty()) {
        url_label_->setText(QStringLiteral("primary_url: —"));
    } else {
        url_label_->setText(
            QStringLiteral("<a href=\"%1\">%1</a>").arg(url.toHtmlEscaped()));
    }
}

void FormulaSubstitutionDialog::update_nav()
{
    const int index = stack_->currentIndex();
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index + 1 < stack_->count());
}
