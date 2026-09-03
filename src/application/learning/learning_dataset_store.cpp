#include "application/learning/learning_dataset_store.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QUuid>

#include <utility>

namespace datalab::application::learning {
namespace {

class ScopedLearningConnection {
public:
    explicit ScopedLearningConnection(const QString& database_path, QString* error_message)
    {
        connection_name_ = QStringLiteral("learning_center_%1")
                               .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection_name_);
        database_.setDatabaseName(database_path);
        if (!database_.open()) {
            if (error_message != nullptr) {
                *error_message = database_.lastError().text();
            }
            QSqlDatabase::removeDatabase(connection_name_);
            connection_name_.clear();
            return;
        }
        opened_ = true;
    }

    ~ScopedLearningConnection()
    {
        close();
    }

    ScopedLearningConnection(const ScopedLearningConnection&) = delete;
    ScopedLearningConnection& operator=(const ScopedLearningConnection&) = delete;

    [[nodiscard]] bool is_open() const { return opened_; }

    QSqlDatabase& database() { return database_; }

private:
    void close()
    {
        if (!connection_name_.isEmpty()) {
            if (database_.isOpen()) {
                database_.close();
            }
            database_ = QSqlDatabase();
            QSqlDatabase::removeDatabase(connection_name_);
            connection_name_.clear();
            opened_ = false;
        }
    }

    QString connection_name_;
    QSqlDatabase database_;
    bool opened_ = false;
};

QString read_meta_value(QSqlDatabase& database, const QString& key)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral("SELECT value FROM meta WHERE key = ?"));
    query.addBindValue(key);
    if (!query.exec() || !query.next()) {
        return {};
    }
    return query.value(0).toString();
}

}  // namespace

QString LearningDatasetStore::materialize_resource_database(QString* error_message)
{
    QFile resource(QString::fromLatin1(kResourcePath));
    if (!resource.open(QIODevice::ReadOnly)) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("无法打开嵌入的学习中心数据库资源。");
        }
        return {};
    }
    QTemporaryFile temp_file;
    temp_file.setAutoRemove(false);
    if (!temp_file.open()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("无法创建临时数据库文件。");
        }
        return {};
    }
    if (temp_file.write(resource.readAll()) < 0) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("无法写入临时数据库文件。");
        }
        return {};
    }
    temp_file.close();
    return temp_file.fileName();
}

QString LearningDatasetStore::catalog_version(QString* error_message)
{
    const QString path = materialize_resource_database(error_message);
    if (path.isEmpty()) {
        return {};
    }
    ScopedLearningConnection connection(path, error_message);
    if (!connection.is_open()) {
        return {};
    }
    return read_meta_value(connection.database(), QStringLiteral("catalog_version"));
}

std::vector<LearningDatasetSummary> LearningDatasetStore::list_datasets(QString* error_message)
{
    std::vector<LearningDatasetSummary> summaries;
    const QString path = materialize_resource_database(error_message);
    if (path.isEmpty()) {
        return summaries;
    }
    ScopedLearningConnection connection(path, error_message);
    if (!connection.is_open()) {
        return summaries;
    }
    QSqlQuery query(connection.database());
    if (!query.exec(QStringLiteral(
            "SELECT dataset_id, title, industry, story, row_count, notes "
            "FROM datasets ORDER BY dataset_id"))) {
        if (error_message != nullptr) {
            *error_message = query.lastError().text();
        }
        return {};
    }
    while (query.next()) {
        LearningDatasetSummary summary;
        summary.dataset_id = query.value(0).toString();
        summary.title = query.value(1).toString();
        summary.industry = query.value(2).toString();
        summary.story = query.value(3).toString();
        summary.row_count = query.value(4).toInt();
        summary.notes = query.value(5).toString();
        summaries.push_back(std::move(summary));
    }
    return summaries;
}

std::optional<domain::DataTable> LearningDatasetStore::load_dataset(
    const QString& dataset_id, QString* error_message)
{
    if (dataset_id.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("dataset_id 为空。");
        }
        return std::nullopt;
    }

    const QString path = materialize_resource_database(error_message);
    if (path.isEmpty()) {
        return std::nullopt;
    }
    ScopedLearningConnection connection(path, error_message);
    if (!connection.is_open()) {
        return std::nullopt;
    }

    QSqlQuery meta_query(connection.database());
    meta_query.prepare(QStringLiteral(
        "SELECT title, row_count FROM datasets WHERE dataset_id = ?"));
    meta_query.addBindValue(dataset_id);
    if (!meta_query.exec() || !meta_query.next()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("未找到数据集：%1").arg(dataset_id);
        }
        return std::nullopt;
    }

    domain::DataTable table;
    table.name = QStringLiteral("demo_%1").arg(dataset_id).toStdString();
    table.import_metadata.dataset_id = dataset_id.toStdString();
    table.import_metadata.provider_id = "learning_center";
    table.import_metadata.object_kind = "demo_dataset";

    QSqlQuery column_query(connection.database());
    column_query.prepare(QStringLiteral(
        "SELECT name FROM dataset_columns WHERE dataset_id = ? ORDER BY column_index"));
    column_query.addBindValue(dataset_id);
    if (!column_query.exec()) {
        if (error_message != nullptr) {
            *error_message = column_query.lastError().text();
        }
        return std::nullopt;
    }
    while (column_query.next()) {
        table.columns.push_back(column_query.value(0).toString().toStdString());
    }

    const int row_count = meta_query.value(1).toInt();
    table.rows.resize(static_cast<std::size_t>(row_count));
    for (auto& row : table.rows) {
        row.resize(table.columns.size());
    }

    QSqlQuery cell_query(connection.database());
    cell_query.prepare(QStringLiteral(
        "SELECT row_index, column_index, value FROM dataset_cells "
        "WHERE dataset_id = ? ORDER BY row_index, column_index"));
    cell_query.addBindValue(dataset_id);
    if (!cell_query.exec()) {
        if (error_message != nullptr) {
            *error_message = cell_query.lastError().text();
        }
        return std::nullopt;
    }
    while (cell_query.next()) {
        const int row_index = cell_query.value(0).toInt();
        const int column_index = cell_query.value(1).toInt();
        const QString value = cell_query.value(2).toString();
        if (row_index >= 0 && row_index < row_count
            && column_index >= 0
            && static_cast<std::size_t>(column_index) < table.columns.size()) {
            table.rows[static_cast<std::size_t>(row_index)]
                     [static_cast<std::size_t>(column_index)] = value.toStdString();
        }
    }

    table.import_metadata.column_count = table.columns.size();
    table.import_metadata.original_row_count = table.rows.size();
    return table;
}

}  // namespace datalab::application::learning
