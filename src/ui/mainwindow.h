#pragma once

#include <QMainWindow>

#include <QList>
#include <string>

#include "application/learning/worksheet_registry.h"
#include "application/license/license_service.h"
#include "domain/quality_types.h"

class AlgorithmHelpDialog;
class FormulaRegistryDialog;
class LearningCenterPage;
class CommandRegistry;
class OutputWorkspace;
class ProjectNavigator;
class WorksheetView;
class WorksheetModel;
namespace datalab::ui { class WorksheetSortFilterProxyModel; }
class QDockWidget;
class QLabel;
class QLineEdit;
class QUndoStack;
class QAction;

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
    void import_database();
    void open_mes_tools();
    void open_usage_permission();
    void open_learning_center();
    void import_learning_dataset(const QString& dataset_id, const QString& worksheet_name);
    void save_current_worksheet_to_registry();
    void activate_worksheet(const QString& name);
    void open_command_wizard();
    void apply_license_gate();
    void refresh_license_status();
    void save_project();
    void export_pdf();
    void exclude_selected_row();
    void clear_exclusions();
    void hide_selected_row();
    void clear_hidden_rows();
    void display_table();
    void reset_worksheet_sort_to_import_order();
    void run_from_spec(const QString& id);
    void copy_selection();
    void copy_chart();
    void cut_selection();
    void clear_selection();
    void clear_selected_columns();
    void remove_selected_columns();
    QList<int> selected_source_columns() const;
    bool selection_covers_whole_columns() const;
    void paste_clipboard();
    bool ensure_data();
    QStringList column_labels() const;
    datalab::domain::AnalysisConfiguration base_configuration() const;
    void publish_page(const datalab::domain::OutputPage& page);
    void restore_cleaning_operations(
        const std::vector<datalab::domain::CleaningOperation>& operations);
    std::vector<std::size_t> excluded_rows() const;
    std::vector<std::size_t> hidden_rows() const;
    void refresh_context_dock();
    bool confirm_discard_output();
    bool eventFilter(QObject* watched, QEvent* event) override;
    void push_table_change(
        const datalab::domain::DataTable& before,
        const datalab::domain::DataTable& after,
        const QString& text,
        bool already_applied = false);

    datalab::domain::DataTable table_;
    datalab::application::learning::WorksheetRegistry worksheet_registry_;
    std::vector<datalab::domain::CleaningOperation> cleaning_operations_;
    WorksheetView* data_table_ = nullptr;
    OutputWorkspace* output_workspace_ = nullptr;
    ProjectNavigator* navigator_ = nullptr;
    QDockWidget* navigator_dock_ = nullptr;
    QDockWidget* context_dock_ = nullptr;
    QLabel* context_status_ = nullptr;
    QLabel* context_detail_ = nullptr;
    QLabel* context_next_ = nullptr;
    QLabel* cell_address_ = nullptr;
    WorksheetModel* worksheet_model_ = nullptr;
    datalab::ui::WorksheetSortFilterProxyModel* worksheet_sort_proxy_ = nullptr;
    QLineEdit* worksheet_filter_edit_ = nullptr;
    CommandRegistry* commands_ = nullptr;
    QUndoStack* undo_stack_ = nullptr;
    bool import_in_progress_ = false;
    bool suppress_table_edit_undo_ = false;
    AlgorithmHelpDialog* algorithm_help_dialog_ = nullptr;
    FormulaRegistryDialog* formula_registry_dialog_ = nullptr;
    LearningCenterPage* learning_center_page_ = nullptr;
    datalab::application::license::LicenseService license_service_;
    QAction* usage_permission_action_ = nullptr;
    QAction* mes_tools_action_ = nullptr;
    bool license_restricted_ = false;
};
