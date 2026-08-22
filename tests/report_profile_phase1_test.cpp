#include "application/report_assembly_service.h"
#include "domain/quality_types.h"
#include "domain/report_types.h"
#include "infrastructure/report_serialization.h"

#include <QtTest/QtTest>

using datalab::application::ReportAssemblyOptions;
using datalab::application::build_report_document;
using datalab::application::facts_fingerprint;
using datalab::application::report_document_preserves_facts;
using datalab::domain::CapabilityFacts;
using datalab::domain::DataTable;
using datalab::domain::DiagnosticMessage;
using datalab::domain::OutputPage;
using datalab::domain::ReportDocument;
using datalab::domain::ReportProfile;
using datalab::domain::ReportTemplateKind;
using datalab::domain::RuleEvidence;
using datalab::domain::StatisticTable;
using datalab::domain::make_report_profile;
using datalab::infrastructure::report_document_from_json;
using datalab::infrastructure::report_document_to_json;

namespace {

DataTable sample_table()
{
    DataTable table;
    table.name = "PinLength";
    table.source_path = "samples/capability/PinLength.csv";
    table.import_metadata.dataset_id = "phase0_report_capability_pin";
    table.import_metadata.filter_summary = "none";
    table.import_metadata.provider_id.clear();
    table.columns = {"Length"};
    table.rows = {{"10.1"}, {"10.2"}, {"9.9"}};
    table.row_ids = {1, 2, 3};
    return table;
}

DataTable table_without_row_ids()
{
    DataTable table = sample_table();
    table.row_ids.clear();
    return table;
}

OutputPage sample_page()
{
    OutputPage page;
    page.id = "cap-1";
    page.title = "过程能力";
    page.method_name = "Capability Analysis";
    page.parameter_summary = "LSL=9;USL=11";
    page.method_metadata.algorithm = "capability_normal";
    page.method_metadata.version = "2";
    page.method_metadata.parameters = "LSL=9;USL=11";
    page.method_metadata.valid_count = 3;
    page.method_metadata.assumption_status = "not_verified";
    page.method_metadata.source_rows = {1, 2, 3};

    CapabilityFacts capability;
    capability.cp = 1.33;
    capability.cpk = 1.21;
    capability.ppk = 1.18;
    capability.assumption_status = "not_verified";
    capability.method = "normal";
    page.facts.capability = capability;

    page.interpretation.push_back(
        {"结论", {"Cpk=1.21，假设未验证"}, DiagnosticMessage::Severity::info});
    page.diagnostics.push_back(
        {DiagnosticMessage::Severity::warning,
         "stability_unverified",
         "稳定性未验证，不得宣称过程合格",
         {2},
         {},
         "",
         "先完成控制图评估"});
    page.diagnostics.push_back(
        {DiagnosticMessage::Severity::info,
         "info_note",
         "技术说明：使用总体标准差",
         {},
         {},
         "",
         ""});

    StatisticTable table;
    table.title = "能力指数";
    table.headers = {"指标", "值"};
    for (int i = 0; i < 30; ++i) {
        table.rows.push_back({"row", std::to_string(i)});
    }
    page.tables.push_back(table);

    RuleEvidence rule;
    rule.id = "beyond_control_limit";
    rule.name = "超控制限";
    rule.status = "triggered";
    rule.threshold = "3σ";
    rule.message = "存在超限点";
    rule.related_rows = {2};
    page.facts.spc = datalab::domain::SpcFacts{};
    page.facts.spc->out_of_control_count = 1;
    page.facts.spc->rules.push_back(rule);
    return page;
}

OutputPage page_without_rules()
{
    OutputPage page = sample_page();
    page.facts.spc.reset();
    page.diagnostics.clear();
    return page;
}

}  // namespace

class ReportProfilePhase1Test final : public QObject {
    Q_OBJECT

private slots:
    void three_templates_preserve_facts()
    {
        const DataTable table = sample_table();
        const std::vector<OutputPage> pages{sample_page()};
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-21T10:00:00Z";

        const ReportDocument customer = build_report_document(
            table, pages, make_report_profile(ReportTemplateKind::customer), options);
        const ReportDocument engineer = build_report_document(
            table, pages, make_report_profile(ReportTemplateKind::engineer), options);
        const ReportDocument audit = build_report_document(
            table, pages, make_report_profile(ReportTemplateKind::audit), options);

        QVERIFY(report_document_preserves_facts(customer, pages));
        QVERIFY(report_document_preserves_facts(engineer, pages));
        QVERIFY(report_document_preserves_facts(audit, pages));

        QCOMPARE(
            facts_fingerprint(customer.pages[0].source_page.facts),
            facts_fingerprint(engineer.pages[0].source_page.facts));
        QCOMPARE(
            facts_fingerprint(engineer.pages[0].source_page.facts),
            facts_fingerprint(audit.pages[0].source_page.facts));
        QCOMPARE(*customer.pages[0].source_page.facts.capability->cpk, 1.21);
        QCOMPARE(*audit.pages[0].source_page.facts.capability->cpk, 1.21);
    }

    void customer_keeps_risks_hides_tech_tables()
    {
        const ReportDocument customer = build_report_document(
            sample_table(),
            {sample_page()},
            make_report_profile(ReportTemplateKind::customer));
        QVERIFY(customer.pages[0].visible_tables.empty());
        QVERIFY(!customer.pages[0].show_parameter_summary);
        QVERIFY(!customer.pages[0].visible_diagnostics.empty());
        QCOMPARE(
            customer.pages[0].visible_diagnostics.front().code,
            std::string("stability_unverified"));
        for (const DiagnosticMessage& diagnostic : customer.pages[0].visible_diagnostics) {
            QVERIFY(
                diagnostic.severity == DiagnosticMessage::Severity::warning
                || diagnostic.severity == DiagnosticMessage::Severity::error);
        }
    }

    void engineer_shows_parameters_rules_and_anomaly_rows()
    {
        const ReportDocument engineer = build_report_document(
            sample_table(),
            {sample_page()},
            make_report_profile(ReportTemplateKind::engineer));
        QVERIFY(engineer.pages[0].show_parameter_summary);
        QVERIFY(!engineer.pages[0].visible_tables.empty());
        QVERIFY(!engineer.pages[0].visible_rules.empty());
        QCOMPARE(engineer.pages[0].visible_rules.front().id, std::string("beyond_control_limit"));
        QCOMPARE(engineer.pages[0].visible_rules.front().related_rows.front(), datalab::domain::RowId{2});
        QVERIFY(engineer.pages[0].visible_diagnostics.size() >= 2);
    }

    void audit_shows_snapshot_filter_version_and_hashes()
    {
        ReportAssemblyOptions options;
        options.generated_at_utc = "2026-08-21T10:00:00Z";
        const ReportDocument audit = build_report_document(
            sample_table(),
            {sample_page()},
            make_report_profile(ReportTemplateKind::audit),
            options);
        QCOMPARE(audit.provenance.source_kind, std::string("worksheet_snapshot"));
        QVERIFY(!audit.provenance.input_snapshot_hash.empty());
        QVERIFY(!audit.provenance.facts_hash.empty());
        QVERIFY(audit.profile.include_import_plan);
        QVERIFY(audit.profile.include_source_hashes);
        QVERIFY(audit.pages[0].show_evidence_appendix);
        QVERIFY(audit.pages[0].show_hashes);
    }

    void json_round_trip_preserves_template_and_evidence_ids()
    {
        const ReportDocument original = build_report_document(
            sample_table(),
            {sample_page()},
            make_report_profile(ReportTemplateKind::audit));
        const ReportDocument restored =
            report_document_from_json(report_document_to_json(original));
        QCOMPARE(restored.profile.profile_id, original.profile.profile_id);
        QCOMPARE(
            static_cast<int>(restored.profile.template_kind),
            static_cast<int>(original.profile.template_kind));
        QCOMPARE(restored.provenance.facts_hash, original.provenance.facts_hash);
        QVERIFY(!original.evidence.evidence.empty());
        QCOMPARE(
            restored.evidence.evidence.front().evidence_id,
            original.evidence.evidence.front().evidence_id);
        QCOMPARE(
            facts_fingerprint(restored.pages[0].source_page.facts),
            facts_fingerprint(original.pages[0].source_page.facts));
    }

    void provenance_tracks_excluded_and_hidden_separately()
    {
        OutputPage page = sample_page();
        page.configuration.excluded_rows = {0};
        page.configuration.hidden_rows = {1, 2};
        const ReportDocument document = build_report_document(
            sample_table(),
            {page},
            make_report_profile(ReportTemplateKind::audit));
        QCOMPARE(document.provenance.excluded_row_count, std::size_t{1});
        QCOMPARE(document.provenance.hidden_row_count, std::size_t{2});
        QVERIFY(document.provenance.filter_summary.find("excluded=1") != std::string::npos);
        QVERIFY(document.provenance.filter_summary.find("hidden=2") != std::string::npos);
        bool saw_row_selection = false;
        for (const auto& ref : document.evidence.evidence) {
            if (ref.kind == datalab::domain::EvidenceKind::row_selection) {
                saw_row_selection = true;
                QCOMPARE(ref.status, std::string("present"));
            }
        }
        QVERIFY(saw_row_selection);
    }

    void empty_and_multiple_evidence_and_missing_row_ids()
    {
        const ReportDocument empty_rules = build_report_document(
            sample_table(),
            {page_without_rules()},
            make_report_profile(ReportTemplateKind::engineer));
        QVERIFY(!empty_rules.evidence.evidence.empty());

        const ReportDocument missing_rows = build_report_document(
            table_without_row_ids(),
            {sample_page()},
            make_report_profile(ReportTemplateKind::audit));
        bool saw_missing_row_status = false;
        for (const auto& ref : missing_rows.evidence.evidence) {
            if (ref.kind == datalab::domain::EvidenceKind::dataset_snapshot
                && ref.status == "missing") {
                saw_missing_row_status = true;
            }
        }
        QVERIFY(saw_missing_row_status);
    }

    void evidence_beyond_preview_is_truncated()
    {
        ReportProfile profile = make_report_profile(ReportTemplateKind::customer);
        profile.max_evidence_rows = 2;
        profile.max_preview_rows = 5;
        // Keep appendix path off so customer filter still applies after truncate.
        const ReportDocument document =
            build_report_document(sample_table(), {sample_page()}, profile);
        // Engineer-like truncation check via custom max on audit appendix path:
        ReportProfile audit = make_report_profile(ReportTemplateKind::audit);
        audit.max_evidence_rows = 3;
        audit.max_preview_rows = 5;
        const ReportDocument truncated =
            build_report_document(sample_table(), {sample_page()}, audit);
        QVERIFY(truncated.pages[0].truncated_evidence_count > 0
                || truncated.pages[0].visible_evidence.size() <= 3);
        QVERIFY(truncated.pages[0].truncated_table_row_count > 0);
        QCOMPARE(truncated.pages[0].visible_tables.front().rows.size(), std::size_t{5});
    }

    void snapshot_source_explained_without_database()
    {
        DataTable table = sample_table();
        table.import_metadata.provider_id.clear();
        table.source_path = "archived/snapshot.csv";
        const ReportDocument audit = build_report_document(
            table, {sample_page()}, make_report_profile(ReportTemplateKind::audit));
        QCOMPARE(audit.provenance.source_kind, std::string("worksheet_snapshot"));
        QVERIFY(!audit.provenance.source_path.empty());
        QVERIFY(!audit.provenance.input_snapshot_hash.empty());
    }

    void chart_embed_limits_and_plot_evidence_respect_visibility()
    {
        OutputPage page = sample_page();
        page.configuration.excluded_rows = {1};
        page.configuration.hidden_rows = {2};
        datalab::domain::PlotSpec primary;
        primary.title = "主图";
        primary.source_rows = {0, 2};
        datalab::domain::PlotSpec secondary;
        secondary.title = "辅图";
        secondary.source_rows = {0};
        page.plots = {primary, secondary};

        const ReportDocument customer = build_report_document(
            sample_table(), {page}, make_report_profile(ReportTemplateKind::customer));
        QCOMPARE(customer.pages[0].visible_plots.size(), std::size_t{1});
        QCOMPARE(customer.pages[0].visible_plots.front().title, std::string("主图"));
        QVERIFY(customer.pages[0].visible_plots.front().subtitle.find("excluded=1")
                != std::string::npos);
        QVERIFY(customer.pages[0].visible_plots.front().subtitle.find("hidden=1")
                != std::string::npos);

        const ReportDocument engineer = build_report_document(
            sample_table(), {page}, make_report_profile(ReportTemplateKind::engineer));
        QCOMPARE(engineer.pages[0].visible_plots.size(), std::size_t{2});

        bool saw_plot_evidence = false;
        for (const auto& ref : engineer.evidence.evidence) {
            if (ref.kind == datalab::domain::EvidenceKind::plot) {
                saw_plot_evidence = true;
                bool saw_excluded = false;
                bool saw_hidden = false;
                for (const auto& key : ref.parameter_keys) {
                    if (key.find("excluded_row_count=1") != std::string::npos) {
                        saw_excluded = true;
                    }
                    if (key.find("hidden_row_count=1") != std::string::npos) {
                        saw_hidden = true;
                    }
                }
                QVERIFY(saw_excluded);
                QVERIFY(saw_hidden);
            }
        }
        QVERIFY(saw_plot_evidence);
        QVERIFY(report_document_preserves_facts(engineer, {page}));
        QCOMPARE(make_report_profile(ReportTemplateKind::customer).max_plots, std::size_t{1});
        QCOMPARE(make_report_profile(ReportTemplateKind::engineer).max_plots, std::size_t{8});
        QCOMPARE(make_report_profile(ReportTemplateKind::audit).max_plots, std::size_t{16});

        const auto json = report_document_to_json(engineer);
        const auto restored = report_document_from_json(json);
        QCOMPARE(restored.profile.max_plots, engineer.profile.max_plots);
    }

    void formula_reference_harvest_respects_profile_and_never_invents_vendor_oracle()
    {
        OutputPage page = sample_page();
        page.facts.capability->mixture_status = "preferred_2comp";
        page.facts.capability->mixture_algorithm_id = "gaussian_mixture_k_bic";
        page.facts.capability->mixture_evidence_type = "formula_reference";
        page.facts.capability->hartigan_dip_status = "evidence_against";

        datalab::domain::ReliabilityFacts reliability;
        reliability.distribution = "weibull";
        reliability.evidence_type = "formula_reference";
        reliability.cif_algorithm_id = "aalen_johansen_cif";
        reliability.cif_evidence_type = "formula_reference";
        reliability.fine_gray_algorithm_id = "fine_gray_binary_ipcw";
        reliability.fine_gray_evidence_type = "formula_reference";
        datalab::domain::ReliabilityModeFitFacts mode;
        mode.failure_mode = "A";
        mode.evidence_type = "formula_reference";
        mode.algorithm_id = "cause_specific_right_censored_competing";
        reliability.mode_fits.push_back(mode);
        page.facts.reliability = reliability;

        datalab::domain::KmIntervalFacts km;
        km.evidence_type = "formula_reference";
        km.algorithm_id = "turnbull_npmle_simplified_grid";
        page.facts.km_interval = km;

        datalab::domain::DesignGenerationFacts design;
        design.design_kind = "ccd";
        design.design_source_id = "ccd_ccc_k2";
        design.evidence_type = "formula_reference";
        page.facts.design_generation = design;

        datalab::domain::RsmFacts rsm;
        rsm.evidence_type = "formula_reference";
        page.facts.rsm = rsm;

        datalab::domain::WarrantyFacts warranty;
        warranty.evidence_type = "formula_reference";
        page.facts.warranty = warranty;

        // Declared non-formula type must not invent vendor_oracle EvidenceKind.
        OutputPage poisoned = page;
        poisoned.facts.capability->evidence_type = "vendor_oracle";

        const ReportDocument customer = build_report_document(
            sample_table(), {page}, make_report_profile(ReportTemplateKind::customer));
        for (const auto& ref : customer.evidence.evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::formula_reference);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
        }
        for (const auto& ref : customer.pages[0].visible_evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::formula_reference);
        }

        const ReportDocument engineer = build_report_document(
            sample_table(), {page}, make_report_profile(ReportTemplateKind::engineer));
        QVERIFY(engineer.profile.include_formula_references);
        QVERIFY(!engineer.profile.include_full_evidence_appendix);

        bool saw_capability = false;
        bool saw_mixture = false;
        bool saw_hartigan = false;
        bool saw_reliability = false;
        bool saw_mode = false;
        bool saw_cif = false;
        bool saw_fg = false;
        bool saw_km = false;
        bool saw_design = false;
        bool saw_rsm = false;
        bool saw_warranty = false;
        for (const auto& ref : engineer.evidence.evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::golden);
            if (ref.kind != datalab::domain::EvidenceKind::formula_reference) {
                continue;
            }
            if (ref.evidence_id.find(":formula:capability") != std::string::npos) {
                saw_capability = true;
                QCOMPARE(ref.formula_ref_id, std::string("normal"));
            }
            if (ref.evidence_id.find(":formula:mixture") != std::string::npos) {
                saw_mixture = true;
                QCOMPARE(ref.formula_ref_id, std::string("gaussian_mixture_k_bic"));
            }
            if (ref.evidence_id.find(":formula:hartigan_dip") != std::string::npos) {
                saw_hartigan = true;
            }
            if (ref.evidence_id.find(":formula:reliability") != std::string::npos
                && ref.evidence_id.find("mode_fit") == std::string::npos) {
                saw_reliability = true;
            }
            if (ref.evidence_id.find(":formula:mode_fit:A") != std::string::npos) {
                saw_mode = true;
            }
            if (ref.evidence_id.find(":formula:cif") != std::string::npos) {
                saw_cif = true;
                QCOMPARE(ref.formula_ref_id, std::string("aalen_johansen_cif"));
            }
            if (ref.evidence_id.find(":formula:fine_gray") != std::string::npos) {
                saw_fg = true;
            }
            if (ref.evidence_id.find(":formula:km_interval") != std::string::npos) {
                saw_km = true;
            }
            if (ref.evidence_id.find(":formula:design_generation") != std::string::npos) {
                saw_design = true;
                QCOMPARE(ref.formula_ref_id, std::string("ccd_ccc_k2"));
            }
            if (ref.evidence_id.find(":formula:rsm") != std::string::npos) {
                saw_rsm = true;
                QCOMPARE(ref.formula_ref_id, std::string("rsm"));
            }
            if (ref.evidence_id.find(":formula:warranty") != std::string::npos) {
                saw_warranty = true;
                QCOMPARE(ref.formula_ref_id, std::string("warranty_claims_per_1000"));
            }
        }
        QVERIFY(saw_capability);
        QVERIFY(saw_mixture);
        QVERIFY(saw_hartigan);
        QVERIFY(saw_reliability);
        QVERIFY(saw_mode);
        QVERIFY(saw_cif);
        QVERIFY(saw_fg);
        QVERIFY(saw_km);
        QVERIFY(saw_design);
        QVERIFY(saw_rsm);
        QVERIFY(saw_warranty);

        bool visible_has_formula = false;
        for (const auto& ref : engineer.pages[0].visible_evidence) {
            if (ref.kind == datalab::domain::EvidenceKind::formula_reference) {
                visible_has_formula = true;
            }
        }
        QVERIFY(visible_has_formula);
        QVERIFY(report_document_preserves_facts(engineer, {page}));

        const ReportDocument audit = build_report_document(
            sample_table(), {page}, make_report_profile(ReportTemplateKind::audit));
        QVERIFY(audit.profile.include_formula_references);
        QVERIFY(audit.profile.include_full_evidence_appendix);
        QVERIFY(audit.pages[0].show_evidence_appendix);
        bool audit_formula = false;
        for (const auto& ref : audit.pages[0].visible_evidence) {
            if (ref.kind == datalab::domain::EvidenceKind::formula_reference) {
                audit_formula = true;
            }
        }
        QVERIFY(audit_formula);

        const ReportDocument poisoned_doc = build_report_document(
            sample_table(),
            {poisoned},
            make_report_profile(ReportTemplateKind::engineer));
        for (const auto& ref : poisoned_doc.evidence.evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
            if (ref.evidence_id.find(":formula:capability") != std::string::npos
                && ref.evidence_id.find("mixture") == std::string::npos
                && ref.evidence_id.find("hartigan") == std::string::npos) {
                QFAIL("vendor_oracle Facts must not harvest a capability formula ref");
            }
        }
        QVERIFY(report_document_preserves_facts(poisoned_doc, {poisoned}));
    }

    void customer_keeps_capability_gate_limiting_evidence_under_truncation()
    {
        OutputPage page = sample_page();
        page.facts.capability->pass_fail_judgment_allowed = false;
        page.facts.capability->gate_status = "gated_research";
        page.facts.capability->research_preview = true;
        page.facts.capability->method = "johnson";
        page.facts.capability->johnson_family = "SU";
        page.facts.capability->evidence_type = "formula_reference";
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "johnson_capability_gated",
            "Johnson 能力为研究/预览结果：可显示拟合与变换尺度指数，"
            "但不得输出过程合格判定（门禁：缺 golden/尾部/人工解释验收）。"});

        // Flood supporting plot evidence beyond customer max_evidence_rows.
        page.plots.clear();
        for (int i = 0; i < 30; ++i) {
            datalab::domain::PlotSpec plot;
            plot.title = "flood-" + std::to_string(i);
            plot.source_rows = {0, 1, 2};
            page.plots.push_back(plot);
        }

        ReportProfile customer_profile = make_report_profile(ReportTemplateKind::customer);
        QCOMPARE(customer_profile.max_evidence_rows, std::size_t{20});
        const ReportDocument customer =
            build_report_document(sample_table(), {page}, customer_profile);

        bool saw_gate = false;
        for (const auto& ref : customer.pages[0].visible_evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::golden);
            if (ref.evidence_id.find(":gate:johnson") != std::string::npos) {
                saw_gate = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(ref.diagnostic_code, std::string("johnson_capability_gated"));
                QCOMPARE(ref.label_text_id, std::string("evidence.johnson_capability_gated"));
                bool saw_pass_fail = false;
                bool saw_gate_status = false;
                bool saw_formula = false;
                for (const std::string& key : ref.parameter_keys) {
                    if (key == "pass_fail_judgment_allowed=false") {
                        saw_pass_fail = true;
                    }
                    if (key == "gate_status=gated_research") {
                        saw_gate_status = true;
                    }
                    if (key == "evidence_type=formula_reference") {
                        saw_formula = true;
                    }
                    QVERIFY(key.find("vendor_oracle") == std::string::npos);
                }
                QVERIFY(saw_pass_fail);
                QVERIFY(saw_gate_status);
                QVERIFY(saw_formula);
            }
        }
        QVERIFY2(saw_gate, "customer visible_evidence must retain Johnson gate limiting ref");
        QVERIFY(customer.pages[0].truncated_evidence_count > 0);

        // Stability-prerequisite path (non-Johnson) also survives flood.
        OutputPage blocked = sample_page();
        blocked.facts.capability->pass_fail_judgment_allowed = false;
        blocked.facts.capability->gate_status = "stability_unverified";
        blocked.facts.capability->method = "normal";
        blocked.plots.clear();
        for (int i = 0; i < 30; ++i) {
            datalab::domain::PlotSpec plot;
            plot.title = "flood-b-" + std::to_string(i);
            plot.source_rows = {0};
            blocked.plots.push_back(plot);
        }
        const ReportDocument blocked_customer =
            build_report_document(sample_table(), {blocked}, customer_profile);
        bool saw_blocked = false;
        for (const auto& ref : blocked_customer.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:capability_pass_fail_blocked")
                != std::string::npos) {
                saw_blocked = true;
                QCOMPARE(
                    ref.diagnostic_code,
                    std::string("capability_pass_fail_blocked_by_stability_prerequisite"));
            }
        }
        QVERIFY(saw_blocked);

        // Reliability / warranty / KM honesty refs.
        OutputPage honesty = sample_page();
        honesty.facts.capability.reset();
        datalab::domain::ReliabilityFacts reliability;
        reliability.failure_count = 0;
        reliability.valid_count = 10;
        reliability.censored_count = 10;
        reliability.not_computed_reason = "all_censored";
        reliability.evidence_type = "formula_reference";
        honesty.facts.reliability = reliability;
        datalab::domain::KmIntervalFacts km;
        km.evidence_type = "formula_reference";
        honesty.facts.km_interval = km;
        datalab::domain::WarrantyFacts warranty;
        warranty.quantity_label = "prediction";
        warranty.evidence_type = "formula_reference";
        honesty.facts.warranty = warranty;
        honesty.plots.clear();
        for (int i = 0; i < 30; ++i) {
            datalab::domain::PlotSpec plot;
            plot.title = "flood-h-" + std::to_string(i);
            honesty.plots.push_back(plot);
        }
        const ReportDocument honesty_customer =
            build_report_document(sample_table(), {honesty}, customer_profile);
        bool saw_all_censored = false;
        bool saw_km = false;
        bool saw_warranty_legal = false;
        bool saw_warranty_prediction = false;
        for (const auto& ref : honesty_customer.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:reliability_all_censored") != std::string::npos) {
                saw_all_censored = true;
            }
            if (ref.evidence_id.find(":gate:km_not_long_term_guarantee") != std::string::npos) {
                saw_km = true;
            }
            if (ref.evidence_id.find(":gate:warranty_not_legal_promise") != std::string::npos) {
                saw_warranty_legal = true;
            }
            if (ref.evidence_id.find(":gate:warranty_prediction_not_observation")
                != std::string::npos) {
                saw_warranty_prediction = true;
            }
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
        }
        QVERIFY(saw_all_censored);
        QVERIFY(saw_km);
        QVERIFY(saw_warranty_legal);
        QVERIFY(saw_warranty_prediction);

        // Facts fingerprint unchanged across templates.
        const ReportDocument engineer =
            build_report_document(sample_table(), {page}, make_report_profile(ReportTemplateKind::engineer));
        QCOMPARE(
            facts_fingerprint(customer.pages[0].source_page.facts),
            facts_fingerprint(engineer.pages[0].source_page.facts));
        QCOMPARE(page.facts.capability->pass_fail_judgment_allowed, false);
        QCOMPARE(engineer.pages[0].source_page.facts.capability->pass_fail_judgment_allowed, false);

        // Poisoned Facts evidence_type must not leak onto gate parameter_keys.
        OutputPage poisoned_gate = page;
        poisoned_gate.facts.capability->evidence_type = "vendor_oracle";
        const ReportDocument poisoned_customer = build_report_document(
            sample_table(),
            {poisoned_gate},
            customer_profile);
        for (const auto& ref : poisoned_customer.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:") == std::string::npos) {
                continue;
            }
            for (const std::string& key : ref.parameter_keys) {
                QVERIFY(key.find("vendor_oracle") == std::string::npos);
                QVERIFY(key.find("golden") == std::string::npos);
                if (key.rfind("evidence_type=", 0) == 0) {
                    QCOMPARE(key, std::string("evidence_type=formula_reference"));
                }
            }
        }
    }

    void customer_keeps_rsm_lof_limiting_evidence_under_truncation()
    {
        ReportProfile customer_profile = make_report_profile(ReportTemplateKind::customer);
        QCOMPARE(customer_profile.max_evidence_rows, std::size_t{20});

        auto flood_plots = [](OutputPage& page) {
            page.plots.clear();
            for (int i = 0; i < 30; ++i) {
                datalab::domain::PlotSpec plot;
                plot.title = "rsm-flood-" + std::to_string(i);
                plot.source_rows = {0, 1};
                page.plots.push_back(plot);
            }
        };

        // Path A: no pure error — must not imply residual MS as PE.
        OutputPage no_pe;
        no_pe.id = "rsm-no-pe";
        no_pe.title = "RSM";
        datalab::domain::RsmFacts rsm_no_pe;
        rsm_no_pe.pure_error_available = false;
        rsm_no_pe.lack_of_fit_available = false;
        rsm_no_pe.pure_error_df = 0;
        rsm_no_pe.lack_of_fit_df = 0;
        rsm_no_pe.surface_is_static = true;
        rsm_no_pe.design_source_id = "ccd_ccc_k2";
        rsm_no_pe.design_kind = "ccd";
        rsm_no_pe.evidence_type = "formula_reference";
        no_pe.facts.rsm = rsm_no_pe;
        flood_plots(no_pe);

        const ReportDocument no_pe_customer =
            build_report_document(sample_table(), {no_pe}, customer_profile);
        bool saw_insufficient_pe = false;
        bool saw_static = false;
        bool saw_lof_available = false;
        for (const auto& ref : no_pe_customer.pages[0].visible_evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::golden);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::formula_reference);
            if (ref.evidence_id.find(":gate:rsm_insufficient_pure_error")
                != std::string::npos) {
                saw_insufficient_pe = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(ref.diagnostic_code, std::string("rsm_insufficient_pure_error"));
                QCOMPARE(
                    ref.label_text_id,
                    std::string("evidence.rsm_insufficient_pure_error"));
                bool saw_residual_honesty = false;
                bool saw_formula = false;
                for (const std::string& key : ref.parameter_keys) {
                    if (key == "residual_ms_as_pure_error=false") {
                        saw_residual_honesty = true;
                    }
                    if (key == "evidence_type=formula_reference") {
                        saw_formula = true;
                    }
                    QVERIFY(key.find("vendor_oracle") == std::string::npos);
                }
                QVERIFY(saw_residual_honesty);
                QVERIFY(saw_formula);
            }
            if (ref.evidence_id.find(":gate:rsm_lof_formula_reference")
                != std::string::npos) {
                saw_lof_available = true;
            }
            if (ref.evidence_id.find(":gate:rsm_static_surface") != std::string::npos) {
                saw_static = true;
            }
        }
        QVERIFY2(saw_insufficient_pe, "customer must keep rsm_insufficient_pure_error gate");
        QVERIFY(saw_static);
        QVERIFY(!saw_lof_available);
        QVERIFY(no_pe_customer.pages[0].truncated_evidence_count > 0);

        // Path B: LOF available with pure error — limiting LOF honesty retained.
        OutputPage with_lof;
        with_lof.id = "rsm-lof";
        with_lof.title = "RSM";
        datalab::domain::RsmFacts rsm_lof;
        rsm_lof.pure_error_available = true;
        rsm_lof.lack_of_fit_available = true;
        rsm_lof.pure_error_df = 3;
        rsm_lof.lack_of_fit_df = 2;
        rsm_lof.lack_of_fit_f = 1.2;
        rsm_lof.lack_of_fit_p = 0.4;
        rsm_lof.surface_is_static = false;
        rsm_lof.evidence_type = "formula_reference";
        with_lof.facts.rsm = rsm_lof;
        flood_plots(with_lof);

        const ReportDocument lof_customer =
            build_report_document(sample_table(), {with_lof}, customer_profile);
        bool saw_lof = false;
        bool saw_pe_missing = false;
        for (const auto& ref : lof_customer.pages[0].visible_evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
            if (ref.evidence_id.find(":gate:rsm_lof_formula_reference")
                != std::string::npos) {
                saw_lof = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(
                    ref.label_text_id,
                    std::string("evidence.rsm_lof_formula_reference"));
                bool saw_pe_df = false;
                bool saw_lof_df = false;
                for (const std::string& key : ref.parameter_keys) {
                    if (key == "pure_error_df=3") {
                        saw_pe_df = true;
                    }
                    if (key == "lack_of_fit_df=2") {
                        saw_lof_df = true;
                    }
                    if (key.rfind("evidence_type=", 0) == 0) {
                        QCOMPARE(key, std::string("evidence_type=formula_reference"));
                    }
                }
                QVERIFY(saw_pe_df);
                QVERIFY(saw_lof_df);
            }
            if (ref.evidence_id.find(":gate:rsm_insufficient_pure_error")
                != std::string::npos) {
                saw_pe_missing = true;
            }
        }
        QVERIFY2(saw_lof, "customer must keep rsm_lof_formula_reference gate");
        QVERIFY(!saw_pe_missing);

        // Engineer still harvests formula:rsm; customer must not show formula refs.
        const ReportDocument engineer =
            build_report_document(
                sample_table(), {with_lof}, make_report_profile(ReportTemplateKind::engineer));
        bool engineer_formula = false;
        for (const auto& ref : engineer.evidence.evidence) {
            if (ref.evidence_id.find(":formula:rsm") != std::string::npos) {
                engineer_formula = true;
                QCOMPARE(ref.kind, datalab::domain::EvidenceKind::formula_reference);
            }
        }
        QVERIFY(engineer_formula);
        for (const auto& ref : lof_customer.pages[0].visible_evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::formula_reference);
        }

        // Poisoned RSM evidence_type must clamp on gate keys.
        OutputPage poisoned = with_lof;
        poisoned.facts.rsm->evidence_type = "vendor_oracle";
        flood_plots(poisoned);
        const ReportDocument poisoned_customer =
            build_report_document(sample_table(), {poisoned}, customer_profile);
        for (const auto& ref : poisoned_customer.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:rsm_") == std::string::npos) {
                continue;
            }
            for (const std::string& key : ref.parameter_keys) {
                QVERIFY(key.find("vendor_oracle") == std::string::npos);
                if (key.rfind("evidence_type=", 0) == 0) {
                    QCOMPARE(key, std::string("evidence_type=formula_reference"));
                }
            }
        }

        QCOMPARE(
            facts_fingerprint(lof_customer.pages[0].source_page.facts),
            facts_fingerprint(engineer.pages[0].source_page.facts));
    }

    void customer_keeps_ccd_bbd_design_limiting_evidence_under_truncation()
    {
        ReportProfile customer_profile = make_report_profile(ReportTemplateKind::customer);
        QCOMPARE(customer_profile.max_evidence_rows, std::size_t{20});

        auto flood_plots = [](OutputPage& page) {
            page.plots.clear();
            for (int i = 0; i < 30; ++i) {
                datalab::domain::PlotSpec plot;
                plot.title = "design-flood-" + std::to_string(i);
                plot.source_rows = {0, 1};
                page.plots.push_back(plot);
            }
        };

        // CCD with axial beyond-range honesty.
        OutputPage ccd;
        ccd.id = "ccd-design";
        ccd.title = "CCD";
        datalab::domain::DesignGenerationFacts ccd_facts;
        ccd_facts.design_kind = "ccd";
        ccd_facts.ccd_variant = "ccc";
        ccd_facts.design_source_id = "ccd_ccc_k2";
        ccd_facts.alpha = 1.41421356237;
        ccd_facts.beyond_range_detected = true;
        ccd_facts.allow_beyond_range = false;
        ccd_facts.evidence_type = "formula_reference";
        ccd.facts.design_generation = ccd_facts;
        flood_plots(ccd);

        const ReportDocument ccd_customer =
            build_report_document(sample_table(), {ccd}, customer_profile);
        bool saw_formula_only = false;
        bool saw_beyond = false;
        bool saw_bbd = false;
        for (const auto& ref : ccd_customer.pages[0].visible_evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::golden);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::formula_reference);
            if (ref.evidence_id.find(":gate:design_formula_reference_only")
                != std::string::npos) {
                saw_formula_only = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(
                    ref.label_text_id,
                    std::string("evidence.design_formula_reference_only"));
                bool saw_not_vendor = false;
                bool saw_commercial = false;
                for (const std::string& key : ref.parameter_keys) {
                    if (key == "not_vendor_oracle=true") {
                        saw_not_vendor = true;
                    }
                    if (key == "commercial_alignment=false") {
                        saw_commercial = true;
                    }
                    if (key.rfind("evidence_type=", 0) == 0) {
                        QCOMPARE(key, std::string("evidence_type=formula_reference"));
                    }
                }
                QVERIFY(saw_not_vendor);
                QVERIFY(saw_commercial);
            }
            if (ref.evidence_id.find(":gate:ccd_beyond_range") != std::string::npos) {
                saw_beyond = true;
                QCOMPARE(ref.diagnostic_code, std::string("ccd_ccc_beyond_range"));
                bool saw_exec = false;
                for (const std::string& key : ref.parameter_keys) {
                    if (key == "executable_process_claim=false") {
                        saw_exec = true;
                    }
                    if (key == "beyond_range_detected=true") {
                        QVERIFY(true);
                    }
                }
                QVERIFY(saw_exec);
            }
            if (ref.evidence_id.find(":gate:bbd_no_corners") != std::string::npos) {
                saw_bbd = true;
            }
        }
        QVERIFY2(saw_formula_only, "customer must keep design_formula_reference_only");
        QVERIFY2(saw_beyond, "customer must keep ccd_beyond_range under plot flood");
        QVERIFY(!saw_bbd);
        QVERIFY(ccd_customer.pages[0].truncated_evidence_count > 0);

        // BBD no-corners honesty.
        OutputPage bbd;
        bbd.id = "bbd-design";
        bbd.title = "BBD";
        datalab::domain::DesignGenerationFacts bbd_facts;
        bbd_facts.design_kind = "bbd";
        bbd_facts.design_source_id = "bbd_k3";
        bbd_facts.beyond_range_detected = false;
        bbd_facts.evidence_type = "formula_reference";
        bbd.facts.design_generation = bbd_facts;
        flood_plots(bbd);

        const ReportDocument bbd_customer =
            build_report_document(sample_table(), {bbd}, customer_profile);
        bool saw_bbd_gate = false;
        bool saw_ccd_gate = false;
        bool saw_bbd_formula_only = false;
        for (const auto& ref : bbd_customer.pages[0].visible_evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
            if (ref.evidence_id.find(":gate:bbd_no_corners") != std::string::npos) {
                saw_bbd_gate = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(ref.diagnostic_code, std::string("bbd_no_corners"));
                QCOMPARE(ref.label_text_id, std::string("evidence.bbd_no_corners"));
                bool saw_no_corners = false;
                bool saw_not_optimal = false;
                for (const std::string& key : ref.parameter_keys) {
                    if (key == "has_corner_points=false") {
                        saw_no_corners = true;
                    }
                    if (key == "domain_wide_prediction_optimal=false") {
                        saw_not_optimal = true;
                    }
                }
                QVERIFY(saw_no_corners);
                QVERIFY(saw_not_optimal);
            }
            if (ref.evidence_id.find(":gate:ccd_beyond_range") != std::string::npos) {
                saw_ccd_gate = true;
            }
            if (ref.evidence_id.find(":gate:design_formula_reference_only")
                != std::string::npos) {
                saw_bbd_formula_only = true;
            }
        }
        QVERIFY2(saw_bbd_gate, "customer must keep bbd_no_corners under plot flood");
        QVERIFY2(
            saw_bbd_formula_only,
            "BBD customer must also keep design_formula_reference_only");
        QVERIFY(!saw_ccd_gate);

        // Engineer still harvests formula:design_generation.
        const ReportDocument engineer = build_report_document(
            sample_table(), {ccd}, make_report_profile(ReportTemplateKind::engineer));
        bool engineer_formula = false;
        for (const auto& ref : engineer.evidence.evidence) {
            if (ref.evidence_id.find(":formula:design_generation") != std::string::npos) {
                engineer_formula = true;
                QCOMPARE(ref.kind, datalab::domain::EvidenceKind::formula_reference);
            }
        }
        QVERIFY(engineer_formula);

        // Poisoned design evidence_type clamps on gate keys.
        OutputPage poisoned = ccd;
        poisoned.facts.design_generation->evidence_type = "vendor_oracle";
        flood_plots(poisoned);
        const ReportDocument poisoned_customer =
            build_report_document(sample_table(), {poisoned}, customer_profile);
        for (const auto& ref : poisoned_customer.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:design_") == std::string::npos
                && ref.evidence_id.find(":gate:ccd_") == std::string::npos
                && ref.evidence_id.find(":gate:bbd_") == std::string::npos) {
                continue;
            }
            for (const std::string& key : ref.parameter_keys) {
                QVERIFY(key != "evidence_type=vendor_oracle");
                QVERIFY(key != "evidence_type=golden");
                QVERIFY(key.find("=vendor_oracle") == std::string::npos
                        || key == "not_vendor_oracle=true");
                if (key.rfind("evidence_type=", 0) == 0) {
                    QCOMPARE(key, std::string("evidence_type=formula_reference"));
                }
            }
        }

        QCOMPARE(
            facts_fingerprint(ccd_customer.pages[0].source_page.facts),
            facts_fingerprint(engineer.pages[0].source_page.facts));
    }

    void customer_keeps_cif_fine_gray_warranty_strata_limiting_evidence_under_truncation()
    {
        ReportProfile customer_profile = make_report_profile(ReportTemplateKind::customer);
        QCOMPARE(customer_profile.max_evidence_rows, std::size_t{20});

        auto flood_plots = [](OutputPage& page) {
            page.plots.clear();
            for (int i = 0; i < 30; ++i) {
                datalab::domain::PlotSpec plot;
                plot.title = "rel-flood-" + std::to_string(i);
                plot.source_rows = {0};
                page.plots.push_back(plot);
            }
        };

        OutputPage page;
        page.id = "rel-cif-fg";
        page.title = "Reliability";
        datalab::domain::ReliabilityFacts reliability;
        reliability.cif_algorithm_id = "aalen_johansen_cif";
        reliability.cif_evidence_type = "formula_reference";
        reliability.fine_gray_algorithm_id = "fine_gray_multi_ipcw";
        reliability.fine_gray_evidence_type = "formula_reference";
        reliability.fine_gray_kind = "multi";
        page.facts.reliability = reliability;

        datalab::domain::WarrantyFacts warranty;
        warranty.evidence_type = "formula_reference";
        warranty.exposure_source = "scalar";
        warranty.uses_pooled_reliability = true;
        warranty.uses_mode_specific_reliability = false;
        warranty.stratum_kind = "failure_mode";
        warranty.quantity_label = "prediction";
        datalab::domain::WarrantyStratumFacts stratum;
        stratum.label = "A";
        stratum.kind = "failure_mode";
        stratum.exposure_attribution = "proportional_scalar";
        stratum.uses_mode_specific_reliability = false;
        warranty.strata.push_back(stratum);
        page.facts.warranty = warranty;
        flood_plots(page);

        const ReportDocument customer =
            build_report_document(sample_table(), {page}, customer_profile);
        bool saw_cif = false;
        bool saw_fg = false;
        bool saw_exposure = false;
        bool saw_basis = false;
        for (const auto& ref : customer.pages[0].visible_evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::golden);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::formula_reference);
            if (ref.evidence_id.find(":gate:cif_not_fine_gray") != std::string::npos) {
                saw_cif = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(ref.label_text_id, std::string("evidence.cif_not_fine_gray"));
                bool saw_not_fg = false;
                for (const std::string& key : ref.parameter_keys) {
                    if (key == "not_fine_gray=true") {
                        saw_not_fg = true;
                    }
                    if (key.rfind("evidence_type=", 0) == 0) {
                        QCOMPARE(key, std::string("evidence_type=formula_reference"));
                    }
                }
                QVERIFY(saw_not_fg);
            }
            if (ref.evidence_id.find(":gate:fine_gray_formula_reference_only")
                != std::string::npos) {
                saw_fg = true;
                QCOMPARE(ref.diagnostic_code, std::string("fine_gray_formula_reference_only"));
                bool saw_not_cox = false;
                bool saw_not_r = false;
                for (const std::string& key : ref.parameter_keys) {
                    if (key == "not_cause_specific_cox=true") {
                        saw_not_cox = true;
                    }
                    if (key == "not_pinned_r_survival_finegray=true") {
                        saw_not_r = true;
                    }
                    if (key == "fine_gray_kind=multi") {
                        QVERIFY(true);
                    }
                }
                QVERIFY(saw_not_cox);
                QVERIFY(saw_not_r);
            }
            if (ref.evidence_id.find(":gate:warranty_strata_exposure_honesty")
                != std::string::npos) {
                saw_exposure = true;
                bool saw_prop = false;
                bool saw_measured = false;
                for (const std::string& key : ref.parameter_keys) {
                    if (key == "proportional_scalar=true") {
                        saw_prop = true;
                    }
                    if (key == "measured_denominator=false") {
                        saw_measured = true;
                    }
                }
                QVERIFY(saw_prop);
                QVERIFY(saw_measured);
            }
            if (ref.evidence_id.find(":gate:warranty_strata_reliability_basis")
                != std::string::npos) {
                saw_basis = true;
                bool saw_pooled = false;
                bool saw_not_mode = false;
                for (const std::string& key : ref.parameter_keys) {
                    if (key == "uses_pooled_reliability=true") {
                        saw_pooled = true;
                    }
                    if (key == "pooled_as_mode_specific=false") {
                        saw_not_mode = true;
                    }
                }
                QVERIFY(saw_pooled);
                QVERIFY(saw_not_mode);
            }
        }
        QVERIFY2(saw_cif, "customer must keep cif_not_fine_gray under plot flood");
        QVERIFY2(saw_fg, "customer must keep fine_gray_formula_reference_only");
        QVERIFY2(saw_exposure, "customer must keep warranty_strata_exposure_honesty");
        QVERIFY2(saw_basis, "customer must keep warranty_strata_reliability_basis");
        QVERIFY(customer.pages[0].truncated_evidence_count > 0);

        const ReportDocument engineer = build_report_document(
            sample_table(), {page}, make_report_profile(ReportTemplateKind::engineer));
        bool saw_formula_cif = false;
        bool saw_formula_fg = false;
        for (const auto& ref : engineer.evidence.evidence) {
            if (ref.evidence_id.find(":formula:cif") != std::string::npos) {
                saw_formula_cif = true;
                QCOMPARE(ref.kind, datalab::domain::EvidenceKind::formula_reference);
            }
            if (ref.evidence_id.find(":formula:fine_gray") != std::string::npos) {
                saw_formula_fg = true;
                QCOMPARE(ref.kind, datalab::domain::EvidenceKind::formula_reference);
            }
        }
        QVERIFY(saw_formula_cif);
        QVERIFY(saw_formula_fg);

        OutputPage poisoned = page;
        poisoned.facts.reliability->cif_evidence_type = "vendor_oracle";
        poisoned.facts.reliability->fine_gray_evidence_type = "golden";
        poisoned.facts.warranty->evidence_type = "vendor_oracle";
        flood_plots(poisoned);
        const ReportDocument poisoned_customer =
            build_report_document(sample_table(), {poisoned}, customer_profile);
        for (const auto& ref : poisoned_customer.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:cif_") == std::string::npos
                && ref.evidence_id.find(":gate:fine_gray_") == std::string::npos
                && ref.evidence_id.find(":gate:warranty_strata_") == std::string::npos) {
                continue;
            }
            for (const std::string& key : ref.parameter_keys) {
                QVERIFY(key != "evidence_type=vendor_oracle");
                QVERIFY(key != "evidence_type=golden");
                if (key.rfind("evidence_type=", 0) == 0) {
                    QCOMPARE(key, std::string("evidence_type=formula_reference"));
                }
            }
        }

        QCOMPARE(
            facts_fingerprint(customer.pages[0].source_page.facts),
            facts_fingerprint(engineer.pages[0].source_page.facts));
    }

    void customer_keeps_hexbin_rectangular_bins_limiting_evidence_under_truncation()
    {
        ReportProfile customer_profile = make_report_profile(ReportTemplateKind::customer);
        QCOMPARE(customer_profile.max_evidence_rows, std::size_t{20});

        OutputPage page;
        page.id = "hexbin-1";
        page.title = "Hexbin / 二维分箱";
        page.method_name = "Hexbin Plot";
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "hexbin_rectangular_bins",
            "使用矩形二维分箱（Binned Scatter）；非正六边形镶嵌。"});
        datalab::domain::PlotSpec hex;
        hex.kind = datalab::domain::PlotKind::hexbin;
        hex.title = "Hexbin";
        hex.source_rows = {0, 1, 2};
        page.plots.push_back(hex);
        for (int i = 0; i < 30; ++i) {
            datalab::domain::PlotSpec flood;
            flood.title = "flood-" + std::to_string(i);
            flood.source_rows = {0};
            page.plots.push_back(flood);
        }

        const ReportDocument customer =
            build_report_document(sample_table(), {page}, customer_profile);
        bool saw_gate = false;
        for (const auto& ref : customer.pages[0].visible_evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::formula_reference);
            if (ref.evidence_id.find(":gate:hexbin_rectangular_bins") != std::string::npos) {
                saw_gate = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(ref.diagnostic_code, std::string("hexbin_rectangular_bins"));
                QCOMPARE(
                    ref.label_text_id, std::string("evidence.hexbin_rectangular_bins"));
                bool saw_rect = false;
                bool saw_not_hex = false;
                bool saw_formula = false;
                for (const std::string& key : ref.parameter_keys) {
                    if (key == "binning=rectangular") {
                        saw_rect = true;
                    }
                    if (key == "hexagonal_tessellation=false") {
                        saw_not_hex = true;
                    }
                    if (key == "evidence_type=formula_reference") {
                        saw_formula = true;
                    }
                    QVERIFY(key != "evidence_type=vendor_oracle");
                }
                QVERIFY(saw_rect);
                QVERIFY(saw_not_hex);
                QVERIFY(saw_formula);
            }
        }
        QVERIFY2(saw_gate, "customer must keep hexbin_rectangular_bins under plot flood");
        QVERIFY(customer.pages[0].truncated_evidence_count > 0);

        // Info diagnostic alone (no plot kind) still emits gate via diagnostic code.
        OutputPage diag_only;
        diag_only.id = "hexbin-diag";
        diag_only.title = "EDA";
        diag_only.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "hexbin_rectangular_bins",
            "使用矩形二维分箱（Binned Scatter）；非正六边形镶嵌。"});
        for (int i = 0; i < 30; ++i) {
            datalab::domain::PlotSpec flood;
            flood.title = "d-flood-" + std::to_string(i);
            diag_only.plots.push_back(flood);
        }
        const ReportDocument diag_customer =
            build_report_document(sample_table(), {diag_only}, customer_profile);
        bool saw_diag_gate = false;
        for (const auto& ref : diag_customer.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:hexbin_rectangular_bins") != std::string::npos) {
                saw_diag_gate = true;
            }
        }
        QVERIFY(saw_diag_gate);
    }

    void customer_keeps_box_cox_limiting_evidence_under_truncation()
    {
        ReportProfile customer_profile = make_report_profile(ReportTemplateKind::customer);
        QCOMPARE(customer_profile.max_evidence_rows, std::size_t{20});

        OutputPage page;
        page.id = "box-cox-gate";
        page.title = "Box-Cox 变换";
        page.method_name = "Box-Cox Transformation";
        datalab::domain::BoxCoxFacts box_cox;
        box_cox.lambda = 0.5;
        box_cox.n = 5;
        box_cox.assumption_status = "not_verified";
        page.facts.box_cox = box_cox;
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "box_cox_invalid_spec_limit",
            "规格下限无法变换（须为正有限数）。"});
        for (int i = 0; i < 30; ++i) {
            datalab::domain::PlotSpec plot;
            plot.title = "flood-" + std::to_string(i);
            plot.source_rows = {0, 1, 2};
            page.plots.push_back(plot);
        }

        const ReportDocument customer =
            build_report_document(sample_table(), {page}, customer_profile);
        bool saw_spec_gate = false;
        bool saw_not_pass_fail = false;
        for (const auto& ref : customer.pages[0].visible_evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::golden);
            if (ref.evidence_id.find(":gate:box_cox_spec_limit") != std::string::npos) {
                saw_spec_gate = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(ref.label_text_id, std::string("evidence.box_cox_spec_limit_gate"));
                QCOMPARE(ref.diagnostic_code, std::string("box_cox_invalid_spec_limit"));
            }
            if (ref.evidence_id.find(":gate:box_cox_not_pass_fail") != std::string::npos) {
                saw_not_pass_fail = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(ref.label_text_id, std::string("evidence.box_cox_not_pass_fail"));
            }
        }
        QVERIFY2(
            saw_spec_gate,
            "customer visible_evidence must retain Box-Cox spec-limit gate under plot flood");
        QVERIFY2(
            saw_not_pass_fail,
            "customer visible_evidence must retain Box-Cox not-pass/fail gate under plot flood");
        QVERIFY(customer.pages[0].truncated_evidence_count > 0);
    }

    void customer_keeps_johnson_spec_limit_evidence_under_truncation()
    {
        OutputPage page = sample_page();
        page.facts.capability->pass_fail_judgment_allowed = false;
        page.facts.capability->gate_status = "gated_research";
        page.facts.capability->research_preview = true;
        page.facts.capability->method = "johnson";
        page.facts.capability->johnson_family = "SL";
        page.facts.capability->evidence_type = "formula_reference";
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::error,
            "johnson_spec_outside_support",
            "规格限落在 Johnson 变换定义域外，无法计算 Pp/Ppk。"});
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "johnson_capability_gated",
            "Johnson 能力为研究/预览结果：可显示拟合与变换尺度指数，"
            "但不得输出过程合格判定（门禁：缺 golden/尾部/人工解释验收）。"});
        for (int i = 0; i < 30; ++i) {
            datalab::domain::PlotSpec plot;
            plot.title = "flood-" + std::to_string(i);
            plot.source_rows = {0, 1, 2};
            page.plots.push_back(plot);
        }

        ReportProfile customer_profile = make_report_profile(ReportTemplateKind::customer);
        const ReportDocument customer =
            build_report_document(sample_table(), {page}, customer_profile);
        bool saw_spec_gate = false;
        bool saw_johnson_gate = false;
        for (const auto& ref : customer.pages[0].visible_evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::golden);
            if (ref.evidence_id.find(":gate:johnson_spec_limit") != std::string::npos) {
                saw_spec_gate = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(ref.label_text_id, std::string("evidence.johnson_spec_limit_gate"));
                QCOMPARE(ref.diagnostic_code, std::string("johnson_spec_outside_support"));
            }
            if (ref.evidence_id.find(":gate:johnson") != std::string::npos
                && ref.evidence_id.find(":gate:johnson_spec_limit") == std::string::npos) {
                saw_johnson_gate = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(ref.label_text_id, std::string("evidence.johnson_capability_gated"));
            }
        }
        QVERIFY2(
            saw_spec_gate,
            "customer visible_evidence must retain Johnson spec-limit gate under plot flood");
        QVERIFY2(
            saw_johnson_gate,
            "customer visible_evidence must retain Johnson research gate under plot flood");
        QVERIFY(customer.pages[0].truncated_evidence_count > 0);
    }

    void customer_keeps_warranty_exposure_gate_evidence_under_truncation()
    {
        OutputPage page;
        page.id = "warranty-exposure-gate";
        page.title = "保修摘要";
        page.method_name = "Warranty Summary";
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::error,
            "invalid_exposure_value",
            "暴露量列必须为有限非负数；缺失或非法值不会被静默补齐。"});
        for (int i = 0; i < 30; ++i) {
            datalab::domain::PlotSpec plot;
            plot.title = "warranty-flood-" + std::to_string(i);
            plot.source_rows = {0};
            page.plots.push_back(plot);
        }

        ReportProfile customer_profile = make_report_profile(ReportTemplateKind::customer);
        const ReportDocument customer =
            build_report_document(sample_table(), {page}, customer_profile);
        bool saw_exposure_gate = false;
        for (const auto& ref : customer.pages[0].visible_evidence) {
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::vendor_oracle);
            QVERIFY(ref.kind != datalab::domain::EvidenceKind::golden);
            if (ref.evidence_id.find(":gate:warranty_exposure") != std::string::npos) {
                saw_exposure_gate = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(ref.label_text_id, std::string("evidence.warranty_exposure_gate"));
                QCOMPARE(ref.diagnostic_code, std::string("invalid_exposure_value"));
            }
        }
        QVERIFY2(
            saw_exposure_gate,
            "customer visible_evidence must retain warranty exposure gate under plot flood");
        QVERIFY(customer.pages[0].truncated_evidence_count > 0);
    }

    void customer_keeps_density_curve_not_discrete_marks_limiting_evidence_under_truncation()
    {
        OutputPage page;
        page.id = "density-1";
        page.title = "密度图（分面）";
        page.method_name = "Faceted Density Plot";
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "density_curve_not_discrete_marks",
            "密度曲线是连续估计网格，不是离散观测标记；"
            "点选曲线不会映射到单条工作表行（hidden/excluded 仍分别计入分析/显示 N）。"});
        datalab::domain::PlotSpec density;
        density.kind = datalab::domain::PlotKind::density;
        density.title = "Density";
        page.plots.push_back(density);
        for (int i = 0; i < 30; ++i) {
            datalab::domain::PlotSpec flood;
            flood.title = "flood-" + std::to_string(i);
            flood.source_rows = {0};
            page.plots.push_back(flood);
        }

        ReportProfile customer_profile = make_report_profile(ReportTemplateKind::customer);
        const ReportDocument customer =
            build_report_document(sample_table(), {page}, customer_profile);
        bool saw_gate = false;
        for (const auto& ref : customer.pages[0].visible_evidence) {
            if (ref.evidence_id.find(":gate:density_curve_not_discrete_marks")
                    != std::string::npos) {
                saw_gate = true;
                QCOMPARE(ref.role, datalab::domain::EvidenceRole::limiting);
                QCOMPARE(
                    ref.label_text_id,
                    std::string("evidence.density_curve_not_discrete_marks"));
                QCOMPARE(ref.diagnostic_code, std::string("density_curve_not_discrete_marks"));
            }
        }
        QVERIFY2(
            saw_gate,
            "customer visible_evidence must retain density curve honesty gate under plot flood");
        QVERIFY(customer.pages[0].truncated_evidence_count > 0);
    }
};

QTEST_MAIN(ReportProfilePhase1Test)
#include "report_profile_phase1_test.moc"
