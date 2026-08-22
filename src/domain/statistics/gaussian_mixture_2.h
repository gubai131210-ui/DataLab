#pragma once

// Gaussian mixture EM + BIC selection for capability gating.
// formula_reference:
//   gaussian_mixture_2_em      — fixed k=2 vs one Normal
//   gaussian_mixture_k_bic     — search k=1..k_max (default 4) by BIC
// Never opens process pass/fail; not vendor_oracle; not non-Gaussian mixtures.

#include "domain/quality_types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct GaussianMixture2Result {
    bool ok = false;
    std::string status = "not_run";
    // not_run | insufficient_n | failed | not_preferred | preferred_2comp
    double weight1 = 0.5;
    double weight2 = 0.5;
    double mean1 = 0.0;
    double mean2 = 0.0;
    double sd1 = 1.0;
    double sd2 = 1.0;
    double log_likelihood_1 = 0.0;
    double log_likelihood_2 = 0.0;
    double bic_1 = 0.0;
    double bic_2 = 0.0;
    double delta_bic = 0.0;  // bic_1 - bic_2; positive favors 2-comp
    int iterations = 0;
    bool converged = false;
    std::string evidence_type = "formula_reference";
    std::string algorithm_id = "gaussian_mixture_2_em";
    std::vector<DiagnosticMessage> diagnostics;
};

struct GaussianMixtureComponent {
    double weight = 0.0;
    double mean = 0.0;
    double sd = 1.0;
};

struct GaussianMixtureSearchResult {
    bool ok = false;
    std::string status = "not_run";
    // not_run | insufficient_n | failed | not_preferred | preferred_2comp | preferred_kcomp
    int k_max = 4;
    int k_selected = 1;
    std::vector<GaussianMixtureComponent> components;
    std::vector<double> bic_by_k;  // index 0 unused; bic_by_k[k] for k=1..k_max
    double bic_1 = 0.0;
    double bic_selected = 0.0;
    double delta_bic = 0.0;  // bic_1 - bic_selected; positive favors k_selected > 1
    int iterations = 0;
    bool converged = false;
    std::string evidence_type = "formula_reference";
    std::string algorithm_id = "gaussian_mixture_k_bic";
    std::vector<DiagnosticMessage> diagnostics;
};

// Requires n>=30 finite values. Prefers 2-comp when delta_bic >= 2 and both
// weights in (0.1, 0.9). Does not claim commercial/vendor alignment.
GaussianMixture2Result fit_gaussian_mixture_2(
    const std::vector<double>& observations,
    int max_iterations = 200,
    double tol = 1e-8);

// BIC search over k=1..k_max (clamped to [2,5]). Prefers k>=2 when
// delta_bic(bic_1 - bic_k*) >= 2 and every weight in (0.05, 0.95).
// status preferred_2comp when k*=2; preferred_kcomp when k*>2.
GaussianMixtureSearchResult fit_gaussian_mixture_search(
    const std::vector<double>& observations,
    int k_max = 4,
    int max_iterations = 200,
    double tol = 1e-8);

}  // namespace datalab::domain::statistics
