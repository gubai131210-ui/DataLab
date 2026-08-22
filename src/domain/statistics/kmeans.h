#pragma once

#include "domain/quality_types.h"

#include <cstddef>
#include <vector>

namespace datalab::domain::statistics {

struct KMeansOptions {
    std::size_t cluster_count = 2;
    std::size_t max_iterations = 100;
    bool standardize = false;
};

struct KMeansResult {
    std::size_t cluster_count = 0;
    std::size_t observation_count = 0;
    std::size_t variable_count = 0;
    std::size_t iterations = 0;
    bool converged = false;
    bool standardized = false;
    double total_within_ss = 0.0;

    std::vector<std::size_t> valid_rows;
    std::vector<std::size_t> assignments;  // 0-based cluster id
    std::vector<double> distances_to_centroid;
    std::vector<std::vector<double>> centroids;  // [cluster][variable] in analysis scale
    std::vector<std::size_t> cluster_sizes;
    std::vector<double> within_ss;
    std::vector<double> means;
    std::vector<double> scales;
    std::vector<DiagnosticMessage> diagnostics;
};

// Row-oriented complete-case numeric matrix. valid_rows are indices into the
// input rows vector (0-based). Euclidean K-Means; centroids start from the
// first k valid observations.
KMeansResult cluster_kmeans(
    const std::vector<std::vector<double>>& rows,
    const KMeansOptions& options = {});

}  // namespace datalab::domain::statistics
