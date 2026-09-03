#include "ui/learning_center_page.h"

#include "application/learning/learning_dataset_store.h"
#include "application/learning/learning_tutorial_catalog.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

using datalab::application::learning::LearningDatasetStore;
using datalab::application::learning::LearningOutputGuideItem;
using datalab::application::learning::LearningTutorialCatalog;
using datalab::application::learning::LearningTutorialEntry;

namespace {

QString section_html(const QString& title, const QString& body)
{
    return QStringLiteral(
               "<h3 style=\"margin-top:18px; margin-bottom:6px;\">%1</h3>"
               "<div style=\"margin-bottom:8px;\">%2</div>")
        .arg(title, body);
}

QString list_html(const QStringList& items)
{
    if (items.isEmpty()) {
        return QStringLiteral("<p>（无）</p>");
    }
    QString html = QStringLiteral("<ol>");
    for (const QString& item : items) {
        html += QStringLiteral("<li>%1</li>").arg(item.toHtmlEscaped());
    }
    html += QStringLiteral("</ol>");
    return html;
}

bool matches_filter(const LearningTutorialEntry& entry, const QString& filter)
{
    if (filter.isEmpty()) {
        return true;
    }
    const auto contains = [&](const QString& text) {
        return text.contains(filter, Qt::CaseInsensitive);
    };
    if (contains(entry.command_id) || contains(entry.title) || contains(entry.category)
        || contains(entry.menu_path) || contains(entry.implemented_status)
        || contains(entry.used_for) || contains(entry.not_for) || contains(entry.scenario)) {
        return true;
    }
    for (const QString& step : entry.click_steps) {
        if (contains(step)) {
            return true;
        }
    }
    for (const QString& mistake : entry.common_mistakes) {
        if (contains(mistake)) {
            return true;
        }
    }
    return false;
}

}  // namespace

LearningCenterPage::LearningCenterPage(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("学习中心"));
    resize(1080, 720);

    auto* root_layout = new QVBoxLayout(this);
    auto* intro = new QLabel(QStringLiteral(
        "按命令浏览黑带级场景说明；可一键导入演示数据到工作区（新建工作表，不覆盖当前表）。"));
    intro->setWordWrap(true);
    root_layout->addWidget(intro);

    search_edit_ = new QLineEdit(this);
    search_edit_->setPlaceholderText(QStringLiteral("搜索命令、场景、菜单路径…"));
    root_layout->addWidget(search_edit_);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    tree_ = new QTreeWidget(splitter);
    tree_->setHeaderHidden(true);
    tree_->setMinimumWidth(260);
    detail_browser_ = new QTextBrowser(splitter);
    detail_browser_->setOpenExternalLinks(true);
    splitter->addWidget(tree_);
    splitter->addWidget(detail_browser_);
    splitter->setStretchFactor(1, 1);
    root_layout->addWidget(splitter, 1);

    auto* button_row = new QHBoxLayout();
    import_button_ = new QPushButton(QStringLiteral("导入测试数据到工作区"), this);
    formula_button_ = new QPushButton(QStringLiteral("打开公式说明"), this);
    auto* export_button = new QPushButton(QStringLiteral("导出 SQLite 数据库…"), this);
    button_row->addWidget(import_button_);
    button_row->addWidget(formula_button_);
    button_row->addStretch();
    button_row->addWidget(export_button);
    root_layout->addLayout(button_row);

    connect(search_edit_, &QLineEdit::textChanged, this, &LearningCenterPage::on_search_text_changed);
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, &LearningCenterPage::on_tree_selection_changed);
    connect(import_button_, &QPushButton::clicked, this, &LearningCenterPage::import_current_dataset);
    connect(formula_button_, &QPushButton::clicked, this, &LearningCenterPage::open_formula_help);
    connect(export_button, &QPushButton::clicked, this, &LearningCenterPage::export_database);

    QString error;
    const auto loaded = LearningTutorialCatalog::load_all(&error);
    entries_.clear();
    entries_.reserve(static_cast<int>(loaded.size()));
    for (const LearningTutorialEntry& entry : loaded) {
        entries_.push_back(entry);
    }
    if (!error.isEmpty()) {
        load_error_ = error;
    } else {
        const QString version = LearningDatasetStore::catalog_version(&error);
        if (!error.isEmpty()) {
            load_error_ = error;
        } else if (version != QString::fromLatin1(LearningDatasetStore::kExpectedCatalogVersion)) {
            load_error_ = QStringLiteral("学习中心数据库版本不匹配：%1").arg(version);
        }
    }

    rebuild_tree(QString());
    if (!load_error_.isEmpty()) {
        detail_browser_->setHtml(section_html(QStringLiteral("加载失败"), load_error_.toHtmlEscaped()));
        import_button_->setEnabled(false);
    }
}

void LearningCenterPage::select_entry(const QString& command_id)
{
    QTreeWidgetItemIterator iterator(tree_);
    while (*iterator != nullptr) {
        QTreeWidgetItem* item = *iterator;
        if (item->data(0, Qt::UserRole).toString() == command_id) {
            tree_->setCurrentItem(item);
            tree_->scrollToItem(item);
            return;
        }
        ++iterator;
    }
}

void LearningCenterPage::on_search_text_changed(const QString& text)
{
    rebuild_tree(text.trimmed());
}

void LearningCenterPage::on_tree_selection_changed()
{
    const QList<QTreeWidgetItem*> selected = tree_->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const QString command_id = selected.front()->data(0, Qt::UserRole).toString();
    if (command_id.isEmpty()) {
        return;
    }
    for (const LearningTutorialEntry& entry : entries_) {
        if (entry.command_id == command_id) {
            show_entry(entry);
            return;
        }
    }
}

void LearningCenterPage::import_current_dataset()
{
    if (current_dataset_id_.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("学习中心"),
                                 QStringLiteral("本命令无需导入演示数据，请直接按步骤打开菜单。"));
        return;
    }
    emit import_demo_requested(current_dataset_id_, current_worksheet_name_);
}

void LearningCenterPage::open_formula_help()
{
    if (current_command_id_.isEmpty()) {
        return;
    }
    emit open_formula_help_requested(current_command_id_);
}

void LearningCenterPage::export_database()
{
    const QString target_path = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出学习中心数据库"), QStringLiteral("learning_center.sqlite"),
        QStringLiteral("SQLite 数据库 (*.sqlite *.db)"));
    if (target_path.isEmpty()) {
        return;
    }
    QFile resource(QString::fromLatin1(LearningDatasetStore::kResourcePath));
    if (!resource.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
                               QStringLiteral("无法读取嵌入数据库。"));
        return;
    }
    QFile target(target_path);
    if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
                               QStringLiteral("无法写入目标文件。"));
        return;
    }
    if (target.write(resource.readAll()) < 0) {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
                               QStringLiteral("写入目标文件失败。"));
        return;
    }
    QMessageBox::information(this, QStringLiteral("导出完成"),
                             QStringLiteral("已导出到：%1").arg(target_path));
}

void LearningCenterPage::rebuild_tree(const QString& filter)
{
    tree_->clear();
    QMap<QString, QTreeWidgetItem*> category_nodes;
    for (const LearningTutorialEntry& entry : entries_) {
        if (!matches_filter(entry, filter)) {
            continue;
        }
        const QString category = entry.category.isEmpty() ? QStringLiteral("其他") : entry.category;
        QTreeWidgetItem* parent = category_nodes.value(category, nullptr);
        if (parent == nullptr) {
            parent = new QTreeWidgetItem(tree_, {category});
            category_nodes.insert(category, parent);
        }
        auto* item = new QTreeWidgetItem(parent, {entry.title});
        item->setData(0, Qt::UserRole, entry.command_id);
    }
    tree_->expandAll();
}

void LearningCenterPage::show_entry(const LearningTutorialEntry& entry)
{
    current_command_id_ = entry.command_id;
    current_dataset_id_ = entry.dataset_id.value_or(QString());
    current_worksheet_name_ = current_dataset_id_.isEmpty()
        ? QString()
        : QStringLiteral("demo_%1").arg(current_dataset_id_);
    import_button_->setEnabled(!current_dataset_id_.isEmpty());
    detail_browser_->setHtml(build_entry_html(entry));
}

QString LearningCenterPage::build_entry_html(const LearningTutorialEntry& entry) const
{
    QString html = QStringLiteral("<h2>%1</h2><p><b>状态：</b>%2</p>")
                       .arg(entry.title.toHtmlEscaped(), status_label(entry.implemented_status));

    html += section_html(QStringLiteral("一句话用途"), entry.used_for.toHtmlEscaped());
    html += section_html(QStringLiteral("不能当什么用"), entry.not_for.toHtmlEscaped());
    html += section_html(QStringLiteral("制造现场故事"), entry.scenario.toHtmlEscaped());

    if (!entry.menu_path.isEmpty()) {
        html += section_html(QStringLiteral("菜单路径"),
                             entry.menu_path.toHtmlEscaped());
    }

    html += section_html(QStringLiteral("逐步操作"), list_html(entry.click_steps));

    if (!entry.dialog_fill.isEmpty()) {
        QString mapping_html = QStringLiteral("<ul>");
        for (auto it = entry.dialog_fill.cbegin(); it != entry.dialog_fill.cend(); ++it) {
            mapping_html += QStringLiteral("<li><b>%1</b> → %2</li>")
                                .arg(it.key().toHtmlEscaped(), it.value().toHtmlEscaped());
        }
        mapping_html += QStringLiteral("</ul>");
        html += section_html(QStringLiteral("对话框列映射"), mapping_html);
    }

    if (!entry.output_guide.isEmpty()) {
        QString guide_html = QStringLiteral("<ul>");
        for (const LearningOutputGuideItem& item : entry.output_guide) {
            guide_html += QStringLiteral("<li><b>%1</b>：%2</li>")
                              .arg(item.name.toHtmlEscaped(), item.meaning.toHtmlEscaped());
        }
        guide_html += QStringLiteral("</ul>");
        html += section_html(QStringLiteral("输出解读"), guide_html);
    }

    html += section_html(QStringLiteral("常见误用"), list_html(entry.common_mistakes));

    if (!entry.dataset_id.has_value()) {
        html += section_html(QStringLiteral("演示数据"),
                             QStringLiteral("<p>本命令无需导入数据。</p>"));
    } else {
        html += section_html(QStringLiteral("演示数据"),
                             QStringLiteral("<p>可导入工作表 <code>%1</code>（数据集 %2）。</p>")
                                 .arg(current_worksheet_name_.toHtmlEscaped(),
                                      entry.dataset_id.value().toHtmlEscaped()));
    }

    return html;
}

QString LearningCenterPage::status_label(const QString& status) const
{
    if (status == QStringLiteral("implemented")) {
        return QStringLiteral("已实现菜单");
    }
    if (status == QStringLiteral("formula_reference")) {
        return QStringLiteral("公式参考（菜单可能未实现）");
    }
    if (status == QStringLiteral("partial")) {
        return QStringLiteral("部分实现");
    }
    if (status == QStringLiteral("graph_reference")) {
        return QStringLiteral("图形参考");
    }
    if (status == QStringLiteral("orchestration")) {
        return QStringLiteral("编排/流程参考");
    }
    return status;
}
