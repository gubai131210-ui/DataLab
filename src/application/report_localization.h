#pragma once

#include "domain/quality_types.h"
#include "domain/report_text_catalog.h"
#include "domain/report_types.h"

#include <string>
#include <vector>

namespace datalab::application {

struct ReportLocalizationResult {
    domain::ReportDocument document;
    std::vector<domain::MissingTranslation> missing_translations;
    domain::ReportTextCoverage coverage;
};

// Applies report-locale presentation metadata without mutating Facts numerics.
// Adds diagnostics when translations are missing (no silent mixed-language).
ReportLocalizationResult localize_report_document(const domain::ReportDocument& document);

}  // namespace datalab::application
