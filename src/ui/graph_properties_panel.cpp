#include "ui/graph_properties_panel.h"

#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <cstddef>

namespace {

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

void fill_line_style_combo(QComboBox* combo)
{
    combo->clear();
    for (int index = 0; index < 4; ++index) {
        combo->addItem(line_style_name(line_style_from_index(index)));
    }
}

void fill_point_style_combo(QComboBox* combo)
{
    combo->clear();
    for (int index = 0; index < 5; ++index) {
        combo->addItem(point_style_name(point_style_from_index(index)));
    }
}

}  // namespace

GraphPropertiesPanel::GraphPropertiesPanel(const ChartModel& model, QWidget* parent)
    : QWidget(parent), defaults_(model), model_(model)
{
    setObjectName(QStringLiteral("graph_properties_panel"));
    setMinimumWidth(300);
    setMaximumWidth(460);
    setStyleSheet(QStringLiteral(
        "QGroupBox { font-weight: 600; border: 1px solid #d6e1e5; "
        "border-radius: 6px; margin-top: 8px; padding: 8px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #dcefed; color: #176e73; }"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    auto* heading = new QLabel(QStringLiteral("图形属性"), this);
    heading->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 700;"));
    root->addWidget(heading);

    selection_path_ = new QLabel(QStringLiteral("图形"), this);
    selection_path_->setObjectName(QStringLiteral("selection_path"));
    selection_path_->setWordWrap(true);
    selection_path_->setStyleSheet(QStringLiteral("color:#39777b; padding:4px 0;"));
    root->addWidget(selection_path_);

    visibility_banner_ = new QLabel(this);
    visibility_banner_->setObjectName(QStringLiteral("row_visibility_banner"));
    visibility_banner_->setWordWrap(true);
    visibility_banner_->setStyleSheet(QStringLiteral(
        "background:#eef6fb; color:#1f4e79; border:1px solid #c5d9ea;"
        " border-radius:6px; padding:8px;"));
    root->addWidget(visibility_banner_);
    refresh_visibility_banner();

    auto* object_group = new QGroupBox(QStringLiteral("编辑对象"), this);
    auto* object_layout = new QVBoxLayout(object_group);
    object_list_ = new QListWidget(object_group);
    object_list_->setObjectName(QStringLiteral("graph_object_list"));
    object_layout->addWidget(object_list_);
    root->addWidget(object_group);

    auto* scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("graph_properties_scroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* content = new QWidget(scroll);
    auto* content_layout = new QVBoxLayout(content);
    content_layout->setContentsMargins(0, 0, 0, 0);

    auto* general_group = new QGroupBox(QStringLiteral("标题与坐标轴"), content);
    auto* general_form = new QFormLayout(general_group);
    title_ = new QLineEdit(general_group);
    subtitle_ = new QLineEdit(general_group);
    x_axis_title_ = new QLineEdit(general_group);
    y_axis_title_ = new QLineEdit(general_group);
    general_form->addRow(QStringLiteral("标题"), title_);
    general_form->addRow(QStringLiteral("副标题"), subtitle_);
    general_form->addRow(QStringLiteral("X 轴标题"), x_axis_title_);
    general_form->addRow(QStringLiteral("Y 轴标题"), y_axis_title_);
    content_layout->addWidget(general_group);

    auto* appearance_group = new QGroupBox(QStringLiteral("外观"), content);
    auto* appearance_form = new QFormLayout(appearance_group);
    grid_ = new QCheckBox(QStringLiteral("显示网格"), appearance_group);
    legend_ = new QCheckBox(QStringLiteral("显示图例"), appearance_group);
    line_width_ = new QDoubleSpinBox(appearance_group);
    line_width_->setObjectName(QStringLiteral("default_line_width"));
    line_width_->setRange(0.5, 8.0);
    line_width_->setSingleStep(0.5);
    theme_ = new QComboBox(appearance_group);
    theme_->addItem(QStringLiteral("默认"), QStringLiteral("default"));
    theme_->addItem(QStringLiteral("打印"), QStringLiteral("print"));
    theme_->addItem(QStringLiteral("深色"), QStringLiteral("dark"));
    appearance_form->addRow(grid_);
    appearance_form->addRow(legend_);
    appearance_form->addRow(QStringLiteral("默认线宽"), line_width_);
    appearance_form->addRow(QStringLiteral("主题"), theme_);
    content_layout->addWidget(appearance_group);

    auto* scale_group = new QGroupBox(QStringLiteral("刻度与字号"), content);
    auto* scale_form = new QFormLayout(scale_group);
    x_min_auto_ = new QCheckBox(QStringLiteral("自动 X 最小值"), scale_group);
    x_max_auto_ = new QCheckBox(QStringLiteral("自动 X 最大值"), scale_group);
    y_min_auto_ = new QCheckBox(QStringLiteral("自动 Y 最小值"), scale_group);
    y_max_auto_ = new QCheckBox(QStringLiteral("自动 Y 最大值"), scale_group);
    x_min_auto_->setObjectName(QStringLiteral("x_min_auto"));
    x_max_auto_->setObjectName(QStringLiteral("x_max_auto"));
    y_min_auto_->setObjectName(QStringLiteral("y_min_auto"));
    y_max_auto_->setObjectName(QStringLiteral("y_max_auto"));
    x_min_ = new QDoubleSpinBox(scale_group);
    x_max_ = new QDoubleSpinBox(scale_group);
    y_min_ = new QDoubleSpinBox(scale_group);
    y_max_ = new QDoubleSpinBox(scale_group);
    x_min_->setObjectName(QStringLiteral("x_min"));
    x_max_->setObjectName(QStringLiteral("x_max"));
    y_min_->setObjectName(QStringLiteral("y_min"));
    y_max_->setObjectName(QStringLiteral("y_max"));
    for (auto* spin : {x_min_, x_max_, y_min_, y_max_}) {
        spin->setRange(-1.0e9, 1.0e9);
        spin->setDecimals(4);
    }
    title_font_size_ = new QSpinBox(scale_group);
    axis_font_size_ = new QSpinBox(scale_group);
    legend_font_size_ = new QSpinBox(scale_group);
    title_font_size_->setObjectName(QStringLiteral("title_font_size"));
    axis_font_size_->setObjectName(QStringLiteral("axis_font_size"));
    legend_font_size_->setObjectName(QStringLiteral("legend_font_size"));
    title_font_size_->setRange(8, 32);
    axis_font_size_->setRange(6, 24);
    legend_font_size_->setRange(6, 24);
    scale_form->addRow(x_min_auto_);
    scale_form->addRow(QStringLiteral("X 最小值"), x_min_);
    scale_form->addRow(x_max_auto_);
    scale_form->addRow(QStringLiteral("X 最大值"), x_max_);
    scale_form->addRow(y_min_auto_);
    scale_form->addRow(QStringLiteral("Y 最小值"), y_min_);
    scale_form->addRow(y_max_auto_);
    scale_form->addRow(QStringLiteral("Y 最大值"), y_max_);
    scale_form->addRow(QStringLiteral("标题字号"), title_font_size_);
    scale_form->addRow(QStringLiteral("轴字号"), axis_font_size_);
    scale_form->addRow(QStringLiteral("图例字号"), legend_font_size_);
    content_layout->addWidget(scale_group);

    series_group_ = new QGroupBox(QStringLiteral("选中系列"), content);
    auto* series_form = new QFormLayout(series_group_);
    series_color_ = new QPushButton(series_group_);
    series_color_->setObjectName(QStringLiteral("series_color"));
    set_color_button(series_color_, QStringLiteral("#455a64"));
    series_line_width_ = new QDoubleSpinBox(series_group_);
    series_line_width_->setObjectName(QStringLiteral("series_line_width"));
    series_line_width_->setRange(0.5, 8.0);
    series_line_width_->setSingleStep(0.5);
    series_form->addRow(QStringLiteral("系列颜色"), series_color_);
    series_line_style_ = new QComboBox(series_group_);
    series_line_style_->setObjectName(QStringLiteral("series_line_style"));
    fill_line_style_combo(series_line_style_);
    series_point_style_ = new QComboBox(series_group_);
    series_point_style_->setObjectName(QStringLiteral("series_point_style"));
    fill_point_style_combo(series_point_style_);
    series_form->addRow(QStringLiteral("系列线型"), series_line_style_);
    series_form->addRow(QStringLiteral("系列点型"), series_point_style_);
    series_form->addRow(QStringLiteral("系列线宽"), series_line_width_);
    series_group_->setVisible(false);
    content_layout->addWidget(series_group_);

    if (model.kind == ChartKind::Control) {
        auto* reference_group = new QGroupBox(QStringLiteral("控制线"), content);
        auto* reference_form = new QFormLayout(reference_group);
        center_visible_ = new QCheckBox(QStringLiteral("显示 CL"), reference_group);
        lower_visible_ = new QCheckBox(QStringLiteral("显示 LCL"), reference_group);
        upper_visible_ = new QCheckBox(QStringLiteral("显示 UCL"), reference_group);
        reference_form->addRow(center_visible_);
        reference_form->addRow(lower_visible_);
        reference_form->addRow(upper_visible_);
        content_layout->addWidget(reference_group);
    }
    content_layout->addStretch(1);
    scroll->setWidget(content);
    root->addWidget(scroll, 1);

    auto* buttons = new QHBoxLayout();
    close_button_ = new QPushButton(QStringLiteral("关闭面板"), this);
    auto* restore = new QPushButton(QStringLiteral("恢复默认"), this);
    auto* revert = new QPushButton(QStringLiteral("撤销本次编辑"), this);
    auto* apply = new QPushButton(QStringLiteral("应用"), this);
    close_button_->setObjectName(QStringLiteral("close_panel"));
    restore->setObjectName(QStringLiteral("restore_defaults"));
    apply->setObjectName(QStringLiteral("apply_changes"));
    buttons->addWidget(close_button_);
    buttons->addStretch(1);
    buttons->addWidget(restore);
    buttons->addWidget(revert);
    buttons->addWidget(apply);
    root->addLayout(buttons);

    connect(close_button_, &QPushButton::clicked, this, &GraphPropertiesPanel::close_requested);
    connect(restore, &QPushButton::clicked, this, &GraphPropertiesPanel::restore_defaults);
    connect(revert, &QPushButton::clicked, this, &GraphPropertiesPanel::revert_changes);
    connect(apply, &QPushButton::clicked, this, &GraphPropertiesPanel::apply_changes);
    connect(object_list_, &QListWidget::currentTextChanged, this,
            [this](const QString& text) {
                selection_path_->setText(text);
                load_series_editors();
            });
    connect(series_color_, &QPushButton::clicked, this, [this]() {
        const QColor selected = QColorDialog::getColor(
            QColor(color_from_button(series_color_)), this, QStringLiteral("系列颜色"));
        if (!selected.isValid()) {
            return;
        }
        set_color_button(series_color_, selected.name(QColor::HexArgb));
        schedule_apply();
    });
    connect(series_line_width_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this]() { schedule_apply(); });
    connect(series_line_style_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this]() { schedule_apply(); });
    connect(series_point_style_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this]() { schedule_apply(); });

    set_model(model);
    connect(title_, &QLineEdit::textChanged, this,
            [this]() { schedule_apply(); });
    connect(subtitle_, &QLineEdit::textChanged, this,
            [this]() { schedule_apply(); });
    connect(x_axis_title_, &QLineEdit::textChanged, this,
            [this]() { schedule_apply(); });
    connect(y_axis_title_, &QLineEdit::textChanged, this,
            [this]() { schedule_apply(); });
    connect(grid_, &QCheckBox::toggled, this,
            [this]() { schedule_apply(); });
    connect(legend_, &QCheckBox::toggled, this,
            [this]() { schedule_apply(); });
    connect(line_width_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this]() { schedule_apply(); });
    connect(theme_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this]() { apply_changes(); });
    connect(x_min_auto_, &QCheckBox::toggled, this, [this](bool checked) {
        x_min_->setEnabled(!checked);
        schedule_apply();
    });
    connect(x_max_auto_, &QCheckBox::toggled, this, [this](bool checked) {
        x_max_->setEnabled(!checked);
        schedule_apply();
    });
    connect(y_min_auto_, &QCheckBox::toggled, this, [this](bool checked) {
        y_min_->setEnabled(!checked);
        schedule_apply();
    });
    connect(y_max_auto_, &QCheckBox::toggled, this, [this](bool checked) {
        y_max_->setEnabled(!checked);
        schedule_apply();
    });
    connect(x_min_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this]() { schedule_apply(); });
    connect(x_max_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this]() { schedule_apply(); });
    connect(y_min_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this]() { schedule_apply(); });
    connect(y_max_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this]() { schedule_apply(); });
    connect(title_font_size_, qOverload<int>(&QSpinBox::valueChanged), this,
            [this]() { schedule_apply(); });
    connect(axis_font_size_, qOverload<int>(&QSpinBox::valueChanged), this,
            [this]() { schedule_apply(); });
    connect(legend_font_size_, qOverload<int>(&QSpinBox::valueChanged), this,
            [this]() { schedule_apply(); });
    if (center_visible_ != nullptr) {
        connect(center_visible_, &QCheckBox::toggled, this,
                [this]() { schedule_apply(); });
        connect(lower_visible_, &QCheckBox::toggled, this,
                [this]() { schedule_apply(); });
        connect(upper_visible_, &QCheckBox::toggled, this,
                [this]() { schedule_apply(); });
    apply_debounce_timer_ = new QTimer(this);
    apply_debounce_timer_->setSingleShot(true);
    apply_debounce_timer_->setInterval(180);
    connect(apply_debounce_timer_, &QTimer::timeout, this, [this]() {
        apply_changes();
    });
    }
}

void GraphPropertiesPanel::schedule_apply()
{
    if (apply_debounce_timer_ == nullptr) {
        apply_changes();
        return;
    }
    apply_debounce_timer_->start();
}

ChartModel GraphPropertiesPanel::model() const
{
    return model_;
}

void GraphPropertiesPanel::set_model(const ChartModel& model)
{
    model_ = model;
    original_ = model;
    title_->setText(model.title);
    subtitle_->setText(model.subtitle);
    x_axis_title_->setText(model.x_axis_title);
    y_axis_title_->setText(model.y_axis_title);
    grid_->setChecked(model.show_grid);
    legend_->setChecked(model.show_legend);
    line_width_->setValue(model.line_width);
    const int theme_index = theme_->findData(model.theme_preset);
    theme_->setCurrentIndex(theme_index >= 0 ? theme_index : 0);
    const QSignalBlocker x_min_auto_blocker(x_min_auto_);
    const QSignalBlocker x_max_auto_blocker(x_max_auto_);
    const QSignalBlocker y_min_auto_blocker(y_min_auto_);
    const QSignalBlocker y_max_auto_blocker(y_max_auto_);
    const QSignalBlocker x_min_blocker(x_min_);
    const QSignalBlocker x_max_blocker(x_max_);
    const QSignalBlocker y_min_blocker(y_min_);
    const QSignalBlocker y_max_blocker(y_max_);
    const QSignalBlocker title_font_blocker(title_font_size_);
    const QSignalBlocker axis_font_blocker(axis_font_size_);
    const QSignalBlocker legend_font_blocker(legend_font_size_);
    x_min_auto_->setChecked(!model.x_min.has_value());
    x_max_auto_->setChecked(!model.x_max.has_value());
    y_min_auto_->setChecked(!model.y_min.has_value());
    y_max_auto_->setChecked(!model.y_max.has_value());
    x_min_->setValue(model.x_min.value_or(0.0));
    x_max_->setValue(model.x_max.value_or(1.0));
    y_min_->setValue(model.y_min.value_or(0.0));
    y_max_->setValue(model.y_max.value_or(1.0));
    x_min_->setEnabled(!x_min_auto_->isChecked());
    x_max_->setEnabled(!x_max_auto_->isChecked());
    y_min_->setEnabled(!y_min_auto_->isChecked());
    y_max_->setEnabled(!y_max_auto_->isChecked());
    title_font_size_->setValue(model.title_font_size);
    axis_font_size_->setValue(model.axis_font_size);
    legend_font_size_->setValue(model.legend_font_size);
    if (center_visible_ != nullptr) {
        center_visible_->setChecked(model.center_style.visible);
        lower_visible_->setChecked(model.lower_style.visible);
        upper_visible_->setChecked(model.upper_style.visible);
    }
    rebuild_object_list();
    refresh_reference_visibility();
    load_series_editors();
}

void GraphPropertiesPanel::set_selected_path(const QString& path)
{
    const QString resolved = path.isEmpty() ? QStringLiteral("图形") : path;
    if (object_list_ != nullptr) {
        const QSignalBlocker blocker(object_list_);
        int matched = 0;
        for (int row = 0; row < object_list_->count(); ++row) {
            const QString text = object_list_->item(row)->text();
            const QString series_name = text.section(QStringLiteral(" / "), 1, 1);
            if ((!series_name.isEmpty() && resolved.contains(series_name))
                || resolved.contains(text)
                || (text == QStringLiteral("图形") && resolved == QStringLiteral("图形"))) {
                matched = row;
                if (!series_name.isEmpty() && resolved.contains(series_name)) {
                    break;
                }
            }
        }
        object_list_->setCurrentRow(matched);
    }
    selection_path_->setText(resolved);
    load_series_editors();
}

void GraphPropertiesPanel::set_row_visibility_summary(
    const std::size_t excluded_count,
    const std::size_t hidden_count,
    const std::size_t analysis_n,
    const std::size_t display_n)
{
    excluded_count_ = excluded_count;
    hidden_count_ = hidden_count;
    analysis_n_ = analysis_n;
    display_n_ = display_n;
    refresh_visibility_banner();
}

void GraphPropertiesPanel::refresh_visibility_banner()
{
    if (visibility_banner_ == nullptr) {
        return;
    }
    visibility_banner_->setText(
        QStringLiteral(
            "行可见性契约（只读）\n"
            "排除 %1 行（分析与显示均省略）· 隐藏 %2 行（仅显示省略，分析仍纳入）\n"
            "分析 N = %3 · 显示 N = %4\n"
            "不得将 hidden 与 excluded 合并叙述；改标记请用数据菜单。")
            .arg(static_cast<qulonglong>(excluded_count_))
            .arg(static_cast<qulonglong>(hidden_count_))
            .arg(static_cast<qulonglong>(analysis_n_))
            .arg(static_cast<qulonglong>(display_n_)));
}

void GraphPropertiesPanel::apply_changes()
{
    model_.title = title_->text();
    model_.subtitle = subtitle_->text();
    model_.x_axis_title = x_axis_title_->text();
    model_.y_axis_title = y_axis_title_->text();
    model_.show_grid = grid_->isChecked();
    model_.show_legend = legend_->isChecked();
    model_.line_width = line_width_->value();
    model_.theme_preset = theme_->currentData().toString();
    model_.title_font_size = title_font_size_->value();
    model_.axis_font_size = axis_font_size_->value();
    model_.legend_font_size = legend_font_size_->value();
    if (x_min_auto_->isChecked()) {
        model_.x_min.reset();
    } else {
        model_.x_min = x_min_->value();
    }
    if (x_max_auto_->isChecked()) {
        model_.x_max.reset();
    } else {
        model_.x_max = x_max_->value();
    }
    if (y_min_auto_->isChecked()) {
        model_.y_min.reset();
    } else {
        model_.y_min = y_min_->value();
    }
    if (y_max_auto_->isChecked()) {
        model_.y_max.reset();
    } else {
        model_.y_max = y_max_->value();
    }
    if (model_.x_min.has_value() && model_.x_max.has_value()
        && *model_.x_min >= *model_.x_max) {
        model_.x_max = *model_.x_min + 1.0;
        const QSignalBlocker blocker(x_max_);
        x_max_->setValue(*model_.x_max);
    }
    if (model_.y_min.has_value() && model_.y_max.has_value()
        && *model_.y_min >= *model_.y_max) {
        model_.y_max = *model_.y_min + 1.0;
        const QSignalBlocker blocker(y_max_);
        y_max_->setValue(*model_.y_max);
    }
    if (const auto series_index = selected_series_index();
        series_index.has_value() && *series_index < model_.series.size()) {
        auto& series = model_.series[*series_index];
        series.style.color = color_from_button(series_color_);
        if (!QColor(series.style.color).isValid()) {
            series.style.color = QStringLiteral("#455a64");
            set_color_button(series_color_, series.style.color);
        }
        series.style.line_width = series_line_width_->value();
        series.line_width = series_line_width_->value();
        if (series_line_style_ != nullptr) {
            series.style.line_style = line_style_from_index(series_line_style_->currentIndex());
        }
        if (series_point_style_ != nullptr) {
            series.style.point_style = point_style_from_index(series_point_style_->currentIndex());
            series.show_points = series.style.point_style != ChartPointStyle::None;
        }
    }
    if (center_visible_ != nullptr) {
        model_.center_style.visible = center_visible_->isChecked();
        model_.lower_style.visible = lower_visible_->isChecked();
        model_.upper_style.visible = upper_visible_->isChecked();
    }
    emit model_changed(model_);
}

void GraphPropertiesPanel::restore_defaults()
{
    set_model(defaults_);
    emit model_changed(model_);
}

void GraphPropertiesPanel::revert_changes()
{
    set_model(original_);
    emit model_changed(model_);
}

void GraphPropertiesPanel::rebuild_object_list()
{
    if (object_list_ == nullptr) {
        return;
    }
    const QString previous =
        object_list_->currentItem() != nullptr ? object_list_->currentItem()->text()
                                               : QString();
    object_list_->clear();
    object_list_->addItem(QStringLiteral("图形"));
    object_list_->addItem(QStringLiteral("数据区 / 观测值"));
    for (std::size_t index = 0; index < model_.series.size(); ++index) {
        const QString label = model_.series[index].label.isEmpty()
            ? QStringLiteral("系列 %1").arg(static_cast<qulonglong>(index + 1))
            : model_.series[index].label;
        object_list_->addItem(QStringLiteral("数据系列 / ") + label);
    }
    if (model_.kind == ChartKind::Control) {
        object_list_->addItem(QStringLiteral("控制线 / CL"));
        object_list_->addItem(QStringLiteral("控制线 / LCL"));
        object_list_->addItem(QStringLiteral("控制线 / UCL"));
    }
    int row = 0;
    for (int index = 0; index < object_list_->count(); ++index) {
        if (object_list_->item(index)->text() == previous) {
            row = index;
            break;
        }
    }
    object_list_->setCurrentRow(row);
}

std::optional<std::size_t> GraphPropertiesPanel::selected_series_index() const
{
    if (object_list_ == nullptr || object_list_->currentItem() == nullptr) {
        return std::nullopt;
    }
    const QString text = object_list_->currentItem()->text();
    const QString prefix = QStringLiteral("数据系列 / ");
    if (!text.startsWith(prefix)) {
        return std::nullopt;
    }
    const QString label = text.mid(prefix.size());
    for (std::size_t index = 0; index < model_.series.size(); ++index) {
        const QString series_label = model_.series[index].label.isEmpty()
            ? QStringLiteral("系列 %1").arg(static_cast<qulonglong>(index + 1))
            : model_.series[index].label;
        if (series_label == label) {
            return index;
        }
    }
    return std::nullopt;
}

void GraphPropertiesPanel::load_series_editors()
{
    const auto series_index = selected_series_index();
    const bool series_selected =
        series_index.has_value() && *series_index < model_.series.size();
    if (series_group_ != nullptr) {
        series_group_->setVisible(series_selected);
    }
    if (!series_selected || series_color_ == nullptr || series_line_width_ == nullptr) {
        return;
    }
    const ChartSeries& series = model_.series[*series_index];
    const QSignalBlocker color_blocker(series_color_);
    const QSignalBlocker width_blocker(series_line_width_);
    const QSignalBlocker line_blocker(series_line_style_);
    const QSignalBlocker point_blocker(series_point_style_);
    set_color_button(series_color_, series.style.color);
    series_line_width_->setValue(
        series.style.line_width > 0.0 ? series.style.line_width : series.line_width);
    if (series_line_style_ != nullptr) {
        series_line_style_->setCurrentIndex(static_cast<int>(series.style.line_style));
    }
    if (series_point_style_ != nullptr) {
        series_point_style_->setCurrentIndex(static_cast<int>(series.style.point_style));
    }
}

void GraphPropertiesPanel::refresh_reference_visibility()
{
    if (center_visible_ == nullptr) {
        return;
    }
    center_visible_->setEnabled(!model_.center.empty());
    lower_visible_->setEnabled(!model_.lower.empty());
    upper_visible_->setEnabled(!model_.upper.empty());
}
