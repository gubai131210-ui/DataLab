#pragma once

#include "domain/quality_types.h"

#include <QString>

#include <optional>

namespace datalab::infrastructure {

class PythonTableImporter final {
public:
    static std::optional<domain::DataTable> import_file(
        const QString& file_path,
        QString* error_message = nullptr,
        const QString& interpreter_path = {},
        const QString& script_path = {});
};

}  // namespace datalab::infrastructure
