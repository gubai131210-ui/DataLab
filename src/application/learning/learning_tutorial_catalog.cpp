#include "application/learning/learning_tutorial_catalog.h"

#include "application/learning/learning_dataset_store.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>

namespace datalab::application::learning {
namespace {

const QStringList& wanted_tutorial_columns()
{
    static const QStringList kColumns = {
        QStringLiteral("command_id"),
        QStringLiteral("title"),
        QStringLiteral("category"),
        QStringLiteral("menu_path"),
        QStringLiteral("implemented_status"),
        QStringLiteral("used_for"),
        QStringLiteral("not_for"),
        QStringLiteral("scenario"),
        QStringLiteral("dataset_id"),
        QStringLiteral("click_steps"),
        QStringLiteral("dialog_fill"),
        QStringLiteral("output_guide"),
        QStringLiteral("common_mistakes"),
        QStringLiteral("related_ids"),
        QStringLiteral("glossary"),
        QStringLiteral("dialog_fill_detail"),
        QStringLiteral("buried_signals"),
        QStringLiteral("prereq_quiz"),
        QStringLiteral("self_explain"),
        QStringLiteral("fade_levels"),
        QStringLiteral("retrieval_quiz"),
        QStringLiteral("misconceptions"),
        QStringLiteral("skill_mission"),
    };
    return kColumns;
}

QStringList available_tutorial_columns(QSqlDatabase& database)
{
    QSet<QString> names;
    QSqlQuery pragma(database);
    if (pragma.exec(QStringLiteral("PRAGMA table_info(tutorials)"))) {
        while (pragma.next()) {
            names.insert(pragma.value(1).toString());
        }
    }
    QStringList selected;
    for (const QString& name : wanted_tutorial_columns()) {
        if (names.contains(name)) {
            selected.push_back(name);
        }
    }
    return selected;
}

QString column_text(const QSqlQuery& query, const QString& name)
{
    const int index = query.record().indexOf(name);
    if (index < 0) {
        return {};
    }
    return query.value(index).toString();
}

QJsonDocument parse_json_document(const QString& text)
{
    return QJsonDocument::fromJson(text.toUtf8());
}

QJsonArray parse_json_array(const QString& text)
{
    if (text.trimmed().isEmpty()) {
        return {};
    }
    const QJsonDocument document = parse_json_document(text);
    if (!document.isArray()) {
        return {};
    }
    return document.array();
}

QJsonObject parse_json_object(const QString& text)
{
    if (text.trimmed().isEmpty()) {
        return {};
    }
    const QJsonDocument document = parse_json_document(text);
    if (!document.isObject()) {
        return {};
    }
    return document.object();
}

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

QVector<LearningGlossaryItem> parse_glossary(const QJsonArray& array)
{
    QVector<LearningGlossaryItem> items;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        LearningGlossaryItem item;
        item.term = object.value(QStringLiteral("term")).toString();
        item.plain = object.value(QStringLiteral("plain")).toString();
        item.remember = object.value(QStringLiteral("remember")).toString();
        items.push_back(std::move(item));
    }
    return items;
}

QVector<LearningDialogFillDetail> parse_dialog_fill_detail(const QJsonArray& array)
{
    QVector<LearningDialogFillDetail> items;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        LearningDialogFillDetail item;
        item.field = object.value(QStringLiteral("field")).toString();
        item.put = object.value(QStringLiteral("put")).toString();
        item.meaning = object.value(QStringLiteral("meaning")).toString();
        items.push_back(std::move(item));
    }
    return items;
}

int json_int(const QJsonObject& object, const QString& key)
{
    const QJsonValue value = object.value(key);
    if (value.isDouble()) {
        return value.toInt();
    }
    if (value.isString()) {
        bool ok = false;
        const int parsed = value.toString().toInt(&ok);
        return ok ? parsed : 0;
    }
    return 0;
}

QVector<LearningBuriedSignal> parse_buried_signals(const QJsonArray& array)
{
    QVector<LearningBuriedSignal> items;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        LearningBuriedSignal item;
        item.row = json_int(object, QStringLiteral("row"));
        item.what = object.value(QStringLiteral("what")).toString();
        item.expect = object.value(QStringLiteral("expect")).toString();
        items.push_back(std::move(item));
    }
    return items;
}

QVector<LearningPrereqItem> parse_prereq_quiz(const QJsonArray& array)
{
    QVector<LearningPrereqItem> items;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        LearningPrereqItem item;
        item.q = object.value(QStringLiteral("q")).toString();
        item.good = object.value(QStringLiteral("good")).toString();
        item.bad = object.value(QStringLiteral("bad")).toString();
        items.push_back(std::move(item));
    }
    return items;
}

QVector<LearningSelfExplain> parse_self_explain(const QJsonArray& array)
{
    QVector<LearningSelfExplain> items;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        LearningSelfExplain item;
        item.after = object.value(QStringLiteral("after")).toString();
        item.prompt = object.value(QStringLiteral("prompt")).toString();
        items.push_back(std::move(item));
    }
    return items;
}

QVector<LearningFadeLevel> parse_fade_levels(const QJsonArray& array)
{
    QVector<LearningFadeLevel> items;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        LearningFadeLevel item;
        item.level = json_int(object, QStringLiteral("level"));
        item.student = object.value(QStringLiteral("student")).toString();
        item.scaffold = object.value(QStringLiteral("scaffold")).toString();
        items.push_back(std::move(item));
    }
    return items;
}

QVector<LearningMisconception> parse_misconceptions(const QJsonArray& array)
{
    QVector<LearningMisconception> items;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        LearningMisconception item;
        item.wrong = object.value(QStringLiteral("wrong")).toString();
        item.right = object.value(QStringLiteral("right")).toString();
        items.push_back(std::move(item));
    }
    return items;
}

LearningTutorialEntry parse_tutorial_row(QSqlQuery& query)
{
    LearningTutorialEntry entry;
    entry.command_id = column_text(query, QStringLiteral("command_id"));
    entry.title = column_text(query, QStringLiteral("title"));
    entry.category = column_text(query, QStringLiteral("category"));
    entry.menu_path = column_text(query, QStringLiteral("menu_path"));
    entry.implemented_status = column_text(query, QStringLiteral("implemented_status"));
    entry.used_for = column_text(query, QStringLiteral("used_for"));
    entry.not_for = column_text(query, QStringLiteral("not_for"));
    entry.scenario = column_text(query, QStringLiteral("scenario"));

    const QString dataset_id = column_text(query, QStringLiteral("dataset_id"));
    if (!dataset_id.isEmpty()) {
        entry.dataset_id = dataset_id;
    }

    entry.click_steps = parse_string_array(parse_json_array(
        column_text(query, QStringLiteral("click_steps"))));
    entry.dialog_fill = parse_dialog_fill(parse_json_object(
        column_text(query, QStringLiteral("dialog_fill"))));
    entry.output_guide = parse_output_guide(parse_json_array(
        column_text(query, QStringLiteral("output_guide"))));
    entry.common_mistakes = parse_string_array(parse_json_array(
        column_text(query, QStringLiteral("common_mistakes"))));
    entry.related_ids = parse_string_array(parse_json_array(
        column_text(query, QStringLiteral("related_ids"))));
    entry.glossary = parse_glossary(parse_json_array(
        column_text(query, QStringLiteral("glossary"))));
    entry.dialog_fill_detail = parse_dialog_fill_detail(parse_json_array(
        column_text(query, QStringLiteral("dialog_fill_detail"))));
    entry.buried_signals = parse_buried_signals(parse_json_array(
        column_text(query, QStringLiteral("buried_signals"))));
    entry.prereq_quiz = parse_prereq_quiz(parse_json_array(
        column_text(query, QStringLiteral("prereq_quiz"))));
    entry.self_explain = parse_self_explain(parse_json_array(
        column_text(query, QStringLiteral("self_explain"))));
    entry.fade_levels = parse_fade_levels(parse_json_array(
        column_text(query, QStringLiteral("fade_levels"))));
    entry.retrieval_quiz = parse_string_array(parse_json_array(
        column_text(query, QStringLiteral("retrieval_quiz"))));
    entry.misconceptions = parse_misconceptions(parse_json_array(
        column_text(query, QStringLiteral("misconceptions"))));
    entry.skill_mission = column_text(query, QStringLiteral("skill_mission"));
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

    const QStringList columns = available_tutorial_columns(database);
    QSqlQuery query(database);
    const QString sql = QStringLiteral("SELECT %1 FROM tutorials ORDER BY category, title")
                            .arg(columns.join(QStringLiteral(", ")));
    if (columns.isEmpty() || !query.exec(sql)) {
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
