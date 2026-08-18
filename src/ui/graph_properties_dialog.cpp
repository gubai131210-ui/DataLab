#include "ui/graph_properties_dialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>

namespace {

QString line_style_name(const ChartLineStyle style)
{
    switch (style) {
    case ChartLineStyle::Dash: return QStringLiteral("虚线");
    case ChartLineStyle::Dot: return QStringLiteral("点线");
    case ChartLineStyle::DashDot: return QStringLiteral("点划线");
    case ChartLineStyle::Solid:
    default: return QStringLiteral("实线");
    }
}

QString point_style_name(const ChartPointStyle style)
{
    switch (style) {
    case ChartPointStyle::Circle: return QStringLiteral("圆形");
    case ChartPointStyle::Square: return QStringLiteral("方形");
    case ChartPointStyle::Triangle: return QStringLiteral("三角形");
    case ChartPointStyle::Cross: return QStringLiteral("十字");
    case ChartPointStyle::None:
    default: return QStringLiteral("无");
    }
}

ChartLineStyle line_style_from_index(const int index)
{
    switch (index) {
    case 1: return ChartLineStyle::Dash;
    case 2: return ChartLineStyle::Dot;
    case 3: return ChartLineStyle::DashDot;
    case 0:
    default: return ChartLineStyle::Solid;
    }
}

ChartPointStyle point_style_from_index(const int index)
{
    switch (index) {
    case 1: return ChartPointStyle::Circle;
    case 2: return ChartPointStyle::Square;
    case 3: return ChartPointStyle::Triangle;
    case 4: return ChartPointStyle::Cross;
    case 0:
    default: return ChartPointStyle::None;
    }
}

void set_color_button(QPushButton* button, const QString& color)
{
    const QColor configured(color);
    const QColor safe = configured.isValid() ? configured : QColor("#455a64");
    button->setProperty("chartColor", safe.name(QColor::HexArgb));
    button->setText(safe.name(QColor::HexArgb));
    button->setStyleSheet(QStringLiteral("background-color: %1;").arg(safe.name()));
}

QString color_from_button(const QPushButton* button)
{
    const QString color = button->property("chartColor").toString();
    return QColor(color).isValid() ? color : QStringLiteral("#455a64");
}

}  // namespace

GraphPropertiesDialog::GraphPropertiesDialog(const ChartModel& model, QWidget* parent)
    : QDialog(parent), model_(model)
{
    setWindowTitle(QStringLiteral("编辑图形属性"));
    auto* layout = new QFormLayout(this);
    title_ = new QLineEdit(model_.title, this);
    subtitle_ = new QLineEdit(model_.subtitle, this);
    x_axis_title_ = new QLineEdit(model_.x_axis_title, this);
    y_axis_title_ = new QLineEdit(model_.y_axis_title, this);
    grid_ = new QCheckBox(QStringLiteral("显示网格"), this);
    grid_->setChecked(model_.show_grid);
    legend_ = new QCheckBox(QStringLiteral("显示图例"), this);
    legend_->setChecked(model_.show_legend);
    line_width_ = new QDoubleSpinBox(this);
    line_width_->setRange(0.5, 8.0);
    line_width_->setSingleStep(0.5);
    line_width_->setValue(model_.line_width);
    legend_font_size_ = new QSpinBox(this);
    legend_font_size_->setRange(6, 24);
    legend_font_size_->setValue(model_.legend_font_size);
    const auto create_color_editor = [this](QPushButton*& target, const QString& color) {
        target = new QPushButton(this);
        set_color_button(target, color);
        connect(target, &QPushButton::clicked, this, [target, this]() {
            const QColor selected = QColorDialog::getColor(
                QColor(color_from_button(target)), this, QStringLiteral("选择参考线颜色"));
            if (selected.isValid()) {
                set_color_button(target, selected.name(QColor::HexArgb));
            }
        });
        return target;
    };
    value_color_ = create_color_editor(value_color_, model_.value_style.color);
    center_color_ = create_color_editor(center_color_, model_.center_style.color);
    lower_color_ = create_color_editor(lower_color_, model_.lower_style.color);
    upper_color_ = create_color_editor(upper_color_, model_.upper_style.color);
    layout->addRow(QStringLiteral("标题"), title_);
    layout->addRow(QStringLiteral("副标题"), subtitle_);
    layout->addRow(QStringLiteral("X 轴标题"), x_axis_title_);
    layout->addRow(QStringLiteral("Y 轴标题"), y_axis_title_);
    layout->addRow(grid_);
    layout->addRow(legend_);
    layout->addRow(QStringLiteral("线宽"), line_width_);
    layout->addRow(QStringLiteral("图例字号"), legend_font_size_);
    layout->addRow(QStringLiteral("观测值颜色"), value_color_);
    layout->addRow(QStringLiteral("中心线颜色"), center_color_);
    layout->addRow(QStringLiteral("下控制限颜色"), lower_color_);
    layout->addRow(QStringLiteral("上控制限颜色"), upper_color_);

    series_table_ = new QTableWidget(this);
    series_table_->setColumnCount(6);
    series_table_->setHorizontalHeaderLabels({
        QStringLiteral("显示"), QStringLiteral("图例名称"), QStringLiteral("颜色"),
        QStringLiteral("线型"), QStringLiteral("点型"), QStringLiteral("线宽")});
    series_table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    series_table_->setMinimumHeight(100);
    series_table_->setRowCount(static_cast<int>(model_.series.size()));
    for (int row = 0; row < series_table_->rowCount(); ++row) {
        const ChartSeries& series = model_.series[static_cast<std::size_t>(row)];
        auto* visible = new QCheckBox(series_table_);
        visible->setChecked(series.style.visible);
        series_table_->setCellWidget(row, 0, visible);
        series_table_->setItem(row, 1, new QTableWidgetItem(series.label));

        auto* color = new QPushButton(series_table_);
        set_color_button(color, series.style.color);
        connect(color, &QPushButton::clicked, this, [color, this]() {
            const QColor selected = QColorDialog::getColor(
                QColor(color_from_button(color)), this, QStringLiteral("选择系列颜色"));
            if (selected.isValid()) {
                set_color_button(color, selected.name(QColor::HexArgb));
            }
        });
        series_table_->setCellWidget(row, 2, color);

        auto* line = new QComboBox(series_table_);
        for (int index = 0; index < 4; ++index) {
            line->addItem(line_style_name(line_style_from_index(index)));
        }
        line->setCurrentIndex(static_cast<int>(series.style.line_style));
        series_table_->setCellWidget(row, 3, line);

        auto* point = new QComboBox(series_table_);
        for (int index = 0; index < 5; ++index) {
            point->addItem(point_style_name(point_style_from_index(index)));
        }
        point->setCurrentIndex(static_cast<int>(series.style.point_style));
        series_table_->setCellWidget(row, 4, point);

        auto* width = new QDoubleSpinBox(series_table_);
        width->setRange(0.5, 8.0);
        width->setSingleStep(0.5);
        width->setValue(series.style.line_width);
        series_table_->setCellWidget(row, 5, width);
    }
    layout->addRow(QStringLiteral("数据系列"), series_table_);
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
    result.x_axis_title = x_axis_title_->text();
    result.y_axis_title = y_axis_title_->text();
    result.show_grid = grid_->isChecked();
    result.show_legend = legend_->isChecked();
    result.line_width = line_width_->value();
    result.legend_font_size = legend_font_size_->value();
    result.value_style.color = color_from_button(value_color_);
    result.center_style.color = color_from_button(center_color_);
    result.lower_style.color = color_from_button(lower_color_);
    result.upper_style.color = color_from_button(upper_color_);
    for (int row = 0; row < series_table_->rowCount(); ++row) {
        ChartSeries& series = result.series[static_cast<std::size_t>(row)];
        auto* visible = qobject_cast<QCheckBox*>(series_table_->cellWidget(row, 0));
        auto* color = qobject_cast<QPushButton*>(series_table_->cellWidget(row, 2));
        auto* line = qobject_cast<QComboBox*>(series_table_->cellWidget(row, 3));
        auto* point = qobject_cast<QComboBox*>(series_table_->cellWidget(row, 4));
        auto* width = qobject_cast<QDoubleSpinBox*>(series_table_->cellWidget(row, 5));
        if (auto* label = series_table_->item(row, 1)) {
            series.label = label->text().trimmed();
        }
        series.style.visible = visible != nullptr && visible->isChecked();
        series.style.color = color_from_button(color);
        series.style.line_style = line_style_from_index(line == nullptr ? 0 : line->currentIndex());
        series.style.point_style =
            point_style_from_index(point == nullptr ? 0 : point->currentIndex());
        series.style.line_width = width == nullptr ? 1.8 : width->value();
        series.show_points = series.style.point_style != ChartPointStyle::None;
    }
    return result;
}
