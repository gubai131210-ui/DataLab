#pragma once

#include "domain/report_types.h"

#include <QJsonObject>

namespace datalab::infrastructure {

QJsonObject report_profile_to_json(const domain::ReportProfile& profile);
domain::ReportProfile report_profile_from_json(const QJsonObject& object);

QJsonObject evidence_bundle_to_json(const domain::EvidenceBundle& bundle);
domain::EvidenceBundle evidence_bundle_from_json(const QJsonObject& object);

QJsonObject report_document_to_json(const domain::ReportDocument& document);
domain::ReportDocument report_document_from_json(const QJsonObject& object);

QJsonObject report_export_manifest_to_json(const domain::ReportExportManifest& manifest);
domain::ReportExportManifest report_export_manifest_from_json(const QJsonObject& object);

}  // namespace datalab::infrastructure
