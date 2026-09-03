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

struct LearningGlossaryItem {
    QString term;
    QString plain;
    QString remember;
};

struct LearningDialogFillDetail {
    QString field;
    QString put;
    QString meaning;
};

struct LearningBuriedSignal {
    int row = 0;
    QString what;
    QString expect;
};

struct LearningPrereqItem {
    QString q;
    QString good;
    QString bad;
};

struct LearningSelfExplain {
    QString after;
    QString prompt;
};

struct LearningFadeLevel {
    int level = 0;
    QString student;
    QString scaffold;
};

struct LearningMisconception {
    QString wrong;
    QString right;
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
    QVector<LearningGlossaryItem> glossary;
    QVector<LearningDialogFillDetail> dialog_fill_detail;
    QVector<LearningBuriedSignal> buried_signals;
    QVector<LearningPrereqItem> prereq_quiz;
    QVector<LearningSelfExplain> self_explain;
    QVector<LearningFadeLevel> fade_levels;
    QStringList retrieval_quiz;
    QVector<LearningMisconception> misconceptions;
    QString skill_mission;
};

struct LearningDatasetSummary {
    QString dataset_id;
    QString title;
    QString industry;
    QString story;
    int row_count = 0;
    QString notes;
};

}  // namespace datalab::application::learning
