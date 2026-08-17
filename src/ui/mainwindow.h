#pragma once

#include <QMainWindow>

#include "domain/quality_types.h"

class CommandRegistry;
class OutputWorkspace;
class ProjectNavigator;
class WorksheetView;
class WorksheetModel;
class QDockWidget;
class QLineEdit;
class QUndoStack;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void create_commands();
    void create_layout();
    void new_project();
    void open_project();
    void import_data();
    void save_project();
    void export_pdf();
    void exclude_selected_row();
    void clear_exclusions();
    void display_table();
    void run_from_spec(const QString& id);
    void copy_selection();
    void cut_selection();
    void clear_selection();
    void paste_clipboard();
    bool ensure_data();
    QStringList column_labels() const;
    datalab::domain::AnalysisConfiguration base_configuration() const;
    void publish_page(const datalab::domain::OutputPage& page);
    void restore_cleaning_operations(
        const std::vector<datalab::domain::CleaningOperation>& operations);
    std::vector<std::size_t> excluded_rows() const;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void push_table_change(
        const datalab::domain::DataTable& before,
        const datalab::domain::DataTable& after,
        const QString& text,
        bool already_applied = false);

    datalab::domain::DataTable table_;
    std::vector<datalab::domain::CleaningOperation> cleaning_operations_;
    WorksheetView* data_table_ = nullptr;
    OutputWorkspace* output_workspace_ = nullptr;
    ProjectNavigator* navigator_ = nullptr;
    QDockWidget* navigator_dock_ = nullptr;
    QDockWidget* context_dock_ = nullptr;
    WorksheetModel* worksheet_model_ = nullptr;
    QLineEdit* formula_bar_ = nullptr;
    CommandRegistry* commands_ = nullptr;
    QUndoStack* undo_stack_ = nullptr;
};
