#pragma once

#include <QDoubleSpinBox>
#include <QVariant>
#include <optional>

namespace chart_axis_bound_editor {

// Spin boxes format doubles for editing. Preserve the exact original value
// when another property is changed without editing this bound.
inline void load(QDoubleSpinBox* spin, const std::optional<double>& value, double automatic_value)
{
    spin->setValue(value.value_or(automatic_value));
    spin->setProperty("axisLoadedValue", spin->value());
    spin->setProperty("axisExactValue", value.has_value() ? QVariant(*value) : QVariant());
}

inline double read(const QDoubleSpinBox* spin)
{
    const auto exact = spin->property("axisExactValue");
    if (exact.isValid() && spin->value() == spin->property("axisLoadedValue").toDouble()) return exact.toDouble();
    return spin->value();
}

} // namespace chart_axis_bound_editor
