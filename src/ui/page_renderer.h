#pragma once

// 分析页共享渲染器（阶段 3.4）：output_workspace 与 report_preview_dialog 原先
// 各自内联"标题/方法卡/解释/表格/诊断/图"的渲染，此处收敛为单一实现。
// 差异点用选项参数化：图是否交互（workspace 连接行选择/属性写回，预览不交互）、
// 是否显示方法名卡（workspace 显示方法名 + 参数摘要，预览只显示摘要）。

#include "domain/quality_types.h"

#include <QString>

#include <cstddef>
#include <functional>
#include <vector>

class ChartModel;
class QWidget;

namespace page_renderer {

struct PageRenderOptions {
    // 图表是否交互：true 时连接 rows_selected 与 display_properties_changed。
    bool interactive_charts = false;
    // 是否渲染"方法名 + 参数摘要"卡（false 时仅在有摘要时渲染摘要卡）。
    bool include_method = true;
    // 图表显示属性变更回调（interactive_charts 时使用）。
    std::function<void(std::size_t plot_index, const ChartModel&)>
        on_display_properties_changed;
    // 图表行选择回调（interactive_charts 时使用）。
    std::function<void(const std::vector<std::size_t>&)> on_rows_selected;
    // 报告/预览图表空态与占位文案 locale（默认 zh-CN；预览/PDF 应传入 ReportProfile locale）。
    std::string chart_language_tag = "zh-CN";
};

// 分析图标资源路径（页标题/页签图标共用；缺省回退 report.svg）。
QString icon_resource(const std::string& analysis_id);

// 构建单个分析页的内容控件（标题行/方法卡/解释卡/表格/诊断/图）。
// 调用方负责放入滚动区并在页间插入分隔。
QWidget* build_page_widget(
    const datalab::domain::OutputPage& page,
    QWidget* parent,
    const PageRenderOptions& options = {});

}  // namespace page_renderer
