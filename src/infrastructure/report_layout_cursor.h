#pragma once

#include <QFont>
#include <QRectF>
#include <QString>

class ReportLayoutCursor final {
public:
    ReportLayoutCursor(const QRectF& page_rect, double line_spacing);

    double measure_text(const QString& text, const QFont& font) const;
    void advance(double height);
    bool needs_page_break(double height) const;
    void new_page();

    double x() const;
    double y() const;
    int page_number() const;

private:
    QRectF page_rect_;
    double line_spacing_ = 0.0;
    double y_ = 0.0;
    int page_number_ = 1;
};
