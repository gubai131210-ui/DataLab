#include "ui/report_preview_dialog.h"

#include "ui/page_renderer.h"

#include <QDialogButtonBox>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

ReportPreviewDialog::ReportPreviewDialog(
    const std::vector<datalab::domain::OutputPage>& pages,
    QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("PDF 导出预览"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/export-pdf.svg")));
    resize(980, 720);
    auto* outer = new QVBoxLayout(this);
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto* content = new QWidget(scroll);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(28, 24, 28, 28);
    layout->setSpacing(14);

    for (std::size_t index = 0; index < pages.size(); ++index) {
        // 预览不交互、不显示方法名（渲染器仅在存在参数摘要时显示摘要卡）。
        page_renderer::PageRenderOptions options;
        options.include_method = false;
        layout->addWidget(page_renderer::build_page_widget(pages[index], content, options));
        if (index + 1 < pages.size()) {
            auto* separator = new QLabel(QStringLiteral("— 下一页 —"), content);
            separator->setAlignment(Qt::AlignCenter);
            layout->addWidget(separator);
        }
    }
    layout->addStretch();
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Cancel | QDialogButtonBox::Ok, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确认导出"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    buttons->button(QDialogButtonBox::Ok)->setIcon(
        QIcon(QStringLiteral(":/icons/export-pdf.svg")));
    buttons->button(QDialogButtonBox::Cancel)->setIcon(
        QIcon(QStringLiteral(":/icons/error.svg")));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    outer->addWidget(buttons);
}
