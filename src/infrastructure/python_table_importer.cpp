#include "infrastructure/python_table_importer.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

namespace datalab::infrastructure {

std::optional<domain::DataTable> PythonTableImporter::import_file(
    const QString& file_path,
    QString* error_message)
{
    const QString python_path =
        QCoreApplication::applicationDirPath() + QStringLiteral("/.venv/Scripts/python.exe");
    QString interpreter = QFileInfo::exists(python_path)
        ? python_path
        : QStandardPaths::findExecutable(QStringLiteral("python"));
    if (interpreter.isEmpty()) {
        interpreter = QStandardPaths::findExecutable(QStringLiteral("python3"));
    }
    if (interpreter.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("未找到 Python 解释器，无法导入 Excel 文件。");
        }
        return std::nullopt;
    }
    const QString script_path =
        QStringLiteral(DATALAB_SOURCE_DIR "/tools/import_table.py");

    QProcess process;
    process.start(interpreter, {script_path, file_path});
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
    return table;
}

}  // namespace datalab::infrastructure
