#include "infrastructure/report_painter.h"

#include <QFontMetrics>

#include <algorithm>

namespace datalab::infrastructure {

std::vector<double> ReportPainter::measure_column_widths(
    const std::vector<std::string>& headers,
    const std::vector<std::vector<std::string>>& rows,
    const QFont& header_font,
    const QFont& cell_font,
    double available_width)
{
    QFontMetrics header_metrics(header_font);
    QFontMetrics cell_metrics(cell_font);
    std::vector<double> widths(headers.size(), 48.0);
    for (std::size_t column = 0; column < headers.size(); ++column) {
        double width = header_metrics.horizontalAdvance(
            QString::fromStdString(headers[column])) + 16.0;
        for (const auto& row : rows) {
            if (column < row.size()) {
                width = std::max(width, static_cast<double>(
                    cell_metrics.horizontalAdvance(QString::fromStdString(row[column])) + 16));
            }
        }
        widths[column] = std::min(width, 150.0);
    }
    double total = 0.0;
    for (const double width : widths) {
        total += width;
    }
    if (total > available_width && total > 0.0) {
        const double scale = available_width / total;
        for (double& width : widths) {
            width = std::max(36.0, width * scale);
        }
        double adjusted_total = 0.0;
        for (const double width : widths) {
            adjusted_total += width;
        }
        if (adjusted_total > available_width && adjusted_total > 0.0) {
            const double second_scale = available_width / adjusted_total;
            for (double& width : widths) {
                width *= second_scale;
            }
        }
    }
    return widths;
}

double ReportPainter::wrapped_height(
    const QString& text, const QFont& font, double width, double minimum_height)
{
    QFontMetrics metrics(font);
    const QRect bounds = metrics.boundingRect(
        QRect(0, 0, std::max(1, static_cast<int>(width)), 10000),
        Qt::TextWordWrap, text);
    return std::max(minimum_height, static_cast<double>(bounds.height()));
}

QRectF ReportPainter::contain(const QRectF& bounds, double aspect_ratio)
{
    if (bounds.isEmpty() || aspect_ratio <= 0.0) {
        return bounds;
    }
    double width = bounds.width();
    double height = width / aspect_ratio;
    if (height > bounds.height()) {
        height = bounds.height();
        width = height * aspect_ratio;
    }
    return QRectF(
        bounds.left() + (bounds.width() - width) / 2.0,
        bounds.top() + (bounds.height() - height) / 2.0,
        width, height);
}

void ReportPainter::paint_paginated_table(
    std::size_t row_count,
    double header_height,
    const std::function<double(std::size_t row_index)>& row_height_at,
    const std::function<bool(double needed_height)>& needs_page_break,
    const std::function<void()>& begin_new_page,
    const std::function<void()>& paint_header,
    const std::function<void(std::size_t row_index)>& paint_row)
{
    if (!paint_header || !paint_row || !needs_page_break || !begin_new_page || !row_height_at) {
        return;
    }
    paint_header();
    for (std::size_t row_index = 0; row_index < row_count; ++row_index) {
        const double row_height = row_height_at(row_index);
        if (needs_page_break(row_height + header_height)) {
            begin_new_page();
            paint_header();
        }
        paint_row(row_index);
    }
}

}  // namespace datalab::infrastructure
