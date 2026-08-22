#pragma once

#include <QFont>
#include <QRectF>
#include <QString>

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace datalab::infrastructure {

class ReportPainter final {
public:
    static std::vector<double> measure_column_widths(
        const std::vector<std::string>& headers,
        const std::vector<std::vector<std::string>>& rows,
        const QFont& header_font,
        const QFont& cell_font,
        double available_width);

    static double wrapped_height(
        const QString& text, const QFont& font, double width, double minimum_height = 0.0);

    static QRectF contain(const QRectF& bounds, double aspect_ratio);

    // Shared pagination for PDF tables: paint header, then each row; when the next
    // row would not fit with a repeated header, begin a new page and paint header again.
    // Does not claim PDF/A or PDF/UA compliance.
    static void paint_paginated_table(
        std::size_t row_count,
        double header_height,
        const std::function<double(std::size_t row_index)>& row_height_at,
        const std::function<bool(double needed_height)>& needs_page_break,
        const std::function<void()>& begin_new_page,
        const std::function<void()>& paint_header,
        const std::function<void(std::size_t row_index)>& paint_row);
};

}  // namespace datalab::infrastructure
