#include "infrastructure/project_repository.h"
#include "infrastructure/output_serialization.h"

#include "domain/column_extract.h"

#include <QDateTime>
#include <QHash>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace datalab::infrastructure {
namespace {

constexpr int kProjectSchemaVersion = 1;

bool ensure_schema_version(QSqlQuery& query, QString* error_message)
{
    if (!query.exec(QStringLiteral("PRAGMA user_version"))) {
        if (error_message != nullptr) {
            *error_message = query.lastError().text();
        }
        return false;
    }
    const int version = query.next() ? query.value(0).toInt() : 0;
    if (version > kProjectSchemaVersion) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("项目文件版本高于当前程序支持的版本。");
        }
        return false;
    }
    if (version < kProjectSchemaVersion
        && !query.exec(QStringLiteral("PRAGMA user_version = 1"))) {
        if (error_message != nullptr) {
            *error_message = query.lastError().text();
        }
        return false;
    }
    return true;
}

}  // namespace

bool ProjectRepository::save(
    const QString& project_path,
    const domain::DataTable& table,
    const std::vector<domain::CleaningOperation>& operations,
    QString* error_message) const
{
    return save(project_path, table, operations, {}, error_message);
}

bool ProjectRepository::save(
    const QString& project_path,
    const domain::DataTable& table,
    const std::vector<domain::CleaningOperation>& operations,
    const std::vector<domain::OutputPage>& pages,
    QString* error_message) const
{
    const QString connection_name =
        QStringLiteral("datalab_project_%1").arg(reinterpret_cast<quintptr>(this));
    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection_name);
    database.setDatabaseName(project_path);
    QSqlQuery query(database);

    auto fail = [&](const QString& message) {
        if (error_message != nullptr) {
            *error_message = message;
        }
        query.clear();
        database.close();
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(connection_name);
        return false;
    };

    if (!database.open()) {
        return fail(database.lastError().text());
    }
    const QStringList schema = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS project_meta (key TEXT PRIMARY KEY, value TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS raw_columns (position INTEGER PRIMARY KEY, name TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS raw_rows (row_number INTEGER PRIMARY KEY, values_json TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS cleaning_operations (id INTEGER PRIMARY KEY AUTOINCREMENT, operation TEXT, reason TEXT, affected_rows TEXT)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS output_pages (id INTEGER PRIMARY KEY AUTOINCREMENT, page_id TEXT, payload TEXT)")
    };
    for (const QString& statement : schema) {
        if (!query.exec(statement)) {
            return fail(query.lastError().text());
        }
    }
    if (!ensure_schema_version(query, error_message)) {
        return fail(error_message != nullptr
            ? *error_message : QStringLiteral("项目文件版本初始化失败。"));
    }

    if (!database.transaction()) {
        return fail(database.lastError().text());
    }

    query.prepare(QStringLiteral("INSERT OR REPLACE INTO project_meta (key, value) VALUES (?, ?)"));
    query.addBindValue(QStringLiteral("name"));
    query.addBindValue(QString::fromStdString(table.name));
    if (!query.exec()) {
        database.rollback();
        return fail(query.lastError().text());
    }
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO project_meta (key, value) VALUES (?, ?)"));
    query.addBindValue(QStringLiteral("source_path"));
    query.addBindValue(QString::fromStdString(table.source_path));
    if (!query.exec()) {
        database.rollback();
        return fail(query.lastError().text());
    }
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO project_meta (key, value) VALUES (?, ?)"));
    query.addBindValue(QStringLiteral("saved_at"));
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!query.exec()) {
        database.rollback();
        return fail(query.lastError().text());
    }

    const QStringList cleanup = {
        QStringLiteral("DELETE FROM raw_columns"),
        QStringLiteral("DELETE FROM raw_rows"),
        QStringLiteral("DELETE FROM cleaning_operations"),
        QStringLiteral("DELETE FROM output_pages")};
    for (const QString& statement : cleanup) {
        if (!query.exec(statement)) {
            database.rollback();
            return fail(query.lastError().text());
        }
    }

    query.prepare(QStringLiteral("INSERT INTO raw_columns (position, name) VALUES (?, ?)"));
    for (std::size_t index = 0; index < table.columns.size(); ++index) {
        query.addBindValue(static_cast<qlonglong>(index));
        query.addBindValue(QString::fromStdString(table.columns[index]));
        if (!query.exec()) {
            database.rollback();
            return fail(query.lastError().text());
        }
    }

    query.prepare(QStringLiteral("INSERT INTO raw_rows (row_number, values_json) VALUES (?, ?)"));
    for (std::size_t row_index = 0; row_index < table.rows.size(); ++row_index) {
        QStringList values;
        for (const std::string& value : table.rows[row_index]) {
            values.append(QString::fromStdString(value).replace('\\', QStringLiteral("\\\\")).replace('|', QStringLiteral("\\|")));
        }
        query.addBindValue(static_cast<qlonglong>(row_index));
        query.addBindValue(values.join('|'));
        if (!query.exec()) {
            database.rollback();
            return fail(query.lastError().text());
        }
    }

    query.prepare(QStringLiteral("INSERT INTO cleaning_operations (operation, reason, affected_rows) VALUES (?, ?, ?)"));
    for (const domain::CleaningOperation& operation : operations) {
        QStringList affected_rows;
        for (const std::size_t row : operation.affected_rows) {
            affected_rows.append(QString::number(static_cast<qulonglong>(row)));
        }
        query.addBindValue(QString::fromStdString(operation.operation));
        query.addBindValue(QString::fromStdString(operation.reason));
        query.addBindValue(affected_rows.join(','));
        if (!query.exec()) {
            database.rollback();
            return fail(query.lastError().text());
        }
    }

    query.prepare(QStringLiteral("INSERT INTO output_pages (page_id, payload) VALUES (?, ?)"));
    for (const domain::OutputPage& page : pages) {
        query.addBindValue(QString::fromStdString(page.id));
        query.addBindValue(QString::fromUtf8(
            QJsonDocument(output_page_to_json(page)).toJson(QJsonDocument::Compact)));
        if (!query.exec()) {
            database.rollback();
            return fail(query.lastError().text());
        }
    }

    if (!database.commit()) {
        return fail(database.lastError().text());
    }
    query.clear();
    query = QSqlQuery();
    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection_name);
    return true;
}

bool ProjectRepository::load(
    const QString& project_path,
    domain::DataTable* table,
    std::vector<domain::CleaningOperation>* operations,
    QString* error_message) const
{
    return load(project_path, table, operations, nullptr, error_message);
}

bool ProjectRepository::load(
    const QString& project_path,
    domain::DataTable* table,
    std::vector<domain::CleaningOperation>* operations,
    std::vector<domain::OutputPage>* pages,
    QString* error_message) const
{
    if (table == nullptr || operations == nullptr) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("无效的项目输出参数。");
        }
        return false;
    }
    const QString connection_name =
        QStringLiteral("datalab_load_%1").arg(reinterpret_cast<quintptr>(this));
    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection_name);
    database.setDatabaseName(project_path);
    if (!database.open()) {
        if (error_message != nullptr) {
            *error_message = database.lastError().text();
        }
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(connection_name);
        return false;
    }
    QSqlQuery query(database);
    if (!ensure_schema_version(query, error_message)) {
        query.clear();
        database.close();
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(connection_name);
        return false;
    }
    auto fail = [&](const QString& message) {
        if (error_message != nullptr) {
            *error_message = message;
        }
        query.clear();
        database.close();
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(connection_name);
        return false;
    };

    QHash<QString, QString> metadata;
    if (!query.exec(QStringLiteral("SELECT key, value FROM project_meta"))) {
        return fail(query.lastError().text());
    }
    while (query.next()) {
        metadata.insert(query.value(0).toString(), query.value(1).toString());
    }
    domain::DataTable loaded;
    loaded.name = metadata.value(QStringLiteral("name")).toStdString();
    loaded.source_path = metadata.value(QStringLiteral("source_path")).toStdString();
    if (!query.exec(QStringLiteral("SELECT position, name FROM raw_columns ORDER BY position"))) {
        return fail(query.lastError().text());
    }
    while (query.next()) {
        loaded.columns.push_back(query.value(1).toString().toStdString());
    }
    if (!query.exec(QStringLiteral("SELECT row_number, values_json FROM raw_rows ORDER BY row_number"))) {
        return fail(query.lastError().text());
    }
    while (query.next()) {
        const QString encoded = query.value(1).toString();
        QStringList values;
        QString current;
        bool escaped = false;
        for (const QChar character : encoded) {
            if (escaped) {
                current += character;
                escaped = false;
            } else if (character == QLatin1Char('\\')) {
                escaped = true;
            } else if (character == QLatin1Char('|')) {
                values.append(current);
                current.clear();
            } else {
                current += character;
            }
        }
        values.append(current);
        std::vector<std::string> row;
        for (const QString& value : values) {
            row.push_back(value.toStdString());
        }
        loaded.rows.push_back(std::move(row));
    }
    std::vector<domain::CleaningOperation> loaded_operations;
    if (!query.exec(QStringLiteral(
            "SELECT operation, reason, affected_rows FROM cleaning_operations ORDER BY id"))) {
        return fail(query.lastError().text());
    }
    while (query.next()) {
        domain::CleaningOperation operation;
        operation.operation = query.value(0).toString().toStdString();
        operation.reason = query.value(1).toString().toStdString();
        for (const QString& row : query.value(2).toString().split(',', Qt::SkipEmptyParts)) {
            operation.affected_rows.push_back(static_cast<std::size_t>(row.toULongLong()));
        }
        loaded_operations.push_back(std::move(operation));
    }
    if (pages != nullptr) {
        pages->clear();
        if (query.exec(QStringLiteral("SELECT payload FROM output_pages ORDER BY id"))) {
            while (query.next()) {
                const QJsonDocument document = QJsonDocument::fromJson(query.value(0).toByteArray());
                if (document.isObject()) {
                    pages->push_back(output_page_from_json(document.object()));
                }
            }
        }
    }
    domain::populate_data_table_contract(loaded);
    query.clear();
    database.close();
    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection_name);
    *table = std::move(loaded);
    *operations = std::move(loaded_operations);
    return true;
}

}  // namespace datalab::infrastructure
