#pragma once

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

namespace datalab::application::learning {

struct LearningOutputGuideItem {
    QString name;
    QString meaning;
};

struct LearningTutorialEntry {
    QString command_id;
    QString title;
    QString category;
    QString menu_path;
    QString implemented_status;
    QString used_for;
    QString not_for;
    QString scenario;
    std::optional<QString> dataset_id;
    QStringList click_steps;
    QMap<QString, QString> dialog_fill;
    QVector<LearningOutputGuideItem> output_guide;
    QStringList common_mistakes;
    QStringList related_ids;
};

struct LearningDatasetSummary {
    QString dataset_id;
    QString title;
    QString industry;
    QString story;
    int row_count = 0;
};

}  // namespace datalab::application::learning
