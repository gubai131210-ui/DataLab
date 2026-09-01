#pragma once

#include "domain/quality_types.h"

#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>

class NonlinearRegressionDialog final : public QDialog {
    Q_OBJECT

public:
    explicit NonlinearRegressionDialog(
        const QStringList& column_labels, QWidget* parent = nullptr);

    datalab::domain::NonlinearRegressionConfiguration configuration() const;
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
    QComboBox* response_combo_ = nullptr;
    QComboBox* predictor_combo_ = nullptr;
    QComboBox* model_combo_ = nullptr;
    QDoubleSpinBox* start_a_spin_ = nullptr;
    QDoubleSpinBox* start_b_spin_ = nullptr;
    QComboBox* algorithm_combo_ = nullptr;
    QSpinBox* max_iter_spin_ = nullptr;
    QDoubleSpinBox* tolerance_spin_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
