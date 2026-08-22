#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct DiscriminantResult {
    std::size_t observation_count = 0;
    std::size_t predictor_count = 0;
    std::size_t class_count = 0;
    double train_accuracy = 0.0;
    std::vector<std::string> class_labels;
    std::vector<std::size_t> class_sizes;
    std::vector<std::vector<double>> class_means;  // [class][var]
    std::vector<std::size_t> predicted;
    std::vector<std::vector<std::size_t>> confusion;
    std::vector<double> ld1;
    std::vector<double> ld2;
    std::vector<DiagnosticMessage> diagnostics;
};

DiscriminantResult linear_discriminant(
    const std::vector<std::size_t>& class_index,
    const std::vector<std::vector<double>>& predictors,
    const std::vector<std::string>& class_labels);

}  // namespace datalab::domain::statistics
