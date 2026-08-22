#pragma once

#include "application/database/database_import_service.h"
#include "domain/database_types.h"
#include "domain/quality_types.h"

#include <QDialog>

#include <memory>
#include <optional>
#include <vector>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QStackedWidget;
class QTableView;
class QTableWidget;
class QCheckBox;
class QPushButton;
class QSpinBox;

namespace datalab::ui {

class DatabasePreviewModel;

class DatabaseImportWizard final : public QDialog {
    Q_OBJECT

public:
    explicit DatabaseImportWizard(QWidget* parent = nullptr);
    ~DatabaseImportWizard() override;

    std::optional<datalab::domain::DataTable> imported_table() const;

private slots:
    void go_next();
    void go_back();
    void test_connection();
    void refresh_objects();
    void on_object_selected();
    void run_preview();
    void run_import();
    void cancel_import();

private:
    void build_pages();
    void update_navigation();
    void show_status(const QString& text, bool error = false);
    void update_connection_fields();
    bool build_current_plan(datalab::domain::ImportPlan* plan, QString* error);
    std::vector<std::string> selected_column_names() const;
    std::string preferred_keyset_column() const;
    std::vector<std::string> preferred_keyset_columns() const;

    std::unique_ptr<datalab::application::DatabaseImportService> service_;
    std::optional<datalab::domain::DataTable> imported_table_;
    datalab::domain::ImportCancellationToken cancel_token_;
    datalab::domain::TableMetadata current_metadata_;
    datalab::domain::ObjectRef current_object_;

    QStackedWidget* stack_ = nullptr;
    QComboBox* provider_combo_ = nullptr;
    QWidget* sqlite_fields_ = nullptr;
    QWidget* postgres_fields_ = nullptr;
    QLineEdit* file_path_edit_ = nullptr;
    QLineEdit* pg_host_edit_ = nullptr;
    QSpinBox* pg_port_spin_ = nullptr;
    QLineEdit* pg_database_edit_ = nullptr;
    QLineEdit* pg_schema_edit_ = nullptr;
    QLineEdit* pg_user_edit_ = nullptr;
    QLineEdit* pg_password_edit_ = nullptr;
    QCheckBox* read_only_check_ = nullptr;
    QLabel* connection_status_ = nullptr;
    QLineEdit* object_search_ = nullptr;
    QListWidget* object_list_ = nullptr;
    QTableWidget* column_table_ = nullptr;
    QComboBox* filter_column_ = nullptr;
    QComboBox* filter_op_ = nullptr;
    QLineEdit* filter_value_ = nullptr;
    QComboBox* order_key_ = nullptr;
    QSpinBox* row_limit_ = nullptr;
    QLineEdit* worksheet_name_ = nullptr;
    QTableView* preview_view_ = nullptr;
    datalab::ui::DatabasePreviewModel* preview_model_ = nullptr;
    QLabel* preview_summary_ = nullptr;
    QLabel* result_summary_ = nullptr;
    QLabel* page_title_ = nullptr;
    QLabel* status_label_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* import_button_ = nullptr;
    QPushButton* cancel_button_ = nullptr;
};

}  // namespace datalab::ui
