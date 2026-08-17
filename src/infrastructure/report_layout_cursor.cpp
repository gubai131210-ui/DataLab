#include "report_layout_cursor.h"

#include <QFontMetricsF>

ReportLayoutCursor::ReportLayoutCursor(const QRectF& page_rect, double line_spacing)
    : page_rect_(page_rect)
    , line_spacing_(line_spacing)
    , y_(page_rect.top())
{
}

double ReportLayoutCursor::measure_text(const QString& text, const QFont& font) const
{
    const QFontMetricsF metrics(font);
    return metrics.boundingRect(
        QRectF(0.0, 0.0, page_rect_.width(), 100000.0),
        Qt::TextWordWrap,
        text).height() + line_spacing_;
}

void ReportLayoutCursor::advance(double height)
{
    y_ += height;
}

bool ReportLayoutCursor::needs_page_break(double height) const
{
    return y_ + height > page_rect_.bottom();
}

void ReportLayoutCursor::new_page()
{
    ++page_number_;
    y_ = page_rect_.top();
}

double ReportLayoutCursor::x() const { return page_rect_.left(); }
double ReportLayoutCursor::y() const { return y_; }
int ReportLayoutCursor::page_number() const { return page_number_; }
