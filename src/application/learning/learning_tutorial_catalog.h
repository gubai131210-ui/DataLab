#pragma once

#include "application/learning/learning_types.h"

#include <QString>

#include <optional>
#include <vector>

namespace datalab::application::learning {

class LearningTutorialCatalog {
public:
    static std::vector<LearningTutorialEntry> load_all(QString* error_message = nullptr);
    static std::optional<LearningTutorialEntry> find_by_id(
        const QString& command_id, QString* error_message = nullptr);
};

}  // namespace datalab::application::learning
