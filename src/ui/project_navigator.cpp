#include "ui/project_navigator.h"

ProjectNavigator::ProjectNavigator(QWidget* parent)
    : QTreeWidget(parent)
{
    setHeaderHidden(true);
    setRootIsDecorated(true);
    setIndentation(20);
    setMinimumWidth(220);
    setStyleSheet(QStringLiteral(
        "QTreeWidget { background: #eaf2f5; border: 0; padding: 10px 8px; color: #49636d; }"
        "QTreeWidget::item { padding: 7px 6px; border-radius: 5px; }"
        "QTreeWidget::item:hover { background: #e2eef1; }"
        "QTreeWidget::item:selected { background: #bfe7e9; color: #146f77; font-weight: 600; }"));

    auto* project = new QTreeWidgetItem(this, {QStringLiteral("DataLab 项目")});
    project->setExpanded(true);
    worksheets_ = new QTreeWidgetItem(project, {QStringLiteral("工作表")});
    analyses_ = new QTreeWidgetItem(project, {QStringLiteral("分析结果")});
    reports_ = new QTreeWidgetItem(project, {QStringLiteral("报告")});
    worksheets_->setExpanded(true);
    analyses_->setExpanded(true);
    reports_->setExpanded(true);

    connect(this, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        if (item == nullptr) {
            return;
        }
        const QString id = item->data(0, Qt::UserRole).toString();
        if (!id.isEmpty()) {
            emit analysis_activated(id);
        }
    });
}

void ProjectNavigator::set_project_name(const QString& name)
{
    if (topLevelItemCount() > 0) {
        topLevelItem(0)->setText(0, name);
    }
}

void ProjectNavigator::clear_contents()
{
    if (worksheets_ != nullptr) {
        qDeleteAll(worksheets_->takeChildren());
    }
    if (analyses_ != nullptr) {
        qDeleteAll(analyses_->takeChildren());
    }
    if (reports_ != nullptr) {
        qDeleteAll(reports_->takeChildren());
    }
}

void ProjectNavigator::add_worksheet(const QString& name)
{
    new QTreeWidgetItem(worksheets_, {name});
    worksheets_->setExpanded(true);
}

void ProjectNavigator::add_analysis(const QString& id, const QString& name)
{
    auto* item = new QTreeWidgetItem(analyses_, {name});
    item->setData(0, Qt::UserRole, id);
    analyses_->setExpanded(true);
}

void ProjectNavigator::rename_analysis(const QString& id, const QString& name)
{
    QTreeWidgetItemIterator iterator(this);
    while (*iterator != nullptr) {
        QTreeWidgetItem* item = *iterator;
        if (item->data(0, Qt::UserRole).toString() == id) {
            item->setText(0, name);
            return;
        }
        ++iterator;
    }
}

void ProjectNavigator::add_report(const QString& name)
{
    new QTreeWidgetItem(reports_, {name});
    reports_->setExpanded(true);
}
