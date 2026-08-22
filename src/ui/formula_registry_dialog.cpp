#include "ui/formula_registry_dialog.h"

#include "ui/algorithm_help_catalog.h"
#include "ui/formula_renderer.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSplitter>
#include <QHash>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <optional>

namespace {

bool entry_has_formula_content(const AlgorithmHelpEntry& entry)
{
    return !entry.formula_blocks.isEmpty()
        || !entry.reference_links.isEmpty()
        || !entry.source_documents.isEmpty();
}

bool matches_registry_filter(const AlgorithmHelpEntry& entry, const QString& filter)
{
    if (filter.isEmpty()) {
        return true;
    }
    const auto contains = [&](const QString& text) {
        return text.contains(filter, Qt::CaseInsensitive);
    };
    if (contains(entry.id) || contains(entry.title) || contains(entry.category)
        || contains(entry.wiring.command_id) || contains(entry.wiring.facts_type)
        || contains(entry.wiring.service_method)) {
        return true;
    }
    for (const FormulaBlock& block : entry.formula_blocks) {
        if (contains(block.title) || contains(block.plain_text)
            || contains(block.explanation) || contains(block.conditions)
            || contains(FormulaRenderer::to_plain_text(block.nodes))) {
            return true;
        }
    }
    for (const HelpReferenceLink& link : entry.reference_links) {
        if (contains(link.label) || contains(link.url) || contains(link.kind)) {
            return true;
        }
    }
    for (const HelpSourceDocument& source : entry.source_documents) {
        if (contains(source.label) || contains(source.path) || contains(source.section)) {
            return true;
        }
    }
    return false;
}

QString section_html(const QString& title, const QString& body)
{
    return QStringLiteral(
        "<h3 style=\"margin-top:18px; margin-bottom:6px;\">%1</h3>"
        "<div style=\"margin-bottom:8px;\">%2</div>")
        .arg(FormulaRenderer::escape_html(title), body);
}

}  // namespace

FormulaRegistryDialog::FormulaRegistryDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("公式注册表"));
    setMinimumSize(960, 620);
    resize(1120, 720);

    catalog_ = AlgorithmHelpCatalogLoader::load_from_resource();
    QString validation_error;
    if (!AlgorithmHelpCatalogLoader::validate(catalog_, &validation_error)) {
        load_error_ = validation_error;
    }

    auto* root_layout = new QVBoxLayout(this);
    auto* intro = new QLabel(
        QStringLiteral(
            "按命令 id 或公式符号搜索；公式块来自 algorithm_help.json，"
            "外链为 NIST/Minitab 等 Primary URL，仓库路径为 research md。"),
        this);
    intro->setWordWrap(true);
    intro->setStyleSheet(QStringLiteral("color:#526a73; padding:0 0 6px;"));
    root_layout->addWidget(intro);

    auto* toolbar = new QHBoxLayout();
    search_edit_ = new QLineEdit(this);
    search_edit_->setPlaceholderText(QStringLiteral("搜索 id、公式符号、NIST/Minitab 链接、research md 路径…"));
    copy_id_button_ = new QPushButton(QStringLiteral("复制 id"), this);
    copy_formula_button_ = new QPushButton(QStringLiteral("复制公式纯文本"), this);
    open_reference_button_ = new QPushButton(QStringLiteral("打开参考网站"), this);
    toolbar->addWidget(search_edit_, 1);
    toolbar->addWidget(copy_id_button_);
    toolbar->addWidget(copy_formula_button_);
    toolbar->addWidget(open_reference_button_);
    root_layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    tree_ = new QTreeWidget(splitter);
    tree_->setHeaderLabels({QStringLiteral("id / 标题")});
    tree_->setColumnWidth(0, 320);
    detail_browser_ = new QTextBrowser(splitter);
    detail_browser_->setObjectName(QStringLiteral("formulaRegistryDetailBrowser"));
    detail_browser_->setOpenExternalLinks(true);
    detail_browser_->setStyleSheet(
        QStringLiteral("QTextBrowser { font-family: 'Segoe UI','Microsoft YaHei',sans-serif; }"));
    splitter->addWidget(tree_);
    splitter->addWidget(detail_browser_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({320, 760});
    root_layout->addWidget(splitter, 1);

    connect(search_edit_, &QLineEdit::textChanged, this, &FormulaRegistryDialog::on_search_text_changed);
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, &FormulaRegistryDialog::on_tree_selection_changed);
    connect(copy_formula_button_, &QPushButton::clicked, this, &FormulaRegistryDialog::copy_formula_plain_text);
    connect(copy_id_button_, &QPushButton::clicked, this, &FormulaRegistryDialog::copy_entry_id);
    connect(open_reference_button_, &QPushButton::clicked, this, &FormulaRegistryDialog::open_selected_reference);

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
        detail_browser_->setHtml(QStringLiteral(
            "<h2>公式注册表加载失败</h2><p>%1</p>")
                                     .arg(FormulaRenderer::escape_html(
                                         load_error_.isEmpty()
                                             ? QStringLiteral("资源缺失或格式错误。")
                                             : load_error_)));
    }
}

void FormulaRegistryDialog::select_entry(const QString& id)
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

void FormulaRegistryDialog::rebuild_tree(const QString& filter)
{
    tree_->clear();
    QHash<QString, QTreeWidgetItem*> categories;
    for (const AlgorithmHelpEntry& entry : catalog_.entries) {
        if (!entry_has_formula_content(entry) || !matches_registry_filter(entry, filter)) {
            continue;
        }
        QTreeWidgetItem* category_item = categories.value(entry.category, nullptr);
        if (category_item == nullptr) {
            category_item = new QTreeWidgetItem(tree_, {entry.category});
            categories.insert(entry.category, category_item);
        }
        auto* entry_item = new QTreeWidgetItem(
            category_item,
            {QStringLiteral("%1 — %2").arg(entry.id, entry.title)});
        entry_item->setData(0, Qt::UserRole, entry.id);
        entry_item->setToolTip(
            0,
            QStringLiteral("命令 id: %1\nFacts: %2")
                .arg(entry.wiring.command_id, entry.wiring.facts_type));
    }
    tree_->expandAll();
}

void FormulaRegistryDialog::on_search_text_changed(const QString& text)
{
    rebuild_tree(text.trimmed());
    if (tree_->topLevelItemCount() > 0) {
        for (int category_index = 0; category_index < tree_->topLevelItemCount(); ++category_index) {
            QTreeWidgetItem* category_item = tree_->topLevelItem(category_index);
            if (category_item != nullptr && category_item->childCount() > 0) {
                tree_->setCurrentItem(category_item->child(0));
                return;
            }
        }
    }
    detail_browser_->clear();
}

void FormulaRegistryDialog::on_tree_selection_changed()
{
    QTreeWidgetItem* item = tree_->currentItem();
    if (item == nullptr || item->parent() == nullptr) {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    const std::optional<AlgorithmHelpEntry> entry =
        AlgorithmHelpCatalogLoader::find_by_id(catalog_, id);
    if (entry.has_value()) {
        show_entry(*entry);
    }
}

void FormulaRegistryDialog::show_entry(const AlgorithmHelpEntry& entry)
{
    detail_browser_->setHtml(build_formula_html(entry));
}

QString FormulaRegistryDialog::build_formula_html(const AlgorithmHelpEntry& entry) const
{
    QString html = QStringLiteral(
        "<h2 style=\"margin-top:0;\">%1</h2>"
        "<p><b>命令 id</b>: <code>%2</code><br/>"
        "<b>Facts</b>: <code>%3</code><br/>"
        "<b>服务方法</b>: <code>%4</code></p>")
                       .arg(
                           FormulaRenderer::escape_html(entry.title),
                           FormulaRenderer::escape_html(entry.id),
                           FormulaRenderer::escape_html(entry.wiring.facts_type),
                           FormulaRenderer::escape_html(entry.wiring.service_method));

    if (entry.formula_blocks.isEmpty()) {
        html += section_html(
            QStringLiteral("公式块"),
            QStringLiteral("<p>（该条目暂无 formula_blocks；请查方法说明或 research md。）</p>"));
    } else {
        for (const FormulaBlock& block : entry.formula_blocks) {
            QString block_html = FormulaRenderer::to_html(block.nodes);
            if (!block.explanation.isEmpty()) {
                block_html += QStringLiteral("<p>%1</p>")
                                  .arg(FormulaRenderer::escape_html(block.explanation));
            }
            if (!block.conditions.isEmpty()) {
                block_html += QStringLiteral("<p><i>条件：</i>%1</p>")
                                  .arg(FormulaRenderer::escape_html(block.conditions));
            }
            if (!block.note.isEmpty()) {
                block_html += QStringLiteral("<p><i>注：</i>%1</p>")
                                  .arg(FormulaRenderer::escape_html(block.note));
            }
            html += section_html(block.title, block_html);
        }
    }

    if (!entry.reference_links.isEmpty()) {
        QString links_html = QStringLiteral("<ul>");
        for (const HelpReferenceLink& link : entry.reference_links) {
            links_html += QStringLiteral(
                "<li><a href=\"%1\">%2</a> <span style=\"color:#647b84;\">(%3 · %4)</span></li>")
                              .arg(
                                  FormulaRenderer::escape_html(link.url),
                                  FormulaRenderer::escape_html(link.label),
                                  FormulaRenderer::escape_html(link.kind),
                                  FormulaRenderer::escape_html(link.accessed));
        }
        links_html += QStringLiteral("</ul>");
        html += section_html(QStringLiteral("官方参考链接（Primary URL）"), links_html);
    }

    if (!entry.source_documents.isEmpty()) {
        QString docs_html = QStringLiteral("<ul>");
        for (const HelpSourceDocument& source : entry.source_documents) {
            docs_html += QStringLiteral("<li><b>%1</b><br/><code>%2</code>")
                             .arg(
                                 FormulaRenderer::escape_html(source.label),
                                 FormulaRenderer::escape_html(source.path));
            if (!source.section.isEmpty()) {
                docs_html += QStringLiteral(" · %1")
                                 .arg(FormulaRenderer::escape_html(source.section));
            }
            docs_html += QStringLiteral("</li>");
        }
        docs_html += QStringLiteral("</ul>");
        html += section_html(QStringLiteral("仓库 research md"), docs_html);
    }

    html += section_html(
        QStringLiteral("证据类型"),
        QStringLiteral(
            "<p>测试标注 <code># source: formula_reference</code>；"
            "不得把 Minitab 未导出数值登记为 golden / vendor_oracle。</p>"));
    return html;
}

void FormulaRegistryDialog::copy_formula_plain_text()
{
    QTreeWidgetItem* item = tree_->currentItem();
    if (item == nullptr || item->parent() == nullptr) {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    const std::optional<AlgorithmHelpEntry> entry =
        AlgorithmHelpCatalogLoader::find_by_id(catalog_, id);
    if (!entry.has_value()) {
        return;
    }
    QStringList parts;
    parts.push_back(entry->id);
    for (const FormulaBlock& block : entry->formula_blocks) {
        parts.push_back(block.title);
        if (!block.plain_text.isEmpty()) {
            parts.push_back(block.plain_text);
        } else {
            parts.push_back(FormulaRenderer::to_plain_text(block.nodes));
        }
    }
    QApplication::clipboard()->setText(parts.join(QStringLiteral("\n\n")));
}

void FormulaRegistryDialog::copy_entry_id()
{
    QTreeWidgetItem* item = tree_->currentItem();
    if (item == nullptr || item->parent() == nullptr) {
        return;
    }
    QApplication::clipboard()->setText(item->data(0, Qt::UserRole).toString());
}

void FormulaRegistryDialog::open_selected_reference()
{
    QTreeWidgetItem* item = tree_->currentItem();
    if (item == nullptr || item->parent() == nullptr) {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    const std::optional<AlgorithmHelpEntry> entry =
        AlgorithmHelpCatalogLoader::find_by_id(catalog_, id);
    if (!entry.has_value() || entry->reference_links.isEmpty()) {
        QMessageBox::information(
            this,
            QStringLiteral("打开参考网站"),
            QStringLiteral("当前条目没有官方参考链接。"));
        return;
    }
    const HelpReferenceLink& link = entry->reference_links.front();
    if (!QDesktopServices::openUrl(QUrl(link.url))) {
        QMessageBox::warning(
            this,
            QStringLiteral("打开参考网站"),
            QStringLiteral("无法打开链接：%1").arg(link.url));
    }
}
