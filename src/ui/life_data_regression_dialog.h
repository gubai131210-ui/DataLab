#pragma once

#include "domain/quality_types.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>

class LifeDataRegressionDialog final : public QDialog {
    Q_OBJECT

public:
    explicit LifeDataRegressionDialog(
        const QStringList& column_labels, QWidget* parent = nullptr);

    datalab::domain::LifeDataRegressionConfiguration configuration() const;
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
    QComboBox* time_combo_ = nullptr;
    QComboBox* censor_combo_ = nullptr;
    QListWidget* covariate_list_ = nullptr;
    QComboBox* distribution_combo_ = nullptr;
    QCheckBox* percentile_1_check_ = nullptr;
    QCheckBox* percentile_5_check_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
