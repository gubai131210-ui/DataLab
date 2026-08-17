#pragma once

#include <cstddef>
#include <optional>

namespace datalab::domain::statistics {

class SpcConstants final {
public:
    static std::optional<double> d2(std::size_t subgroup_size);
    static std::optional<double> d3(std::size_t subgroup_size);
    static std::optional<double> a2(std::size_t subgroup_size);
    static std::optional<double> d3_limit(std::size_t subgroup_size);
    static std::optional<double> d4(std::size_t subgroup_size);
    static std::optional<double> median_moving_range_constant(std::size_t length);
};

}  // namespace datalab::domain::statistics
