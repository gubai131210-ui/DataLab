#pragma once

#include "domain/quality_types.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>

class NhppRepairableDialog final : public QDialog {
    Q_OBJECT

public:
    explicit NhppRepairableDialog(
        const QStringList& column_labels, QWidget* parent = nullptr);

    datalab::domain::NhppRepairableConfiguration configuration() const;
    bool accepted_valid() const { return accepted_valid_; }

private slots:
    void on_back();
    void on_next();
    void on_accept();

private:
    void update_nav();
    void rebuild_results_note();
    bool validate_data(QString* error) const;

    QStackedWidget* stack_ = nullptr;
    QComboBox* time_combo_ = nullptr;
    QDoubleSpinBox* t_spin_ = nullptr;
    QCheckBox* use_custom_t_ = nullptr;
    QCheckBox* duane_check_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QPlainTextEdit* results_note_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
