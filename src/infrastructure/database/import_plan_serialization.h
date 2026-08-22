#pragma once

#include "domain/database_types.h"

#include <QJsonObject>

namespace datalab::infrastructure {

QJsonObject import_plan_to_json(const domain::ImportPlan& plan);
domain::ImportPlan import_plan_from_json(const QJsonObject& object);

}  // namespace datalab::infrastructure
