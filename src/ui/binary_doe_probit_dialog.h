#pragma once

#include "domain/quality_types.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>

class BinaryDoeProbitDialog final : public QDialog {
    Q_OBJECT

public:
    explicit BinaryDoeProbitDialog(
        const QStringList& column_labels, QWidget* parent = nullptr);

    datalab::domain::BinaryDoeProbitConfiguration configuration() const;
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
    QListWidget* factor_list_ = nullptr;
    QComboBox* events_combo_ = nullptr;
    QComboBox* trials_combo_ = nullptr;
    QComboBox* binary_combo_ = nullptr;
    QComboBox* link_combo_ = nullptr;
    QCheckBox* interaction_check_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
