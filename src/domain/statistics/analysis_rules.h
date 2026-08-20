#pragma once

#include "domain/quality_types.h"
#include "domain/statistics/attribute_agreement.h"
#include "domain/statistics/gage_rr.h"
#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/inference_extensions.h"
#include "domain/statistics/msa_type1.h"
#include "domain/statistics/nested_gage_rr.h"
#include "domain/statistics/regression.h"
#include "domain/statistics/reliability.h"
#include "domain/statistics/two_factor_anova.h"

#include <string>
#include <vector>

namespace datalab::domain::statistics {

struct AnalysisRuleSpec {
    const char* id = "";
    const char* name = "";
    const char* description = "";
};

AssumptionCheck make_assumption_check(
    const std::string& name,
    const std::string& status,
    std::optional<double> statistic = std::nullopt,
    std::optional<double> p_value = std::nullopt,
    const std::string& evidence_summary = {});

RuleEvidence make_rule_evidence(
    const std::string& id,
    const std::string& status,
    const std::string& message,
    const std::vector<RowId>& related_rows = {},
    const std::string& suggested_action = {});

std::string combine_assumption_status(const std::vector<AssumptionCheck>& checks);

std::vector<AnalysisRuleSpec> regression_rule_catalog();
std::vector<AnalysisRuleSpec> anova_rule_catalog();
std::vector<AnalysisRuleSpec> msa_rule_catalog();
std::vector<AnalysisRuleSpec> reliability_rule_catalog();

RegressionFacts regression_facts_from(const RegressionResult& result);
AnovaFacts one_way_anova_facts_from(
    const AnovaResult& result,
    const TukeyResult& tukey = {},
    bool tukey_grouping_available = false,
    std::size_t grouping_letter_count = 0);
AnovaFacts two_factor_anova_facts_from(const TwoFactorAnovaResult& result);
MsaFacts gage_rr_facts_from(const GageRrResult& result);
MsaFacts nested_gage_facts_from(const NestedGageRrResult& result);
MsaFacts type1_facts_from(const MsaType1Result& result);
MsaFacts bias_linearity_facts_from(const BiasLinearityResult& result);
MsaFacts stability_facts_from(const StabilityResult& result);
MsaFacts attribute_agreement_facts_from(const AttributeAgreementResult& result);
ReliabilityFacts kaplan_meier_facts_from(const KaplanMeierResult& result);
ReliabilityFacts weibull_facts_from(const WeibullResult& result);
ReliabilityFacts exponential_facts_from(const ExponentialResult& result);
ReliabilityFacts lognormal_facts_from(const LognormalResult& result);

}  // namespace datalab::domain::statistics
