#include "ui/row_visibility_clipboard.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPixmap>

namespace datalab::ui {

QString row_visibility_footnote(
    const std::size_t excluded_count,
    const std::size_t hidden_count,
    const std::size_t analysis_n,
    const std::size_t display_n)
{
    if (excluded_count == 0 && hidden_count == 0 && analysis_n == 0 && display_n == 0) {
        return {};
    }
    QString footnote = QStringLiteral(
        "行可见性契约：排除 %1 行（分析与显示均省略）· 隐藏 %2 行（仅显示省略，分析仍纳入）")
                           .arg(static_cast<qulonglong>(excluded_count))
                           .arg(static_cast<qulonglong>(hidden_count));
    if (analysis_n != 0 || display_n != 0) {
        footnote += QStringLiteral(" · 分析 N = %1 · 显示 N = %2")
                        .arg(static_cast<qulonglong>(analysis_n))
                        .arg(static_cast<qulonglong>(display_n));
    }
    footnote += QStringLiteral("（hidden 与 excluded 不得合并叙述）");
    return footnote;
}

QPixmap compose_chart_pixmap_with_footnote(const QPixmap& chart, const QString& footnote)
{
    if (chart.isNull()) {
        return {};
    }
    if (footnote.isEmpty()) {
        return chart;
    }
    const QFont font(QStringLiteral("Segoe UI"), 9);
    QFontMetrics metrics(font);
    const int margin = 8;
    const int text_width = chart.width() - 2 * margin;
    const QRect bounding = metrics.boundingRect(
        QRect(0, 0, text_width, 0),
        Qt::TextWordWrap | Qt::AlignLeft,
        footnote);
    const int footnote_height = bounding.height() + 2 * margin;
    QPixmap composed(chart.width(), chart.height() + footnote_height);
    composed.fill(Qt::white);
    QPainter painter(&composed);
    painter.drawPixmap(0, 0, chart);
    painter.setFont(font);
    painter.setPen(QColor(QStringLiteral("#526a73")));
    painter.drawText(
        QRect(margin, chart.height() + margin, text_width, bounding.height()),
        Qt::TextWordWrap | Qt::AlignLeft,
        footnote);
    return composed;
}

QString append_clipboard_footnote_comments(const QString& body, const QString& footnote)
{
    if (footnote.isEmpty()) {
        return body;
    }
    QStringList lines = body.split(QLatin1Char('\n'));
    for (const QString& line : footnote.split(QLatin1Char('\n'))) {
        if (!line.isEmpty()) {
            lines.push_back(QStringLiteral("# %1").arg(line));
        }
    }
    return lines.join(QLatin1Char('\n'));
}

}  // namespace datalab::ui
