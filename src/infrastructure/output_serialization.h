#pragma once

#include "domain/quality_types.h"

#include <QJsonObject>
#include <QString>

namespace datalab::infrastructure {

QJsonObject output_page_to_json(const domain::OutputPage& page);
domain::OutputPage output_page_from_json(const QJsonObject& object);

inline QJsonObject serialize_output_page(const domain::OutputPage& page)
{
    return output_page_to_json(page);
}

inline domain::OutputPage deserialize_output_page(const QJsonObject& object)
{
    return output_page_from_json(object);
}

}  // namespace datalab::infrastructure
