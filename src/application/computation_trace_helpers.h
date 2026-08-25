#pragma once

#include "domain/quality_types.h"

#include <initializer_list>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace datalab::application::trace_helpers {

using datalab::domain::ComputationStep;
using datalab::domain::ComputationTrace;
using datalab::domain::FormulaBinding;
using datalab::domain::OutputPage;

std::string fmt_num(double value);
std::string opt_fmt(const std::optional<double>& value, const std::string& fallback = {});
std::string opt_size_fmt(const std::optional<std::size_t>& value, const std::string& fallback = {});

FormulaBinding bind(
    const std::string& symbol,
    const std::string& label,
    const std::string& value,
    const std::string& role);

ComputationStep make_step(
    int order,
    const std::string& description,
    const std::string& expression_before,
    const std::string& expression_after,
    const std::string& value);

void push_step(std::vector<ComputationStep>& steps, ComputationStep step);

std::string table_value_impl(const OutputPage& page, const std::vector<std::string>& wanted);

template<typename... Args>
std::string table_value(const OutputPage& page, Args&&... labels)
{
    return table_value_impl(page, {std::string(std::forward<Args>(labels))...});
}

std::string first_nonempty(std::initializer_list<std::string> values);

// Prefer Facts / table; empty string when unavailable (avoid 见结果表 on L3 primary path).
template<typename... Args>
std::string require_value(Args&&... values)
{
    return first_nonempty({std::string(std::forward<Args>(values))...});
}

void finalize_trace(ComputationTrace& trace);

void attach_trace(OutputPage& page, ComputationTrace trace);

}  // namespace datalab::application::trace_helpers
