#include "domain/statistics/mixture_design.h"

#include <algorithm>
#include <random>

namespace datalab::domain::statistics {

MixtureDesign generate_mixture_simplex_lattice(const MixtureDesignOptions& options)
{
    MixtureDesign design;
    design.degree = 2;
    design.design_kind = "simplex_lattice";

    const std::size_t q = options.component_count;
    if (q < 3 || q > 4) {
        design.diagnostics.push_back({
            DiagnosticMessage::Severity::error, "mixture_bad_q",
            "Mixture simplex-lattice（本波）仅支持分量数 q = 3 或 4。"});
        return design;
    }
    design.component_count = q;
    design.run_count = q * (q + 1) / 2;

    design.component_names = options.component_names;
    if (design.component_names.size() < q) {
        for (std::size_t i = design.component_names.size(); i < q; ++i) {
            design.component_names.push_back("x" + std::to_string(i + 1));
        }
    } else if (design.component_names.size() > q) {
        design.component_names.resize(q);
    }

    std::vector<MixtureRun> runs;
    // Vertices e_i.
    for (std::size_t i = 0; i < q; ++i) {
        MixtureRun run;
        run.proportions.assign(q, 0.0);
        run.proportions[i] = 1.0;
        runs.push_back(std::move(run));
    }
    // Edge midpoints: xi = xj = 1/2.
    for (std::size_t i = 0; i < q; ++i) {
        for (std::size_t j = i + 1; j < q; ++j) {
            MixtureRun run;
            run.proportions.assign(q, 0.0);
            run.proportions[i] = 0.5;
            run.proportions[j] = 0.5;
            runs.push_back(std::move(run));
        }
    }

    std::vector<std::size_t> order(runs.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
        order[i] = i;
    }
    if (options.randomize) {
        std::mt19937_64 rng(options.random_seed == 0 ? 1 : options.random_seed);
        std::shuffle(order.begin(), order.end(), rng);
    }

    design.runs.reserve(runs.size());
    for (std::size_t run_order = 0; run_order < order.size(); ++run_order) {
        MixtureRun run = runs[order[run_order]];
        run.standard_order = order[run_order] + 1;
        run.run_order = run_order + 1;
        design.runs.push_back(std::move(run));
    }
    design.run_count = design.runs.size();
    return design;
}

}  // namespace datalab::domain::statistics
