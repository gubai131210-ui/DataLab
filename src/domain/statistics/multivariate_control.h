#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct HotellingT2Options {
    double alpha = 0.05;
    std::string phase = "phase1";  // phase1 | phase2
};

struct HotellingT2Result {
    std::size_t observation_count = 0;
    std::size_t variable_count = 0;
    std::vector<double> t2;
    std::vector<std::size_t> source_rows;
    std::vector<double> mean_vector;
    std::vector<std::vector<double>> covariance;
    double upper_control_limit = 0.0;
    std::optional<double> lower_control_limit;
    std::size_t out_of_control_count = 0;
    std::string limit_method;
    std::vector<DiagnosticMessage> diagnostics;
};

// rows[i][j] = observation i, variable j. Complete rectangular matrix required.
HotellingT2Result hotelling_t2_individuals(
    const std::vector<std::vector<double>>& rows,
    const std::vector<std::size_t>& source_rows,
    const HotellingT2Options& options = {});

struct MewmaOptions {
    double lambda = 0.1;
    double alpha = 0.05;
    std::optional<double> upper_control_limit;  // if set, overrides chi-square approx
};

struct MewmaResult {
    std::size_t observation_count = 0;
    std::size_t variable_count = 0;
    double lambda = 0.1;
    std::vector<double> t2;
    std::vector<std::size_t> source_rows;
    double upper_control_limit = 0.0;
    std::string ucl_method;
    std::size_t out_of_control_count = 0;
    std::vector<DiagnosticMessage> diagnostics;
};

MewmaResult mewma_chart(
    const std::vector<std::vector<double>>& rows,
    const std::vector<std::size_t>& source_rows,
    const MewmaOptions& options = {});

struct GeneralizedVarianceOptions {
    int subgroup_size = 0;  // required > p
};

struct GeneralizedVarianceResult {
    std::size_t subgroup_count = 0;
    std::size_t variable_count = 0;
    int subgroup_size = 0;
    double b1 = 0.0;
    double b2 = 0.0;
    double sigma_determinant = 0.0;  // |Σ̂|
    double center_line = 0.0;
    double upper_control_limit = 0.0;
    double lower_control_limit = 0.0;
    std::vector<double> plotted_determinants;
    std::vector<std::size_t> source_rows;  // first row of each subgroup
    std::size_t out_of_control_count = 0;
    std::vector<DiagnosticMessage> diagnostics;
};

// subgroups[g][i][j] = subgroup g, observation i, variable j. Equal n, n > p.
GeneralizedVarianceResult generalized_variance_chart(
    const std::vector<std::vector<std::vector<double>>>& subgroups,
    const std::vector<std::size_t>& subgroup_source_rows = {});

struct EmpCrossedResult {
    double icc_no_bias = 0.0;
    double icc_with_bias = 0.0;
    double icc_with_interaction = 0.0;
    double probable_error = 0.0;
    std::string classification;  // First|Second|Third|Fourth
    std::string classification_basis = "icc_with_interaction";
    double attenuation_percent = 0.0;
    std::vector<DiagnosticMessage> diagnostics;
};

EmpCrossedResult emp_classification_from_components(
    double part_variance,
    double repeatability,
    double operator_variance,
    double interaction_variance);

}  // namespace datalab::domain::statistics
