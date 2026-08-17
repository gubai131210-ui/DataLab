#include "ui/analysis_output_view.h"

#include "ui/analysis_chart_widget.h"

#include <QTextEdit>

AnalysisOutputView::AnalysisOutputView(QWidget* parent)
    : QTabWidget(parent)
{
    summary_ = new QTextEdit(this);
    summary_->setReadOnly(true);
    summary_->setPlaceholderText(QStringLiteral("运行分析后显示统计结果、诊断和结论。"));
    chart_ = new AnalysisChartWidget(this);
    connect(chart_, &AnalysisChartWidget::rows_selected,
            this, &AnalysisOutputView::rows_selected);
    addTab(summary_, QStringLiteral("统计摘要"));
    addTab(chart_, QStringLiteral("控制图"));
}

void AnalysisOutputView::show_result(const datalab::domain::AnalysisResult& result)
{
    summary_->clear();
    summary_->append(QStringLiteral("<h2>%1</h2>")
                         .arg(QString::fromStdString(result.analysis_name)));
    for (std::size_t index = 0; index < result.statistic_names.size(); ++index) {
        summary_->append(
            QStringLiteral("<b>%1：</b>%2")
                .arg(QString::fromStdString(result.statistic_names[index]))
                .arg(result.statistic_values[index], 0, 'g', 12));
    }
    summary_->append(QStringLiteral("<h3>诊断信息</h3>"));
    if (result.diagnostics.empty()) {
        summary_->append(QStringLiteral("未发现诊断信息。"));
    } else {
        for (const auto& diagnostic : result.diagnostics) {
            summary_->append(QString::fromStdString(diagnostic.message));
        }
    }
    chart_->set_data(
        result.plotted_values,
        result.center_line,
        result.lower_control_limit,
        result.upper_control_limit);
    setCurrentWidget(summary_);
}

void AnalysisOutputView::set_source_rows(const std::vector<std::size_t>& rows)
{
    chart_->set_source_rows(rows);
}
