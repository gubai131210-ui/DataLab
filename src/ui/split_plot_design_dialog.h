#pragma once

#include "domain/quality_types.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>

class SplitPlotDesignDialog final : public QDialog {
    Q_OBJECT

public:
    explicit SplitPlotDesignDialog(QWidget* parent = nullptr);

    datalab::domain::SplitPlotDesignConfiguration configuration() const;
    bool accepted_valid() const { return accepted_valid_; }

private slots:
    void on_back();
    void on_next();
    void on_accept();

private:
    void update_nav();
    void rebuild_preview();
    bool validate_input(QString* error) const;

    QStackedWidget* stack_ = nullptr;
    QSpinBox* factor_count_spin_ = nullptr;
    QLineEdit* factor_name_1_ = nullptr;
    QLineEdit* factor_name_2_ = nullptr;
    QLineEdit* factor_name_3_ = nullptr;
    QLineEdit* factor_name_4_ = nullptr;
    QComboBox* htc_combo_ = nullptr;
    QSpinBox* replicate_spin_ = nullptr;
    QCheckBox* randomize_check_ = nullptr;
    QPlainTextEdit* design_note_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
