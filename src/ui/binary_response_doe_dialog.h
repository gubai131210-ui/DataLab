#pragma once

#include "domain/quality_types.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>

class BinaryResponseDoeDialog final : public QDialog {
    Q_OBJECT

public:
    explicit BinaryResponseDoeDialog(
        const QStringList& column_labels, QWidget* parent = nullptr);

    datalab::domain::BinaryResponseDoeConfiguration configuration() const;
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
    QListWidget* factor_list_ = nullptr;
    QRadioButton* events_trials_radio_ = nullptr;
    QRadioButton* binary_radio_ = nullptr;
    QComboBox* events_combo_ = nullptr;
    QComboBox* trials_combo_ = nullptr;
    QComboBox* binary_combo_ = nullptr;
    QCheckBox* interaction_check_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
