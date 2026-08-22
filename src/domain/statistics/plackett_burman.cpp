#include "domain/statistics/plackett_burman.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <set>

namespace datalab::domain::statistics {
namespace {

// First-row generators for classic PB N=8,12,16,20,24 (length N-1).
std::vector<int> pb_first_row(std::size_t n)
{
    switch (n) {
    case 8:
        return {+1, +1, +1, -1, +1, -1, -1};
    case 12:
        return {+1, +1, -1, +1, +1, +1, -1, -1, -1, +1, -1};
    case 16:
        return {+1, +1, +1, +1, -1, +1, -1, +1, +1, -1, -1, +1, -1, -1, -1};
    case 20:
        return {+1, +1, -1, -1, +1, +1, +1, +1, -1, +1, -1, +1, -1, -1, -1, -1, +1, +1, -1};
    case 24:
        return {+1, +1, +1, +1, +1, -1, +1, -1, +1, +1, -1, -1, +1, -1, -1, -1,
                -1, +1, +1, -1, +1, -1, -1};
    default:
        return {};
    }
}

std::size_t choose_pb_n(std::size_t factor_count)
{
    const std::size_t need = factor_count + 1;
    for (std::size_t n : {8u, 12u, 16u, 20u, 24u}) {
        if (n >= need) {
            return n;
        }
    }
    return 0;
}

}  // namespace

PlackettBurmanDesign generate_plackett_burman(const PlackettBurmanOptions& options)
{
    PlackettBurmanDesign design;
    design.factors = options.factors;
    design.factor_count = options.factors.size();
    if (options.factors.empty() || options.factors.size() > 23) {
        design.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "pb_factor_count",
            "Plackett–Burman 本轮支持 1…23 个因子。"});
        return design;
    }
    std::set<std::string> names;
    for (const DoeFactor& factor : options.factors) {
        if (factor.name.empty() || factor.low_level.empty()
            || factor.high_level.empty()) {
            design.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "pb_incomplete_factor",
                "每个因子需要名称与高低水平。"});
            return design;
        }
        if (!names.insert(factor.name).second) {
            design.diagnostics.push_back({
                DiagnosticMessage::Severity::error, "pb_duplicate_name",
                "因子名称必须唯一。"});
            return design;
        }
    }

    const std::size_t n = choose_pb_n(options.factors.size());
    const auto first = pb_first_row(n);
    if (n == 0 || first.size() != n - 1) {
        design.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "pb_unsupported_n",
            "无法为该因子数选择支持的 PB 运行数。"});
        return design;
    }
    design.run_count = n + options.center_point_count;

    // Cyclic rows of size (N-1)×(N-1), then append all −1 row → N runs × (N-1) columns.
    std::vector<std::vector<int>> matrix(n, std::vector<int>(n - 1, -1));
    for (std::size_t col = 0; col < n - 1; ++col) {
        matrix[0][col] = first[col];
    }
    for (std::size_t row = 1; row < n - 1; ++row) {
        for (std::size_t col = 0; col < n - 1; ++col) {
            matrix[row][col] = matrix[row - 1][(col + n - 2) % (n - 1)];
        }
    }
    // Last row already −1.

    std::vector<std::size_t> order(n);
    std::iota(order.begin(), order.end(), 0);
    if (options.randomize) {
        std::mt19937_64 rng(options.random_seed == 0 ? 1 : options.random_seed);
        std::shuffle(order.begin(), order.end(), rng);
    }

    for (std::size_t run_index = 0; run_index < n; ++run_index) {
        const std::size_t source = order[run_index];
        DoeRun run;
        run.standard_order = source + 1;
        run.run_order = run_index + 1;
        run.block = 1;
        run.center_point = false;
        for (std::size_t f = 0; f < options.factors.size(); ++f) {
            run.coded_levels.push_back(matrix[source][f]);
        }
        design.runs.push_back(std::move(run));
    }
    for (std::size_t c = 0; c < options.center_point_count; ++c) {
        DoeRun run;
        run.standard_order = n + c + 1;
        run.run_order = n + c + 1;
        run.block = 1;
        run.center_point = true;
        run.coded_levels.assign(options.factors.size(), 0);
        design.runs.push_back(std::move(run));
    }
    design.diagnostics.push_back({
        DiagnosticMessage::Severity::info, "pb_scope",
        "Plackett–Burman 筛选设计；分辨率 III；非 CCD；非 Minitab golden。"});
    return design;
}

}  // namespace datalab::domain::statistics
