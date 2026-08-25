#pragma once

#include "domain/quality_types.h"

#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>

#include <vector>

class FormulaSubstitutionDialog final : public QDialog {
    Q_OBJECT

public:
    explicit FormulaSubstitutionDialog(
        datalab::domain::OutputPage page, QWidget* parent = nullptr);

signals:
    void open_in_formula_registry(const QString& command_id);

private slots:
    void on_back();
    void on_next();
    void on_trace_selected();
    void on_open_registry();

private:
    void rebuild_bindings();
    void rebuild_preview();
    void rebuild_source();
    void update_nav();
    int selected_trace_index() const;

    datalab::domain::OutputPage page_;
    QStackedWidget* stack_ = nullptr;
    QListWidget* list_ = nullptr;
    QTableWidget* bindings_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QLabel* result_label_ = nullptr;
    QTableWidget* steps_table_ = nullptr;
    QLabel* evidence_label_ = nullptr;
    QLabel* url_label_ = nullptr;
    QPushButton* open_registry_button_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
};
