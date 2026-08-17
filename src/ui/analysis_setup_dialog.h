#pragma once

#include <QDialog>
#include <QStringList>

#include <optional>
#include <vector>

class QListWidget;
class QLineEdit;
class QFormLayout;
class QWidget;

class AnalysisSetupDialog final : public QDialog {
    Q_OBJECT

public:
    AnalysisSetupDialog(
        const QString& title,
        const QStringList& column_labels,
        QWidget* parent = nullptr);

    void add_role(const QString& id, const QString& label, bool multi, bool optional);
    QWidget* add_line_edit(const QString& id, const QString& label, const QString& placeholder = {});
    QList<int> role_indices(const QString& id) const;
    int first_role_index(const QString& id) const;
    QString line_text(const QString& id) const;
    std::optional<double> line_number(const QString& id) const;
    std::optional<int> line_int(const QString& id) const;

private:
    void select_into_role();
    void set_active_role(QListWidget* list);
    QListWidget* role_list(const QString& id) const;

    QStringList column_labels_;
    QListWidget* available_ = nullptr;
    QFormLayout* roles_layout_ = nullptr;
    QListWidget* active_role_ = nullptr;
};
