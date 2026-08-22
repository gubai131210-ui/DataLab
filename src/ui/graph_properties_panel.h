#pragma once

#include "reporting/chart_model.h"

#include <QWidget>

#include <cstddef>
#include <optional>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QTimer;

class GraphPropertiesPanel final : public QWidget {
    Q_OBJECT

public:
    explicit GraphPropertiesPanel(const ChartModel& model, QWidget* parent = nullptr);

    ChartModel model() const;
    void set_model(const ChartModel& model);
    void set_selected_path(const QString& path);
    // Presentation-only: excluded vs hidden counts (never merges into one flag).
    void set_row_visibility_summary(
        std::size_t excluded_count,
        std::size_t hidden_count,
        std::size_t analysis_n = 0,
        std::size_t display_n = 0);

signals:
    void model_changed(const ChartModel& model);
    void close_requested();

private:
    void apply_changes();
    void schedule_apply();
    void restore_defaults();
    void revert_changes();
    void refresh_reference_visibility();
    void rebuild_object_list();
    void load_series_editors();
    void refresh_visibility_banner();
    std::optional<std::size_t> selected_series_index() const;

    ChartModel defaults_;
    ChartModel model_;
    ChartModel original_;
    QLabel* selection_path_ = nullptr;
    QLabel* visibility_banner_ = nullptr;
    std::size_t excluded_count_ = 0;
    std::size_t hidden_count_ = 0;
    std::size_t analysis_n_ = 0;
    std::size_t display_n_ = 0;
    QListWidget* object_list_ = nullptr;
    QLineEdit* title_ = nullptr;
    QLineEdit* subtitle_ = nullptr;
    QLineEdit* x_axis_title_ = nullptr;
    QLineEdit* y_axis_title_ = nullptr;
    QCheckBox* grid_ = nullptr;
    QCheckBox* legend_ = nullptr;
    QDoubleSpinBox* line_width_ = nullptr;
    QComboBox* theme_ = nullptr;
    QCheckBox* x_min_auto_ = nullptr;
    QCheckBox* x_max_auto_ = nullptr;
    QCheckBox* y_min_auto_ = nullptr;
    QCheckBox* y_max_auto_ = nullptr;
    QDoubleSpinBox* x_min_ = nullptr;
    QDoubleSpinBox* x_max_ = nullptr;
    QDoubleSpinBox* y_min_ = nullptr;
    QDoubleSpinBox* y_max_ = nullptr;
    QSpinBox* title_font_size_ = nullptr;
    QSpinBox* axis_font_size_ = nullptr;
    QSpinBox* legend_font_size_ = nullptr;
    QCheckBox* center_visible_ = nullptr;
    QCheckBox* lower_visible_ = nullptr;
    QCheckBox* upper_visible_ = nullptr;
    QGroupBox* series_group_ = nullptr;
    QPushButton* series_color_ = nullptr;
    QDoubleSpinBox* series_line_width_ = nullptr;
    QComboBox* series_line_style_ = nullptr;
    QComboBox* series_point_style_ = nullptr;
    QPushButton* close_button_ = nullptr;
    QTimer* apply_debounce_timer_ = nullptr;
};
