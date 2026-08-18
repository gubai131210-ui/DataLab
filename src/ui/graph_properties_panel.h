#pragma once

#include "reporting/chart_model.h"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

class GraphPropertiesPanel final : public QWidget {
    Q_OBJECT

public:
    explicit GraphPropertiesPanel(const ChartModel& model, QWidget* parent = nullptr);

    ChartModel model() const;
    void set_model(const ChartModel& model);
    void set_selected_path(const QString& path);

signals:
    void model_changed(const ChartModel& model);
    void close_requested();

private:
    void apply_changes();
    void restore_defaults();
    void refresh_reference_visibility();

    ChartModel defaults_;
    ChartModel model_;
    ChartModel original_;
    QLabel* selection_path_ = nullptr;
    QListWidget* object_list_ = nullptr;
    QLineEdit* title_ = nullptr;
    QLineEdit* subtitle_ = nullptr;
    QLineEdit* x_axis_title_ = nullptr;
    QLineEdit* y_axis_title_ = nullptr;
    QCheckBox* grid_ = nullptr;
    QCheckBox* legend_ = nullptr;
    QDoubleSpinBox* line_width_ = nullptr;
    QComboBox* theme_ = nullptr;
    QCheckBox* center_visible_ = nullptr;
    QCheckBox* lower_visible_ = nullptr;
    QCheckBox* upper_visible_ = nullptr;
    QPushButton* close_button_ = nullptr;
};
