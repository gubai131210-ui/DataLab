#include "domain/quality_diagnostics.h"

#include <algorithm>

namespace datalab::domain {

void add_diagnostic(
    std::vector<DiagnosticMessage>& diagnostics,
    DiagnosticMessage::Severity severity,
    const std::string& code,
    const std::string& message,
    const std::vector<RowId>& related_rows)
{
    diagnostics.push_back({severity, code, message, related_rows, {}, {}, {}});
}

void add_error(
    std::vector<DiagnosticMessage>& diagnostics,
    const std::string& code,
    const std::string& message,
    const std::vector<RowId>& related_rows)
{
    add_diagnostic(diagnostics, DiagnosticMessage::Severity::error, code, message, related_rows);
}

void add_warning(
    std::vector<DiagnosticMessage>& diagnostics,
    const std::string& code,
    const std::string& message,
    const std::vector<RowId>& related_rows)
{
    add_diagnostic(diagnostics, DiagnosticMessage::Severity::warning, code, message, related_rows);
}

bool has_diagnostic_code(
    const std::vector<DiagnosticMessage>& diagnostics,
    const std::string& code)
{
    return std::any_of(diagnostics.cbegin(), diagnostics.cend(),
        [&code](const DiagnosticMessage& diagnostic) {
            return diagnostic.code == code;
        });
}

void apply_evidence(MethodMetadata& metadata, const QualityEvidence& evidence)
{
    metadata.version = evidence.method_version;
    metadata.valid_count = evidence.valid_count;
    metadata.missing_count = evidence.missing_count;
    metadata.assumption_status = evidence.assumption_status;
    metadata.parameter_source = evidence.parameter_source;
    metadata.not_computed_reason = evidence.not_computed_reason;
    if (!evidence.source_rows.empty()) {
        metadata.source_rows.clear();
        metadata.source_rows.reserve(evidence.source_rows.size());
        for (const std::size_t row : evidence.source_rows) {
            metadata.source_rows.push_back(static_cast<RowId>(row));
        }
    }
}

}  // namespace datalab::domain
