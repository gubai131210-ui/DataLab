#pragma once

#include "domain/quality_types.h"

#include <QComboBox>
#include <QDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>

class MixedEffectsRemlDialog final : public QDialog {
    Q_OBJECT

public:
    explicit MixedEffectsRemlDialog(
        const QStringList& column_labels, QWidget* parent = nullptr);

    datalab::domain::MixedEffectsRemlConfiguration configuration() const;
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
    QComboBox* random_combo_ = nullptr;
    QComboBox* fixed_a_combo_ = nullptr;
    QComboBox* fixed_b_combo_ = nullptr;
    QComboBox* covariate_combo_ = nullptr;
    QComboBox* method_combo_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
