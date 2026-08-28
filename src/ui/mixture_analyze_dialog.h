#pragma once

#include "domain/quality_types.h"

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>

class MixtureAnalyzeDialog final : public QDialog {
    Q_OBJECT

public:
    explicit MixtureAnalyzeDialog(
        const QStringList& column_labels, QWidget* parent = nullptr);

    datalab::domain::MixtureAnalyzeConfiguration configuration() const;
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
    QListWidget* component_list_ = nullptr;
    QListWidget* response_list_ = nullptr;
    QComboBox* model_combo_ = nullptr;
    QPlainTextEdit* method_note_ = nullptr;
    QPlainTextEdit* preview_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* run_button_ = nullptr;
    bool accepted_valid_ = false;
};
