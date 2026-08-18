#include "infrastructure/python_table_importer.h"

#include "domain/column_extract.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTextStream>

namespace datalab::infrastructure {

std::optional<domain::DataTable> PythonTableImporter::import_file(
    const QString& file_path,
    QString* error_message,
    const QString& interpreter_path,
    const QString& script_path)
{
    const QString python_path =
        QCoreApplication::applicationDirPath() + QStringLiteral("/.venv/Scripts/python.exe");
    QString interpreter = interpreter_path;
    if (interpreter.isEmpty()) {
        interpreter = QFileInfo::exists(python_path)
            ? python_path
            : QStandardPaths::findExecutable(QStringLiteral("python"));
    }
    if (interpreter.isEmpty()) {
        interpreter = QStandardPaths::findExecutable(QStringLiteral("python3"));
    }
    if (interpreter.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("未找到 Python 解释器，无法导入 Excel 文件。");
        }
        return std::nullopt;
    }
    QString effective_script_path = script_path;
    QTemporaryFile temporary_file;
    if (effective_script_path.isEmpty()) {
        QFile resource(QStringLiteral(":/tools/import_table.py"));
        if (!resource.open(QIODevice::ReadOnly)) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral("未找到内置 Python 导入脚本。");
            }
            return std::nullopt;
        }
        temporary_file.setFileTemplate(
            QDir::tempPath() + QStringLiteral("/datalab-import-XXXXXX.py"));
        if (!temporary_file.open()
            || temporary_file.write(resource.readAll()) < 0) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral("无法释放内置 Python 导入脚本。");
            }
            return std::nullopt;
        }
        temporary_file.close();
        effective_script_path = temporary_file.fileName();
    }

    QProcess process;
    process.start(interpreter, {effective_script_path, file_path});
    if (!process.waitForStarted(5000)) {
        if (error_message != nullptr) {
            *error_message = process.errorString();
        }
        return std::nullopt;
    }
    if (!process.waitForFinished(120000)) {
        if (error_message != nullptr) {
            *error_message = process.errorString();
        }
        return std::nullopt;
    }

    const QByteArray standard_error = process.readAllStandardError();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error_message != nullptr) {
            *error_message = QString::fromUtf8(standard_error).trimmed();
            if (error_message->isEmpty()) {
                *error_message = QStringLiteral("Python 表格导入进程失败。");
            }
        }
        return std::nullopt;
    }
    const QJsonDocument document = QJsonDocument::fromJson(process.readAllStandardOutput());
    if (!document.isObject()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("Python importer returned invalid JSON.");
        }
        return std::nullopt;
    }
    const QJsonObject object = document.object();
    if (object.value(QStringLiteral("schema_version")).toInt(-1) != 1) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral(
                "Python importer returned an unsupported schema version.");
        }
        return std::nullopt;
    }
    if (object.contains(QStringLiteral("error"))) {
        if (error_message != nullptr) {
            *error_message = object.value(QStringLiteral("error")).toString();
        }
        return std::nullopt;
    }

    domain::DataTable table;
    table.name = object.value(QStringLiteral("name")).toString().toStdString();
    table.source_path = object.value(QStringLiteral("source_path")).toString().toStdString();
    for (const QJsonValue& value : object.value(QStringLiteral("columns")).toArray()) {
        table.columns.push_back(value.toString().toStdString());
    }
    for (const QJsonValue& row_value : object.value(QStringLiteral("rows")).toArray()) {
        std::vector<std::string> row;
        for (const QJsonValue& value : row_value.toArray()) {
            row.push_back(value.toString().toStdString());
        }
        table.rows.push_back(std::move(row));
    }
    if (table.columns.empty()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("Python importer returned no columns.");
        }
        return std::nullopt;
    }
    for (const auto& row : table.rows) {
        if (row.size() != table.columns.size()) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral(
                    "Python importer returned rows with inconsistent column counts.");
            }
            return std::nullopt;
        }
    }
    domain::populate_data_table_contract(table);
    table.import_metadata.schema_version =
        object.value(QStringLiteral("schema_version")).toInt(1);
    table.import_metadata.sheet_name =
        object.value(QStringLiteral("sheet_name")).toString().toStdString();
    table.import_metadata.sheet_index =
        static_cast<std::size_t>(object.value(QStringLiteral("sheet_index")).toInt(0));
    table.import_metadata.warnings = table.import_warnings;
    return table;
}

}  // namespace datalab::infrastructure
