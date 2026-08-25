#pragma once

#include "domain/quality_types.h"

#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>

class MixtureDesignDialog final : public QDialog {
    Q_OBJECT

public:
    explicit MixtureDesignDialog(QWidget* parent = nullptr);

    datalab::domain::MixtureDesignConfiguration configuration() const;
    bool accepted_valid() const { return accepted_valid_; }

private slots:
    void on_back();
    void on_next();
    void on_accept();

private:
    void update_nav();
    void rebuild_matrix_preview();
    void rebuild_confirm();
    bool validate_options(QString* error) const;

    QStackedWidget* stack_ = nullptr;
    QSpinBox* q_spin_ = nullptr;
    QLineEdit* names_edit_ = nullptr;
    QCheckBox* randomize_check_ = nullptr;
    QSpinBox* seed_spin_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QTableWidget* matrix_preview_ = nullptr;
    QPlainTextEdit* confirm_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
