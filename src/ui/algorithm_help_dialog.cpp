#include "ui/algorithm_help_dialog.h"

#include "ui/algorithm_help_catalog.h"
#include "ui/formula_renderer.h"

#include <QApplication>
#include <QClipboard>
#include <QHash>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStringList>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QString section_html(const QString& title, const QString& body)
{
    return QStringLiteral(
        "<h3 style=\"margin-top:18px; margin-bottom:6px;\">%1</h3>"
        "<div style=\"margin-bottom:8px;\">%2</div>")
        .arg(FormulaRenderer::escape_html(title), body);
}

bool matches_filter(const AlgorithmHelpEntry& entry, const QString& filter)
{
    if (filter.isEmpty()) {
        return true;
    }
    const auto contains = [&](const QString& text) {
        return text.contains(filter, Qt::CaseInsensitive);
    };
    if (contains(entry.id) || contains(entry.title) || contains(entry.category)
        || contains(entry.menu_path) || contains(entry.implemented_status)
        || contains(entry.purpose) || contains(entry.method_overview)
        || contains(entry.input_description) || contains(entry.missing_value_policy)
        || contains(entry.invalid_input_conditions) || contains(entry.output_interpretation)
        || contains(entry.interpretation_limits)) {
        return true;
    }
    for (const QString& alias : entry.aliases) {
        if (contains(alias)) {
            return true;
        }
    }
    for (const QString& step : entry.calculation_steps) {
        if (contains(step)) {
            return true;
        }
    }
    for (const QString& rule : entry.decision_rules) {
        if (contains(rule)) {
            return true;
        }
    }
    for (const HelpSymbol& symbol : entry.symbol_definitions) {
        if (contains(symbol.symbol) || contains(symbol.meaning)) {
            return true;
        }
    }
    for (const FormulaBlock& block : entry.formula_blocks) {
        if (contains(block.title) || contains(block.plain_text)
            || contains(block.explanation) || contains(block.conditions)
            || contains(FormulaRenderer::to_plain_text(block.nodes))) {
            return true;
        }
    }
    for (const HelpReferenceLink& link : entry.reference_links) {
        if (contains(link.label) || contains(link.url)) {
            return true;
        }
    }
    for (const HelpSourceDocument& source : entry.source_documents) {
        if (contains(source.label) || contains(source.path)) {
            return true;
        }
    }
    if (contains(entry.wiring.service_method) || contains(entry.wiring.facts_type)) {
        return true;
    }
    return false;
}

QString list_html(const QStringList& items)
{
    QString html = QStringLiteral("<ol>");
    for (const QString& item : items) {
        html += QStringLiteral("<li>%1</li>")
                    .arg(FormulaRenderer::escape_html(item).replace('\n', QStringLiteral("<br/>")));
    }
    html += QStringLiteral("</ol>");
    return html;
}

QString bullet_html(const QStringList& items)
{
    QString html = QStringLiteral("<ul>");
    for (const QString& item : items) {
        html += QStringLiteral("<li>%1</li>")
                    .arg(FormulaRenderer::escape_html(item).replace('\n', QStringLiteral("<br/>")));
    }
    html += QStringLiteral("</ul>");
    return html;
}

}  // namespace

AlgorithmHelpDialog::AlgorithmHelpDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("算法、公式与参考资料"));
    setMinimumSize(980, 640);
    resize(1180, 760);

    catalog_ = AlgorithmHelpCatalogLoader::load_from_resource();
    QString validation_error;
    if (!AlgorithmHelpCatalogLoader::validate(catalog_, &validation_error)) {
        load_error_ = validation_error;
    }

    auto* root_layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    search_edit_ = new QLineEdit(this);
    search_edit_->setPlaceholderText(QStringLiteral("搜索标题、命令 id、公式符号、参考来源…"));
    copy_formula_button_ = new QPushButton(QStringLiteral("复制公式纯文本"), this);
    copy_summary_button_ = new QPushButton(QStringLiteral("复制条目摘要"), this);
    open_reference_button_ = new QPushButton(QStringLiteral("打开参考网站"), this);
    formula_registry_button_ = new QPushButton(QStringLiteral("在公式注册表中打开"), this);
    toolbar->addWidget(search_edit_, 1);
    toolbar->addWidget(copy_formula_button_);
    toolbar->addWidget(copy_summary_button_);
    toolbar->addWidget(open_reference_button_);
    toolbar->addWidget(formula_registry_button_);
    root_layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    tree_ = new QTreeWidget(splitter);
    tree_->setHeaderHidden(true);
    detail_tabs_ = new QTabWidget(splitter);
    detail_tabs_->setObjectName(QStringLiteral("algorithmHelpDetailTabs"));
    detail_browser_ = new QTextBrowser(detail_tabs_);
    detail_browser_->setOpenExternalLinks(true);
    detail_browser_->setStyleSheet(
        QStringLiteral("QTextBrowser { font-family: 'Segoe UI','Microsoft YaHei',sans-serif; }"));
    formula_browser_ = new QTextBrowser(detail_tabs_);
    formula_browser_->setObjectName(QStringLiteral("algorithmHelpFormulaBrowser"));
    formula_browser_->setOpenExternalLinks(true);
    formula_browser_->setStyleSheet(
        QStringLiteral("QTextBrowser { font-family: 'Segoe UI','Microsoft YaHei',sans-serif; }"));
    detail_tabs_->addTab(detail_browser_, QStringLiteral("方法说明"));
    detail_tabs_->addTab(formula_browser_, QStringLiteral("公式与来源"));
    splitter->addWidget(tree_);
    splitter->addWidget(detail_tabs_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({280, 860});
    root_layout->addWidget(splitter, 1);

    connect(search_edit_, &QLineEdit::textChanged, this, &AlgorithmHelpDialog::on_search_text_changed);
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, &AlgorithmHelpDialog::on_tree_selection_changed);
    connect(copy_formula_button_, &QPushButton::clicked, this, &AlgorithmHelpDialog::copy_formula_plain_text);
    connect(copy_summary_button_, &QPushButton::clicked, this, &AlgorithmHelpDialog::copy_entry_summary);
    connect(open_reference_button_, &QPushButton::clicked, this, &AlgorithmHelpDialog::open_selected_reference);
    connect(formula_registry_button_, &QPushButton::clicked, this,
            &AlgorithmHelpDialog::emit_open_in_formula_registry);

    rebuild_tree(QString());
    if (tree_->topLevelItemCount() > 0) {
        for (int category_index = 0; category_index < tree_->topLevelItemCount(); ++category_index) {
            QTreeWidgetItem* category_item = tree_->topLevelItem(category_index);
            if (category_item != nullptr && category_item->childCount() > 0) {
                tree_->setCurrentItem(category_item->child(0));
                break;
            }
        }
    } else {
        const QString error_html = QStringLiteral(
            "<h2>帮助目录加载失败</h2><p>%1</p>")
                                       .arg(FormulaRenderer::escape_html(load_error_.isEmpty()
                                                                             ? QStringLiteral("资源缺失或格式错误。")
                                                                             : load_error_));
        detail_browser_->setHtml(error_html);
        formula_browser_->setHtml(error_html);
    }
}

void AlgorithmHelpDialog::select_entry(const QString& id)
{
    for (int category_index = 0; category_index < tree_->topLevelItemCount(); ++category_index) {
        QTreeWidgetItem* category_item = tree_->topLevelItem(category_index);
        for (int entry_index = 0; entry_index < category_item->childCount(); ++entry_index) {
            QTreeWidgetItem* entry_item = category_item->child(entry_index);
            if (entry_item->data(0, Qt::UserRole).toString() == id) {
                tree_->setCurrentItem(entry_item);
                return;
            }
        }
    }
}

void AlgorithmHelpDialog::rebuild_tree(const QString& filter)
{
    tree_->clear();
    QHash<QString, QTreeWidgetItem*> categories;
    for (const AlgorithmHelpEntry& entry : catalog_.entries) {
        if (!matches_filter(entry, filter)) {
            continue;
        }
        QTreeWidgetItem* category_item = categories.value(entry.category, nullptr);
        if (category_item == nullptr) {
            category_item = new QTreeWidgetItem(tree_, {entry.category});
            categories.insert(entry.category, category_item);
        }
        auto* item = new QTreeWidgetItem(category_item, {entry.title});
        item->setData(0, Qt::UserRole, entry.id);
        item->setToolTip(0, entry.menu_path);
    }
    tree_->expandAll();
    if (tree_->topLevelItemCount() == 0) {
        detail_browser_->setHtml(QStringLiteral("<p>没有匹配的算法条目。</p>"));
        formula_browser_->setHtml(QStringLiteral("<p>没有匹配的算法条目。</p>"));
        current_formula_plain_text_.clear();
        current_reference_url_.clear();
    }
}

void AlgorithmHelpDialog::on_search_text_changed(const QString& text)
{
    rebuild_tree(text.trimmed());
}

void AlgorithmHelpDialog::on_tree_selection_changed()
{
    const QList<QTreeWidgetItem*> selected = tree_->selectedItems();
    if (selected.isEmpty() || selected.front()->childCount() > 0) {
        return;
    }
    const QString id = selected.front()->data(0, Qt::UserRole).toString();
    const auto entry = AlgorithmHelpCatalogLoader::find_by_id(catalog_, id);
    if (entry.has_value()) {
        show_entry(*entry);
    }
}

void AlgorithmHelpDialog::show_entry(const AlgorithmHelpEntry& entry)
{
    current_entry_id_ = entry.id;
    detail_browser_->setHtml(build_entry_html(entry));
    formula_browser_->setHtml(build_formula_sources_html(entry));
    QStringList formula_texts;
    for (const FormulaBlock& block : entry.formula_blocks) {
        if (!block.plain_text.isEmpty()) {
            formula_texts.push_back(block.plain_text);
        } else if (!block.nodes.isEmpty()) {
            formula_texts.push_back(
                block.title + QStringLiteral("\n") + FormulaRenderer::to_plain_text(block.nodes));
        }
    }
    current_formula_plain_text_ = formula_texts.join(QStringLiteral("\n\n"));
    current_reference_url_ = entry.reference_links.isEmpty()
        ? QString()
        : entry.reference_links.front().url;
    copy_formula_button_->setEnabled(!current_formula_plain_text_.isEmpty());
    copy_summary_button_->setEnabled(true);
    open_reference_button_->setEnabled(!current_reference_url_.isEmpty());
}

QString AlgorithmHelpDialog::status_label(const QString& status) const
{
    if (status == QStringLiteral("implemented")) {
        return QStringLiteral("已实现");
    }
    if (status == QStringLiteral("formula_reference")) {
        return QStringLiteral("公式参考（非 Minitab golden）");
    }
    if (status == QStringLiteral("diagnostic_only")) {
        return QStringLiteral("仅诊断/部分输出");
    }
    if (status == QStringLiteral("deferred")) {
        return QStringLiteral("延后");
    }
    return status;
}

QString AlgorithmHelpDialog::build_entry_html(const AlgorithmHelpEntry& entry) const
{
    QString html;
    html += QStringLiteral("<h2>%1</h2>").arg(FormulaRenderer::escape_html(entry.title));
    html += QStringLiteral("<p><b>命令 id：</b>%1<br/>"
                           "<b>分类：</b>%2<br/>"
                           "<b>菜单位置：</b>%3<br/>"
                           "<b>实现状态：</b>%4</p>")
                .arg(FormulaRenderer::escape_html(entry.id),
                     FormulaRenderer::escape_html(entry.category),
                     FormulaRenderer::escape_html(entry.menu_path),
                     FormulaRenderer::escape_html(status_label(entry.implemented_status)));

    html += section_html(QStringLiteral("用途"),
                         FormulaRenderer::escape_html(entry.purpose).replace('\n', QStringLiteral("<br/>")));
    html += section_html(QStringLiteral("方法概述"),
                         FormulaRenderer::escape_html(entry.method_overview).replace('\n', QStringLiteral("<br/>")));
    html += section_html(QStringLiteral("适用场景与输入"),
                         FormulaRenderer::escape_html(entry.input_description).replace('\n', QStringLiteral("<br/>")));
    html += section_html(QStringLiteral("缺失值处理"),
                         FormulaRenderer::escape_html(entry.missing_value_policy).replace('\n', QStringLiteral("<br/>")));
    html += section_html(QStringLiteral("计算步骤"), list_html(entry.calculation_steps));

    html += section_html(QStringLiteral("核心公式"), QString());
    for (const FormulaBlock& block : entry.formula_blocks) {
        html += QStringLiteral("<div style=\"margin:8px 0 12px 0; padding:8px 10px; border-left:3px solid #4a78c2; background:rgba(74,120,194,0.08);\">");
        html += QStringLiteral("<div style=\"font-weight:600; margin-bottom:4px;\">%1</div>")
                    .arg(FormulaRenderer::escape_html(block.title));
        if (!block.nodes.isEmpty()) {
            html += FormulaRenderer::to_html(block.nodes);
        } else if (!block.plain_text.isEmpty()) {
            html += FormulaRenderer::plain_text_to_html(block.plain_text);
        }
        if (!block.explanation.isEmpty()) {
            html += QStringLiteral("<div style=\"margin-top:6px;\">%1</div>")
                        .arg(FormulaRenderer::escape_html(block.explanation)
                                 .replace('\n', QStringLiteral("<br/>")));
        }
        if (!block.conditions.isEmpty()) {
            html += QStringLiteral("<div style=\"margin-top:6px;\"><b>适用条件：</b>%1</div>")
                        .arg(FormulaRenderer::escape_html(block.conditions)
                                 .replace('\n', QStringLiteral("<br/>")));
        }
        if (!block.note.isEmpty()) {
            html += QStringLiteral("<div style=\"margin-top:6px; color:#555;\">%1</div>")
                        .arg(FormulaRenderer::escape_html(block.note));
        }
        html += QStringLiteral("</div>");
    }

    QString symbols = QStringLiteral("<table style=\"border-collapse:collapse;\">");
    for (const HelpSymbol& symbol : entry.symbol_definitions) {
        symbols += QStringLiteral(
                       "<tr><td style=\"padding:3px 10px 3px 0; font-weight:600; vertical-align:top;\">%1</td>"
                       "<td style=\"padding:3px 0;\">%2</td></tr>")
                       .arg(FormulaRenderer::escape_html(symbol.symbol),
                            FormulaRenderer::escape_html(symbol.meaning));
    }
    symbols += QStringLiteral("</table>");
    html += section_html(QStringLiteral("符号定义"), symbols);
    html += section_html(QStringLiteral("判定规则"), bullet_html(entry.decision_rules));
    html += section_html(QStringLiteral("输出表 / 图"),
                         FormulaRenderer::escape_html(entry.output_description).replace('\n', QStringLiteral("<br/>")));
    html += section_html(QStringLiteral("如何阅读结果"),
                         FormulaRenderer::escape_html(entry.output_interpretation).replace('\n', QStringLiteral("<br/>")));
    html += section_html(QStringLiteral("不能计算 / 错误边界"),
                         FormulaRenderer::escape_html(entry.invalid_input_conditions).replace('\n', QStringLiteral("<br/>")));
    html += section_html(QStringLiteral("假设与边界"),
                         FormulaRenderer::escape_html(entry.assumptions_and_boundaries).replace('\n', QStringLiteral("<br/>")));
    html += section_html(QStringLiteral("解释限制"),
                         FormulaRenderer::escape_html(entry.interpretation_limits).replace('\n', QStringLiteral("<br/>")));

    QString references;
    for (const HelpReferenceLink& link : entry.reference_links) {
        references += QStringLiteral("<li><a href=\"%1\">%2</a> <span style=\"color:#666;\">(%3)</span></li>")
                          .arg(FormulaRenderer::escape_html(link.url),
                               FormulaRenderer::escape_html(link.label),
                               FormulaRenderer::escape_html(link.accessed));
    }
    html += section_html(QStringLiteral("延伸阅读（可选，需要网络）"),
                         QStringLiteral("<ul>%1</ul>").arg(references));

    html += QStringLiteral(
        "<h3 style=\"margin-top:28px; color:#888;\">维护信息（使用本软件不必阅读）</h3>");
    QString sources;
    for (const HelpSourceDocument& source : entry.source_documents) {
        sources += QStringLiteral("<li>%1 — <code>%2</code> %3</li>")
                       .arg(FormulaRenderer::escape_html(source.label),
                            FormulaRenderer::escape_html(source.path),
                            FormulaRenderer::escape_html(source.section));
    }
    html += section_html(QStringLiteral("仓库公式文档"), QStringLiteral("<ul>%1</ul>").arg(sources));

    html += section_html(
        QStringLiteral("接线与测试"),
        QStringLiteral("<ul>"
                       "<li>命令 id：<code>%1</code></li>"
                       "<li>服务方法：<code>%2</code></li>"
                       "<li>Facts：<code>%3</code></li>"
                       "<li>主要测试：<code>%4</code></li>"
                       "</ul>")
            .arg(FormulaRenderer::escape_html(entry.wiring.command_id),
                 FormulaRenderer::escape_html(entry.wiring.service_method),
                 FormulaRenderer::escape_html(entry.wiring.facts_type),
                 FormulaRenderer::escape_html(entry.wiring.primary_test)));
    return html;
}

QString AlgorithmHelpDialog::build_formula_sources_html(const AlgorithmHelpEntry& entry) const
{
    QString html;
    html += QStringLiteral("<h2>%1</h2>")
                .arg(FormulaRenderer::escape_html(entry.title));
    html += QStringLiteral("<p><code>%1</code> · %2</p>")
                .arg(FormulaRenderer::escape_html(entry.id),
                     FormulaRenderer::escape_html(status_label(entry.implemented_status)));
    html += section_html(QStringLiteral("核心公式"), QString());
    for (const FormulaBlock& block : entry.formula_blocks) {
        html += QStringLiteral("<h4 style=\"margin-top:12px;\">%1</h4>")
                    .arg(FormulaRenderer::escape_html(block.title));
        if (!block.nodes.isEmpty()) {
            html += FormulaRenderer::to_html(block.nodes);
        } else if (!block.plain_text.isEmpty()) {
            html += FormulaRenderer::plain_text_to_html(block.plain_text);
        }
        if (!block.explanation.isEmpty()) {
            html += QStringLiteral("<p>%1</p>")
                        .arg(FormulaRenderer::escape_html(block.explanation)
                                 .replace('\n', QStringLiteral("<br/>")));
        }
        if (!block.conditions.isEmpty()) {
            html += QStringLiteral("<p><em>条件：%1</em></p>")
                        .arg(FormulaRenderer::escape_html(block.conditions)
                                 .replace('\n', QStringLiteral("<br/>")));
        }
        if (!block.note.isEmpty()) {
            html += QStringLiteral("<p style=\"color:#666;\">%1</p>")
                        .arg(FormulaRenderer::escape_html(block.note));
        }
    }
    QString references;
    for (const HelpReferenceLink& link : entry.reference_links) {
        references += QStringLiteral(
                          "<li><a href=\"%1\">%2</a> <span style=\"color:#666;\">访问日期 %3</span></li>")
                          .arg(FormulaRenderer::escape_html(link.url),
                               FormulaRenderer::escape_html(link.label),
                               FormulaRenderer::escape_html(link.accessed));
    }
    html += section_html(QStringLiteral("官方链接与来源"),
                         references.isEmpty()
                             ? QStringLiteral("<p>本条目暂无外链。</p>")
                             : QStringLiteral("<ul>%1</ul>").arg(references));
    return html;
}

void AlgorithmHelpDialog::copy_formula_plain_text()
{
    if (current_formula_plain_text_.isEmpty()) {
        return;
    }
    QApplication::clipboard()->setText(current_formula_plain_text_);
}

void AlgorithmHelpDialog::copy_entry_summary()
{
    const QList<QTreeWidgetItem*> selected = tree_->selectedItems();
    if (selected.isEmpty() || selected.front()->childCount() > 0) {
        return;
    }
    const QString id = selected.front()->data(0, Qt::UserRole).toString();
    const auto entry = AlgorithmHelpCatalogLoader::find_by_id(catalog_, id);
    if (!entry.has_value()) {
        return;
    }
    QString summary = entry->title + QStringLiteral("\n")
        + QStringLiteral("命令 id: ") + entry->id + QStringLiteral("\n")
        + QStringLiteral("状态: ") + status_label(entry->implemented_status) + QStringLiteral("\n\n")
        + QStringLiteral("用途\n") + entry->purpose + QStringLiteral("\n\n")
        + QStringLiteral("方法概述\n") + entry->method_overview + QStringLiteral("\n\n")
        + QStringLiteral("计算步骤\n") + entry->calculation_steps.join(QStringLiteral("\n"))
        + QStringLiteral("\n\n公式\n") + current_formula_plain_text_ + QStringLiteral("\n\n")
        + QStringLiteral("判定规则\n") + entry->decision_rules.join(QStringLiteral("\n"))
        + QStringLiteral("\n\n解释限制\n") + entry->interpretation_limits;
    QApplication::clipboard()->setText(summary);
}

void AlgorithmHelpDialog::open_selected_reference()
{
    if (current_reference_url_.isEmpty()) {
        return;
    }
    const QUrl url(current_reference_url_);
    if (!url.isValid() || !url.scheme().startsWith(QStringLiteral("http"))) {
        QMessageBox::warning(this, QStringLiteral("无效链接"),
                             QStringLiteral("参考链接无效：%1").arg(current_reference_url_));
        return;
    }
    QDesktopServices::openUrl(url);
}

void AlgorithmHelpDialog::emit_open_in_formula_registry()
{
    if (current_entry_id_.isEmpty()) {
        return;
    }
    emit open_in_formula_registry(current_entry_id_);
}
