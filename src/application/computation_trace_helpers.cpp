#include "application/computation_trace_helpers.h"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace datalab::application::trace_helpers {

using datalab::domain::StatisticTable;

std::string table_value_impl(
    const OutputPage& page, const std::vector<std::string>& wanted)
{
    for (const StatisticTable& table : page.tables) {
        for (const auto& row : table.rows) {
            if (row.empty()) {
                continue;
            }
            for (const std::string& label : wanted) {
                if (row.front() == label && row.size() >= 2) {
                    return row[1];
                }
                for (std::size_t i = 0; i + 1 < row.size(); ++i) {
                    if (row[i] == label) {
                        return row[i + 1];
                    }
                }
            }
        }
    }
    return {};
}

std::string fmt_num(double value)
{
    if (!std::isfinite(value)) {
        return {};
    }
    std::ostringstream oss;
    oss << std::setprecision(10) << value;
    return oss.str();
}

std::string opt_fmt(const std::optional<double>& value, const std::string& fallback)
{
    if (value.has_value() && std::isfinite(*value)) {
        return fmt_num(*value);
    }
    return fallback;
}

std::string opt_size_fmt(const std::optional<std::size_t>& value, const std::string& fallback)
{
    if (value.has_value()) {
        return std::to_string(*value);
    }
    return fallback;
}

FormulaBinding bind(
    const std::string& symbol,
    const std::string& label,
    const std::string& value,
    const std::string& role)
{
    FormulaBinding b;
    b.symbol = symbol;
    b.label = label;
    b.value = value;
    b.role = role;
    return b;
}

ComputationStep make_step(
    int order,
    const std::string& description,
    const std::string& expression_before,
    const std::string& expression_after,
    const std::string& value)
{
    ComputationStep step;
    step.order = order;
    step.description = description;
    step.expression_before = expression_before;
    step.expression_after = expression_after;
    step.value = value;
    return step;
}

void push_step(std::vector<ComputationStep>& steps, ComputationStep step)
{
    if (step.order <= 0) {
        step.order = static_cast<int>(steps.size()) + 1;
    }
    steps.push_back(std::move(step));
}

std::string first_nonempty(std::initializer_list<std::string> values)
{
    for (const std::string& value : values) {
        if (!value.empty()) {
            return value;
        }
    }
    return {};
}

void finalize_trace(ComputationTrace& trace)
{
    for (std::size_t i = 0; i < trace.steps.size(); ++i) {
        if (trace.steps[i].order <= 0) {
            trace.steps[i].order = static_cast<int>(i) + 1;
        }
    }
    if (trace.substituted_text.empty() && !trace.plain_formula.empty()) {
        trace.substituted_text = trace.plain_formula;
        if (!trace.result_value.empty()) {
            trace.substituted_text += " = " + trace.result_value;
        }
    }
}

void attach_trace(OutputPage& page, ComputationTrace trace)
{
    finalize_trace(trace);
    page.computation_traces.push_back(std::move(trace));
}

}  // namespace datalab::application::trace_helpers
