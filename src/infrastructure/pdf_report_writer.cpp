#include "infrastructure/pdf_report_writer.h"

#include "infrastructure/report_layout_cursor.h"
#include "infrastructure/report_painter.h"
#include "reporting/chart_adapter.h"
#include "reporting/chart_renderer.h"

#include <QDateTime>
#include <QFontDatabase>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QPdfWriter>

#include <algorithm>
#include <numeric>

namespace datalab::infrastructure {


bool PdfReportWriter::write(
    const QString& file_path,
    const domain::DataTable& table,
    const std::vector<domain::OutputPage>& pages,
    QString* error_message)
{
    QPdfWriter writer(file_path);
    writer.setResolution(72);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    writer.setTitle(QStringLiteral("DataLab 质量分析报告"));
    QPainter painter(&writer);
    if (!painter.isActive()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("无法创建 PDF 报告。");
        }
        return false;
    }

    const QString family = QFontDatabase().families().contains(QStringLiteral("Microsoft YaHei"))
        ? QStringLiteral("Microsoft YaHei")
        : QStringLiteral("SimSun");
    const QRectF page_rect(90.0, 100.0, writer.width() - 180.0, writer.height() - 190.0);
    ReportLayoutCursor cursor(page_rect, 8.0);
    const QFont body_font(family, 10);
    const QFont heading_font(family, 13, QFont::Bold);
    const QFont title_font(family, 18, QFont::Bold);

    const auto draw_footer = [&] {
        painter.setFont(QFont(family, 8));
        painter.setPen(QColor("#68737d"));
        painter.drawText(QRectF(90.0, writer.height() - 65.0, writer.width() - 180.0, 20.0),
                         Qt::AlignCenter,
                         QStringLiteral("DataLab 质量分析报告  ·  第 %1 页").arg(cursor.page_number()));
    };
    const auto new_page = [&] {
        draw_footer();
        writer.newPage();
        cursor.new_page();
    };
    const auto ensure_space = [&](double height) {
        if (cursor.needs_page_break(height)) {
            new_page();
        }
    };
    const auto draw_wrapped = [&](const QString& text, const QFont& font, double indent = 0.0) {
        const double height = cursor.measure_text(text, font);
        ensure_space(height);
        painter.setFont(font);
        painter.setPen(Qt::black);
        painter.drawText(QRectF(cursor.x() + indent, cursor.y(), page_rect.width() - indent, height),
                         Qt::TextWordWrap, text);
        cursor.advance(height);
    };

    draw_wrapped(QStringLiteral("DataLab 质量分析报告"), title_font);
    draw_wrapped(QStringLiteral("生成时间：") + QDateTime::currentDateTime().toString(Qt::ISODate), body_font);
    draw_wrapped(QStringLiteral("数据源：") + QString::fromStdString(table.source_path), body_font);
    draw_wrapped(
        QStringLiteral("样本数：%1    字段数：%2    输出页：%3")
            .arg(static_cast<qulonglong>(table.rows.size()))
            .arg(static_cast<qulonglong>(table.columns.size()))
            .arg(static_cast<qulonglong>(pages.size())),
        body_font);

    if (pages.empty()) {
        draw_wrapped(QStringLiteral("暂无分析结果。"), body_font);
    }

    for (const domain::OutputPage& page : pages) {
        ensure_space(80.0);
        draw_wrapped(QString::fromStdString(page.title), heading_font);
        draw_wrapped(QString::fromStdString(page.method_name + "    " + page.parameter_summary), body_font);
        for (const auto& section : page.interpretation) {
            draw_wrapped(QStringLiteral("【") + QString::fromStdString(section.heading) + QStringLiteral("】"),
                         QFont(family, 11, QFont::Bold));
            for (const auto& bullet : section.bullets) {
                draw_wrapped(QStringLiteral("• ") + QString::fromStdString(bullet), body_font, 8.0);
            }
        }
        for (const auto& table_block : page.tables) {
            draw_wrapped(QString::fromStdString(table_block.title), QFont(family, 11, QFont::Bold));
            const QFont header_font(family, 8, QFont::Bold);
            const QFont cell_font(family, 8);
            QFontMetrics header_metrics(header_font);
            QFontMetrics cell_metrics(cell_font);
            const std::vector<double> column_widths = ReportPainter::measure_column_widths(
                table_block.headers, table_block.rows, header_font, cell_font, page_rect.width());
            const auto row_height_for = [&](const std::vector<std::string>& row) {
                double height = 24.0;
                for (std::size_t column = 0; column < table_block.headers.size(); ++column) {
                    const QString text = column < row.size()
                        ? QString::fromStdString(row[column]) : QString();
                    const QRect bounds = cell_metrics.boundingRect(
                        QRect(0, 0, static_cast<int>(column_widths[column] - 8.0), 1000),
                        Qt::TextWordWrap, text);
                    height = std::max(height, bounds.height() + 8.0);
                }
                return std::min(height, 72.0);
            };
            ensure_space(32.0);
            painter.setFont(QFont(family, 8, QFont::Bold));
            double x = page_rect.left();
            for (std::size_t column = 0; column < table_block.headers.size(); ++column) {
                const QRectF cell(x, cursor.y(), column_widths[column], 32.0);
                painter.fillRect(cell, QColor("#e8f0f7"));
                painter.drawRect(cell);
                painter.drawText(cell.adjusted(4, 4, -4, -4), Qt::TextWordWrap | Qt::AlignVCenter,
                                 QString::fromStdString(table_block.headers[column]));
                x += column_widths[column];
            }
            cursor.advance(32.0);
            painter.setFont(cell_font);
            for (const auto& row : table_block.rows) {
                const double row_height = row_height_for(row);
                ensure_space(row_height);
                x = page_rect.left();
                for (std::size_t column = 0; column < table_block.headers.size(); ++column) {
                    const QRectF cell(x, cursor.y(), column_widths[column], row_height);
                    painter.drawRect(cell);
                    const QString text = column < row.size()
                        ? QString::fromStdString(row[column])
                        : QString();
                    painter.drawText(cell.adjusted(4, 4, -4, -4),
                                     Qt::TextWordWrap | Qt::AlignVCenter, text);
                    x += column_widths[column];
                }
                cursor.advance(row_height);
            }
        }
        for (const auto& diagnostic : page.diagnostics) {
            draw_wrapped(QStringLiteral("• ") + QString::fromStdString(diagnostic.message), body_font, 8.0);
        }
        if (page.method_name == "Capability Sixpack" && page.plots.size() >= 6) {
            const double grid_height = std::min(540.0, page_rect.bottom() - cursor.y());
            ensure_space(grid_height);
            const double cell_width = page_rect.width() / 2.0 - 8.0;
            const double cell_height = grid_height / 3.0 - 8.0;
            for (std::size_t index = 0; index < 6; ++index) {
                const int row = static_cast<int>(index / 2);
                const int column = static_cast<int>(index % 2);
                const QRectF cell(
                    page_rect.left() + column * (cell_width + 16.0),
                    cursor.y() + row * (cell_height + 8.0),
                    cell_width, cell_height);
                ChartRenderer::render(
                    painter, ReportPainter::contain(cell, 16.0 / 9.0),
                    chart_model_from_plot(page.plots[index]));
            }
            cursor.advance(grid_height);
        } else {
            for (const auto& plot : page.plots) {
                const QRectF chart_bounds(page_rect.left(), cursor.y(), page_rect.width(), 340.0);
                const QRectF chart_rect = ReportPainter::contain(chart_bounds, 16.0 / 9.0);
                ensure_space(chart_rect.height() + 18.0);
                ChartRenderer::render(
                    painter,
                    chart_rect,
                    chart_model_from_plot(plot));
                cursor.advance(chart_rect.height() + 18.0);
            }
        }
    }
    draw_footer();
    painter.end();
    return true;
}

}  // namespace datalab::infrastructure
