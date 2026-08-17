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
    void run_descriptive();
    void run_normality_test();
    void run_correlation();
    void run_one_sample_t();
    void run_two_sample_t();
    void run_one_way_anova();
    void run_paired_t();
    void run_regression();
    void run_two_proportions();
    void run_chi_square();
    void run_mann_whitney();
    void run_wilcoxon_signed_rank();
    void run_kruskal_wallis();
    void run_box_cox();
    void run_gage_rr();
    void run_msa_type1();
    void run_reliability();
    void run_t_power();
    void run_imr();
    void run_xbar_r();
    void run_xbar_s();
    void run_p_chart();
    void run_np_chart();
    void run_c_chart();
    void run_u_chart();
    void run_laney_p_chart();
    void run_laney_u_chart();
    void run_ewma();
    void run_cusum();
    void run_time_series_smoothing();
    void run_arima();
    void run_two_factor_anova();
    void run_logistic_regression();
    void run_variance_test();
    void run_time_series_decomposition();
    void run_doe_factorial();
    void run_nested_gage_rr();
    void run_attribute_agreement();
    void run_seasonal_forecasting();
    void run_pca();
    void run_capability();
    void run_capability_sixpack();
    void run_histogram();
    void run_boxplot();
    void run_pareto();
    void run_analysis(const QString& id);
    void copy_selection();
    void cut_selection();
    void clear_selection();
    void paste_clipboard();
    bool ensure_data();
    QStringList column_labels() const;
    datalab::domain::AnalysisConfiguration base_configuration() const;
    void publish_page(const datalab::domain::OutputPage& page);
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
