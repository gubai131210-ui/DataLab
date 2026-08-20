#include "ui/algorithm_help_catalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace {

FormulaNode parse_formula_node(const QJsonObject& object);

QVector<FormulaNode> parse_node_array(const QJsonArray& array)
{
    QVector<FormulaNode> nodes;
    for (const QJsonValue& value : array) {
        nodes.push_back(parse_formula_node(value.toObject()));
    }
    return nodes;
}

FormulaNode parse_formula_node(const QJsonObject& object)
{
    FormulaNode node;
    node.type = object.value(QStringLiteral("type")).toString();
    node.value = object.value(QStringLiteral("value")).toString();
    if (object.contains(QStringLiteral("content"))) {
        node.content = parse_node_array(object.value(QStringLiteral("content")).toArray());
    }
    if (object.contains(QStringLiteral("num"))) {
        node.numerator = parse_node_array(object.value(QStringLiteral("num")).toArray());
    }
    if (object.contains(QStringLiteral("den"))) {
        node.denominator = parse_node_array(object.value(QStringLiteral("den")).toArray());
    }
    if (object.contains(QStringLiteral("when"))) {
        node.when = parse_node_array(object.value(QStringLiteral("when")).toArray());
    }
    if (object.contains(QStringLiteral("rows"))) {
        const QJsonArray rows = object.value(QStringLiteral("rows")).toArray();
        for (const QJsonValue& row_value : rows) {
            node.rows.push_back(parse_node_array(row_value.toArray()));
        }
    }
    return node;
}

FormulaBlock parse_formula_block(const QJsonObject& object)
{
    FormulaBlock block;
    block.title = object.value(QStringLiteral("title")).toString();
    block.note = object.value(QStringLiteral("note")).toString();
    block.explanation = object.value(QStringLiteral("explanation")).toString();
    block.conditions = object.value(QStringLiteral("conditions")).toString();
    block.plain_text = object.value(QStringLiteral("plain_text")).toString();
    const QJsonArray nodes = object.value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue& value : nodes) {
        block.nodes.push_back(parse_formula_node(value.toObject()));
    }
    return block;
}

HelpReferenceLink parse_reference(const QJsonObject& object)
{
    HelpReferenceLink link;
    link.label = object.value(QStringLiteral("label")).toString();
    link.url = object.value(QStringLiteral("url")).toString();
    link.accessed = object.value(QStringLiteral("accessed")).toString();
    link.kind = object.value(QStringLiteral("kind")).toString(QStringLiteral("minitab"));
    return link;
}

HelpSourceDocument parse_source(const QJsonObject& object)
{
    HelpSourceDocument source;
    source.label = object.value(QStringLiteral("label")).toString();
    source.path = object.value(QStringLiteral("path")).toString();
    source.section = object.value(QStringLiteral("section")).toString();
    return source;
}

HelpWiring parse_wiring(const QJsonObject& object)
{
    HelpWiring wiring;
    wiring.command_id = object.value(QStringLiteral("command_id")).toString();
    wiring.service_method = object.value(QStringLiteral("service_method")).toString();
    wiring.facts_type = object.value(QStringLiteral("facts_type")).toString();
    wiring.primary_test = object.value(QStringLiteral("primary_test")).toString();
    return wiring;
}

QStringList parse_string_list(const QJsonArray& array)
{
    QStringList values;
    for (const QJsonValue& value : array) {
        values.push_back(value.toString());
    }
    return values;
}

AlgorithmHelpEntry parse_entry(const QJsonObject& object)
{
    AlgorithmHelpEntry entry;
    entry.id = object.value(QStringLiteral("id")).toString();
    entry.title = object.value(QStringLiteral("title")).toString();
    entry.category = object.value(QStringLiteral("category")).toString();
    entry.menu_path = object.value(QStringLiteral("menu_path")).toString();
    entry.implemented_status = object.value(QStringLiteral("implemented_status")).toString();
    entry.purpose = object.value(QStringLiteral("purpose")).toString();
    entry.method_overview = object.value(QStringLiteral("method_overview")).toString();
    entry.input_description = object.value(QStringLiteral("input_description")).toString();
    entry.missing_value_policy = object.value(QStringLiteral("missing_value_policy")).toString();
    entry.invalid_input_conditions =
        object.value(QStringLiteral("invalid_input_conditions")).toString();
    entry.output_description = object.value(QStringLiteral("output_description")).toString();
    entry.output_interpretation = object.value(QStringLiteral("output_interpretation")).toString();
    entry.assumptions_and_boundaries =
        object.value(QStringLiteral("assumptions_and_boundaries")).toString();
    entry.interpretation_limits =
        object.value(QStringLiteral("interpretation_limits")).toString();
    entry.aliases = parse_string_list(object.value(QStringLiteral("aliases")).toArray());
    entry.calculation_steps =
        parse_string_list(object.value(QStringLiteral("calculation_steps")).toArray());
    entry.decision_rules =
        parse_string_list(object.value(QStringLiteral("decision_rules")).toArray());
    const QJsonArray symbols = object.value(QStringLiteral("symbol_definitions")).toArray();
    for (const QJsonValue& value : symbols) {
        const QJsonObject symbol_object = value.toObject();
        HelpSymbol symbol;
        symbol.symbol = symbol_object.value(QStringLiteral("symbol")).toString();
        symbol.meaning = symbol_object.value(QStringLiteral("meaning")).toString();
        entry.symbol_definitions.push_back(symbol);
    }
    const QJsonArray formulas = object.value(QStringLiteral("formula_blocks")).toArray();
    for (const QJsonValue& value : formulas) {
        entry.formula_blocks.push_back(parse_formula_block(value.toObject()));
    }
    const QJsonArray references = object.value(QStringLiteral("reference_links")).toArray();
    for (const QJsonValue& value : references) {
        entry.reference_links.push_back(parse_reference(value.toObject()));
    }
    const QJsonArray sources = object.value(QStringLiteral("source_documents")).toArray();
    for (const QJsonValue& value : sources) {
        entry.source_documents.push_back(parse_source(value.toObject()));
    }
    entry.wiring = parse_wiring(object.value(QStringLiteral("wiring")).toObject());
    return entry;
}

bool looks_like_markdown_only(const QString& text)
{
    return text.contains(QStringLiteral("docs/"))
        || text.contains(QStringLiteral(".md"))
        || text.contains(QStringLiteral("见仓库"));
}

bool validate_formula_nodes(const QVector<FormulaNode>& nodes, QString* error_message)
{
    for (const FormulaNode& node : nodes) {
        if (node.type.isEmpty()) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral("公式节点缺少 type。");
            }
            return false;
        }
        if (node.type == QStringLiteral("frac")) {
            if (node.numerator.isEmpty() || node.denominator.isEmpty()) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral("分式节点分子/分母不能为空。");
                }
                return false;
            }
            if (!validate_formula_nodes(node.numerator, error_message)
                || !validate_formula_nodes(node.denominator, error_message)) {
                return false;
            }
        } else if (node.type == QStringLiteral("sqrt") || node.type == QStringLiteral("abs")
                   || node.type == QStringLiteral("group") || node.type == QStringLiteral("sup")
                   || node.type == QStringLiteral("sub") || node.type == QStringLiteral("bar")
                   || node.type == QStringLiteral("piecewise") || node.type == QStringLiteral("stack")
                   || node.type == QStringLiteral("line")) {
            if (node.content.isEmpty() && node.type != QStringLiteral("heading")) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral("公式节点 content 不能为空。");
                }
                return false;
            }
            if (!validate_formula_nodes(node.content, error_message)) {
                return false;
            }
        } else if (node.type == QStringLiteral("heading")) {
            if (node.value.isEmpty()) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral("heading 节点缺少标题。");
                }
                return false;
            }
        } else if (node.type == QStringLiteral("case")) {
            if (node.content.isEmpty()) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral("分段 case 必须有公式内容。");
                }
                return false;
            }
            if (!validate_formula_nodes(node.content, error_message)
                || !validate_formula_nodes(node.when, error_message)) {
                return false;
            }
        } else if (node.type == QStringLiteral("matrix")) {
            if (node.rows.isEmpty()) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral("矩阵节点 rows 不能为空。");
                }
                return false;
            }
            for (const auto& row : node.rows) {
                if (!validate_formula_nodes(row, error_message)) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool require_text(const QString& value, const QString& entry_id, const char* field,
                  QString* error_message)
{
    if (!value.trimmed().isEmpty()) {
        return true;
    }
    if (error_message != nullptr) {
        *error_message = QStringLiteral("条目 %1 缺少 %2。")
                             .arg(entry_id, QString::fromLatin1(field));
    }
    return false;
}

}  // namespace

AlgorithmHelpCatalog AlgorithmHelpCatalogLoader::load_from_resource(const QString& resource_path)
{
    AlgorithmHelpCatalog catalog;
    QFile file(resource_path);
    if (!file.open(QIODevice::ReadOnly)) {
        return catalog;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return catalog;
    }
    const QJsonObject root = document.object();
    catalog.catalog_version = root.value(QStringLiteral("catalog_version")).toString();
    catalog.last_reviewed = root.value(QStringLiteral("last_reviewed")).toString();
    const QJsonArray entries = root.value(QStringLiteral("entries")).toArray();
    for (const QJsonValue& value : entries) {
        catalog.entries.push_back(parse_entry(value.toObject()));
    }
    return catalog;
}

bool AlgorithmHelpCatalogLoader::validate(
    const AlgorithmHelpCatalog& catalog, QString* error_message)
{
    if (catalog.entries.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("帮助目录为空。");
        }
        return false;
    }
    QSet<QString> ids;
    for (const AlgorithmHelpEntry& entry : catalog.entries) {
        if (entry.id.isEmpty() || entry.title.isEmpty() || entry.category.isEmpty()) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral("条目缺少 id/title/category。");
            }
            return false;
        }
        if (ids.contains(entry.id)) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral("条目 id 重复：%1").arg(entry.id);
            }
            return false;
        }
        ids.insert(entry.id);
        if (!require_text(entry.purpose, entry.id, "purpose", error_message)
            || !require_text(entry.method_overview, entry.id, "method_overview", error_message)
            || !require_text(entry.input_description, entry.id, "input_description", error_message)
            || !require_text(entry.missing_value_policy, entry.id, "missing_value_policy",
                             error_message)
            || !require_text(entry.invalid_input_conditions, entry.id, "invalid_input_conditions",
                             error_message)
            || !require_text(entry.output_interpretation, entry.id, "output_interpretation",
                             error_message)
            || !require_text(entry.interpretation_limits, entry.id, "interpretation_limits",
                             error_message)) {
            return false;
        }
        if (entry.calculation_steps.isEmpty() || entry.symbol_definitions.isEmpty()
            || entry.decision_rules.isEmpty() || entry.formula_blocks.isEmpty()
            || entry.reference_links.isEmpty()) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral(
                    "条目 %1 缺少计算步骤、符号、判定规则、公式或参考链接。")
                                     .arg(entry.id);
            }
            return false;
        }
        if (looks_like_markdown_only(entry.purpose)
            || looks_like_markdown_only(entry.method_overview)) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral("条目 %1 的用户说明不能只引用仓库文档。")
                                     .arg(entry.id);
            }
            return false;
        }
        for (const FormulaBlock& block : entry.formula_blocks) {
            if (block.title.isEmpty()
                || (block.plain_text.isEmpty() && block.nodes.isEmpty())) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral("条目 %1 的公式块缺少标题或内容。")
                                         .arg(entry.id);
                }
                return false;
            }
            if (looks_like_markdown_only(block.plain_text)
                || looks_like_markdown_only(block.explanation)) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral("条目 %1 的公式不能只引用仓库文档。")
                                         .arg(entry.id);
                }
                return false;
            }
            if (!validate_formula_nodes(block.nodes, error_message)) {
                return false;
            }
        }
        for (const HelpSymbol& symbol : entry.symbol_definitions) {
            if (symbol.symbol.isEmpty() || symbol.meaning.isEmpty()) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral("条目 %1 的符号表不完整。").arg(entry.id);
                }
                return false;
            }
        }
        for (const HelpReferenceLink& link : entry.reference_links) {
            if (!link.url.startsWith(QStringLiteral("https://"))) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral("条目 %1 的参考链接必须是 https://。")
                                          .arg(entry.id);
                }
                return false;
            }
            if (link.accessed.isEmpty() || link.label.isEmpty()) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral("条目 %1 的参考链接缺少名称或访问日期。")
                                          .arg(entry.id);
                }
                return false;
            }
        }
    }
    return true;
}

std::optional<AlgorithmHelpEntry> AlgorithmHelpCatalogLoader::find_by_id(
    const AlgorithmHelpCatalog& catalog, const QString& id)
{
    for (const AlgorithmHelpEntry& entry : catalog.entries) {
        if (entry.id == id) {
            return entry;
        }
    }
    return std::nullopt;
}
