#pragma once

#include "domain/quality_types.h"

#include <QCheckBox>
#include <QDialog>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>

class MultipleCorrespondenceDialog final : public QDialog {
    Q_OBJECT

public:
    explicit MultipleCorrespondenceDialog(
        const QStringList& column_labels, QWidget* parent = nullptr);

    datalab::domain::MultipleCorrespondenceConfiguration configuration() const;
    bool accepted_valid() const { return accepted_valid_; }

private slots:
    void on_back();
    void on_next();
    void on_accept();

private:
    void update_nav();
    void rebuild_preview();
    bool validate_columns(QString* error) const;

    QStackedWidget* stack_ = nullptr;
    QListWidget* variable_list_ = nullptr;
    QSpinBox* component_spin_ = nullptr;
    QCheckBox* col_contrib_check_ = nullptr;
    QPlainTextEdit* output_note_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
