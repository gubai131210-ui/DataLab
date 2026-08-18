#pragma once

#include "ui/analysis_commands.h"

#include <QDialog>
#include <QStringList>

#include <functional>
#include <optional>
#include <vector>

class QListWidget;
class QLineEdit;
class QFormLayout;
class QLabel;
class QWidget;
class QToolButton;

class AnalysisSetupDialog final : public QDialog {
    Q_OBJECT

public:
    AnalysisSetupDialog(
        const QString& title,
        const QStringList& column_labels,
        QWidget* parent = nullptr,
        const QString& icon_resource = QStringLiteral(":/icons/app-mark.svg"),
        const std::vector<datalab::domain::ColumnType>& column_types = {});

    void add_role(const QString& id, const QString& label, bool multi, bool optional);
    void add_role(const analysis_commands::RoleSpec& spec);
    QWidget* add_input(const analysis_commands::InputSpec& spec);
    QWidget* add_line_edit(const QString& id, const QString& label, const QString& placeholder = {});
    QList<int> role_indices(const QString& id) const;
    int first_role_index(const QString& id) const;
    QString line_text(const QString& id) const;
    std::optional<double> line_number(const QString& id) const;
    std::optional<int> line_int(const QString& id) const;
    void set_accept_validator(std::function<bool(QString*, QString*)> validator);
    void set_field_error(const QString& id, const QString& message);
    void clear_errors();
    void reset_defaults();

private:
    void select_into_role();
    void remove_from_role();
    void set_active_role(QListWidget* list);
    QListWidget* role_list(const QString& id) const;
    void validate_and_accept();

    QStringList column_labels_;
    std::vector<datalab::domain::ColumnType> column_types_;
    QListWidget* available_ = nullptr;
    QFormLayout* roles_layout_ = nullptr;
    QFormLayout* advanced_layout_ = nullptr;
    QWidget* advanced_panel_ = nullptr;
    QToolButton* advanced_toggle_ = nullptr;
    QListWidget* active_role_ = nullptr;
    QLabel* error_banner_ = nullptr;
    std::function<bool(QString*, QString*)> accept_validator_;
};
