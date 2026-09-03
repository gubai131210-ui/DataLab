#include "application/learning/learning_tutorial_catalog.h"

#include "application/learning/learning_dataset_store.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace datalab::application::learning {
namespace {

QStringList parse_string_array(const QJsonArray& array)
{
    QStringList values;
    for (const QJsonValue& value : array) {
        values.push_back(value.toString());
    }
    return values;
}

QMap<QString, QString> parse_dialog_fill(const QJsonObject& object)
{
    QMap<QString, QString> mapping;
    for (auto it = object.begin(); it != object.end(); ++it) {
        mapping.insert(it.key(), it.value().toString());
    }
    return mapping;
}

QVector<LearningOutputGuideItem> parse_output_guide(const QJsonArray& array)
{
    QVector<LearningOutputGuideItem> items;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        LearningOutputGuideItem item;
        item.name = object.value(QStringLiteral("name")).toString();
        item.meaning = object.value(QStringLiteral("meaning")).toString();
        items.push_back(std::move(item));
    }
    return items;
}

LearningTutorialEntry parse_tutorial_row(QSqlQuery& query)
{
    LearningTutorialEntry entry;
    entry.command_id = query.value(QStringLiteral("command_id")).toString();
    entry.title = query.value(QStringLiteral("title")).toString();
    entry.category = query.value(QStringLiteral("category")).toString();
    entry.menu_path = query.value(QStringLiteral("menu_path")).toString();
    entry.implemented_status = query.value(QStringLiteral("implemented_status")).toString();
    entry.used_for = query.value(QStringLiteral("used_for")).toString();
    entry.not_for = query.value(QStringLiteral("not_for")).toString();
    entry.scenario = query.value(QStringLiteral("scenario")).toString();

    const QString dataset_id = query.value(QStringLiteral("dataset_id")).toString();
    if (!dataset_id.isEmpty()) {
        entry.dataset_id = dataset_id;
    }

    const auto parse_json = [](const QString& text) {
        return QJsonDocument::fromJson(text.toUtf8()).array();
    };
    const auto parse_json_object = [](const QString& text) {
        return QJsonDocument::fromJson(text.toUtf8()).object();
    };

    entry.click_steps = parse_string_array(parse_json(
        query.value(QStringLiteral("click_steps")).toString()));
    entry.dialog_fill = parse_dialog_fill(parse_json_object(
        query.value(QStringLiteral("dialog_fill")).toString()));
    entry.output_guide = parse_output_guide(parse_json(
        query.value(QStringLiteral("output_guide")).toString()));
    entry.common_mistakes = parse_string_array(parse_json(
        query.value(QStringLiteral("common_mistakes")).toString()));
    entry.related_ids = parse_string_array(parse_json(
        query.value(QStringLiteral("related_ids")).toString()));
    return entry;
}

std::vector<LearningTutorialEntry> query_all_tutorials(QString* error_message)
{
    std::vector<LearningTutorialEntry> entries;
    const QString path = LearningDatasetStore::materialize_resource_database(error_message);
    if (path.isEmpty()) {
        return entries;
    }

    const QString connection_name = QStringLiteral("learning_center_catalog_%1")
                                        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection_name);
    database.setDatabaseName(path);
    if (!database.open()) {
        if (error_message != nullptr) {
            *error_message = database.lastError().text();
        }
        QSqlDatabase::removeDatabase(connection_name);
        return entries;
    }

    QSqlQuery query(database);
    if (!query.exec(QStringLiteral(
            "SELECT command_id, title, category, menu_path, implemented_status, "
            "used_for, not_for, scenario, dataset_id, click_steps, dialog_fill, "
            "output_guide, common_mistakes, related_ids FROM tutorials "
            "ORDER BY category, title"))) {
        if (error_message != nullptr) {
            *error_message = query.lastError().text();
        }
        database.close();
        QSqlDatabase::removeDatabase(connection_name);
        return entries;
    }

    while (query.next()) {
        entries.push_back(parse_tutorial_row(query));
    }

    database.close();
    QSqlDatabase::removeDatabase(connection_name);
    return entries;
}

}  // namespace

std::vector<LearningTutorialEntry> LearningTutorialCatalog::load_all(QString* error_message)
{
    return query_all_tutorials(error_message);
}

std::optional<LearningTutorialEntry> LearningTutorialCatalog::find_by_id(
    const QString& command_id, QString* error_message)
{
    for (const LearningTutorialEntry& entry : query_all_tutorials(error_message)) {
        if (entry.command_id == command_id) {
            return entry;
        }
    }
    return std::nullopt;
}

}  // namespace datalab::application::learning
