#pragma once

#include "domain/quality_types.h"

#include <QString>

#include <optional>

namespace datalab::infrastructure {

// 按文件扩展名选择 CSV 或 Excel(.xlsx) 导入器，收敛导入分派与错误传递。
class DataImportService final {
public:
    static std::optional<domain::DataTable> import_file(
        const QString& file_path,
        QString* error_message = nullptr);
};

}  // namespace datalab::infrastructure
