#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/manova_one_way.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct GeneralManovaOptions {
    bool include_interaction = true;
    bool wilks = true;
    bool pillai = true;
    bool lawley_hotelling = true;
    bool roy = true;
};

struct GeneralManovaEffectTest {
    std::string effect_name;
    std::vector<ManovaTestRow> test_rows;
};

struct GeneralManovaCellMean {
    std::string cell_label;
    std::size_t count = 0;
    std::vector<double> means;
};

struct GeneralManovaResult {
    std::size_t observation_count = 0;
    std::size_t response_count = 0;
    std::vector<GeneralManovaEffectTest> effect_tests;
    std::vector<GeneralManovaCellMean> cell_means;
    std::vector<std::size_t> observation_source_rows;
    std::string algorithm_id = "general_manova_type3_sscp";
    std::string evidence_type = "formula_reference";
    std::vector<DiagnosticMessage> diagnostics;
};

GeneralManovaResult general_manova_analyze(
    const std::vector<std::vector<double>>& responses,
    const std::vector<std::string>& factor_a,
    const std::vector<std::string>& factor_b = {},
    const std::vector<double>& covariate = {},
    const std::vector<std::size_t>& source_rows = {},
    const GeneralManovaOptions& options = {});

}  // namespace datalab::domain::statistics
