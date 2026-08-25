#pragma once

#include "domain/quality_types.h"

#include <string>

namespace datalab::application {

// Attach G9 computation traces for a finished OutputPage.
// No-op when command_id is empty or E-exempt (tests / rule_policy).
void attach_computation_traces(
    datalab::domain::OutputPage& page, const std::string& command_id);

// Resolve analysis_commands id from page.analysis_command_id, page.id prefix, or id itself.
std::string resolve_command_id_from_page(const datalab::domain::OutputPage& page);

}  // namespace datalab::application
