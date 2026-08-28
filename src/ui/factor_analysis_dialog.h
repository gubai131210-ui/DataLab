#pragma once

#include "domain/quality_types.h"

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>

class FactorAnalysisDialog final : public QDialog {
    Q_OBJECT

public:
    explicit FactorAnalysisDialog(
        const QStringList& column_labels, QWidget* parent = nullptr);

    datalab::domain::FactorAnalysisConfiguration configuration() const;
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
    QSpinBox* factor_count_spin_ = nullptr;
    QCheckBox* kaiser_check_ = nullptr;
    QCheckBox* varimax_check_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
