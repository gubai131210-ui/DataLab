#pragma once

#include "domain/quality_types.h"

#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>

class ReliabilityTestPlanDialog final : public QDialog {
    Q_OBJECT

public:
    explicit ReliabilityTestPlanDialog(QWidget* parent = nullptr);

    datalab::domain::ReliabilityTestPlanConfiguration configuration() const;
    bool accepted_valid() const { return accepted_valid_; }

private slots:
    void on_back();
    void on_next();
    void on_accept();

private:
    void update_nav();
    void rebuild_results();
    bool validate_inputs(QString* error) const;

    QStackedWidget* stack_ = nullptr;
    QDoubleSpinBox* beta_spin_ = nullptr;
    QDoubleSpinBox* r_spin_ = nullptr;
    QDoubleSpinBox* cl_spin_ = nullptr;
    QDoubleSpinBox* t0_spin_ = nullptr;
    QDoubleSpinBox* tm_spin_ = nullptr;
    QSpinBox* allowed_spin_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QPlainTextEdit* results_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
