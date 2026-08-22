#pragma once

#include <QPixmap>
#include <QString>

#include <cstddef>

namespace datalab::ui {

QString row_visibility_footnote(
    std::size_t excluded_count,
    std::size_t hidden_count,
    std::size_t analysis_n = 0,
    std::size_t display_n = 0);

QString append_clipboard_footnote_comments(const QString& body, const QString& footnote);

QPixmap compose_chart_pixmap_with_footnote(
    const QPixmap& chart,
    const QString& footnote);

}  // namespace datalab::ui
