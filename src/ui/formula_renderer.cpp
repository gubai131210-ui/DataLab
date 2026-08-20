#include "ui/formula_renderer.h"

#include <QStringList>

namespace {

QString render_nodes(const QVector<FormulaNode>& nodes, bool html);

QString math_font_wrap(const QString& inner)
{
    return QStringLiteral(
               "<span style=\"font-family:'Cambria Math','Segoe UI','Microsoft YaHei',serif; "
               "font-size:16px; line-height:1.85;\">%1</span>")
        .arg(inner);
}

QString render_node(const FormulaNode& node, bool html)
{
    if (node.type == QStringLiteral("heading")) {
        if (html) {
            return QStringLiteral(
                       "<div style=\"font-weight:700; margin:10px 0 4px 0; font-size:14px;\">%1</div>")
                .arg(FormulaRenderer::escape_html(node.value));
        }
        return node.value;
    }
    if (node.type == QStringLiteral("text") || node.type == QStringLiteral("inline")
        || node.type == QStringLiteral("operator")) {
        return html ? FormulaRenderer::escape_html(node.value) : node.value;
    }
    if (node.type == QStringLiteral("sup")) {
        const QString inner = render_nodes(node.content, html);
        return html ? QStringLiteral("<sup>%1</sup>").arg(inner)
                    : QStringLiteral("^(%1)").arg(inner);
    }
    if (node.type == QStringLiteral("sub")) {
        const QString inner = render_nodes(node.content, html);
        return html ? QStringLiteral("<sub>%1</sub>").arg(inner)
                    : QStringLiteral("_%1").arg(inner);
    }
    if (node.type == QStringLiteral("bar")) {
        const QString inner = render_nodes(node.content, html);
        if (html) {
            return QStringLiteral(
                       "<span style=\"display:inline-block; border-top:1px solid currentColor; "
                       "padding:1px 2px 0 2px; line-height:1.15; min-width:0.7em; "
                       "text-align:center;\">%1</span>")
                .arg(inner);
        }
        return QStringLiteral("bar(%1)").arg(inner);
    }
    if (node.type == QStringLiteral("group")) {
        const QString inner = render_nodes(node.content, html);
        return QStringLiteral("(%1)").arg(inner);
    }
    if (node.type == QStringLiteral("abs")) {
        const QString inner = render_nodes(node.content, html);
        return QStringLiteral("|%1|").arg(inner);
    }
    if (node.type == QStringLiteral("sqrt")) {
        const QString inner = render_nodes(node.content, html);
        if (html) {
            return QStringLiteral(
                       "<span style=\"white-space:nowrap; display:inline-block; vertical-align:middle;\">"
                       "<span style=\"font-size:130%; line-height:1;\">&radic;</span>"
                       "<span style=\"border-top:1px solid currentColor; padding:0 3px 1px 2px;\">%1</span>"
                       "</span>")
                .arg(inner);
        }
        return QStringLiteral("sqrt(%1)").arg(inner);
    }
    if (node.type == QStringLiteral("frac")) {
        const QString num = render_nodes(node.numerator, html);
        const QString den = render_nodes(node.denominator, html);
        if (html) {
            return QStringLiteral(
                       "<table style=\"display:inline-table; vertical-align:middle; "
                       "border-collapse:collapse; margin:0 4px;\">"
                       "<tr><td style=\"border-bottom:1px solid currentColor; padding:1px 8px; "
                       "text-align:center; font-size:15px;\">%1</td></tr>"
                       "<tr><td style=\"padding:1px 8px; text-align:center; font-size:15px;\">%2</td></tr>"
                       "</table>")
                .arg(num, den);
        }
        return QStringLiteral("(%1)/(%2)").arg(num, den);
    }
    if (node.type == QStringLiteral("line")) {
        const QString inner = render_nodes(node.content, html);
        if (html) {
            return QStringLiteral("<div style=\"margin:3px 0;\">%1</div>").arg(inner);
        }
        return inner;
    }
    if (node.type == QStringLiteral("stack")) {
        if (html) {
            QString inner;
            for (const FormulaNode& child : node.content) {
                inner += render_node(child, true);
            }
            return QStringLiteral("<div style=\"margin:4px 0 8px 0;\">%1</div>").arg(inner);
        }
        QStringList lines;
        for (const FormulaNode& child : node.content) {
            lines.push_back(render_node(child, false));
        }
        return lines.join(QStringLiteral("\n"));
    }
    if (node.type == QStringLiteral("case")) {
        const QString result = render_nodes(node.content, html);
        const QString when = render_nodes(node.when, html);
        if (html) {
            if (when.isEmpty()) {
                return QStringLiteral(
                           "<tr><td style=\"padding:2px 4px 2px 0; white-space:nowrap;\">%1</td></tr>")
                    .arg(result);
            }
            return QStringLiteral(
                       "<tr><td style=\"padding:2px 10px 2px 0; white-space:nowrap;\">%1</td>"
                       "<td style=\"padding:2px 0; color:#555;\">，当 %2</td></tr>")
                .arg(result, when);
        }
        if (when.isEmpty()) {
            return result;
        }
        return QStringLiteral("%1  ，当 %2").arg(result, when);
    }
    if (node.type == QStringLiteral("piecewise")) {
        if (html) {
            QString rows;
            for (const FormulaNode& child : node.content) {
                rows += render_node(child, true);
            }
            return QStringLiteral(
                       "<table style=\"border-collapse:collapse; margin:6px 0 10px 4px;\">"
                       "<tr>"
                       "<td style=\"font-size:42px; font-weight:200; vertical-align:middle; "
                       "padding:0 8px 0 0; line-height:0.85;\">{</td>"
                       "<td style=\"vertical-align:middle;\">"
                       "<table style=\"border-collapse:collapse;\">%1</table>"
                       "</td>"
                       "</tr></table>")
                .arg(rows);
        }
        QStringList lines;
        for (const FormulaNode& child : node.content) {
            lines.push_back(render_node(child, false));
        }
        return QStringLiteral("{\n  %1\n}").arg(lines.join(QStringLiteral("\n  ")));
    }
    if (node.type == QStringLiteral("matrix")) {
        if (html) {
            QString rows_html;
            for (const auto& row : node.rows) {
                rows_html += QStringLiteral("<tr>");
                for (const FormulaNode& cell : row) {
                    rows_html += QStringLiteral("<td style=\"padding:2px 8px; text-align:center;\">%1</td>")
                                     .arg(render_node(cell, true));
                }
                rows_html += QStringLiteral("</tr>");
            }
            return QStringLiteral(
                       "<table style=\"display:inline-table; border-collapse:collapse; "
                       "border-left:2px solid currentColor; border-right:2px solid currentColor; "
                       "margin:0 4px;\">%1</table>")
                .arg(rows_html);
        }
        QStringList row_texts;
        for (const auto& row : node.rows) {
            QStringList cells;
            for (const FormulaNode& cell : row) {
                cells.push_back(render_node(cell, false));
            }
            row_texts.push_back(QStringLiteral("[") + cells.join(QStringLiteral(", "))
                                + QStringLiteral("]"));
        }
        return row_texts.join(QStringLiteral("; "));
    }
    return html ? FormulaRenderer::escape_html(node.value) : node.value;
}

QString render_nodes(const QVector<FormulaNode>& nodes, bool html)
{
    QString out;
    for (const FormulaNode& node : nodes) {
        out += render_node(node, html);
    }
    return out;
}

}  // namespace

QString FormulaRenderer::escape_html(const QString& text)
{
    QString escaped = text;
    escaped.replace('&', QStringLiteral("&amp;"));
    escaped.replace('<', QStringLiteral("&lt;"));
    escaped.replace('>', QStringLiteral("&gt;"));
    escaped.replace('"', QStringLiteral("&quot;"));
    return escaped;
}

QString FormulaRenderer::plain_text_to_html(const QString& text)
{
    QString normalized = text;
    normalized.replace(QStringLiteral("；"), QStringLiteral("\n"));
    const QStringList lines = normalized.split('\n', Qt::SkipEmptyParts);
    QString html = QStringLiteral("<div style=\"margin:4px 0;\">");
    for (QString line : lines) {
        line = line.trimmed();
        if (line.endsWith(QStringLiteral("。")) && line.size() > 1) {
            line.chop(1);
        }
        html += QStringLiteral("<div style=\"margin:3px 0;\">%1</div>")
                    .arg(escape_html(line));
    }
    html += QStringLiteral("</div>");
    return math_font_wrap(html);
}

QString FormulaRenderer::to_html(const QVector<FormulaNode>& nodes)
{
    return math_font_wrap(render_nodes(nodes, true));
}

QString FormulaRenderer::to_plain_text(const QVector<FormulaNode>& nodes)
{
    return render_nodes(nodes, false);
}
