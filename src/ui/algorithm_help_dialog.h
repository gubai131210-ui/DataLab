#pragma once

#include "ui/algorithm_help_types.h"

#include <QDialog>

class QLineEdit;
class QTreeWidget;
class QTextBrowser;
class QPushButton;
class QTabWidget;

class AlgorithmHelpDialog final : public QDialog {
    Q_OBJECT

public:
    explicit AlgorithmHelpDialog(QWidget* parent = nullptr);

    void select_entry(const QString& id);

private slots:
    void on_search_text_changed(const QString& text);
    void on_tree_selection_changed();
    void copy_formula_plain_text();
    void copy_entry_summary();
    void open_selected_reference();

private:
    void rebuild_tree(const QString& filter);
    void show_entry(const AlgorithmHelpEntry& entry);
    QString build_entry_html(const AlgorithmHelpEntry& entry) const;
    QString build_formula_sources_html(const AlgorithmHelpEntry& entry) const;
    QString status_label(const QString& status) const;

    AlgorithmHelpCatalog catalog_;
    QString load_error_;
    QLineEdit* search_edit_ = nullptr;
    QTreeWidget* tree_ = nullptr;
    QTabWidget* detail_tabs_ = nullptr;
    QTextBrowser* detail_browser_ = nullptr;
    QTextBrowser* formula_browser_ = nullptr;
    QPushButton* copy_formula_button_ = nullptr;
    QPushButton* copy_summary_button_ = nullptr;
    QPushButton* open_reference_button_ = nullptr;
    QString current_formula_plain_text_;
    QString current_reference_url_;
};
