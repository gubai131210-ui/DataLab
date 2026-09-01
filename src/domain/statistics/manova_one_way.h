#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct ManovaOneWayOptions {
    bool wilks = true;
    bool pillai = true;
    bool lawley_hotelling = true;
    bool roy = true;
};

struct ManovaTestRow {
    std::string test_name;
    double value = 0.0;
    std::optional<double> f_statistic;
    std::optional<double> numerator_df;
    std::optional<double> denominator_df;
    std::optional<double> p_value;
    bool approximate = false;
};

struct ManovaGroupMeanVector {
    std::string group;
    std::size_t count = 0;
    std::vector<double> means;
};

struct ManovaEigenRow {
    std::size_t index = 0;
    double eigenvalue = 0.0;
    double proportion = 0.0;
};

struct ManovaOneWayResult {
    std::size_t observation_count = 0;
    std::size_t response_count = 0;
    std::size_t group_count = 0;
    std::vector<ManovaTestRow> test_rows;
    std::vector<ManovaGroupMeanVector> group_means;
    std::vector<ManovaEigenRow> eigenvalues;
    std::vector<std::size_t> observation_source_rows;
    std::string algorithm_id = "manova_one_way_multivariate";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

ManovaOneWayResult manova_one_way_analyze(
    const std::vector<std::vector<double>>& responses,
    const std::vector<std::string>& factor,
    const std::vector<std::size_t>& source_rows = {},
    const ManovaOneWayOptions& options = {});

}  // namespace datalab::domain::statistics
