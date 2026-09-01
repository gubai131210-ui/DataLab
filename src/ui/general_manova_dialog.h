#pragma once

#include "domain/quality_types.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>

class GeneralManovaDialog final : public QDialog {
    Q_OBJECT

public:
    explicit GeneralManovaDialog(
        const QStringList& column_labels, QWidget* parent = nullptr);

    datalab::domain::GeneralManovaConfiguration configuration() const;
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
    QListWidget* response_list_ = nullptr;
    QComboBox* factor_a_combo_ = nullptr;
    QComboBox* factor_b_combo_ = nullptr;
    QComboBox* covariate_combo_ = nullptr;
    QCheckBox* interaction_check_ = nullptr;
    QCheckBox* wilks_check_ = nullptr;
    QCheckBox* pillai_check_ = nullptr;
    QCheckBox* lh_check_ = nullptr;
    QCheckBox* roy_check_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
