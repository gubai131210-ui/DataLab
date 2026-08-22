#pragma once

#include "ui/algorithm_help_types.h"

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QTreeWidget>

#include <QString>

class FormulaRegistryDialog final : public QDialog {
    Q_OBJECT

public:
    explicit FormulaRegistryDialog(QWidget* parent = nullptr);

    void select_entry(const QString& id);

private slots:
    void on_search_text_changed(const QString& text);
    void on_tree_selection_changed();
    void copy_formula_plain_text();
    void copy_entry_id();
    void open_selected_reference();

private:
    void rebuild_tree(const QString& filter);
    void show_entry(const AlgorithmHelpEntry& entry);
    QString build_formula_html(const AlgorithmHelpEntry& entry) const;

    AlgorithmHelpCatalog catalog_;
    QString load_error_;
    QLineEdit* search_edit_ = nullptr;
    QTreeWidget* tree_ = nullptr;
    QTextBrowser* detail_browser_ = nullptr;
    QPushButton* copy_formula_button_ = nullptr;
    QPushButton* copy_id_button_ = nullptr;
    QPushButton* open_reference_button_ = nullptr;
};
