#include "application/computation_trace_attach.h"
#include "infrastructure/output_serialization.h"

#include "domain/quality_types.h"

#include <QtTest/QtTest>

#include <string>

using datalab::application::attach_computation_traces;
using datalab::domain::CapabilityFacts;
using datalab::domain::OutputPage;
using datalab::domain::TTestFacts;
using datalab::domain::WeibayesFacts;
using datalab::infrastructure::output_page_from_json;
using datalab::infrastructure::output_page_to_json;

class G9FormulaSubstitutionTrackTest : public QObject {
    Q_OBJECT

private slots:
    void capabilityPilotHasRichBindings();
    void oneSampleTPilotHasKeySymbols();
    void weibayesPilotHasBetaEta();
    void serializeRoundTripComputationTraces();
    void histogramDisplaySummary();
    void evidenceTypeFormulaReferenceMarker();
};

void G9FormulaSubstitutionTrackTest::capabilityPilotHasRichBindings()
{
    OutputPage page;
    page.analysis_command_id = "capability";
    page.configuration.specifications.lower = 10.0;
    page.configuration.specifications.upper = 20.0;
    CapabilityFacts facts;
    facts.cpk = 1.33;
    page.facts.capability = facts;
    datalab::domain::StatisticTable table;
    table.title = "Capability";
    table.headers = {"Item", "Value"};
    table.rows.push_back({"Mean", "15"});
    table.rows.push_back({"StDev", "1.25"});
    table.rows.push_back({"Cpk", "1.33"});
    page.tables.push_back(table);

    attach_computation_traces(page, "capability");
    QVERIFY(!page.computation_traces.empty());
    const auto& tr = page.computation_traces.front();
    QCOMPARE(QString::fromStdString(tr.command_id), QStringLiteral("capability"));
    QVERIFY(tr.bindings.size() >= 4);
    bool has_usl = false;
    bool has_cpk = false;
    for (const auto& b : tr.bindings) {
        if (b.symbol == "USL") {
            has_usl = true;
        }
        if (b.symbol == "Cpk") {
            has_cpk = true;
            QCOMPARE(QString::fromStdString(b.value), QStringLiteral("1.33"));
        }
    }
    QVERIFY(has_usl);
    QVERIFY(has_cpk);
}

void G9FormulaSubstitutionTrackTest::oneSampleTPilotHasKeySymbols()
{
    OutputPage page;
    page.analysis_command_id = "one_sample_t";
    page.configuration.inference.hypothesis_mean = 0.0;
    TTestFacts facts;
    facts.kind = "one_sample";
    facts.n = 20;
    facts.mean = 1.5;
    facts.sample_standard_deviation = 2.0;
    page.facts.t_test = facts;
    datalab::domain::StatisticTable table;
    table.headers = {"Item", "Value"};
    table.rows.push_back({"T", "3.354"});
    page.tables.push_back(table);

    attach_computation_traces(page, "one_sample_t");
    QVERIFY(!page.computation_traces.empty());
    const auto& tr = page.computation_traces.front();
    QStringList symbols;
    for (const auto& b : tr.bindings) {
        symbols << QString::fromStdString(b.symbol);
    }
    QVERIFY(symbols.contains(QStringLiteral("n")));
    QVERIFY(symbols.contains(QStringLiteral("x̄")) || symbols.contains(QString::fromUtf8("x̄")));
    QVERIFY(symbols.contains(QStringLiteral("μ₀")) || symbols.contains(QString::fromUtf8("μ₀")));
    QVERIFY(symbols.contains(QStringLiteral("t")));
}

void G9FormulaSubstitutionTrackTest::weibayesPilotHasBetaEta()
{
    OutputPage page;
    page.analysis_command_id = "weibayes";
    page.configuration.weibayes.shape_prior = 2.5;
    WeibayesFacts facts;
    facts.shape_prior = 2.5;
    facts.failure_count = 3;
    facts.scale = 100.0;
    page.facts.weibayes = facts;

    attach_computation_traces(page, "weibayes");
    QVERIFY(!page.computation_traces.empty());
    const auto& tr = page.computation_traces.front();
    QStringList symbols;
    for (const auto& b : tr.bindings) {
        symbols << QString::fromStdString(b.symbol);
    }
    QVERIFY(symbols.contains(QString::fromUtf8("β")));
    QVERIFY(symbols.contains(QStringLiteral("r")));
    QVERIFY(symbols.contains(QString::fromUtf8("η")));
}

void G9FormulaSubstitutionTrackTest::serializeRoundTripComputationTraces()
{
    OutputPage page;
    page.id = "capability_1";
    page.analysis_command_id = "capability";
    attach_computation_traces(page, "capability");
    QVERIFY(!page.computation_traces.empty());

    const auto json = output_page_to_json(page);
    QVERIFY(json.contains(QStringLiteral("computation_traces")));
    const OutputPage restored = output_page_from_json(json);
    QCOMPARE(restored.computation_traces.size(), page.computation_traces.size());
    QCOMPARE(QString::fromStdString(restored.analysis_command_id),
             QStringLiteral("capability"));
    QCOMPARE(QString::fromStdString(restored.computation_traces.front().formula_id),
             QString::fromStdString(page.computation_traces.front().formula_id));
    QCOMPARE(restored.computation_traces.front().bindings.size(),
             page.computation_traces.front().bindings.size());
}

void G9FormulaSubstitutionTrackTest::histogramDisplaySummary()
{
    OutputPage page;
    page.analysis_command_id = "histogram";
    datalab::domain::EdaPlotFacts eda;
    eda.n = 42;
    page.facts.eda = eda;
    attach_computation_traces(page, "histogram");
    QVERIFY(!page.computation_traces.empty());
    QCOMPARE(QString::fromStdString(page.computation_traces.front().evidence_type),
             QStringLiteral("display_summary"));
}

void G9FormulaSubstitutionTrackTest::evidenceTypeFormulaReferenceMarker()
{
    // # source: formula_reference
    OutputPage page;
    attach_computation_traces(page, "descriptive");
    QVERIFY(!page.computation_traces.empty());
    QCOMPARE(QString::fromStdString(page.computation_traces.front().evidence_type),
             QStringLiteral("formula_reference"));
}

QTEST_MAIN(G9FormulaSubstitutionTrackTest)
#include "g9_formula_substitution_track_test.moc"
