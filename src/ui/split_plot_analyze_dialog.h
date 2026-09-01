#pragma once

#include "domain/quality_types.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>

class SplitPlotAnalyzeDialog final : public QDialog {
    Q_OBJECT

public:
    explicit SplitPlotAnalyzeDialog(
        const QStringList& column_labels, QWidget* parent = nullptr);

    datalab::domain::SplitPlotAnalyzeConfiguration configuration() const;
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
    QComboBox* htc_combo_ = nullptr;
    QComboBox* etc_a_combo_ = nullptr;
    QComboBox* etc_b_combo_ = nullptr;
    QComboBox* wp_combo_ = nullptr;
    QCheckBox* htc_etc_check_ = nullptr;
    QCheckBox* etc_interaction_check_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
