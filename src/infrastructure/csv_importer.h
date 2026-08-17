#pragma once

#include "domain/quality_types.h"

#include <QString>

#include <optional>

namespace datalab::infrastructure {

class CsvImporter final {
public:
    static std::optional<domain::DataTable> import_file(
        const QString& file_path,
        QString* error_message = nullptr);
};

}  // namespace datalab::infrastructure
