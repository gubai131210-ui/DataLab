#pragma once

#include "application/learning/learning_types.h"
#include "domain/quality_types.h"

#include <QString>

#include <optional>
#include <vector>

namespace datalab::application::learning {

class LearningDatasetStore {
public:
    static constexpr const char* kResourcePath = ":/help/learning_center.sqlite";
    static constexpr const char* kExpectedCatalogVersion = "learning-center-v2";

    static QString catalog_version(QString* error_message = nullptr);
    static std::vector<LearningDatasetSummary> list_datasets(QString* error_message = nullptr);
    static std::optional<domain::DataTable> load_dataset(
        const QString& dataset_id, QString* error_message = nullptr);
    static QString materialize_resource_database(QString* error_message = nullptr);
};

}  // namespace datalab::application::learning
