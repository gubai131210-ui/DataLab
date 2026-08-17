#pragma once

#include "domain/quality_types.h"

#include <QString>

#include <vector>

namespace datalab::infrastructure {

class ProjectRepository final {
public:
    bool save(
        const QString& project_path,
        const domain::DataTable& table,
        const std::vector<domain::CleaningOperation>& operations,
        QString* error_message = nullptr) const;

    bool save(
        const QString& project_path,
        const domain::DataTable& table,
        const std::vector<domain::CleaningOperation>& operations,
        const std::vector<domain::OutputPage>& pages,
        QString* error_message = nullptr) const;

    bool load(
        const QString& project_path,
        domain::DataTable* table,
        std::vector<domain::CleaningOperation>* operations,
        QString* error_message = nullptr) const;

    bool load(
        const QString& project_path,
        domain::DataTable* table,
        std::vector<domain::CleaningOperation>* operations,
        std::vector<domain::OutputPage>* pages,
        QString* error_message = nullptr) const;
};

}  // namespace datalab::infrastructure
