#pragma once

#include <QTreeWidget>

class ProjectNavigator final : public QTreeWidget {
    Q_OBJECT

public:
    explicit ProjectNavigator(QWidget* parent = nullptr);

    void set_project_name(const QString& name);
    void clear_contents();
    void add_worksheet(const QString& name);
    void add_analysis(const QString& id, const QString& name);
    void rename_analysis(const QString& id, const QString& name);
    void add_report(const QString& name);

signals:
    void analysis_activated(const QString& id);

private:
    QTreeWidgetItem* worksheets_ = nullptr;
    QTreeWidgetItem* analyses_ = nullptr;
    QTreeWidgetItem* reports_ = nullptr;
};
