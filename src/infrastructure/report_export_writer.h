#pragma once

#include "domain/report_types.h"

#include <QString>

#include <functional>

namespace datalab::infrastructure {

struct ReportExportResult {
    bool ok = false;
    QString pdf_path;
    QString audit_json_path;
    QString manifest_path;
    domain::ReportExportManifest manifest;
    QString error_message;
};

// Optional veraPDF CLI probe. Reads DATALAB_VERAPDF (path to veraPDF launcher).
// Does not invent validated_pass; caller merges via application honesty rules.
domain::ExternalPdfaValidatorResult run_optional_verapdf(const QString& pdf_path);

// Optional PAC / PDF/UA CLI probe. Reads DATALAB_PAC.
// Even when exit_code==0, QPainter pipeline must keep pdfua_status=unsupported
// (no tagged-PDF). Caller merges via merge_optional_pac_result.
domain::ExternalPdfaValidatorResult run_optional_pac(const QString& pdf_path);

// Atomic package export (infrastructure I/O only).
// after_pdf may mutate the manifest after the temp PDF exists (e.g. optional veraPDF).
// On failure: temps cleaned; final PDF/audit/manifest are not left as fake success.
ReportExportResult export_report_package(
    const QString& pdf_path,
    const domain::ReportDocument& document,
    domain::ReportExportManifest manifest,
    const std::function<void(domain::ReportExportManifest&, const QString& temp_pdf)>&
        after_pdf = {});

}  // namespace datalab::infrastructure
