#pragma once

#include "domain/quality_types.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>

class ExpandedGageUnbalancedDialog final : public QDialog {
    Q_OBJECT

public:
    explicit ExpandedGageUnbalancedDialog(
        const QStringList& column_labels, QWidget* parent = nullptr);

    datalab::domain::ExpandedGageUnbalancedConfiguration configuration() const;
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
    QComboBox* measurement_combo_ = nullptr;
    QComboBox* part_combo_ = nullptr;
    QComboBox* operator_combo_ = nullptr;
    QComboBox* additional_combo_ = nullptr;
    QCheckBox* additional_check_ = nullptr;
    QCheckBox* part_random_check_ = nullptr;
    QCheckBox* operator_random_check_ = nullptr;
    QCheckBox* additional_random_check_ = nullptr;
    QDoubleSpinBox* tolerance_spin_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
