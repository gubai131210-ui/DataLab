#include "domain/report_types.h"
#include "domain/quality_types.h"

#include <QtTest/QtTest>

#include <cmath>
#include <string>

using datalab::domain::CapabilityFacts;
using datalab::domain::InterpretationFacts;
using datalab::domain::OutputPage;
using datalab::domain::PdfComplianceStatus;
using datalab::domain::ReportProfile;
using datalab::domain::ReportTemplateKind;
using datalab::domain::make_report_profile;
using datalab::domain::report_profile_changes_facts;
using datalab::domain::report_template_kind_id;

namespace {

OutputPage sample_page()
{
    OutputPage page;
    page.id = "page-capability-1";
    page.title = "过程能力";
    page.method_name = "Capability Analysis";
    page.method_metadata.algorithm = "capability_normal";
    page.method_metadata.version = "1";
    page.method_metadata.valid_count = 42;
    page.method_metadata.missing_count = 2;
    page.method_metadata.assumption_status = "not_verified";

    CapabilityFacts capability;
    capability.cp = 1.33;
    capability.cpk = 1.21;
    capability.ppk = 1.18;
    capability.assumption_status = "not_verified";
    capability.method = "normal";
    page.facts.capability = capability;
    return page;
}

bool same_capability_numbers(
    const InterpretationFacts& left, const InterpretationFacts& right)
{
    if (!left.capability.has_value() || !right.capability.has_value()) {
        return false;
    }
    const CapabilityFacts& a = *left.capability;
    const CapabilityFacts& b = *right.capability;
    const auto eq = [](const std::optional<double>& x, const std::optional<double>& y) {
        if (!x.has_value() && !y.has_value()) {
            return true;
        }
        if (!x.has_value() || !y.has_value()) {
            return false;
        }
        return std::fabs(*x - *y) < 1e-15;
    };
    return eq(a.cp, b.cp) && eq(a.cpk, b.cpk) && eq(a.ppk, b.ppk)
        && a.assumption_status == b.assumption_status && a.method == b.method;
}

}  // namespace

class ReportContractPhase0Test final : public QObject {
    Q_OBJECT

private slots:
    void profile_presets_are_distinct()
    {
        const ReportProfile customer = make_report_profile(ReportTemplateKind::customer);
        const ReportProfile engineer = make_report_profile(ReportTemplateKind::engineer);
        const ReportProfile audit = make_report_profile(ReportTemplateKind::audit);

        QCOMPARE(customer.profile_id, QStringLiteral("customer"));
        QCOMPARE(engineer.profile_id, QStringLiteral("engineer"));
        QCOMPARE(audit.profile_id, QStringLiteral("audit"));

        QVERIFY(customer.include_key_risks_and_limits);
        QVERIFY(!customer.include_parameters);
        QVERIFY(!customer.include_source_hashes);

        QVERIFY(engineer.include_parameters);
        QVERIFY(engineer.include_rule_evidence);
        QVERIFY(engineer.include_anomaly_rows);

        QVERIFY(audit.include_input_snapshot);
        QVERIFY(audit.include_import_plan);
        QVERIFY(audit.include_source_hashes);
        QVERIFY(audit.include_full_evidence_appendix);
    }

    void template_switch_does_not_change_facts()
    {
        QVERIFY(!report_profile_changes_facts());

        OutputPage page = sample_page();
        const InterpretationFacts original = page.facts;
        const std::string original_assumption = page.facts.capability->assumption_status;
        const double original_cpk = *page.facts.capability->cpk;

        // Phase 0: profiles are display policy only. Holding three profiles must not
        // require mutating OutputPage::facts. Phase 1 will replace this with apply/render.
        const ReportProfile customer = make_report_profile(ReportTemplateKind::customer);
        const ReportProfile engineer = make_report_profile(ReportTemplateKind::engineer);
        const ReportProfile audit = make_report_profile(ReportTemplateKind::audit);
        QVERIFY(customer.profile_id != engineer.profile_id);
        QVERIFY(engineer.profile_id != audit.profile_id);

        QVERIFY(same_capability_numbers(page.facts, original));
        QCOMPARE(*page.facts.capability->cpk, original_cpk);
        QCOMPARE(page.facts.capability->assumption_status, original_assumption);
        QCOMPARE(page.method_metadata.valid_count, static_cast<std::size_t>(42));
        QCOMPARE(page.method_metadata.version, std::string("1"));
    }

    void locale_defaults_are_report_scoped()
    {
        const ReportProfile profile = make_report_profile(ReportTemplateKind::engineer);
        QCOMPARE(profile.locale.language_tag, std::string("zh-CN"));
        QCOMPARE(profile.locale.timezone_id, std::string("Asia/Shanghai"));
        // Report locale is stored on the profile; UI language must not overwrite it in Phase 3.
        QVERIFY(!profile.locale.language_tag.empty());
    }

    void pdf_compliance_defaults_are_not_validated()
    {
        datalab::domain::ReportExportManifest manifest;
        QCOMPARE(
            static_cast<int>(manifest.pdfa_status),
            static_cast<int>(PdfComplianceStatus::not_validated));
        QCOMPARE(
            static_cast<int>(manifest.pdfua_status),
            static_cast<int>(PdfComplianceStatus::not_validated));
        QVERIFY(manifest.validator_name.empty());
    }

    void template_kind_ids_are_stable()
    {
        QCOMPARE(
            report_template_kind_id(ReportTemplateKind::customer), std::string("customer"));
        QCOMPARE(
            report_template_kind_id(ReportTemplateKind::engineer), std::string("engineer"));
        QCOMPARE(report_template_kind_id(ReportTemplateKind::audit), std::string("audit"));
    }
};

QTEST_MAIN(ReportContractPhase0Test)
#include "report_contract_phase0_test.moc"
