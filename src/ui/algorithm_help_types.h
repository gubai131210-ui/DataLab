#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct FormulaNode {
    QString type;
    QString value;
    QVector<FormulaNode> content;
    QVector<FormulaNode> numerator;
    QVector<FormulaNode> denominator;
    QVector<FormulaNode> when;
    QVector<QVector<FormulaNode>> rows;
};

struct FormulaBlock {
    QString title;
    QString note;
    QString explanation;
    QString conditions;
    QVector<FormulaNode> nodes;
    QString plain_text;
};

struct HelpReferenceLink {
    QString label;
    QString url;
    QString accessed;
    QString kind = QStringLiteral("minitab");
};

struct HelpSourceDocument {
    QString label;
    QString path;
    QString section;
};

struct HelpWiring {
    QString command_id;
    QString service_method;
    QString facts_type;
    QString primary_test;
};

struct HelpSymbol {
    QString symbol;
    QString meaning;
};

struct AlgorithmHelpEntry {
    QString id;
    QString title;
    QString category;
    QString menu_path;
    QStringList aliases;
    QString implemented_status;
    QString purpose;
    QString method_overview;
    QString input_description;
    QString missing_value_policy;
    QStringList calculation_steps;
    QVector<HelpSymbol> symbol_definitions;
    QStringList decision_rules;
    QString invalid_input_conditions;
    QString output_description;
    QString output_interpretation;
    QString assumptions_and_boundaries;
    QString interpretation_limits;
    QVector<FormulaBlock> formula_blocks;
    QVector<HelpReferenceLink> reference_links;
    QVector<HelpSourceDocument> source_documents;
    HelpWiring wiring;
};

struct AlgorithmHelpCatalog {
    QString catalog_version;
    QString last_reviewed;
    QVector<AlgorithmHelpEntry> entries;
};
