#pragma once

#include "domain/quality_types.h"

#include <QJsonObject>
#include <QString>

namespace datalab::infrastructure {

QJsonObject output_page_to_json(const domain::OutputPage& page);
domain::OutputPage output_page_from_json(const QJsonObject& object);

}  // namespace datalab::infrastructure
