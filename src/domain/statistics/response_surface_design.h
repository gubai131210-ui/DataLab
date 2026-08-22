#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

enum class CcdVariant {
    ccc,
    cci,
    ccf
};

enum class ResponseSurfaceDesignKind {
    ccd,
    bbd
};

struct ResponseSurfaceFactor {
    std::string id;
    std::string name;
    std::string unit;
    std::string type = "continuous";  // Phase 4: continuous only
    double low = 0.0;
    double high = 0.0;
    std::optional<double> center;
};

struct ResponseSurfaceDesignOptions {
    ResponseSurfaceDesignKind design_kind = ResponseSurfaceDesignKind::ccd;
    CcdVariant ccd_variant = CcdVariant::ccf;
    std::vector<ResponseSurfaceFactor> factors;
    std::size_t center_point_count = 1;
    std::size_t block_count = 1;
    bool randomize = true;
    std::uint64_t random_seed = 1;
    // If false, CCC star points beyond original low/high block with error.
    bool allow_beyond_range = false;
    // Optional alpha override; empty/NaN → variant default.
    std::optional<double> alpha_override;
};

struct ResponseSurfaceRun {
    std::size_t standard_order = 0;
    std::size_t run_order = 0;
    std::size_t block = 1;
    std::string run_id;
    std::string point_type;  // cube | star | center | edge
    std::vector<double> coded_levels;
    std::vector<double> actual_levels;
};

struct ResponseSurfaceDesign {
    ResponseSurfaceDesignKind design_kind = ResponseSurfaceDesignKind::ccd;
    CcdVariant ccd_variant = CcdVariant::ccf;
    std::string design_kind_id = "ccd";
    std::string ccd_variant_id = "ccf";
    std::size_t factor_count = 0;
    std::size_t run_count = 0;
    std::size_t cube_count = 0;
    std::size_t star_count = 0;
    std::size_t edge_count = 0;
    std::size_t center_count = 0;
    double alpha = 1.0;
    bool beyond_range_detected = false;
    bool randomized = false;
    std::uint64_t random_seed = 0;
    std::vector<ResponseSurfaceFactor> factors;
    std::vector<ResponseSurfaceRun> runs;
    std::vector<DiagnosticMessage> diagnostics;
    bool ok = false;
};

double coded_to_actual(double coded, const ResponseSurfaceFactor& factor);
double actual_to_coded(double actual, const ResponseSurfaceFactor& factor);
double factor_center(const ResponseSurfaceFactor& factor);
double factor_half_range(const ResponseSurfaceFactor& factor);

double default_ccd_alpha(CcdVariant variant, std::size_t factor_count);

ResponseSurfaceDesign generate_ccd(const ResponseSurfaceDesignOptions& options);
ResponseSurfaceDesign generate_bbd(const ResponseSurfaceDesignOptions& options);
ResponseSurfaceDesign generate_response_surface_design(
    const ResponseSurfaceDesignOptions& options);

}  // namespace datalab::domain::statistics
