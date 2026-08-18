#include "ui/graph_properties_dialog.h"

#include "reporting/chart_renderer.h"

#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <vector>

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
    if (button == nullptr) {
        return QStringLiteral("#455a64");
    }
    const QString color = button->property("chartColor").toString();
    return QColor(color).isValid() ? color : QStringLiteral("#455a64");
}

QComboBox* make_line_combo(QWidget* parent, ChartLineStyle current)
{
    auto* combo = new QComboBox(parent);
    for (int index = 0; index < 4; ++index) {
        combo->addItem(line_style_name(line_style_from_index(index)));
    }
    combo->setCurrentIndex(static_cast<int>(current));
    return combo;
}

QComboBox* make_point_combo(QWidget* parent, ChartPointStyle current)
{
    auto* combo = new QComboBox(parent);
    for (int index = 0; index < 5; ++index) {
        combo->addItem(point_style_name(point_style_from_index(index)));
    }
    combo->setCurrentIndex(static_cast<int>(current));
    return combo;
}

QScrollArea* scrollable_page(QWidget* page, QWidget* parent)
{
    auto* scroll = new QScrollArea(parent);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(page);
    return scroll;
}

}  // namespace

GraphPropertiesDialog::GraphPropertiesDialog(const ChartModel& model, QWidget* parent)
    : QDialog(parent), original_(model), model_(model)
{
    setWindowTitle(QStringLiteral("编辑图形属性"));
    setMinimumSize(960, 620);
    resize(1080, 700);
    auto* root = new QVBoxLayout(this);
    tabs_ = new QTabWidget(this);
    reference_tab_enabled_ = model.kind == ChartKind::Control;

    auto* basic_page = new QWidget(tabs_);
    auto* basic_layout = new QFormLayout(basic_page);
    title_ = new QLineEdit(basic_page);
    subtitle_ = new QLineEdit(basic_page);
    x_axis_title_ = new QLineEdit(basic_page);
    y_axis_title_ = new QLineEdit(basic_page);
    basic_layout->addRow(QStringLiteral("标题"), title_);
    basic_layout->addRow(QStringLiteral("副标题"), subtitle_);
    basic_layout->addRow(QStringLiteral("X 轴标题"), x_axis_title_);
    basic_layout->addRow(QStringLiteral("Y 轴标题"), y_axis_title_);
    tabs_->addTab(scrollable_page(basic_page, tabs_), QStringLiteral("基本信息"));

    auto* axis_page = new QWidget(tabs_);
    auto* axis_layout = new QFormLayout(axis_page);
    grid_ = new QCheckBox(QStringLiteral("显示网格"), axis_page);
    legend_ = new QCheckBox(QStringLiteral("显示图例"), axis_page);
    line_width_ = new QDoubleSpinBox(axis_page);
    line_width_->setRange(0.5, 8.0);
    line_width_->setSingleStep(0.5);
    grid_color_ = new QPushButton(axis_page);
    y_min_auto_ = new QCheckBox(QStringLiteral("自动 Y 最小值"), axis_page);
    y_max_auto_ = new QCheckBox(QStringLiteral("自动 Y 最大值"), axis_page);
    x_min_auto_ = new QCheckBox(QStringLiteral("自动 X 最小值"), axis_page);
    x_max_auto_ = new QCheckBox(QStringLiteral("自动 X 最大值"), axis_page);
    y_min_ = new QDoubleSpinBox(axis_page);
    y_max_ = new QDoubleSpinBox(axis_page);
    x_min_ = new QDoubleSpinBox(axis_page);
    x_max_ = new QDoubleSpinBox(axis_page);
    for (QDoubleSpinBox* box : {y_min_, y_max_, x_min_, x_max_}) {
        box->setRange(-1.0e12, 1.0e12);
        box->setDecimals(6);
        box->setSingleStep(0.1);
    }
    data_region_fill_ = new QPushButton(axis_page);
    clear_y_range_ = new QPushButton(QStringLiteral("清除 Y 范围"), axis_page);
    clear_x_range_ = new QPushButton(QStringLiteral("清除 X 范围"), axis_page);
    axis_layout->addRow(grid_);
    axis_layout->addRow(legend_);
    axis_layout->addRow(QStringLiteral("默认线宽"), line_width_);
    axis_layout->addRow(QStringLiteral("网格颜色"), grid_color_);
    axis_layout->addRow(y_min_auto_);
    axis_layout->addRow(QStringLiteral("Y 最小值"), y_min_);
    axis_layout->addRow(y_max_auto_);
    axis_layout->addRow(QStringLiteral("Y 最大值"), y_max_);
    axis_layout->addRow(x_min_auto_);
    axis_layout->addRow(QStringLiteral("X 最小值"), x_min_);
    axis_layout->addRow(x_max_auto_);
    axis_layout->addRow(QStringLiteral("X 最大值"), x_max_);
    auto* clear_row = new QHBoxLayout();
    clear_row->addWidget(clear_y_range_);
    clear_row->addWidget(clear_x_range_);
    axis_layout->addRow(clear_row);
    axis_layout->addRow(QStringLiteral("数据区填色"), data_region_fill_);
    tabs_->addTab(scrollable_page(axis_page, tabs_), QStringLiteral("坐标轴与网格图例"));

    auto* font_page = new QWidget(tabs_);
    auto* font_layout = new QFormLayout(font_page);
    title_font_size_ = new QSpinBox(font_page);
    title_font_size_->setRange(8, 32);
    axis_font_size_ = new QSpinBox(font_page);
    axis_font_size_->setRange(6, 24);
    legend_font_size_ = new QSpinBox(font_page);
    legend_font_size_->setRange(6, 24);
    theme_preset_ = new QComboBox(font_page);
    theme_preset_->addItem(QStringLiteral("默认"), QStringLiteral("default"));
    theme_preset_->addItem(QStringLiteral("打印"), QStringLiteral("print"));
    theme_preset_->addItem(QStringLiteral("深色"), QStringLiteral("dark"));
    font_layout->addRow(QStringLiteral("标题字号"), title_font_size_);
    font_layout->addRow(QStringLiteral("坐标轴字号"), axis_font_size_);
    font_layout->addRow(QStringLiteral("图例字号"), legend_font_size_);
    font_layout->addRow(QStringLiteral("主题"), theme_preset_);
    tabs_->addTab(scrollable_page(font_page, tabs_), QStringLiteral("字体与主题"));

    auto* series_page = new QWidget(tabs_);
    auto* series_layout = new QVBoxLayout(series_page);
    series_table_ = new QTableWidget(series_page);
    series_table_->setObjectName(QStringLiteral("series_table"));
    series_table_->setColumnCount(9);
    series_table_->setHorizontalHeaderLabels({
        QStringLiteral("显示"), QStringLiteral("名称"), QStringLiteral("颜色"),
        QStringLiteral("填充色"), QStringLiteral("线型"), QStringLiteral("点型"),
        QStringLiteral("线宽"), QStringLiteral("点大小"), QStringLiteral("透明度")});
    series_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    series_table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    series_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    series_table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    for (int column = 4; column < 9; ++column) {
        series_table_->horizontalHeader()->setSectionResizeMode(column, QHeaderView::Stretch);
    }
    series_table_->verticalHeader()->setDefaultSectionSize(26);
    series_table_->horizontalHeader()->setMinimumHeight(34);
    series_table_->setMinimumHeight(160);
    series_table_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    series_table_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    series_table_->setWordWrap(false);
    series_table_->horizontalHeader()->setMinimumSectionSize(72);
    series_layout->addWidget(series_table_);
    value_color_ = new QPushButton(series_page);
    auto* value_form = new QFormLayout();
    value_form->addRow(QStringLiteral("观测值颜色"), value_color_);
    series_layout->addLayout(value_form);
    tabs_->addTab(scrollable_page(series_page, tabs_), QStringLiteral("数据系列"));

    auto* reference_page = new QWidget(tabs_);
    auto* reference_layout = new QFormLayout(reference_page);
    center_visible_ = new QCheckBox(QStringLiteral("显示中心线"), reference_page);
    lower_visible_ = new QCheckBox(QStringLiteral("显示下控制限"), reference_page);
    upper_visible_ = new QCheckBox(QStringLiteral("显示上控制限"), reference_page);
    center_color_ = new QPushButton(reference_page);
    lower_color_ = new QPushButton(reference_page);
    upper_color_ = new QPushButton(reference_page);
    center_style_ = make_line_combo(reference_page, ChartLineStyle::Dash);
    lower_style_ = make_line_combo(reference_page, ChartLineStyle::Dash);
    upper_style_ = make_line_combo(reference_page, ChartLineStyle::Dash);
    center_width_ = new QDoubleSpinBox(reference_page);
    lower_width_ = new QDoubleSpinBox(reference_page);
    upper_width_ = new QDoubleSpinBox(reference_page);
    for (QDoubleSpinBox* box : {center_width_, lower_width_, upper_width_}) {
        box->setRange(0.5, 8.0);
        box->setSingleStep(0.5);
    }
    reference_layout->addRow(center_visible_);
    reference_layout->addRow(QStringLiteral("中心线颜色"), center_color_);
    reference_layout->addRow(QStringLiteral("中心线线型"), center_style_);
    reference_layout->addRow(QStringLiteral("中心线线宽"), center_width_);
    reference_layout->addRow(lower_visible_);
    reference_layout->addRow(QStringLiteral("LCL 颜色"), lower_color_);
    reference_layout->addRow(QStringLiteral("LCL 线型"), lower_style_);
    reference_layout->addRow(QStringLiteral("LCL 线宽"), lower_width_);
    reference_layout->addRow(upper_visible_);
    reference_layout->addRow(QStringLiteral("UCL 颜色"), upper_color_);
    reference_layout->addRow(QStringLiteral("UCL 线型"), upper_style_);
    reference_layout->addRow(QStringLiteral("UCL 线宽"), upper_width_);
    if (reference_tab_enabled_) {
        tabs_->addTab(scrollable_page(reference_page, tabs_), QStringLiteral("参考线"));
    }

    preview_ = new QLabel(this);
    preview_->setObjectName(QStringLiteral("preview"));
    preview_->setMinimumWidth(280);
    preview_->setMinimumHeight(200);
    preview_->setAlignment(Qt::AlignCenter);
    preview_->setStyleSheet(QStringLiteral("background:#ffffff; border:1px solid #d6e1e5;"));
    auto* body = new QHBoxLayout();
    body->addWidget(tabs_, 1);
    body->addWidget(preview_, 1);
    root->addLayout(body, 1);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults,
        this);
    buttons->setObjectName(QStringLiteral("dialog_buttons"));
    auto* apply_button = buttons->addButton(QStringLiteral("应用"), QDialogButtonBox::ApplyRole);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        QString error;
        const ChartModel collected = collect_model();
        if (!validate_model(collected, &error)) {
            QMessageBox::warning(this, QStringLiteral("图形属性无效"), error);
            return;
        }
        model_ = collected;
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
            [this]() { populate_from(original_); });
    connect(apply_button, &QPushButton::clicked, this, [this]() {
        QString error;
        const ChartModel collected = collect_model();
        if (!validate_model(collected, &error)) {
            QMessageBox::warning(this, QStringLiteral("图形属性无效"), error);
            return;
        }
        model_ = collected;
        refresh_preview();
    });
    root->addWidget(buttons);

    const auto connect_preview = [this](QWidget* widget) {
        if (auto* line = qobject_cast<QLineEdit*>(widget)) {
            connect(line, &QLineEdit::textChanged, this, &GraphPropertiesDialog::refresh_preview);
        } else if (auto* box = qobject_cast<QCheckBox*>(widget)) {
            connect(box, &QCheckBox::toggled, this, &GraphPropertiesDialog::refresh_preview);
        } else if (auto* spin = qobject_cast<QDoubleSpinBox*>(widget)) {
            connect(spin, &QDoubleSpinBox::valueChanged, this,
                    &GraphPropertiesDialog::refresh_preview);
        } else if (auto* int_spin = qobject_cast<QSpinBox*>(widget)) {
            connect(int_spin, &QSpinBox::valueChanged, this,
                    &GraphPropertiesDialog::refresh_preview);
        } else if (auto* combo = qobject_cast<QComboBox*>(widget)) {
            connect(combo, &QComboBox::currentIndexChanged, this,
                    &GraphPropertiesDialog::refresh_preview);
        }
    };
    for (QWidget* widget : std::vector<QWidget*>{
             title_, subtitle_, x_axis_title_, y_axis_title_, grid_, legend_,
             line_width_, title_font_size_, axis_font_size_, legend_font_size_,
             theme_preset_, y_min_auto_, y_max_auto_, y_min_, y_max_,
             x_min_auto_, x_max_auto_, x_min_, x_max_,
             center_visible_, lower_visible_,
             upper_visible_, center_style_, lower_style_, upper_style_,
             center_width_, lower_width_, upper_width_}) {
        connect_preview(widget);
    }
    const auto bind_color = [this](QPushButton* button, const QString& title) {
        connect(button, &QPushButton::clicked, this, [button, title, this]() {
            const QColor selected = QColorDialog::getColor(
                QColor(color_from_button(button)), this, title);
            if (selected.isValid()) {
                set_color_button(button, selected.name(QColor::HexArgb));
                refresh_preview();
            }
        });
    };
    bind_color(grid_color_, QStringLiteral("选择网格颜色"));
    bind_color(data_region_fill_, QStringLiteral("选择数据区填色"));
    bind_color(value_color_, QStringLiteral("选择观测值颜色"));
    bind_color(center_color_, QStringLiteral("选择中心线颜色"));
    bind_color(lower_color_, QStringLiteral("选择下控制限颜色"));
    bind_color(upper_color_, QStringLiteral("选择上控制限颜色"));

    connect(y_min_auto_, &QCheckBox::toggled, this, [this](bool checked) {
        y_min_->setEnabled(!checked);
        refresh_preview();
    });
    connect(y_max_auto_, &QCheckBox::toggled, this, [this](bool checked) {
        y_max_->setEnabled(!checked);
        refresh_preview();
    });
    connect(x_min_auto_, &QCheckBox::toggled, this, [this](bool checked) {
        x_min_->setEnabled(!checked);
        refresh_preview();
    });
    connect(x_max_auto_, &QCheckBox::toggled, this, [this](bool checked) {
        x_max_->setEnabled(!checked);
        refresh_preview();
    });
    connect(clear_y_range_, &QPushButton::clicked, this, [this]() {
        y_min_auto_->setChecked(true);
        y_max_auto_->setChecked(true);
        refresh_preview();
    });
    connect(clear_x_range_, &QPushButton::clicked, this, [this]() {
        x_min_auto_->setChecked(true);
        x_max_auto_->setChecked(true);
        refresh_preview();
    });
    populate_from(model_);
}

void GraphPropertiesDialog::populate_from(const ChartModel& model)
{
    model_ = model;
    title_->setText(model.title);
    subtitle_->setText(model.subtitle);
    x_axis_title_->setText(model.x_axis_title);
    y_axis_title_->setText(model.y_axis_title);
    grid_->setChecked(model.show_grid);
    legend_->setChecked(model.show_legend);
    line_width_->setValue(model.line_width);
    title_font_size_->setValue(model.title_font_size);
    axis_font_size_->setValue(model.axis_font_size);
    legend_font_size_->setValue(model.legend_font_size);
    const int theme_index = theme_preset_->findData(model.theme_preset);
    theme_preset_->setCurrentIndex(theme_index >= 0 ? theme_index : 0);
    set_color_button(grid_color_, model.grid_color);
    y_min_auto_->setChecked(!model.y_min.has_value());
    y_max_auto_->setChecked(!model.y_max.has_value());
    y_min_->setValue(model.y_min.value_or(0.0));
    y_max_->setValue(model.y_max.value_or(1.0));
    y_min_->setEnabled(!y_min_auto_->isChecked());
    y_max_->setEnabled(!y_max_auto_->isChecked());
    x_min_auto_->setChecked(!model.x_min.has_value());
    x_max_auto_->setChecked(!model.x_max.has_value());
    x_min_->setValue(model.x_min.value_or(0.0));
    x_max_->setValue(model.x_max.value_or(1.0));
    x_min_->setEnabled(!x_min_auto_->isChecked());
    x_max_->setEnabled(!x_max_auto_->isChecked());
    set_color_button(data_region_fill_,
                     model.data_region_fill.isEmpty()
                         ? QStringLiteral("#00000000") : model.data_region_fill);
    set_color_button(value_color_, model.value_style.color);
    center_visible_->setChecked(model.center_style.visible);
    lower_visible_->setChecked(model.lower_style.visible);
    upper_visible_->setChecked(model.upper_style.visible);
    set_color_button(center_color_, model.center_style.color);
    set_color_button(lower_color_, model.lower_style.color);
    set_color_button(upper_color_, model.upper_style.color);
    center_style_->setCurrentIndex(static_cast<int>(model.center_style.line_style));
    lower_style_->setCurrentIndex(static_cast<int>(model.lower_style.line_style));
    upper_style_->setCurrentIndex(static_cast<int>(model.upper_style.line_style));
    center_width_->setValue(model.center_style.line_width);
    lower_width_->setValue(model.lower_style.line_width);
    upper_width_->setValue(model.upper_style.line_width);

    series_table_->setRowCount(static_cast<int>(model.series.size()));
    for (int row = 0; row < series_table_->rowCount(); ++row) {
        const ChartSeries& series = model.series[static_cast<std::size_t>(row)];
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
                refresh_preview();
            }
        });
        series_table_->setCellWidget(row, 2, color);

        auto* fill = new QPushButton(series_table_);
        set_color_button(fill, series.style.fill_color.isEmpty()
                                  ? series.style.color : series.style.fill_color);
        connect(fill, &QPushButton::clicked, this, [fill, this]() {
            const QColor selected = QColorDialog::getColor(
                QColor(color_from_button(fill)), this, QStringLiteral("选择填充色"));
            if (selected.isValid()) {
                set_color_button(fill, selected.name(QColor::HexArgb));
                refresh_preview();
            }
        });
        series_table_->setCellWidget(row, 3, fill);

        auto* line = make_line_combo(series_table_, series.style.line_style);
        connect(line, &QComboBox::currentIndexChanged, this,
                &GraphPropertiesDialog::refresh_preview);
        series_table_->setCellWidget(row, 4, line);

        auto* point = make_point_combo(series_table_, series.style.point_style);
        connect(point, &QComboBox::currentIndexChanged, this,
                &GraphPropertiesDialog::refresh_preview);
        series_table_->setCellWidget(row, 5, point);

        auto* width = new QDoubleSpinBox(series_table_);
        width->setRange(0.5, 8.0);
        width->setSingleStep(0.5);
        width->setValue(series.style.line_width);
        connect(width, &QDoubleSpinBox::valueChanged, this,
                &GraphPropertiesDialog::refresh_preview);
        series_table_->setCellWidget(row, 6, width);

        auto* point_size = new QDoubleSpinBox(series_table_);
        point_size->setRange(1.0, 16.0);
        point_size->setSingleStep(0.5);
        point_size->setValue(series.style.point_size);
        connect(point_size, &QDoubleSpinBox::valueChanged, this,
                &GraphPropertiesDialog::refresh_preview);
        series_table_->setCellWidget(row, 7, point_size);

        auto* opacity = new QDoubleSpinBox(series_table_);
        opacity->setRange(0.1, 1.0);
        opacity->setSingleStep(0.05);
        opacity->setValue(series.style.opacity);
        connect(opacity, &QDoubleSpinBox::valueChanged, this,
                &GraphPropertiesDialog::refresh_preview);
        series_table_->setCellWidget(row, 8, opacity);
        connect(visible, &QCheckBox::toggled, this, &GraphPropertiesDialog::refresh_preview);
    }
    refresh_preview();
}

ChartModel GraphPropertiesDialog::collect_model() const
{
    ChartModel result = model_;
    result.title = title_->text();
    result.subtitle = subtitle_->text();
    result.x_axis_title = x_axis_title_->text();
    result.y_axis_title = y_axis_title_->text();
    result.show_grid = grid_->isChecked();
    result.show_legend = legend_->isChecked();
    result.line_width = line_width_->value();
    result.title_font_size = title_font_size_->value();
    result.axis_font_size = axis_font_size_->value();
    result.legend_font_size = legend_font_size_->value();
    result.theme_preset = theme_preset_->currentData().toString();
    result.grid_color = color_from_button(grid_color_);
    if (y_min_auto_->isChecked()) {
        result.y_min.reset();
    } else {
        result.y_min = y_min_->value();
    }
    if (y_max_auto_->isChecked()) {
        result.y_max.reset();
    } else {
        result.y_max = y_max_->value();
    }
    if (x_min_auto_->isChecked()) {
        result.x_min.reset();
    } else {
        result.x_min = x_min_->value();
    }
    if (x_max_auto_->isChecked()) {
        result.x_max.reset();
    } else {
        result.x_max = x_max_->value();
    }
    result.data_region_fill = color_from_button(data_region_fill_);
    if (result.data_region_fill.compare(QStringLiteral("#00000000"), Qt::CaseInsensitive) == 0) {
        result.data_region_fill.clear();
    }
    result.value_style.color = color_from_button(value_color_);
    if (reference_tab_enabled_) {
        result.center_style.visible = center_visible_->isChecked();
        result.lower_style.visible = lower_visible_->isChecked();
        result.upper_style.visible = upper_visible_->isChecked();
        result.center_style.color = color_from_button(center_color_);
        result.lower_style.color = color_from_button(lower_color_);
        result.upper_style.color = color_from_button(upper_color_);
        result.center_style.line_style = line_style_from_index(center_style_->currentIndex());
        result.lower_style.line_style = line_style_from_index(lower_style_->currentIndex());
        result.upper_style.line_style = line_style_from_index(upper_style_->currentIndex());
        result.center_style.line_width = center_width_->value();
        result.lower_style.line_width = lower_width_->value();
        result.upper_style.line_width = upper_width_->value();
    }
    const int series_count = qMin(series_table_->rowCount(),
                                  static_cast<int>(result.series.size()));
    for (int row = 0; row < series_count; ++row) {
        ChartSeries& series = result.series[static_cast<std::size_t>(row)];
        auto* visible = qobject_cast<QCheckBox*>(series_table_->cellWidget(row, 0));
        auto* color = qobject_cast<QPushButton*>(series_table_->cellWidget(row, 2));
        auto* fill = qobject_cast<QPushButton*>(series_table_->cellWidget(row, 3));
        auto* line = qobject_cast<QComboBox*>(series_table_->cellWidget(row, 4));
        auto* point = qobject_cast<QComboBox*>(series_table_->cellWidget(row, 5));
        auto* width = qobject_cast<QDoubleSpinBox*>(series_table_->cellWidget(row, 6));
        auto* point_size = qobject_cast<QDoubleSpinBox*>(series_table_->cellWidget(row, 7));
        auto* opacity = qobject_cast<QDoubleSpinBox*>(series_table_->cellWidget(row, 8));
        if (auto* label = series_table_->item(row, 1)) {
            series.label = label->text().trimmed();
        }
        series.style.visible = visible != nullptr && visible->isChecked();
        series.style.color = color_from_button(color);
        series.style.fill_color = color_from_button(fill);
        series.style.line_style = line_style_from_index(line == nullptr ? 0 : line->currentIndex());
        series.style.point_style =
            point_style_from_index(point == nullptr ? 0 : point->currentIndex());
        series.style.line_width = width == nullptr ? 1.8 : width->value();
        series.style.point_size = point_size == nullptr ? 3.5 : point_size->value();
        series.style.opacity = opacity == nullptr ? 1.0 : opacity->value();
        series.show_points = series.style.point_style != ChartPointStyle::None;
    }
    return result;
}

bool GraphPropertiesDialog::validate_model(
    const ChartModel& model, QString* error_message) const
{
    const auto invalid_color = [](const QString& color) {
        return !color.isEmpty() && !QColor(color).isValid();
    };
    if (invalid_color(model.value_style.color) || invalid_color(model.grid_color)
        || invalid_color(model.data_region_fill)
        || invalid_color(model.center_style.color) || invalid_color(model.lower_style.color)
        || invalid_color(model.upper_style.color)) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("存在无效颜色值。");
        }
        return false;
    }
    if (model.line_width <= 0.0 || model.legend_font_size < 6
        || model.title_font_size < 8 || model.axis_font_size < 6) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("线宽或字号超出允许范围。");
        }
        return false;
    }
    if (model.y_min.has_value() && model.y_max.has_value()
        && !(*model.y_min < *model.y_max)) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("Y 最小值必须小于 Y 最大值。");
        }
        return false;
    }
    if (model.x_min.has_value() && model.x_max.has_value()
        && !(*model.x_min < *model.x_max)) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("X 最小值必须小于 X 最大值。");
        }
        return false;
    }
    return true;
}

void GraphPropertiesDialog::refresh_preview()
{
    const ChartModel preview_model = collect_model();
    const int width = preview_->width() > 120 ? preview_->width() : 360;
    const int height = preview_->height() > 120 ? preview_->height() : 400;
    preview_->setPixmap(
        ChartRenderer::render_to_pixmap(preview_model, QSize(width, height)));
}

ChartModel GraphPropertiesDialog::model() const
{
    return collect_model();
}
