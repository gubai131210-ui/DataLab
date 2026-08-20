#pragma once

#include "ui/algorithm_help_types.h"

#include <optional>

class AlgorithmHelpCatalogLoader {
public:
    static AlgorithmHelpCatalog load_from_resource(const QString& resource_path = QStringLiteral(":/help/algorithm_help.json"));
    static bool validate(const AlgorithmHelpCatalog& catalog, QString* error_message = nullptr);
    static std::optional<AlgorithmHelpEntry> find_by_id(
        const AlgorithmHelpCatalog& catalog, const QString& id);
};
