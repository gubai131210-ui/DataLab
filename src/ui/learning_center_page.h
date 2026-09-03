#pragma once

#include "application/learning/learning_types.h"

#include <QDialog>
#include <QVector>

class QLineEdit;
class QPushButton;
class QTextBrowser;
class QTreeWidget;

class LearningCenterPage final : public QDialog {
    Q_OBJECT

public:
    explicit LearningCenterPage(QWidget* parent = nullptr);

    void select_entry(const QString& command_id);

signals:
    void import_demo_requested(const QString& dataset_id, const QString& worksheet_name);
    void open_formula_help_requested(const QString& command_id);

private slots:
    void on_search_text_changed(const QString& text);
    void on_tree_selection_changed();
    void import_current_dataset();
    void open_formula_help();
    void export_database();

private:
    void rebuild_tree(const QString& filter);
    void show_entry(const datalab::application::learning::LearningTutorialEntry& entry);
    QString build_entry_html(const datalab::application::learning::LearningTutorialEntry& entry) const;
    QString status_label(const QString& status) const;

    QVector<datalab::application::learning::LearningTutorialEntry> entries_;
    QString load_error_;
    QLineEdit* search_edit_ = nullptr;
    QTreeWidget* tree_ = nullptr;
    QTextBrowser* detail_browser_ = nullptr;
    QPushButton* import_button_ = nullptr;
    QPushButton* formula_button_ = nullptr;
    QString current_command_id_;
    QString current_dataset_id_;
    QString current_worksheet_name_;
};
