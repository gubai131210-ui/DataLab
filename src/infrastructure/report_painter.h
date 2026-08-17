#pragma once

#include <QFont>
#include <QRectF>
#include <QString>

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
};

}  // namespace datalab::infrastructure
