#include "ui/algorithm_help_catalog.h"
#include "ui/analysis_commands.h"
#include "ui/formula_renderer.h"

#include <QSet>
#include <QtTest/QtTest>

class AlgorithmHelpCatalogTest final : public QObject {
    Q_OBJECT

private slots:
    void resourceLoadsAndValidates();
    void entriesHaveUserManualFields();
    void wiringIndexCommandsAreCovered();
    void userTextDoesNotDependOnMarkdown();
    void formulaNodesRenderWithoutCrash();
    void piecewiseAndFractionNodesWork();
    void referenceLinksUseHttps();
    void keyFamiliesHaveFormulasAndReferences();
};

void AlgorithmHelpCatalogTest::resourceLoadsAndValidates()
{
    const AlgorithmHelpCatalog catalog =
        AlgorithmHelpCatalogLoader::load_from_resource(QStringLiteral(":/help/algorithm_help.json"));
    QVERIFY(!catalog.catalog_version.isEmpty());
    QVERIFY(!catalog.last_reviewed.isEmpty());
    QVERIFY(catalog.entries.size() >= 70);

    QString error;
    QVERIFY2(AlgorithmHelpCatalogLoader::validate(catalog, &error), qPrintable(error));
}

void AlgorithmHelpCatalogTest::entriesHaveUserManualFields()
{
    const AlgorithmHelpCatalog catalog =
        AlgorithmHelpCatalogLoader::load_from_resource(QStringLiteral(":/help/algorithm_help.json"));
    QSet<QString> ids;
    for (const AlgorithmHelpEntry& entry : catalog.entries) {
        QVERIFY2(!entry.id.isEmpty(), "entry id");
        QVERIFY2(!entry.purpose.isEmpty(), qPrintable(entry.id));
        QVERIFY2(!entry.method_overview.isEmpty(), qPrintable(entry.id));
        QVERIFY2(!entry.calculation_steps.isEmpty(), qPrintable(entry.id));
        QVERIFY2(!entry.symbol_definitions.isEmpty(), qPrintable(entry.id));
        QVERIFY2(!entry.decision_rules.isEmpty(), qPrintable(entry.id));
        QVERIFY2(!entry.invalid_input_conditions.isEmpty(), qPrintable(entry.id));
        QVERIFY2(!entry.missing_value_policy.isEmpty(), qPrintable(entry.id));
        QVERIFY2(!entry.output_interpretation.isEmpty(), qPrintable(entry.id));
        QVERIFY2(!entry.formula_blocks.isEmpty(), qPrintable(entry.id));
        QVERIFY2(!ids.contains(entry.id), qPrintable(entry.id));
        ids.insert(entry.id);
        for (const FormulaBlock& block : entry.formula_blocks) {
            QVERIFY2(!block.title.isEmpty(), qPrintable(entry.id));
            QVERIFY2(!block.plain_text.isEmpty() || !block.nodes.isEmpty(), qPrintable(entry.id));
        }
    }
}

void AlgorithmHelpCatalogTest::wiringIndexCommandsAreCovered()
{
    const AlgorithmHelpCatalog catalog =
        AlgorithmHelpCatalogLoader::load_from_resource(QStringLiteral(":/help/algorithm_help.json"));
    QSet<QString> catalog_ids;
    for (const AlgorithmHelpEntry& entry : catalog.entries) {
        catalog_ids.insert(entry.id);
    }
    for (const analysis_commands::AnalysisCommand& command : analysis_commands::all()) {
        QVERIFY2(catalog_ids.contains(command.id),
                 qPrintable(QStringLiteral("missing help entry for command %1").arg(command.id)));
    }
}

void AlgorithmHelpCatalogTest::userTextDoesNotDependOnMarkdown()
{
    const AlgorithmHelpCatalog catalog =
        AlgorithmHelpCatalogLoader::load_from_resource(QStringLiteral(":/help/algorithm_help.json"));
    for (const AlgorithmHelpEntry& entry : catalog.entries) {
        const QString blob = entry.purpose + entry.method_overview + entry.output_interpretation;
        QVERIFY2(!blob.contains(QStringLiteral(".md")), qPrintable(entry.id));
        QVERIFY2(!blob.contains(QStringLiteral("docs/")), qPrintable(entry.id));
        QVERIFY2(!blob.contains(QStringLiteral("见仓库")), qPrintable(entry.id));
        for (const FormulaBlock& block : entry.formula_blocks) {
            QVERIFY2(!block.plain_text.contains(QStringLiteral(".md")), qPrintable(entry.id));
            QVERIFY2(!block.plain_text.contains(QStringLiteral("见仓库")), qPrintable(entry.id));
        }
    }
}

void AlgorithmHelpCatalogTest::formulaNodesRenderWithoutCrash()
{
    const AlgorithmHelpCatalog catalog =
        AlgorithmHelpCatalogLoader::load_from_resource(QStringLiteral(":/help/algorithm_help.json"));
    const auto capability = AlgorithmHelpCatalogLoader::find_by_id(catalog, QStringLiteral("capability"));
    QVERIFY(capability.has_value());
    QVERIFY(!capability->purpose.isEmpty());
    for (const FormulaBlock& block : capability->formula_blocks) {
        if (!block.nodes.isEmpty()) {
            const QString html = FormulaRenderer::to_html(block.nodes);
            const QString plain = FormulaRenderer::to_plain_text(block.nodes);
            QVERIFY(!html.isEmpty());
            QVERIFY(!plain.isEmpty());
            QVERIFY(!html.contains(QStringLiteral("<script")));
        }
    }
}

void AlgorithmHelpCatalogTest::piecewiseAndFractionNodesWork()
{
    const AlgorithmHelpCatalog catalog =
        AlgorithmHelpCatalogLoader::load_from_resource(QStringLiteral(":/help/algorithm_help.json"));
    const auto box_cox = AlgorithmHelpCatalogLoader::find_by_id(catalog, QStringLiteral("box_cox"));
    QVERIFY(box_cox.has_value());
    bool has_piecewise = false;
    for (const FormulaBlock& block : box_cox->formula_blocks) {
        for (const FormulaNode& node : block.nodes) {
            if (node.type == QStringLiteral("piecewise")) {
                has_piecewise = true;
                const QString html = FormulaRenderer::to_html(block.nodes);
                const QString plain = FormulaRenderer::to_plain_text(block.nodes);
                QVERIFY(html.contains(QStringLiteral("ln")));
                QVERIFY(plain.contains(QStringLiteral("λ")));
            }
        }
    }
    QVERIFY(has_piecewise);

    const auto xbar_s = AlgorithmHelpCatalogLoader::find_by_id(catalog, QStringLiteral("xbar_s"));
    QVERIFY(xbar_s.has_value());
    bool has_cases = false;
    bool has_bar = false;
    for (const FormulaBlock& block : xbar_s->formula_blocks) {
        QVERIFY(!block.nodes.isEmpty());
        const QString html = FormulaRenderer::to_html(block.nodes);
        QVERIFY(html.contains(QStringLiteral("UCL")) || html.contains(QStringLiteral("c"))
                || html.contains(QStringLiteral("σ")));
        QVERIFY(!html.contains(QStringLiteral("<pre")));
        for (const FormulaNode& node : block.nodes) {
            if (node.type == QStringLiteral("stack") || node.type == QStringLiteral("piecewise")
                || node.type == QStringLiteral("line")) {
                has_cases = true;
            }
        }
        const QString plain = FormulaRenderer::to_plain_text(block.nodes);
        if (plain.contains(QStringLiteral("bar(")) || html.contains(QStringLiteral("border-top"))) {
            has_bar = true;
        }
    }
    QVERIFY(has_cases);
    QVERIFY(has_bar);
}

void AlgorithmHelpCatalogTest::referenceLinksUseHttps()
{
    const AlgorithmHelpCatalog catalog =
        AlgorithmHelpCatalogLoader::load_from_resource(QStringLiteral(":/help/algorithm_help.json"));
    for (const AlgorithmHelpEntry& entry : catalog.entries) {
        QVERIFY2(!entry.reference_links.isEmpty(), qPrintable(entry.id));
        for (const HelpReferenceLink& link : entry.reference_links) {
            QVERIFY2(link.url.startsWith(QStringLiteral("https://")),
                     qPrintable(entry.id + QStringLiteral(": ") + link.url));
            QVERIFY2(!link.accessed.isEmpty(), qPrintable(entry.id));
            QVERIFY2(!link.label.isEmpty(), qPrintable(entry.id));
        }
    }
}

void AlgorithmHelpCatalogTest::keyFamiliesHaveFormulasAndReferences()
{
    const AlgorithmHelpCatalog catalog =
        AlgorithmHelpCatalogLoader::load_from_resource(QStringLiteral(":/help/algorithm_help.json"));
    const QStringList required = {
        QStringLiteral("imr"),
        QStringLiteral("capability"),
        QStringLiteral("regression"),
        QStringLiteral("doe_factorial"),
        QStringLiteral("msa_type1"),
        QStringLiteral("gage_rr"),
        QStringLiteral("arima"),
        QStringLiteral("reliability"),
    };
    for (const QString& id : required) {
        const auto entry = AlgorithmHelpCatalogLoader::find_by_id(catalog, id);
        QVERIFY2(entry.has_value(), qPrintable(id));
        QVERIFY2(!entry->formula_blocks.isEmpty(), qPrintable(id));
        QVERIFY2(!entry->reference_links.isEmpty(), qPrintable(id));
        QVERIFY2(!entry->decision_rules.isEmpty(), qPrintable(id));
        QVERIFY2(entry->method_overview.size() > 40, qPrintable(id));
    }
}

QTEST_APPLESS_MAIN(AlgorithmHelpCatalogTest)
#include "algorithm_help_catalog_test.moc"
