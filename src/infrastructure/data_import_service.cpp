#include "infrastructure/data_import_service.h"

#include "domain/column_extract.h"
#include "infrastructure/csv_importer.h"
#include "infrastructure/python_table_importer.h"

#include <QDateTime>

namespace datalab::infrastructure {

std::optional<domain::DataTable> DataImportService::import_file(
    const QString& file_path,
    QString* error_message)
{
    const bool is_excel = file_path.endsWith(QStringLiteral(".xlsx"), Qt::CaseInsensitive)
        || file_path.endsWith(QStringLiteral(".xls"), Qt::CaseInsensitive);
    auto imported = is_excel
        ? PythonTableImporter::import_file(file_path, error_message)
        : CsvImporter::import_file(file_path, error_message);
    if (!imported.has_value()) {
        return std::nullopt;
    }
    imported->source_path = file_path.toStdString();
    imported->import_metadata.imported_at =
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
    const std::string contract_error = domain::validate_data_table_contract(*imported);
    if (!contract_error.empty()) {
        if (error_message != nullptr) {
            *error_message = QString::fromStdString(contract_error);
        }
        return std::nullopt;
    }
    if (imported->source_path.empty()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("导入结果缺少可追溯的源文件路径。");
        }
        return std::nullopt;
    }
    return imported;
}

}  // namespace datalab::infrastructure
