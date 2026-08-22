#include "ui/database_import_wizard.h"

#include "infrastructure/database/database_provider_bootstrap.h"
#include "ui/database_preview_model.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace datalab::ui {
namespace {

QString object_kind_label(datalab::domain::DatabaseObjectKind kind)
{
    return kind == datalab::domain::DatabaseObjectKind::view
        ? QStringLiteral("视图")
        : QStringLiteral("表");
}

}  // namespace

DatabaseImportWizard::DatabaseImportWizard(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("数据库导入"));
    setMinimumSize(780, 560);
    datalab::infrastructure::ensure_default_database_providers_registered();
    service_ = std::make_unique<datalab::application::DatabaseImportService>(
        datalab::domain::default_database_provider_registry());
    build_pages();
    update_navigation();
}

DatabaseImportWizard::~DatabaseImportWizard()
{
    if (service_) {
        service_->disconnect();
    }
}

std::optional<datalab::domain::DataTable> DatabaseImportWizard::imported_table() const
{
    return imported_table_;
}

void DatabaseImportWizard::build_pages()
{
    auto* root = new QVBoxLayout(this);
    page_title_ = new QLabel(this);
    page_title_->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 600;"));
    root->addWidget(page_title_);
    stack_ = new QStackedWidget(this);

    // 0 connection
    auto* connection_page = new QWidget(stack_);
    auto* connection_layout = new QVBoxLayout(connection_page);
    connection_layout->addWidget(new QLabel(QStringLiteral("选择 Provider"), connection_page));
    provider_combo_ = new QComboBox(connection_page);
    for (const auto& descriptor : service_->list_providers()) {
        const QString label = descriptor.available
            ? QString::fromStdString(descriptor.display_name)
            : QStringLiteral("%1（不可用）")
                  .arg(QString::fromStdString(descriptor.display_name));
        provider_combo_->addItem(label, QString::fromStdString(descriptor.id));
        provider_combo_->setItemData(
            provider_combo_->count() - 1,
            QString::fromStdString(descriptor.unavailable_reason),
            Qt::ToolTipRole);
    }
    connection_layout->addWidget(provider_combo_);

    sqlite_fields_ = new QWidget(connection_page);
    auto* sqlite_layout = new QVBoxLayout(sqlite_fields_);
    sqlite_layout->setContentsMargins(0, 0, 0, 0);
    sqlite_layout->addWidget(new QLabel(QStringLiteral("SQLite 文件路径"), sqlite_fields_));
    auto* file_row = new QHBoxLayout();
    file_path_edit_ = new QLineEdit(sqlite_fields_);
    auto* browse = new QPushButton(QStringLiteral("浏览…"), sqlite_fields_);
    file_row->addWidget(file_path_edit_);
    file_row->addWidget(browse);
    sqlite_layout->addLayout(file_row);
    connection_layout->addWidget(sqlite_fields_);

    postgres_fields_ = new QWidget(connection_page);
    auto* pg_layout = new QVBoxLayout(postgres_fields_);
    pg_layout->setContentsMargins(0, 0, 0, 0);
    pg_host_edit_ = new QLineEdit(QStringLiteral("127.0.0.1"), postgres_fields_);
    pg_port_spin_ = new QSpinBox(postgres_fields_);
    pg_port_spin_->setRange(1, 65535);
    pg_port_spin_->setValue(5432);
    pg_database_edit_ = new QLineEdit(postgres_fields_);
    pg_schema_edit_ = new QLineEdit(QStringLiteral("public"), postgres_fields_);
    pg_user_edit_ = new QLineEdit(postgres_fields_);
    pg_password_edit_ = new QLineEdit(postgres_fields_);
    pg_password_edit_->setEchoMode(QLineEdit::Password);
    pg_layout->addWidget(new QLabel(QStringLiteral("Host"), postgres_fields_));
    pg_layout->addWidget(pg_host_edit_);
    pg_layout->addWidget(new QLabel(QStringLiteral("Port"), postgres_fields_));
    pg_layout->addWidget(pg_port_spin_);
    pg_layout->addWidget(new QLabel(QStringLiteral("Database"), postgres_fields_));
    pg_layout->addWidget(pg_database_edit_);
    pg_layout->addWidget(new QLabel(QStringLiteral("Schema"), postgres_fields_));
    pg_layout->addWidget(pg_schema_edit_);
    pg_layout->addWidget(new QLabel(QStringLiteral("Username"), postgres_fields_));
    pg_layout->addWidget(pg_user_edit_);
    pg_layout->addWidget(new QLabel(QStringLiteral("Password（仅内存，不写入 ImportPlan/项目）"), postgres_fields_));
    pg_layout->addWidget(pg_password_edit_);
    connection_layout->addWidget(postgres_fields_);
    postgres_fields_->setVisible(false);

    read_only_check_ = new QCheckBox(QStringLiteral("只读连接"), connection_page);
    read_only_check_->setChecked(true);
    connection_layout->addWidget(read_only_check_);
    auto* test_button = new QPushButton(QStringLiteral("测试连接"), connection_page);
    connection_layout->addWidget(test_button);
    connection_status_ = new QLabel(QStringLiteral("尚未连接"), connection_page);
    connection_status_->setWordWrap(true);
    connection_layout->addWidget(connection_status_);
    connection_layout->addStretch();
    stack_->addWidget(connection_page);
    connect(browse, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("选择 SQLite 数据库"), QString(),
            QStringLiteral("SQLite (*.db *.sqlite *.sqlite3);;All Files (*.*)"));
        if (!path.isEmpty()) {
            file_path_edit_->setText(path);
        }
    });
    connect(test_button, &QPushButton::clicked, this, &DatabaseImportWizard::test_connection);
    connect(provider_combo_, &QComboBox::currentIndexChanged, this,
            [this](int) { update_connection_fields(); });
    update_connection_fields();

    // 1 objects
    auto* objects_page = new QWidget(stack_);
    auto* objects_layout = new QVBoxLayout(objects_page);
    objects_layout->addWidget(new QLabel(QStringLiteral("搜索表/视图"), objects_page));
    object_search_ = new QLineEdit(objects_page);
    objects_layout->addWidget(object_search_);
    object_list_ = new QListWidget(objects_page);
    objects_layout->addWidget(object_list_);
    auto* refresh = new QPushButton(QStringLiteral("刷新对象"), objects_page);
    objects_layout->addWidget(refresh);
    stack_->addWidget(objects_page);
    connect(refresh, &QPushButton::clicked, this, &DatabaseImportWizard::refresh_objects);
    connect(object_search_, &QLineEdit::textChanged, this, &DatabaseImportWizard::refresh_objects);
    connect(object_list_, &QListWidget::currentRowChanged,
            this, &DatabaseImportWizard::on_object_selected);

    // 2 columns & filter
    auto* columns_page = new QWidget(stack_);
    auto* columns_layout = new QVBoxLayout(columns_page);
    columns_layout->addWidget(new QLabel(
        QStringLiteral("勾选要导入的列（顺序=表格顺序）；可添加一条结构化过滤"), columns_page));
    column_table_ = new QTableWidget(columns_page);
    column_table_->setColumnCount(6);
    column_table_->setHorizontalHeaderLabels({
        QStringLiteral("导入"), QStringLiteral("列名"), QStringLiteral("类型"),
        QStringLiteral("可空"), QStringLiteral("主键"), QStringLiteral("隐藏")});
    column_table_->horizontalHeader()->setStretchLastSection(true);
    column_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    columns_layout->addWidget(column_table_);

    auto* filter_row = new QHBoxLayout();
    filter_column_ = new QComboBox(columns_page);
    filter_op_ = new QComboBox(columns_page);
    filter_op_->addItems({
        QStringLiteral("(无)"), QStringLiteral("="), QStringLiteral("!="),
        QStringLiteral("<"), QStringLiteral("<="), QStringLiteral(">"),
        QStringLiteral(">="), QStringLiteral("LIKE"),
        QStringLiteral("IS NULL"), QStringLiteral("IS NOT NULL")});
    filter_value_ = new QLineEdit(columns_page);
    filter_row->addWidget(new QLabel(QStringLiteral("过滤"), columns_page));
    filter_row->addWidget(filter_column_, 1);
    filter_row->addWidget(filter_op_);
    filter_row->addWidget(filter_value_, 1);
    columns_layout->addLayout(filter_row);

    auto* options_row = new QHBoxLayout();
    order_key_ = new QComboBox(columns_page);
    row_limit_ = new QSpinBox(columns_page);
    row_limit_->setRange(0, 100000000);
    row_limit_->setSpecialValueText(QStringLiteral("不限制"));
    row_limit_->setValue(0);
    worksheet_name_ = new QLineEdit(columns_page);
    options_row->addWidget(new QLabel(QStringLiteral("排序键"), columns_page));
    options_row->addWidget(order_key_, 1);
    options_row->addWidget(new QLabel(QStringLiteral("行数上限"), columns_page));
    options_row->addWidget(row_limit_);
    options_row->addWidget(new QLabel(QStringLiteral("工作表名"), columns_page));
    options_row->addWidget(worksheet_name_, 1);
    columns_layout->addLayout(options_row);
    stack_->addWidget(columns_page);

    // 3 preview
    auto* preview_page = new QWidget(stack_);
    auto* preview_layout = new QVBoxLayout(preview_page);
    preview_summary_ = new QLabel(
        QStringLiteral("点击“刷新预览”加载有限样本；预览限制不影响最终 ImportPlan。"),
        preview_page);
    preview_summary_->setWordWrap(true);
    preview_layout->addWidget(preview_summary_);
    preview_model_ = new DatabasePreviewModel(preview_page);
    preview_view_ = new QTableView(preview_page);
    preview_view_->setModel(preview_model_);
    preview_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    preview_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    preview_layout->addWidget(preview_view_);
    auto* preview_button = new QPushButton(QStringLiteral("刷新预览"), preview_page);
    preview_layout->addWidget(preview_button);
    stack_->addWidget(preview_page);
    connect(preview_button, &QPushButton::clicked, this, &DatabaseImportWizard::run_preview);

    // 4 result
    auto* result_page = new QWidget(stack_);
    auto* result_layout = new QVBoxLayout(result_page);
    result_summary_ = new QLabel(
        QStringLiteral("确认后执行导入。失败或取消不会留下半份工作表。"), result_page);
    result_summary_->setWordWrap(true);
    result_layout->addWidget(result_summary_);
    import_button_ = new QPushButton(QStringLiteral("导入到工作表"), result_page);
    cancel_button_ = new QPushButton(QStringLiteral("取消导入"), result_page);
    cancel_button_->setEnabled(false);
    result_layout->addWidget(import_button_);
    result_layout->addWidget(cancel_button_);
    result_layout->addStretch();
    stack_->addWidget(result_page);
    connect(import_button_, &QPushButton::clicked, this, &DatabaseImportWizard::run_import);
    connect(cancel_button_, &QPushButton::clicked, this, &DatabaseImportWizard::cancel_import);

    root->addWidget(stack_, 1);
    status_label_ = new QLabel(this);
    status_label_->setWordWrap(true);
    root->addWidget(status_label_);

    auto* nav = new QHBoxLayout();
    back_button_ = new QPushButton(QStringLiteral("上一步"), this);
    next_button_ = new QPushButton(QStringLiteral("下一步"), this);
    auto* close_button = new QPushButton(QStringLiteral("关闭"), this);
    nav->addWidget(back_button_);
    nav->addStretch();
    nav->addWidget(next_button_);
    nav->addWidget(close_button);
    root->addLayout(nav);
    connect(back_button_, &QPushButton::clicked, this, &DatabaseImportWizard::go_back);
    connect(next_button_, &QPushButton::clicked, this, &DatabaseImportWizard::go_next);
    connect(close_button, &QPushButton::clicked, this, &QDialog::reject);
}

void DatabaseImportWizard::update_navigation()
{
    const int index = stack_->currentIndex();
    static const char* titles[] = {
        "1. 来源与连接",
        "2. 数据对象",
        "3. 列与过滤",
        "4. 预览与质量检查",
        "5. 导入结果"};
    page_title_->setText(QString::fromUtf8(titles[index]));
    back_button_->setEnabled(index > 0);
    next_button_->setEnabled(index < stack_->count() - 1);
}

void DatabaseImportWizard::show_status(const QString& text, bool error)
{
    status_label_->setText(text);
    status_label_->setStyleSheet(error
        ? QStringLiteral("color: #b71c1c;")
        : QStringLiteral("color: #243b44;"));
}

std::vector<std::string> DatabaseImportWizard::selected_column_names() const
{
    std::vector<std::string> names;
    for (int row = 0; row < column_table_->rowCount(); ++row) {
        auto* check = qobject_cast<QCheckBox*>(column_table_->cellWidget(row, 0));
        if (check != nullptr && check->isChecked()) {
            names.push_back(column_table_->item(row, 1)->text().toStdString());
        }
    }
    return names;
}

bool DatabaseImportWizard::build_current_plan(
    datalab::domain::ImportPlan* plan,
    QString* error)
{
    if (plan == nullptr) {
        return false;
    }
    if (current_object_.name.empty()) {
        if (error != nullptr) {
            *error = QStringLiteral("请先选择数据对象。");
        }
        return false;
    }
    const auto columns = selected_column_names();
    if (columns.empty()) {
        if (error != nullptr) {
            *error = QStringLiteral("请至少勾选一列。");
        }
        return false;
    }

    std::vector<datalab::domain::StructuredFilter> filters;
    if (filter_op_->currentIndex() > 0 && !filter_column_->currentText().isEmpty()) {
        datalab::domain::StructuredFilter filter;
        filter.column = filter_column_->currentText().toStdString();
        filter.op = filter_op_->currentText().toStdString();
        filter.value = filter_value_->text().toStdString();
        filters.push_back(filter);
    }

    std::optional<std::uint64_t> limit;
    if (row_limit_->value() > 0) {
        limit = static_cast<std::uint64_t>(row_limit_->value());
    }

    *plan = service_->build_plan(
        current_object_,
        columns,
        filters,
        order_key_->currentData().toString().toStdString(),
        limit,
        datalab::domain::DatabaseNullPolicy::keep_null,
        worksheet_name_->text().trimmed().toStdString());

    const std::string validation =
        datalab::domain::validate_import_plan(*plan, current_metadata_);
    if (!validation.empty()) {
        if (error != nullptr) {
            *error = QString::fromStdString(validation);
        }
        return false;
    }
    return true;
}

void DatabaseImportWizard::go_next()
{
    const int index = stack_->currentIndex();
    if (index == 0) {
        if (!service_->has_active_provider()) {
            test_connection();
            if (!service_->has_active_provider()) {
                return;
            }
        }
        refresh_objects();
    }
    if (index == 1 && object_list_->currentRow() < 0) {
        show_status(QStringLiteral("请先选择一个表或视图。"), true);
        return;
    }
    if (index == 2) {
        QString error;
        datalab::domain::ImportPlan plan;
        if (!build_current_plan(&plan, &error)) {
            show_status(error, true);
            return;
        }
        run_preview();
    }
    if (index < stack_->count() - 1) {
        stack_->setCurrentIndex(index + 1);
        update_navigation();
    }
}

void DatabaseImportWizard::go_back()
{
    if (stack_->currentIndex() > 0) {
        stack_->setCurrentIndex(stack_->currentIndex() - 1);
        update_navigation();
    }
}

void DatabaseImportWizard::update_connection_fields()
{
    const QString provider_id = provider_combo_->currentData().toString();
    const bool sqlite = provider_id == QStringLiteral("sqlite");
    const bool serverish = provider_id == QStringLiteral("postgresql")
        || provider_id == QStringLiteral("mysql")
        || provider_id == QStringLiteral("odbc");
    if (sqlite_fields_ != nullptr) {
        sqlite_fields_->setVisible(sqlite);
    }
    if (postgres_fields_ != nullptr) {
        postgres_fields_->setVisible(serverish);
    }
    if (provider_id == QStringLiteral("mysql") && pg_port_spin_->value() == 5432) {
        pg_port_spin_->setValue(3306);
    }
    if (provider_id == QStringLiteral("odbc")) {
        pg_host_edit_->setPlaceholderText(QStringLiteral("可留空"));
        pg_database_edit_->setPlaceholderText(QStringLiteral("ODBC DSN 名称"));
    }
}

std::string DatabaseImportWizard::preferred_keyset_column() const
{
    const auto columns = preferred_keyset_columns();
    return columns.empty() ? std::string{} : columns.front();
}

std::vector<std::string> DatabaseImportWizard::preferred_keyset_columns() const
{
    if (!current_metadata_.primary_key_columns.empty()) {
        return current_metadata_.primary_key_columns;
    }
    if (!order_key_->currentData().toString().isEmpty()) {
        return {order_key_->currentData().toString().toStdString()};
    }
    return {};
}

void DatabaseImportWizard::test_connection()
{
    imported_table_.reset();
    const QString provider_id = provider_combo_->currentData().toString();
    if (provider_id != QStringLiteral("sqlite")
        && provider_id != QStringLiteral("postgresql")
        && provider_id != QStringLiteral("mysql")
        && provider_id != QStringLiteral("odbc")) {
        show_status(QStringLiteral("未知 Provider。"), true);
        connection_status_->setText(QStringLiteral("Provider 不可用"));
        return;
    }
    datalab::domain::ConnectionOptions options;
    options.provider_id = provider_id.toStdString();
    options.read_only = read_only_check_->isChecked();
    if (provider_id == QStringLiteral("sqlite")) {
        options.file_path = file_path_edit_->text().trimmed().toStdString();
    } else if (provider_id == QStringLiteral("odbc")) {
        // Reuse host field as DSN name when ODBC selected via postgres fields temporarily.
        // Dedicated ODBC DSN field: use pg_database as DSN label for now if host empty.
        options.dsn = pg_database_edit_->text().trimmed().isEmpty()
            ? pg_host_edit_->text().trimmed().toStdString()
            : pg_database_edit_->text().trimmed().toStdString();
        options.username = pg_user_edit_->text().trimmed().toStdString();
        options.password = pg_password_edit_->text().toStdString();
        options.display_name = options.dsn;
    } else {
        options.host = pg_host_edit_->text().trimmed().toStdString();
        options.port = pg_port_spin_->value();
        if (provider_id == QStringLiteral("mysql") && options.port == 5432) {
            options.port = 3306;
        }
        options.database = pg_database_edit_->text().trimmed().toStdString();
        options.schema = pg_schema_edit_->text().trimmed().toStdString();
        options.username = pg_user_edit_->text().trimmed().toStdString();
        options.password = pg_password_edit_->text().toStdString();
    }
    const auto result = service_->connect(options);
    if (!result.ok) {
        connection_status_->setText(
            QStringLiteral("连接失败：%1").arg(QString::fromStdString(result.error_message)));
        show_status(QString::fromStdString(result.error_message), true);
        return;
    }
    connection_status_->setText(
        QStringLiteral("已连接（%1）")
            .arg(QString::fromStdString(result.value.server_version)));
    show_status(QStringLiteral("连接成功。"), false);
}

void DatabaseImportWizard::refresh_objects()
{
    object_list_->clear();
    if (!service_->has_active_provider()) {
        show_status(QStringLiteral("请先测试连接。"), true);
        return;
    }
    datalab::domain::ObjectFilter filter;
    filter.name_contains = object_search_->text().trimmed().toStdString();
    const auto result = service_->list_objects(filter);
    if (!result.ok) {
        show_status(QString::fromStdString(result.error_message), true);
        return;
    }
    for (const auto& object : result.value) {
        auto* item = new QListWidgetItem(
            QStringLiteral("[%1] %2")
                .arg(object_kind_label(object.ref.kind),
                     QString::fromStdString(object.ref.name)),
            object_list_);
        item->setData(Qt::UserRole, QString::fromStdString(object.ref.name));
        item->setData(Qt::UserRole + 1, static_cast<int>(object.ref.kind));
        item->setData(Qt::UserRole + 2, QString::fromStdString(object.ref.schema));
    }
    show_status(QStringLiteral("发现 %1 个对象。").arg(object_list_->count()), false);
}

void DatabaseImportWizard::on_object_selected()
{
    column_table_->setRowCount(0);
    filter_column_->clear();
    order_key_->clear();
    order_key_->addItem(QStringLiteral("(默认/主键)"), QString());
    current_object_ = {};
    current_metadata_ = {};
    const auto* item = object_list_->currentItem();
    if (item == nullptr || !service_->has_active_provider()) {
        return;
    }
    current_object_.name = item->data(Qt::UserRole).toString().toStdString();
    current_object_.schema = item->data(Qt::UserRole + 2).toString().toStdString();
    if (current_object_.schema.empty()) {
        current_object_.schema = "main";
    }
    current_object_.kind = static_cast<datalab::domain::DatabaseObjectKind>(
        item->data(Qt::UserRole + 1).toInt());
    worksheet_name_->setText(QString::fromStdString(current_object_.name));

    const auto result = service_->describe(current_object_);
    if (!result.ok) {
        show_status(QString::fromStdString(result.error_message), true);
        return;
    }
    current_metadata_ = result.value;
    column_table_->setRowCount(static_cast<int>(current_metadata_.columns.size()));
    int row = 0;
    for (const auto& column : current_metadata_.columns) {
        auto* check = new QCheckBox(column_table_);
        check->setChecked(column.selectable && !column.hidden);
        column_table_->setCellWidget(row, 0, check);
        column_table_->setItem(
            row, 1, new QTableWidgetItem(QString::fromStdString(column.name)));
        column_table_->setItem(
            row, 2, new QTableWidgetItem(QString::fromStdString(column.native_type)));
        column_table_->setItem(
            row, 3, new QTableWidgetItem(column.nullable ? QStringLiteral("是") : QStringLiteral("否")));
        column_table_->setItem(
            row, 4, new QTableWidgetItem(column.primary_key ? QStringLiteral("是") : QStringLiteral("否")));
        column_table_->setItem(
            row, 5, new QTableWidgetItem(column.hidden ? QStringLiteral("是") : QStringLiteral("否")));
        filter_column_->addItem(QString::fromStdString(column.name));
        order_key_->addItem(QString::fromStdString(column.name),
                            QString::fromStdString(column.name));
        ++row;
    }
}

void DatabaseImportWizard::run_preview()
{
    QString error;
    datalab::domain::ImportPlan plan;
    if (!build_current_plan(&plan, &error)) {
        show_status(error, true);
        return;
    }
    // Preview uses a fixed sample size; does not mutate plan.row_limit for import.
    const auto preview = service_->preview(plan, 50);
    if (!preview.ok) {
        show_status(QString::fromStdString(preview.error_message), true);
        return;
    }
    const auto& sample = preview.value.sample;
    preview_model_->set_sample(sample, preview.value.truncated);
    const auto keyset_columns = preferred_keyset_columns();
    preview_model_->configure_paging(
        plan,
        50,
        [this](const datalab::domain::ImportPlan& page_plan,
               std::uint64_t,
               std::uint64_t) {
            auto page = service_->preview(page_plan, page_plan.row_limit.value_or(50));
            if (!page.ok) {
                return datalab::domain::DatabaseResult<datalab::domain::DataTable>::failure(
                    page.error_code, page.error_message, page.diagnostics);
            }
            return datalab::domain::DatabaseResult<datalab::domain::DataTable>::success(
                page.value.sample);
        },
        keyset_columns);
    QString keyset_label = QStringLiteral("OFFSET 回退");
    if (!keyset_columns.empty()) {
        QStringList names;
        for (const auto& column : keyset_columns) {
            names.append(QString::fromStdString(column));
        }
        keyset_label = QStringLiteral("keyset(%1)").arg(names.join(QStringLiteral(",")));
    }
    preview_summary_->setText(
        QStringLiteral("预览样本 %1 行（truncated=%2）；ImportPlan 行上限=%3；分页=%4。")
            .arg(static_cast<qulonglong>(preview.value.preview_row_count))
            .arg(preview.value.truncated ? QStringLiteral("是") : QStringLiteral("否"))
            .arg(plan.row_limit.has_value()
                     ? QString::number(static_cast<qulonglong>(*plan.row_limit))
                     : QStringLiteral("不限制"))
            .arg(keyset_label));
    show_status(QStringLiteral("预览完成。"), false);
}

void DatabaseImportWizard::run_import()
{
    imported_table_.reset();
    QString error;
    datalab::domain::ImportPlan plan;
    if (!build_current_plan(&plan, &error)) {
        show_status(error, true);
        return;
    }
    cancel_token_ = datalab::domain::ImportCancellationToken{};
    cancel_button_->setEnabled(true);
    import_button_->setEnabled(false);
    show_status(QStringLiteral("正在导入…"), false);

    const auto result = service_->import_table(plan, &cancel_token_);
    cancel_button_->setEnabled(false);
    import_button_->setEnabled(true);

    if (!result.ok) {
        imported_table_.reset();
        result_summary_->setText(
            QStringLiteral("导入失败：%1（未写入工作表）")
                .arg(QString::fromStdString(result.error_message)));
        show_status(QString::fromStdString(result.error_message), true);
        return;
    }

    imported_table_ = result.value.table;
    result_summary_->setText(
        QStringLiteral("导入成功：%1 行 × %2 列；provider=%3；对象=%4；synthetic_row_id=%5；过滤=%6")
            .arg(static_cast<qulonglong>(imported_table_->rows.size()))
            .arg(static_cast<qulonglong>(imported_table_->columns.size()))
            .arg(QString::fromStdString(imported_table_->import_metadata.provider_id))
            .arg(QString::fromStdString(imported_table_->import_metadata.source_object))
            .arg(imported_table_->import_metadata.row_id_is_synthetic
                     ? QStringLiteral("是")
                     : QStringLiteral("否"))
            .arg(QString::fromStdString(imported_table_->import_metadata.filter_summary)));
    show_status(QStringLiteral("导入成功，可关闭向导以应用到主窗口。"), false);
    accept();
}

void DatabaseImportWizard::cancel_import()
{
    cancel_token_.request_cancel();
    show_status(QStringLiteral("已请求取消…"), false);
}

}  // namespace datalab::ui
