#include "domain/statistics/taguchi_orthogonal.h"

#include <algorithm>
#include <cctype>
#include <random>

namespace datalab::domain::statistics {
namespace {

// Classic Taguchi orthogonal arrays (columns = factors, rows = runs).
// Values: 0/1 for 2-level; 0/1/2 for 3-level. Mapped to coded -1/+1 or -1/0/+1.
const int kL8[8][7] = {
    {0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 1, 1, 1},
    {0, 1, 1, 0, 0, 1, 1},
    {0, 1, 1, 1, 1, 0, 0},
    {1, 0, 1, 0, 1, 0, 1},
    {1, 0, 1, 1, 0, 1, 0},
    {1, 1, 0, 0, 1, 1, 0},
    {1, 1, 0, 1, 0, 0, 1},
};

const int kL9[9][4] = {
    {0, 0, 0, 0},
    {0, 1, 1, 1},
    {0, 2, 2, 2},
    {1, 0, 1, 2},
    {1, 1, 2, 0},
    {1, 2, 0, 1},
    {2, 0, 2, 1},
    {2, 1, 0, 2},
    {2, 2, 1, 0},
};

const int kL12[12][11] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
    {0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1},
    {0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1},
    {0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0},
    {1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0},
    {1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1},
    {1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0},
    {1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1},
    {1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0},
    {1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1},
};

int map_two_level(int raw)
{
    return raw == 0 ? -1 : 1;
}

int map_three_level(int raw)
{
    if (raw <= 0) {
        return -1;
    }
    if (raw == 1) {
        return 0;
    }
    return 1;
}

std::string ascii_lower(std::string text)
{
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text;
}

}  // namespace

TaguchiArray parse_taguchi_array(const std::string& text)
{
    const std::string lower = ascii_lower(text);
    if (lower == "l9" || lower == "9") {
        return TaguchiArray::L9;
    }
    if (lower == "l12" || lower == "12") {
        return TaguchiArray::L12;
    }
    return TaguchiArray::L8;
}

std::string taguchi_array_name(TaguchiArray array)
{
    switch (array) {
    case TaguchiArray::L9:
        return "L9";
    case TaguchiArray::L12:
        return "L12";
    case TaguchiArray::L8:
    default:
        return "L8";
    }
}

std::size_t taguchi_max_factors(TaguchiArray array)
{
    switch (array) {
    case TaguchiArray::L9:
        return 4;
    case TaguchiArray::L12:
        return 11;
    case TaguchiArray::L8:
    default:
        return 7;
    }
}

std::size_t taguchi_levels(TaguchiArray array)
{
    return array == TaguchiArray::L9 ? 3 : 2;
}

TaguchiOrthogonalDesign generate_taguchi_orthogonal(
    const TaguchiOrthogonalOptions& options)
{
    TaguchiOrthogonalDesign design;
    design.array = options.array;
    design.array_name = taguchi_array_name(options.array);
    design.levels_per_factor = taguchi_levels(options.array);
    design.max_factors = taguchi_max_factors(options.array);
    design.design_kind = "taguchi_orthogonal";

    if (options.factors.empty()) {
        design.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "taguchi_no_factors",
            "Taguchi 正交设计至少需要一个因子。"});
        return design;
    }

    std::vector<DoeFactor> factors = options.factors;
    if (factors.size() > design.max_factors) {
        design.diagnostics.push_back({
            DiagnosticMessage::Severity::warning, "taguchi_factor_cap",
            "因子数超过 " + design.array_name + " 上限 "
                + std::to_string(design.max_factors) + "，已截断。"});
        factors.resize(design.max_factors);
    }
    design.factors = factors;
    design.factor_count = factors.size();

    const std::size_t run_count =
        options.array == TaguchiArray::L9
            ? 9u
            : (options.array == TaguchiArray::L12 ? 12u : 8u);
    design.run_count = run_count;

    std::vector<std::size_t> order(run_count);
    for (std::size_t i = 0; i < run_count; ++i) {
        order[i] = i;
    }
    if (options.randomize) {
        std::mt19937_64 rng(options.random_seed == 0 ? 1 : options.random_seed);
        std::shuffle(order.begin(), order.end(), rng);
    }

    for (std::size_t run_order = 0; run_order < run_count; ++run_order) {
        const std::size_t std_index = order[run_order];
        DoeRun run;
        run.standard_order = std_index + 1;
        run.run_order = run_order + 1;
        run.block = 1;
        run.center_point = false;
        run.coded_levels.resize(factors.size());
        for (std::size_t f = 0; f < factors.size(); ++f) {
            int raw = 0;
            if (options.array == TaguchiArray::L8) {
                raw = kL8[std_index][f];
                run.coded_levels[f] = map_two_level(raw);
            } else if (options.array == TaguchiArray::L9) {
                raw = kL9[std_index][f];
                run.coded_levels[f] = map_three_level(raw);
            } else {
                raw = kL12[std_index][f];
                run.coded_levels[f] = map_two_level(raw);
            }
        }
        design.runs.push_back(std::move(run));
    }
    return design;
}

DoeFactorialDesign taguchi_to_factorial_design(const TaguchiOrthogonalDesign& design)
{
    DoeFactorialDesign out;
    out.factors = design.factors;
    out.runs = design.runs;
    out.diagnostics = design.diagnostics;
    out.design_kind = design.design_kind;
    out.fraction_p = 0;
    out.resolution = 0;
    return out;
}

}  // namespace datalab::domain::statistics
