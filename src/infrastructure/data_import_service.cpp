#include "infrastructure/data_import_service.h"

#include "infrastructure/csv_importer.h"
#include "infrastructure/python_table_importer.h"

namespace datalab::infrastructure {

std::optional<domain::DataTable> DataImportService::import_file(
    const QString& file_path,
    QString* error_message)
{
    const bool is_excel = file_path.endsWith(QStringLiteral(".xlsx"), Qt::CaseInsensitive)
        || file_path.endsWith(QStringLiteral(".xls"), Qt::CaseInsensitive);
    return is_excel
        ? PythonTableImporter::import_file(file_path, error_message)
        : CsvImporter::import_file(file_path, error_message);
}

}  // namespace datalab::infrastructure
