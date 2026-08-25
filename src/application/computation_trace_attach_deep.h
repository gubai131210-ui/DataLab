#pragma once

#include "domain/quality_types.h"

#include <string>

namespace datalab::application {

// G9-D L3 deep computation traces (79 former generic stubs).
bool attach_deep_trace(datalab::domain::OutputPage& page, const std::string& command_id);

}  // namespace datalab::application
