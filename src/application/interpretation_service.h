#pragma once

#include "domain/quality_types.h"

namespace datalab::application {

class InterpretationService final {
public:
    static void enrich(domain::OutputPage& page);
};

}  // namespace datalab::application
