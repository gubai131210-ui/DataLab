#pragma once

#include "domain/quality_types.h"
#include "domain/report_types.h"

#include <QString>

#include <vector>

namespace datalab::infrastructure {

class PdfReportWriter final {
public:
    // Legacy path: engineer-like dump of raw OutputPages (no profile filtering).
    static bool write(
        const QString& file_path,
        const domain::DataTable& table,
        const std::vector<domain::OutputPage>& pages,
        QString* error_message = nullptr);

    // Phase 1 profile-aware export. Does not recompute Facts.
    static bool write_document(
        const QString& file_path,
        const domain::ReportDocument& document,
        QString* error_message = nullptr);
};

}  // namespace datalab::infrastructure
