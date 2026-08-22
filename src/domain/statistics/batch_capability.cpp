#include "domain/statistics/batch_capability.h"

#include <cmath>
#include <limits>
#include <map>
#include <numeric>

namespace datalab::domain::statistics {
namespace {

double sample_standard_deviation(const std::vector<double>& values, double mean)
{
    if (values.size() < 2) {
        return 0.0;
    }
    double sum = 0.0;
    for (double value : values) {
        const double delta = value - mean;
        sum += delta * delta;
    }
    return std::sqrt(sum / static_cast<double>(values.size() - 1));
}

}  // namespace

BatchCapabilityResult compute_batch_capability(
    const std::vector<double>& values,
    const std::vector<std::string>& batch_labels,
    const std::vector<std::size_t>& source_rows,
    const SpecificationLimits& specifications,
    std::size_t min_batch_size)
{
    BatchCapabilityResult result;
    if (values.size() != batch_labels.size() || values.empty()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "batch_capability_shape",
            "批次能力需要等长的测量值与批次标签。"});
        return result;
    }
    if (!specifications.lower.has_value() && !specifications.upper.has_value()) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "batch_capability_specs",
            "请输入 LSL 或 USL。"});
        return result;
    }
    if (min_batch_size < 2) {
        min_batch_size = 2;
    }

    struct Accumulator {
        std::vector<double> values;
        std::vector<std::size_t> source_rows;
    };
    std::map<std::string, Accumulator> grouped;
    for (std::size_t index = 0; index < values.size(); ++index) {
        grouped[batch_labels[index]].values.push_back(values[index]);
        if (index < source_rows.size()) {
            grouped[batch_labels[index]].source_rows.push_back(source_rows[index]);
        }
    }

    for (const auto& [batch_id, accumulator] : grouped) {
        if (accumulator.values.size() < min_batch_size) {
            ++result.skipped_batch_count;
            result.diagnostics.push_back({
                DiagnosticMessage::Severity::warning, "batch_too_small",
                "批次 " + batch_id + " 观测不足，已跳过。"});
            continue;
        }
        const double mean = std::accumulate(accumulator.values.begin(), accumulator.values.end(), 0.0)
            / static_cast<double>(accumulator.values.size());
        const double sigma = sample_standard_deviation(accumulator.values, mean);
        const ProcessCapabilityResult capability =
            ProcessCapability::calculate(accumulator.values, sigma, specifications);
        BatchCapabilityRow row;
        row.batch_id = batch_id;
        row.sample_size = accumulator.values.size();
        row.mean = mean;
        row.within_standard_deviation = sigma;
        row.cp = capability.cp;
        row.cpk = capability.cpk;
        row.pp = capability.pp;
        row.ppk = capability.ppk;
        row.source_rows = accumulator.source_rows;
        result.batches.push_back(std::move(row));
        result.total_observations += accumulator.values.size();
    }

    result.batch_count = result.batches.size();
    if (result.batch_count == 0) {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "batch_capability_empty",
            "没有满足最小样本量的批次。"});
    } else {
        result.diagnostics.push_back({
            DiagnosticMessage::Severity::info, "batch_capability_scope",
            "逐批正态能力（样本 σ）；非 Between/Within；formula_reference。"});
    }
    return result;
}

}  // namespace datalab::domain::statistics
