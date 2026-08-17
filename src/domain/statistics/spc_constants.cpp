#include "domain/statistics/spc_constants.h"

#include <cmath>

namespace datalab::domain::statistics {
namespace {

constexpr int kMinSize = 2;
constexpr int kMaxSize = 25;

// ASTM/AIAG unbiasing constants d2, d3 for n = 2..25.
constexpr double kD2[26] = {
    0.0, 0.0,
    1.128, 1.693, 2.059, 2.326, 2.534, 2.704, 2.847, 2.970, 3.078,
    3.173, 3.258, 3.336, 3.407, 3.472, 3.532, 3.588, 3.640, 3.689,
    3.735, 3.778, 3.819, 3.858, 3.895, 3.931
};

constexpr double kD3[26] = {
    0.0, 0.0,
    0.853, 0.888, 0.880, 0.864, 0.848, 0.833, 0.820, 0.808, 0.797,
    0.787, 0.778, 0.770, 0.763, 0.756, 0.750, 0.744, 0.739, 0.734,
    0.729, 0.724, 0.720, 0.716, 0.712, 0.708
};

bool valid_size(std::size_t subgroup_size)
{
    return subgroup_size >= static_cast<std::size_t>(kMinSize)
        && subgroup_size <= static_cast<std::size_t>(kMaxSize);
}

}  // namespace

std::optional<double> SpcConstants::d2(std::size_t subgroup_size)
{
    if (!valid_size(subgroup_size)) {
        return std::nullopt;
    }
    return kD2[subgroup_size];
}

std::optional<double> SpcConstants::d3(std::size_t subgroup_size)
{
    if (!valid_size(subgroup_size)) {
        return std::nullopt;
    }
    return kD3[subgroup_size];
}

std::optional<double> SpcConstants::a2(std::size_t subgroup_size)
{
    const std::optional<double> d2_value = d2(subgroup_size);
    if (!d2_value.has_value()) {
        return std::nullopt;
    }
    return 3.0 / (*d2_value * std::sqrt(static_cast<double>(subgroup_size)));
}

std::optional<double> SpcConstants::d3_limit(std::size_t subgroup_size)
{
    const std::optional<double> d2_value = d2(subgroup_size);
    const std::optional<double> d3_value = d3(subgroup_size);
    if (!d2_value.has_value() || !d3_value.has_value()) {
        return std::nullopt;
    }
    return std::max(0.0, 1.0 - 3.0 * (*d3_value) / (*d2_value));
}

std::optional<double> SpcConstants::d4(std::size_t subgroup_size)
{
    const std::optional<double> d2_value = d2(subgroup_size);
    const std::optional<double> d3_value = d3(subgroup_size);
    if (!d2_value.has_value() || !d3_value.has_value()) {
        return std::nullopt;
    }
    return 1.0 + 3.0 * (*d3_value) / (*d2_value);
}

std::optional<double> SpcConstants::median_moving_range_constant(std::size_t length)
{
    if (length == 2) {
        return 0.954;
    }
    return d2(length);
}

}  // namespace datalab::domain::statistics
