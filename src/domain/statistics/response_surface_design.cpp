#include "domain/statistics/response_surface_design.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <set>
#include <sstream>

namespace datalab::domain::statistics {
namespace {

bool is_finite_number(double value)
{
    return std::isfinite(value);
}

DiagnosticMessage error_diag(const std::string& code, const std::string& message)
{
    return {DiagnosticMessage::Severity::error, code, message};
}

DiagnosticMessage warning_diag(const std::string& code, const std::string& message)
{
    return {DiagnosticMessage::Severity::warning, code, message};
}

DiagnosticMessage info_diag(const std::string& code, const std::string& message)
{
    return {DiagnosticMessage::Severity::info, code, message};
}

bool validate_factors(
    const std::vector<ResponseSurfaceFactor>& factors,
    std::vector<DiagnosticMessage>& diagnostics)
{
    if (factors.empty()) {
        diagnostics.push_back(error_diag("rsd_no_factors", "至少需要一个连续因素。"));
        return false;
    }
    std::set<std::string> ids;
    for (const ResponseSurfaceFactor& factor : factors) {
        if (factor.id.empty()) {
            diagnostics.push_back(error_diag("rsd_empty_factor_id", "因素 ID 不能为空。"));
            return false;
        }
        if (!ids.insert(factor.id).second) {
            diagnostics.push_back(error_diag("rsd_duplicate_factor_id", "因素 ID 必须唯一。"));
            return false;
        }
        if (factor.type != "continuous") {
            diagnostics.push_back(error_diag(
                "rsd_categorical_blocked",
                "第一阶段只接受连续因素；分类因素不得静默编码。"));
            return false;
        }
        if (!is_finite_number(factor.low) || !is_finite_number(factor.high)) {
            diagnostics.push_back(error_diag("rsd_nonfinite_bounds", "因素低/高水平必须有限。"));
            return false;
        }
        if (!(factor.low < factor.high)) {
            diagnostics.push_back(error_diag(
                "rsd_invalid_bounds", "因素低水平必须严格小于高水平。"));
            return false;
        }
        const double center = factor_center(factor);
        if (!is_finite_number(center) || center < factor.low || center > factor.high) {
            diagnostics.push_back(error_diag(
                "rsd_invalid_center", "中心值必须有限且落在 [low, high] 内。"));
            return false;
        }
    }
    return true;
}

void apply_run_order(
    std::vector<ResponseSurfaceRun>& runs,
    bool randomize,
    std::uint64_t seed)
{
    std::vector<std::size_t> order(runs.size());
    std::iota(order.begin(), order.end(), 0);
    if (randomize && !runs.empty()) {
        std::mt19937_64 rng(seed == 0 ? 1 : seed);
        std::shuffle(order.begin(), order.end(), rng);
    }
    std::vector<ResponseSurfaceRun> reordered;
    reordered.reserve(runs.size());
    for (std::size_t index = 0; index < order.size(); ++index) {
        ResponseSurfaceRun run = runs[order[index]];
        run.run_order = index + 1;
        reordered.push_back(std::move(run));
    }
    runs = std::move(reordered);
}

ResponseSurfaceRun make_run(
    std::size_t standard_order,
    std::size_t block,
    const std::string& point_type,
    const std::vector<double>& coded,
    const std::vector<ResponseSurfaceFactor>& factors)
{
    ResponseSurfaceRun run;
    run.standard_order = standard_order;
    run.block = block;
    run.point_type = point_type;
    run.coded_levels = coded;
    run.actual_levels.reserve(coded.size());
    for (std::size_t index = 0; index < coded.size(); ++index) {
        run.actual_levels.push_back(coded_to_actual(coded[index], factors[index]));
    }
    std::ostringstream id;
    id << "run-" << standard_order << "-" << point_type;
    run.run_id = id.str();
    return run;
}

std::string variant_id(CcdVariant variant)
{
    switch (variant) {
    case CcdVariant::ccc:
        return "ccc";
    case CcdVariant::cci:
        return "cci";
    case CcdVariant::ccf:
        return "ccf";
    }
    return "ccf";
}

}  // namespace

double factor_center(const ResponseSurfaceFactor& factor)
{
    if (factor.center.has_value()) {
        return *factor.center;
    }
    return 0.5 * (factor.low + factor.high);
}

double factor_half_range(const ResponseSurfaceFactor& factor)
{
    return 0.5 * (factor.high - factor.low);
}

double coded_to_actual(double coded, const ResponseSurfaceFactor& factor)
{
    return factor_center(factor) + coded * factor_half_range(factor);
}

double actual_to_coded(double actual, const ResponseSurfaceFactor& factor)
{
    const double half = factor_half_range(factor);
    if (half == 0.0) {
        return 0.0;
    }
    return (actual - factor_center(factor)) / half;
}

double default_ccd_alpha(CcdVariant variant, std::size_t factor_count)
{
    if (variant == CcdVariant::ccf || variant == CcdVariant::cci) {
        return 1.0;
    }
    // Rotatable CCC for full factorial: alpha = 2^(k/4)
    return std::pow(2.0, static_cast<double>(factor_count) / 4.0);
}

ResponseSurfaceDesign generate_ccd(const ResponseSurfaceDesignOptions& options)
{
    ResponseSurfaceDesign design;
    design.design_kind = ResponseSurfaceDesignKind::ccd;
    design.ccd_variant = options.ccd_variant;
    design.design_kind_id = "ccd";
    design.ccd_variant_id = variant_id(options.ccd_variant);
    design.factors = options.factors;
    design.factor_count = options.factors.size();
    design.randomized = options.randomize;
    design.random_seed = options.random_seed;

    if (!validate_factors(options.factors, design.diagnostics)) {
        return design;
    }
    if (options.factors.size() < 2) {
        design.diagnostics.push_back(error_diag(
            "ccd_min_factors", "CCD 至少需要 2 个连续因素。"));
        return design;
    }

    const std::size_t k = options.factors.size();
    const double rotatable_alpha = std::pow(2.0, static_cast<double>(k) / 4.0);
    double alpha = options.alpha_override.has_value()
        ? *options.alpha_override
        : default_ccd_alpha(options.ccd_variant, k);
    if (options.ccd_variant == CcdVariant::cci && !options.alpha_override.has_value()) {
        // CCI reports the rotatable scale used to pull cube points inside the region.
        alpha = rotatable_alpha;
    }
    if (!is_finite_number(alpha) || alpha <= 0.0) {
        design.diagnostics.push_back(error_diag("ccd_invalid_alpha", "alpha 必须为正有限数。"));
        return design;
    }
    design.alpha = alpha;

    // CCC may place star points outside the original factor bounds in actual units.
    if (options.ccd_variant == CcdVariant::ccc && alpha > 1.0 + 1e-12) {
        design.beyond_range_detected = true;
        if (!options.allow_beyond_range) {
            design.diagnostics.push_back(error_diag(
                "ccd_ccc_beyond_range",
                "CCC 星点超出原始因素范围；请允许超范围星点，或改用 CCI/CCF。"));
            return design;
        }
        design.diagnostics.push_back(warning_diag(
            "ccd_ccc_beyond_range_allowed",
            "CCC 星点超出原始 low/high；已按允许超范围策略生成，实验可行性需人工确认。"));
    }

    std::vector<ResponseSurfaceRun> runs;
    const std::size_t cube_n = 1ull << k;
    std::size_t standard = 1;

    // Cube / factorial points.
    for (std::size_t mask = 0; mask < cube_n; ++mask) {
        std::vector<double> coded(k, 0.0);
        for (std::size_t f = 0; f < k; ++f) {
            coded[f] = ((mask >> f) & 1ull) != 0 ? 1.0 : -1.0;
        }
        // CCI: star at factor limits (±1); cube scaled by 1/α_rotatable.
        if (options.ccd_variant == CcdVariant::cci && alpha > 0.0) {
            for (double& value : coded) {
                value /= alpha;
            }
        }
        runs.push_back(make_run(standard++, 1, "cube", coded, options.factors));
    }
    design.cube_count = runs.size();

    // Star / axial points.
    for (std::size_t f = 0; f < k; ++f) {
        for (double sign : {-1.0, 1.0}) {
            std::vector<double> coded(k, 0.0);
            if (options.ccd_variant == CcdVariant::cci) {
                coded[f] = sign;
            } else {
                coded[f] = sign * alpha;
            }
            runs.push_back(make_run(standard++, 1, "star", coded, options.factors));
        }
    }
    design.star_count = 2 * k;

    for (std::size_t c = 0; c < options.center_point_count; ++c) {
        std::vector<double> coded(k, 0.0);
        runs.push_back(make_run(standard++, 1, "center", coded, options.factors));
    }
    design.center_count = options.center_point_count;

    apply_run_order(runs, options.randomize, options.random_seed);
    design.runs = std::move(runs);
    design.run_count = design.runs.size();
    design.ok = true;
    design.diagnostics.push_back(info_diag(
        "ccd_formula_reference",
        "CCD 点集按 NIST Response Surface / CCD 定义生成；证据类型 formula_reference，"
        "非 vendor_oracle，未冻结为商业软件对齐 golden。"));
    return design;
}

ResponseSurfaceDesign generate_bbd(const ResponseSurfaceDesignOptions& options)
{
    ResponseSurfaceDesign design;
    design.design_kind = ResponseSurfaceDesignKind::bbd;
    design.design_kind_id = "bbd";
    design.ccd_variant_id.clear();
    design.factors = options.factors;
    design.factor_count = options.factors.size();
    design.randomized = options.randomize;
    design.random_seed = options.random_seed;
    design.alpha = 1.0;

    if (!validate_factors(options.factors, design.diagnostics)) {
        return design;
    }
    if (options.factors.size() < 3 || options.factors.size() > 7) {
        design.diagnostics.push_back(error_diag(
            "bbd_factor_count",
            "BBD 第一阶段支持 3–7 个连续因素；2 因素不接受。"));
        return design;
    }

    const std::size_t k = options.factors.size();
    std::vector<ResponseSurfaceRun> runs;
    std::size_t standard = 1;

    for (std::size_t i = 0; i < k; ++i) {
        for (std::size_t j = i + 1; j < k; ++j) {
            for (double si : {-1.0, 1.0}) {
                for (double sj : {-1.0, 1.0}) {
                    std::vector<double> coded(k, 0.0);
                    coded[i] = si;
                    coded[j] = sj;
                    runs.push_back(make_run(standard++, 1, "edge", coded, options.factors));
                }
            }
        }
    }
    design.edge_count = runs.size();

    // Explicitly note absence of full-factorial corners.
    design.diagnostics.push_back(info_diag(
        "bbd_no_corners",
        "BBD 不包含所有因素同时处于极端水平的角点；这是设计空间边界，不是实现缺陷。"));

    for (std::size_t c = 0; c < options.center_point_count; ++c) {
        std::vector<double> coded(k, 0.0);
        runs.push_back(make_run(standard++, 1, "center", coded, options.factors));
    }
    design.center_count = options.center_point_count;

    apply_run_order(runs, options.randomize, options.random_seed);
    design.runs = std::move(runs);
    design.run_count = design.runs.size();
    design.ok = true;
    design.diagnostics.push_back(info_diag(
        "bbd_formula_reference",
        "BBD 点集按 NIST Box–Behnken 定义生成；证据类型 formula_reference，"
        "非 vendor_oracle。"));
    return design;
}

ResponseSurfaceDesign generate_response_surface_design(
    const ResponseSurfaceDesignOptions& options)
{
    if (options.design_kind == ResponseSurfaceDesignKind::bbd) {
        return generate_bbd(options);
    }
    return generate_ccd(options);
}

}  // namespace datalab::domain::statistics
