#pragma once

#include "domain/quality_types.h"

#include <QString>

#include <vector>

namespace datalab::infrastructure {

class PdfReportWriter final {
public:
    static bool write(
        const QString& file_path,
        const domain::DataTable& table,
        const domain::AnalysisResult& result,
        QString* error_message = nullptr);

    static bool write(
        const QString& file_path,
        const domain::DataTable& table,
        const std::vector<domain::OutputPage>& pages,
        QString* error_message = nullptr);
};

}  // namespace datalab::infrastructure
