#include "ui/graph_properties_panel.h"

#include <QCheckBox>
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
#include <QVBoxLayout>
#include <QWidget>

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
    selection_path_->setWordWrap(true);
    selection_path_->setStyleSheet(QStringLiteral("color:#39777b; padding:4px 0;"));
    root->addWidget(selection_path_);

    auto* object_group = new QGroupBox(QStringLiteral("编辑对象"), this);
    auto* object_layout = new QVBoxLayout(object_group);
    object_list_ = new QListWidget(object_group);
    object_list_->setObjectName(QStringLiteral("graph_object_list"));
    object_list_->addItem(QStringLiteral("图形"));
    object_list_->addItem(QStringLiteral("数据区 / 观测值"));
    if (model.kind == ChartKind::Control) {
        object_list_->addItem(QStringLiteral("控制线 / CL"));
        object_list_->addItem(QStringLiteral("控制线 / LCL"));
        object_list_->addItem(QStringLiteral("控制线 / UCL"));
    }
    object_list_->setCurrentRow(0);
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
    auto* apply = new QPushButton(QStringLiteral("应用"), this);
    close_button_->setObjectName(QStringLiteral("close_panel"));
    restore->setObjectName(QStringLiteral("restore_defaults"));
    apply->setObjectName(QStringLiteral("apply_changes"));
    buttons->addWidget(close_button_);
    buttons->addStretch(1);
    buttons->addWidget(restore);
    buttons->addWidget(apply);
    root->addLayout(buttons);

    connect(close_button_, &QPushButton::clicked, this, &GraphPropertiesPanel::close_requested);
    connect(restore, &QPushButton::clicked, this, &GraphPropertiesPanel::restore_defaults);
    connect(apply, &QPushButton::clicked, this, &GraphPropertiesPanel::apply_changes);
    connect(object_list_, &QListWidget::currentTextChanged, this,
            [this](const QString& text) { selection_path_->setText(text); });

    set_model(model);
    connect(title_, &QLineEdit::textChanged, this,
            [this]() { apply_changes(); });
    connect(subtitle_, &QLineEdit::textChanged, this,
            [this]() { apply_changes(); });
    connect(x_axis_title_, &QLineEdit::textChanged, this,
            [this]() { apply_changes(); });
    connect(y_axis_title_, &QLineEdit::textChanged, this,
            [this]() { apply_changes(); });
    connect(grid_, &QCheckBox::toggled, this,
            [this]() { apply_changes(); });
    connect(legend_, &QCheckBox::toggled, this,
            [this]() { apply_changes(); });
    connect(line_width_, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
            [this]() { apply_changes(); });
    connect(theme_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this]() { apply_changes(); });
    if (center_visible_ != nullptr) {
        connect(center_visible_, &QCheckBox::toggled, this,
                [this]() { apply_changes(); });
        connect(lower_visible_, &QCheckBox::toggled, this,
                [this]() { apply_changes(); });
        connect(upper_visible_, &QCheckBox::toggled, this,
                [this]() { apply_changes(); });
    }
}

ChartModel GraphPropertiesPanel::model() const
{
    return model_;
}

void GraphPropertiesPanel::set_model(const ChartModel& model)
{
    model_ = model;
    title_->setText(model.title);
    subtitle_->setText(model.subtitle);
    x_axis_title_->setText(model.x_axis_title);
    y_axis_title_->setText(model.y_axis_title);
    grid_->setChecked(model.show_grid);
    legend_->setChecked(model.show_legend);
    line_width_->setValue(model.line_width);
    const int theme_index = theme_->findData(model.theme_preset);
    theme_->setCurrentIndex(theme_index >= 0 ? theme_index : 0);
    if (center_visible_ != nullptr) {
        center_visible_->setChecked(model.center_style.visible);
        lower_visible_->setChecked(model.lower_style.visible);
        upper_visible_->setChecked(model.upper_style.visible);
    }
    refresh_reference_visibility();
}

void GraphPropertiesPanel::set_selected_path(const QString& path)
{
    selection_path_->setText(path.isEmpty() ? QStringLiteral("图形") : path);
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
    if (center_visible_ != nullptr) {
        model_.center_style.visible = center_visible_->isChecked();
        model_.lower_style.visible = lower_visible_->isChecked();
        model_.upper_style.visible = upper_visible_->isChecked();
    }
    original_ = model_;
    emit model_changed(model_);
}

void GraphPropertiesPanel::restore_defaults()
{
    set_model(defaults_);
    emit model_changed(model_);
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
