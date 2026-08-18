#pragma once

#include "domain/quality_types.h"

#include <string>
#include <vector>

namespace datalab::domain {

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const std::string& code,
    const std::string& message,
    const std::vector<RowId>& related_rows = {});

void add_error(
    std::vector<DiagnosticMessage>& diagnostics,
    const std::string& code,
    const std::string& message,
    const std::vector<RowId>& related_rows = {});

void add_warning(
    std::vector<DiagnosticMessage>& diagnostics,
    const std::string& code,
    const std::string& message,
    const std::vector<RowId>& related_rows = {});

bool has_diagnostic_code(
    const std::vector<DiagnosticMessage>& diagnostics,
    const std::string& code);

void apply_evidence(MethodMetadata& metadata, const QualityEvidence& evidence);

}  // namespace datalab::domain
