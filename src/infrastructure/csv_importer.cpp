#include "infrastructure/csv_importer.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <algorithm>

namespace datalab::infrastructure {
namespace {

std::vector<std::string> split_line(const QString& line, QChar delimiter)
{
    std::vector<std::string> fields;
    QString field;
    bool quoted = false;
    for (int index = 0; index < line.size(); ++index) {
        const QChar character = line.at(index);
        if (character == '"') {
            if (quoted && index + 1 < line.size() && line.at(index + 1) == '"') {
                field += '"';
                ++index;
            } else {
                quoted = !quoted;
            }
        } else if (character == delimiter && !quoted) {
            fields.push_back(field.toStdString());
            field.clear();
        } else {
            field += character;
        }
    }
    fields.push_back(field.toStdString());
    return fields;
}

}  // namespace

std::optional<domain::DataTable> CsvImporter::import_file(
    const QString& file_path,
    QString* error_message)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error_message != nullptr) {
            *error_message = file.errorString();
        }
        return std::nullopt;
    }

    QTextStream stream(&file);
    if (stream.atEnd()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("The CSV file is empty.");
        }
        return std::nullopt;
    }

    QString header_line = stream.readLine();
    if (!header_line.isEmpty() && header_line.front() == QChar(0xFEFF)) {
        header_line.remove(0, 1);
    }
    const QChar delimiter =
        header_line.count(',') >= header_line.count(';') ? ',' : ';';

    domain::DataTable table;
    table.name = QFileInfo(file_path).completeBaseName().toStdString();
    table.source_path = file_path.toStdString();
    table.columns = split_line(header_line, delimiter);
    for (std::size_t index = 0; index < table.columns.size(); ++index) {
        if (table.columns[index].empty()) {
            table.import_warnings.push_back(
                "第 " + std::to_string(index + 1) + " 列的列名为空。");
        }
        if (std::find(table.columns.begin(), table.columns.begin() + index,
                      table.columns[index]) != table.columns.begin() + index) {
            table.import_warnings.push_back(
                "检测到重复列名：" + table.columns[index]);
        }
    }
    std::size_t line_number = 2;
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (!line.trimmed().isEmpty()) {
            auto row = split_line(line, delimiter);
            if (row.size() != table.columns.size()) {
                table.import_warnings.push_back(
                    "第 " + std::to_string(line_number) + " 行包含 "
                    + std::to_string(row.size()) + " 个字段，表头包含 "
                    + std::to_string(table.columns.size()) + " 个字段。");
            }
            row.resize(table.columns.size());
            table.rows.push_back(std::move(row));
        }
        ++line_number;
    }
    return table;
}

}  // namespace datalab::infrastructure
