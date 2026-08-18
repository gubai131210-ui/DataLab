#pragma once

#include "reporting/chart_model.h"

#include <QDialog>

#include <vector>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

class GraphPropertiesDialog final : public QDialog {
    Q_OBJECT

public:
    GraphPropertiesDialog(const ChartModel& model, QWidget* parent = nullptr);
    ChartModel model() const;

private:
    ChartModel model_;
    QLineEdit* title_ = nullptr;
    QLineEdit* subtitle_ = nullptr;
    QLineEdit* x_axis_title_ = nullptr;
    QLineEdit* y_axis_title_ = nullptr;
    QCheckBox* grid_ = nullptr;
    QCheckBox* legend_ = nullptr;
    QDoubleSpinBox* line_width_ = nullptr;
    QSpinBox* legend_font_size_ = nullptr;
    QTableWidget* series_table_ = nullptr;
    QPushButton* value_color_ = nullptr;
    QPushButton* center_color_ = nullptr;
    QPushButton* lower_color_ = nullptr;
    QPushButton* upper_color_ = nullptr;
};
