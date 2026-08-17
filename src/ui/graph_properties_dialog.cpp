#include "ui/graph_properties_dialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>

GraphPropertiesDialog::GraphPropertiesDialog(const ChartModel& model, QWidget* parent)
    : QDialog(parent), model_(model)
{
    setWindowTitle(QStringLiteral("编辑图形属性"));
    auto* layout = new QFormLayout(this);
    title_ = new QLineEdit(model_.title, this);
    subtitle_ = new QLineEdit(model_.subtitle, this);
    grid_ = new QCheckBox(QStringLiteral("显示网格"), this);
    grid_->setChecked(model_.show_grid);
    legend_ = new QCheckBox(QStringLiteral("显示图例"), this);
    legend_->setChecked(model_.show_legend);
    line_width_ = new QDoubleSpinBox(this);
    line_width_->setRange(0.5, 8.0);
    line_width_->setSingleStep(0.5);
    line_width_->setValue(model_.line_width);
    layout->addRow(QStringLiteral("标题"), title_);
    layout->addRow(QStringLiteral("副标题"), subtitle_);
    layout->addRow(grid_);
    layout->addRow(legend_);
    layout->addRow(QStringLiteral("线宽"), line_width_);
    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addRow(buttons);
}

ChartModel GraphPropertiesDialog::model() const
{
    ChartModel result = model_;
    result.title = title_->text();
    result.subtitle = subtitle_->text();
    result.show_grid = grid_->isChecked();
    result.show_legend = legend_->isChecked();
    result.line_width = line_width_->value();
    return result;
}
