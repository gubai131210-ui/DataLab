#include "application/computation_trace_attach.h"
#include "application/computation_trace_attach_deep.h"
#include "infrastructure/output_serialization.h"

#include "domain/quality_types.h"

#include <QtTest/QtTest>

using datalab::application::attach_computation_traces;
using datalab::domain::CapabilityFacts;
using datalab::domain::MsaFacts;
using datalab::domain::OutputPage;
using datalab::domain::RegressionFacts;
using datalab::domain::SpcFacts;
using datalab::domain::TTestFacts;
using datalab::infrastructure::output_page_from_json;
using datalab::infrastructure::output_page_to_json;

class G9ShowYourWorkDeepenTrackTest : public QObject {
    Q_OBJECT

private slots:
    void computationStepSerializeRoundTrip();
    void capabilityPilotL3Steps();
    void oneSampleTPilotL3Steps();
    void imrControlChartL3Steps();
    void regressionFamilyL3Steps();
    void gageRrMsaL3Steps();
    void deepTraceNoStubFormulaLiteral();
};

static bool has_step_field(const datalab::domain::ComputationTrace& tr, const char* field)
{
    for (const auto& step : tr.steps) {
        if (field == std::string("expression_before") && !step.expression_before.empty()) {
            return true;
        }
        if (field == std::string("expression_after") && !step.expression_after.empty()) {
            return true;
        }
        if (field == std::string("value") && !step.value.empty()) {
            return true;
        }
    }
    return false;
}

void G9ShowYourWorkDeepenTrackTest::computationStepSerializeRoundTrip()
{
    OutputPage page;
    page.analysis_command_id = "one_sample_t";
    page.configuration.inference.hypothesis_mean = 0.0;
    TTestFacts facts;
    facts.n = 10;
    facts.mean = 1.0;
    facts.sample_standard_deviation = 2.0;
    page.facts.t_test = facts;
    attach_computation_traces(page, "one_sample_t");
    QVERIFY(page.computation_traces.size() >= 1);
    const auto json = output_page_to_json(page);
    const OutputPage restored = output_page_from_json(json);
    QCOMPARE(restored.computation_traces.size(), page.computation_traces.size());
    const auto& step = restored.computation_traces.front().steps.front();
    QVERIFY(step.order >= 1);
    QVERIFY(!step.description.empty() || !step.expression_before.empty());
}

void G9ShowYourWorkDeepenTrackTest::capabilityPilotL3Steps()
{
    OutputPage page;
    page.analysis_command_id = "capability";
    page.configuration.specifications.lower = 10.0;
    page.configuration.specifications.upper = 20.0;
    CapabilityFacts facts;
    facts.cpk = 1.33;
    page.facts.capability = facts;
    datalab::domain::StatisticTable table;
    table.headers = {"Item", "Value"};
    table.rows.push_back({"Mean", "15"});
    table.rows.push_back({"StDev", "1.25"});
    page.tables.push_back(table);
    attach_computation_traces(page, "capability");
    QVERIFY(!page.computation_traces.empty());
    const auto& tr = page.computation_traces.front();
    QVERIFY(tr.steps.size() >= 2);
    QVERIFY(has_step_field(tr, "expression_before"));
    QVERIFY(has_step_field(tr, "value"));
    QVERIFY(!tr.plain_formula.contains(QStringLiteral("主公式")));
}

void G9ShowYourWorkDeepenTrackTest::oneSampleTPilotL3Steps()
{
    OutputPage page;
    page.analysis_command_id = "one_sample_t";
    page.configuration.inference.hypothesis_mean = 10.0;
    TTestFacts facts;
    facts.n = 20;
    facts.mean = 10.24;
    facts.sample_standard_deviation = 1.12;
    page.facts.t_test = facts;
    attach_computation_traces(page, "one_sample_t");
    const auto& tr = page.computation_traces.front();
    QVERIFY(tr.steps.size() >= 2);
    bool has_se = false;
    for (const auto& step : tr.steps) {
        if (step.description.find("标准误") != std::string::npos
            || step.expression_before.find("√") != std::string::npos) {
            has_se = true;
        }
    }
    QVERIFY(has_se);
}

void G9ShowYourWorkDeepenTrackTest::imrControlChartL3Steps()
{
    OutputPage page;
    page.analysis_command_id = "imr";
    SpcFacts spc;
    spc.sigma_within = 0.5;
    spc.estimated_sigma = 0.5;
    page.facts.spc = spc;
    datalab::domain::StatisticTable table;
    table.headers = {"Item", "Value"};
    table.rows.push_back({"CL", "10.0"});
    table.rows.push_back({"UCL", "11.5"});
    table.rows.push_back({"LCL", "8.5"});
    table.rows.push_back({"MR-bar", "0.56"});
    page.tables.push_back(table);
    attach_computation_traces(page, "imr");
    QVERIFY(!page.computation_traces.empty());
    const auto& tr = page.computation_traces.front();
    QVERIFY(tr.steps.size() >= 2);
    QVERIFY(!tr.plain_formula.contains(QStringLiteral("主公式")));
    QStringList syms;
    for (const auto& b : tr.bindings) {
        syms << QString::fromStdString(b.symbol);
    }
    QVERIFY(syms.contains(QStringLiteral("UCL")) || syms.contains(QStringLiteral("CL")));
}

void G9ShowYourWorkDeepenTrackTest::regressionFamilyL3Steps()
{
    OutputPage page;
    page.analysis_command_id = "regression";
    RegressionFacts facts;
    facts.r_squared = 0.87;
    page.facts.regression = facts;
    datalab::domain::StatisticTable table;
    table.headers = {"Item", "Value"};
    table.rows.push_back({"R-sq", "0.87"});
    table.rows.push_back({"N", "50"});
    page.tables.push_back(table);
    attach_computation_traces(page, "regression");
    QVERIFY(!page.computation_traces.empty());
    const auto& tr = page.computation_traces.front();
    QVERIFY(tr.steps.size() >= 2);
    QVERIFY(!tr.plain_formula.contains(QStringLiteral("主公式")));
}

void G9ShowYourWorkDeepenTrackTest::gageRrMsaL3Steps()
{
    OutputPage page;
    page.analysis_command_id = "gage_rr";
    MsaFacts facts;
    facts.gage_percent_study_variation = 15.2;
    facts.ndc = 5.0;
    facts.ndc_available = true;
    page.facts.msa = facts;
    datalab::domain::StatisticTable table;
    table.headers = {"Item", "Value"};
    table.rows.push_back({"%Study Var", "15.2"});
    table.rows.push_back({"ndc", "5"});
    page.tables.push_back(table);
    attach_computation_traces(page, "gage_rr");
    QVERIFY(!page.computation_traces.empty());
    const auto& tr = page.computation_traces.front();
    QVERIFY(tr.steps.size() >= 2);
    QVERIFY(!tr.plain_formula.contains(QStringLiteral("主公式")));
}

void G9ShowYourWorkDeepenTrackTest::deepTraceNoStubFormulaLiteral()
{
    const char* probes[] = {
        "normality_test", "two_sample_t", "imr", "gage_rr", "reliability",
    };
    for (const char* cid : probes) {
        OutputPage page;
        page.analysis_command_id = cid;
        attach_computation_traces(page, cid);
        if (page.computation_traces.empty()) {
            QFAIL(qPrintable(QStringLiteral("no trace for %1").arg(cid)));
        }
        for (const auto& tr : page.computation_traces) {
            QVERIFY(!QString::fromStdString(tr.plain_formula).contains(
                QStringLiteral("主公式")));
            if (tr.evidence_type == "formula_reference") {
                QVERIFY(tr.steps.size() >= 2 || tr.bindings.size() >= 3);
            }
        }
    }
}

QTEST_APPLESS_MAIN(G9ShowYourWorkDeepenTrackTest)
#include "g9_show_your_work_deepen_track_test.moc"
