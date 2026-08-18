#pragma once

#include "reporting/chart_model.h"

#include <QDialog>

#include <vector>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTabWidget;

class GraphPropertiesDialog final : public QDialog {
    Q_OBJECT

public:
    GraphPropertiesDialog(const ChartModel& model, QWidget* parent = nullptr);
    ChartModel model() const;

private:
    void populate_from(const ChartModel& model);
    void refresh_preview();
    ChartModel collect_model() const;
    bool validate_model(const ChartModel& model, QString* error_message) const;

    ChartModel original_;
    ChartModel model_;
    QLineEdit* title_ = nullptr;
    QLineEdit* subtitle_ = nullptr;
    QLineEdit* x_axis_title_ = nullptr;
    QLineEdit* y_axis_title_ = nullptr;
    QCheckBox* grid_ = nullptr;
    QCheckBox* legend_ = nullptr;
    QCheckBox* y_min_auto_ = nullptr;
    QCheckBox* y_max_auto_ = nullptr;
    QCheckBox* x_min_auto_ = nullptr;
    QCheckBox* x_max_auto_ = nullptr;
    QPushButton* clear_y_range_ = nullptr;
    QPushButton* clear_x_range_ = nullptr;
    QDoubleSpinBox* line_width_ = nullptr;
    QDoubleSpinBox* y_min_ = nullptr;
    QDoubleSpinBox* y_max_ = nullptr;
    QDoubleSpinBox* x_min_ = nullptr;
    QDoubleSpinBox* x_max_ = nullptr;
    QSpinBox* legend_font_size_ = nullptr;
    QSpinBox* title_font_size_ = nullptr;
    QSpinBox* axis_font_size_ = nullptr;
    QComboBox* theme_preset_ = nullptr;
    QPushButton* grid_color_ = nullptr;
    QPushButton* data_region_fill_ = nullptr;
    QTableWidget* series_table_ = nullptr;
    QPushButton* value_color_ = nullptr;
    QCheckBox* center_visible_ = nullptr;
    QCheckBox* lower_visible_ = nullptr;
    QCheckBox* upper_visible_ = nullptr;
    QPushButton* center_color_ = nullptr;
    QPushButton* lower_color_ = nullptr;
    QPushButton* upper_color_ = nullptr;
    QComboBox* center_style_ = nullptr;
    QComboBox* lower_style_ = nullptr;
    QComboBox* upper_style_ = nullptr;
    QDoubleSpinBox* center_width_ = nullptr;
    QDoubleSpinBox* lower_width_ = nullptr;
    QDoubleSpinBox* upper_width_ = nullptr;
    QTabWidget* tabs_ = nullptr;
    bool reference_tab_enabled_ = false;
    QLabel* preview_ = nullptr;
};
