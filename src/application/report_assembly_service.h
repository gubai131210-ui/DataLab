#pragma once

#include "domain/quality_types.h"
#include "domain/report_types.h"

#include <string>
#include <vector>

namespace datalab::application {

struct ReportAssemblyOptions {
    std::string software_version = "DataLab";
    std::string generated_at_utc;
    std::string report_id_prefix = "report";
};

// Stable non-cryptographic digest for provenance / Facts fingerprinting.
// Not a security hash; used for audit consistency checks only.
std::string stable_content_digest(const std::string& text);

std::string facts_fingerprint(const domain::InterpretationFacts& facts);
std::string input_snapshot_fingerprint(const domain::DataTable& table);

std::vector<domain::RuleEvidence> collect_page_rules(const domain::OutputPage& page);

domain::EvidenceBundle build_evidence_bundle(
    const domain::DataTable& table,
    const domain::OutputPage& page,
    const domain::ReportProvenance& provenance,
    const domain::ReportProfile& profile);

domain::ReportPageView apply_report_profile(
    const domain::OutputPage& page,
    const domain::EvidenceBundle& evidence,
    const domain::ReportProfile& profile);

domain::ReportDocument build_report_document(
    const domain::DataTable& table,
    const std::vector<domain::OutputPage>& pages,
    const domain::ReportProfile& profile,
    const ReportAssemblyOptions& options = {});

// Returns true only when every page's source_page.facts matches the originals.
bool report_document_preserves_facts(
    const domain::ReportDocument& document,
    const std::vector<domain::OutputPage>& original_pages);

}  // namespace datalab::application
