#include "infrastructure/database/import_plan_serialization.h"

#include <QJsonArray>

namespace datalab::infrastructure {

QJsonObject import_plan_to_json(const domain::ImportPlan& plan)
{
    QJsonObject object;
    object.insert(QStringLiteral("source_connection_id"),
                  QString::fromStdString(plan.source_connection_id));
    object.insert(QStringLiteral("provider_id"), QString::fromStdString(plan.provider_id));
    QJsonObject ref;
    ref.insert(QStringLiteral("catalog"), QString::fromStdString(plan.object_ref.catalog));
    ref.insert(QStringLiteral("schema"), QString::fromStdString(plan.object_ref.schema));
    ref.insert(QStringLiteral("name"), QString::fromStdString(plan.object_ref.name));
    ref.insert(QStringLiteral("kind"), static_cast<int>(plan.object_ref.kind));
    object.insert(QStringLiteral("object_ref"), ref);

    QJsonArray selected;
    for (const std::string& column : plan.selected_columns) {
        selected.append(QString::fromStdString(column));
    }
    object.insert(QStringLiteral("selected_columns"), selected);

    QJsonArray order;
    for (const std::string& column : plan.column_order) {
        order.append(QString::fromStdString(column));
    }
    object.insert(QStringLiteral("column_order"), order);

    QJsonArray aliases;
    for (const std::string& alias : plan.aliases) {
        aliases.append(QString::fromStdString(alias));
    }
    object.insert(QStringLiteral("aliases"), aliases);

    QJsonArray filters;
    for (const domain::StructuredFilter& filter : plan.structured_filter) {
        QJsonObject item;
        item.insert(QStringLiteral("column"), QString::fromStdString(filter.column));
        item.insert(QStringLiteral("op"), QString::fromStdString(filter.op));
        // Bind values are not secrets here, but never persist passwords/DSN.
        item.insert(QStringLiteral("value"), QString::fromStdString(filter.value));
        filters.append(item);
    }
    object.insert(QStringLiteral("structured_filter"), filters);
    object.insert(QStringLiteral("order_key"), QString::fromStdString(plan.order_key));
    if (plan.row_limit.has_value()) {
        object.insert(QStringLiteral("row_limit"), static_cast<qint64>(*plan.row_limit));
    }
    if (plan.row_offset.has_value()) {
        object.insert(QStringLiteral("row_offset"), static_cast<qint64>(*plan.row_offset));
    }
    if (plan.keyset_after.has_value()) {
        QJsonObject keyset;
        QJsonArray columns;
        QJsonArray values;
        for (const std::string& column : plan.keyset_after->columns) {
            columns.append(QString::fromStdString(column));
        }
        for (const std::string& value : plan.keyset_after->after_values) {
            values.append(QString::fromStdString(value));
        }
        keyset.insert(QStringLiteral("columns"), columns);
        keyset.insert(QStringLiteral("after_values"), values);
        // Backward-compatible single-column aliases.
        if (plan.keyset_after->columns.size() == 1) {
            keyset.insert(QStringLiteral("column"),
                          QString::fromStdString(plan.keyset_after->columns.front()));
            keyset.insert(QStringLiteral("after_value"),
                          QString::fromStdString(plan.keyset_after->after_values.front()));
        }
        object.insert(QStringLiteral("keyset_after"), keyset);
    }
    if (plan.page_size.has_value()) {
        object.insert(QStringLiteral("page_size"), static_cast<qint64>(*plan.page_size));
    }
    object.insert(QStringLiteral("null_policy"), static_cast<int>(plan.null_policy));
    object.insert(QStringLiteral("target_worksheet_name"),
                  QString::fromStdString(plan.target_worksheet_name));
    object.insert(QStringLiteral("source_snapshot_time"),
                  QString::fromStdString(plan.source_snapshot_time));
    return object;
}

domain::ImportPlan import_plan_from_json(const QJsonObject& object)
{
    domain::ImportPlan plan;
    plan.source_connection_id =
        object.value(QStringLiteral("source_connection_id")).toString().toStdString();
    plan.provider_id = object.value(QStringLiteral("provider_id")).toString().toStdString();
    const QJsonObject ref = object.value(QStringLiteral("object_ref")).toObject();
    plan.object_ref.catalog = ref.value(QStringLiteral("catalog")).toString().toStdString();
    plan.object_ref.schema = ref.value(QStringLiteral("schema")).toString().toStdString();
    plan.object_ref.name = ref.value(QStringLiteral("name")).toString().toStdString();
    plan.object_ref.kind = static_cast<domain::DatabaseObjectKind>(
        ref.value(QStringLiteral("kind")).toInt());
    for (const QJsonValue& value : object.value(QStringLiteral("selected_columns")).toArray()) {
        plan.selected_columns.push_back(value.toString().toStdString());
    }
    for (const QJsonValue& value : object.value(QStringLiteral("column_order")).toArray()) {
        plan.column_order.push_back(value.toString().toStdString());
    }
    for (const QJsonValue& value : object.value(QStringLiteral("aliases")).toArray()) {
        plan.aliases.push_back(value.toString().toStdString());
    }
    for (const QJsonValue& value : object.value(QStringLiteral("structured_filter")).toArray()) {
        const QJsonObject item = value.toObject();
        domain::StructuredFilter filter;
        filter.column = item.value(QStringLiteral("column")).toString().toStdString();
        filter.op = item.value(QStringLiteral("op")).toString().toStdString();
        filter.value = item.value(QStringLiteral("value")).toString().toStdString();
        plan.structured_filter.push_back(std::move(filter));
    }
    plan.order_key = object.value(QStringLiteral("order_key")).toString().toStdString();
    if (object.contains(QStringLiteral("row_limit"))) {
        plan.row_limit = static_cast<std::uint64_t>(object.value(QStringLiteral("row_limit")).toInteger());
    }
    if (object.contains(QStringLiteral("row_offset"))) {
        plan.row_offset = static_cast<std::uint64_t>(object.value(QStringLiteral("row_offset")).toInteger());
    }
    if (object.contains(QStringLiteral("keyset_after"))) {
        const QJsonObject keyset = object.value(QStringLiteral("keyset_after")).toObject();
        domain::KeysetCursor cursor;
        const QJsonArray columns = keyset.value(QStringLiteral("columns")).toArray();
        const QJsonArray values = keyset.value(QStringLiteral("after_values")).toArray();
        if (!columns.isEmpty()) {
            for (const QJsonValue& value : columns) {
                cursor.columns.push_back(value.toString().toStdString());
            }
            for (const QJsonValue& value : values) {
                cursor.after_values.push_back(value.toString().toStdString());
            }
        } else if (keyset.contains(QStringLiteral("column"))) {
            cursor = domain::KeysetCursor::single(
                keyset.value(QStringLiteral("column")).toString().toStdString(),
                keyset.value(QStringLiteral("after_value")).toString().toStdString());
        }
        if (!cursor.empty()) {
            plan.keyset_after = cursor;
        }
    }
    if (object.contains(QStringLiteral("page_size"))) {
        plan.page_size = static_cast<std::uint64_t>(object.value(QStringLiteral("page_size")).toInteger());
    }
    plan.null_policy = static_cast<domain::DatabaseNullPolicy>(
        object.value(QStringLiteral("null_policy")).toInt());
    plan.target_worksheet_name =
        object.value(QStringLiteral("target_worksheet_name")).toString().toStdString();
    plan.source_snapshot_time =
        object.value(QStringLiteral("source_snapshot_time")).toString().toStdString();
    return plan;
}

}  // namespace datalab::infrastructure
