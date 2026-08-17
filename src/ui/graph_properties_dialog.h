#pragma once

#include "reporting/chart_model.h"

#include <QDialog>

class QCheckBox;
class QDoubleSpinBox;
class QLineEdit;

class GraphPropertiesDialog final : public QDialog {
    Q_OBJECT

public:
    GraphPropertiesDialog(const ChartModel& model, QWidget* parent = nullptr);
    ChartModel model() const;

private:
    ChartModel model_;
    QLineEdit* title_ = nullptr;
    QLineEdit* subtitle_ = nullptr;
    QCheckBox* grid_ = nullptr;
    QCheckBox* legend_ = nullptr;
    QDoubleSpinBox* line_width_ = nullptr;
};
