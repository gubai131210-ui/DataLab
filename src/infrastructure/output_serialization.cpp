#include "infrastructure/output_serialization.h"

#include <QJsonArray>
#include <QJsonObject>
#include <algorithm>
#include <optional>
#include <utility>

namespace datalab::infrastructure {
namespace {

QJsonArray string_array(const std::vector<std::string>& values)
{
    QJsonArray array;
    for (const std::string& value : values) {
        array.append(QString::fromStdString(value));
    }
    return array;
}

QJsonArray number_array(const std::vector<double>& values)
{
    QJsonArray array;
    for (const double value : values) {
        array.append(value);
    }
    return array;
}

QJsonArray size_array(const std::vector<std::size_t>& values)
{
    QJsonArray array;
    for (const std::size_t value : values) {
        array.append(static_cast<qint64>(value));
    }
    return array;
}

QJsonArray row_array(const std::vector<domain::RowId>& values)
{
    QJsonArray array;
    for (const domain::RowId value : values) {
        array.append(static_cast<qint64>(value));
    }
    return array;
}

QJsonArray int_array(const std::vector<int>& values)
{
    QJsonArray array;
    for (const int value : values) {
        array.append(value);
    }
    return array;
}

QJsonArray int_rows_array(const std::vector<std::vector<int>>& values)
{
    QJsonArray outer;
    for (const auto& row : values) {
        QJsonArray inner;
        for (const int value : row) {
            inner.append(value);
        }
        outer.append(inner);
    }
    return outer;
}

std::vector<std::string> to_strings(const QJsonArray& array)
{
    std::vector<std::string> values;
    for (const QJsonValue& value : array) {
        values.push_back(value.toString().toStdString());
    }
    return values;
}

std::vector<double> to_numbers(const QJsonArray& array)
{
    std::vector<double> values;
    for (const QJsonValue& value : array) {
        values.push_back(value.toDouble());
    }
    return values;
}

std::vector<std::size_t> to_sizes(const QJsonArray& array)
{
    std::vector<std::size_t> values;
    for (const QJsonValue& value : array) {
        values.push_back(static_cast<std::size_t>(value.toInteger()));
    }
    return values;
}

std::vector<int> to_ints(const QJsonArray& array)
{
    std::vector<int> values;
    for (const QJsonValue& value : array) {
        values.push_back(value.toInt());
    }
    return values;
}

std::vector<domain::RowId> to_rows(const QJsonArray& array)
{
    std::vector<domain::RowId> values;
    for (const QJsonValue& value : array) {
        values.push_back(static_cast<domain::RowId>(value.toInteger()));
    }
    return values;
}

std::vector<std::vector<int>> to_int_rows(const QJsonArray& array)
{
    std::vector<std::vector<int>> values;
    for (const QJsonValue& value : array) {
        std::vector<int> row;
        for (const QJsonValue& item : value.toArray()) {
            row.push_back(item.toInt());
        }
        values.push_back(std::move(row));
    }
    return values;
}

QJsonValue optional_number(const std::optional<double>& value)
{
    return value.has_value() ? QJsonValue(*value) : QJsonValue();
}

QJsonValue optional_size(const std::optional<std::size_t>& value)
{
    return value.has_value() ? QJsonValue(static_cast<qint64>(*value)) : QJsonValue();
}

std::optional<std::size_t> read_optional_size(const QJsonValue& value);
std::optional<double> read_optional(const QJsonValue& value);

QJsonArray write_assumptions(const std::vector<domain::AssumptionCheck>& checks)
{
    QJsonArray array;
    for (const auto& check : checks) {
        QJsonObject object;
        object.insert(QStringLiteral("name"), QString::fromStdString(check.name));
        object.insert(QStringLiteral("status"), QString::fromStdString(check.status));
        object.insert(QStringLiteral("statistic"), optional_number(check.statistic));
        object.insert(QStringLiteral("p_value"), optional_number(check.p_value));
        object.insert(QStringLiteral("evidence_summary"),
                      QString::fromStdString(check.evidence_summary));
        array.append(object);
    }
    return array;
}

std::vector<domain::AssumptionCheck> read_assumptions(const QJsonArray& array)
{
    std::vector<domain::AssumptionCheck> checks;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        checks.push_back({
            object.value(QStringLiteral("name")).toString().toStdString(),
            object.value(QStringLiteral("status")).toString("not_verified").toStdString(),
            read_optional(object.value(QStringLiteral("statistic"))),
            read_optional(object.value(QStringLiteral("p_value"))),
            object.value(QStringLiteral("evidence_summary")).toString().toStdString()});
    }
    return checks;
}

QJsonArray write_rules(const std::vector<domain::RuleEvidence>& rules)
{
    QJsonArray array;
    for (const auto& rule : rules) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), QString::fromStdString(rule.id));
        object.insert(QStringLiteral("status"), QString::fromStdString(rule.status));
        object.insert(QStringLiteral("message"), QString::fromStdString(rule.message));
        object.insert(QStringLiteral("related_rows"), row_array(rule.related_rows));
        object.insert(QStringLiteral("suggested_action"),
                      QString::fromStdString(rule.suggested_action));
        object.insert(QStringLiteral("name"), QString::fromStdString(rule.name));
        object.insert(QStringLiteral("window"), QString::fromStdString(rule.window));
        object.insert(QStringLiteral("threshold"), QString::fromStdString(rule.threshold));
        object.insert(QStringLiteral("comparison_direction"),
                      QString::fromStdString(rule.comparison_direction));
        QJsonArray plotted;
        for (const std::size_t point : rule.plotted_points) {
            plotted.append(static_cast<int>(point));
        }
        object.insert(QStringLiteral("plotted_points"), plotted);
        object.insert(QStringLiteral("not_applicable_reason"),
                      QString::fromStdString(rule.not_applicable_reason));
        object.insert(QStringLiteral("not_verified_reason"),
                      QString::fromStdString(rule.not_verified_reason));
        object.insert(QStringLiteral("calculation_failed_reason"),
                      QString::fromStdString(rule.calculation_failed_reason));
        array.append(object);
    }
    return array;
}

std::vector<domain::RuleEvidence> read_rules(const QJsonArray& array)
{
    std::vector<domain::RuleEvidence> rules;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        domain::RuleEvidence rule;
        rule.id = object.value(QStringLiteral("id")).toString().toStdString();
        rule.status = object.value(QStringLiteral("status"))
                          .toString("not_applicable")
                          .toStdString();
        rule.message = object.value(QStringLiteral("message")).toString().toStdString();
        rule.related_rows = to_rows(object.value(QStringLiteral("related_rows")).toArray());
        rule.suggested_action =
            object.value(QStringLiteral("suggested_action")).toString().toStdString();
        rule.name = object.value(QStringLiteral("name")).toString().toStdString();
        rule.window = object.value(QStringLiteral("window")).toString().toStdString();
        rule.threshold = object.value(QStringLiteral("threshold")).toString().toStdString();
        rule.comparison_direction =
            object.value(QStringLiteral("comparison_direction")).toString().toStdString();
        for (const QJsonValue& point :
             object.value(QStringLiteral("plotted_points")).toArray()) {
            rule.plotted_points.push_back(static_cast<std::size_t>(point.toInt()));
        }
        rule.not_applicable_reason =
            object.value(QStringLiteral("not_applicable_reason")).toString().toStdString();
        rule.not_verified_reason =
            object.value(QStringLiteral("not_verified_reason")).toString().toStdString();
        rule.calculation_failed_reason =
            object.value(QStringLiteral("calculation_failed_reason")).toString().toStdString();
        rules.push_back(std::move(rule));
    }
    return rules;
}

void write_interpretation_facts(
    QJsonObject& object,
    const domain::InterpretationFacts& facts)
{
    QJsonObject serialized;
    if (facts.capability.has_value()) {
        QJsonObject capability;
        capability.insert(QStringLiteral("cpk"), optional_number(facts.capability->cpk));
        capability.insert(QStringLiteral("ppk"), optional_number(facts.capability->ppk));
        capability.insert(QStringLiteral("cpm"), optional_number(facts.capability->cpm));
        capability.insert(QStringLiteral("z_bench"),
                          optional_number(facts.capability->z_bench));
        capability.insert(QStringLiteral("assumption_status"),
                          QString::fromStdString(facts.capability->assumption_status));
        capability.insert(QStringLiteral("specification_mode"),
                          QString::fromStdString(facts.capability->specification_mode));
        capability.insert(QStringLiteral("method"),
                          QString::fromStdString(facts.capability->method));
        capability.insert(QStringLiteral("johnson_family"),
                          QString::fromStdString(facts.capability->johnson_family));
        capability.insert(QStringLiteral("normality_p_value"),
                          optional_number(facts.capability->normality_p_value));
        capability.insert(QStringLiteral("transform_p_value"),
                          optional_number(facts.capability->transform_p_value));
        capability.insert(QStringLiteral("transform_anderson_darling"),
                          optional_number(facts.capability->transform_anderson_darling));
        capability.insert(QStringLiteral("nonnormal_distribution"),
                          QString::fromStdString(facts.capability->nonnormal_distribution));
        capability.insert(QStringLiteral("fitted_shape"),
                          optional_number(facts.capability->fitted_shape));
        capability.insert(QStringLiteral("fitted_scale"),
                          optional_number(facts.capability->fitted_scale));
        capability.insert(QStringLiteral("average_p"),
                          optional_number(facts.capability->average_p));
        capability.insert(QStringLiteral("percent_defective"),
                          optional_number(facts.capability->percent_defective));
        capability.insert(QStringLiteral("ppm_defective"),
                          optional_number(facts.capability->ppm_defective));
        capability.insert(QStringLiteral("process_z"),
                          optional_number(facts.capability->process_z));
        capability.insert(QStringLiteral("mean_dpu"),
                          optional_number(facts.capability->mean_dpu));
        capability.insert(QStringLiteral("cp"), optional_number(facts.capability->cp));
        capability.insert(QStringLiteral("pp"), optional_number(facts.capability->pp));
        capability.insert(QStringLiteral("cpk_lower"),
                          optional_number(facts.capability->cpk_lower));
        capability.insert(QStringLiteral("cpk_upper"),
                          optional_number(facts.capability->cpk_upper));
        capability.insert(QStringLiteral("ppk_lower"),
                          optional_number(facts.capability->ppk_lower));
        capability.insert(QStringLiteral("ppk_upper"),
                          optional_number(facts.capability->ppk_upper));
        capability.insert(QStringLiteral("capability_ci_method"),
                          QString::fromStdString(facts.capability->capability_ci_method));
        capability.insert(QStringLiteral("pass_fail_judgment_allowed"),
                          facts.capability->pass_fail_judgment_allowed);
        capability.insert(QStringLiteral("research_preview"),
                          facts.capability->research_preview);
        capability.insert(QStringLiteral("gate_status"),
                          QString::fromStdString(facts.capability->gate_status));
        capability.insert(QStringLiteral("evidence_type"),
                          QString::fromStdString(facts.capability->evidence_type));
        capability.insert(QStringLiteral("stability_screen_status"),
                          QString::fromStdString(facts.capability->stability_screen_status));
        capability.insert(QStringLiteral("stability_out_of_control_count"),
                          static_cast<int>(facts.capability->stability_out_of_control_count));
        capability.insert(QStringLiteral("bimodality_screen_status"),
                          QString::fromStdString(facts.capability->bimodality_screen_status));
        capability.insert(QStringLiteral("bimodality_peak_count"),
                          static_cast<int>(facts.capability->bimodality_peak_count));
        capability.insert(QStringLiteral("hartigan_dip_status"),
                          QString::fromStdString(facts.capability->hartigan_dip_status));
        capability.insert(QStringLiteral("hartigan_dip_statistic"),
                          facts.capability->hartigan_dip_statistic);
        if (facts.capability->hartigan_dip_p_value.has_value()) {
            capability.insert(QStringLiteral("hartigan_dip_p_value"),
                              *facts.capability->hartigan_dip_p_value);
        }
        capability.insert(QStringLiteral("mixture_status"),
                          QString::fromStdString(facts.capability->mixture_status));
        capability.insert(QStringLiteral("mixture_k_selected"),
                          facts.capability->mixture_k_selected);
        capability.insert(QStringLiteral("mixture_k_max"),
                          facts.capability->mixture_k_max);
        capability.insert(QStringLiteral("mixture_weight1"),
                          facts.capability->mixture_weight1);
        capability.insert(QStringLiteral("mixture_mean1"),
                          facts.capability->mixture_mean1);
        capability.insert(QStringLiteral("mixture_mean2"),
                          facts.capability->mixture_mean2);
        capability.insert(QStringLiteral("mixture_sd1"),
                          facts.capability->mixture_sd1);
        capability.insert(QStringLiteral("mixture_sd2"),
                          facts.capability->mixture_sd2);
        capability.insert(QStringLiteral("mixture_delta_bic"),
                          facts.capability->mixture_delta_bic);
        capability.insert(QStringLiteral("mixture_algorithm_id"),
                          QString::fromStdString(facts.capability->mixture_algorithm_id));
        capability.insert(QStringLiteral("mixture_evidence_type"),
                          QString::fromStdString(facts.capability->mixture_evidence_type));
        QJsonArray mixture_components;
        for (const auto& c : facts.capability->mixture_components) {
            QJsonObject item;
            item.insert(QStringLiteral("weight"), c.weight);
            item.insert(QStringLiteral("mean"), c.mean);
            item.insert(QStringLiteral("sd"), c.sd);
            mixture_components.push_back(item);
        }
        capability.insert(QStringLiteral("mixture_components"), mixture_components);
        serialized.insert(QStringLiteral("capability"), capability);
    }
    if (facts.spc.has_value()) {
        QJsonObject spc;
        spc.insert(QStringLiteral("out_of_control_count"),
                   optional_size(facts.spc->out_of_control_count));
        spc.insert(QStringLiteral("sigma_z"), optional_number(facts.spc->sigma_z));
        spc.insert(QStringLiteral("sigma_within"),
                   optional_number(facts.spc->sigma_within));
        spc.insert(QStringLiteral("sigma_between"),
                   optional_number(facts.spc->sigma_between));
        spc.insert(QStringLiteral("sigma_between_within"),
                   optional_number(facts.spc->sigma_between_within));
        spc.insert(QStringLiteral("between_within_method"),
                   QString::fromStdString(facts.spc->between_within_method));
        spc.insert(QStringLiteral("rule_policy"),
                   QString::fromStdString(facts.spc->rule_policy));
        QJsonArray enabled_tests;
        for (const int test : facts.spc->enabled_special_cause_tests) {
            enabled_tests.append(test);
        }
        spc.insert(QStringLiteral("enabled_special_cause_tests"), enabled_tests);
        QJsonArray enabled_rule_ids;
        for (const std::string& rule_id : facts.spc->enabled_special_cause_rule_ids) {
            enabled_rule_ids.append(QString::fromStdString(rule_id));
        }
        spc.insert(QStringLiteral("enabled_special_cause_rule_ids"), enabled_rule_ids);
        spc.insert(QStringLiteral("rules"), write_rules(facts.spc->rules));
        spc.insert(QStringLiteral("sigma_method"),
                   QString::fromStdString(facts.spc->sigma_method));
        spc.insert(QStringLiteral("use_nelson_estimate"),
                   facts.spc->use_nelson_estimate);
        spc.insert(QStringLiteral("nelson_excluded_ranges"),
                   static_cast<int>(facts.spc->nelson_excluded_ranges));
        spc.insert(QStringLiteral("estimated_sigma"),
                   optional_number(facts.spc->estimated_sigma));
        spc.insert(QStringLiteral("historical_parameters_used"),
                   facts.spc->historical_parameters_used);
        spc.insert(QStringLiteral("stage_count"),
                   static_cast<int>(facts.spc->stage_count));
        serialized.insert(QStringLiteral("spc"), spc);
    }
    if (facts.multivariate_spc.has_value()) {
        QJsonObject mv;
        mv.insert(QStringLiteral("kind"),
                  QString::fromStdString(facts.multivariate_spc->kind));
        mv.insert(QStringLiteral("observation_count"),
                  static_cast<int>(facts.multivariate_spc->observation_count));
        mv.insert(QStringLiteral("variable_count"),
                  static_cast<int>(facts.multivariate_spc->variable_count));
        mv.insert(QStringLiteral("subgroup_count"),
                  static_cast<int>(facts.multivariate_spc->subgroup_count));
        mv.insert(QStringLiteral("out_of_control_count"),
                  static_cast<int>(facts.multivariate_spc->out_of_control_count));
        mv.insert(QStringLiteral("upper_control_limit"),
                  optional_number(facts.multivariate_spc->upper_control_limit));
        mv.insert(QStringLiteral("lower_control_limit"),
                  optional_number(facts.multivariate_spc->lower_control_limit));
        mv.insert(QStringLiteral("center_line"),
                  optional_number(facts.multivariate_spc->center_line));
        mv.insert(QStringLiteral("lambda"),
                  optional_number(facts.multivariate_spc->lambda));
        mv.insert(QStringLiteral("limit_method"),
                  QString::fromStdString(facts.multivariate_spc->limit_method));
        mv.insert(QStringLiteral("phase"),
                  QString::fromStdString(facts.multivariate_spc->phase));
        serialized.insert(QStringLiteral("multivariate_spc"), mv);
    }
    if (facts.regression.has_value()) {
        QJsonObject regression;
        regression.insert(QStringLiteral("r_squared"),
                          optional_number(facts.regression->r_squared));
        regression.insert(QStringLiteral("residual_normality_p"),
                          optional_number(facts.regression->residual_normality_p));
        regression.insert(QStringLiteral("residual_anderson_darling"),
                          optional_number(facts.regression->residual_anderson_darling));
        regression.insert(QStringLiteral("residual_plot_count"),
                          static_cast<int>(facts.regression->residual_plot_count));
        regression.insert(QStringLiteral("influential_count"),
                          static_cast<int>(facts.regression->influential_count));
        regression.insert(QStringLiteral("assumption_status"),
                          QString::fromStdString(facts.regression->assumption_status));
        regression.insert(QStringLiteral("outlier_count"),
                          static_cast<int>(facts.regression->outlier_count));
        regression.insert(QStringLiteral("high_leverage_count"),
                          static_cast<int>(facts.regression->high_leverage_count));
        regression.insert(QStringLiteral("max_vif"),
                          optional_number(facts.regression->max_vif));
        regression.insert(QStringLiteral("durbin_watson"),
                          optional_number(facts.regression->durbin_watson));
        regression.insert(QStringLiteral("durbin_watson_dl"),
                          optional_number(facts.regression->durbin_watson_dl));
        regression.insert(QStringLiteral("durbin_watson_du"),
                          optional_number(facts.regression->durbin_watson_du));
        regression.insert(QStringLiteral("durbin_watson_decision"),
                          QString::fromStdString(facts.regression->durbin_watson_decision));
        regression.insert(QStringLiteral("error_degrees_of_freedom"),
                          optional_number(facts.regression->error_degrees_of_freedom));
        regression.insert(QStringLiteral("rank_deficient"),
                          facts.regression->rank_deficient);
        regression.insert(QStringLiteral("assumptions"),
                          write_assumptions(facts.regression->assumptions));
        regression.insert(QStringLiteral("rules"), write_rules(facts.regression->rules));
        serialized.insert(QStringLiteral("regression"), regression);
    }
    if (facts.anova.has_value()) {
        QJsonObject anova;
        anova.insert(QStringLiteral("p_value"), optional_number(facts.anova->p_value));
        anova.insert(QStringLiteral("error_degrees_of_freedom"),
                     static_cast<int>(facts.anova->error_degrees_of_freedom));
        anova.insert(QStringLiteral("estimable"), facts.anova->estimable);
        anova.insert(QStringLiteral("not_estimable_term_count"),
                     static_cast<int>(facts.anova->not_estimable_term_count));
        anova.insert(QStringLiteral("significant_terms"),
                     string_array(facts.anova->significant_terms));
        anova.insert(QStringLiteral("family_confidence_level"),
                     optional_number(facts.anova->family_confidence_level));
        anova.insert(QStringLiteral("tukey_significant_pairs"),
                     static_cast<int>(facts.anova->tukey_significant_pairs));
        anova.insert(QStringLiteral("tukey_method"),
                     QString::fromStdString(facts.anova->tukey_method));
        anova.insert(QStringLiteral("tukey_interval_columns"),
                     QString::fromStdString(facts.anova->tukey_interval_columns));
        anova.insert(QStringLiteral("tukey_grouping_available"),
                     facts.anova->tukey_grouping_available);
        anova.insert(QStringLiteral("grouping_letter_count"),
                     static_cast<int>(facts.anova->grouping_letter_count));
        anova.insert(QStringLiteral("assumption_status"),
                     QString::fromStdString(facts.anova->assumption_status));
        anova.insert(QStringLiteral("assumptions"),
                     write_assumptions(facts.anova->assumptions));
        anova.insert(QStringLiteral("rules"), write_rules(facts.anova->rules));
        serialized.insert(QStringLiteral("anova"), anova);
    }
    if (facts.msa.has_value()) {
        QJsonObject msa;
        msa.insert(QStringLiteral("slope"), optional_number(facts.msa->slope));
        msa.insert(QStringLiteral("bias_low"), optional_number(facts.msa->bias_low));
        msa.insert(QStringLiteral("bias_high"), optional_number(facts.msa->bias_high));
        msa.insert(QStringLiteral("p_value"), optional_number(facts.msa->p_value));
        msa.insert(QStringLiteral("cgk"), optional_number(facts.msa->cgk));
        msa.insert(QStringLiteral("tolerance_percent"),
                   optional_number(facts.msa->tolerance_percent));
        msa.insert(QStringLiteral("ndc"), optional_number(facts.msa->ndc));
        msa.insert(QStringLiteral("ndc_available"), facts.msa->ndc_available);
        msa.insert(QStringLiteral("design_balanced"), facts.msa->design_balanced);
        msa.insert(QStringLiteral("interaction_retained"), facts.msa->interaction_retained);
        msa.insert(QStringLiteral("interaction_p_value"),
                   optional_number(facts.msa->interaction_p_value));
        msa.insert(QStringLiteral("interaction_reduction_recommended"),
                   facts.msa->interaction_reduction_recommended);
        msa.insert(QStringLiteral("negative_variance_truncated"),
                   facts.msa->negative_variance_truncated);
        msa.insert(QStringLiteral("gage_percent_study_variation"),
                   optional_number(facts.msa->gage_percent_study_variation));
        msa.insert(QStringLiteral("gage_percent_contribution"),
                   optional_number(facts.msa->gage_percent_contribution));
        msa.insert(QStringLiteral("linearity"), optional_number(facts.msa->linearity));
        msa.insert(QStringLiteral("percent_linearity"),
                   optional_number(facts.msa->percent_linearity));
        msa.insert(QStringLiteral("slope_p_value"),
                   optional_number(facts.msa->slope_p_value));
        msa.insert(QStringLiteral("intercept_p_value"),
                   optional_number(facts.msa->intercept_p_value));
        msa.insert(QStringLiteral("residual_s"),
                   optional_number(facts.msa->residual_s));
        msa.insert(QStringLiteral("average_bias"),
                   optional_number(facts.msa->average_bias));
        msa.insert(QStringLiteral("average_bias_p"),
                   optional_number(facts.msa->average_bias_p));
        msa.insert(QStringLiteral("process_variation_used"),
                   optional_number(facts.msa->process_variation_used));
        msa.insert(QStringLiteral("ratings_are_ordinal"), facts.msa->ratings_are_ordinal);
        msa.insert(QStringLiteral("kendall_available"), facts.msa->kendall_available);
        msa.insert(QStringLiteral("weighted_kappa_available"),
                   facts.msa->weighted_kappa_available);
        msa.insert(QStringLiteral("kappa_weight_scheme"),
                   QString::fromStdString(facts.msa->kappa_weight_scheme));
        msa.insert(QStringLiteral("by_part_plot_available"),
                   facts.msa->by_part_plot_available);
        msa.insert(QStringLiteral("interaction_plot_available"),
                   facts.msa->interaction_plot_available);
        msa.insert(QStringLiteral("plot_point_count"),
                   static_cast<qint64>(facts.msa->plot_point_count));
        msa.insert(QStringLiteral("kendall_w"), optional_number(facts.msa->kendall_w));
        msa.insert(QStringLiteral("kendall_w_p"), optional_number(facts.msa->kendall_w_p));
        msa.insert(QStringLiteral("kendall_tau"), optional_number(facts.msa->kendall_tau));
        msa.insert(QStringLiteral("kendall_tau_p"), optional_number(facts.msa->kendall_tau_p));
        msa.insert(QStringLiteral("emp_available"), facts.msa->emp_available);
        msa.insert(QStringLiteral("emp_icc_no_bias"),
                   optional_number(facts.msa->emp_icc_no_bias));
        msa.insert(QStringLiteral("emp_icc_with_bias"),
                   optional_number(facts.msa->emp_icc_with_bias));
        msa.insert(QStringLiteral("emp_icc_with_interaction"),
                   optional_number(facts.msa->emp_icc_with_interaction));
        msa.insert(QStringLiteral("emp_probable_error"),
                   optional_number(facts.msa->emp_probable_error));
        msa.insert(QStringLiteral("emp_classification"),
                   QString::fromStdString(facts.msa->emp_classification));
        msa.insert(QStringLiteral("assumption_status"),
                   QString::fromStdString(facts.msa->assumption_status));
        msa.insert(QStringLiteral("rules"), write_rules(facts.msa->rules));
        serialized.insert(QStringLiteral("msa"), msa);
    }
    if (facts.reliability.has_value()) {
        QJsonObject reliability;
        reliability.insert(QStringLiteral("shape"),
                           optional_number(facts.reliability->shape));
        reliability.insert(QStringLiteral("censored_count"),
                           optional_size(facts.reliability->censored_count));
        reliability.insert(QStringLiteral("failure_count"),
                           optional_size(facts.reliability->failure_count));
        reliability.insert(QStringLiteral("valid_count"),
                           optional_size(facts.reliability->valid_count));
        reliability.insert(QStringLiteral("median_life"),
                           optional_number(facts.reliability->median_life));
        reliability.insert(QStringLiteral("identifiable"), facts.reliability->identifiable);
        reliability.insert(QStringLiteral("converged"), facts.reliability->converged);
        reliability.insert(QStringLiteral("not_computed_reason"),
                           QString::fromStdString(facts.reliability->not_computed_reason));
        reliability.insert(QStringLiteral("event_encoding"),
                           QString::fromStdString(facts.reliability->event_encoding));
        reliability.insert(QStringLiteral("distribution"),
                           QString::fromStdString(facts.reliability->distribution));
        reliability.insert(QStringLiteral("location"),
                           optional_number(facts.reliability->location));
        reliability.insert(QStringLiteral("scale"),
                           optional_number(facts.reliability->scale));
        reliability.insert(QStringLiteral("threshold"),
                           optional_number(facts.reliability->threshold));
        reliability.insert(QStringLiteral("rules"), write_rules(facts.reliability->rules));
        reliability.insert(QStringLiteral("evidence_type"),
                           QString::fromStdString(facts.reliability->evidence_type));
        reliability.insert(QStringLiteral("time_unit"),
                           QString::fromStdString(facts.reliability->time_unit));
        reliability.insert(QStringLiteral("exact_count"),
                           static_cast<int>(facts.reliability->exact_count));
        reliability.insert(QStringLiteral("right_censored_count"),
                           static_cast<int>(facts.reliability->right_censored_count));
        reliability.insert(QStringLiteral("left_censored_count"),
                           static_cast<int>(facts.reliability->left_censored_count));
        reliability.insert(QStringLiteral("interval_censored_count"),
                           static_cast<int>(facts.reliability->interval_censored_count));
        QJsonArray failure_modes;
        for (const auto& mode : facts.reliability->failure_modes) {
            failure_modes.append(QString::fromStdString(mode));
        }
        reliability.insert(QStringLiteral("failure_modes"), failure_modes);
        reliability.insert(QStringLiteral("failure_mode_distinct_count"),
                           static_cast<int>(facts.reliability->failure_mode_distinct_count));
        reliability.insert(QStringLiteral("total_exposure"),
                           optional_number(facts.reliability->total_exposure));
        reliability.insert(QStringLiteral("exposure_row_count"),
                           static_cast<int>(facts.reliability->exposure_row_count));
        reliability.insert(QStringLiteral("exposure_source"),
                           QString::fromStdString(facts.reliability->exposure_source));
        reliability.insert(QStringLiteral("mode_fit_scheme"),
                           QString::fromStdString(facts.reliability->mode_fit_scheme));
        QJsonArray mode_fits;
        for (const auto& fit : facts.reliability->mode_fits) {
            QJsonObject item;
            item.insert(QStringLiteral("failure_mode"),
                        QString::fromStdString(fit.failure_mode));
            item.insert(QStringLiteral("failure_count"),
                        static_cast<int>(fit.failure_count));
            item.insert(QStringLiteral("competing_failure_count"),
                        static_cast<int>(fit.competing_failure_count));
            item.insert(QStringLiteral("right_censored_count"),
                        static_cast<int>(fit.right_censored_count));
            item.insert(QStringLiteral("valid_count"),
                        static_cast<int>(fit.valid_count));
            item.insert(QStringLiteral("identifiable"), fit.identifiable);
            item.insert(QStringLiteral("converged"), fit.converged);
            item.insert(QStringLiteral("shape"), optional_number(fit.shape));
            item.insert(QStringLiteral("scale"), optional_number(fit.scale));
            item.insert(QStringLiteral("location"), optional_number(fit.location));
            item.insert(QStringLiteral("rate"), optional_number(fit.rate));
            item.insert(QStringLiteral("median_life"), optional_number(fit.median_life));
            item.insert(QStringLiteral("reliability_at_warranty"),
                        optional_number(fit.reliability_at_warranty));
            item.insert(QStringLiteral("not_computed_reason"),
                        QString::fromStdString(fit.not_computed_reason));
            item.insert(QStringLiteral("evidence_type"),
                        QString::fromStdString(fit.evidence_type));
            item.insert(QStringLiteral("algorithm_id"),
                        QString::fromStdString(fit.algorithm_id));
            QJsonArray rows;
            for (const std::size_t row : fit.source_rows) {
                rows.push_back(static_cast<int>(row));
            }
            item.insert(QStringLiteral("source_rows"), rows);
            mode_fits.push_back(item);
        }
        reliability.insert(QStringLiteral("mode_fits"), mode_fits);
        reliability.insert(QStringLiteral("cif_algorithm_id"),
                           QString::fromStdString(facts.reliability->cif_algorithm_id));
        reliability.insert(QStringLiteral("cif_evidence_type"),
                           QString::fromStdString(facts.reliability->cif_evidence_type));
        QJsonArray cif_modes;
        for (const auto& mode : facts.reliability->cif_modes) {
            QJsonObject item;
            item.insert(QStringLiteral("failure_mode"),
                        QString::fromStdString(mode.failure_mode));
            item.insert(QStringLiteral("failure_count"),
                        static_cast<int>(mode.failure_count));
            item.insert(QStringLiteral("cif_at_last_event"),
                        optional_number(mode.cif_at_last_event));
            item.insert(QStringLiteral("cif_at_warranty"),
                        optional_number(mode.cif_at_warranty));
            item.insert(QStringLiteral("point_count"),
                        static_cast<int>(mode.point_count));
            cif_modes.push_back(item);
        }
        reliability.insert(QStringLiteral("cif_modes"), cif_modes);
        reliability.insert(QStringLiteral("fine_gray_algorithm_id"),
                           QString::fromStdString(facts.reliability->fine_gray_algorithm_id));
        reliability.insert(QStringLiteral("fine_gray_evidence_type"),
                           QString::fromStdString(facts.reliability->fine_gray_evidence_type));
        reliability.insert(QStringLiteral("fine_gray_kind"),
                           QString::fromStdString(facts.reliability->fine_gray_kind));
        reliability.insert(QStringLiteral("fine_gray_target_mode"),
                           QString::fromStdString(facts.reliability->fine_gray_target_mode));
        reliability.insert(QStringLiteral("fine_gray_covariate_name"),
                           QString::fromStdString(facts.reliability->fine_gray_covariate_name));
        reliability.insert(QStringLiteral("fine_gray_group0"),
                           QString::fromStdString(facts.reliability->fine_gray_group0));
        reliability.insert(QStringLiteral("fine_gray_group1"),
                           QString::fromStdString(facts.reliability->fine_gray_group1));
        reliability.insert(QStringLiteral("fine_gray_converged"),
                           facts.reliability->fine_gray_converged);
        reliability.insert(QStringLiteral("fine_gray_covariate_mean"),
                           optional_number(facts.reliability->fine_gray_covariate_mean));
        reliability.insert(QStringLiteral("fine_gray_beta"),
                           optional_number(facts.reliability->fine_gray_beta));
        reliability.insert(QStringLiteral("fine_gray_se"),
                           optional_number(facts.reliability->fine_gray_se));
        reliability.insert(QStringLiteral("fine_gray_hazard_ratio"),
                           optional_number(facts.reliability->fine_gray_hazard_ratio));
        reliability.insert(QStringLiteral("fine_gray_p_value"),
                           optional_number(facts.reliability->fine_gray_p_value));
        reliability.insert(
            QStringLiteral("fine_gray_not_computed_reason"),
            QString::fromStdString(facts.reliability->fine_gray_not_computed_reason));
        reliability.insert(QStringLiteral("fine_gray_target_failures"),
                           static_cast<int>(facts.reliability->fine_gray_target_failures));
        reliability.insert(
            QStringLiteral("fine_gray_competing_failures"),
            static_cast<int>(facts.reliability->fine_gray_competing_failures));
        QJsonArray fine_gray_terms;
        for (const auto& term : facts.reliability->fine_gray_terms) {
            QJsonObject item;
            item.insert(QStringLiteral("name"), QString::fromStdString(term.name));
            item.insert(QStringLiteral("mean"), optional_number(term.mean));
            item.insert(QStringLiteral("beta"), optional_number(term.beta));
            item.insert(QStringLiteral("se"), optional_number(term.se));
            item.insert(QStringLiteral("hazard_ratio"), optional_number(term.hazard_ratio));
            item.insert(QStringLiteral("p_value"), optional_number(term.p_value));
            fine_gray_terms.push_back(item);
        }
        reliability.insert(QStringLiteral("fine_gray_terms"), fine_gray_terms);
        reliability.insert(QStringLiteral("log_rank_group_count"),
                           optional_size(facts.reliability->log_rank_group_count));
        reliability.insert(QStringLiteral("log_rank_chi_square"),
                           optional_number(facts.reliability->log_rank_chi_square));
        reliability.insert(QStringLiteral("log_rank_df"),
                           optional_number(facts.reliability->log_rank_df));
        reliability.insert(QStringLiteral("log_rank_p_value"),
                           optional_number(facts.reliability->log_rank_p_value));
        QJsonArray log_rank_groups;
        for (const auto& group : facts.reliability->log_rank_groups) {
            QJsonObject item;
            item.insert(QStringLiteral("label"), QString::fromStdString(group.label));
            item.insert(QStringLiteral("group_id"), group.group_id);
            item.insert(QStringLiteral("n"), static_cast<int>(group.n));
            item.insert(QStringLiteral("failures"), static_cast<int>(group.failures));
            item.insert(QStringLiteral("censored"), static_cast<int>(group.censored));
            log_rank_groups.push_back(item);
        }
        reliability.insert(QStringLiteral("log_rank_groups"), log_rank_groups);
        reliability.insert(QStringLiteral("gray_chi_square"),
                           optional_number(facts.reliability->gray_chi_square));
        reliability.insert(QStringLiteral("gray_df"),
                           optional_number(facts.reliability->gray_df));
        reliability.insert(QStringLiteral("gray_p_value"),
                           optional_number(facts.reliability->gray_p_value));
        if (facts.reliability->gray_group_count.has_value()) {
            reliability.insert(QStringLiteral("gray_group_count"),
                               static_cast<int>(*facts.reliability->gray_group_count));
        }
        reliability.insert(QStringLiteral("gray_not_computed_reason"),
                           QString::fromStdString(facts.reliability->gray_not_computed_reason));
        reliability.insert(QStringLiteral("gray_algorithm_id"),
                           QString::fromStdString(facts.reliability->gray_algorithm_id));
        serialized.insert(QStringLiteral("reliability"), reliability);
    }
    if (facts.warranty.has_value()) {
        QJsonObject warranty;
        warranty.insert(QStringLiteral("warranty_time"), facts.warranty->warranty_time);
        warranty.insert(QStringLiteral("time_unit"),
                        QString::fromStdString(facts.warranty->time_unit));
        warranty.insert(QStringLiteral("exposure"), facts.warranty->exposure);
        warranty.insert(QStringLiteral("reliability_at_warranty"),
                        facts.warranty->reliability_at_warranty);
        warranty.insert(QStringLiteral("failure_probability"),
                        facts.warranty->failure_probability);
        warranty.insert(QStringLiteral("expected_failures"),
                        facts.warranty->expected_failures);
        warranty.insert(QStringLiteral("claims_per_1000"),
                        facts.warranty->claims_per_1000);
        warranty.insert(QStringLiteral("observed_failures"),
                        static_cast<int>(facts.warranty->observed_failures));
        warranty.insert(QStringLiteral("censored_count"),
                        static_cast<int>(facts.warranty->censored_count));
        warranty.insert(QStringLiteral("valid_count"),
                        static_cast<int>(facts.warranty->valid_count));
        warranty.insert(QStringLiteral("model_name"),
                        QString::fromStdString(facts.warranty->model_name));
        warranty.insert(QStringLiteral("quantity_label"),
                        QString::fromStdString(facts.warranty->quantity_label));
        warranty.insert(QStringLiteral("evidence_type"),
                        QString::fromStdString(facts.warranty->evidence_type));
        warranty.insert(QStringLiteral("exposure_source"),
                        QString::fromStdString(facts.warranty->exposure_source));
        warranty.insert(QStringLiteral("exposure_row_count"),
                        static_cast<int>(facts.warranty->exposure_row_count));
        warranty.insert(QStringLiteral("stratum_kind"),
                        QString::fromStdString(facts.warranty->stratum_kind));
        warranty.insert(QStringLiteral("uses_pooled_reliability"),
                        facts.warranty->uses_pooled_reliability);
        warranty.insert(QStringLiteral("uses_mode_specific_reliability"),
                        facts.warranty->uses_mode_specific_reliability);
        QJsonArray strata;
        for (const auto& stratum : facts.warranty->strata) {
            QJsonObject item;
            item.insert(QStringLiteral("label"),
                        QString::fromStdString(stratum.label));
            item.insert(QStringLiteral("kind"),
                        QString::fromStdString(stratum.kind));
            item.insert(QStringLiteral("exposure"), stratum.exposure);
            item.insert(QStringLiteral("observed_failures"),
                        static_cast<int>(stratum.observed_failures));
            item.insert(QStringLiteral("censored_count"),
                        static_cast<int>(stratum.censored_count));
            item.insert(QStringLiteral("valid_count"),
                        static_cast<int>(stratum.valid_count));
            item.insert(QStringLiteral("expected_failures"),
                        stratum.expected_failures);
            item.insert(QStringLiteral("share_of_total_exposure"),
                        stratum.share_of_total_exposure);
            item.insert(QStringLiteral("exposure_attribution"),
                        QString::fromStdString(stratum.exposure_attribution));
            item.insert(QStringLiteral("reliability_at_warranty"),
                        optional_number(stratum.reliability_at_warranty));
            item.insert(QStringLiteral("uses_mode_specific_reliability"),
                        stratum.uses_mode_specific_reliability);
            QJsonArray rows;
            for (const std::size_t row : stratum.source_rows) {
                rows.push_back(static_cast<int>(row));
            }
            item.insert(QStringLiteral("source_rows"), rows);
            strata.push_back(item);
        }
        warranty.insert(QStringLiteral("strata"), strata);
        serialized.insert(QStringLiteral("warranty"), warranty);
    }
    if (facts.forecast.has_value()) {
        QJsonObject forecast;
        forecast.insert(QStringLiteral("mape"), optional_number(facts.forecast->mape));
        forecast.insert(QStringLiteral("mase"), optional_number(facts.forecast->mase));
        forecast.insert(QStringLiteral("rolling_origin_mape"),
                        optional_number(facts.forecast->rolling_origin_mape));
        forecast.insert(QStringLiteral("rolling_origin_mase"),
                        optional_number(facts.forecast->rolling_origin_mase));
        serialized.insert(QStringLiteral("forecast"), forecast);
    }
    if (facts.power.has_value()) {
        QJsonObject power;
        power.insert(QStringLiteral("power"), optional_number(facts.power->power));
        power.insert(QStringLiteral("effect_size"), optional_number(facts.power->effect_size));
        power.insert(QStringLiteral("mode"), QString::fromStdString(facts.power->mode));
        power.insert(QStringLiteral("sample_size"), optional_size(facts.power->sample_size));
        power.insert(QStringLiteral("target"), optional_number(facts.power->target));
        power.insert(QStringLiteral("actual_power"), optional_number(facts.power->actual_power));
        serialized.insert(QStringLiteral("power"), power);
    }
    if (facts.descriptive.has_value()) {
        QJsonObject descriptive;
        descriptive.insert(QStringLiteral("n"), static_cast<int>(facts.descriptive->n));
        descriptive.insert(QStringLiteral("missing_count"),
                           static_cast<int>(facts.descriptive->missing_count));
        descriptive.insert(QStringLiteral("mean"), optional_number(facts.descriptive->mean));
        descriptive.insert(QStringLiteral("standard_deviation"),
                           optional_number(facts.descriptive->standard_deviation));
        serialized.insert(QStringLiteral("descriptive"), descriptive);
    }
    if (facts.chi_square.has_value()) {
        QJsonObject chi_square;
        chi_square.insert(QStringLiteral("statistic"),
                          optional_number(facts.chi_square->statistic));
        chi_square.insert(QStringLiteral("p_value"),
                          optional_number(facts.chi_square->p_value));
        chi_square.insert(QStringLiteral("degrees_of_freedom"),
                          optional_number(facts.chi_square->degrees_of_freedom));
        chi_square.insert(QStringLiteral("expected_count_warning"),
                          facts.chi_square->expected_count_warning);
        chi_square.insert(QStringLiteral("row_count"),
                          static_cast<int>(facts.chi_square->row_count));
        chi_square.insert(QStringLiteral("column_count"),
                          static_cast<int>(facts.chi_square->column_count));
        chi_square.insert(QStringLiteral("total_count"),
                          static_cast<int>(facts.chi_square->total_count));
        chi_square.insert(QStringLiteral("missing_count"),
                          static_cast<int>(facts.chi_square->missing_count));
        chi_square.insert(QStringLiteral("likelihood_ratio_statistic"),
                          optional_number(facts.chi_square->likelihood_ratio_statistic));
        chi_square.insert(QStringLiteral("likelihood_ratio_p_value"),
                          optional_number(facts.chi_square->likelihood_ratio_p_value));
        chi_square.insert(QStringLiteral("plot_available"),
                          facts.chi_square->plot_available);
        chi_square.insert(QStringLiteral("fisher_p_value"),
                          optional_number(facts.chi_square->fisher_p_value));
        chi_square.insert(QStringLiteral("odds_ratio"),
                          optional_number(facts.chi_square->odds_ratio));
        chi_square.insert(QStringLiteral("method"),
                          QString::fromStdString(facts.chi_square->method));
        chi_square.insert(QStringLiteral("max_abs_adjusted_residual"),
                          optional_number(facts.chi_square->max_abs_adjusted_residual));
        chi_square.insert(QStringLiteral("largest_contribution_cell"),
                          QString::fromStdString(facts.chi_square->largest_contribution_cell));
        chi_square.insert(QStringLiteral("residual_heatmap_available"),
                          facts.chi_square->residual_heatmap_available);
        chi_square.insert(QStringLiteral("percent_tables_available"),
                          facts.chi_square->percent_tables_available);
        serialized.insert(QStringLiteral("chi_square"), chi_square);
    }
    if (facts.cross_tab.has_value()) {
        QJsonObject cross_tab;
        cross_tab.insert(QStringLiteral("row_count"),
                         static_cast<int>(facts.cross_tab->row_count));
        cross_tab.insert(QStringLiteral("column_count"),
                         static_cast<int>(facts.cross_tab->column_count));
        cross_tab.insert(QStringLiteral("total_count"),
                         static_cast<int>(facts.cross_tab->total_count));
        cross_tab.insert(QStringLiteral("missing_count"),
                         static_cast<int>(facts.cross_tab->missing_count));
        cross_tab.insert(QStringLiteral("percent_tables_available"),
                         facts.cross_tab->percent_tables_available);
        serialized.insert(QStringLiteral("cross_tab"), cross_tab);
    }
    if (facts.chi_square_gof.has_value()) {
        QJsonObject gof;
        gof.insert(QStringLiteral("statistic"),
                   optional_number(facts.chi_square_gof->statistic));
        gof.insert(QStringLiteral("p_value"),
                   optional_number(facts.chi_square_gof->p_value));
        gof.insert(QStringLiteral("degrees_of_freedom"),
                   optional_number(facts.chi_square_gof->degrees_of_freedom));
        gof.insert(QStringLiteral("category_count"),
                   static_cast<int>(facts.chi_square_gof->category_count));
        gof.insert(QStringLiteral("total_count"),
                   static_cast<int>(facts.chi_square_gof->total_count));
        gof.insert(QStringLiteral("missing_count"),
                   static_cast<int>(facts.chi_square_gof->missing_count));
        gof.insert(QStringLiteral("expected_count_warning"),
                   facts.chi_square_gof->expected_count_warning);
        gof.insert(QStringLiteral("expected_below_five_count"),
                   static_cast<int>(facts.chi_square_gof->expected_below_five_count));
        gof.insert(QStringLiteral("minimum_expected_count"),
                   optional_number(facts.chi_square_gof->minimum_expected_count));
        gof.insert(QStringLiteral("validity_status"),
                   QString::fromStdString(facts.chi_square_gof->validity_status));
        gof.insert(QStringLiteral("recommendation"),
                   QString::fromStdString(facts.chi_square_gof->recommendation));
        gof.insert(QStringLiteral("plot_available"),
                   facts.chi_square_gof->plot_available);
        gof.insert(QStringLiteral("proportion_source"),
                   QString::fromStdString(facts.chi_square_gof->proportion_source));
        gof.insert(QStringLiteral("method"),
                   QString::fromStdString(facts.chi_square_gof->method));
        gof.insert(QStringLiteral("lambda_hat"),
                   optional_number(facts.chi_square_gof->lambda_hat));
        serialized.insert(QStringLiteral("chi_square_gof"), gof);
    }
    if (facts.mcnemar.has_value()) {
        QJsonObject mcnemar;
        mcnemar.insert(QStringLiteral("a"), static_cast<int>(facts.mcnemar->a));
        mcnemar.insert(QStringLiteral("b"), static_cast<int>(facts.mcnemar->b));
        mcnemar.insert(QStringLiteral("c"), static_cast<int>(facts.mcnemar->c));
        mcnemar.insert(QStringLiteral("d"), static_cast<int>(facts.mcnemar->d));
        mcnemar.insert(QStringLiteral("discordant"),
                       static_cast<int>(facts.mcnemar->discordant));
        mcnemar.insert(QStringLiteral("pair_count"),
                       static_cast<int>(facts.mcnemar->pair_count));
        mcnemar.insert(QStringLiteral("missing_count"),
                       static_cast<int>(facts.mcnemar->missing_count));
        mcnemar.insert(QStringLiteral("chi_square"),
                       optional_number(facts.mcnemar->chi_square));
        mcnemar.insert(QStringLiteral("p_value"),
                       optional_number(facts.mcnemar->p_value));
        mcnemar.insert(QStringLiteral("degrees_of_freedom"),
                       facts.mcnemar->degrees_of_freedom);
        mcnemar.insert(QStringLiteral("continuity_correction"),
                       facts.mcnemar->continuity_correction);
        mcnemar.insert(QStringLiteral("method"),
                       QString::fromStdString(facts.mcnemar->method));
        mcnemar.insert(QStringLiteral("computable"), facts.mcnemar->computable);
        serialized.insert(QStringLiteral("mcnemar"), mcnemar);
    }
    if (facts.cochran_q.has_value()) {
        QJsonObject cochran;
        cochran.insert(QStringLiteral("treatment_count"),
                       static_cast<int>(facts.cochran_q->treatment_count));
        cochran.insert(QStringLiteral("subject_count"),
                       static_cast<int>(facts.cochran_q->subject_count));
        cochran.insert(QStringLiteral("missing_count"),
                       static_cast<int>(facts.cochran_q->missing_count));
        cochran.insert(QStringLiteral("q_statistic"),
                       optional_number(facts.cochran_q->q_statistic));
        cochran.insert(QStringLiteral("p_value"),
                       optional_number(facts.cochran_q->p_value));
        cochran.insert(QStringLiteral("degrees_of_freedom"),
                       facts.cochran_q->degrees_of_freedom);
        cochran.insert(QStringLiteral("computable"), facts.cochran_q->computable);
        cochran.insert(QStringLiteral("approximation"),
                       QString::fromStdString(facts.cochran_q->approximation));
        serialized.insert(QStringLiteral("cochran_q"), cochran);
    }
    if (facts.nonparametric.has_value()) {
        QJsonObject nonparametric;
        nonparametric.insert(QStringLiteral("method"),
                             QString::fromStdString(facts.nonparametric->method));
        nonparametric.insert(QStringLiteral("statistic"),
                             optional_number(facts.nonparametric->statistic));
        nonparametric.insert(QStringLiteral("p_value"),
                             optional_number(facts.nonparametric->p_value));
        nonparametric.insert(QStringLiteral("tie_correction"),
                             facts.nonparametric->tie_correction);
        nonparametric.insert(QStringLiteral("continuity_correction"),
                             facts.nonparametric->continuity_correction);
        nonparametric.insert(QStringLiteral("approximation"),
                             QString::fromStdString(facts.nonparametric->approximation));
        nonparametric.insert(QStringLiteral("small_sample_warning"),
                             facts.nonparametric->small_sample_warning);
        nonparametric.insert(QStringLiteral("effect_size"),
                             optional_number(facts.nonparametric->effect_size));
        nonparametric.insert(QStringLiteral("p_value_unadjusted"),
                             optional_number(facts.nonparametric->p_value_unadjusted));
        nonparametric.insert(QStringLiteral("group_count"),
                             static_cast<int>(facts.nonparametric->group_count));
        nonparametric.insert(QStringLiteral("plot_point_count"),
                             static_cast<int>(facts.nonparametric->plot_point_count));
        nonparametric.insert(QStringLiteral("missing_count"),
                             static_cast<int>(facts.nonparametric->missing_count));
        nonparametric.insert(QStringLiteral("location_estimate"),
                             optional_number(facts.nonparametric->location_estimate));
        nonparametric.insert(QStringLiteral("ci_lower"),
                             optional_number(facts.nonparametric->ci_lower));
        nonparametric.insert(QStringLiteral("ci_upper"),
                             optional_number(facts.nonparametric->ci_upper));
        nonparametric.insert(QStringLiteral("dunn_available"),
                             facts.nonparametric->dunn_available);
        nonparametric.insert(QStringLiteral("steel_dwass_available"),
                             facts.nonparametric->steel_dwass_available);
        nonparametric.insert(QStringLiteral("nemenyi_available"),
                             facts.nonparametric->nemenyi_available);
        nonparametric.insert(QStringLiteral("posthoc_method"),
                             QString::fromStdString(facts.nonparametric->posthoc_method));
        nonparametric.insert(QStringLiteral("posthoc_pair_count"),
                             static_cast<int>(facts.nonparametric->posthoc_pair_count));
        nonparametric.insert(QStringLiteral("grouping_letter_count"),
                             static_cast<int>(facts.nonparametric->grouping_letter_count));
        serialized.insert(QStringLiteral("nonparametric"), nonparametric);
    }
    if (facts.logistic.has_value()) {
        QJsonObject logistic;
        logistic.insert(QStringLiteral("converged"), facts.logistic->converged);
        logistic.insert(QStringLiteral("complete_separation"),
                        facts.logistic->complete_separation);
        logistic.insert(QStringLiteral("hosmer_lemeshow_p"),
                        optional_number(facts.logistic->hosmer_lemeshow_p));
        logistic.insert(QStringLiteral("hosmer_lemeshow_statistic"),
                        optional_number(facts.logistic->hosmer_lemeshow_statistic));
        if (facts.logistic->hosmer_lemeshow_df.has_value()) {
            logistic.insert(QStringLiteral("hosmer_lemeshow_df"),
                            static_cast<qint64>(*facts.logistic->hosmer_lemeshow_df));
        }
        logistic.insert(QStringLiteral("hosmer_lemeshow_groups"),
                        static_cast<qint64>(facts.logistic->hosmer_lemeshow_groups));
        logistic.insert(QStringLiteral("hosmer_lemeshow_status"),
                        QString::fromStdString(facts.logistic->hosmer_lemeshow_status));
        logistic.insert(QStringLiteral("high_leverage_count"),
                        static_cast<qint64>(facts.logistic->high_leverage_count));
        logistic.insert(QStringLiteral("leverage_threshold"),
                        optional_number(facts.logistic->leverage_threshold));
        logistic.insert(QStringLiteral("maximum_leverage"),
                        optional_number(facts.logistic->maximum_leverage));
        logistic.insert(QStringLiteral("maximum_vif"),
                        optional_number(facts.logistic->maximum_vif));
        logistic.insert(QStringLiteral("concordant_pairs"),
                        static_cast<qint64>(facts.logistic->concordant_pairs));
        logistic.insert(QStringLiteral("discordant_pairs"),
                        static_cast<qint64>(facts.logistic->discordant_pairs));
        logistic.insert(QStringLiteral("tied_pairs"),
                        static_cast<qint64>(facts.logistic->tied_pairs));
        logistic.insert(QStringLiteral("pairs_concordance_percent"),
                        optional_number(facts.logistic->pairs_concordance_percent));
        logistic.insert(QStringLiteral("true_positive"),
                        static_cast<qint64>(facts.logistic->true_positive));
        logistic.insert(QStringLiteral("true_negative"),
                        static_cast<qint64>(facts.logistic->true_negative));
        logistic.insert(QStringLiteral("false_positive"),
                        static_cast<qint64>(facts.logistic->false_positive));
        logistic.insert(QStringLiteral("false_negative"),
                        static_cast<qint64>(facts.logistic->false_negative));
        logistic.insert(QStringLiteral("stepwise_method"),
                        QString::fromStdString(facts.logistic->stepwise_method));
        logistic.insert(QStringLiteral("stepwise_criterion"),
                        QString::fromStdString(facts.logistic->stepwise_criterion));
        logistic.insert(QStringLiteral("stepwise_step_count"),
                        static_cast<qint64>(facts.logistic->stepwise_step_count));
        logistic.insert(QStringLiteral("stepwise_selected_count"),
                        static_cast<qint64>(facts.logistic->stepwise_selected_count));
        logistic.insert(QStringLiteral("stepwise_best_step_index"),
                        static_cast<qint64>(facts.logistic->stepwise_best_step_index));
        logistic.insert(QStringLiteral("stepwise_log_likelihood"),
                        optional_number(facts.logistic->stepwise_log_likelihood));
        logistic.insert(QStringLiteral("stepwise_aic"),
                        optional_number(facts.logistic->stepwise_aic));
        logistic.insert(QStringLiteral("stepwise_bic"),
                        optional_number(facts.logistic->stepwise_bic));
        QJsonArray stepwise_steps;
        for (const auto& step : facts.logistic->stepwise_steps) {
            QJsonObject item;
            item.insert(QStringLiteral("step"), static_cast<int>(step.step));
            item.insert(QStringLiteral("action"), QString::fromStdString(step.action));
            item.insert(QStringLiteral("term"), QString::fromStdString(step.term));
            item.insert(QStringLiteral("deviance"), optional_number(step.deviance));
            item.insert(QStringLiteral("aic"), optional_number(step.aic));
            item.insert(QStringLiteral("aicc"), optional_number(step.aicc));
            item.insert(QStringLiteral("bic"), optional_number(step.bic));
            item.insert(QStringLiteral("enter_p"), optional_number(step.enter_p));
            item.insert(QStringLiteral("remove_p"), optional_number(step.remove_p));
            stepwise_steps.push_back(item);
        }
        logistic.insert(QStringLiteral("stepwise_steps"), stepwise_steps);
        serialized.insert(QStringLiteral("logistic"), logistic);
    }
    if (facts.distribution_identification.has_value()) {
        QJsonObject distribution_id;
        distribution_id.insert(QStringLiteral("best_distribution"),
                               QString::fromStdString(
                                   facts.distribution_identification->best_distribution));
        distribution_id.insert(QStringLiteral("best_anderson_darling"),
                               optional_number(
                                   facts.distribution_identification->best_anderson_darling));
        distribution_id.insert(QStringLiteral("best_p_value"),
                               optional_number(
                                   facts.distribution_identification->best_p_value));
        distribution_id.insert(QStringLiteral("did_not_change_capability_defaults"),
                             facts.distribution_identification
                                 ->did_not_change_capability_defaults);
        serialized.insert(QStringLiteral("distribution_identification"), distribution_id);
    }
    if (facts.pca.has_value()) {
        QJsonObject pca;
        pca.insert(QStringLiteral("mode"), QString::fromStdString(facts.pca->mode));
        pca.insert(QStringLiteral("retained_component_count"),
                   static_cast<int>(facts.pca->retained_component_count));
        pca.insert(QStringLiteral("anomaly_count"),
                   static_cast<int>(facts.pca->anomaly_count));
        pca.insert(QStringLiteral("observation_count"),
                   static_cast<int>(facts.pca->observation_count));
        pca.insert(QStringLiteral("t2_limit"), optional_number(facts.pca->t2_limit));
        pca.insert(QStringLiteral("q_limit"), optional_number(facts.pca->q_limit));
        pca.insert(QStringLiteral("residual_ad_p"), optional_number(facts.pca->residual_ad_p));
        pca.insert(QStringLiteral("diagnostic_plot_count"),
                   static_cast<int>(facts.pca->diagnostic_plot_count));
        pca.insert(QStringLiteral("converged"), facts.pca->converged);
        serialized.insert(QStringLiteral("pca"), pca);
    }
    if (facts.kmeans.has_value()) {
        QJsonObject kmeans;
        kmeans.insert(QStringLiteral("k"), static_cast<int>(facts.kmeans->k));
        kmeans.insert(QStringLiteral("n"), static_cast<int>(facts.kmeans->n));
        kmeans.insert(QStringLiteral("variable_count"),
                      static_cast<int>(facts.kmeans->variable_count));
        kmeans.insert(QStringLiteral("iterations"),
                      static_cast<int>(facts.kmeans->iterations));
        kmeans.insert(QStringLiteral("converged"), facts.kmeans->converged);
        kmeans.insert(QStringLiteral("standardized"), facts.kmeans->standardized);
        kmeans.insert(QStringLiteral("total_within_ss"),
                      optional_number(facts.kmeans->total_within_ss));
        serialized.insert(QStringLiteral("kmeans"), kmeans);
    }
    if (facts.cart_tree.has_value()) {
        QJsonObject cart;
        cart.insert(QStringLiteral("task"), QString::fromStdString(facts.cart_tree->task));
        cart.insert(QStringLiteral("n"), static_cast<int>(facts.cart_tree->n));
        cart.insert(QStringLiteral("predictor_count"),
                    static_cast<int>(facts.cart_tree->predictor_count));
        cart.insert(QStringLiteral("max_depth"),
                    static_cast<int>(facts.cart_tree->max_depth));
        cart.insert(QStringLiteral("node_count"),
                    static_cast<int>(facts.cart_tree->node_count));
        cart.insert(QStringLiteral("leaf_count"),
                    static_cast<int>(facts.cart_tree->leaf_count));
        cart.insert(QStringLiteral("train_metric"),
                    optional_number(facts.cart_tree->train_metric));
        cart.insert(QStringLiteral("top_variable"),
                    QString::fromStdString(facts.cart_tree->top_variable));
        serialized.insert(QStringLiteral("cart_tree"), cart);
    }
    if (facts.adf.has_value()) {
        QJsonObject adf;
        adf.insert(QStringLiteral("n"), static_cast<int>(facts.adf->n));
        adf.insert(QStringLiteral("missing_count"),
                   static_cast<int>(facts.adf->missing_count));
        adf.insert(QStringLiteral("lags"), static_cast<int>(facts.adf->lags));
        adf.insert(QStringLiteral("used_observations"),
                   static_cast<int>(facts.adf->used_observations));
        adf.insert(QStringLiteral("regression"),
                   QString::fromStdString(facts.adf->regression));
        adf.insert(QStringLiteral("tau"), optional_number(facts.adf->tau));
        adf.insert(QStringLiteral("critical_5"), optional_number(facts.adf->critical_5));
        adf.insert(QStringLiteral("reject_unit_root_at_5"),
                   facts.adf->reject_unit_root_at_5);
        serialized.insert(QStringLiteral("adf"), adf);
    }
    if (facts.poisson_regression.has_value()) {
        QJsonObject poisson;
        poisson.insert(QStringLiteral("n"), static_cast<int>(facts.poisson_regression->n));
        poisson.insert(QStringLiteral("predictor_count"),
                       static_cast<int>(facts.poisson_regression->predictor_count));
        poisson.insert(QStringLiteral("iteration_count"),
                       static_cast<int>(facts.poisson_regression->iteration_count));
        poisson.insert(QStringLiteral("converged"), facts.poisson_regression->converged);
        poisson.insert(QStringLiteral("deviance"),
                       optional_number(facts.poisson_regression->deviance));
        poisson.insert(QStringLiteral("aic"), optional_number(facts.poisson_regression->aic));
        serialized.insert(QStringLiteral("poisson_regression"), poisson);
    }
    if (facts.isolation_forest.has_value()) {
        QJsonObject forest;
        forest.insert(QStringLiteral("n"), static_cast<int>(facts.isolation_forest->n));
        forest.insert(QStringLiteral("variable_count"),
                      static_cast<int>(facts.isolation_forest->variable_count));
        forest.insert(QStringLiteral("tree_count"),
                      static_cast<int>(facts.isolation_forest->tree_count));
        forest.insert(QStringLiteral("anomaly_count"),
                      static_cast<int>(facts.isolation_forest->anomaly_count));
        forest.insert(QStringLiteral("score_threshold"),
                      optional_number(facts.isolation_forest->score_threshold));
        serialized.insert(QStringLiteral("isolation_forest"), forest);
    }
    if (facts.bootstrap_mean.has_value()) {
        QJsonObject bootstrap;
        bootstrap.insert(QStringLiteral("n"), static_cast<int>(facts.bootstrap_mean->n));
        bootstrap.insert(QStringLiteral("replicates"),
                         static_cast<int>(facts.bootstrap_mean->replicates));
        bootstrap.insert(QStringLiteral("method"),
                         QString::fromStdString(facts.bootstrap_mean->method));
        bootstrap.insert(QStringLiteral("sample_mean"),
                         optional_number(facts.bootstrap_mean->sample_mean));
        bootstrap.insert(QStringLiteral("ci_lower"),
                         optional_number(facts.bootstrap_mean->ci_lower));
        bootstrap.insert(QStringLiteral("ci_upper"),
                         optional_number(facts.bootstrap_mean->ci_upper));
        bootstrap.insert(QStringLiteral("confidence_level"),
                         facts.bootstrap_mean->confidence_level);
        serialized.insert(QStringLiteral("bootstrap_mean"), bootstrap);
    }
    if (facts.bootstrap_two_sample.has_value()) {
        QJsonObject bootstrap;
        bootstrap.insert(QStringLiteral("n_first"),
                         static_cast<int>(facts.bootstrap_two_sample->n_first));
        bootstrap.insert(QStringLiteral("n_second"),
                         static_cast<int>(facts.bootstrap_two_sample->n_second));
        bootstrap.insert(QStringLiteral("replicates"),
                         static_cast<int>(facts.bootstrap_two_sample->replicates));
        bootstrap.insert(QStringLiteral("method"),
                         QString::fromStdString(facts.bootstrap_two_sample->method));
        bootstrap.insert(QStringLiteral("mean_first"),
                         optional_number(facts.bootstrap_two_sample->mean_first));
        bootstrap.insert(QStringLiteral("mean_second"),
                         optional_number(facts.bootstrap_two_sample->mean_second));
        bootstrap.insert(QStringLiteral("mean_difference"),
                         optional_number(facts.bootstrap_two_sample->mean_difference));
        bootstrap.insert(QStringLiteral("ci_lower"),
                         optional_number(facts.bootstrap_two_sample->ci_lower));
        bootstrap.insert(QStringLiteral("ci_upper"),
                         optional_number(facts.bootstrap_two_sample->ci_upper));
        bootstrap.insert(QStringLiteral("confidence_level"),
                         facts.bootstrap_two_sample->confidence_level);
        serialized.insert(QStringLiteral("bootstrap_two_sample"), bootstrap);
    }
    if (facts.probit_reliability.has_value()) {
        QJsonObject probit;
        probit.insert(QStringLiteral("n"), static_cast<int>(facts.probit_reliability->n));
        probit.insert(QStringLiteral("iteration_count"),
                      static_cast<int>(facts.probit_reliability->iteration_count));
        probit.insert(QStringLiteral("converged"), facts.probit_reliability->converged);
        probit.insert(QStringLiteral("link"),
                      QString::fromStdString(facts.probit_reliability->link));
        probit.insert(QStringLiteral("intercept"),
                      optional_number(facts.probit_reliability->intercept));
        probit.insert(QStringLiteral("stress_coefficient"),
                      optional_number(facts.probit_reliability->stress_coefficient));
        probit.insert(QStringLiteral("ld50"),
                      optional_number(facts.probit_reliability->ld50));
        probit.insert(QStringLiteral("ld50_standard_error"),
                      optional_number(facts.probit_reliability->ld50_standard_error));
        probit.insert(QStringLiteral("ld50_confidence_lower"),
                      optional_number(facts.probit_reliability->ld50_confidence_lower));
        probit.insert(QStringLiteral("ld50_confidence_upper"),
                      optional_number(facts.probit_reliability->ld50_confidence_upper));
        probit.insert(QStringLiteral("log_likelihood"),
                      optional_number(facts.probit_reliability->log_likelihood));
        probit.insert(QStringLiteral("deviance"),
                      optional_number(facts.probit_reliability->deviance));
        probit.insert(QStringLiteral("aic"),
                      optional_number(facts.probit_reliability->aic));
        serialized.insert(QStringLiteral("probit_reliability"), probit);
    }
    if (facts.hierarchical_cluster.has_value()) {
        QJsonObject hierarchical;
        hierarchical.insert(QStringLiteral("n"),
                            static_cast<int>(facts.hierarchical_cluster->n));
        hierarchical.insert(QStringLiteral("variable_count"),
                            static_cast<int>(facts.hierarchical_cluster->variable_count));
        hierarchical.insert(QStringLiteral("cluster_count"),
                            static_cast<int>(facts.hierarchical_cluster->cluster_count));
        hierarchical.insert(QStringLiteral("merge_count"),
                            static_cast<int>(facts.hierarchical_cluster->merge_count));
        hierarchical.insert(QStringLiteral("linkage"),
                            QString::fromStdString(facts.hierarchical_cluster->linkage));
        hierarchical.insert(QStringLiteral("standardized"),
                            facts.hierarchical_cluster->standardized);
        serialized.insert(QStringLiteral("hierarchical_cluster"), hierarchical);
    }
    if (facts.ordinal_logistic.has_value()) {
        QJsonObject ordinal;
        ordinal.insert(QStringLiteral("n"), static_cast<int>(facts.ordinal_logistic->n));
        ordinal.insert(QStringLiteral("category_count"),
                       static_cast<int>(facts.ordinal_logistic->category_count));
        ordinal.insert(QStringLiteral("predictor_count"),
                       static_cast<int>(facts.ordinal_logistic->predictor_count));
        ordinal.insert(QStringLiteral("iteration_count"),
                       static_cast<int>(facts.ordinal_logistic->iteration_count));
        ordinal.insert(QStringLiteral("converged"), facts.ordinal_logistic->converged);
        ordinal.insert(QStringLiteral("log_likelihood"),
                       optional_number(facts.ordinal_logistic->log_likelihood));
        ordinal.insert(QStringLiteral("aic"), optional_number(facts.ordinal_logistic->aic));
        serialized.insert(QStringLiteral("ordinal_logistic"), ordinal);
    }
    if (facts.discriminant.has_value()) {
        QJsonObject disc;
        disc.insert(QStringLiteral("n"), static_cast<int>(facts.discriminant->n));
        disc.insert(QStringLiteral("class_count"),
                    static_cast<int>(facts.discriminant->class_count));
        disc.insert(QStringLiteral("predictor_count"),
                    static_cast<int>(facts.discriminant->predictor_count));
        disc.insert(QStringLiteral("train_accuracy"),
                    optional_number(facts.discriminant->train_accuracy));
        serialized.insert(QStringLiteral("discriminant"), disc);
    }
    if (facts.ccf.has_value()) {
        QJsonObject ccf;
        ccf.insert(QStringLiteral("n"), static_cast<int>(facts.ccf->n));
        ccf.insert(QStringLiteral("missing_count"),
                   static_cast<int>(facts.ccf->missing_count));
        ccf.insert(QStringLiteral("max_lag"), static_cast<int>(facts.ccf->max_lag));
        ccf.insert(QStringLiteral("band_half_width"),
                   optional_number(facts.ccf->band_half_width));
        ccf.insert(QStringLiteral("ccf_at_zero"), optional_number(facts.ccf->ccf_at_zero));
        serialized.insert(QStringLiteral("ccf"), ccf);
    }
    if (facts.correlogram.has_value()) {
        QJsonObject correlogram;
        correlogram.insert(QStringLiteral("variable_count"),
                           static_cast<int>(facts.correlogram->variable_count));
        correlogram.insert(QStringLiteral("method"),
                           QString::fromStdString(facts.correlogram->method));
        correlogram.insert(QStringLiteral("pair_count"),
                           static_cast<int>(facts.correlogram->pair_count));
        serialized.insert(QStringLiteral("correlogram"), correlogram);
    }
    if (facts.stepwise_regression.has_value()) {
        QJsonObject stepwise;
        stepwise.insert(QStringLiteral("n"), static_cast<int>(facts.stepwise_regression->n));
        stepwise.insert(QStringLiteral("candidate_count"),
                        static_cast<int>(facts.stepwise_regression->candidate_count));
        stepwise.insert(QStringLiteral("selected_count"),
                        static_cast<int>(facts.stepwise_regression->selected_count));
        stepwise.insert(QStringLiteral("step_count"),
                        static_cast<int>(facts.stepwise_regression->step_count));
        stepwise.insert(QStringLiteral("method"),
                        QString::fromStdString(facts.stepwise_regression->method));
        stepwise.insert(QStringLiteral("criterion"),
                        QString::fromStdString(facts.stepwise_regression->criterion));
        stepwise.insert(QStringLiteral("best_step_index"),
                        static_cast<int>(facts.stepwise_regression->best_step_index));
        stepwise.insert(QStringLiteral("r_squared"),
                        optional_number(facts.stepwise_regression->r_squared));
        stepwise.insert(QStringLiteral("adjusted_r_squared"),
                        optional_number(facts.stepwise_regression->adjusted_r_squared));
        stepwise.insert(QStringLiteral("best_aicc"),
                        optional_number(facts.stepwise_regression->best_aicc));
        stepwise.insert(QStringLiteral("best_bic"),
                        optional_number(facts.stepwise_regression->best_bic));
        serialized.insert(QStringLiteral("stepwise_regression"), stepwise);
    }
    if (facts.nominal_logistic.has_value()) {
        QJsonObject nominal;
        nominal.insert(QStringLiteral("n"), static_cast<int>(facts.nominal_logistic->n));
        nominal.insert(QStringLiteral("category_count"),
                       static_cast<int>(facts.nominal_logistic->category_count));
        nominal.insert(QStringLiteral("logit_count"),
                       static_cast<int>(facts.nominal_logistic->logit_count));
        nominal.insert(QStringLiteral("converged"), facts.nominal_logistic->converged);
        nominal.insert(QStringLiteral("reference_category"),
                       QString::fromStdString(facts.nominal_logistic->reference_category));
        nominal.insert(QStringLiteral("log_likelihood"),
                       optional_number(facts.nominal_logistic->log_likelihood));
        nominal.insert(QStringLiteral("aic"), optional_number(facts.nominal_logistic->aic));
        nominal.insert(QStringLiteral("g_p_value"),
                       optional_number(facts.nominal_logistic->g_p_value));
        serialized.insert(QStringLiteral("nominal_logistic"), nominal);
    }
    if (facts.nonparametric_capability.has_value()) {
        QJsonObject nonparam;
        nonparam.insert(QStringLiteral("n"),
                        static_cast<int>(facts.nonparametric_capability->n));
        nonparam.insert(QStringLiteral("tolerance_k"),
                        facts.nonparametric_capability->tolerance_k);
        nonparam.insert(QStringLiteral("cnp"),
                        optional_number(facts.nonparametric_capability->cnp));
        nonparam.insert(QStringLiteral("cnpl"),
                        optional_number(facts.nonparametric_capability->cnpl));
        nonparam.insert(QStringLiteral("cnpu"),
                        optional_number(facts.nonparametric_capability->cnpu));
        nonparam.insert(QStringLiteral("cnpk"),
                        optional_number(facts.nonparametric_capability->cnpk));
        nonparam.insert(QStringLiteral("median"),
                        optional_number(facts.nonparametric_capability->median));
        nonparam.insert(QStringLiteral("lower_percentile"),
                        optional_number(facts.nonparametric_capability->lower_percentile));
        nonparam.insert(QStringLiteral("upper_percentile"),
                        optional_number(facts.nonparametric_capability->upper_percentile));
        nonparam.insert(QStringLiteral("observed_ppm_below"),
                        optional_number(facts.nonparametric_capability->observed_ppm_below));
        nonparam.insert(QStringLiteral("observed_ppm_above"),
                        optional_number(facts.nonparametric_capability->observed_ppm_above));
        nonparam.insert(QStringLiteral("observed_ppm_total"),
                        optional_number(facts.nonparametric_capability->observed_ppm_total));
        serialized.insert(QStringLiteral("nonparametric_capability"), nonparam);
    }
    if (facts.cox_regression.has_value()) {
        QJsonObject cox;
        cox.insert(QStringLiteral("n"), static_cast<int>(facts.cox_regression->n));
        cox.insert(QStringLiteral("events"), static_cast<int>(facts.cox_regression->events));
        cox.insert(QStringLiteral("censored"), static_cast<int>(facts.cox_regression->censored));
        cox.insert(QStringLiteral("converged"), facts.cox_regression->converged);
        cox.insert(QStringLiteral("log_likelihood"),
                   optional_number(facts.cox_regression->log_likelihood));
        cox.insert(QStringLiteral("evidence_type"),
                   QString::fromStdString(facts.cox_regression->evidence_type));
        cox.insert(QStringLiteral("algorithm_id"),
                   QString::fromStdString(facts.cox_regression->algorithm_id));
        cox.insert(QStringLiteral("ties_method"),
                   QString::fromStdString(facts.cox_regression->ties_method));
        QJsonArray coefficients;
        for (const auto& term : facts.cox_regression->coefficients) {
            QJsonObject item;
            item.insert(QStringLiteral("term"), QString::fromStdString(term.term));
            item.insert(QStringLiteral("beta"), optional_number(term.beta));
            item.insert(QStringLiteral("se"), optional_number(term.se));
            item.insert(QStringLiteral("z"), optional_number(term.z));
            item.insert(QStringLiteral("p_value"), optional_number(term.p_value));
            item.insert(QStringLiteral("hazard_ratio"), optional_number(term.hazard_ratio));
            item.insert(QStringLiteral("ci_lower"), optional_number(term.ci_lower));
            item.insert(QStringLiteral("ci_upper"), optional_number(term.ci_upper));
            coefficients.push_back(item);
        }
        cox.insert(QStringLiteral("coefficients"), coefficients);
        serialized.insert(QStringLiteral("cox_regression"), cox);
    }
    if (facts.accelerated_life.has_value()) {
        QJsonObject alt;
        alt.insert(QStringLiteral("n"), static_cast<int>(facts.accelerated_life->n));
        alt.insert(QStringLiteral("failure_count"),
                   static_cast<int>(facts.accelerated_life->failure_count));
        alt.insert(QStringLiteral("censored_count"),
                   static_cast<int>(facts.accelerated_life->censored_count));
        alt.insert(QStringLiteral("stress_level_count"),
                   static_cast<int>(facts.accelerated_life->stress_level_count));
        alt.insert(QStringLiteral("converged"), facts.accelerated_life->converged);
        alt.insert(QStringLiteral("transform"),
                   QString::fromStdString(facts.accelerated_life->transform));
        alt.insert(QStringLiteral("shape"),
                   optional_number(facts.accelerated_life->shape));
        alt.insert(QStringLiteral("log_likelihood"),
                   optional_number(facts.accelerated_life->log_likelihood));
        alt.insert(QStringLiteral("use_stress_celsius"),
                   optional_number(facts.accelerated_life->use_stress_celsius));
        alt.insert(QStringLiteral("b10_at_use_stress"),
                   optional_number(facts.accelerated_life->b10_at_use_stress));
        alt.insert(QStringLiteral("b50_at_use_stress"),
                   optional_number(facts.accelerated_life->b50_at_use_stress));
        alt.insert(QStringLiteral("b90_at_use_stress"),
                   optional_number(facts.accelerated_life->b90_at_use_stress));
        serialized.insert(QStringLiteral("accelerated_life"), alt);
    }
    if (facts.best_subsets_regression.has_value()) {
        QJsonObject best_subsets;
        best_subsets.insert(QStringLiteral("n"),
                            static_cast<int>(facts.best_subsets_regression->n));
        best_subsets.insert(QStringLiteral("candidate_count"),
                            static_cast<int>(facts.best_subsets_regression->candidate_count));
        best_subsets.insert(QStringLiteral("model_count"),
                            static_cast<int>(facts.best_subsets_regression->model_count));
        best_subsets.insert(QStringLiteral("models_per_size"),
                            static_cast<int>(facts.best_subsets_regression->models_per_size));
        best_subsets.insert(QStringLiteral("best_r_squared"),
                            optional_number(facts.best_subsets_regression->best_r_squared));
        best_subsets.insert(
            QStringLiteral("best_adjusted_r_squared"),
            optional_number(facts.best_subsets_regression->best_adjusted_r_squared));
        if (facts.best_subsets_regression->best_predictor_count.has_value()) {
            best_subsets.insert(QStringLiteral("best_predictor_count"),
                                static_cast<int>(
                                    *facts.best_subsets_regression->best_predictor_count));
        }
        serialized.insert(QStringLiteral("best_subsets_regression"), best_subsets);
    }
    if (facts.batch_capability.has_value()) {
        QJsonObject batch_capability;
        batch_capability.insert(QStringLiteral("batch_count"),
                                static_cast<int>(facts.batch_capability->batch_count));
        batch_capability.insert(QStringLiteral("skipped_batch_count"),
                                static_cast<int>(facts.batch_capability->skipped_batch_count));
        batch_capability.insert(QStringLiteral("total_observations"),
                                static_cast<int>(facts.batch_capability->total_observations));
        serialized.insert(QStringLiteral("batch_capability"), batch_capability);
    }
    if (facts.km_interval.has_value()) {
        QJsonObject km;
        km.insert(QStringLiteral("n"), static_cast<int>(facts.km_interval->n));
        km.insert(QStringLiteral("exact_count"),
                  static_cast<int>(facts.km_interval->exact_count));
        km.insert(QStringLiteral("left_censored_count"),
                  static_cast<int>(facts.km_interval->left_censored_count));
        km.insert(QStringLiteral("right_censored_count"),
                  static_cast<int>(facts.km_interval->right_censored_count));
        km.insert(QStringLiteral("interval_censored_count"),
                  static_cast<int>(facts.km_interval->interval_censored_count));
        km.insert(QStringLiteral("iteration_count"),
                  static_cast<int>(facts.km_interval->iteration_count));
        km.insert(QStringLiteral("converged"), facts.km_interval->converged);
        km.insert(QStringLiteral("identifiable"), facts.km_interval->identifiable);
        km.insert(QStringLiteral("median_life"),
                  optional_number(facts.km_interval->median_life));
        km.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.km_interval->evidence_type));
        km.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.km_interval->algorithm_id));
        km.insert(QStringLiteral("gate_status"),
                  QString::fromStdString(facts.km_interval->gate_status));
        km.insert(QStringLiteral("research_preview"),
                  facts.km_interval->research_preview);
        km.insert(QStringLiteral("classic_km_equivalent"),
                  facts.km_interval->classic_km_equivalent);
        serialized.insert(QStringLiteral("km_interval"), km);
    }
    if (facts.plackett_burman.has_value()) {
        QJsonObject pb;
        pb.insert(QStringLiteral("factor_count"),
                  static_cast<int>(facts.plackett_burman->factor_count));
        pb.insert(QStringLiteral("run_count"),
                  static_cast<int>(facts.plackett_burman->run_count));
        pb.insert(QStringLiteral("center_point_count"),
                  static_cast<int>(facts.plackett_burman->center_point_count));
        serialized.insert(QStringLiteral("plackett_burman"), pb);
    }
    if (facts.random_forest.has_value()) {
        QJsonObject rf;
        rf.insert(QStringLiteral("task"), QString::fromStdString(facts.random_forest->task));
        rf.insert(QStringLiteral("n"), static_cast<int>(facts.random_forest->n));
        rf.insert(QStringLiteral("predictor_count"),
                  static_cast<int>(facts.random_forest->predictor_count));
        rf.insert(QStringLiteral("n_trees"),
                  static_cast<int>(facts.random_forest->n_trees));
        rf.insert(QStringLiteral("max_depth"),
                  static_cast<int>(facts.random_forest->max_depth));
        rf.insert(QStringLiteral("train_metric"),
                  optional_number(facts.random_forest->train_metric));
        rf.insert(QStringLiteral("oob_metric"),
                  optional_number(facts.random_forest->oob_metric));
        rf.insert(QStringLiteral("top_variable"),
                  QString::fromStdString(facts.random_forest->top_variable));
        rf.insert(QStringLiteral("disclosure"),
                  QString::fromStdString(facts.random_forest->disclosure));
        rf.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.random_forest->evidence_type));
        rf.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.random_forest->algorithm_id));
        serialized.insert(QStringLiteral("random_forest"), rf);
    }
    if (facts.weibayes.has_value()) {
        QJsonObject wb;
        wb.insert(QStringLiteral("n"), static_cast<int>(facts.weibayes->n));
        wb.insert(QStringLiteral("failure_count"),
                  static_cast<int>(facts.weibayes->failure_count));
        wb.insert(QStringLiteral("censored_count"),
                  static_cast<int>(facts.weibayes->censored_count));
        wb.insert(QStringLiteral("shape_prior"), facts.weibayes->shape_prior);
        wb.insert(QStringLiteral("scale"), optional_number(facts.weibayes->scale));
        wb.insert(QStringLiteral("zero_failure_bound"), facts.weibayes->zero_failure_bound);
        wb.insert(QStringLiteral("b10"), optional_number(facts.weibayes->b10));
        wb.insert(QStringLiteral("b50"), optional_number(facts.weibayes->b50));
        wb.insert(QStringLiteral("b90"), optional_number(facts.weibayes->b90));
        wb.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.weibayes->evidence_type));
        wb.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.weibayes->algorithm_id));
        serialized.insert(QStringLiteral("weibayes"), wb);
    }
    if (facts.taguchi_orthogonal.has_value()) {
        QJsonObject tg;
        tg.insert(QStringLiteral("array"),
                  QString::fromStdString(facts.taguchi_orthogonal->array));
        tg.insert(QStringLiteral("factor_count"),
                  static_cast<int>(facts.taguchi_orthogonal->factor_count));
        tg.insert(QStringLiteral("run_count"),
                  static_cast<int>(facts.taguchi_orthogonal->run_count));
        tg.insert(QStringLiteral("levels_per_factor"),
                  static_cast<int>(facts.taguchi_orthogonal->levels_per_factor));
        tg.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.taguchi_orthogonal->evidence_type));
        tg.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.taguchi_orthogonal->algorithm_id));
        serialized.insert(QStringLiteral("taguchi_orthogonal"), tg);
    }
    if (facts.distribution_calculator.has_value()) {
        QJsonObject dc;
        dc.insert(QStringLiteral("distribution"),
                  QString::fromStdString(facts.distribution_calculator->distribution));
        dc.insert(QStringLiteral("operation"),
                  QString::fromStdString(facts.distribution_calculator->operation));
        dc.insert(QStringLiteral("param1"), facts.distribution_calculator->param1);
        dc.insert(QStringLiteral("param2"), facts.distribution_calculator->param2);
        dc.insert(QStringLiteral("param3"), facts.distribution_calculator->param3);
        dc.insert(QStringLiteral("value"), facts.distribution_calculator->value);
        dc.insert(QStringLiteral("result"),
                  optional_number(facts.distribution_calculator->result));
        dc.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.distribution_calculator->evidence_type));
        dc.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.distribution_calculator->algorithm_id));
        serialized.insert(QStringLiteral("distribution_calculator"), dc);
    }
    if (facts.taguchi_analyze.has_value()) {
        QJsonObject ta;
        ta.insert(QStringLiteral("sn_type"),
                  QString::fromStdString(facts.taguchi_analyze->sn_type));
        ta.insert(QStringLiteral("factor_count"),
                  static_cast<int>(facts.taguchi_analyze->factor_count));
        ta.insert(QStringLiteral("response_count"),
                  static_cast<int>(facts.taguchi_analyze->response_count));
        ta.insert(QStringLiteral("run_count"),
                  static_cast<int>(facts.taguchi_analyze->run_count));
        ta.insert(QStringLiteral("top_delta"),
                  optional_number(facts.taguchi_analyze->top_delta));
        ta.insert(QStringLiteral("top_factor"),
                  QString::fromStdString(facts.taguchi_analyze->top_factor));
        ta.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.taguchi_analyze->evidence_type));
        ta.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.taguchi_analyze->algorithm_id));
        serialized.insert(QStringLiteral("taguchi_analyze"), ta);
    }
    if (facts.mixture_design.has_value()) {
        QJsonObject md;
        md.insert(QStringLiteral("component_count"),
                  static_cast<int>(facts.mixture_design->component_count));
        md.insert(QStringLiteral("degree"),
                  static_cast<int>(facts.mixture_design->degree));
        md.insert(QStringLiteral("run_count"),
                  static_cast<int>(facts.mixture_design->run_count));
        md.insert(QStringLiteral("design_kind"),
                  QString::fromStdString(facts.mixture_design->design_kind));
        md.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.mixture_design->evidence_type));
        md.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.mixture_design->algorithm_id));
        serialized.insert(QStringLiteral("mixture_design"), md);
    }
    if (facts.nhpp_repairable.has_value()) {
        QJsonObject nh;
        nh.insert(QStringLiteral("failure_count"),
                  static_cast<int>(facts.nhpp_repairable->failure_count));
        nh.insert(QStringLiteral("truncation_time"), facts.nhpp_repairable->truncation_time);
        nh.insert(QStringLiteral("beta"), optional_number(facts.nhpp_repairable->beta));
        nh.insert(QStringLiteral("lambda"), optional_number(facts.nhpp_repairable->lambda));
        nh.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.nhpp_repairable->evidence_type));
        nh.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.nhpp_repairable->algorithm_id));
        serialized.insert(QStringLiteral("nhpp_repairable"), nh);
    }
    if (facts.reliability_test_plan.has_value()) {
        QJsonObject rtp;
        rtp.insert(QStringLiteral("shape_beta"), facts.reliability_test_plan->shape_beta);
        rtp.insert(QStringLiteral("target_reliability"),
                   facts.reliability_test_plan->target_reliability);
        rtp.insert(QStringLiteral("confidence_level"),
                   facts.reliability_test_plan->confidence_level);
        rtp.insert(QStringLiteral("test_time"), facts.reliability_test_plan->test_time);
        rtp.insert(QStringLiteral("mission_time"),
                   facts.reliability_test_plan->mission_time);
        rtp.insert(QStringLiteral("time_ratio_delta"),
                   facts.reliability_test_plan->time_ratio_delta);
        rtp.insert(QStringLiteral("allowed_failures"),
                   static_cast<int>(facts.reliability_test_plan->allowed_failures));
        if (facts.reliability_test_plan->sample_size.has_value()) {
            rtp.insert(QStringLiteral("sample_size"),
                       static_cast<int>(*facts.reliability_test_plan->sample_size));
        } else {
            rtp.insert(QStringLiteral("sample_size"), QJsonValue());
        }
        rtp.insert(QStringLiteral("evidence_type"),
                   QString::fromStdString(facts.reliability_test_plan->evidence_type));
        rtp.insert(QStringLiteral("algorithm_id"),
                   QString::fromStdString(facts.reliability_test_plan->algorithm_id));
        serialized.insert(QStringLiteral("reliability_test_plan"), rtp);
    }
    if (facts.mixture_analyze.has_value()) {
        QJsonObject ma;
        ma.insert(QStringLiteral("component_count"),
                  static_cast<int>(facts.mixture_analyze->component_count));
        ma.insert(QStringLiteral("observation_count"),
                  static_cast<int>(facts.mixture_analyze->observation_count));
        ma.insert(QStringLiteral("model_order"),
                  QString::fromStdString(facts.mixture_analyze->model_order));
        ma.insert(QStringLiteral("r_squared"),
                  optional_number(facts.mixture_analyze->r_squared));
        ma.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.mixture_analyze->evidence_type));
        ma.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.mixture_analyze->algorithm_id));
        serialized.insert(QStringLiteral("mixture_analyze"), ma);
    }
    if (facts.glm_two_way.has_value()) {
        QJsonObject glm;
        glm.insert(QStringLiteral("observation_count"),
                   static_cast<int>(facts.glm_two_way->observation_count));
        glm.insert(QStringLiteral("include_interaction"),
                   facts.glm_two_way->include_interaction);
        glm.insert(QStringLiteral("design_balanced"),
                   facts.glm_two_way->design_balanced);
        glm.insert(QStringLiteral("residual_normality_p"),
                   optional_number(facts.glm_two_way->residual_normality_p));
        glm.insert(QStringLiteral("evidence_type"),
                   QString::fromStdString(facts.glm_two_way->evidence_type));
        glm.insert(QStringLiteral("algorithm_id"),
                   QString::fromStdString(facts.glm_two_way->algorithm_id));
        serialized.insert(QStringLiteral("glm_two_way"), glm);
    }
    if (facts.analyze_variability.has_value()) {
        QJsonObject av;
        av.insert(QStringLiteral("run_count"),
                  static_cast<int>(facts.analyze_variability->run_count));
        av.insert(QStringLiteral("factor_count"),
                  static_cast<int>(facts.analyze_variability->factor_count));
        av.insert(QStringLiteral("replicate_count"),
                  static_cast<int>(facts.analyze_variability->replicate_count));
        av.insert(QStringLiteral("estimation_method"),
                  QString::fromStdString(facts.analyze_variability->estimation_method));
        av.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.analyze_variability->evidence_type));
        av.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.analyze_variability->algorithm_id));
        serialized.insert(QStringLiteral("analyze_variability"), av);
    }
    if (facts.factor_analysis.has_value()) {
        QJsonObject fa;
        fa.insert(QStringLiteral("observation_count"),
                  static_cast<int>(facts.factor_analysis->observation_count));
        fa.insert(QStringLiteral("variable_count"),
                  static_cast<int>(facts.factor_analysis->variable_count));
        fa.insert(QStringLiteral("retained_factor_count"),
                  static_cast<int>(facts.factor_analysis->retained_factor_count));
        fa.insert(QStringLiteral("varimax_applied"),
                  facts.factor_analysis->varimax_applied);
        fa.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.factor_analysis->evidence_type));
        fa.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.factor_analysis->algorithm_id));
        serialized.insert(QStringLiteral("factor_analysis"), fa);
    }
    if (facts.binary_response_doe.has_value()) {
        QJsonObject br;
        br.insert(QStringLiteral("design_row_count"),
                  static_cast<int>(facts.binary_response_doe->design_row_count));
        br.insert(QStringLiteral("expanded_observation_count"),
                  static_cast<int>(facts.binary_response_doe->expanded_observation_count));
        br.insert(QStringLiteral("factor_count"),
                  static_cast<int>(facts.binary_response_doe->factor_count));
        br.insert(QStringLiteral("event_count"),
                  static_cast<int>(facts.binary_response_doe->event_count));
        br.insert(QStringLiteral("trial_count"),
                  static_cast<int>(facts.binary_response_doe->trial_count));
        br.insert(QStringLiteral("include_ab_interaction"),
                  facts.binary_response_doe->include_ab_interaction);
        br.insert(QStringLiteral("converged"), facts.binary_response_doe->converged);
        br.insert(QStringLiteral("deviance"), facts.binary_response_doe->deviance);
        br.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.binary_response_doe->evidence_type));
        br.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.binary_response_doe->algorithm_id));
        serialized.insert(QStringLiteral("binary_response_doe"), br);
    }
    if (facts.cluster_variables.has_value()) {
        QJsonObject cv;
        cv.insert(QStringLiteral("observation_count"),
                  static_cast<int>(facts.cluster_variables->observation_count));
        cv.insert(QStringLiteral("variable_count"),
                  static_cast<int>(facts.cluster_variables->variable_count));
        cv.insert(QStringLiteral("merge_count"),
                  static_cast<int>(facts.cluster_variables->merge_count));
        cv.insert(QStringLiteral("linkage"),
                  QString::fromStdString(facts.cluster_variables->linkage));
        cv.insert(QStringLiteral("max_distance"), facts.cluster_variables->max_distance);
        cv.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.cluster_variables->evidence_type));
        cv.insert(QStringLiteral("algorithm_id"),
                  QString::fromStdString(facts.cluster_variables->algorithm_id));
        serialized.insert(QStringLiteral("cluster_variables"), cv);
    }
    if (facts.glm_three_factor.has_value()) {
        QJsonObject glm3;
        glm3.insert(QStringLiteral("observation_count"),
                    static_cast<int>(facts.glm_three_factor->observation_count));
        glm3.insert(QStringLiteral("include_ab_interaction"),
                    facts.glm_three_factor->include_ab_interaction);
        glm3.insert(QStringLiteral("include_ac_interaction"),
                    facts.glm_three_factor->include_ac_interaction);
        glm3.insert(QStringLiteral("include_bc_interaction"),
                    facts.glm_three_factor->include_bc_interaction);
        glm3.insert(QStringLiteral("design_balanced"),
                    facts.glm_three_factor->design_balanced);
        glm3.insert(QStringLiteral("residual_normality_p"),
                    optional_number(facts.glm_three_factor->residual_normality_p));
        glm3.insert(QStringLiteral("evidence_type"),
                    QString::fromStdString(facts.glm_three_factor->evidence_type));
        glm3.insert(QStringLiteral("algorithm_id"),
                    QString::fromStdString(facts.glm_three_factor->algorithm_id));
        serialized.insert(QStringLiteral("glm_three_factor"), glm3);
    }
    if (facts.life_data_regression.has_value()) {
        QJsonObject ldr;
        ldr.insert(QStringLiteral("observation_count"),
                   static_cast<int>(facts.life_data_regression->observation_count));
        ldr.insert(QStringLiteral("failure_count"),
                   static_cast<int>(facts.life_data_regression->failure_count));
        ldr.insert(QStringLiteral("censored_count"),
                   static_cast<int>(facts.life_data_regression->censored_count));
        ldr.insert(QStringLiteral("covariate_count"),
                   static_cast<int>(facts.life_data_regression->covariate_count));
        ldr.insert(QStringLiteral("converged"), facts.life_data_regression->converged);
        ldr.insert(QStringLiteral("shape"), facts.life_data_regression->shape);
        ldr.insert(QStringLiteral("distribution"),
                   QString::fromStdString(facts.life_data_regression->distribution));
        ldr.insert(QStringLiteral("evidence_type"),
                   QString::fromStdString(facts.life_data_regression->evidence_type));
        ldr.insert(QStringLiteral("algorithm_id"),
                   QString::fromStdString(facts.life_data_regression->algorithm_id));
        serialized.insert(QStringLiteral("life_data_regression"), ldr);
    }
    if (facts.design_generation.has_value()) {
        QJsonObject dg;
        dg.insert(QStringLiteral("design_kind"),
                  QString::fromStdString(facts.design_generation->design_kind));
        dg.insert(QStringLiteral("ccd_variant"),
                  QString::fromStdString(facts.design_generation->ccd_variant));
        dg.insert(QStringLiteral("design_source_id"),
                  QString::fromStdString(facts.design_generation->design_source_id));
        dg.insert(QStringLiteral("factor_count"),
                  static_cast<int>(facts.design_generation->factor_count));
        dg.insert(QStringLiteral("run_count"),
                  static_cast<int>(facts.design_generation->run_count));
        dg.insert(QStringLiteral("cube_count"),
                  static_cast<int>(facts.design_generation->cube_count));
        dg.insert(QStringLiteral("star_count"),
                  static_cast<int>(facts.design_generation->star_count));
        dg.insert(QStringLiteral("edge_count"),
                  static_cast<int>(facts.design_generation->edge_count));
        dg.insert(QStringLiteral("center_count"),
                  static_cast<int>(facts.design_generation->center_count));
        dg.insert(QStringLiteral("alpha"), facts.design_generation->alpha);
        dg.insert(QStringLiteral("allow_beyond_range"),
                  facts.design_generation->allow_beyond_range);
        dg.insert(QStringLiteral("beyond_range_detected"),
                  facts.design_generation->beyond_range_detected);
        dg.insert(QStringLiteral("randomized"), facts.design_generation->randomized);
        dg.insert(QStringLiteral("random_seed"),
                  static_cast<double>(facts.design_generation->random_seed));
        dg.insert(QStringLiteral("evidence_type"),
                  QString::fromStdString(facts.design_generation->evidence_type));
        serialized.insert(QStringLiteral("design_generation"), dg);
    }
    if (facts.multi_vari.has_value()) {
        QJsonObject multi_vari;
        multi_vari.insert(QStringLiteral("factor_count"),
                          static_cast<int>(facts.multi_vari->factor_count));
        multi_vari.insert(QStringLiteral("valid_count"),
                          static_cast<int>(facts.multi_vari->valid_count));
        multi_vari.insert(QStringLiteral("missing_count"),
                          static_cast<int>(facts.multi_vari->missing_count));
        multi_vari.insert(QStringLiteral("combination_coverage"),
                          facts.multi_vari->combination_coverage);
        multi_vari.insert(QStringLiteral("factor_names"),
                          string_array(facts.multi_vari->factor_names));
        serialized.insert(QStringLiteral("multi_vari"), multi_vari);
    }
    if (facts.variability.has_value()) {
        QJsonObject variability;
        variability.insert(QStringLiteral("factor_count"),
                           static_cast<int>(facts.variability->factor_count));
        variability.insert(QStringLiteral("valid_count"),
                           static_cast<int>(facts.variability->valid_count));
        variability.insert(QStringLiteral("missing_count"),
                           static_cast<int>(facts.variability->missing_count));
        variability.insert(QStringLiteral("cell_count"),
                           static_cast<int>(facts.variability->cell_count));
        variability.insert(QStringLiteral("overall_mean"),
                           optional_number(facts.variability->overall_mean));
        variability.insert(QStringLiteral("mean_of_cell_sds"),
                           optional_number(facts.variability->mean_of_cell_sds));
        variability.insert(QStringLiteral("factor_names"),
                           string_array(facts.variability->factor_names));
        serialized.insert(QStringLiteral("variability"), variability);
    }
    if (facts.tolerance.has_value()) {
        QJsonObject tolerance;
        tolerance.insert(QStringLiteral("valid_count"),
                         static_cast<int>(facts.tolerance->valid_count));
        tolerance.insert(QStringLiteral("missing_count"),
                         static_cast<int>(facts.tolerance->missing_count));
        tolerance.insert(QStringLiteral("mean"),
                         optional_number(facts.tolerance->mean));
        tolerance.insert(QStringLiteral("standard_deviation"),
                         optional_number(facts.tolerance->standard_deviation));
        tolerance.insert(QStringLiteral("coverage"),
                         optional_number(facts.tolerance->coverage));
        tolerance.insert(QStringLiteral("confidence_level"),
                         optional_number(facts.tolerance->confidence_level));
        tolerance.insert(QStringLiteral("lower"),
                         optional_number(facts.tolerance->lower));
        tolerance.insert(QStringLiteral("upper"),
                         optional_number(facts.tolerance->upper));
        tolerance.insert(QStringLiteral("k_factor"),
                         optional_number(facts.tolerance->k_factor));
        tolerance.insert(QStringLiteral("achieved_confidence"),
                         optional_number(facts.tolerance->achieved_confidence));
        tolerance.insert(QStringLiteral("method"),
                         QString::fromStdString(facts.tolerance->method));
        tolerance.insert(QStringLiteral("method_family"),
                         QString::fromStdString(facts.tolerance->method_family));
        tolerance.insert(QStringLiteral("interval_type"),
                         QString::fromStdString(facts.tolerance->interval_type));
        tolerance.insert(QStringLiteral("assumption_status"),
                         QString::fromStdString(facts.tolerance->assumption_status));
        serialized.insert(QStringLiteral("tolerance"), tolerance);
    }
    if (facts.variance.has_value()) {
        QJsonObject variance;
        variance.insert(QStringLiteral("method"),
                        QString::fromStdString(facts.variance->method));
        variance.insert(QStringLiteral("statistic"),
                        optional_number(facts.variance->statistic));
        variance.insert(QStringLiteral("p_value"),
                        optional_number(facts.variance->p_value));
        variance.insert(QStringLiteral("ci_lower"),
                        optional_number(facts.variance->ci_lower));
        variance.insert(QStringLiteral("ci_upper"),
                        optional_number(facts.variance->ci_upper));
        variance.insert(QStringLiteral("group_count"),
                        static_cast<int>(facts.variance->group_count));
        serialized.insert(QStringLiteral("variance"), variance);
    }
    if (facts.doe.has_value()) {
        QJsonObject doe;
        doe.insert(QStringLiteral("significant_terms"),
                   string_array(facts.doe->significant_terms));
        doe.insert(QStringLiteral("has_p_value"), facts.doe->has_p_value);
        doe.insert(QStringLiteral("response_count"),
                   static_cast<int>(facts.doe->response_count));
        doe.insert(QStringLiteral("multi_response"), facts.doe->multi_response);
        doe.insert(QStringLiteral("best_overall_desirability"),
                   optional_number(facts.doe->best_overall_desirability));
        doe.insert(QStringLiteral("response_names"),
                   string_array(facts.doe->response_names));
        doe.insert(QStringLiteral("prediction_interval_available"),
                   facts.doe->prediction_interval_available);
        doe.insert(QStringLiteral("largest_standardized_effect_term"),
                   QString::fromStdString(facts.doe->largest_standardized_effect_term));
        doe.insert(QStringLiteral("pareto_reference"),
                   optional_number(facts.doe->pareto_reference));
        doe.insert(QStringLiteral("pareto_method"),
                   QString::fromStdString(facts.doe->pareto_method));
        doe.insert(QStringLiteral("residual_count"),
                   static_cast<int>(facts.doe->residual_count));
        doe.insert(QStringLiteral("factor_count"),
                   static_cast<int>(facts.doe->factor_count));
        doe.insert(QStringLiteral("cube_plot_available"), facts.doe->cube_plot_available);
        doe.insert(QStringLiteral("contour_plot_available"),
                   facts.doe->contour_plot_available);
        doe.insert(QStringLiteral("contour_x_factor"),
                   QString::fromStdString(facts.doe->contour_x_factor));
        doe.insert(QStringLiteral("contour_y_factor"),
                   QString::fromStdString(facts.doe->contour_y_factor));
        doe.insert(QStringLiteral("held_factor_names"),
                   string_array(facts.doe->held_factor_names));
        doe.insert(QStringLiteral("held_actual_values"),
                   string_array(facts.doe->held_actual_values));
        doe.insert(QStringLiteral("held_coded_values"),
                   number_array(facts.doe->held_coded_values));
        doe.insert(QStringLiteral("design_kind"),
                   QString::fromStdString(facts.doe->design_kind));
        doe.insert(QStringLiteral("fraction_p"),
                   static_cast<int>(facts.doe->fraction_p));
        doe.insert(QStringLiteral("resolution"), facts.doe->resolution);
        doe.insert(QStringLiteral("run_count"),
                   static_cast<int>(facts.doe->run_count));
        doe.insert(QStringLiteral("generator_text"),
                   QString::fromStdString(facts.doe->generator_text));
        serialized.insert(QStringLiteral("doe"), doe);
    }
    if (facts.rsm.has_value()) {
        QJsonObject rsm;
        rsm.insert(QStringLiteral("factor_count"),
                   static_cast<int>(facts.rsm->factor_count));
        rsm.insert(QStringLiteral("term_count"),
                   static_cast<int>(facts.rsm->term_count));
        rsm.insert(QStringLiteral("residual_count"),
                   static_cast<int>(facts.rsm->residual_count));
        rsm.insert(QStringLiteral("r_squared"), optional_number(facts.rsm->r_squared));
        rsm.insert(QStringLiteral("adjusted_r_squared"),
                   optional_number(facts.rsm->adjusted_r_squared));
        rsm.insert(QStringLiteral("contour_plot_available"),
                   facts.rsm->contour_plot_available);
        rsm.insert(QStringLiteral("largest_abs_t_term"),
                   QString::fromStdString(facts.rsm->largest_abs_t_term));
        rsm.insert(QStringLiteral("response_name"),
                   QString::fromStdString(facts.rsm->response_name));
        rsm.insert(QStringLiteral("design_source_id"),
                   QString::fromStdString(facts.rsm->design_source_id));
        rsm.insert(QStringLiteral("design_kind"),
                   QString::fromStdString(facts.rsm->design_kind));
        rsm.insert(QStringLiteral("coding_mode"),
                   QString::fromStdString(facts.rsm->coding_mode));
        rsm.insert(QStringLiteral("center_point_count"),
                   static_cast<int>(facts.rsm->center_point_count));
        rsm.insert(QStringLiteral("surface_is_static"), facts.rsm->surface_is_static);
        rsm.insert(QStringLiteral("evidence_type"),
                   QString::fromStdString(facts.rsm->evidence_type));
        rsm.insert(QStringLiteral("pure_error_available"),
                   facts.rsm->pure_error_available);
        rsm.insert(QStringLiteral("lack_of_fit_available"),
                   facts.rsm->lack_of_fit_available);
        rsm.insert(QStringLiteral("pure_error_df"),
                   static_cast<int>(facts.rsm->pure_error_df));
        rsm.insert(QStringLiteral("lack_of_fit_df"),
                   static_cast<int>(facts.rsm->lack_of_fit_df));
        rsm.insert(QStringLiteral("lack_of_fit_f"),
                   optional_number(facts.rsm->lack_of_fit_f));
        rsm.insert(QStringLiteral("lack_of_fit_p"),
                   optional_number(facts.rsm->lack_of_fit_p));
        serialized.insert(QStringLiteral("rsm"), rsm);
    }
    if (facts.acf_pacf.has_value()) {
        QJsonObject acf;
        acf.insert(QStringLiteral("n"), static_cast<int>(facts.acf_pacf->n));
        acf.insert(QStringLiteral("missing_count"),
                   static_cast<int>(facts.acf_pacf->missing_count));
        acf.insert(QStringLiteral("max_lag"),
                   static_cast<int>(facts.acf_pacf->max_lag));
        acf.insert(QStringLiteral("confidence_band_method"),
                   QString::fromStdString(facts.acf_pacf->confidence_band_method));
        acf.insert(QStringLiteral("band_half_width"),
                   optional_number(facts.acf_pacf->band_half_width));
        acf.insert(QStringLiteral("alpha"), optional_number(facts.acf_pacf->alpha));
        acf.insert(QStringLiteral("ljung_box_available"),
                   facts.acf_pacf->ljung_box_available);
        acf.insert(QStringLiteral("ljung_box_statistic"),
                   optional_number(facts.acf_pacf->ljung_box_statistic));
        acf.insert(QStringLiteral("ljung_box_p_value"),
                   optional_number(facts.acf_pacf->ljung_box_p_value));
        serialized.insert(QStringLiteral("acf_pacf"), acf);
    }
    if (facts.eda.has_value()) {
        QJsonObject eda;
        eda.insert(QStringLiteral("kind"), QString::fromStdString(facts.eda->kind));
        eda.insert(QStringLiteral("n"), static_cast<int>(facts.eda->n));
        eda.insert(QStringLiteral("bandwidth"), optional_number(facts.eda->bandwidth));
        eda.insert(QStringLiteral("category_count"),
                   static_cast<int>(facts.eda->category_count));
        eda.insert(QStringLiteral("x_bins"), static_cast<int>(facts.eda->x_bins));
        eda.insert(QStringLiteral("y_bins"), static_cast<int>(facts.eda->y_bins));
        eda.insert(QStringLiteral("sorted_by_count"), facts.eda->sorted_by_count);
        eda.insert(QStringLiteral("has_cumulative_percent"),
                   facts.eda->has_cumulative_percent);
        eda.insert(QStringLiteral("hidden_count"),
                   static_cast<int>(facts.eda->hidden_count));
        eda.insert(QStringLiteral("excluded_count"),
                   static_cast<int>(facts.eda->excluded_count));
        eda.insert(QStringLiteral("analysis_eligible_n"),
                   static_cast<int>(facts.eda->analysis_eligible_n));
        eda.insert(QStringLiteral("display_eligible_n"),
                   static_cast<int>(facts.eda->display_eligible_n));
        eda.insert(QStringLiteral("hidden_excluded_distinct"),
                   facts.eda->hidden_excluded_distinct);
        eda.insert(QStringLiteral("analysis_n"), static_cast<int>(facts.eda->analysis_n));
        eda.insert(QStringLiteral("analysis_category_count"),
                   static_cast<int>(facts.eda->analysis_category_count));
        eda.insert(QStringLiteral("facet_enabled"), facts.eda->facet_enabled);
        eda.insert(QStringLiteral("facet_panel_count"),
                   static_cast<int>(facts.eda->facet_panel_count));
        eda.insert(QStringLiteral("facet_level_count"),
                   static_cast<int>(facts.eda->facet_level_count));
        eda.insert(QStringLiteral("facet_truncated_levels"),
                   static_cast<int>(facts.eda->facet_truncated_levels));
        eda.insert(QStringLiteral("facet_max_panels"), facts.eda->facet_max_panels);
        serialized.insert(QStringLiteral("eda"), eda);
    }
    if (facts.proportion.has_value()) {
        QJsonObject proportion;
        proportion.insert(QStringLiteral("kind"),
                          QString::fromStdString(facts.proportion->kind));
        proportion.insert(QStringLiteral("events"),
                          static_cast<int>(facts.proportion->events));
        proportion.insert(QStringLiteral("trials"),
                          static_cast<int>(facts.proportion->trials));
        proportion.insert(QStringLiteral("proportion"),
                          optional_number(facts.proportion->proportion));
        if (facts.proportion->second_events.has_value()) {
            proportion.insert(QStringLiteral("second_events"),
                              static_cast<int>(*facts.proportion->second_events));
        }
        if (facts.proportion->second_trials.has_value()) {
            proportion.insert(QStringLiteral("second_trials"),
                              static_cast<int>(*facts.proportion->second_trials));
        }
        proportion.insert(QStringLiteral("second_proportion"),
                          optional_number(facts.proportion->second_proportion));
        proportion.insert(QStringLiteral("difference"),
                          optional_number(facts.proportion->difference));
        proportion.insert(QStringLiteral("hypothesized"),
                          optional_number(facts.proportion->hypothesized));
        proportion.insert(QStringLiteral("method"),
                          QString::fromStdString(facts.proportion->method));
        proportion.insert(QStringLiteral("ci_method"),
                          QString::fromStdString(facts.proportion->ci_method));
        proportion.insert(QStringLiteral("p_value"),
                          optional_number(facts.proportion->p_value));
        proportion.insert(QStringLiteral("fisher_p_value"),
                          optional_number(facts.proportion->fisher_p_value));
        proportion.insert(QStringLiteral("ci_lower"),
                          optional_number(facts.proportion->ci_lower));
        proportion.insert(QStringLiteral("ci_upper"),
                          optional_number(facts.proportion->ci_upper));
        proportion.insert(QStringLiteral("assumption_status"),
                          QString::fromStdString(facts.proportion->assumption_status));
        serialized.insert(QStringLiteral("proportion"), proportion);
    }
    if (facts.box_cox.has_value()) {
        QJsonObject box_cox;
        box_cox.insert(QStringLiteral("lambda"), facts.box_cox->lambda);
        box_cox.insert(QStringLiteral("n"), static_cast<int>(facts.box_cox->n));
        box_cox.insert(QStringLiteral("missing_count"),
                       static_cast<int>(facts.box_cox->missing_count));
        box_cox.insert(QStringLiteral("transformed_standard_deviation"),
                       optional_number(facts.box_cox->transformed_standard_deviation));
        box_cox.insert(QStringLiteral("rounded_lambda"), facts.box_cox->rounded_lambda);
        box_cox.insert(QStringLiteral("assumption_status"),
                       QString::fromStdString(facts.box_cox->assumption_status));
        serialized.insert(QStringLiteral("box_cox"), box_cox);
    }
    if (facts.poisson_rate.has_value()) {
        QJsonObject rate;
        rate.insert(QStringLiteral("kind"),
                    QString::fromStdString(facts.poisson_rate->kind));
        rate.insert(QStringLiteral("events"),
                    static_cast<int>(facts.poisson_rate->events));
        rate.insert(QStringLiteral("exposure"), facts.poisson_rate->exposure);
        rate.insert(QStringLiteral("rate"),
                    optional_number(facts.poisson_rate->rate));
        if (facts.poisson_rate->second_events.has_value()) {
            rate.insert(QStringLiteral("second_events"),
                        static_cast<int>(*facts.poisson_rate->second_events));
        }
        rate.insert(QStringLiteral("second_exposure"),
                    optional_number(facts.poisson_rate->second_exposure));
        rate.insert(QStringLiteral("second_rate"),
                    optional_number(facts.poisson_rate->second_rate));
        rate.insert(QStringLiteral("hypothesized"),
                    optional_number(facts.poisson_rate->hypothesized));
        rate.insert(QStringLiteral("method"),
                    QString::fromStdString(facts.poisson_rate->method));
        rate.insert(QStringLiteral("comparison"),
                    QString::fromStdString(facts.poisson_rate->comparison));
        rate.insert(QStringLiteral("ratio"),
                    optional_number(facts.poisson_rate->ratio));
        rate.insert(QStringLiteral("ratio_ci_lower"),
                    optional_number(facts.poisson_rate->ratio_ci_lower));
        rate.insert(QStringLiteral("ratio_ci_upper"),
                    optional_number(facts.poisson_rate->ratio_ci_upper));
        rate.insert(QStringLiteral("z_statistic"),
                    optional_number(facts.poisson_rate->z_statistic));
        rate.insert(QStringLiteral("p_value"),
                    optional_number(facts.poisson_rate->p_value));
        rate.insert(QStringLiteral("ci_lower"),
                    optional_number(facts.poisson_rate->ci_lower));
        rate.insert(QStringLiteral("ci_upper"),
                    optional_number(facts.poisson_rate->ci_upper));
        rate.insert(QStringLiteral("assumption_status"),
                    QString::fromStdString(facts.poisson_rate->assumption_status));
        serialized.insert(QStringLiteral("poisson_rate"), rate);
    }
    if (facts.equivalence.has_value()) {
        QJsonObject equivalence;
        equivalence.insert(QStringLiteral("kind"),
                           QString::fromStdString(facts.equivalence->kind));
        equivalence.insert(QStringLiteral("difference"),
                           optional_number(facts.equivalence->difference));
        equivalence.insert(QStringLiteral("lower"),
                           optional_number(facts.equivalence->lower));
        equivalence.insert(QStringLiteral("upper"),
                           optional_number(facts.equivalence->upper));
        equivalence.insert(QStringLiteral("ci_lower"),
                           optional_number(facts.equivalence->ci_lower));
        equivalence.insert(QStringLiteral("ci_upper"),
                           optional_number(facts.equivalence->ci_upper));
        equivalence.insert(QStringLiteral("p_lower"),
                           optional_number(facts.equivalence->p_lower));
        equivalence.insert(QStringLiteral("p_upper"),
                           optional_number(facts.equivalence->p_upper));
        equivalence.insert(QStringLiteral("alpha"),
                           optional_number(facts.equivalence->alpha));
        equivalence.insert(QStringLiteral("ci_method"),
                           QString::fromStdString(facts.equivalence->ci_method));
        equivalence.insert(QStringLiteral("within_limits"),
                           facts.equivalence->within_limits);
        equivalence.insert(QStringLiteral("both_pvalues_below_alpha"),
                           facts.equivalence->both_pvalues_below_alpha);
        equivalence.insert(QStringLiteral("assumption_status"),
                           QString::fromStdString(facts.equivalence->assumption_status));
        serialized.insert(QStringLiteral("equivalence"), equivalence);
    }
    if (facts.t_test.has_value()) {
        QJsonObject t_test;
        t_test.insert(QStringLiteral("kind"),
                      QString::fromStdString(facts.t_test->kind));
        t_test.insert(QStringLiteral("n"), static_cast<int>(facts.t_test->n));
        t_test.insert(QStringLiteral("missing_count"),
                      static_cast<int>(facts.t_test->missing_count));
        t_test.insert(QStringLiteral("mean"), optional_number(facts.t_test->mean));
        t_test.insert(QStringLiteral("difference"),
                      optional_number(facts.t_test->difference));
        t_test.insert(QStringLiteral("p_value"),
                      optional_number(facts.t_test->p_value));
        t_test.insert(QStringLiteral("ci_lower"),
                      optional_number(facts.t_test->ci_lower));
        t_test.insert(QStringLiteral("ci_upper"),
                      optional_number(facts.t_test->ci_upper));
        t_test.insert(QStringLiteral("variance_method"),
                      QString::fromStdString(facts.t_test->variance_method));
        t_test.insert(QStringLiteral("assumption_status"),
                      QString::fromStdString(facts.t_test->assumption_status));
        t_test.insert(QStringLiteral("z_statistic"),
                      optional_number(facts.t_test->z_statistic));
        t_test.insert(QStringLiteral("known_sigma"),
                      optional_number(facts.t_test->known_sigma));
        t_test.insert(QStringLiteral("sample_standard_deviation"),
                      optional_number(facts.t_test->sample_standard_deviation));
        serialized.insert(QStringLiteral("t_test"), t_test);
    }
    if (facts.normality.has_value()) {
        QJsonObject normality;
        normality.insert(QStringLiteral("n"), static_cast<int>(facts.normality->n));
        normality.insert(QStringLiteral("missing_count"),
                         static_cast<int>(facts.normality->missing_count));
        normality.insert(QStringLiteral("method"),
                         QString::fromStdString(facts.normality->method));
        normality.insert(QStringLiteral("decision"),
                         QString::fromStdString(facts.normality->decision));
        normality.insert(QStringLiteral("p_value"),
                         optional_number(facts.normality->p_value));
        normality.insert(QStringLiteral("anderson_darling"),
                         optional_number(facts.normality->anderson_darling));
        normality.insert(QStringLiteral("ryan_joiner_r"),
                         optional_number(facts.normality->ryan_joiner_r));
        normality.insert(QStringLiteral("alpha"), facts.normality->alpha);
        normality.insert(QStringLiteral("assumption_status"),
                         QString::fromStdString(facts.normality->assumption_status));
        serialized.insert(QStringLiteral("normality"), normality);
    }
    if (facts.outlier_test.has_value()) {
        QJsonObject outlier_test;
        outlier_test.insert(QStringLiteral("n"), static_cast<int>(facts.outlier_test->n));
        outlier_test.insert(QStringLiteral("missing_count"),
                            static_cast<int>(facts.outlier_test->missing_count));
        outlier_test.insert(QStringLiteral("mean"),
                            optional_number(facts.outlier_test->mean));
        outlier_test.insert(QStringLiteral("standard_deviation"),
                            optional_number(facts.outlier_test->standard_deviation));
        outlier_test.insert(QStringLiteral("g_statistic"),
                            optional_number(facts.outlier_test->g_statistic));
        outlier_test.insert(QStringLiteral("p_value"),
                            optional_number(facts.outlier_test->p_value));
        outlier_test.insert(QStringLiteral("outlier_value"),
                            optional_number(facts.outlier_test->outlier_value));
        if (facts.outlier_test->source_row.has_value()) {
            outlier_test.insert(QStringLiteral("source_row"),
                                static_cast<int>(*facts.outlier_test->source_row));
        }
        outlier_test.insert(QStringLiteral("direction"),
                            QString::fromStdString(facts.outlier_test->direction));
        outlier_test.insert(QStringLiteral("alternative"),
                            QString::fromStdString(facts.outlier_test->alternative));
        outlier_test.insert(QStringLiteral("alpha"), facts.outlier_test->alpha);
        outlier_test.insert(QStringLiteral("assumption_status"),
                            QString::fromStdString(facts.outlier_test->assumption_status));
        outlier_test.insert(QStringLiteral("method"),
                            QString::fromStdString(facts.outlier_test->method));
        outlier_test.insert(QStringLiteral("dixon_r"),
                            optional_number(facts.outlier_test->dixon_r));
        outlier_test.insert(QStringLiteral("critical_value"),
                            optional_number(facts.outlier_test->critical_value));
        serialized.insert(QStringLiteral("outlier_test"), outlier_test);
    }
    if (facts.correlation.has_value()) {
        QJsonObject correlation;
        correlation.insert(QStringLiteral("method"),
                           QString::fromStdString(facts.correlation->method));
        correlation.insert(QStringLiteral("variable_count"),
                           static_cast<int>(facts.correlation->variable_count));
        correlation.insert(QStringLiteral("n"), static_cast<int>(facts.correlation->n));
        correlation.insert(QStringLiteral("missing_skipped"),
                           static_cast<int>(facts.correlation->missing_skipped));
        correlation.insert(QStringLiteral("assumption_status"),
                           QString::fromStdString(facts.correlation->assumption_status));
        correlation.insert(QStringLiteral("covariance_available"),
                           facts.correlation->covariance_available);
        correlation.insert(QStringLiteral("partial_available"),
                           facts.correlation->partial_available);
        serialized.insert(QStringLiteral("correlation"), correlation);
    }
    if (facts.acceptance_sampling.has_value()) {
        QJsonObject acceptance;
        acceptance.insert(QStringLiteral("sample_size"),
                          static_cast<int>(facts.acceptance_sampling->sample_size));
        acceptance.insert(QStringLiteral("acceptance_number"),
                          static_cast<int>(facts.acceptance_sampling->acceptance_number));
        acceptance.insert(QStringLiteral("lot_size"),
                          optional_size(facts.acceptance_sampling->lot_size));
        acceptance.insert(QStringLiteral("model"),
                          QString::fromStdString(facts.acceptance_sampling->model));
        acceptance.insert(QStringLiteral("aql"),
                          optional_number(facts.acceptance_sampling->aql));
        acceptance.insert(QStringLiteral("rql"),
                          optional_number(facts.acceptance_sampling->rql));
        acceptance.insert(QStringLiteral("pa_at_aql"),
                          optional_number(facts.acceptance_sampling->pa_at_aql));
        acceptance.insert(QStringLiteral("pa_at_rql"),
                          optional_number(facts.acceptance_sampling->pa_at_rql));
        acceptance.insert(QStringLiteral("oc_point_count"),
                          static_cast<int>(facts.acceptance_sampling->oc_point_count));
        serialized.insert(QStringLiteral("acceptance_sampling"), acceptance);
    }
    if (facts.anom.has_value()) {
        QJsonObject anom;
        anom.insert(QStringLiteral("overall_mean"), facts.anom->overall_mean);
        anom.insert(QStringLiteral("pooled_sd"), facts.anom->pooled_sd);
        anom.insert(QStringLiteral("udl"), facts.anom->udl);
        anom.insert(QStringLiteral("ldl"), facts.anom->ldl);
        anom.insert(QStringLiteral("alpha"), facts.anom->alpha);
        anom.insert(QStringLiteral("group_count"),
                      static_cast<int>(facts.anom->group_count));
        anom.insert(QStringLiteral("total_n"), static_cast<int>(facts.anom->total_n));
        anom.insert(QStringLiteral("outside_count"),
                      static_cast<int>(facts.anom->outside_count));
        anom.insert(QStringLiteral("decision_limit_method"),
                      QString::fromStdString(facts.anom->decision_limit_method));
        serialized.insert(QStringLiteral("anom"), anom);
    }
    if (facts.run_chart.has_value()) {
        QJsonObject run_chart;
        run_chart.insert(QStringLiteral("n"), static_cast<int>(facts.run_chart->n));
        run_chart.insert(QStringLiteral("median"),
                         optional_number(facts.run_chart->median));
        run_chart.insert(QStringLiteral("runs_about_median"),
                         static_cast<int>(facts.run_chart->runs_about_median));
        run_chart.insert(QStringLiteral("runs_up_down"),
                         static_cast<int>(facts.run_chart->runs_up_down));
        run_chart.insert(QStringLiteral("p_clustering"),
                         optional_number(facts.run_chart->p_clustering));
        run_chart.insert(QStringLiteral("p_mixtures"),
                         optional_number(facts.run_chart->p_mixtures));
        run_chart.insert(QStringLiteral("p_trends"),
                         optional_number(facts.run_chart->p_trends));
        run_chart.insert(QStringLiteral("p_oscillation"),
                         optional_number(facts.run_chart->p_oscillation));
        run_chart.insert(QStringLiteral("missing_count"),
                         static_cast<int>(facts.run_chart->missing_count));
        serialized.insert(QStringLiteral("run_chart"), run_chart);
    }
    if (facts.zone_chart.has_value()) {
        QJsonObject zone_chart;
        zone_chart.insert(QStringLiteral("n"), static_cast<int>(facts.zone_chart->n));
        zone_chart.insert(QStringLiteral("missing_count"),
                          static_cast<int>(facts.zone_chart->missing_count));
        zone_chart.insert(QStringLiteral("center"),
                          optional_number(facts.zone_chart->center));
        zone_chart.insert(QStringLiteral("sigma"),
                          optional_number(facts.zone_chart->sigma));
        zone_chart.insert(QStringLiteral("signal_threshold"),
                          facts.zone_chart->signal_threshold);
        zone_chart.insert(QStringLiteral("signal_count"),
                          static_cast<int>(facts.zone_chart->signal_count));
        serialized.insert(QStringLiteral("zone_chart"), zone_chart);
    }
    if (facts.z_mr.has_value()) {
        QJsonObject z_mr;
        z_mr.insert(QStringLiteral("n"), static_cast<int>(facts.z_mr->n));
        z_mr.insert(QStringLiteral("missing_count"),
                    static_cast<int>(facts.z_mr->missing_count));
        z_mr.insert(QStringLiteral("group_count"),
                    static_cast<int>(facts.z_mr->group_count));
        z_mr.insert(QStringLiteral("used_sample_parameters"),
                    facts.z_mr->used_sample_parameters);
        z_mr.insert(QStringLiteral("average_mr"),
                    optional_number(facts.z_mr->average_mr));
        z_mr.insert(QStringLiteral("z_out_of_control_count"),
                    static_cast<int>(facts.z_mr->z_out_of_control_count));
        serialized.insert(QStringLiteral("z_mr"), z_mr);
    }
    if (facts.moving_average.has_value()) {
        QJsonObject moving_average;
        moving_average.insert(QStringLiteral("n"),
                              static_cast<int>(facts.moving_average->n));
        moving_average.insert(QStringLiteral("missing_count"),
                              static_cast<int>(facts.moving_average->missing_count));
        moving_average.insert(QStringLiteral("window"), facts.moving_average->window);
        moving_average.insert(QStringLiteral("limit_sigma"),
                              facts.moving_average->limit_sigma);
        moving_average.insert(QStringLiteral("center"),
                              optional_number(facts.moving_average->center));
        moving_average.insert(QStringLiteral("sigma_within"),
                              optional_number(facts.moving_average->sigma_within));
        moving_average.insert(QStringLiteral("out_of_control_count"),
                              static_cast<int>(facts.moving_average->out_of_control_count));
        serialized.insert(QStringLiteral("moving_average"), moving_average);
    }
    if (facts.cause_effect.has_value()) {
        QJsonObject cause_effect;
        cause_effect.insert(QStringLiteral("effect"),
                            QString::fromStdString(facts.cause_effect->effect));
        cause_effect.insert(QStringLiteral("category_count"),
                            static_cast<int>(facts.cause_effect->category_count));
        cause_effect.insert(QStringLiteral("cause_count"),
                            static_cast<int>(facts.cause_effect->cause_count));
        cause_effect.insert(QStringLiteral("missing_count"),
                            static_cast<int>(facts.cause_effect->missing_count));
        serialized.insert(QStringLiteral("cause_effect"), cause_effect);
    }
    if (!serialized.isEmpty()) {
        object.insert(QStringLiteral("facts"), serialized);
    }
}

void read_interpretation_facts(
    const QJsonObject& object,
    domain::InterpretationFacts& facts)
{
    const QJsonObject serialized = object.value(QStringLiteral("facts")).toObject();
    if (serialized.isEmpty()) {
        return;
    }
    const QJsonObject capability = serialized.value(QStringLiteral("capability")).toObject();
    if (!capability.isEmpty()) {
        domain::CapabilityFacts value;
        value.cpk = read_optional(capability.value(QStringLiteral("cpk")));
        value.ppk = read_optional(capability.value(QStringLiteral("ppk")));
        value.cpm = read_optional(capability.value(QStringLiteral("cpm")));
        value.z_bench = read_optional(capability.value(QStringLiteral("z_bench")));
        value.assumption_status = capability.value(QStringLiteral("assumption_status"))
            .toString("not_verified").toStdString();
        value.specification_mode =
            capability.value(QStringLiteral("specification_mode")).toString().toStdString();
        value.method = capability.value(QStringLiteral("method")).toString().toStdString();
        value.johnson_family =
            capability.value(QStringLiteral("johnson_family")).toString().toStdString();
        value.normality_p_value =
            read_optional(capability.value(QStringLiteral("normality_p_value")));
        value.transform_p_value =
            read_optional(capability.value(QStringLiteral("transform_p_value")));
        value.transform_anderson_darling =
            read_optional(capability.value(QStringLiteral("transform_anderson_darling")));
        value.nonnormal_distribution =
            capability.value(QStringLiteral("nonnormal_distribution")).toString().toStdString();
        value.fitted_shape = read_optional(capability.value(QStringLiteral("fitted_shape")));
        value.fitted_scale = read_optional(capability.value(QStringLiteral("fitted_scale")));
        value.average_p = read_optional(capability.value(QStringLiteral("average_p")));
        value.percent_defective =
            read_optional(capability.value(QStringLiteral("percent_defective")));
        value.ppm_defective = read_optional(capability.value(QStringLiteral("ppm_defective")));
        value.process_z = read_optional(capability.value(QStringLiteral("process_z")));
        value.mean_dpu = read_optional(capability.value(QStringLiteral("mean_dpu")));
        value.cp = read_optional(capability.value(QStringLiteral("cp")));
        value.pp = read_optional(capability.value(QStringLiteral("pp")));
        value.cpk_lower = read_optional(capability.value(QStringLiteral("cpk_lower")));
        value.cpk_upper = read_optional(capability.value(QStringLiteral("cpk_upper")));
        value.ppk_lower = read_optional(capability.value(QStringLiteral("ppk_lower")));
        value.ppk_upper = read_optional(capability.value(QStringLiteral("ppk_upper")));
        value.capability_ci_method =
            capability.value(QStringLiteral("capability_ci_method")).toString().toStdString();
        value.pass_fail_judgment_allowed = false;
        // Until a productized verification workflow exists, ignore any stored true.
        (void)capability.value(QStringLiteral("pass_fail_judgment_allowed"));
        value.research_preview =
            capability.value(QStringLiteral("research_preview")).toBool(
                value.method == "johnson");
        value.gate_status =
            capability.value(QStringLiteral("gate_status"))
                .toString(value.method == "johnson"
                              ? QStringLiteral("gated_research")
                              : QStringLiteral("stability_unverified"))
                .toStdString();
        value.evidence_type =
            capability.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference"))
                .toStdString();
        value.stability_screen_status =
            capability.value(QStringLiteral("stability_screen_status"))
                .toString(QStringLiteral("not_run"))
                .toStdString();
        value.stability_out_of_control_count = static_cast<std::size_t>(
            capability.value(QStringLiteral("stability_out_of_control_count")).toInt(0));
        value.bimodality_screen_status =
            capability.value(QStringLiteral("bimodality_screen_status"))
                .toString(QStringLiteral("not_run"))
                .toStdString();
        value.bimodality_peak_count = static_cast<std::size_t>(
            capability.value(QStringLiteral("bimodality_peak_count")).toInt(0));
        value.hartigan_dip_status =
            capability.value(QStringLiteral("hartigan_dip_status"))
                .toString(QStringLiteral("not_run"))
                .toStdString();
        value.hartigan_dip_statistic =
            capability.value(QStringLiteral("hartigan_dip_statistic")).toDouble(0.0);
        if (capability.contains(QStringLiteral("hartigan_dip_p_value"))) {
            value.hartigan_dip_p_value =
                capability.value(QStringLiteral("hartigan_dip_p_value")).toDouble();
        }
        value.mixture_status =
            capability.value(QStringLiteral("mixture_status"))
                .toString(QStringLiteral("not_run"))
                .toStdString();
        value.mixture_k_selected =
            capability.value(QStringLiteral("mixture_k_selected")).toInt(1);
        value.mixture_k_max =
            capability.value(QStringLiteral("mixture_k_max")).toInt(4);
        value.mixture_weight1 =
            capability.value(QStringLiteral("mixture_weight1")).toDouble(0.0);
        value.mixture_mean1 =
            capability.value(QStringLiteral("mixture_mean1")).toDouble(0.0);
        value.mixture_mean2 =
            capability.value(QStringLiteral("mixture_mean2")).toDouble(0.0);
        value.mixture_sd1 =
            capability.value(QStringLiteral("mixture_sd1")).toDouble(0.0);
        value.mixture_sd2 =
            capability.value(QStringLiteral("mixture_sd2")).toDouble(0.0);
        value.mixture_delta_bic =
            capability.value(QStringLiteral("mixture_delta_bic")).toDouble(0.0);
        value.mixture_algorithm_id =
            capability.value(QStringLiteral("mixture_algorithm_id"))
                .toString(QStringLiteral("gaussian_mixture_k_bic"))
                .toStdString();
        value.mixture_evidence_type =
            capability.value(QStringLiteral("mixture_evidence_type"))
                .toString(QStringLiteral("formula_reference"))
                .toStdString();
        if (value.mixture_evidence_type == "vendor_oracle"
            || value.mixture_evidence_type == "golden") {
            value.mixture_evidence_type = "formula_reference";
        }
        if (value.mixture_algorithm_id != "gaussian_mixture_k_bic"
            && value.mixture_algorithm_id != "gaussian_mixture_2_em") {
            value.mixture_algorithm_id = "gaussian_mixture_k_bic";
        }
        const QJsonArray mixture_components =
            capability.value(QStringLiteral("mixture_components")).toArray();
        for (const QJsonValue& entry : mixture_components) {
            const QJsonObject item = entry.toObject();
            domain::CapabilityFacts::MixtureComponentFacts c;
            c.weight = item.value(QStringLiteral("weight")).toDouble(0.0);
            c.mean = item.value(QStringLiteral("mean")).toDouble(0.0);
            c.sd = item.value(QStringLiteral("sd")).toDouble(0.0);
            value.mixture_components.push_back(c);
        }
        facts.capability = std::move(value);
    }
    const QJsonObject spc = serialized.value(QStringLiteral("spc")).toObject();
    if (!spc.isEmpty()) {
        facts.spc = domain::SpcFacts{};
        facts.spc->out_of_control_count =
            read_optional_size(spc.value(QStringLiteral("out_of_control_count")));
        facts.spc->sigma_z = read_optional(spc.value(QStringLiteral("sigma_z")));
        facts.spc->sigma_within = read_optional(spc.value(QStringLiteral("sigma_within")));
        facts.spc->sigma_between = read_optional(spc.value(QStringLiteral("sigma_between")));
        facts.spc->sigma_between_within =
            read_optional(spc.value(QStringLiteral("sigma_between_within")));
        facts.spc->between_within_method =
            spc.value(QStringLiteral("between_within_method")).toString().toStdString();
        facts.spc->rule_policy =
            spc.value(QStringLiteral("rule_policy")).toString().toStdString();
        for (const QJsonValue& item :
             spc.value(QStringLiteral("enabled_special_cause_tests")).toArray()) {
            facts.spc->enabled_special_cause_tests.push_back(item.toInt());
        }
        for (const QJsonValue& item :
             spc.value(QStringLiteral("enabled_special_cause_rule_ids")).toArray()) {
            facts.spc->enabled_special_cause_rule_ids.push_back(
                item.toString().toStdString());
        }
        facts.spc->rules = read_rules(spc.value(QStringLiteral("rules")).toArray());
        facts.spc->sigma_method =
            spc.value(QStringLiteral("sigma_method")).toString().toStdString();
        facts.spc->use_nelson_estimate =
            spc.value(QStringLiteral("use_nelson_estimate")).toBool(false);
        facts.spc->nelson_excluded_ranges = static_cast<std::size_t>(
            spc.value(QStringLiteral("nelson_excluded_ranges")).toInt(0));
        facts.spc->estimated_sigma =
            read_optional(spc.value(QStringLiteral("estimated_sigma")));
        facts.spc->historical_parameters_used =
            spc.value(QStringLiteral("historical_parameters_used")).toBool(false);
        facts.spc->stage_count = static_cast<std::size_t>(
            spc.value(QStringLiteral("stage_count")).toInt(0));
    }
    const QJsonObject multivariate_spc =
        serialized.value(QStringLiteral("multivariate_spc")).toObject();
    if (!multivariate_spc.isEmpty()) {
        domain::MultivariateSpcFacts value;
        value.kind = multivariate_spc.value(QStringLiteral("kind")).toString().toStdString();
        value.observation_count = static_cast<std::size_t>(
            multivariate_spc.value(QStringLiteral("observation_count")).toInt(0));
        value.variable_count = static_cast<std::size_t>(
            multivariate_spc.value(QStringLiteral("variable_count")).toInt(0));
        value.subgroup_count = static_cast<std::size_t>(
            multivariate_spc.value(QStringLiteral("subgroup_count")).toInt(0));
        value.out_of_control_count = static_cast<std::size_t>(
            multivariate_spc.value(QStringLiteral("out_of_control_count")).toInt(0));
        value.upper_control_limit =
            read_optional(multivariate_spc.value(QStringLiteral("upper_control_limit")));
        value.lower_control_limit =
            read_optional(multivariate_spc.value(QStringLiteral("lower_control_limit")));
        value.center_line =
            read_optional(multivariate_spc.value(QStringLiteral("center_line")));
        value.lambda = read_optional(multivariate_spc.value(QStringLiteral("lambda")));
        value.limit_method =
            multivariate_spc.value(QStringLiteral("limit_method")).toString().toStdString();
        value.phase = multivariate_spc.value(QStringLiteral("phase")).toString().toStdString();
        facts.multivariate_spc = std::move(value);
    }
    const QJsonObject regression = serialized.value(QStringLiteral("regression")).toObject();
    if (!regression.isEmpty()) {
        domain::RegressionFacts value;
        value.r_squared = read_optional(regression.value(QStringLiteral("r_squared")));
        value.residual_normality_p =
            read_optional(regression.value(QStringLiteral("residual_normality_p")));
        value.residual_anderson_darling =
            read_optional(regression.value(QStringLiteral("residual_anderson_darling")));
        value.residual_plot_count = static_cast<std::size_t>(
            regression.value(QStringLiteral("residual_plot_count")).toInt(0));
        value.influential_count = static_cast<std::size_t>(
            regression.value(QStringLiteral("influential_count")).toInt(0));
        value.assumption_status = regression.value(QStringLiteral("assumption_status"))
            .toString("not_verified").toStdString();
        value.outlier_count = static_cast<std::size_t>(
            regression.value(QStringLiteral("outlier_count")).toInt(0));
        value.high_leverage_count = static_cast<std::size_t>(
            regression.value(QStringLiteral("high_leverage_count")).toInt(0));
        value.max_vif = read_optional(regression.value(QStringLiteral("max_vif")));
        value.durbin_watson = read_optional(regression.value(QStringLiteral("durbin_watson")));
        value.durbin_watson_dl =
            read_optional(regression.value(QStringLiteral("durbin_watson_dl")));
        value.durbin_watson_du =
            read_optional(regression.value(QStringLiteral("durbin_watson_du")));
        value.durbin_watson_decision =
            regression.value(QStringLiteral("durbin_watson_decision"))
                .toString(QStringLiteral("not_computed"))
                .toStdString();
        value.error_degrees_of_freedom =
            read_optional(regression.value(QStringLiteral("error_degrees_of_freedom")));
        value.rank_deficient = regression.value(QStringLiteral("rank_deficient")).toBool(false);
        value.assumptions = read_assumptions(
            regression.value(QStringLiteral("assumptions")).toArray());
        value.rules = read_rules(regression.value(QStringLiteral("rules")).toArray());
        facts.regression = std::move(value);
    }
    const QJsonObject anova = serialized.value(QStringLiteral("anova")).toObject();
    if (!anova.isEmpty()) {
        domain::AnovaFacts value;
        value.p_value = read_optional(anova.value(QStringLiteral("p_value")));
        value.error_degrees_of_freedom = static_cast<std::size_t>(
            anova.value(QStringLiteral("error_degrees_of_freedom")).toInt(0));
        value.estimable = anova.value(QStringLiteral("estimable")).toBool(true);
        value.not_estimable_term_count = static_cast<std::size_t>(
            anova.value(QStringLiteral("not_estimable_term_count")).toInt(0));
        value.significant_terms = to_strings(
            anova.value(QStringLiteral("significant_terms")).toArray());
        value.family_confidence_level =
            read_optional(anova.value(QStringLiteral("family_confidence_level")));
        value.tukey_significant_pairs = static_cast<std::size_t>(
            anova.value(QStringLiteral("tukey_significant_pairs")).toInt(0));
        value.tukey_method = anova.value(QStringLiteral("tukey_method")).toString().toStdString();
        value.tukey_interval_columns =
            anova.value(QStringLiteral("tukey_interval_columns")).toString().toStdString();
        value.tukey_grouping_available =
            anova.value(QStringLiteral("tukey_grouping_available")).toBool(false);
        value.grouping_letter_count = static_cast<std::size_t>(
            anova.value(QStringLiteral("grouping_letter_count")).toInt(0));
        value.assumption_status = anova.value(QStringLiteral("assumption_status"))
            .toString("not_verified").toStdString();
        value.assumptions = read_assumptions(
            anova.value(QStringLiteral("assumptions")).toArray());
        value.rules = read_rules(anova.value(QStringLiteral("rules")).toArray());
        facts.anova = std::move(value);
    }
    const QJsonObject msa = serialized.value(QStringLiteral("msa")).toObject();
    if (!msa.isEmpty()) {
        domain::MsaFacts value;
        value.slope = read_optional(msa.value(QStringLiteral("slope")));
        value.bias_low = read_optional(msa.value(QStringLiteral("bias_low")));
        value.bias_high = read_optional(msa.value(QStringLiteral("bias_high")));
        value.p_value = read_optional(msa.value(QStringLiteral("p_value")));
        value.cgk = read_optional(msa.value(QStringLiteral("cgk")));
        value.tolerance_percent = read_optional(msa.value(QStringLiteral("tolerance_percent")));
        value.ndc = read_optional(msa.value(QStringLiteral("ndc")));
        value.ndc_available = msa.value(QStringLiteral("ndc_available")).toBool(false);
        value.design_balanced = msa.value(QStringLiteral("design_balanced")).toBool(true);
        value.interaction_retained =
            msa.value(QStringLiteral("interaction_retained")).toBool(true);
        value.interaction_p_value =
            read_optional(msa.value(QStringLiteral("interaction_p_value")));
        value.interaction_reduction_recommended =
            msa.value(QStringLiteral("interaction_reduction_recommended")).toBool(false);
        value.negative_variance_truncated =
            msa.value(QStringLiteral("negative_variance_truncated")).toBool(false);
        value.gage_percent_study_variation =
            read_optional(msa.value(QStringLiteral("gage_percent_study_variation")));
        value.gage_percent_contribution =
            read_optional(msa.value(QStringLiteral("gage_percent_contribution")));
        value.linearity = read_optional(msa.value(QStringLiteral("linearity")));
        value.percent_linearity =
            read_optional(msa.value(QStringLiteral("percent_linearity")));
        value.slope_p_value = read_optional(msa.value(QStringLiteral("slope_p_value")));
        value.intercept_p_value =
            read_optional(msa.value(QStringLiteral("intercept_p_value")));
        value.residual_s = read_optional(msa.value(QStringLiteral("residual_s")));
        value.average_bias = read_optional(msa.value(QStringLiteral("average_bias")));
        value.average_bias_p =
            read_optional(msa.value(QStringLiteral("average_bias_p")));
        value.process_variation_used =
            read_optional(msa.value(QStringLiteral("process_variation_used")));
        value.ratings_are_ordinal =
            msa.value(QStringLiteral("ratings_are_ordinal")).toBool(false);
        value.weighted_kappa_available =
            msa.value(QStringLiteral("weighted_kappa_available")).toBool(false);
        value.kappa_weight_scheme =
            msa.value(QStringLiteral("kappa_weight_scheme")).toString("none").toStdString();
        if (value.kappa_weight_scheme.empty()) {
            value.kappa_weight_scheme = "none";
        }
        value.kendall_available =
            msa.value(QStringLiteral("kendall_available")).toBool(false);
        value.by_part_plot_available =
            msa.value(QStringLiteral("by_part_plot_available")).toBool(false);
        value.interaction_plot_available =
            msa.value(QStringLiteral("interaction_plot_available")).toBool(false);
        value.plot_point_count = static_cast<std::size_t>(
            msa.value(QStringLiteral("plot_point_count")).toInt(0));
        value.kendall_w = read_optional(msa.value(QStringLiteral("kendall_w")));
        value.kendall_w_p = read_optional(msa.value(QStringLiteral("kendall_w_p")));
        value.kendall_tau = read_optional(msa.value(QStringLiteral("kendall_tau")));
        value.kendall_tau_p = read_optional(msa.value(QStringLiteral("kendall_tau_p")));
        value.emp_available = msa.value(QStringLiteral("emp_available")).toBool(false);
        value.emp_icc_no_bias =
            read_optional(msa.value(QStringLiteral("emp_icc_no_bias")));
        value.emp_icc_with_bias =
            read_optional(msa.value(QStringLiteral("emp_icc_with_bias")));
        value.emp_icc_with_interaction =
            read_optional(msa.value(QStringLiteral("emp_icc_with_interaction")));
        value.emp_probable_error =
            read_optional(msa.value(QStringLiteral("emp_probable_error")));
        value.emp_classification =
            msa.value(QStringLiteral("emp_classification")).toString().toStdString();
        value.assumption_status = msa.value(QStringLiteral("assumption_status"))
            .toString("not_verified").toStdString();
        value.rules = read_rules(msa.value(QStringLiteral("rules")).toArray());
        facts.msa = std::move(value);
    }
    const QJsonObject reliability = serialized.value(QStringLiteral("reliability")).toObject();
    if (!reliability.isEmpty()) {
        domain::ReliabilityFacts value;
        value.shape = read_optional(reliability.value(QStringLiteral("shape")));
        value.censored_count =
            read_optional_size(reliability.value(QStringLiteral("censored_count")));
        value.failure_count =
            read_optional_size(reliability.value(QStringLiteral("failure_count")));
        value.valid_count =
            read_optional_size(reliability.value(QStringLiteral("valid_count")));
        value.median_life = read_optional(reliability.value(QStringLiteral("median_life")));
        value.identifiable = reliability.value(QStringLiteral("identifiable")).toBool(false);
        value.converged = reliability.value(QStringLiteral("converged")).toBool(false);
        value.not_computed_reason =
            reliability.value(QStringLiteral("not_computed_reason")).toString().toStdString();
        value.event_encoding = reliability.value(QStringLiteral("event_encoding"))
            .toString("failure_suspension").toStdString();
        value.distribution = reliability.value(QStringLiteral("distribution"))
            .toString().toStdString();
        value.location = read_optional(reliability.value(QStringLiteral("location")));
        value.scale = read_optional(reliability.value(QStringLiteral("scale")));
        value.threshold = read_optional(reliability.value(QStringLiteral("threshold")));
        value.rules = read_rules(reliability.value(QStringLiteral("rules")).toArray());
        value.evidence_type = reliability.value(QStringLiteral("evidence_type"))
            .toString(QStringLiteral("formula_reference")).toStdString();
        value.time_unit =
            reliability.value(QStringLiteral("time_unit")).toString().toStdString();
        value.exact_count = static_cast<std::size_t>(
            reliability.value(QStringLiteral("exact_count")).toInt(0));
        value.right_censored_count = static_cast<std::size_t>(
            reliability.value(QStringLiteral("right_censored_count")).toInt(0));
        value.left_censored_count = static_cast<std::size_t>(
            reliability.value(QStringLiteral("left_censored_count")).toInt(0));
        value.interval_censored_count = static_cast<std::size_t>(
            reliability.value(QStringLiteral("interval_censored_count")).toInt(0));
        for (const QJsonValue item :
             reliability.value(QStringLiteral("failure_modes")).toArray()) {
            value.failure_modes.push_back(item.toString().toStdString());
        }
        value.failure_mode_distinct_count = static_cast<std::size_t>(
            reliability.value(QStringLiteral("failure_mode_distinct_count")).toInt(
                static_cast<int>(value.failure_modes.size())));
        value.total_exposure = read_optional(reliability.value(QStringLiteral("total_exposure")));
        value.exposure_row_count = static_cast<std::size_t>(
            reliability.value(QStringLiteral("exposure_row_count")).toInt(0));
        value.exposure_source =
            reliability.value(QStringLiteral("exposure_source")).toString().toStdString();
        value.mode_fit_scheme =
            reliability.value(QStringLiteral("mode_fit_scheme")).toString().toStdString();
        for (const QJsonValue entry :
             reliability.value(QStringLiteral("mode_fits")).toArray()) {
            const QJsonObject item = entry.toObject();
            domain::ReliabilityModeFitFacts fit;
            fit.failure_mode =
                item.value(QStringLiteral("failure_mode")).toString().toStdString();
            fit.failure_count = static_cast<std::size_t>(
                item.value(QStringLiteral("failure_count")).toInt(0));
            fit.competing_failure_count = static_cast<std::size_t>(
                item.value(QStringLiteral("competing_failure_count")).toInt(0));
            fit.right_censored_count = static_cast<std::size_t>(
                item.value(QStringLiteral("right_censored_count")).toInt(0));
            fit.valid_count = static_cast<std::size_t>(
                item.value(QStringLiteral("valid_count")).toInt(0));
            fit.identifiable = item.value(QStringLiteral("identifiable")).toBool(false);
            fit.converged = item.value(QStringLiteral("converged")).toBool(false);
            fit.shape = read_optional(item.value(QStringLiteral("shape")));
            fit.scale = read_optional(item.value(QStringLiteral("scale")));
            fit.location = read_optional(item.value(QStringLiteral("location")));
            fit.rate = read_optional(item.value(QStringLiteral("rate")));
            fit.median_life = read_optional(item.value(QStringLiteral("median_life")));
            fit.reliability_at_warranty =
                read_optional(item.value(QStringLiteral("reliability_at_warranty")));
            fit.not_computed_reason =
                item.value(QStringLiteral("not_computed_reason")).toString().toStdString();
            fit.evidence_type =
                item.value(QStringLiteral("evidence_type"))
                    .toString(QStringLiteral("formula_reference"))
                    .toStdString();
            if (fit.evidence_type == "vendor_oracle" || fit.evidence_type == "golden") {
                fit.evidence_type = "formula_reference";
            }
            fit.algorithm_id =
                item.value(QStringLiteral("algorithm_id"))
                    .toString(QStringLiteral("cause_specific_right_censored_competing"))
                    .toStdString();
            for (const QJsonValue row :
                 item.value(QStringLiteral("source_rows")).toArray()) {
                fit.source_rows.push_back(static_cast<std::size_t>(row.toInt(0)));
            }
            value.mode_fits.push_back(std::move(fit));
        }
        value.cif_algorithm_id =
            reliability.value(QStringLiteral("cif_algorithm_id")).toString().toStdString();
        value.cif_evidence_type =
            reliability.value(QStringLiteral("cif_evidence_type"))
                .toString(QStringLiteral("formula_reference"))
                .toStdString();
        if (value.cif_evidence_type == "vendor_oracle" || value.cif_evidence_type == "golden") {
            value.cif_evidence_type = "formula_reference";
        }
        if (value.cif_algorithm_id == "fine_gray" || value.cif_algorithm_id.find("vendor") != std::string::npos) {
            value.cif_algorithm_id = "aalen_johansen_cif";
        }
        for (const QJsonValue entry :
             reliability.value(QStringLiteral("cif_modes")).toArray()) {
            const QJsonObject item = entry.toObject();
            domain::ReliabilityCifModeFacts mode;
            mode.failure_mode =
                item.value(QStringLiteral("failure_mode")).toString().toStdString();
            mode.failure_count = static_cast<std::size_t>(
                item.value(QStringLiteral("failure_count")).toInt(0));
            mode.cif_at_last_event =
                read_optional(item.value(QStringLiteral("cif_at_last_event")));
            mode.cif_at_warranty =
                read_optional(item.value(QStringLiteral("cif_at_warranty")));
            mode.point_count = static_cast<std::size_t>(
                item.value(QStringLiteral("point_count")).toInt(0));
            value.cif_modes.push_back(std::move(mode));
        }
        value.fine_gray_algorithm_id =
            reliability.value(QStringLiteral("fine_gray_algorithm_id")).toString().toStdString();
        value.fine_gray_evidence_type =
            reliability.value(QStringLiteral("fine_gray_evidence_type"))
                .toString(QStringLiteral("formula_reference"))
                .toStdString();
        if (value.fine_gray_evidence_type == "vendor_oracle"
            || value.fine_gray_evidence_type == "golden") {
            value.fine_gray_evidence_type = "formula_reference";
        }
        value.fine_gray_kind =
            reliability.value(QStringLiteral("fine_gray_kind")).toString().toStdString();
        if (value.fine_gray_algorithm_id == "fine_gray_continuous_ipcw") {
            value.fine_gray_kind = "continuous";
        } else if (value.fine_gray_algorithm_id == "fine_gray_binary_ipcw") {
            value.fine_gray_kind = "binary";
        } else if (value.fine_gray_algorithm_id == "fine_gray_multi_ipcw") {
            value.fine_gray_kind = "multi";
        } else if (!value.fine_gray_algorithm_id.empty()) {
            if (value.fine_gray_kind == "multi") {
                value.fine_gray_algorithm_id = "fine_gray_multi_ipcw";
            } else if (value.fine_gray_kind == "continuous") {
                value.fine_gray_algorithm_id = "fine_gray_continuous_ipcw";
            } else {
                value.fine_gray_algorithm_id = "fine_gray_binary_ipcw";
                if (value.fine_gray_kind != "binary") {
                    value.fine_gray_kind = "binary";
                }
            }
        }
        value.fine_gray_target_mode =
            reliability.value(QStringLiteral("fine_gray_target_mode")).toString().toStdString();
        value.fine_gray_covariate_name =
            reliability.value(QStringLiteral("fine_gray_covariate_name")).toString().toStdString();
        value.fine_gray_group0 =
            reliability.value(QStringLiteral("fine_gray_group0")).toString().toStdString();
        value.fine_gray_group1 =
            reliability.value(QStringLiteral("fine_gray_group1")).toString().toStdString();
        value.fine_gray_converged =
            reliability.value(QStringLiteral("fine_gray_converged")).toBool(false);
        value.fine_gray_covariate_mean =
            read_optional(reliability.value(QStringLiteral("fine_gray_covariate_mean")));
        value.fine_gray_beta =
            read_optional(reliability.value(QStringLiteral("fine_gray_beta")));
        value.fine_gray_se =
            read_optional(reliability.value(QStringLiteral("fine_gray_se")));
        value.fine_gray_hazard_ratio =
            read_optional(reliability.value(QStringLiteral("fine_gray_hazard_ratio")));
        value.fine_gray_p_value =
            read_optional(reliability.value(QStringLiteral("fine_gray_p_value")));
        value.fine_gray_not_computed_reason =
            reliability.value(QStringLiteral("fine_gray_not_computed_reason"))
                .toString()
                .toStdString();
        value.fine_gray_target_failures = static_cast<std::size_t>(
            reliability.value(QStringLiteral("fine_gray_target_failures")).toInt(0));
        value.fine_gray_competing_failures = static_cast<std::size_t>(
            reliability.value(QStringLiteral("fine_gray_competing_failures")).toInt(0));
        const QJsonArray terms =
            reliability.value(QStringLiteral("fine_gray_terms")).toArray();
        for (const QJsonValue& entry : terms) {
            const QJsonObject item = entry.toObject();
            domain::ReliabilityFineGrayTermFacts term;
            term.name = item.value(QStringLiteral("name")).toString().toStdString();
            term.mean = read_optional(item.value(QStringLiteral("mean")));
            term.beta = read_optional(item.value(QStringLiteral("beta")));
            term.se = read_optional(item.value(QStringLiteral("se")));
            term.hazard_ratio = read_optional(item.value(QStringLiteral("hazard_ratio")));
            term.p_value = read_optional(item.value(QStringLiteral("p_value")));
            value.fine_gray_terms.push_back(std::move(term));
        }
        value.log_rank_group_count =
            read_optional_size(reliability.value(QStringLiteral("log_rank_group_count")));
        value.log_rank_chi_square =
            read_optional(reliability.value(QStringLiteral("log_rank_chi_square")));
        value.log_rank_df =
            read_optional(reliability.value(QStringLiteral("log_rank_df")));
        value.log_rank_p_value =
            read_optional(reliability.value(QStringLiteral("log_rank_p_value")));
        for (const QJsonValue& entry :
             reliability.value(QStringLiteral("log_rank_groups")).toArray()) {
            const QJsonObject item = entry.toObject();
            domain::ReliabilityLogRankGroupFacts group;
            group.label = item.value(QStringLiteral("label")).toString().toStdString();
            group.group_id = item.value(QStringLiteral("group_id")).toInt(0);
            group.n = static_cast<std::size_t>(item.value(QStringLiteral("n")).toInt(0));
            group.failures =
                static_cast<std::size_t>(item.value(QStringLiteral("failures")).toInt(0));
            group.censored =
                static_cast<std::size_t>(item.value(QStringLiteral("censored")).toInt(0));
            value.log_rank_groups.push_back(std::move(group));
        }
        value.gray_chi_square =
            read_optional(reliability.value(QStringLiteral("gray_chi_square")));
        value.gray_df = read_optional(reliability.value(QStringLiteral("gray_df")));
        value.gray_p_value =
            read_optional(reliability.value(QStringLiteral("gray_p_value")));
        if (reliability.contains(QStringLiteral("gray_group_count"))) {
            value.gray_group_count = static_cast<std::size_t>(
                reliability.value(QStringLiteral("gray_group_count")).toInt(0));
        }
        value.gray_not_computed_reason = reliability.value(QStringLiteral("gray_not_computed_reason"))
                                             .toString().toStdString();
        value.gray_algorithm_id = reliability.value(QStringLiteral("gray_algorithm_id"))
                                      .toString().toStdString();
        facts.reliability = std::move(value);
    }
    const QJsonObject warranty = serialized.value(QStringLiteral("warranty")).toObject();
    if (!warranty.isEmpty()) {
        domain::WarrantyFacts value;
        value.warranty_time = warranty.value(QStringLiteral("warranty_time")).toDouble(0.0);
        value.time_unit =
            warranty.value(QStringLiteral("time_unit")).toString().toStdString();
        value.exposure = warranty.value(QStringLiteral("exposure")).toDouble(0.0);
        value.reliability_at_warranty =
            warranty.value(QStringLiteral("reliability_at_warranty")).toDouble(0.0);
        value.failure_probability =
            warranty.value(QStringLiteral("failure_probability")).toDouble(0.0);
        value.expected_failures =
            warranty.value(QStringLiteral("expected_failures")).toDouble(0.0);
        value.claims_per_1000 =
            warranty.value(QStringLiteral("claims_per_1000")).toDouble(0.0);
        value.observed_failures = static_cast<std::size_t>(
            warranty.value(QStringLiteral("observed_failures")).toInt(0));
        value.censored_count = static_cast<std::size_t>(
            warranty.value(QStringLiteral("censored_count")).toInt(0));
        value.valid_count = static_cast<std::size_t>(
            warranty.value(QStringLiteral("valid_count")).toInt(0));
        value.model_name =
            warranty.value(QStringLiteral("model_name")).toString().toStdString();
        value.quantity_label =
            warranty.value(QStringLiteral("quantity_label"))
                .toString(QStringLiteral("prediction"))
                .toStdString();
        value.evidence_type =
            warranty.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference"))
                .toStdString();
        value.exposure_source =
            warranty.value(QStringLiteral("exposure_source")).toString().toStdString();
        value.exposure_row_count = static_cast<std::size_t>(
            warranty.value(QStringLiteral("exposure_row_count")).toInt(0));
        value.stratum_kind =
            warranty.value(QStringLiteral("stratum_kind")).toString().toStdString();
        value.uses_pooled_reliability =
            warranty.value(QStringLiteral("uses_pooled_reliability")).toBool(true);
        value.uses_mode_specific_reliability =
            warranty.value(QStringLiteral("uses_mode_specific_reliability")).toBool(false);
        const QJsonArray strata = warranty.value(QStringLiteral("strata")).toArray();
        for (const QJsonValue& entry : strata) {
            const QJsonObject item = entry.toObject();
            domain::WarrantyStratumFacts stratum;
            stratum.label = item.value(QStringLiteral("label")).toString().toStdString();
            stratum.kind = item.value(QStringLiteral("kind")).toString().toStdString();
            stratum.exposure = item.value(QStringLiteral("exposure")).toDouble(0.0);
            stratum.observed_failures = static_cast<std::size_t>(
                item.value(QStringLiteral("observed_failures")).toInt(0));
            stratum.censored_count = static_cast<std::size_t>(
                item.value(QStringLiteral("censored_count")).toInt(0));
            stratum.valid_count = static_cast<std::size_t>(
                item.value(QStringLiteral("valid_count")).toInt(0));
            stratum.expected_failures =
                item.value(QStringLiteral("expected_failures")).toDouble(0.0);
            stratum.share_of_total_exposure =
                item.value(QStringLiteral("share_of_total_exposure")).toDouble(0.0);
            stratum.exposure_attribution =
                item.value(QStringLiteral("exposure_attribution")).toString().toStdString();
            stratum.reliability_at_warranty =
                read_optional(item.value(QStringLiteral("reliability_at_warranty")));
            stratum.uses_mode_specific_reliability =
                item.value(QStringLiteral("uses_mode_specific_reliability")).toBool(false);
            for (const QJsonValue& row : item.value(QStringLiteral("source_rows")).toArray()) {
                stratum.source_rows.push_back(
                    static_cast<std::size_t>(row.toInt(0)));
            }
            value.strata.push_back(std::move(stratum));
        }
        facts.warranty = std::move(value);
    }
    const QJsonObject forecast = serialized.value(QStringLiteral("forecast")).toObject();
    if (!forecast.isEmpty()) {
        facts.forecast = domain::ForecastFacts{
            read_optional(forecast.value(QStringLiteral("mape"))),
            read_optional(forecast.value(QStringLiteral("mase"))),
            read_optional(forecast.value(QStringLiteral("rolling_origin_mape"))),
            read_optional(forecast.value(QStringLiteral("rolling_origin_mase")))};
    }
    const QJsonObject power_facts = serialized.value(QStringLiteral("power")).toObject();
    if (!power_facts.isEmpty()) {
        domain::PowerFacts value;
        value.power = read_optional(power_facts.value(QStringLiteral("power")));
        value.effect_size = read_optional(power_facts.value(QStringLiteral("effect_size")));
        value.mode = power_facts.value(QStringLiteral("mode")).toString().toStdString();
        value.sample_size = read_optional_size(power_facts.value(QStringLiteral("sample_size")));
        value.target = read_optional(power_facts.value(QStringLiteral("target")));
        value.actual_power = read_optional(power_facts.value(QStringLiteral("actual_power")));
        facts.power = std::move(value);
    }
    const QJsonObject descriptive = serialized.value(QStringLiteral("descriptive")).toObject();
    if (!descriptive.isEmpty()) {
        domain::DescriptiveFacts value;
        value.n = static_cast<std::size_t>(descriptive.value(QStringLiteral("n")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            descriptive.value(QStringLiteral("missing_count")).toInt(0));
        value.mean = read_optional(descriptive.value(QStringLiteral("mean")));
        value.standard_deviation =
            read_optional(descriptive.value(QStringLiteral("standard_deviation")));
        facts.descriptive = std::move(value);
    }
    const QJsonObject chi_square = serialized.value(QStringLiteral("chi_square")).toObject();
    if (!chi_square.isEmpty()) {
        domain::ChiSquareFacts value;
        value.statistic = read_optional(chi_square.value(QStringLiteral("statistic")));
        value.p_value = read_optional(chi_square.value(QStringLiteral("p_value")));
        value.degrees_of_freedom =
            read_optional(chi_square.value(QStringLiteral("degrees_of_freedom")));
        value.expected_count_warning =
            chi_square.value(QStringLiteral("expected_count_warning")).toBool(false);
        value.row_count = static_cast<std::size_t>(
            chi_square.value(QStringLiteral("row_count")).toInt(0));
        value.column_count = static_cast<std::size_t>(
            chi_square.value(QStringLiteral("column_count")).toInt(0));
        value.total_count = static_cast<std::size_t>(
            chi_square.value(QStringLiteral("total_count")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            chi_square.value(QStringLiteral("missing_count")).toInt(0));
        value.likelihood_ratio_statistic =
            read_optional(chi_square.value(QStringLiteral("likelihood_ratio_statistic")));
        value.likelihood_ratio_p_value =
            read_optional(chi_square.value(QStringLiteral("likelihood_ratio_p_value")));
        value.plot_available =
            chi_square.value(QStringLiteral("plot_available")).toBool(false);
        value.fisher_p_value =
            read_optional(chi_square.value(QStringLiteral("fisher_p_value")));
        value.odds_ratio =
            read_optional(chi_square.value(QStringLiteral("odds_ratio")));
        value.method = chi_square.value(QStringLiteral("method")).toString().toStdString();
        value.max_abs_adjusted_residual =
            read_optional(chi_square.value(QStringLiteral("max_abs_adjusted_residual")));
        value.largest_contribution_cell =
            chi_square.value(QStringLiteral("largest_contribution_cell")).toString().toStdString();
        value.residual_heatmap_available =
            chi_square.value(QStringLiteral("residual_heatmap_available")).toBool(false);
        value.percent_tables_available =
            chi_square.value(QStringLiteral("percent_tables_available")).toBool(false);
        facts.chi_square = std::move(value);
    }
    const QJsonObject cross_tab = serialized.value(QStringLiteral("cross_tab")).toObject();
    if (!cross_tab.isEmpty()) {
        domain::CrossTabFacts value;
        value.row_count = static_cast<std::size_t>(
            cross_tab.value(QStringLiteral("row_count")).toInt(0));
        value.column_count = static_cast<std::size_t>(
            cross_tab.value(QStringLiteral("column_count")).toInt(0));
        value.total_count = static_cast<std::size_t>(
            cross_tab.value(QStringLiteral("total_count")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            cross_tab.value(QStringLiteral("missing_count")).toInt(0));
        value.percent_tables_available =
            cross_tab.value(QStringLiteral("percent_tables_available")).toBool(false);
        facts.cross_tab = std::move(value);
    }
    const QJsonObject chi_square_gof = serialized.value(QStringLiteral("chi_square_gof")).toObject();
    if (!chi_square_gof.isEmpty()) {
        domain::ChiSquareGofFacts value;
        value.statistic = read_optional(chi_square_gof.value(QStringLiteral("statistic")));
        value.p_value = read_optional(chi_square_gof.value(QStringLiteral("p_value")));
        value.degrees_of_freedom =
            read_optional(chi_square_gof.value(QStringLiteral("degrees_of_freedom")));
        value.category_count = static_cast<std::size_t>(
            chi_square_gof.value(QStringLiteral("category_count")).toInt(0));
        value.total_count = static_cast<std::size_t>(
            chi_square_gof.value(QStringLiteral("total_count")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            chi_square_gof.value(QStringLiteral("missing_count")).toInt(0));
        value.expected_count_warning =
            chi_square_gof.value(QStringLiteral("expected_count_warning")).toBool(false);
        value.expected_below_five_count = static_cast<std::size_t>(
            chi_square_gof.value(QStringLiteral("expected_below_five_count")).toInt(0));
        value.minimum_expected_count =
            read_optional(chi_square_gof.value(QStringLiteral("minimum_expected_count")));
        value.validity_status = chi_square_gof.value(QStringLiteral("validity_status"))
                                    .toString(QStringLiteral("ok")).toStdString();
        value.recommendation = chi_square_gof.value(QStringLiteral("recommendation"))
                                   .toString().toStdString();
        value.plot_available =
            chi_square_gof.value(QStringLiteral("plot_available")).toBool(false);
        value.proportion_source = chi_square_gof.value(QStringLiteral("proportion_source"))
                                      .toString(QStringLiteral("equal")).toStdString();
        value.method = chi_square_gof.value(QStringLiteral("method")).toString().toStdString();
        value.lambda_hat = read_optional(chi_square_gof.value(QStringLiteral("lambda_hat")));
        facts.chi_square_gof = std::move(value);
    }
    const QJsonObject mcnemar = serialized.value(QStringLiteral("mcnemar")).toObject();
    if (!mcnemar.isEmpty()) {
        domain::McNemarFacts value;
        value.a = static_cast<std::size_t>(mcnemar.value(QStringLiteral("a")).toInt(0));
        value.b = static_cast<std::size_t>(mcnemar.value(QStringLiteral("b")).toInt(0));
        value.c = static_cast<std::size_t>(mcnemar.value(QStringLiteral("c")).toInt(0));
        value.d = static_cast<std::size_t>(mcnemar.value(QStringLiteral("d")).toInt(0));
        value.discordant = static_cast<std::size_t>(
            mcnemar.value(QStringLiteral("discordant")).toInt(0));
        value.pair_count = static_cast<std::size_t>(
            mcnemar.value(QStringLiteral("pair_count")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            mcnemar.value(QStringLiteral("missing_count")).toInt(0));
        value.chi_square = read_optional(mcnemar.value(QStringLiteral("chi_square")));
        value.p_value = read_optional(mcnemar.value(QStringLiteral("p_value")));
        value.degrees_of_freedom =
            mcnemar.value(QStringLiteral("degrees_of_freedom")).toDouble(1.0);
        value.continuity_correction =
            mcnemar.value(QStringLiteral("continuity_correction")).toBool(true);
        value.method = mcnemar.value(QStringLiteral("method"))
                           .toString(QStringLiteral("edwards"))
                           .toStdString();
        value.computable = mcnemar.value(QStringLiteral("computable")).toBool(false);
        facts.mcnemar = std::move(value);
    }
    const QJsonObject cochran = serialized.value(QStringLiteral("cochran_q")).toObject();
    if (!cochran.isEmpty()) {
        domain::CochranQFacts value;
        value.treatment_count = static_cast<std::size_t>(
            cochran.value(QStringLiteral("treatment_count")).toInt(0));
        value.subject_count = static_cast<std::size_t>(
            cochran.value(QStringLiteral("subject_count")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            cochran.value(QStringLiteral("missing_count")).toInt(0));
        value.q_statistic = read_optional(cochran.value(QStringLiteral("q_statistic")));
        value.p_value = read_optional(cochran.value(QStringLiteral("p_value")));
        value.degrees_of_freedom =
            cochran.value(QStringLiteral("degrees_of_freedom")).toDouble(0.0);
        value.computable = cochran.value(QStringLiteral("computable")).toBool(false);
        value.approximation = cochran.value(QStringLiteral("approximation"))
                                  .toString(QStringLiteral("chi_square"))
                                  .toStdString();
        facts.cochran_q = std::move(value);
    }
    const QJsonObject nonparametric = serialized.value(QStringLiteral("nonparametric")).toObject();
    if (!nonparametric.isEmpty()) {
        domain::NonparametricFacts value;
        value.method = nonparametric.value(QStringLiteral("method")).toString().toStdString();
        value.statistic = read_optional(nonparametric.value(QStringLiteral("statistic")));
        value.p_value = read_optional(nonparametric.value(QStringLiteral("p_value")));
        value.tie_correction = nonparametric.value(QStringLiteral("tie_correction")).toBool(false);
        value.continuity_correction =
            nonparametric.value(QStringLiteral("continuity_correction")).toBool(true);
        value.approximation = nonparametric.value(QStringLiteral("approximation"))
                                  .toString(QStringLiteral("normal")).toStdString();
        value.small_sample_warning =
            nonparametric.value(QStringLiteral("small_sample_warning")).toBool(false);
        value.effect_size = read_optional(nonparametric.value(QStringLiteral("effect_size")));
        value.p_value_unadjusted =
            read_optional(nonparametric.value(QStringLiteral("p_value_unadjusted")));
        value.group_count = static_cast<std::size_t>(
            nonparametric.value(QStringLiteral("group_count")).toInt(0));
        value.plot_point_count = static_cast<std::size_t>(
            nonparametric.value(QStringLiteral("plot_point_count")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            nonparametric.value(QStringLiteral("missing_count")).toInt(0));
        value.location_estimate =
            read_optional(nonparametric.value(QStringLiteral("location_estimate")));
        value.ci_lower = read_optional(nonparametric.value(QStringLiteral("ci_lower")));
        value.ci_upper = read_optional(nonparametric.value(QStringLiteral("ci_upper")));
        value.dunn_available =
            nonparametric.value(QStringLiteral("dunn_available")).toBool(false);
        value.steel_dwass_available =
            nonparametric.value(QStringLiteral("steel_dwass_available")).toBool(false);
        value.nemenyi_available =
            nonparametric.value(QStringLiteral("nemenyi_available")).toBool(false);
        value.posthoc_method = nonparametric.value(QStringLiteral("posthoc_method"))
                                   .toString(QStringLiteral("dunn")).toStdString();
        value.posthoc_pair_count = static_cast<std::size_t>(
            nonparametric.value(QStringLiteral("posthoc_pair_count")).toInt(0));
        value.grouping_letter_count = static_cast<std::size_t>(
            nonparametric.value(QStringLiteral("grouping_letter_count")).toInt(0));
        facts.nonparametric = std::move(value);
    }
    const QJsonObject logistic = serialized.value(QStringLiteral("logistic")).toObject();
    if (!logistic.isEmpty()) {
        domain::LogisticFacts value;
        value.converged = logistic.value(QStringLiteral("converged")).toBool(false);
        value.complete_separation =
            logistic.value(QStringLiteral("complete_separation")).toBool(false);
        value.hosmer_lemeshow_p =
            read_optional(logistic.value(QStringLiteral("hosmer_lemeshow_p")));
        value.hosmer_lemeshow_statistic =
            read_optional(logistic.value(QStringLiteral("hosmer_lemeshow_statistic")));
        if (logistic.contains(QStringLiteral("hosmer_lemeshow_df"))) {
            value.hosmer_lemeshow_df =
                static_cast<std::size_t>(
                    logistic.value(QStringLiteral("hosmer_lemeshow_df")).toInteger(0));
        }
        value.hosmer_lemeshow_groups =
            static_cast<std::size_t>(
                logistic.value(QStringLiteral("hosmer_lemeshow_groups")).toInteger(0));
        value.hosmer_lemeshow_status =
            logistic.value(QStringLiteral("hosmer_lemeshow_status"))
                .toString(QStringLiteral("not_computed")).toStdString();
        value.high_leverage_count =
            static_cast<std::size_t>(
                logistic.value(QStringLiteral("high_leverage_count")).toInteger(0));
        value.leverage_threshold =
            read_optional(logistic.value(QStringLiteral("leverage_threshold")));
        value.maximum_leverage =
            read_optional(logistic.value(QStringLiteral("maximum_leverage")));
        value.maximum_vif =
            read_optional(logistic.value(QStringLiteral("maximum_vif")));
        if (logistic.contains(QStringLiteral("concordant_pairs"))) {
            value.concordant_pairs = static_cast<std::size_t>(
                logistic.value(QStringLiteral("concordant_pairs")).toInteger(0));
            value.discordant_pairs = static_cast<std::size_t>(
                logistic.value(QStringLiteral("discordant_pairs")).toInteger(0));
            value.tied_pairs = static_cast<std::size_t>(
                logistic.value(QStringLiteral("tied_pairs")).toInteger(0));
            value.pairs_concordance_percent = read_optional(
                logistic.value(QStringLiteral("pairs_concordance_percent")));
            value.true_positive = static_cast<std::size_t>(
                logistic.value(QStringLiteral("true_positive")).toInteger(0));
            value.true_negative = static_cast<std::size_t>(
                logistic.value(QStringLiteral("true_negative")).toInteger(0));
            value.false_positive = static_cast<std::size_t>(
                logistic.value(QStringLiteral("false_positive")).toInteger(0));
            value.false_negative = static_cast<std::size_t>(
                logistic.value(QStringLiteral("false_negative")).toInteger(0));
        }
        value.stepwise_method =
            logistic.value(QStringLiteral("stepwise_method")).toString().toStdString();
        value.stepwise_criterion =
            logistic.value(QStringLiteral("stepwise_criterion")).toString().toStdString();
        value.stepwise_step_count = static_cast<std::size_t>(
            logistic.value(QStringLiteral("stepwise_step_count")).toInteger(0));
        value.stepwise_selected_count = static_cast<std::size_t>(
            logistic.value(QStringLiteral("stepwise_selected_count")).toInteger(0));
        value.stepwise_best_step_index = static_cast<std::size_t>(
            logistic.value(QStringLiteral("stepwise_best_step_index")).toInteger(0));
        value.stepwise_log_likelihood =
            read_optional(logistic.value(QStringLiteral("stepwise_log_likelihood")));
        value.stepwise_aic = read_optional(logistic.value(QStringLiteral("stepwise_aic")));
        value.stepwise_bic = read_optional(logistic.value(QStringLiteral("stepwise_bic")));
        for (const QJsonValue& entry :
             logistic.value(QStringLiteral("stepwise_steps")).toArray()) {
            const QJsonObject item = entry.toObject();
            domain::LogisticStepwiseStepFacts step;
            step.step = static_cast<std::size_t>(item.value(QStringLiteral("step")).toInt(0));
            step.action = item.value(QStringLiteral("action")).toString().toStdString();
            step.term = item.value(QStringLiteral("term")).toString().toStdString();
            step.deviance = read_optional(item.value(QStringLiteral("deviance")));
            step.aic = read_optional(item.value(QStringLiteral("aic")));
            step.aicc = read_optional(item.value(QStringLiteral("aicc")));
            step.bic = read_optional(item.value(QStringLiteral("bic")));
            step.enter_p = read_optional(item.value(QStringLiteral("enter_p")));
            step.remove_p = read_optional(item.value(QStringLiteral("remove_p")));
            value.stepwise_steps.push_back(std::move(step));
        }
        facts.logistic = std::move(value);
    }
    const QJsonObject distribution_id =
        serialized.value(QStringLiteral("distribution_identification")).toObject();
    if (!distribution_id.isEmpty()) {
        domain::DistributionIdentificationFacts value;
        value.best_distribution =
            distribution_id.value(QStringLiteral("best_distribution"))
                .toString().toStdString();
        value.best_anderson_darling =
            read_optional(distribution_id.value(QStringLiteral("best_anderson_darling")));
        value.best_p_value =
            read_optional(distribution_id.value(QStringLiteral("best_p_value")));
        value.did_not_change_capability_defaults =
            distribution_id.value(QStringLiteral("did_not_change_capability_defaults"))
                .toBool(true);
        facts.distribution_identification = std::move(value);
    }
    const QJsonObject pca = serialized.value(QStringLiteral("pca")).toObject();
    if (!pca.isEmpty()) {
        domain::PcaFacts value;
        value.mode = pca.value(QStringLiteral("mode"))
                         .toString(QStringLiteral("covariance")).toStdString();
        value.retained_component_count = static_cast<std::size_t>(
            pca.value(QStringLiteral("retained_component_count")).toInt(0));
        value.anomaly_count = static_cast<std::size_t>(
            pca.value(QStringLiteral("anomaly_count")).toInt(0));
        value.observation_count = static_cast<std::size_t>(
            pca.value(QStringLiteral("observation_count")).toInt(0));
        value.t2_limit = read_optional(pca.value(QStringLiteral("t2_limit")));
        value.q_limit = read_optional(pca.value(QStringLiteral("q_limit")));
        value.residual_ad_p = read_optional(pca.value(QStringLiteral("residual_ad_p")));
        value.diagnostic_plot_count = static_cast<std::size_t>(
            pca.value(QStringLiteral("diagnostic_plot_count")).toInt(0));
        value.converged = pca.value(QStringLiteral("converged")).toBool(false);
        facts.pca = std::move(value);
    }
    const QJsonObject kmeans = serialized.value(QStringLiteral("kmeans")).toObject();
    if (!kmeans.isEmpty()) {
        domain::KMeansFacts value;
        value.k = static_cast<std::size_t>(kmeans.value(QStringLiteral("k")).toInt(0));
        value.n = static_cast<std::size_t>(kmeans.value(QStringLiteral("n")).toInt(0));
        value.variable_count = static_cast<std::size_t>(
            kmeans.value(QStringLiteral("variable_count")).toInt(0));
        value.iterations = static_cast<std::size_t>(
            kmeans.value(QStringLiteral("iterations")).toInt(0));
        value.converged = kmeans.value(QStringLiteral("converged")).toBool(false);
        value.standardized = kmeans.value(QStringLiteral("standardized")).toBool(false);
        value.total_within_ss =
            read_optional(kmeans.value(QStringLiteral("total_within_ss")));
        facts.kmeans = std::move(value);
    }
    const QJsonObject cart_tree = serialized.value(QStringLiteral("cart_tree")).toObject();
    if (!cart_tree.isEmpty()) {
        domain::CartTreeFacts value;
        value.task = cart_tree.value(QStringLiteral("task"))
                         .toString(QStringLiteral("classification")).toStdString();
        value.n = static_cast<std::size_t>(cart_tree.value(QStringLiteral("n")).toInt(0));
        value.predictor_count = static_cast<std::size_t>(
            cart_tree.value(QStringLiteral("predictor_count")).toInt(0));
        value.max_depth = static_cast<std::size_t>(
            cart_tree.value(QStringLiteral("max_depth")).toInt(0));
        value.node_count = static_cast<std::size_t>(
            cart_tree.value(QStringLiteral("node_count")).toInt(0));
        value.leaf_count = static_cast<std::size_t>(
            cart_tree.value(QStringLiteral("leaf_count")).toInt(0));
        value.train_metric =
            read_optional(cart_tree.value(QStringLiteral("train_metric")));
        value.top_variable = cart_tree.value(QStringLiteral("top_variable"))
                                 .toString().toStdString();
        facts.cart_tree = std::move(value);
    }
    const QJsonObject adf = serialized.value(QStringLiteral("adf")).toObject();
    if (!adf.isEmpty()) {
        domain::AdfFacts value;
        value.n = static_cast<std::size_t>(adf.value(QStringLiteral("n")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            adf.value(QStringLiteral("missing_count")).toInt(0));
        value.lags = static_cast<std::size_t>(adf.value(QStringLiteral("lags")).toInt(0));
        value.used_observations = static_cast<std::size_t>(
            adf.value(QStringLiteral("used_observations")).toInt(0));
        value.regression = adf.value(QStringLiteral("regression"))
                               .toString(QStringLiteral("drift")).toStdString();
        value.tau = read_optional(adf.value(QStringLiteral("tau")));
        value.critical_5 = read_optional(adf.value(QStringLiteral("critical_5")));
        value.reject_unit_root_at_5 =
            adf.value(QStringLiteral("reject_unit_root_at_5")).toBool(false);
        facts.adf = std::move(value);
    }
    const QJsonObject poisson_regression =
        serialized.value(QStringLiteral("poisson_regression")).toObject();
    if (!poisson_regression.isEmpty()) {
        domain::PoissonRegressionFacts value;
        value.n = static_cast<std::size_t>(
            poisson_regression.value(QStringLiteral("n")).toInt(0));
        value.predictor_count = static_cast<std::size_t>(
            poisson_regression.value(QStringLiteral("predictor_count")).toInt(0));
        value.iteration_count = static_cast<std::size_t>(
            poisson_regression.value(QStringLiteral("iteration_count")).toInt(0));
        value.converged = poisson_regression.value(QStringLiteral("converged")).toBool(false);
        value.deviance = read_optional(poisson_regression.value(QStringLiteral("deviance")));
        value.aic = read_optional(poisson_regression.value(QStringLiteral("aic")));
        facts.poisson_regression = std::move(value);
    }
    const QJsonObject isolation_forest =
        serialized.value(QStringLiteral("isolation_forest")).toObject();
    if (!isolation_forest.isEmpty()) {
        domain::IsolationForestFacts value;
        value.n = static_cast<std::size_t>(
            isolation_forest.value(QStringLiteral("n")).toInt(0));
        value.variable_count = static_cast<std::size_t>(
            isolation_forest.value(QStringLiteral("variable_count")).toInt(0));
        value.tree_count = static_cast<std::size_t>(
            isolation_forest.value(QStringLiteral("tree_count")).toInt(0));
        value.anomaly_count = static_cast<std::size_t>(
            isolation_forest.value(QStringLiteral("anomaly_count")).toInt(0));
        value.score_threshold =
            read_optional(isolation_forest.value(QStringLiteral("score_threshold")));
        facts.isolation_forest = std::move(value);
    }
    const QJsonObject bootstrap_mean =
        serialized.value(QStringLiteral("bootstrap_mean")).toObject();
    if (!bootstrap_mean.isEmpty()) {
        domain::BootstrapMeanFacts value;
        value.n = static_cast<std::size_t>(
            bootstrap_mean.value(QStringLiteral("n")).toInt(0));
        value.replicates = static_cast<std::size_t>(
            bootstrap_mean.value(QStringLiteral("replicates")).toInt(0));
        value.method = bootstrap_mean.value(QStringLiteral("method"))
                           .toString(QStringLiteral("percentile")).toStdString();
        value.sample_mean =
            read_optional(bootstrap_mean.value(QStringLiteral("sample_mean")));
        value.ci_lower = read_optional(bootstrap_mean.value(QStringLiteral("ci_lower")));
        value.ci_upper = read_optional(bootstrap_mean.value(QStringLiteral("ci_upper")));
        value.confidence_level =
            bootstrap_mean.value(QStringLiteral("confidence_level")).toDouble(0.95);
        facts.bootstrap_mean = std::move(value);
    }
    const QJsonObject bootstrap_two_sample =
        serialized.value(QStringLiteral("bootstrap_two_sample")).toObject();
    if (!bootstrap_two_sample.isEmpty()) {
        domain::BootstrapTwoSampleFacts value;
        value.n_first = static_cast<std::size_t>(
            bootstrap_two_sample.value(QStringLiteral("n_first")).toInt(0));
        value.n_second = static_cast<std::size_t>(
            bootstrap_two_sample.value(QStringLiteral("n_second")).toInt(0));
        value.replicates = static_cast<std::size_t>(
            bootstrap_two_sample.value(QStringLiteral("replicates")).toInt(0));
        value.method = bootstrap_two_sample.value(QStringLiteral("method"))
                           .toString(QStringLiteral("percentile")).toStdString();
        value.mean_first =
            read_optional(bootstrap_two_sample.value(QStringLiteral("mean_first")));
        value.mean_second =
            read_optional(bootstrap_two_sample.value(QStringLiteral("mean_second")));
        value.mean_difference =
            read_optional(bootstrap_two_sample.value(QStringLiteral("mean_difference")));
        value.ci_lower =
            read_optional(bootstrap_two_sample.value(QStringLiteral("ci_lower")));
        value.ci_upper =
            read_optional(bootstrap_two_sample.value(QStringLiteral("ci_upper")));
        value.confidence_level =
            bootstrap_two_sample.value(QStringLiteral("confidence_level")).toDouble(0.95);
        facts.bootstrap_two_sample = std::move(value);
    }
    const QJsonObject probit_reliability =
        serialized.value(QStringLiteral("probit_reliability")).toObject();
    if (!probit_reliability.isEmpty()) {
        domain::ProbitReliabilityFacts value;
        value.n = static_cast<std::size_t>(
            probit_reliability.value(QStringLiteral("n")).toInt(0));
        value.iteration_count = static_cast<std::size_t>(
            probit_reliability.value(QStringLiteral("iteration_count")).toInt(0));
        value.converged = probit_reliability.value(QStringLiteral("converged")).toBool(false);
        value.link = probit_reliability.value(QStringLiteral("link"))
                         .toString(QStringLiteral("logit")).toStdString();
        value.intercept =
            read_optional(probit_reliability.value(QStringLiteral("intercept")));
        value.stress_coefficient =
            read_optional(probit_reliability.value(QStringLiteral("stress_coefficient")));
        value.ld50 = read_optional(probit_reliability.value(QStringLiteral("ld50")));
        value.ld50_standard_error =
            read_optional(probit_reliability.value(QStringLiteral("ld50_standard_error")));
        value.ld50_confidence_lower =
            read_optional(probit_reliability.value(QStringLiteral("ld50_confidence_lower")));
        value.ld50_confidence_upper =
            read_optional(probit_reliability.value(QStringLiteral("ld50_confidence_upper")));
        value.log_likelihood =
            read_optional(probit_reliability.value(QStringLiteral("log_likelihood")));
        value.deviance =
            read_optional(probit_reliability.value(QStringLiteral("deviance")));
        value.aic = read_optional(probit_reliability.value(QStringLiteral("aic")));
        facts.probit_reliability = std::move(value);
    }
    const QJsonObject hierarchical_cluster =
        serialized.value(QStringLiteral("hierarchical_cluster")).toObject();
    if (!hierarchical_cluster.isEmpty()) {
        domain::HierarchicalClusterFacts value;
        value.n = static_cast<std::size_t>(
            hierarchical_cluster.value(QStringLiteral("n")).toInt(0));
        value.variable_count = static_cast<std::size_t>(
            hierarchical_cluster.value(QStringLiteral("variable_count")).toInt(0));
        value.cluster_count = static_cast<std::size_t>(
            hierarchical_cluster.value(QStringLiteral("cluster_count")).toInt(0));
        value.merge_count = static_cast<std::size_t>(
            hierarchical_cluster.value(QStringLiteral("merge_count")).toInt(0));
        value.linkage = hierarchical_cluster.value(QStringLiteral("linkage"))
                            .toString(QStringLiteral("complete")).toStdString();
        value.standardized =
            hierarchical_cluster.value(QStringLiteral("standardized")).toBool(false);
        facts.hierarchical_cluster = std::move(value);
    }
    const QJsonObject ordinal_logistic =
        serialized.value(QStringLiteral("ordinal_logistic")).toObject();
    if (!ordinal_logistic.isEmpty()) {
        domain::OrdinalLogisticFacts value;
        value.n = static_cast<std::size_t>(
            ordinal_logistic.value(QStringLiteral("n")).toInt(0));
        value.category_count = static_cast<std::size_t>(
            ordinal_logistic.value(QStringLiteral("category_count")).toInt(0));
        value.predictor_count = static_cast<std::size_t>(
            ordinal_logistic.value(QStringLiteral("predictor_count")).toInt(0));
        value.iteration_count = static_cast<std::size_t>(
            ordinal_logistic.value(QStringLiteral("iteration_count")).toInt(0));
        value.converged = ordinal_logistic.value(QStringLiteral("converged")).toBool(false);
        value.log_likelihood =
            read_optional(ordinal_logistic.value(QStringLiteral("log_likelihood")));
        value.aic = read_optional(ordinal_logistic.value(QStringLiteral("aic")));
        facts.ordinal_logistic = std::move(value);
    }
    const QJsonObject discriminant =
        serialized.value(QStringLiteral("discriminant")).toObject();
    if (!discriminant.isEmpty()) {
        domain::DiscriminantFacts value;
        value.n = static_cast<std::size_t>(discriminant.value(QStringLiteral("n")).toInt(0));
        value.class_count = static_cast<std::size_t>(
            discriminant.value(QStringLiteral("class_count")).toInt(0));
        value.predictor_count = static_cast<std::size_t>(
            discriminant.value(QStringLiteral("predictor_count")).toInt(0));
        value.train_accuracy =
            read_optional(discriminant.value(QStringLiteral("train_accuracy")));
        facts.discriminant = std::move(value);
    }
    const QJsonObject ccf = serialized.value(QStringLiteral("ccf")).toObject();
    if (!ccf.isEmpty()) {
        domain::CcfFacts value;
        value.n = static_cast<std::size_t>(ccf.value(QStringLiteral("n")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            ccf.value(QStringLiteral("missing_count")).toInt(0));
        value.max_lag = static_cast<std::size_t>(ccf.value(QStringLiteral("max_lag")).toInt(0));
        value.band_half_width = read_optional(ccf.value(QStringLiteral("band_half_width")));
        value.ccf_at_zero = read_optional(ccf.value(QStringLiteral("ccf_at_zero")));
        facts.ccf = std::move(value);
    }
    const QJsonObject correlogram =
        serialized.value(QStringLiteral("correlogram")).toObject();
    if (!correlogram.isEmpty()) {
        domain::CorrelogramFacts value;
        value.variable_count = static_cast<std::size_t>(
            correlogram.value(QStringLiteral("variable_count")).toInt(0));
        value.method = correlogram.value(QStringLiteral("method"))
                           .toString(QStringLiteral("pearson")).toStdString();
        value.pair_count = static_cast<std::size_t>(
            correlogram.value(QStringLiteral("pair_count")).toInt(0));
        facts.correlogram = std::move(value);
    }
    const QJsonObject stepwise_regression =
        serialized.value(QStringLiteral("stepwise_regression")).toObject();
    if (!stepwise_regression.isEmpty()) {
        domain::StepwiseRegressionFacts value;
        value.n = static_cast<std::size_t>(
            stepwise_regression.value(QStringLiteral("n")).toInt(0));
        value.candidate_count = static_cast<std::size_t>(
            stepwise_regression.value(QStringLiteral("candidate_count")).toInt(0));
        value.selected_count = static_cast<std::size_t>(
            stepwise_regression.value(QStringLiteral("selected_count")).toInt(0));
        value.step_count = static_cast<std::size_t>(
            stepwise_regression.value(QStringLiteral("step_count")).toInt(0));
        value.method = stepwise_regression.value(QStringLiteral("method"))
                           .toString(QStringLiteral("stepwise")).toStdString();
        value.criterion = stepwise_regression.value(QStringLiteral("criterion"))
                              .toString(QStringLiteral("alpha")).toStdString();
        value.best_step_index = static_cast<std::size_t>(
            stepwise_regression.value(QStringLiteral("best_step_index")).toInt(0));
        value.r_squared =
            read_optional(stepwise_regression.value(QStringLiteral("r_squared")));
        value.adjusted_r_squared =
            read_optional(stepwise_regression.value(QStringLiteral("adjusted_r_squared")));
        value.best_aicc =
            read_optional(stepwise_regression.value(QStringLiteral("best_aicc")));
        value.best_bic =
            read_optional(stepwise_regression.value(QStringLiteral("best_bic")));
        facts.stepwise_regression = std::move(value);
    }
    const QJsonObject nominal_logistic =
        serialized.value(QStringLiteral("nominal_logistic")).toObject();
    if (!nominal_logistic.isEmpty()) {
        domain::NominalLogisticFacts value;
        value.n = static_cast<std::size_t>(
            nominal_logistic.value(QStringLiteral("n")).toInt(0));
        value.category_count = static_cast<std::size_t>(
            nominal_logistic.value(QStringLiteral("category_count")).toInt(0));
        value.logit_count = static_cast<std::size_t>(
            nominal_logistic.value(QStringLiteral("logit_count")).toInt(0));
        value.converged = nominal_logistic.value(QStringLiteral("converged")).toBool(false);
        value.reference_category = nominal_logistic.value(QStringLiteral("reference_category"))
                                       .toString().toStdString();
        value.log_likelihood =
            read_optional(nominal_logistic.value(QStringLiteral("log_likelihood")));
        value.aic = read_optional(nominal_logistic.value(QStringLiteral("aic")));
        value.g_p_value = read_optional(nominal_logistic.value(QStringLiteral("g_p_value")));
        facts.nominal_logistic = std::move(value);
    }
    const QJsonObject nonparametric_capability =
        serialized.value(QStringLiteral("nonparametric_capability")).toObject();
    if (!nonparametric_capability.isEmpty()) {
        domain::NonparametricCapabilityFacts value;
        value.n = static_cast<std::size_t>(
            nonparametric_capability.value(QStringLiteral("n")).toInt(0));
        value.tolerance_k = nonparametric_capability.value(QStringLiteral("tolerance_k"))
                                .toDouble(6.0);
        value.cnp = read_optional(nonparametric_capability.value(QStringLiteral("cnp")));
        value.cnpl = read_optional(nonparametric_capability.value(QStringLiteral("cnpl")));
        value.cnpu = read_optional(nonparametric_capability.value(QStringLiteral("cnpu")));
        value.cnpk = read_optional(nonparametric_capability.value(QStringLiteral("cnpk")));
        value.median = read_optional(nonparametric_capability.value(QStringLiteral("median")));
        value.lower_percentile =
            read_optional(nonparametric_capability.value(QStringLiteral("lower_percentile")));
        value.upper_percentile =
            read_optional(nonparametric_capability.value(QStringLiteral("upper_percentile")));
        value.observed_ppm_below =
            read_optional(nonparametric_capability.value(QStringLiteral("observed_ppm_below")));
        value.observed_ppm_above =
            read_optional(nonparametric_capability.value(QStringLiteral("observed_ppm_above")));
        value.observed_ppm_total =
            read_optional(nonparametric_capability.value(QStringLiteral("observed_ppm_total")));
        facts.nonparametric_capability = std::move(value);
    }
    const QJsonObject cox_regression =
        serialized.value(QStringLiteral("cox_regression")).toObject();
    if (!cox_regression.isEmpty()) {
        domain::CoxRegressionFacts value;
        value.n = static_cast<std::size_t>(cox_regression.value(QStringLiteral("n")).toInt(0));
        value.events = static_cast<std::size_t>(
            cox_regression.value(QStringLiteral("events")).toInt(0));
        value.censored = static_cast<std::size_t>(
            cox_regression.value(QStringLiteral("censored")).toInt(0));
        value.converged = cox_regression.value(QStringLiteral("converged")).toBool(false);
        value.log_likelihood =
            read_optional(cox_regression.value(QStringLiteral("log_likelihood")));
        value.evidence_type =
            cox_regression.value(QStringLiteral("evidence_type")).toString().toStdString();
        value.algorithm_id =
            cox_regression.value(QStringLiteral("algorithm_id")).toString().toStdString();
        value.ties_method =
            cox_regression.value(QStringLiteral("ties_method")).toString().toStdString();
        for (const QJsonValue& entry :
             cox_regression.value(QStringLiteral("coefficients")).toArray()) {
            const QJsonObject item = entry.toObject();
            domain::CoxRegressionCoefficientFacts term;
            term.term = item.value(QStringLiteral("term")).toString().toStdString();
            term.beta = read_optional(item.value(QStringLiteral("beta")));
            term.se = read_optional(item.value(QStringLiteral("se")));
            term.z = read_optional(item.value(QStringLiteral("z")));
            term.p_value = read_optional(item.value(QStringLiteral("p_value")));
            term.hazard_ratio = read_optional(item.value(QStringLiteral("hazard_ratio")));
            term.ci_lower = read_optional(item.value(QStringLiteral("ci_lower")));
            term.ci_upper = read_optional(item.value(QStringLiteral("ci_upper")));
            value.coefficients.push_back(std::move(term));
        }
        facts.cox_regression = std::move(value);
    }
    const QJsonObject accelerated_life =
        serialized.value(QStringLiteral("accelerated_life")).toObject();
    if (!accelerated_life.isEmpty()) {
        domain::AcceleratedLifeFacts value;
        value.n = static_cast<std::size_t>(
            accelerated_life.value(QStringLiteral("n")).toInt(0));
        value.failure_count = static_cast<std::size_t>(
            accelerated_life.value(QStringLiteral("failure_count")).toInt(0));
        value.censored_count = static_cast<std::size_t>(
            accelerated_life.value(QStringLiteral("censored_count")).toInt(0));
        value.stress_level_count = static_cast<std::size_t>(
            accelerated_life.value(QStringLiteral("stress_level_count")).toInt(0));
        value.converged = accelerated_life.value(QStringLiteral("converged")).toBool(false);
        value.transform = accelerated_life.value(QStringLiteral("transform"))
                              .toString(QStringLiteral("arrhenius")).toStdString();
        value.shape = read_optional(accelerated_life.value(QStringLiteral("shape")));
        value.log_likelihood =
            read_optional(accelerated_life.value(QStringLiteral("log_likelihood")));
        value.use_stress_celsius =
            read_optional(accelerated_life.value(QStringLiteral("use_stress_celsius")));
        value.b10_at_use_stress =
            read_optional(accelerated_life.value(QStringLiteral("b10_at_use_stress")));
        value.b50_at_use_stress =
            read_optional(accelerated_life.value(QStringLiteral("b50_at_use_stress")));
        value.b90_at_use_stress =
            read_optional(accelerated_life.value(QStringLiteral("b90_at_use_stress")));
        facts.accelerated_life = std::move(value);
    }
    const QJsonObject best_subsets_regression =
        serialized.value(QStringLiteral("best_subsets_regression")).toObject();
    if (!best_subsets_regression.isEmpty()) {
        domain::BestSubsetsRegressionFacts value;
        value.n = static_cast<std::size_t>(
            best_subsets_regression.value(QStringLiteral("n")).toInt(0));
        value.candidate_count = static_cast<std::size_t>(
            best_subsets_regression.value(QStringLiteral("candidate_count")).toInt(0));
        value.model_count = static_cast<std::size_t>(
            best_subsets_regression.value(QStringLiteral("model_count")).toInt(0));
        value.models_per_size = static_cast<std::size_t>(
            best_subsets_regression.value(QStringLiteral("models_per_size")).toInt(1));
        value.best_r_squared =
            read_optional(best_subsets_regression.value(QStringLiteral("best_r_squared")));
        value.best_adjusted_r_squared = read_optional(
            best_subsets_regression.value(QStringLiteral("best_adjusted_r_squared")));
        if (best_subsets_regression.contains(QStringLiteral("best_predictor_count"))) {
            value.best_predictor_count = static_cast<std::size_t>(
                best_subsets_regression.value(QStringLiteral("best_predictor_count")).toInt(0));
        }
        facts.best_subsets_regression = std::move(value);
    }
    const QJsonObject batch_capability =
        serialized.value(QStringLiteral("batch_capability")).toObject();
    if (!batch_capability.isEmpty()) {
        domain::BatchCapabilityFacts value;
        value.batch_count = static_cast<std::size_t>(
            batch_capability.value(QStringLiteral("batch_count")).toInt(0));
        value.skipped_batch_count = static_cast<std::size_t>(
            batch_capability.value(QStringLiteral("skipped_batch_count")).toInt(0));
        value.total_observations = static_cast<std::size_t>(
            batch_capability.value(QStringLiteral("total_observations")).toInt(0));
        facts.batch_capability = std::move(value);
    }
    const QJsonObject km_interval =
        serialized.value(QStringLiteral("km_interval")).toObject();
    if (!km_interval.isEmpty()) {
        domain::KmIntervalFacts value;
        value.n = static_cast<std::size_t>(km_interval.value(QStringLiteral("n")).toInt(0));
        value.exact_count = static_cast<std::size_t>(
            km_interval.value(QStringLiteral("exact_count")).toInt(0));
        value.left_censored_count = static_cast<std::size_t>(
            km_interval.value(QStringLiteral("left_censored_count")).toInt(0));
        value.right_censored_count = static_cast<std::size_t>(
            km_interval.value(QStringLiteral("right_censored_count")).toInt(0));
        value.interval_censored_count = static_cast<std::size_t>(
            km_interval.value(QStringLiteral("interval_censored_count")).toInt(0));
        value.iteration_count = static_cast<std::size_t>(
            km_interval.value(QStringLiteral("iteration_count")).toInt(0));
        value.converged = km_interval.value(QStringLiteral("converged")).toBool(false);
        value.identifiable = km_interval.value(QStringLiteral("identifiable")).toBool(false);
        value.median_life = read_optional(km_interval.value(QStringLiteral("median_life")));
        value.evidence_type =
            km_interval.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference"))
                .toStdString();
        value.algorithm_id =
            km_interval.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("turnbull_npmle_simplified_grid"))
                .toStdString();
        value.gate_status =
            km_interval.value(QStringLiteral("gate_status"))
                .toString(QStringLiteral("open_with_limits"))
                .toStdString();
        value.research_preview =
            km_interval.value(QStringLiteral("research_preview")).toBool(false);
        value.classic_km_equivalent =
            km_interval.value(QStringLiteral("classic_km_equivalent")).toBool(false);
        // Never deserialize a vendor claim for simplified Turnbull.
        if (value.evidence_type == "vendor_oracle" || value.evidence_type == "golden") {
            value.evidence_type = "formula_reference";
        }
        facts.km_interval = std::move(value);
    }
    const QJsonObject plackett_burman =
        serialized.value(QStringLiteral("plackett_burman")).toObject();
    if (!plackett_burman.isEmpty()) {
        domain::PlackettBurmanFacts value;
        value.factor_count = static_cast<std::size_t>(
            plackett_burman.value(QStringLiteral("factor_count")).toInt(0));
        value.run_count = static_cast<std::size_t>(
            plackett_burman.value(QStringLiteral("run_count")).toInt(0));
        value.center_point_count = static_cast<std::size_t>(
            plackett_burman.value(QStringLiteral("center_point_count")).toInt(0));
        facts.plackett_burman = std::move(value);
    }
    const QJsonObject random_forest =
        serialized.value(QStringLiteral("random_forest")).toObject();
    if (!random_forest.isEmpty()) {
        domain::RandomForestFacts value;
        value.task = random_forest.value(QStringLiteral("task"))
                         .toString(QStringLiteral("classification")).toStdString();
        value.n = static_cast<std::size_t>(random_forest.value(QStringLiteral("n")).toInt(0));
        value.predictor_count = static_cast<std::size_t>(
            random_forest.value(QStringLiteral("predictor_count")).toInt(0));
        value.n_trees = static_cast<std::size_t>(
            random_forest.value(QStringLiteral("n_trees")).toInt(0));
        value.max_depth = static_cast<std::size_t>(
            random_forest.value(QStringLiteral("max_depth")).toInt(0));
        value.train_metric =
            read_optional(random_forest.value(QStringLiteral("train_metric")));
        value.oob_metric =
            read_optional(random_forest.value(QStringLiteral("oob_metric")));
        value.top_variable =
            random_forest.value(QStringLiteral("top_variable")).toString().toStdString();
        value.disclosure =
            random_forest.value(QStringLiteral("disclosure")).toString().toStdString();
        value.evidence_type =
            random_forest.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            random_forest.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("bagged_cart_random_forest")).toStdString();
        facts.random_forest = std::move(value);
    }
    const QJsonObject weibayes = serialized.value(QStringLiteral("weibayes")).toObject();
    if (!weibayes.isEmpty()) {
        domain::WeibayesFacts value;
        value.n = static_cast<std::size_t>(weibayes.value(QStringLiteral("n")).toInt(0));
        value.failure_count = static_cast<std::size_t>(
            weibayes.value(QStringLiteral("failure_count")).toInt(0));
        value.censored_count = static_cast<std::size_t>(
            weibayes.value(QStringLiteral("censored_count")).toInt(0));
        value.shape_prior = weibayes.value(QStringLiteral("shape_prior")).toDouble(2.0);
        value.scale = read_optional(weibayes.value(QStringLiteral("scale")));
        value.zero_failure_bound =
            weibayes.value(QStringLiteral("zero_failure_bound")).toBool(false);
        value.b10 = read_optional(weibayes.value(QStringLiteral("b10")));
        value.b50 = read_optional(weibayes.value(QStringLiteral("b50")));
        value.b90 = read_optional(weibayes.value(QStringLiteral("b90")));
        value.evidence_type =
            weibayes.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            weibayes.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("weibayes_fixed_shape")).toStdString();
        facts.weibayes = std::move(value);
    }
    const QJsonObject taguchi_orthogonal =
        serialized.value(QStringLiteral("taguchi_orthogonal")).toObject();
    if (!taguchi_orthogonal.isEmpty()) {
        domain::TaguchiOrthogonalFacts value;
        value.array = taguchi_orthogonal.value(QStringLiteral("array"))
                          .toString(QStringLiteral("L8")).toStdString();
        value.factor_count = static_cast<std::size_t>(
            taguchi_orthogonal.value(QStringLiteral("factor_count")).toInt(0));
        value.run_count = static_cast<std::size_t>(
            taguchi_orthogonal.value(QStringLiteral("run_count")).toInt(0));
        value.levels_per_factor = static_cast<std::size_t>(
            taguchi_orthogonal.value(QStringLiteral("levels_per_factor")).toInt(2));
        value.evidence_type =
            taguchi_orthogonal.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            taguchi_orthogonal.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("taguchi_orthogonal_l8_l9_l12")).toStdString();
        facts.taguchi_orthogonal = std::move(value);
    }
    const QJsonObject distribution_calculator =
        serialized.value(QStringLiteral("distribution_calculator")).toObject();
    if (!distribution_calculator.isEmpty()) {
        domain::DistributionCalculatorFacts value;
        value.distribution =
            distribution_calculator.value(QStringLiteral("distribution"))
                .toString(QStringLiteral("normal")).toStdString();
        value.operation =
            distribution_calculator.value(QStringLiteral("operation"))
                .toString(QStringLiteral("cdf")).toStdString();
        value.param1 = distribution_calculator.value(QStringLiteral("param1")).toDouble(0.0);
        value.param2 = distribution_calculator.value(QStringLiteral("param2")).toDouble(1.0);
        value.param3 = distribution_calculator.value(QStringLiteral("param3")).toDouble(1.0);
        value.value = distribution_calculator.value(QStringLiteral("value")).toDouble(0.0);
        value.result = read_optional(distribution_calculator.value(QStringLiteral("result")));
        value.evidence_type =
            distribution_calculator.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            distribution_calculator.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("distribution_calculator_reuse")).toStdString();
        facts.distribution_calculator = std::move(value);
    }
    const QJsonObject taguchi_analyze =
        serialized.value(QStringLiteral("taguchi_analyze")).toObject();
    if (!taguchi_analyze.isEmpty()) {
        domain::TaguchiAnalyzeFacts value;
        value.sn_type = taguchi_analyze.value(QStringLiteral("sn_type"))
                            .toString(QStringLiteral("larger")).toStdString();
        value.factor_count = static_cast<std::size_t>(
            taguchi_analyze.value(QStringLiteral("factor_count")).toInt(0));
        value.response_count = static_cast<std::size_t>(
            taguchi_analyze.value(QStringLiteral("response_count")).toInt(0));
        value.run_count = static_cast<std::size_t>(
            taguchi_analyze.value(QStringLiteral("run_count")).toInt(0));
        value.top_delta = read_optional(taguchi_analyze.value(QStringLiteral("top_delta")));
        value.top_factor =
            taguchi_analyze.value(QStringLiteral("top_factor")).toString().toStdString();
        value.evidence_type =
            taguchi_analyze.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            taguchi_analyze.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("taguchi_analyze_static_sn")).toStdString();
        facts.taguchi_analyze = std::move(value);
    }
    const QJsonObject mixture_design =
        serialized.value(QStringLiteral("mixture_design")).toObject();
    if (!mixture_design.isEmpty()) {
        domain::MixtureDesignFacts value;
        value.component_count = static_cast<std::size_t>(
            mixture_design.value(QStringLiteral("component_count")).toInt(0));
        value.degree = static_cast<std::size_t>(
            mixture_design.value(QStringLiteral("degree")).toInt(2));
        value.run_count = static_cast<std::size_t>(
            mixture_design.value(QStringLiteral("run_count")).toInt(0));
        value.design_kind =
            mixture_design.value(QStringLiteral("design_kind"))
                .toString(QStringLiteral("simplex_lattice")).toStdString();
        value.evidence_type =
            mixture_design.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            mixture_design.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("mixture_simplex_lattice_m2")).toStdString();
        facts.mixture_design = std::move(value);
    }
    const QJsonObject nhpp_repairable =
        serialized.value(QStringLiteral("nhpp_repairable")).toObject();
    if (!nhpp_repairable.isEmpty()) {
        domain::NhppRepairableFacts value;
        value.failure_count = static_cast<std::size_t>(
            nhpp_repairable.value(QStringLiteral("failure_count")).toInt(0));
        value.truncation_time =
            nhpp_repairable.value(QStringLiteral("truncation_time")).toDouble(0.0);
        value.beta = read_optional(nhpp_repairable.value(QStringLiteral("beta")));
        value.lambda = read_optional(nhpp_repairable.value(QStringLiteral("lambda")));
        value.evidence_type =
            nhpp_repairable.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            nhpp_repairable.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("nhpp_crow_amsaa_mle")).toStdString();
        facts.nhpp_repairable = std::move(value);
    }
    const QJsonObject reliability_test_plan =
        serialized.value(QStringLiteral("reliability_test_plan")).toObject();
    if (!reliability_test_plan.isEmpty()) {
        domain::ReliabilityTestPlanFacts value;
        value.shape_beta =
            reliability_test_plan.value(QStringLiteral("shape_beta")).toDouble(1.0);
        value.target_reliability =
            reliability_test_plan.value(QStringLiteral("target_reliability")).toDouble(0.9);
        value.confidence_level =
            reliability_test_plan.value(QStringLiteral("confidence_level")).toDouble(0.9);
        value.test_time =
            reliability_test_plan.value(QStringLiteral("test_time")).toDouble(1.0);
        value.mission_time =
            reliability_test_plan.value(QStringLiteral("mission_time")).toDouble(1.0);
        value.time_ratio_delta =
            reliability_test_plan.value(QStringLiteral("time_ratio_delta")).toDouble(1.0);
        value.allowed_failures = static_cast<std::size_t>(
            reliability_test_plan.value(QStringLiteral("allowed_failures")).toInt(0));
        const QJsonValue sample = reliability_test_plan.value(QStringLiteral("sample_size"));
        if (!sample.isNull() && sample.isDouble()) {
            value.sample_size = static_cast<std::size_t>(sample.toInt());
        }
        value.evidence_type =
            reliability_test_plan.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            reliability_test_plan.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("reliability_demo_test_plan_weibull")).toStdString();
        facts.reliability_test_plan = std::move(value);
    }
    const QJsonObject mixture_analyze_facts =
        serialized.value(QStringLiteral("mixture_analyze")).toObject();
    if (!mixture_analyze_facts.isEmpty()) {
        domain::MixtureAnalyzeFacts value;
        value.component_count = static_cast<std::size_t>(
            mixture_analyze_facts.value(QStringLiteral("component_count")).toInt(0));
        value.observation_count = static_cast<std::size_t>(
            mixture_analyze_facts.value(QStringLiteral("observation_count")).toInt(0));
        value.model_order =
            mixture_analyze_facts.value(QStringLiteral("model_order"))
                .toString(QStringLiteral("linear")).toStdString();
        value.r_squared = read_optional(
            mixture_analyze_facts.value(QStringLiteral("r_squared")));
        value.evidence_type =
            mixture_analyze_facts.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            mixture_analyze_facts.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("mixture_scheffe_ols")).toStdString();
        facts.mixture_analyze = std::move(value);
    }
    const QJsonObject glm_two_way =
        serialized.value(QStringLiteral("glm_two_way")).toObject();
    if (!glm_two_way.isEmpty()) {
        domain::GlmTwoWayFacts value;
        value.observation_count = static_cast<std::size_t>(
            glm_two_way.value(QStringLiteral("observation_count")).toInt(0));
        value.include_interaction =
            glm_two_way.value(QStringLiteral("include_interaction")).toBool(true);
        value.design_balanced =
            glm_two_way.value(QStringLiteral("design_balanced")).toBool(true);
        value.residual_normality_p = read_optional(
            glm_two_way.value(QStringLiteral("residual_normality_p")));
        value.evidence_type =
            glm_two_way.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            glm_two_way.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("glm_two_way_type3")).toStdString();
        facts.glm_two_way = std::move(value);
    }
    const QJsonObject analyze_variability =
        serialized.value(QStringLiteral("analyze_variability")).toObject();
    if (!analyze_variability.isEmpty()) {
        domain::AnalyzeVariabilityFacts value;
        value.run_count = static_cast<std::size_t>(
            analyze_variability.value(QStringLiteral("run_count")).toInt(0));
        value.factor_count = static_cast<std::size_t>(
            analyze_variability.value(QStringLiteral("factor_count")).toInt(0));
        value.replicate_count = static_cast<std::size_t>(
            analyze_variability.value(QStringLiteral("replicate_count")).toInt(0));
        value.estimation_method =
            analyze_variability.value(QStringLiteral("estimation_method"))
                .toString(QStringLiteral("lse")).toStdString();
        value.evidence_type =
            analyze_variability.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            analyze_variability.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("analyze_variability_ln_sigma_lse"))
                .toStdString();
        facts.analyze_variability = std::move(value);
    }
    const QJsonObject factor_analysis =
        serialized.value(QStringLiteral("factor_analysis")).toObject();
    if (!factor_analysis.isEmpty()) {
        domain::FactorAnalysisFacts value;
        value.observation_count = static_cast<std::size_t>(
            factor_analysis.value(QStringLiteral("observation_count")).toInt(0));
        value.variable_count = static_cast<std::size_t>(
            factor_analysis.value(QStringLiteral("variable_count")).toInt(0));
        value.retained_factor_count = static_cast<std::size_t>(
            factor_analysis.value(QStringLiteral("retained_factor_count")).toInt(0));
        value.varimax_applied =
            factor_analysis.value(QStringLiteral("varimax_applied")).toBool(false);
        value.evidence_type =
            factor_analysis.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            factor_analysis.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("factor_analysis_pca_extraction"))
                .toStdString();
        facts.factor_analysis = std::move(value);
    }
    const QJsonObject binary_response_doe =
        serialized.value(QStringLiteral("binary_response_doe")).toObject();
    if (!binary_response_doe.isEmpty()) {
        domain::BinaryResponseDoeFacts value;
        value.design_row_count = static_cast<std::size_t>(
            binary_response_doe.value(QStringLiteral("design_row_count")).toInt(0));
        value.expanded_observation_count = static_cast<std::size_t>(
            binary_response_doe.value(QStringLiteral("expanded_observation_count")).toInt(0));
        value.factor_count = static_cast<std::size_t>(
            binary_response_doe.value(QStringLiteral("factor_count")).toInt(0));
        value.event_count = static_cast<std::size_t>(
            binary_response_doe.value(QStringLiteral("event_count")).toInt(0));
        value.trial_count = static_cast<std::size_t>(
            binary_response_doe.value(QStringLiteral("trial_count")).toInt(0));
        value.include_ab_interaction =
            binary_response_doe.value(QStringLiteral("include_ab_interaction")).toBool(true);
        value.converged = binary_response_doe.value(QStringLiteral("converged")).toBool(false);
        value.deviance = binary_response_doe.value(QStringLiteral("deviance")).toDouble(0.0);
        value.evidence_type =
            binary_response_doe.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            binary_response_doe.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("binary_response_doe_logit_irwls")).toStdString();
        facts.binary_response_doe = std::move(value);
    }
    const QJsonObject cluster_variables =
        serialized.value(QStringLiteral("cluster_variables")).toObject();
    if (!cluster_variables.isEmpty()) {
        domain::ClusterVariablesFacts value;
        value.observation_count = static_cast<std::size_t>(
            cluster_variables.value(QStringLiteral("observation_count")).toInt(0));
        value.variable_count = static_cast<std::size_t>(
            cluster_variables.value(QStringLiteral("variable_count")).toInt(0));
        value.merge_count = static_cast<std::size_t>(
            cluster_variables.value(QStringLiteral("merge_count")).toInt(0));
        value.linkage =
            cluster_variables.value(QStringLiteral("linkage")).toString().toStdString();
        value.max_distance =
            cluster_variables.value(QStringLiteral("max_distance")).toDouble(0.0);
        value.evidence_type =
            cluster_variables.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            cluster_variables.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("cluster_variables_corr_hclust")).toStdString();
        facts.cluster_variables = std::move(value);
    }
    const QJsonObject glm_three_factor =
        serialized.value(QStringLiteral("glm_three_factor")).toObject();
    if (!glm_three_factor.isEmpty()) {
        domain::GlmThreeFactorFacts value;
        value.observation_count = static_cast<std::size_t>(
            glm_three_factor.value(QStringLiteral("observation_count")).toInt(0));
        value.include_ab_interaction =
            glm_three_factor.value(QStringLiteral("include_ab_interaction")).toBool(true);
        value.include_ac_interaction =
            glm_three_factor.value(QStringLiteral("include_ac_interaction")).toBool(true);
        value.include_bc_interaction =
            glm_three_factor.value(QStringLiteral("include_bc_interaction")).toBool(true);
        value.design_balanced =
            glm_three_factor.value(QStringLiteral("design_balanced")).toBool(true);
        value.residual_normality_p = read_optional(
            glm_three_factor.value(QStringLiteral("residual_normality_p")));
        value.evidence_type =
            glm_three_factor.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            glm_three_factor.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("glm_three_factor_type3")).toStdString();
        facts.glm_three_factor = std::move(value);
    }
    const QJsonObject life_data_regression =
        serialized.value(QStringLiteral("life_data_regression")).toObject();
    if (!life_data_regression.isEmpty()) {
        domain::LifeDataRegressionFacts value;
        value.observation_count = static_cast<std::size_t>(
            life_data_regression.value(QStringLiteral("observation_count")).toInt(0));
        value.failure_count = static_cast<std::size_t>(
            life_data_regression.value(QStringLiteral("failure_count")).toInt(0));
        value.censored_count = static_cast<std::size_t>(
            life_data_regression.value(QStringLiteral("censored_count")).toInt(0));
        value.covariate_count = static_cast<std::size_t>(
            life_data_regression.value(QStringLiteral("covariate_count")).toInt(0));
        value.converged =
            life_data_regression.value(QStringLiteral("converged")).toBool(false);
        value.shape = life_data_regression.value(QStringLiteral("shape")).toDouble(0.0);
        value.distribution =
            life_data_regression.value(QStringLiteral("distribution"))
                .toString(QStringLiteral("weibull")).toStdString();
        value.evidence_type =
            life_data_regression.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference")).toStdString();
        value.algorithm_id =
            life_data_regression.value(QStringLiteral("algorithm_id"))
                .toString(QStringLiteral("life_data_regression_weibull_mle")).toStdString();
        facts.life_data_regression = std::move(value);
    }
    const QJsonObject design_generation =
        serialized.value(QStringLiteral("design_generation")).toObject();
    if (!design_generation.isEmpty()) {
        domain::DesignGenerationFacts value;
        value.design_kind =
            design_generation.value(QStringLiteral("design_kind")).toString().toStdString();
        value.ccd_variant =
            design_generation.value(QStringLiteral("ccd_variant")).toString().toStdString();
        value.design_source_id =
            design_generation.value(QStringLiteral("design_source_id")).toString().toStdString();
        value.factor_count = static_cast<std::size_t>(
            design_generation.value(QStringLiteral("factor_count")).toInt(0));
        value.run_count = static_cast<std::size_t>(
            design_generation.value(QStringLiteral("run_count")).toInt(0));
        value.cube_count = static_cast<std::size_t>(
            design_generation.value(QStringLiteral("cube_count")).toInt(0));
        value.star_count = static_cast<std::size_t>(
            design_generation.value(QStringLiteral("star_count")).toInt(0));
        value.edge_count = static_cast<std::size_t>(
            design_generation.value(QStringLiteral("edge_count")).toInt(0));
        value.center_count = static_cast<std::size_t>(
            design_generation.value(QStringLiteral("center_count")).toInt(0));
        value.alpha = design_generation.value(QStringLiteral("alpha")).toDouble(1.0);
        value.allow_beyond_range =
            design_generation.value(QStringLiteral("allow_beyond_range")).toBool(false);
        value.beyond_range_detected =
            design_generation.value(QStringLiteral("beyond_range_detected")).toBool(false);
        value.randomized =
            design_generation.value(QStringLiteral("randomized")).toBool(false);
        value.random_seed = static_cast<std::uint64_t>(
            design_generation.value(QStringLiteral("random_seed")).toDouble(0.0));
        value.evidence_type =
            design_generation.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference"))
                .toStdString();
        facts.design_generation = std::move(value);
    }
    const QJsonObject multi_vari = serialized.value(QStringLiteral("multi_vari")).toObject();
    if (!multi_vari.isEmpty()) {
        domain::MultiVariFacts value;
        value.factor_count = static_cast<std::size_t>(
            multi_vari.value(QStringLiteral("factor_count")).toInt(0));
        value.valid_count = static_cast<std::size_t>(
            multi_vari.value(QStringLiteral("valid_count")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            multi_vari.value(QStringLiteral("missing_count")).toInt(0));
        value.combination_coverage =
            multi_vari.value(QStringLiteral("combination_coverage")).toDouble(0.0);
        value.factor_names = to_strings(
            multi_vari.value(QStringLiteral("factor_names")).toArray());
        facts.multi_vari = std::move(value);
    }
    const QJsonObject variability = serialized.value(QStringLiteral("variability")).toObject();
    if (!variability.isEmpty()) {
        domain::VariabilityFacts value;
        value.factor_count = static_cast<std::size_t>(
            variability.value(QStringLiteral("factor_count")).toInt(0));
        value.valid_count = static_cast<std::size_t>(
            variability.value(QStringLiteral("valid_count")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            variability.value(QStringLiteral("missing_count")).toInt(0));
        value.cell_count = static_cast<std::size_t>(
            variability.value(QStringLiteral("cell_count")).toInt(0));
        value.overall_mean = read_optional(variability.value(QStringLiteral("overall_mean")));
        value.mean_of_cell_sds =
            read_optional(variability.value(QStringLiteral("mean_of_cell_sds")));
        value.factor_names = to_strings(
            variability.value(QStringLiteral("factor_names")).toArray());
        facts.variability = std::move(value);
    }
    const QJsonObject tolerance = serialized.value(QStringLiteral("tolerance")).toObject();
    if (!tolerance.isEmpty()) {
        domain::ToleranceFacts value;
        value.valid_count = static_cast<std::size_t>(
            tolerance.value(QStringLiteral("valid_count")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            tolerance.value(QStringLiteral("missing_count")).toInt(0));
        value.mean = read_optional(tolerance.value(QStringLiteral("mean")));
        value.standard_deviation =
            read_optional(tolerance.value(QStringLiteral("standard_deviation")));
        value.coverage = read_optional(tolerance.value(QStringLiteral("coverage")));
        value.confidence_level =
            read_optional(tolerance.value(QStringLiteral("confidence_level")));
        value.lower = read_optional(tolerance.value(QStringLiteral("lower")));
        value.upper = read_optional(tolerance.value(QStringLiteral("upper")));
        value.k_factor = read_optional(tolerance.value(QStringLiteral("k_factor")));
        value.achieved_confidence =
            read_optional(tolerance.value(QStringLiteral("achieved_confidence")));
        value.method = tolerance.value(QStringLiteral("method")).toString().toStdString();
        value.method_family =
            tolerance.value(QStringLiteral("method_family"))
                .toString(QStringLiteral("normal")).toStdString();
        value.interval_type =
            tolerance.value(QStringLiteral("interval_type"))
                .toString(QStringLiteral("two_sided")).toStdString();
        value.assumption_status =
            tolerance.value(QStringLiteral("assumption_status"))
                .toString(QStringLiteral("not_verified")).toStdString();
        facts.tolerance = std::move(value);
    }
    const QJsonObject variance = serialized.value(QStringLiteral("variance")).toObject();
    if (!variance.isEmpty()) {
        domain::VarianceFacts value;
        value.method = variance.value(QStringLiteral("method")).toString().toStdString();
        value.statistic = read_optional(variance.value(QStringLiteral("statistic")));
        value.p_value = read_optional(variance.value(QStringLiteral("p_value")));
        value.ci_lower = read_optional(variance.value(QStringLiteral("ci_lower")));
        value.ci_upper = read_optional(variance.value(QStringLiteral("ci_upper")));
        value.group_count = static_cast<std::size_t>(
            variance.value(QStringLiteral("group_count")).toInt(0));
        facts.variance = std::move(value);
    }
    const QJsonObject doe = serialized.value(QStringLiteral("doe")).toObject();
    if (!doe.isEmpty()) {
        domain::DoeFacts value;
        value.significant_terms = to_strings(
            doe.value(QStringLiteral("significant_terms")).toArray());
        value.has_p_value = doe.value(QStringLiteral("has_p_value")).toBool(false);
        value.response_count = static_cast<std::size_t>(
            doe.value(QStringLiteral("response_count")).toInt(0));
        value.multi_response = doe.value(QStringLiteral("multi_response")).toBool(false);
        value.best_overall_desirability =
            read_optional(doe.value(QStringLiteral("best_overall_desirability")));
        value.response_names = to_strings(
            doe.value(QStringLiteral("response_names")).toArray());
        value.prediction_interval_available =
            doe.value(QStringLiteral("prediction_interval_available")).toBool(false);
        value.largest_standardized_effect_term =
            doe.value(QStringLiteral("largest_standardized_effect_term")).toString().toStdString();
        value.pareto_reference =
            read_optional(doe.value(QStringLiteral("pareto_reference")));
        value.pareto_method =
            doe.value(QStringLiteral("pareto_method")).toString().toStdString();
        value.residual_count = static_cast<std::size_t>(
            doe.value(QStringLiteral("residual_count")).toInt(0));
        value.factor_count = static_cast<std::size_t>(
            doe.value(QStringLiteral("factor_count")).toInt(0));
        value.cube_plot_available =
            doe.value(QStringLiteral("cube_plot_available")).toBool(false);
        value.contour_plot_available =
            doe.value(QStringLiteral("contour_plot_available")).toBool(false);
        value.contour_x_factor =
            doe.value(QStringLiteral("contour_x_factor")).toString().toStdString();
        value.contour_y_factor =
            doe.value(QStringLiteral("contour_y_factor")).toString().toStdString();
        value.held_factor_names = to_strings(
            doe.value(QStringLiteral("held_factor_names")).toArray());
        value.held_actual_values = to_strings(
            doe.value(QStringLiteral("held_actual_values")).toArray());
        for (const QJsonValue& coded : doe.value(QStringLiteral("held_coded_values")).toArray()) {
            value.held_coded_values.push_back(coded.toDouble());
        }
        value.design_kind =
            doe.value(QStringLiteral("design_kind")).toString().toStdString();
        value.fraction_p = static_cast<std::size_t>(
            doe.value(QStringLiteral("fraction_p")).toInt(0));
        value.resolution = doe.value(QStringLiteral("resolution")).toInt(0);
        value.run_count = static_cast<std::size_t>(
            doe.value(QStringLiteral("run_count")).toInt(0));
        value.generator_text =
            doe.value(QStringLiteral("generator_text")).toString().toStdString();
        facts.doe = std::move(value);
    }
    const QJsonObject rsm = serialized.value(QStringLiteral("rsm")).toObject();
    if (!rsm.isEmpty()) {
        domain::RsmFacts value;
        value.factor_count = static_cast<std::size_t>(
            rsm.value(QStringLiteral("factor_count")).toInt(0));
        value.term_count = static_cast<std::size_t>(
            rsm.value(QStringLiteral("term_count")).toInt(0));
        value.residual_count = static_cast<std::size_t>(
            rsm.value(QStringLiteral("residual_count")).toInt(0));
        value.r_squared = read_optional(rsm.value(QStringLiteral("r_squared")));
        value.adjusted_r_squared =
            read_optional(rsm.value(QStringLiteral("adjusted_r_squared")));
        value.contour_plot_available =
            rsm.value(QStringLiteral("contour_plot_available")).toBool(false);
        value.largest_abs_t_term =
            rsm.value(QStringLiteral("largest_abs_t_term")).toString().toStdString();
        value.response_name =
            rsm.value(QStringLiteral("response_name")).toString().toStdString();
        value.design_source_id =
            rsm.value(QStringLiteral("design_source_id")).toString().toStdString();
        value.design_kind =
            rsm.value(QStringLiteral("design_kind")).toString().toStdString();
        value.coding_mode =
            rsm.value(QStringLiteral("coding_mode")).toString().toStdString();
        value.center_point_count = static_cast<std::size_t>(
            rsm.value(QStringLiteral("center_point_count")).toInt(0));
        value.surface_is_static =
            rsm.value(QStringLiteral("surface_is_static")).toBool(true);
        value.evidence_type =
            rsm.value(QStringLiteral("evidence_type"))
                .toString(QStringLiteral("formula_reference"))
                .toStdString();
        value.pure_error_available =
            rsm.value(QStringLiteral("pure_error_available")).toBool(false);
        value.lack_of_fit_available =
            rsm.value(QStringLiteral("lack_of_fit_available")).toBool(false);
        value.pure_error_df = static_cast<std::size_t>(
            rsm.value(QStringLiteral("pure_error_df")).toInt(0));
        value.lack_of_fit_df = static_cast<std::size_t>(
            rsm.value(QStringLiteral("lack_of_fit_df")).toInt(0));
        value.lack_of_fit_f =
            read_optional(rsm.value(QStringLiteral("lack_of_fit_f")));
        value.lack_of_fit_p =
            read_optional(rsm.value(QStringLiteral("lack_of_fit_p")));
        facts.rsm = std::move(value);
    }
    const QJsonObject acf_pacf = serialized.value(QStringLiteral("acf_pacf")).toObject();
    if (!acf_pacf.isEmpty()) {
        domain::AcfPacfFacts value;
        value.n = static_cast<std::size_t>(acf_pacf.value(QStringLiteral("n")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            acf_pacf.value(QStringLiteral("missing_count")).toInt(0));
        value.max_lag = static_cast<std::size_t>(
            acf_pacf.value(QStringLiteral("max_lag")).toInt(0));
        value.confidence_band_method =
            acf_pacf.value(QStringLiteral("confidence_band_method"))
                .toString(QStringLiteral("white_noise_fixed"))
                .toStdString();
        value.band_half_width =
            read_optional(acf_pacf.value(QStringLiteral("band_half_width")));
        value.alpha = read_optional(acf_pacf.value(QStringLiteral("alpha")));
        value.ljung_box_available =
            acf_pacf.value(QStringLiteral("ljung_box_available")).toBool(false);
        value.ljung_box_statistic =
            read_optional(acf_pacf.value(QStringLiteral("ljung_box_statistic")));
        value.ljung_box_p_value =
            read_optional(acf_pacf.value(QStringLiteral("ljung_box_p_value")));
        facts.acf_pacf = std::move(value);
    }
    const QJsonObject eda = serialized.value(QStringLiteral("eda")).toObject();
    if (!eda.isEmpty()) {
        domain::EdaPlotFacts value;
        value.kind = eda.value(QStringLiteral("kind")).toString().toStdString();
        value.n = static_cast<std::size_t>(eda.value(QStringLiteral("n")).toInt(0));
        value.bandwidth = read_optional(eda.value(QStringLiteral("bandwidth")));
        value.category_count = static_cast<std::size_t>(
            eda.value(QStringLiteral("category_count")).toInt(0));
        value.x_bins = static_cast<std::size_t>(eda.value(QStringLiteral("x_bins")).toInt(0));
        value.y_bins = static_cast<std::size_t>(eda.value(QStringLiteral("y_bins")).toInt(0));
        value.sorted_by_count = eda.value(QStringLiteral("sorted_by_count")).toBool(false);
        value.has_cumulative_percent =
            eda.value(QStringLiteral("has_cumulative_percent")).toBool(false);
        value.hidden_count = static_cast<std::size_t>(
            eda.value(QStringLiteral("hidden_count")).toInt(0));
        value.excluded_count = static_cast<std::size_t>(
            eda.value(QStringLiteral("excluded_count")).toInt(0));
        value.analysis_eligible_n = static_cast<std::size_t>(
            eda.value(QStringLiteral("analysis_eligible_n")).toInt(0));
        value.display_eligible_n = static_cast<std::size_t>(
            eda.value(QStringLiteral("display_eligible_n")).toInt(0));
        value.hidden_excluded_distinct =
            eda.value(QStringLiteral("hidden_excluded_distinct")).toBool(true);
        value.analysis_n = static_cast<std::size_t>(
            eda.value(QStringLiteral("analysis_n")).toInt(0));
        value.analysis_category_count = static_cast<std::size_t>(
            eda.value(QStringLiteral("analysis_category_count")).toInt(0));
        value.facet_enabled = eda.value(QStringLiteral("facet_enabled")).toBool(false);
        value.facet_panel_count = static_cast<std::size_t>(
            eda.value(QStringLiteral("facet_panel_count")).toInt(0));
        value.facet_level_count = static_cast<std::size_t>(
            eda.value(QStringLiteral("facet_level_count")).toInt(0));
        value.facet_truncated_levels = static_cast<std::size_t>(
            eda.value(QStringLiteral("facet_truncated_levels")).toInt(0));
        value.facet_max_panels = eda.value(QStringLiteral("facet_max_panels")).toInt(0);
        facts.eda = std::move(value);
    }
    const QJsonObject proportion = serialized.value(QStringLiteral("proportion")).toObject();
    if (!proportion.isEmpty()) {
        domain::ProportionFacts value;
        value.kind = proportion.value(QStringLiteral("kind"))
                         .toString(QStringLiteral("one_sample")).toStdString();
        value.events = static_cast<std::size_t>(
            proportion.value(QStringLiteral("events")).toInt(0));
        value.trials = static_cast<std::size_t>(
            proportion.value(QStringLiteral("trials")).toInt(0));
        value.proportion = read_optional(proportion.value(QStringLiteral("proportion")));
        if (proportion.contains(QStringLiteral("second_events"))) {
            value.second_events = static_cast<std::size_t>(
                proportion.value(QStringLiteral("second_events")).toInt(0));
        }
        if (proportion.contains(QStringLiteral("second_trials"))) {
            value.second_trials = static_cast<std::size_t>(
                proportion.value(QStringLiteral("second_trials")).toInt(0));
        }
        value.second_proportion =
            read_optional(proportion.value(QStringLiteral("second_proportion")));
        value.difference = read_optional(proportion.value(QStringLiteral("difference")));
        value.hypothesized = read_optional(proportion.value(QStringLiteral("hypothesized")));
        value.method = proportion.value(QStringLiteral("method")).toString().toStdString();
        value.ci_method = proportion.value(QStringLiteral("ci_method")).toString().toStdString();
        value.p_value = read_optional(proportion.value(QStringLiteral("p_value")));
        value.fisher_p_value = read_optional(proportion.value(QStringLiteral("fisher_p_value")));
        value.ci_lower = read_optional(proportion.value(QStringLiteral("ci_lower")));
        value.ci_upper = read_optional(proportion.value(QStringLiteral("ci_upper")));
        value.assumption_status =
            proportion.value(QStringLiteral("assumption_status"))
                .toString(QStringLiteral("not_verified")).toStdString();
        facts.proportion = std::move(value);
    }
    const QJsonObject box_cox = serialized.value(QStringLiteral("box_cox")).toObject();
    if (!box_cox.isEmpty()) {
        domain::BoxCoxFacts value;
        value.lambda = box_cox.value(QStringLiteral("lambda")).toDouble(0.0);
        value.n = static_cast<std::size_t>(box_cox.value(QStringLiteral("n")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            box_cox.value(QStringLiteral("missing_count")).toInt(0));
        value.transformed_standard_deviation =
            read_optional(box_cox.value(QStringLiteral("transformed_standard_deviation")));
        value.rounded_lambda = box_cox.value(QStringLiteral("rounded_lambda")).toBool(true);
        value.assumption_status =
            box_cox.value(QStringLiteral("assumption_status"))
                .toString(QStringLiteral("not_verified")).toStdString();
        facts.box_cox = std::move(value);
    }
    const QJsonObject poisson_rate = serialized.value(QStringLiteral("poisson_rate")).toObject();
    if (!poisson_rate.isEmpty()) {
        domain::PoissonRateFacts value;
        value.kind = poisson_rate.value(QStringLiteral("kind")).toString().toStdString();
        value.events = static_cast<std::size_t>(
            poisson_rate.value(QStringLiteral("events")).toInt(0));
        value.exposure = poisson_rate.value(QStringLiteral("exposure")).toDouble(0.0);
        value.rate = read_optional(poisson_rate.value(QStringLiteral("rate")));
        if (poisson_rate.contains(QStringLiteral("second_events"))) {
            value.second_events = static_cast<std::size_t>(
                poisson_rate.value(QStringLiteral("second_events")).toInt(0));
        }
        value.second_exposure =
            read_optional(poisson_rate.value(QStringLiteral("second_exposure")));
        value.second_rate = read_optional(poisson_rate.value(QStringLiteral("second_rate")));
        value.hypothesized = read_optional(poisson_rate.value(QStringLiteral("hypothesized")));
        value.method = poisson_rate.value(QStringLiteral("method")).toString().toStdString();
        value.comparison =
            poisson_rate.value(QStringLiteral("comparison")).toString("difference").toStdString();
        if (value.comparison.empty()) {
            value.comparison = "difference";
        }
        value.ratio = read_optional(poisson_rate.value(QStringLiteral("ratio")));
        value.ratio_ci_lower =
            read_optional(poisson_rate.value(QStringLiteral("ratio_ci_lower")));
        value.ratio_ci_upper =
            read_optional(poisson_rate.value(QStringLiteral("ratio_ci_upper")));
        value.z_statistic = read_optional(poisson_rate.value(QStringLiteral("z_statistic")));
        value.p_value = read_optional(poisson_rate.value(QStringLiteral("p_value")));
        value.ci_lower = read_optional(poisson_rate.value(QStringLiteral("ci_lower")));
        value.ci_upper = read_optional(poisson_rate.value(QStringLiteral("ci_upper")));
        value.assumption_status =
            poisson_rate.value(QStringLiteral("assumption_status"))
                .toString(QStringLiteral("not_verified")).toStdString();
        facts.poisson_rate = std::move(value);
    }
    const QJsonObject equivalence = serialized.value(QStringLiteral("equivalence")).toObject();
    if (!equivalence.isEmpty()) {
        domain::EquivalenceFacts value;
        value.kind = equivalence.value(QStringLiteral("kind")).toString().toStdString();
        value.difference = read_optional(equivalence.value(QStringLiteral("difference")));
        value.lower = read_optional(equivalence.value(QStringLiteral("lower")));
        value.upper = read_optional(equivalence.value(QStringLiteral("upper")));
        value.ci_lower = read_optional(equivalence.value(QStringLiteral("ci_lower")));
        value.ci_upper = read_optional(equivalence.value(QStringLiteral("ci_upper")));
        value.p_lower = read_optional(equivalence.value(QStringLiteral("p_lower")));
        value.p_upper = read_optional(equivalence.value(QStringLiteral("p_upper")));
        value.alpha = read_optional(equivalence.value(QStringLiteral("alpha")));
        value.ci_method = equivalence.value(QStringLiteral("ci_method"))
                              .toString(QStringLiteral("tost_1_minus_alpha")).toStdString();
        value.within_limits = equivalence.value(QStringLiteral("within_limits")).toBool(false);
        value.both_pvalues_below_alpha =
            equivalence.value(QStringLiteral("both_pvalues_below_alpha")).toBool(false);
        value.assumption_status =
            equivalence.value(QStringLiteral("assumption_status"))
                .toString(QStringLiteral("not_verified")).toStdString();
        facts.equivalence = std::move(value);
    }
    const QJsonObject t_test = serialized.value(QStringLiteral("t_test")).toObject();
    if (!t_test.isEmpty()) {
        domain::TTestFacts value;
        value.kind = t_test.value(QStringLiteral("kind")).toString().toStdString();
        value.n = static_cast<std::size_t>(t_test.value(QStringLiteral("n")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            t_test.value(QStringLiteral("missing_count")).toInt(0));
        value.mean = read_optional(t_test.value(QStringLiteral("mean")));
        value.difference = read_optional(t_test.value(QStringLiteral("difference")));
        value.p_value = read_optional(t_test.value(QStringLiteral("p_value")));
        value.ci_lower = read_optional(t_test.value(QStringLiteral("ci_lower")));
        value.ci_upper = read_optional(t_test.value(QStringLiteral("ci_upper")));
        value.variance_method = t_test.value(QStringLiteral("variance_method"))
                                    .toString().toStdString();
        value.assumption_status = t_test.value(QStringLiteral("assumption_status"))
                                      .toString(QStringLiteral("not_verified")).toStdString();
        value.z_statistic = read_optional(t_test.value(QStringLiteral("z_statistic")));
        value.known_sigma = read_optional(t_test.value(QStringLiteral("known_sigma")));
        value.sample_standard_deviation =
            read_optional(t_test.value(QStringLiteral("sample_standard_deviation")));
        facts.t_test = std::move(value);
    }
    const QJsonObject normality = serialized.value(QStringLiteral("normality")).toObject();
    if (!normality.isEmpty()) {
        domain::NormalityFacts value;
        value.n = static_cast<std::size_t>(normality.value(QStringLiteral("n")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            normality.value(QStringLiteral("missing_count")).toInt(0));
        value.method = normality.value(QStringLiteral("method"))
                           .toString(QStringLiteral("anderson_darling")).toStdString();
        value.decision = normality.value(QStringLiteral("decision"))
                             .toString(QStringLiteral("not_computed")).toStdString();
        value.p_value = read_optional(normality.value(QStringLiteral("p_value")));
        value.anderson_darling =
            read_optional(normality.value(QStringLiteral("anderson_darling")));
        value.ryan_joiner_r =
            read_optional(normality.value(QStringLiteral("ryan_joiner_r")));
        value.alpha = normality.value(QStringLiteral("alpha")).toDouble(0.05);
        value.assumption_status = normality.value(QStringLiteral("assumption_status"))
                                      .toString(QStringLiteral("not_verified")).toStdString();
        facts.normality = std::move(value);
    }
    const QJsonObject outlier_test = serialized.value(QStringLiteral("outlier_test")).toObject();
    if (!outlier_test.isEmpty()) {
        domain::OutlierTestFacts value;
        value.n = static_cast<std::size_t>(outlier_test.value(QStringLiteral("n")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            outlier_test.value(QStringLiteral("missing_count")).toInt(0));
        value.mean = read_optional(outlier_test.value(QStringLiteral("mean")));
        value.standard_deviation =
            read_optional(outlier_test.value(QStringLiteral("standard_deviation")));
        value.g_statistic = read_optional(outlier_test.value(QStringLiteral("g_statistic")));
        value.p_value = read_optional(outlier_test.value(QStringLiteral("p_value")));
        value.outlier_value = read_optional(outlier_test.value(QStringLiteral("outlier_value")));
        if (outlier_test.contains(QStringLiteral("source_row"))) {
            value.source_row = static_cast<std::size_t>(
                outlier_test.value(QStringLiteral("source_row")).toInt(0));
        }
        value.direction = outlier_test.value(QStringLiteral("direction")).toString().toStdString();
        value.alternative = outlier_test.value(QStringLiteral("alternative"))
                                .toString(QStringLiteral("two_sided")).toStdString();
        value.alpha = outlier_test.value(QStringLiteral("alpha")).toDouble(0.05);
        value.assumption_status = outlier_test.value(QStringLiteral("assumption_status"))
                                      .toString(QStringLiteral("not_verified")).toStdString();
        value.method = outlier_test.value(QStringLiteral("method"))
                           .toString(QStringLiteral("grubbs")).toStdString();
        value.dixon_r = read_optional(outlier_test.value(QStringLiteral("dixon_r")));
        value.critical_value =
            read_optional(outlier_test.value(QStringLiteral("critical_value")));
        facts.outlier_test = std::move(value);
    }
    const QJsonObject correlation = serialized.value(QStringLiteral("correlation")).toObject();
    if (!correlation.isEmpty()) {
        domain::CorrelationFacts value;
        value.method = correlation.value(QStringLiteral("method"))
                           .toString(QStringLiteral("pearson")).toStdString();
        value.variable_count = static_cast<std::size_t>(
            correlation.value(QStringLiteral("variable_count")).toInt(0));
        value.n = static_cast<std::size_t>(correlation.value(QStringLiteral("n")).toInt(0));
        value.missing_skipped = static_cast<std::size_t>(
            correlation.value(QStringLiteral("missing_skipped")).toInt(0));
        value.assumption_status = correlation.value(QStringLiteral("assumption_status"))
                                      .toString(QStringLiteral("not_verified")).toStdString();
        value.covariance_available =
            correlation.value(QStringLiteral("covariance_available")).toBool(false);
        value.partial_available =
            correlation.value(QStringLiteral("partial_available")).toBool(false);
        facts.correlation = std::move(value);
    }
    const QJsonObject acceptance_sampling =
        serialized.value(QStringLiteral("acceptance_sampling")).toObject();
    if (!acceptance_sampling.isEmpty()) {
        domain::AcceptanceSamplingFacts value;
        value.sample_size = static_cast<std::size_t>(
            acceptance_sampling.value(QStringLiteral("sample_size")).toInt(0));
        value.acceptance_number = static_cast<std::size_t>(
            acceptance_sampling.value(QStringLiteral("acceptance_number")).toInt(0));
        value.lot_size = read_optional_size(acceptance_sampling.value(QStringLiteral("lot_size")));
        value.model = acceptance_sampling.value(QStringLiteral("model"))
                          .toString(QStringLiteral("binomial")).toStdString();
        value.aql = read_optional(acceptance_sampling.value(QStringLiteral("aql")));
        value.rql = read_optional(acceptance_sampling.value(QStringLiteral("rql")));
        value.pa_at_aql = read_optional(acceptance_sampling.value(QStringLiteral("pa_at_aql")));
        value.pa_at_rql = read_optional(acceptance_sampling.value(QStringLiteral("pa_at_rql")));
        value.oc_point_count = static_cast<std::size_t>(
            acceptance_sampling.value(QStringLiteral("oc_point_count")).toInt(0));
        facts.acceptance_sampling = std::move(value);
    }
    const QJsonObject anom = serialized.value(QStringLiteral("anom")).toObject();
    if (!anom.isEmpty()) {
        domain::AnomFacts value;
        value.overall_mean = anom.value(QStringLiteral("overall_mean")).toDouble(0.0);
        value.pooled_sd = anom.value(QStringLiteral("pooled_sd")).toDouble(0.0);
        value.udl = anom.value(QStringLiteral("udl")).toDouble(0.0);
        value.ldl = anom.value(QStringLiteral("ldl")).toDouble(0.0);
        value.alpha = anom.value(QStringLiteral("alpha")).toDouble(0.05);
        value.group_count = static_cast<std::size_t>(
            anom.value(QStringLiteral("group_count")).toInt(0));
        value.total_n = static_cast<std::size_t>(anom.value(QStringLiteral("total_n")).toInt(0));
        value.outside_count = static_cast<std::size_t>(
            anom.value(QStringLiteral("outside_count")).toInt(0));
        value.decision_limit_method = anom.value(QStringLiteral("decision_limit_method"))
                                          .toString().toStdString();
        facts.anom = std::move(value);
    }
    const QJsonObject run_chart = serialized.value(QStringLiteral("run_chart")).toObject();
    if (!run_chart.isEmpty()) {
        domain::RunChartFacts value;
        value.n = static_cast<std::size_t>(run_chart.value(QStringLiteral("n")).toInt(0));
        value.median = read_optional(run_chart.value(QStringLiteral("median")));
        value.runs_about_median = static_cast<std::size_t>(
            run_chart.value(QStringLiteral("runs_about_median")).toInt(0));
        value.runs_up_down = static_cast<std::size_t>(
            run_chart.value(QStringLiteral("runs_up_down")).toInt(0));
        value.p_clustering = read_optional(run_chart.value(QStringLiteral("p_clustering")));
        value.p_mixtures = read_optional(run_chart.value(QStringLiteral("p_mixtures")));
        value.p_trends = read_optional(run_chart.value(QStringLiteral("p_trends")));
        value.p_oscillation = read_optional(run_chart.value(QStringLiteral("p_oscillation")));
        value.missing_count = static_cast<std::size_t>(
            run_chart.value(QStringLiteral("missing_count")).toInt(0));
        facts.run_chart = std::move(value);
    }
    const QJsonObject zone_chart = serialized.value(QStringLiteral("zone_chart")).toObject();
    if (!zone_chart.isEmpty()) {
        domain::ZoneChartFacts value;
        value.n = static_cast<std::size_t>(zone_chart.value(QStringLiteral("n")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            zone_chart.value(QStringLiteral("missing_count")).toInt(0));
        value.center = read_optional(zone_chart.value(QStringLiteral("center")));
        value.sigma = read_optional(zone_chart.value(QStringLiteral("sigma")));
        value.signal_threshold =
            zone_chart.value(QStringLiteral("signal_threshold")).toDouble(8.0);
        value.signal_count = static_cast<std::size_t>(
            zone_chart.value(QStringLiteral("signal_count")).toInt(0));
        facts.zone_chart = std::move(value);
    }
    const QJsonObject z_mr = serialized.value(QStringLiteral("z_mr")).toObject();
    if (!z_mr.isEmpty()) {
        domain::ZmrFacts value;
        value.n = static_cast<std::size_t>(z_mr.value(QStringLiteral("n")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            z_mr.value(QStringLiteral("missing_count")).toInt(0));
        value.group_count = static_cast<std::size_t>(
            z_mr.value(QStringLiteral("group_count")).toInt(0));
        value.used_sample_parameters =
            z_mr.value(QStringLiteral("used_sample_parameters")).toBool(true);
        value.average_mr = read_optional(z_mr.value(QStringLiteral("average_mr")));
        value.z_out_of_control_count = static_cast<std::size_t>(
            z_mr.value(QStringLiteral("z_out_of_control_count")).toInt(0));
        facts.z_mr = std::move(value);
    }
    const QJsonObject moving_average =
        serialized.value(QStringLiteral("moving_average")).toObject();
    if (!moving_average.isEmpty()) {
        domain::MovingAverageChartFacts value;
        value.n = static_cast<std::size_t>(moving_average.value(QStringLiteral("n")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            moving_average.value(QStringLiteral("missing_count")).toInt(0));
        value.window = moving_average.value(QStringLiteral("window")).toInt(3);
        value.limit_sigma = moving_average.value(QStringLiteral("limit_sigma")).toDouble(3.0);
        value.center = read_optional(moving_average.value(QStringLiteral("center")));
        value.sigma_within = read_optional(moving_average.value(QStringLiteral("sigma_within")));
        value.out_of_control_count = static_cast<std::size_t>(
            moving_average.value(QStringLiteral("out_of_control_count")).toInt(0));
        facts.moving_average = std::move(value);
    }
    const QJsonObject cause_effect = serialized.value(QStringLiteral("cause_effect")).toObject();
    if (!cause_effect.isEmpty()) {
        domain::CauseEffectFacts value;
        value.effect = cause_effect.value(QStringLiteral("effect")).toString().toStdString();
        value.category_count = static_cast<std::size_t>(
            cause_effect.value(QStringLiteral("category_count")).toInt(0));
        value.cause_count = static_cast<std::size_t>(
            cause_effect.value(QStringLiteral("cause_count")).toInt(0));
        value.missing_count = static_cast<std::size_t>(
            cause_effect.value(QStringLiteral("missing_count")).toInt(0));
        facts.cause_effect = std::move(value);
    }
}

std::optional<std::size_t> read_optional_size(const QJsonValue& value)
{
    if (!value.isDouble()) {
        return std::nullopt;
    }
    const qint64 number = value.toInteger();
    return number >= 0 ? std::optional<std::size_t>(
        static_cast<std::size_t>(number)) : std::nullopt;
}

std::optional<double> read_optional(const QJsonValue& value)
{
    if (value.isDouble()) {
        return value.toDouble();
    }
    return std::nullopt;
}

}  // namespace

QJsonObject output_page_to_json(const domain::OutputPage& page)
{
    QJsonObject object;
    object.insert(QStringLiteral("schema_version"), 2);
    object.insert(QStringLiteral("id"), QString::fromStdString(page.id));
    object.insert(QStringLiteral("title"), QString::fromStdString(page.title));
    object.insert(QStringLiteral("method_name"), QString::fromStdString(page.method_name));
    object.insert(QStringLiteral("analysis_command_id"),
                  QString::fromStdString(page.analysis_command_id));
    object.insert(QStringLiteral("parameter_summary"), QString::fromStdString(page.parameter_summary));
    object.insert(QStringLiteral("method_algorithm"),
                  QString::fromStdString(page.method_metadata.algorithm));
    object.insert(QStringLiteral("method_version"),
                  QString::fromStdString(page.method_metadata.version));
    object.insert(QStringLiteral("method_parameters"),
                  QString::fromStdString(page.method_metadata.parameters));
    object.insert(QStringLiteral("method_missing_policy"),
                  QString::fromStdString(page.method_metadata.missing_policy));
    object.insert(QStringLiteral("method_estimation_method"),
                  QString::fromStdString(page.method_metadata.estimation_method));
    object.insert(QStringLiteral("method_source_rows"),
                  row_array(page.method_metadata.source_rows));
    object.insert(QStringLiteral("method_diagnostic_codes"),
                  string_array(page.method_metadata.diagnostic_codes));
    object.insert(QStringLiteral("method_assumption_status"),
                  QString::fromStdString(page.method_metadata.assumption_status));
    object.insert(QStringLiteral("method_parameter_source"),
                  QString::fromStdString(page.method_metadata.parameter_source));
    object.insert(QStringLiteral("method_valid_count"),
                  static_cast<int>(page.method_metadata.valid_count));
    object.insert(QStringLiteral("method_missing_count"),
                  static_cast<int>(page.method_metadata.missing_count));
    object.insert(QStringLiteral("method_not_computed_reason"),
                  QString::fromStdString(page.method_metadata.not_computed_reason));
    object.insert(QStringLiteral("chart_type"), QString::fromStdString(page.configuration.chart_type));
    object.insert(QStringLiteral("capability_method"),
                  QString::fromStdString(page.configuration.capability_method));
    object.insert(QStringLiteral("nonnormal_distribution"),
                  QString::fromStdString(page.configuration.nonnormal_distribution));
    object.insert(QStringLiteral("analysis_name"), QString::fromStdString(page.configuration.analysis_name));
    object.insert(QStringLiteral("graph_kind"),
                  QString::fromStdString(page.configuration.graph.graph_kind));
    object.insert(QStringLiteral("graph_x_column"),
                  optional_size(page.configuration.graph.x_column));
    object.insert(QStringLiteral("graph_y_column"),
                  optional_size(page.configuration.graph.y_column));
    object.insert(QStringLiteral("graph_size_column"),
                  optional_size(page.configuration.graph.size_column));
    object.insert(QStringLiteral("graph_by_column"),
                  optional_size(page.configuration.graph.by_column));
    object.insert(QStringLiteral("graph_facet_column"),
                  optional_size(page.configuration.graph.facet_column));
    object.insert(QStringLiteral("graph_facet_max_panels"),
                  page.configuration.graph.facet_max_panels);
    object.insert(QStringLiteral("graph_label_column"),
                  optional_size(page.configuration.graph.label_column));
    object.insert(QStringLiteral("graph_variable_columns"),
                  size_array(page.configuration.graph.variable_columns));
    object.insert(QStringLiteral("graph_correlation_method"),
                  QString::fromStdString(page.configuration.graph.correlation_method));
    object.insert(QStringLiteral("graph_confidence_level"),
                  page.configuration.graph.confidence_level);
    object.insert(QStringLiteral("graph_interval_type"),
                  QString::fromStdString(page.configuration.graph.interval_type));
    object.insert(QStringLiteral("graph_show_p_values"),
                  page.configuration.graph.show_p_values);
    object.insert(QStringLiteral("graph_z_column"),
                  optional_size(page.configuration.graph.z_column));
    object.insert(QStringLiteral("graph_time_column"),
                  optional_size(page.configuration.graph.time_column));
    object.insert(QStringLiteral("graph_weight_column"),
                  optional_size(page.configuration.graph.weight_column));
    object.insert(QStringLiteral("graph_bin_count"),
                  page.configuration.graph.bin_count);
    object.insert(QStringLiteral("graph_other_threshold_percent"),
                  page.configuration.graph.other_threshold_percent);
    object.insert(QStringLiteral("graph_show_normal_reference"),
                  page.configuration.graph.show_normal_reference);
    object.insert(QStringLiteral("graph_connect_missing"),
                  page.configuration.graph.connect_missing);
    object.insert(QStringLiteral("graph_stack_mode"),
                  QString::fromStdString(page.configuration.graph.stack_mode));
    object.insert(QStringLiteral("graph_color_scale"),
                  QString::fromStdString(page.configuration.graph.color_scale));
    object.insert(QStringLiteral("graph_contour_levels"),
                  page.configuration.graph.contour_levels);
    object.insert(QStringLiteral("graph_payload_version"),
                  page.configuration.graph.payload_version);
    write_interpretation_facts(object, page.facts);
    object.insert(QStringLiteral("variable_columns"), size_array(page.configuration.variable_columns));
    object.insert(QStringLiteral("excluded_rows"), size_array(page.configuration.excluded_rows));
    object.insert(QStringLiteral("hidden_rows"), size_array(page.configuration.hidden_rows));
    object.insert(QStringLiteral("included_rows"), size_array(page.configuration.included_rows));
    object.insert(QStringLiteral("stage_column"), optional_size(page.configuration.control.stage_column));
    object.insert(QStringLiteral("measurement_column"),
                  static_cast<qint64>(page.configuration.selection.measurement_column));
    object.insert(QStringLiteral("subgroup_column"),
                  optional_size(page.configuration.selection.subgroup_column));
    object.insert(QStringLiteral("time_column"),
                  optional_size(page.configuration.selection.time_column));
    object.insert(QStringLiteral("product_column"),
                  optional_size(page.configuration.selection.product_column));
    object.insert(QStringLiteral("defect_count_column"),
                  optional_size(page.configuration.selection.defect_count_column));
    object.insert(QStringLiteral("inspected_count_column"),
                  optional_size(page.configuration.selection.inspected_count_column));
    object.insert(QStringLiteral("inspected_constant"),
                  optional_size(page.configuration.inspected_constant));
    object.insert(QStringLiteral("first_events_column"),
                  optional_size(page.configuration.inference.first_events_column));
    object.insert(QStringLiteral("first_trials_column"),
                  optional_size(page.configuration.inference.first_trials_column));
    object.insert(QStringLiteral("second_events_column"),
                  optional_size(page.configuration.inference.second_events_column));
    object.insert(QStringLiteral("second_trials_column"),
                  optional_size(page.configuration.inference.second_trials_column));
    object.insert(QStringLiteral("row_category_column"),
                  optional_size(page.configuration.inference.row_category_column));
    object.insert(QStringLiteral("column_category_column"),
                  optional_size(page.configuration.inference.column_category_column));
    object.insert(QStringLiteral("expected_proportions"),
                  QString::fromStdString(page.configuration.inference.expected_proportions));
    object.insert(QStringLiteral("gof_category_column"),
                  optional_size(page.configuration.inference.gof_category_column));
    object.insert(QStringLiteral("lower_spec"), optional_number(page.configuration.specifications.lower));
    object.insert(QStringLiteral("upper_spec"), optional_number(page.configuration.specifications.upper));
    object.insert(QStringLiteral("target_spec"), optional_number(page.configuration.specifications.target));
    object.insert(QStringLiteral("by_column"), optional_size(page.configuration.by_column));
    object.insert(QStringLiteral("moving_range_length"), page.configuration.control.moving_range_length);
    object.insert(QStringLiteral("sigma_method"), QString::fromStdString(page.configuration.control.sigma_method));
    object.insert(QStringLiteral("use_nelson_estimate"), page.configuration.control.use_nelson_estimate);
    object.insert(QStringLiteral("historical_center"),
                  optional_number(page.configuration.control.historical_center));
    object.insert(QStringLiteral("historical_sigma"),
                  optional_number(page.configuration.control.historical_sigma));
    object.insert(QStringLiteral("historical_sigma_z"),
                  optional_number(page.configuration.control.historical_sigma_z));
    object.insert(QStringLiteral("leave_gaps_for_excluded"),
                  page.configuration.leave_gaps_for_excluded);
    QJsonArray enabled_tests;
    for (const int test : page.configuration.control.enabled_special_cause_tests) {
        enabled_tests.append(test);
    }
    object.insert(QStringLiteral("enabled_special_cause_tests"), enabled_tests);
    object.insert(QStringLiteral("special_cause_rule_policy"),
                  QString::fromStdString(page.configuration.control.special_cause_rule_policy));
    object.insert(QStringLiteral("subgroup_size"),
                  page.configuration.control.subgroup_size.has_value()
                      ? QJsonValue(*page.configuration.control.subgroup_size) : QJsonValue());
    object.insert(QStringLiteral("pareto_other_threshold_percent"),
                  optional_number(page.configuration.pareto_other_threshold_percent));
    object.insert(QStringLiteral("effect_title"),
                  QString::fromStdString(page.configuration.effect_title));
    object.insert(QStringLiteral("hypothesis_mean"),
                  optional_number(page.configuration.inference.hypothesis_mean));
    object.insert(QStringLiteral("confidence_level"), page.configuration.inference.confidence_level);
    object.insert(QStringLiteral("coverage_proportion"),
                  optional_number(page.configuration.inference.coverage_proportion));
    object.insert(QStringLiteral("alternative"),
                  QString::fromStdString(page.configuration.inference.alternative));
    object.insert(QStringLiteral("runs_criterion"),
                  QString::fromStdString(page.configuration.inference.runs_criterion));
    object.insert(QStringLiteral("outlier_method"),
                  QString::fromStdString(page.configuration.inference.outlier_method));
    object.insert(QStringLiteral("tolerance_method"),
                  QString::fromStdString(page.configuration.inference.tolerance_method));
    object.insert(QStringLiteral("known_sigma"),
                  optional_number(page.configuration.inference.known_sigma));
    object.insert(QStringLiteral("acceptance_sample_size"),
                  static_cast<qint64>(page.configuration.inference.acceptance_sample_size));
    object.insert(QStringLiteral("acceptance_number"),
                  static_cast<qint64>(page.configuration.inference.acceptance_number));
    object.insert(QStringLiteral("acceptance_aql"),
                  optional_number(page.configuration.inference.acceptance_aql));
    object.insert(QStringLiteral("acceptance_rql"),
                  optional_number(page.configuration.inference.acceptance_rql));
    object.insert(QStringLiteral("acceptance_lot_size"),
                  optional_size(page.configuration.inference.acceptance_lot_size));
    object.insert(QStringLiteral("compute_partial_correlation"),
                  page.configuration.inference.compute_partial_correlation);
    object.insert(QStringLiteral("anom_alpha"), page.configuration.inference.anom_alpha);
    object.insert(QStringLiteral("correlation_method"),
                  QString::fromStdString(page.configuration.inference.correlation_method));
    object.insert(QStringLiteral("variance_method"),
                  QString::fromStdString(page.configuration.inference.variance_method));
    object.insert(QStringLiteral("proportion_method"),
                  QString::fromStdString(page.configuration.inference.proportion_method));
    object.insert(QStringLiteral("normality_method"),
                  QString::fromStdString(page.configuration.inference.normality_method));
    object.insert(QStringLiteral("equivalence_lower"),
                  optional_number(page.configuration.inference.equivalence_lower));
    object.insert(QStringLiteral("equivalence_upper"),
                  optional_number(page.configuration.inference.equivalence_upper));
    object.insert(QStringLiteral("equivalence_ratio_transform"),
                  QString::fromStdString(
                      page.configuration.inference.equivalence_ratio_transform));
    object.insert(QStringLiteral("nonparametric_posthoc"),
                  QString::fromStdString(
                      page.configuration.inference.nonparametric_posthoc));
    object.insert(QStringLiteral("rate_comparison"),
                  QString::fromStdString(page.configuration.inference.rate_comparison));
    object.insert(QStringLiteral("gage_measurement_column"),
                  optional_size(page.configuration.msa.gage_measurement_column));
    object.insert(QStringLiteral("gage_part_column"),
                  optional_size(page.configuration.msa.gage_part_column));
    object.insert(QStringLiteral("gage_operator_column"),
                  optional_size(page.configuration.msa.gage_operator_column));
    object.insert(QStringLiteral("ewma_lambda"), page.configuration.control.ewma_lambda);
    object.insert(QStringLiteral("ewma_limit_sigma"), page.configuration.control.ewma_limit_sigma);
    object.insert(QStringLiteral("cusum_target"), page.configuration.control.cusum_target);
    object.insert(QStringLiteral("cusum_sigma"), page.configuration.control.cusum_sigma);
    object.insert(QStringLiteral("cusum_k"), page.configuration.control.cusum_k);
    object.insert(QStringLiteral("cusum_h"), page.configuration.control.cusum_h);
    object.insert(QStringLiteral("cusum_fast_initial_response"),
                  page.configuration.control.cusum_fast_initial_response);
    object.insert(QStringLiteral("ma_window"), page.configuration.control.ma_window);
    object.insert(QStringLiteral("smoothing_alpha"), page.configuration.time_series.smoothing_alpha);
    object.insert(QStringLiteral("smoothing_gamma"), page.configuration.time_series.smoothing_gamma);
    object.insert(QStringLiteral("smoothing_method"),
                  QString::fromStdString(page.configuration.time_series.smoothing_method));
    object.insert(QStringLiteral("forecast_periods"), page.configuration.time_series.forecast_periods);
    object.insert(QStringLiteral("arima_time_column"),
                  optional_size(page.configuration.time_series.arima_time_column));
    object.insert(QStringLiteral("arima_value_column"),
                  optional_size(page.configuration.time_series.arima_value_column));
    object.insert(QStringLiteral("arima_differencing"), page.configuration.time_series.arima_differencing);
    object.insert(QStringLiteral("arima_selection_criterion"),
                  QString::fromStdString(page.configuration.time_series.arima_selection_criterion));
    object.insert(QStringLiteral("anova_response_column"),
                  optional_size(page.configuration.inference.anova_response_column));
    object.insert(QStringLiteral("anova_factor_a_column"),
                  optional_size(page.configuration.inference.anova_factor_a_column));
    object.insert(QStringLiteral("anova_factor_b_column"),
                  optional_size(page.configuration.inference.anova_factor_b_column));
    object.insert(QStringLiteral("anova_factor_encoding"),
                  QString::fromStdString(page.configuration.inference.anova_factor_encoding));
    object.insert(QStringLiteral("logistic_response_column"),
                  optional_size(page.configuration.inference.logistic_response_column));
    object.insert(QStringLiteral("logistic_predictor_columns"),
                  size_array(page.configuration.inference.logistic_predictor_columns));
    object.insert(QStringLiteral("logistic_event_level"),
                  QString::fromStdString(page.configuration.inference.logistic_event_level));
    object.insert(QStringLiteral("logistic_link"),
                  QString::fromStdString(page.configuration.inference.logistic_link));
    object.insert(QStringLiteral("logistic_max_iterations"),
                  page.configuration.inference.logistic_max_iterations);
    object.insert(QStringLiteral("logistic_tolerance"),
                  page.configuration.inference.logistic_tolerance);
    object.insert(QStringLiteral("variance_first_column"),
                  optional_size(page.configuration.inference.variance_first_column));
    object.insert(QStringLiteral("variance_second_column"),
                  optional_size(page.configuration.inference.variance_second_column));
    object.insert(QStringLiteral("variance_group_column"),
                  optional_size(page.configuration.inference.variance_group_column));
    object.insert(QStringLiteral("hypothesized_variance"),
                  optional_number(page.configuration.inference.hypothesized_variance));
    object.insert(QStringLiteral("variance_test_method"),
                  QString::fromStdString(page.configuration.inference.variance_test_method));
    object.insert(QStringLiteral("variance_alternative"),
                  QString::fromStdString(page.configuration.inference.variance_alternative));
    object.insert(QStringLiteral("decomposition_time_column"),
                  optional_size(page.configuration.time_series.decomposition_time_column));
    object.insert(QStringLiteral("decomposition_value_column"),
                  optional_size(page.configuration.time_series.decomposition_value_column));
    object.insert(QStringLiteral("decomposition_seasonal_period"),
                  page.configuration.time_series.decomposition_seasonal_period);
    object.insert(QStringLiteral("decomposition_model"),
                  QString::fromStdString(page.configuration.time_series.decomposition_model));
    object.insert(QStringLiteral("doe_factor_names"),
                  string_array(page.configuration.doe.factor_names));
    object.insert(QStringLiteral("doe_factor_columns"),
                  size_array(page.configuration.doe.factor_columns));
    object.insert(QStringLiteral("doe_response_column"),
                  optional_size(page.configuration.doe.response_column));
    object.insert(QStringLiteral("doe_response_columns"),
                  size_array(page.configuration.doe.response_columns));
    object.insert(QStringLiteral("doe_low_levels"),
                  string_array(page.configuration.doe.low_levels));
    object.insert(QStringLiteral("doe_high_levels"),
                  string_array(page.configuration.doe.high_levels));
    object.insert(QStringLiteral("doe_center_point_count"),
                  static_cast<qint64>(page.configuration.doe.center_point_count));
    object.insert(QStringLiteral("doe_fraction_p"),
                  static_cast<qint64>(page.configuration.doe.fraction_p));
    object.insert(QStringLiteral("doe_generators_text"),
                  QString::fromStdString(page.configuration.doe.generators_text));
    object.insert(QStringLiteral("doe_block_count"),
                  static_cast<qint64>(page.configuration.doe.block_count));
    object.insert(QStringLiteral("doe_randomize"), page.configuration.doe.randomize);
    object.insert(QStringLiteral("doe_random_seed"),
                  static_cast<qint64>(page.configuration.doe.random_seed));
    object.insert(QStringLiteral("doe_optimization_goal"),
                  QString::fromStdString(page.configuration.doe.optimization_goal));
    object.insert(QStringLiteral("doe_optimization_lower"),
                  optional_number(page.configuration.doe.optimization_lower));
    object.insert(QStringLiteral("doe_optimization_upper"),
                  optional_number(page.configuration.doe.optimization_upper));
    object.insert(QStringLiteral("doe_optimization_target"),
                  optional_number(page.configuration.doe.optimization_target));
    object.insert(QStringLiteral("doe_optimization_weight"),
                  page.configuration.doe.optimization_weight);
    object.insert(QStringLiteral("doe_optimization_confidence"),
                  page.configuration.doe.optimization_confidence);
    object.insert(QStringLiteral("doe_contour_x_factor"),
                  QString::fromStdString(page.configuration.doe.contour_x_factor));
    object.insert(QStringLiteral("doe_contour_y_factor"),
                  QString::fromStdString(page.configuration.doe.contour_y_factor));
    QJsonObject hold_actual;
    for (const auto& [name, value] : page.configuration.doe.contour_hold_actual) {
        hold_actual.insert(QString::fromStdString(name), QString::fromStdString(value));
    }
    object.insert(QStringLiteral("doe_contour_hold_actual"), hold_actual);
    QJsonArray optimization_objectives;
    for (const auto& objective : page.configuration.doe.optimization_objectives) {
        QJsonObject item;
        item.insert(QStringLiteral("goal"),
                    QString::fromStdString(objective.goal));
        item.insert(QStringLiteral("lower"), optional_number(objective.lower));
        item.insert(QStringLiteral("upper"), optional_number(objective.upper));
        item.insert(QStringLiteral("target"), optional_number(objective.target));
        item.insert(QStringLiteral("weight"), objective.weight);
        optimization_objectives.append(item);
    }
    object.insert(QStringLiteral("doe_optimization_objectives"), optimization_objectives);
    object.insert(QStringLiteral("nested_gage_measurement_column"),
                  optional_size(page.configuration.msa.nested_measurement_column));
    object.insert(QStringLiteral("nested_gage_part_column"),
                  optional_size(page.configuration.msa.nested_part_column));
    object.insert(QStringLiteral("nested_gage_operator_column"),
                  optional_size(page.configuration.msa.nested_operator_column));
    object.insert(QStringLiteral("gage_tolerance"), page.configuration.msa.gage_tolerance);
    object.insert(QStringLiteral("msa_reference_column"),
                  optional_size(page.configuration.msa.reference_column));
    object.insert(QStringLiteral("msa_time_column"),
                  optional_size(page.configuration.msa.time_column));
    object.insert(QStringLiteral("msa_reference_value"),
                  optional_number(page.configuration.msa.reference_value));
    object.insert(QStringLiteral("msa_process_variation"),
                  optional_number(page.configuration.msa.process_variation));
    object.insert(QStringLiteral("msa_mode"),
                  QString::fromStdString(page.configuration.msa.mode));
    object.insert(QStringLiteral("reliability_time_column"),
                  optional_size(page.configuration.reliability.time_column));
    object.insert(QStringLiteral("reliability_event_column"),
                  optional_size(page.configuration.reliability.event_column));
    object.insert(QStringLiteral("reliability_group_column"),
                  optional_size(page.configuration.reliability.group_column));
    object.insert(QStringLiteral("reliability_censoring_type_column"),
                  optional_size(page.configuration.reliability.censoring_type_column));
    object.insert(QStringLiteral("reliability_failure_mode_column"),
                  optional_size(page.configuration.reliability.failure_mode_column));
    object.insert(QStringLiteral("reliability_interval_left_column"),
                  optional_size(page.configuration.reliability.interval_left_column));
    object.insert(QStringLiteral("reliability_interval_right_column"),
                  optional_size(page.configuration.reliability.interval_right_column));
    object.insert(QStringLiteral("reliability_exposure_column"),
                  optional_size(page.configuration.reliability.exposure_column));
    object.insert(QStringLiteral("reliability_model"),
                  QString::fromStdString(page.configuration.reliability.model));
    object.insert(QStringLiteral("power_effect_size"), page.configuration.power.effect_size);
    object.insert(QStringLiteral("power_target"), page.configuration.power.target);
    object.insert(QStringLiteral("power_alpha"), page.configuration.power.alpha);
    object.insert(QStringLiteral("power_sample_size"),
                  static_cast<qint64>(page.configuration.power.sample_size));
    object.insert(QStringLiteral("power_mode"),
                  QString::fromStdString(page.configuration.power.mode));
    object.insert(QStringLiteral("power_group_count"),
                  static_cast<qint64>(page.configuration.power.group_count));
    object.insert(QStringLiteral("power_null_proportion"),
                  page.configuration.power.null_proportion);
    object.insert(QStringLiteral("power_second_proportion"),
                  page.configuration.power.second_proportion);
    object.insert(QStringLiteral("power_observation_length"),
                  page.configuration.power.observation_length);
    object.insert(QStringLiteral("power_variance_method"),
                  QString::fromStdString(page.configuration.power.variance_method));
    object.insert(QStringLiteral("power_sample_size_list"),
                  QString::fromStdString(page.configuration.power.sample_size_list));
    object.insert(QStringLiteral("power_effect_size_list"),
                  QString::fromStdString(page.configuration.power.effect_size_list));
    object.insert(QStringLiteral("power_doe_fraction_p"),
                  static_cast<qint64>(page.configuration.power.doe_fraction_p));
    object.insert(QStringLiteral("power_doe_replicates"),
                  static_cast<qint64>(page.configuration.power.doe_replicates));
    object.insert(QStringLiteral("power_equivalence_difference"),
                  page.configuration.power.equivalence_difference);
    if (page.configuration.power.equivalence_lower.has_value()) {
        object.insert(QStringLiteral("power_equivalence_lower"),
                      *page.configuration.power.equivalence_lower);
    }
    if (page.configuration.power.equivalence_upper.has_value()) {
        object.insert(QStringLiteral("power_equivalence_upper"),
                      *page.configuration.power.equivalence_upper);
    }
    object.insert(QStringLiteral("attribute_rating_column"),
                  optional_size(page.configuration.msa.attribute_rating_column));
    object.insert(QStringLiteral("attribute_part_column"),
                  optional_size(page.configuration.msa.attribute_part_column));
    object.insert(QStringLiteral("attribute_appraiser_column"),
                  optional_size(page.configuration.msa.attribute_appraiser_column));
    object.insert(QStringLiteral("attribute_standard_column"),
                  optional_size(page.configuration.msa.attribute_standard_column));
    object.insert(QStringLiteral("attribute_agreement_method"),
                  QString::fromStdString(page.configuration.msa.attribute_agreement_method));
    object.insert(QStringLiteral("ratings_are_ordinal"),
                  page.configuration.msa.ratings_are_ordinal);
    object.insert(QStringLiteral("kappa_weight_scheme"),
                  QString::fromStdString(page.configuration.msa.kappa_weight_scheme));
    object.insert(QStringLiteral("seasonal_period"),
                  static_cast<qint64>(page.configuration.time_series.seasonal_period));
    object.insert(QStringLiteral("seasonal_error_model"),
                  QString::fromStdString(page.configuration.time_series.seasonal_error_model));
    object.insert(QStringLiteral("seasonal_trend_model"),
                  QString::fromStdString(page.configuration.time_series.seasonal_trend_model));
    object.insert(QStringLiteral("seasonal_damped_trend"),
                  page.configuration.time_series.seasonal_damped_trend);
    object.insert(QStringLiteral("seasonal_beta"), page.configuration.time_series.seasonal_beta);
    object.insert(QStringLiteral("seasonal_damping_phi"),
                  page.configuration.time_series.seasonal_damping_phi);
    object.insert(QStringLiteral("validation_initial_size"),
                  static_cast<qint64>(page.configuration.time_series.validation_initial_size));
    object.insert(QStringLiteral("validation_horizon"),
                  static_cast<qint64>(page.configuration.time_series.validation_horizon));
    object.insert(QStringLiteral("validation_step"),
                  static_cast<qint64>(page.configuration.time_series.validation_step));
    object.insert(QStringLiteral("pca_variable_columns"),
                  size_array(page.configuration.pca.variable_columns));
    object.insert(QStringLiteral("pca_mode"),
                  QString::fromStdString(page.configuration.pca.mode));
    object.insert(QStringLiteral("pca_component_count"),
                  static_cast<qint64>(page.configuration.pca.component_count));
    object.insert(QStringLiteral("pca_anomaly_quantile"),
                  page.configuration.pca.anomaly_quantile);
    QJsonArray tables;
    for (const auto& table : page.tables) {
        QJsonObject table_object;
        table_object.insert(QStringLiteral("title"), QString::fromStdString(table.title));
        table_object.insert(QStringLiteral("headers"), string_array(table.headers));
        QJsonArray rows;
        for (const auto& row : table.rows) {
            rows.append(string_array(row));
        }
        table_object.insert(QStringLiteral("rows"), rows);
        if (!table.column_kinds.empty()) {
            table_object.insert(QStringLiteral("column_kinds"), string_array(table.column_kinds));
        }
        if (!table.row_ids.empty()) {
            QJsonArray ids;
            for (const domain::RowId id : table.row_ids) {
                ids.append(static_cast<qint64>(id));
            }
            table_object.insert(QStringLiteral("row_ids"), ids);
        }
        if (!table.rule_ids.empty()) {
            table_object.insert(QStringLiteral("rule_ids"), string_array(table.rule_ids));
        }
        tables.append(table_object);
    }
    object.insert(QStringLiteral("tables"), tables);

    QJsonArray plots;
    for (const auto& plot : page.plots) {
        QJsonObject plot_object;
        plot_object.insert(QStringLiteral("schema_version"), plot.schema_version);
        plot_object.insert(QStringLiteral("kind"), static_cast<int>(plot.kind));
        plot_object.insert(QStringLiteral("title"), QString::fromStdString(plot.title));
        plot_object.insert(QStringLiteral("x_axis_title"), QString::fromStdString(plot.x_axis_title));
        plot_object.insert(QStringLiteral("y_axis_title"), QString::fromStdString(plot.y_axis_title));
        plot_object.insert(QStringLiteral("center_label"), QString::fromStdString(plot.center_label));
        plot_object.insert(QStringLiteral("subtitle"), QString::fromStdString(plot.subtitle));
        plot_object.insert(QStringLiteral("show_grid"), plot.show_grid);
        plot_object.insert(QStringLiteral("show_legend"), plot.show_legend);
        plot_object.insert(QStringLiteral("line_width"), plot.line_width);
        plot_object.insert(QStringLiteral("legend_font_size"), plot.legend_font_size);
        plot_object.insert(QStringLiteral("title_font_size"), plot.title_font_size);
        plot_object.insert(QStringLiteral("axis_font_size"), plot.axis_font_size);
        plot_object.insert(QStringLiteral("theme_preset"),
                           QString::fromStdString(plot.theme_preset));
        plot_object.insert(QStringLiteral("y_min"), optional_number(plot.y_min));
        plot_object.insert(QStringLiteral("y_max"), optional_number(plot.y_max));
        plot_object.insert(QStringLiteral("x_min"), optional_number(plot.x_min));
        plot_object.insert(QStringLiteral("x_max"), optional_number(plot.x_max));
        plot_object.insert(QStringLiteral("data_region_fill"),
                           QString::fromStdString(plot.data_region_fill));
        plot_object.insert(QStringLiteral("grid_color"), QString::fromStdString(plot.grid_color));
        const auto write_series_style = [](QJsonObject& target,
                                           const domain::PlotSeriesStyle& style) {
            target.insert(QStringLiteral("visible"), style.visible);
            target.insert(QStringLiteral("color"), QString::fromStdString(style.color));
            target.insert(QStringLiteral("fill_color"), QString::fromStdString(style.fill_color));
            target.insert(QStringLiteral("line_style"), static_cast<int>(style.line_style));
            target.insert(QStringLiteral("point_style"), static_cast<int>(style.point_style));
            target.insert(QStringLiteral("line_width"), style.line_width);
            target.insert(QStringLiteral("point_size"), style.point_size);
            target.insert(QStringLiteral("opacity"), style.opacity);
        };
        const auto write_reference_style = [](QJsonObject& target,
                                              const domain::PlotReferenceStyle& style) {
            target.insert(QStringLiteral("visible"), style.visible);
            target.insert(QStringLiteral("label"), QString::fromStdString(style.label));
            target.insert(QStringLiteral("color"), QString::fromStdString(style.color));
            target.insert(QStringLiteral("line_style"), static_cast<int>(style.line_style));
            target.insert(QStringLiteral("line_width"), style.line_width);
        };
        QJsonObject value_style;
        write_series_style(value_style, plot.value_style);
        plot_object.insert(QStringLiteral("value_style"), value_style);
        QJsonObject center_style;
        write_reference_style(center_style, plot.center_style);
        plot_object.insert(QStringLiteral("center_style"), center_style);
        QJsonObject lower_style;
        write_reference_style(lower_style, plot.lower_style);
        plot_object.insert(QStringLiteral("lower_style"), lower_style);
        QJsonObject upper_style;
        write_reference_style(upper_style, plot.upper_style);
        plot_object.insert(QStringLiteral("upper_style"), upper_style);
        plot_object.insert(QStringLiteral("values"), number_array(plot.values));
        plot_object.insert(QStringLiteral("x_values"), number_array(plot.x_values));
        plot_object.insert(QStringLiteral("center"), number_array(plot.center));
        plot_object.insert(QStringLiteral("lower"), number_array(plot.lower));
        plot_object.insert(QStringLiteral("upper"), number_array(plot.upper));
        QJsonArray series;
        for (const domain::PlotSeries& item : plot.series) {
            QJsonObject series_object;
            series_object.insert(QStringLiteral("role"), static_cast<int>(item.role));
            series_object.insert(QStringLiteral("label"), QString::fromStdString(item.label));
            series_object.insert(QStringLiteral("values"), number_array(item.values));
            series_object.insert(QStringLiteral("x_values"), number_array(item.x_values));
            series_object.insert(QStringLiteral("lower"), number_array(item.lower));
            series_object.insert(QStringLiteral("upper"), number_array(item.upper));
            series_object.insert(QStringLiteral("line_width"), item.line_width);
            series_object.insert(QStringLiteral("show_points"), item.show_points);
            series_object.insert(QStringLiteral("visible"), item.style.visible);
            series_object.insert(QStringLiteral("color"), QString::fromStdString(item.style.color));
            series_object.insert(QStringLiteral("fill_color"),
                                 QString::fromStdString(item.style.fill_color));
            series_object.insert(QStringLiteral("line_style"),
                                 static_cast<int>(item.style.line_style));
            series_object.insert(QStringLiteral("point_style"),
                                 static_cast<int>(item.style.point_style));
            series_object.insert(QStringLiteral("point_size"), item.style.point_size);
            series_object.insert(QStringLiteral("opacity"), item.style.opacity);
            series.append(series_object);
        }
        plot_object.insert(QStringLiteral("series"), series);
        plot_object.insert(QStringLiteral("source_rows"), size_array(plot.source_rows));
        QJsonArray member_rows;
        for (const auto& members : plot.member_source_rows) {
            member_rows.append(size_array(members));
        }
        plot_object.insert(QStringLiteral("member_source_rows"), member_rows);
        plot_object.insert(QStringLiteral("sigma_z"), plot.sigma_z);
        QJsonArray special_points;
        for (const auto& points : plot.special_cause_points) {
            special_points.append(size_array(points));
        }
        plot_object.insert(QStringLiteral("special_cause_points"), special_points);
        plot_object.insert(QStringLiteral("triggered_tests"),
                           int_rows_array(plot.triggered_tests));
        plot_object.insert(QStringLiteral("primary_test_by_point"),
                           int_array(plot.primary_test_by_point));
        plot_object.insert(QStringLiteral("signal_direction"),
                           int_array(plot.signal_direction));
        plot_object.insert(QStringLiteral("histogram_edges"), number_array(plot.histogram_edges));
        plot_object.insert(QStringLiteral("histogram_counts"), number_array(plot.histogram_counts));
        plot_object.insert(QStringLiteral("lsl"), optional_number(plot.lsl));
        plot_object.insert(QStringLiteral("usl"), optional_number(plot.usl));
        plot_object.insert(QStringLiteral("target"), optional_number(plot.target));
        plot_object.insert(QStringLiteral("process_mean"), optional_number(plot.process_mean));
        plot_object.insert(QStringLiteral("within_sigma"), optional_number(plot.within_sigma));
        plot_object.insert(QStringLiteral("overall_sigma"), optional_number(plot.overall_sigma));
        plot_object.insert(QStringLiteral("categories"), string_array(plot.categories));
        plot_object.insert(QStringLiteral("category_values"), number_array(plot.category_values));
        plot_object.insert(QStringLiteral("cumulative_percent"), number_array(plot.cumulative_percent));
        plot_object.insert(QStringLiteral("box_min"), number_array(plot.box_min));
        plot_object.insert(QStringLiteral("box_q1"), number_array(plot.box_q1));
        plot_object.insert(QStringLiteral("box_median"), number_array(plot.box_median));
        plot_object.insert(QStringLiteral("box_q3"), number_array(plot.box_q3));
        plot_object.insert(QStringLiteral("box_max"), number_array(plot.box_max));
        plot_object.insert(QStringLiteral("box_labels"), string_array(plot.box_labels));
        plot_object.insert(QStringLiteral("interval_lower"), number_array(plot.interval_lower));
        plot_object.insert(QStringLiteral("interval_upper"), number_array(plot.interval_upper));
        plot_object.insert(QStringLiteral("interval_counts"), size_array(plot.interval_counts));
        plot_object.insert(QStringLiteral("point_labels"), string_array(plot.point_labels));
        plot_object.insert(QStringLiteral("point_groups"), string_array(plot.point_groups));
        plot_object.insert(QStringLiteral("bubble_sizes"), number_array(plot.bubble_sizes));
        plot_object.insert(QStringLiteral("matrix_labels"), string_array(plot.matrix_labels));
        QJsonArray matrix_values;
        for (const auto& row : plot.matrix_values) {
            matrix_values.append(number_array(row));
        }
        plot_object.insert(QStringLiteral("matrix_values"), matrix_values);
        QJsonArray matrix_counts;
        for (const auto& row : plot.matrix_counts) {
            matrix_counts.append(size_array(row));
        }
        plot_object.insert(QStringLiteral("matrix_counts"), matrix_counts);
        QJsonArray matrix_p_values;
        for (const auto& row : plot.matrix_p_values) {
            matrix_p_values.append(number_array(row));
        }
        plot_object.insert(QStringLiteral("matrix_p_values"), matrix_p_values);
        plot_object.insert(QStringLiteral("histogram_edges_y"),
                           number_array(plot.histogram_edges_y));
        plot_object.insert(QStringLiteral("histogram_counts_y"),
                           number_array(plot.histogram_counts_y));
        plot_object.insert(QStringLiteral("contour_x"), number_array(plot.contour_x));
        plot_object.insert(QStringLiteral("contour_y"), number_array(plot.contour_y));
        plot_object.insert(QStringLiteral("contour_levels"), number_array(plot.contour_levels));
        plot_object.insert(QStringLiteral("color_min"), optional_number(plot.color_min));
        plot_object.insert(QStringLiteral("color_max"), optional_number(plot.color_max));
        plots.append(plot_object);
    }
    object.insert(QStringLiteral("plots"), plots);
    QJsonArray diagnostics;
    for (const auto& diagnostic : page.diagnostics) {
        QJsonObject item;
        item.insert(QStringLiteral("severity"), static_cast<int>(diagnostic.severity));
        item.insert(QStringLiteral("code"), QString::fromStdString(diagnostic.code));
        item.insert(QStringLiteral("message"), QString::fromStdString(diagnostic.message));
        item.insert(QStringLiteral("related_rows"), row_array(diagnostic.related_rows));
        item.insert(QStringLiteral("related_columns"), size_array(diagnostic.related_columns));
        item.insert(QStringLiteral("related_plot_id"),
                    QString::fromStdString(diagnostic.related_plot_id));
        item.insert(QStringLiteral("suggested_action"),
                    QString::fromStdString(diagnostic.suggested_action));
        diagnostics.append(item);
    }
    object.insert(QStringLiteral("diagnostics"), diagnostics);
    QJsonArray interpretation;
    for (const auto& section : page.interpretation) {
        QJsonObject item;
        item.insert(QStringLiteral("heading"), QString::fromStdString(section.heading));
        item.insert(QStringLiteral("bullets"), string_array(section.bullets));
        item.insert(QStringLiteral("severity"), static_cast<int>(section.severity));
        interpretation.append(item);
    }
    object.insert(QStringLiteral("interpretation"), interpretation);
    if (page.worksheet_export.has_value()) {
        QJsonObject export_object;
        export_object.insert(QStringLiteral("name"),
                             QString::fromStdString(page.worksheet_export->name));
        export_object.insert(QStringLiteral("source_path"),
                             QString::fromStdString(page.worksheet_export->source_path));
        export_object.insert(QStringLiteral("columns"),
                             string_array(page.worksheet_export->columns));
        QJsonArray export_rows;
        for (const auto& row : page.worksheet_export->rows) {
            export_rows.append(string_array(row));
        }
        export_object.insert(QStringLiteral("rows"), export_rows);
        object.insert(QStringLiteral("worksheet_export"), export_object);
    }
    QJsonArray computation_traces;
    for (const auto& trace : page.computation_traces) {
        QJsonObject item;
        item.insert(QStringLiteral("formula_id"), QString::fromStdString(trace.formula_id));
        item.insert(QStringLiteral("title"), QString::fromStdString(trace.title));
        item.insert(QStringLiteral("plain_formula"),
                    QString::fromStdString(trace.plain_formula));
        item.insert(QStringLiteral("substituted_text"),
                    QString::fromStdString(trace.substituted_text));
        item.insert(QStringLiteral("result_symbol"),
                    QString::fromStdString(trace.result_symbol));
        item.insert(QStringLiteral("result_value"),
                    QString::fromStdString(trace.result_value));
        item.insert(QStringLiteral("evidence_type"),
                    QString::fromStdString(trace.evidence_type));
        item.insert(QStringLiteral("primary_url"),
                    QString::fromStdString(trace.primary_url));
        item.insert(QStringLiteral("command_id"),
                    QString::fromStdString(trace.command_id));
        QJsonArray bindings;
        for (const auto& binding : trace.bindings) {
            QJsonObject b;
            b.insert(QStringLiteral("symbol"), QString::fromStdString(binding.symbol));
            b.insert(QStringLiteral("label"), QString::fromStdString(binding.label));
            b.insert(QStringLiteral("value"), QString::fromStdString(binding.value));
            b.insert(QStringLiteral("role"), QString::fromStdString(binding.role));
            bindings.append(b);
        }
        item.insert(QStringLiteral("bindings"), bindings);
        QJsonArray steps;
        for (const auto& step : trace.steps) {
            QJsonObject s;
            s.insert(QStringLiteral("order"), step.order);
            s.insert(QStringLiteral("description"),
                     QString::fromStdString(step.description));
            s.insert(QStringLiteral("expression_before"),
                     QString::fromStdString(step.expression_before));
            s.insert(QStringLiteral("expression_after"),
                     QString::fromStdString(step.expression_after));
            s.insert(QStringLiteral("value"), QString::fromStdString(step.value));
            steps.append(s);
        }
        item.insert(QStringLiteral("steps"), steps);
        computation_traces.append(item);
    }
    object.insert(QStringLiteral("computation_traces"), computation_traces);
    return object;
}

domain::OutputPage output_page_from_json(const QJsonObject& object)
{
    domain::OutputPage page;
    page.id = object.value(QStringLiteral("id")).toString().toStdString();
    page.title = object.value(QStringLiteral("title")).toString().toStdString();
    page.method_name = object.value(QStringLiteral("method_name")).toString().toStdString();
    page.parameter_summary = object.value(QStringLiteral("parameter_summary")).toString().toStdString();
    page.analysis_command_id =
        object.value(QStringLiteral("analysis_command_id")).toString().toStdString();
    page.method_metadata.algorithm =
        object.value(QStringLiteral("method_algorithm")).toString().toStdString();
    page.method_metadata.version =
        object.value(QStringLiteral("method_version")).toString("1").toStdString();
    page.method_metadata.parameters =
        object.value(QStringLiteral("method_parameters")).toString().toStdString();
    page.method_metadata.missing_policy =
        object.value(QStringLiteral("method_missing_policy"))
            .toString("skip_missing").toStdString();
    page.method_metadata.estimation_method =
        object.value(QStringLiteral("method_estimation_method")).toString().toStdString();
    page.method_metadata.source_rows =
        to_rows(object.value(QStringLiteral("method_source_rows")).toArray());
    page.method_metadata.diagnostic_codes =
        to_strings(object.value(QStringLiteral("method_diagnostic_codes")).toArray());
    page.method_metadata.assumption_status =
        object.value(QStringLiteral("method_assumption_status"))
            .toString("not_verified").toStdString();
    page.method_metadata.parameter_source =
        object.value(QStringLiteral("method_parameter_source")).toString().toStdString();
    page.method_metadata.valid_count = static_cast<std::size_t>(
        object.value(QStringLiteral("method_valid_count")).toInt(0));
    page.method_metadata.missing_count = static_cast<std::size_t>(
        object.value(QStringLiteral("method_missing_count")).toInt(0));
    page.method_metadata.not_computed_reason =
        object.value(QStringLiteral("method_not_computed_reason")).toString().toStdString();
    page.configuration.chart_type = object.value(QStringLiteral("chart_type")).toString().toStdString();
    page.configuration.capability_method =
        object.value(QStringLiteral("capability_method"))
            .toString(QStringLiteral("normal")).toStdString();
    page.configuration.nonnormal_distribution =
        object.value(QStringLiteral("nonnormal_distribution"))
            .toString(QStringLiteral("weibull")).toStdString();
    page.configuration.analysis_name = object.value(QStringLiteral("analysis_name")).toString().toStdString();
    page.configuration.graph.graph_kind =
        object.value(QStringLiteral("graph_kind")).toString().toStdString();
    page.configuration.graph.x_column =
        read_optional_size(object.value(QStringLiteral("graph_x_column")));
    page.configuration.graph.y_column =
        read_optional_size(object.value(QStringLiteral("graph_y_column")));
    page.configuration.graph.size_column =
        read_optional_size(object.value(QStringLiteral("graph_size_column")));
    page.configuration.graph.by_column =
        read_optional_size(object.value(QStringLiteral("graph_by_column")));
    page.configuration.graph.facet_column =
        read_optional_size(object.value(QStringLiteral("graph_facet_column")));
    page.configuration.graph.facet_max_panels =
        object.value(QStringLiteral("graph_facet_max_panels")).toInt(6);
    if (page.configuration.graph.facet_max_panels < 1) {
        page.configuration.graph.facet_max_panels = 1;
    }
    if (page.configuration.graph.facet_max_panels > 12) {
        page.configuration.graph.facet_max_panels = 12;
    }
    page.configuration.graph.label_column =
        read_optional_size(object.value(QStringLiteral("graph_label_column")));
    page.configuration.graph.variable_columns =
        to_sizes(object.value(QStringLiteral("graph_variable_columns")).toArray());
    page.configuration.graph.correlation_method =
        object.value(QStringLiteral("graph_correlation_method"))
            .toString(QStringLiteral("pearson")).toStdString();
    page.configuration.graph.confidence_level =
        object.value(QStringLiteral("graph_confidence_level")).toDouble(0.95);
    page.configuration.graph.interval_type =
        object.value(QStringLiteral("graph_interval_type"))
            .toString(QStringLiteral("mean_t")).toStdString();
    page.configuration.graph.show_p_values =
        object.value(QStringLiteral("graph_show_p_values")).toBool(false);
    page.configuration.graph.z_column =
        read_optional_size(object.value(QStringLiteral("graph_z_column")));
    page.configuration.graph.time_column =
        read_optional_size(object.value(QStringLiteral("graph_time_column")));
    page.configuration.graph.weight_column =
        read_optional_size(object.value(QStringLiteral("graph_weight_column")));
    page.configuration.graph.bin_count =
        object.value(QStringLiteral("graph_bin_count")).toInt(0);
    page.configuration.graph.other_threshold_percent =
        object.value(QStringLiteral("graph_other_threshold_percent")).toDouble(5.0);
    page.configuration.graph.show_normal_reference =
        object.value(QStringLiteral("graph_show_normal_reference")).toBool(false);
    page.configuration.graph.connect_missing =
        object.value(QStringLiteral("graph_connect_missing")).toBool(true);
    page.configuration.graph.stack_mode =
        object.value(QStringLiteral("graph_stack_mode"))
            .toString(QStringLiteral("none")).toStdString();
    page.configuration.graph.color_scale =
        object.value(QStringLiteral("graph_color_scale"))
            .toString(QStringLiteral("auto")).toStdString();
    page.configuration.graph.contour_levels =
        object.value(QStringLiteral("graph_contour_levels")).toInt(8);
    page.configuration.graph.payload_version =
        object.value(QStringLiteral("graph_payload_version")).toInt(1);
    const int schema_version = object.value(QStringLiteral("schema_version")).toInt(1);
    page.configuration.variable_columns =
        to_sizes(object.value(QStringLiteral("variable_columns")).toArray());
    page.configuration.excluded_rows =
        to_sizes(object.value(QStringLiteral("excluded_rows")).toArray());
    page.configuration.hidden_rows =
        to_sizes(object.value(QStringLiteral("hidden_rows")).toArray());
    page.configuration.included_rows =
        to_sizes(object.value(QStringLiteral("included_rows")).toArray());
    page.configuration.control.stage_column =
        read_optional_size(object.value(QStringLiteral("stage_column")));
    page.configuration.selection.measurement_column = static_cast<std::size_t>(
        std::max<qint64>(0, object.value(QStringLiteral("measurement_column")).toInteger()));
    page.configuration.selection.subgroup_column =
        read_optional_size(object.value(QStringLiteral("subgroup_column")));
    page.configuration.selection.time_column =
        read_optional_size(object.value(QStringLiteral("time_column")));
    page.configuration.selection.product_column =
        read_optional_size(object.value(QStringLiteral("product_column")));
    page.configuration.selection.defect_count_column =
        read_optional_size(object.value(QStringLiteral("defect_count_column")));
    page.configuration.selection.inspected_count_column =
        read_optional_size(object.value(QStringLiteral("inspected_count_column")));
    page.configuration.inspected_constant =
        read_optional_size(object.value(QStringLiteral("inspected_constant")));
    page.configuration.inference.first_events_column =
        read_optional_size(object.value(QStringLiteral("first_events_column")));
    page.configuration.inference.first_trials_column =
        read_optional_size(object.value(QStringLiteral("first_trials_column")));
    page.configuration.inference.second_events_column =
        read_optional_size(object.value(QStringLiteral("second_events_column")));
    page.configuration.inference.second_trials_column =
        read_optional_size(object.value(QStringLiteral("second_trials_column")));
    page.configuration.inference.row_category_column =
        read_optional_size(object.value(QStringLiteral("row_category_column")));
    page.configuration.inference.column_category_column =
        read_optional_size(object.value(QStringLiteral("column_category_column")));
    page.configuration.inference.expected_proportions =
        object.value(QStringLiteral("expected_proportions")).toString().toStdString();
    page.configuration.inference.gof_category_column =
        read_optional_size(object.value(QStringLiteral("gof_category_column")));
    page.configuration.specifications.lower =
        read_optional(object.value(QStringLiteral("lower_spec")));
    page.configuration.specifications.upper =
        read_optional(object.value(QStringLiteral("upper_spec")));
    page.configuration.specifications.target =
        read_optional(object.value(QStringLiteral("target_spec")));
    page.configuration.by_column =
        read_optional_size(object.value(QStringLiteral("by_column")));
    page.configuration.control.moving_range_length =
        object.value(QStringLiteral("moving_range_length")).toInt(2);
    page.configuration.control.sigma_method =
        object.value(QStringLiteral("sigma_method"))
            .toString("average_moving_range").toStdString();
    page.configuration.control.use_nelson_estimate =
        object.value(QStringLiteral("use_nelson_estimate")).toBool(false);
    page.configuration.control.historical_center =
        read_optional(object.value(QStringLiteral("historical_center")));
    page.configuration.control.historical_sigma =
        read_optional(object.value(QStringLiteral("historical_sigma")));
    page.configuration.control.historical_sigma_z =
        read_optional(object.value(QStringLiteral("historical_sigma_z")));
    page.configuration.leave_gaps_for_excluded =
        object.value(QStringLiteral("leave_gaps_for_excluded")).toBool(false);
    page.configuration.control.enabled_special_cause_tests.clear();
    if (object.contains(QStringLiteral("enabled_special_cause_tests"))) {
        for (const QJsonValue& value : object.value(
                 QStringLiteral("enabled_special_cause_tests")).toArray()) {
            page.configuration.control.enabled_special_cause_tests.push_back(value.toInt());
        }
        page.configuration.control.special_cause_rule_policy =
            object.value(QStringLiteral("special_cause_rule_policy"))
                .toString(QStringLiteral("explicit")).toStdString();
    } else {
        page.configuration.control.enabled_special_cause_tests = {1};
        page.configuration.control.special_cause_rule_policy = "explicit";
    }
    if (object.contains(QStringLiteral("special_cause_rule_policy"))) {
        page.configuration.control.special_cause_rule_policy =
            object.value(QStringLiteral("special_cause_rule_policy")).toString().toStdString();
    }
    if (object.value(QStringLiteral("subgroup_size")).isDouble()) {
        page.configuration.control.subgroup_size =
            object.value(QStringLiteral("subgroup_size")).toInt();
    }
    page.configuration.pareto_other_threshold_percent =
        read_optional(object.value(QStringLiteral("pareto_other_threshold_percent")));
    page.configuration.effect_title =
        object.value(QStringLiteral("effect_title")).toString().toStdString();
    page.configuration.inference.hypothesis_mean =
        read_optional(object.value(QStringLiteral("hypothesis_mean")));
    page.configuration.inference.confidence_level =
        object.value(QStringLiteral("confidence_level")).toDouble(0.95);
    page.configuration.inference.coverage_proportion =
        read_optional(object.value(QStringLiteral("coverage_proportion")));
    page.configuration.inference.alternative =
        object.value(QStringLiteral("alternative")).toString("two_sided").toStdString();
    {
        std::string runs_criterion =
            object.value(QStringLiteral("runs_criterion"))
                .toString(QStringLiteral("mean")).toStdString();
        if (runs_criterion != "median" && runs_criterion != "value") {
            runs_criterion = "mean";
        }
        page.configuration.inference.runs_criterion = std::move(runs_criterion);
    }
    {
        std::string outlier_method =
            object.value(QStringLiteral("outlier_method"))
                .toString(QStringLiteral("grubbs")).toStdString();
        if (outlier_method != "dixon_r10") {
            outlier_method = "grubbs";
        }
        page.configuration.inference.outlier_method = std::move(outlier_method);
    }
    {
        std::string tolerance_method =
            object.value(QStringLiteral("tolerance_method"))
                .toString(QStringLiteral("normal")).toStdString();
        if (tolerance_method != "nonparametric") {
            tolerance_method = "normal";
        }
        page.configuration.inference.tolerance_method = std::move(tolerance_method);
    }
    page.configuration.inference.known_sigma =
        read_optional(object.value(QStringLiteral("known_sigma")));
    page.configuration.inference.acceptance_sample_size = static_cast<std::size_t>(
        object.value(QStringLiteral("acceptance_sample_size")).toInteger(0));
    page.configuration.inference.acceptance_number = static_cast<std::size_t>(
        object.value(QStringLiteral("acceptance_number")).toInteger(0));
    page.configuration.inference.acceptance_aql =
        read_optional(object.value(QStringLiteral("acceptance_aql")));
    page.configuration.inference.acceptance_rql =
        read_optional(object.value(QStringLiteral("acceptance_rql")));
    page.configuration.inference.acceptance_lot_size =
        read_optional_size(object.value(QStringLiteral("acceptance_lot_size")));
    page.configuration.inference.compute_partial_correlation =
        object.value(QStringLiteral("compute_partial_correlation")).toBool(false);
    page.configuration.inference.anom_alpha =
        object.value(QStringLiteral("anom_alpha")).toDouble(0.05);
    page.configuration.inference.correlation_method =
        object.value(QStringLiteral("correlation_method")).toString("pearson").toStdString();
    page.configuration.inference.variance_method =
        object.value(QStringLiteral("variance_method")).toString("welch").toStdString();
    page.configuration.inference.proportion_method =
        object.value(QStringLiteral("proportion_method")).toString("exact").toStdString();
    page.configuration.inference.normality_method =
        object.value(QStringLiteral("normality_method"))
            .toString(QStringLiteral("anderson_darling")).toStdString();
    if (page.configuration.inference.normality_method != "ryan_joiner") {
        page.configuration.inference.normality_method = "anderson_darling";
    }
    page.configuration.inference.equivalence_lower =
        read_optional(object.value(QStringLiteral("equivalence_lower")));
    page.configuration.inference.equivalence_upper =
        read_optional(object.value(QStringLiteral("equivalence_upper")));
    page.configuration.inference.equivalence_ratio_transform =
        object.value(QStringLiteral("equivalence_ratio_transform"))
            .toString(QStringLiteral("none")).toStdString();
    if (page.configuration.inference.equivalence_ratio_transform != "log") {
        page.configuration.inference.equivalence_ratio_transform = "none";
    }
    page.configuration.inference.nonparametric_posthoc =
        object.value(QStringLiteral("nonparametric_posthoc"))
            .toString()
            .toStdString();
    {
        const std::string& posthoc =
            page.configuration.inference.nonparametric_posthoc;
        if (posthoc != "steel_dwass" && posthoc != "nemenyi" && posthoc != "none"
            && !posthoc.empty()) {
            page.configuration.inference.nonparametric_posthoc = "dunn";
        }
    }
    page.configuration.inference.rate_comparison =
        object.value(QStringLiteral("rate_comparison"))
            .toString(QStringLiteral("difference")).toStdString();
    if (page.configuration.inference.rate_comparison.empty()) {
        page.configuration.inference.rate_comparison = "difference";
    }
    page.configuration.msa.gage_measurement_column =
        read_optional_size(object.value(QStringLiteral("gage_measurement_column")));
    page.configuration.msa.gage_part_column =
        read_optional_size(object.value(QStringLiteral("gage_part_column")));
    page.configuration.msa.gage_operator_column =
        read_optional_size(object.value(QStringLiteral("gage_operator_column")));
    page.configuration.control.ewma_lambda = object.value(QStringLiteral("ewma_lambda")).toDouble(0.2);
    page.configuration.control.ewma_limit_sigma =
        object.value(QStringLiteral("ewma_limit_sigma")).toDouble(3.0);
    page.configuration.control.cusum_target = object.value(QStringLiteral("cusum_target")).toDouble(0.0);
    page.configuration.control.cusum_sigma = object.value(QStringLiteral("cusum_sigma")).toDouble(1.0);
    page.configuration.control.cusum_k = object.value(QStringLiteral("cusum_k")).toDouble(0.5);
    page.configuration.control.cusum_h = object.value(QStringLiteral("cusum_h")).toDouble(4.0);
    page.configuration.control.cusum_fast_initial_response =
        object.value(QStringLiteral("cusum_fast_initial_response")).toBool(false);
    page.configuration.control.ma_window =
        object.value(QStringLiteral("ma_window")).toInt(3);
    page.configuration.time_series.smoothing_alpha =
        object.value(QStringLiteral("smoothing_alpha")).toDouble(0.2);
    page.configuration.time_series.smoothing_gamma =
        object.value(QStringLiteral("smoothing_gamma")).toDouble(0.2);
    page.configuration.time_series.smoothing_method =
        object.value(QStringLiteral("smoothing_method")).toString("double").toStdString();
    page.configuration.time_series.forecast_periods =
        object.value(QStringLiteral("forecast_periods")).toInt(1);
    page.configuration.time_series.arima_time_column =
        read_optional_size(object.value(QStringLiteral("arima_time_column")));
    page.configuration.time_series.arima_value_column =
        read_optional_size(object.value(QStringLiteral("arima_value_column")));
    page.configuration.time_series.arima_differencing =
        object.value(QStringLiteral("arima_differencing")).toInt(1);
    page.configuration.time_series.arima_selection_criterion =
        object.value(QStringLiteral("arima_selection_criterion")).toString("aicc").toStdString();
    page.configuration.inference.anova_response_column =
        read_optional_size(object.value(QStringLiteral("anova_response_column")));
    page.configuration.inference.anova_factor_a_column =
        read_optional_size(object.value(QStringLiteral("anova_factor_a_column")));
    page.configuration.inference.anova_factor_b_column =
        read_optional_size(object.value(QStringLiteral("anova_factor_b_column")));
    page.configuration.inference.anova_factor_encoding =
        object.value(QStringLiteral("anova_factor_encoding")).toString("reference").toStdString();
    page.configuration.inference.logistic_response_column =
        read_optional_size(object.value(QStringLiteral("logistic_response_column")));
    page.configuration.inference.logistic_predictor_columns =
        to_sizes(object.value(QStringLiteral("logistic_predictor_columns")).toArray());
    page.configuration.inference.logistic_event_level =
        object.value(QStringLiteral("logistic_event_level")).toString("1").toStdString();
    page.configuration.inference.logistic_link =
        object.value(QStringLiteral("logistic_link")).toString("logit").toStdString();
    page.configuration.inference.logistic_max_iterations =
        object.value(QStringLiteral("logistic_max_iterations")).toInt(100);
    page.configuration.inference.logistic_tolerance =
        object.value(QStringLiteral("logistic_tolerance")).toDouble(1.0e-8);
    page.configuration.inference.variance_first_column =
        read_optional_size(object.value(QStringLiteral("variance_first_column")));
    page.configuration.inference.variance_second_column =
        read_optional_size(object.value(QStringLiteral("variance_second_column")));
    page.configuration.inference.variance_group_column =
        read_optional_size(object.value(QStringLiteral("variance_group_column")));
    page.configuration.inference.hypothesized_variance =
        read_optional(object.value(QStringLiteral("hypothesized_variance")));
    page.configuration.inference.variance_test_method =
        object.value(QStringLiteral("variance_test_method")).toString("f").toStdString();
    page.configuration.inference.variance_alternative =
        object.value(QStringLiteral("variance_alternative"))
            .toString("two_sided").toStdString();
    page.configuration.time_series.decomposition_time_column =
        read_optional_size(object.value(QStringLiteral("decomposition_time_column")));
    page.configuration.time_series.decomposition_value_column =
        read_optional_size(object.value(QStringLiteral("decomposition_value_column")));
    page.configuration.time_series.decomposition_seasonal_period =
        object.value(QStringLiteral("decomposition_seasonal_period")).toInt(1);
    page.configuration.time_series.decomposition_model =
        object.value(QStringLiteral("decomposition_model"))
            .toString("additive").toStdString();
    page.configuration.doe.factor_names =
        to_strings(object.value(QStringLiteral("doe_factor_names")).toArray());
    page.configuration.doe.factor_columns =
        to_sizes(object.value(QStringLiteral("doe_factor_columns")).toArray());
    page.configuration.doe.response_column =
        read_optional_size(object.value(QStringLiteral("doe_response_column")));
    page.configuration.doe.response_columns =
        to_sizes(object.value(QStringLiteral("doe_response_columns")).toArray());
    if (page.configuration.doe.response_columns.empty()
        && page.configuration.doe.response_column.has_value()) {
        page.configuration.doe.response_columns.push_back(
            *page.configuration.doe.response_column);
    }
    page.configuration.doe.low_levels =
        to_strings(object.value(QStringLiteral("doe_low_levels")).toArray());
    page.configuration.doe.high_levels =
        to_strings(object.value(QStringLiteral("doe_high_levels")).toArray());
    page.configuration.doe.center_point_count =
        static_cast<std::size_t>(object.value(QStringLiteral("doe_center_point_count")).toInteger());
    page.configuration.doe.fraction_p =
        static_cast<std::size_t>(object.value(QStringLiteral("doe_fraction_p")).toInteger());
    page.configuration.doe.generators_text =
        object.value(QStringLiteral("doe_generators_text")).toString().toStdString();
    page.configuration.doe.block_count =
        static_cast<std::size_t>(object.value(QStringLiteral("doe_block_count")).toInteger(1));
    page.configuration.doe.randomize =
        object.value(QStringLiteral("doe_randomize")).toBool(false);
    page.configuration.doe.random_seed =
        static_cast<std::uint64_t>(object.value(QStringLiteral("doe_random_seed")).toInteger());
    page.configuration.doe.optimization_goal =
        object.value(QStringLiteral("doe_optimization_goal"))
            .toString(QStringLiteral("maximize")).toStdString();
    page.configuration.doe.optimization_lower =
        read_optional(object.value(QStringLiteral("doe_optimization_lower")));
    page.configuration.doe.optimization_upper =
        read_optional(object.value(QStringLiteral("doe_optimization_upper")));
    page.configuration.doe.optimization_target =
        read_optional(object.value(QStringLiteral("doe_optimization_target")));
    page.configuration.doe.optimization_weight =
        object.value(QStringLiteral("doe_optimization_weight")).toDouble(1.0);
    page.configuration.doe.optimization_confidence =
        object.value(QStringLiteral("doe_optimization_confidence")).toDouble(0.95);
    page.configuration.doe.contour_x_factor =
        object.value(QStringLiteral("doe_contour_x_factor")).toString().toStdString();
    page.configuration.doe.contour_y_factor =
        object.value(QStringLiteral("doe_contour_y_factor")).toString().toStdString();
    page.configuration.doe.contour_hold_actual.clear();
    const QJsonObject hold_actual =
        object.value(QStringLiteral("doe_contour_hold_actual")).toObject();
    for (auto it = hold_actual.begin(); it != hold_actual.end(); ++it) {
        page.configuration.doe.contour_hold_actual[it.key().toStdString()] =
            it.value().toString().toStdString();
    }
    page.configuration.doe.optimization_objectives.clear();
    for (const QJsonValue& value :
         object.value(QStringLiteral("doe_optimization_objectives")).toArray()) {
        const QJsonObject item = value.toObject();
        domain::DoeResponseObjectiveConfig objective;
        objective.goal = item.value(QStringLiteral("goal"))
                             .toString(QStringLiteral("maximize")).toStdString();
        objective.lower = read_optional(item.value(QStringLiteral("lower")));
        objective.upper = read_optional(item.value(QStringLiteral("upper")));
        objective.target = read_optional(item.value(QStringLiteral("target")));
        objective.weight = item.value(QStringLiteral("weight")).toDouble(1.0);
        page.configuration.doe.optimization_objectives.push_back(objective);
    }
    page.configuration.msa.nested_measurement_column =
        read_optional_size(object.value(QStringLiteral("nested_gage_measurement_column")));
    page.configuration.msa.nested_part_column =
        read_optional_size(object.value(QStringLiteral("nested_gage_part_column")));
    page.configuration.msa.nested_operator_column =
        read_optional_size(object.value(QStringLiteral("nested_gage_operator_column")));
    page.configuration.msa.gage_tolerance =
        object.value(QStringLiteral("gage_tolerance")).toDouble(0.0);
    page.configuration.msa.reference_column =
        read_optional_size(object.value(QStringLiteral("msa_reference_column")));
    page.configuration.msa.time_column =
        read_optional_size(object.value(QStringLiteral("msa_time_column")));
    page.configuration.msa.reference_value =
        read_optional(object.value(QStringLiteral("msa_reference_value")));
    page.configuration.msa.process_variation =
        read_optional(object.value(QStringLiteral("msa_process_variation")));
    page.configuration.msa.mode =
        object.value(QStringLiteral("msa_mode")).toString(QStringLiteral("type1")).toStdString();
    page.configuration.reliability.time_column =
        read_optional_size(object.value(QStringLiteral("reliability_time_column")));
    page.configuration.reliability.event_column =
        read_optional_size(object.value(QStringLiteral("reliability_event_column")));
    page.configuration.reliability.group_column =
        read_optional_size(object.value(QStringLiteral("reliability_group_column")));
    page.configuration.reliability.censoring_type_column =
        read_optional_size(object.value(QStringLiteral("reliability_censoring_type_column")));
    page.configuration.reliability.failure_mode_column =
        read_optional_size(object.value(QStringLiteral("reliability_failure_mode_column")));
    page.configuration.reliability.interval_left_column =
        read_optional_size(object.value(QStringLiteral("reliability_interval_left_column")));
    page.configuration.reliability.interval_right_column =
        read_optional_size(object.value(QStringLiteral("reliability_interval_right_column")));
    page.configuration.reliability.exposure_column =
        read_optional_size(object.value(QStringLiteral("reliability_exposure_column")));
    page.configuration.reliability.model =
        object.value(QStringLiteral("reliability_model"))
            .toString(QStringLiteral("kaplan_meier")).toStdString();
    page.configuration.power.effect_size =
        object.value(QStringLiteral("power_effect_size")).toDouble(0.5);
    page.configuration.power.target =
        object.value(QStringLiteral("power_target")).toDouble(0.8);
    page.configuration.power.alpha =
        object.value(QStringLiteral("power_alpha")).toDouble(0.05);
    page.configuration.power.sample_size =
        static_cast<std::size_t>(object.value(QStringLiteral("power_sample_size")).toInteger());
    page.configuration.power.mode =
        object.value(QStringLiteral("power_mode"))
            .toString(QStringLiteral("one_sample_sample_size")).toStdString();
    page.configuration.power.group_count =
        static_cast<std::size_t>(object.value(QStringLiteral("power_group_count")).toInteger(3));
    page.configuration.power.null_proportion =
        object.value(QStringLiteral("power_null_proportion")).toDouble(0.5);
    page.configuration.power.second_proportion =
        object.value(QStringLiteral("power_second_proportion")).toDouble(0.7);
    page.configuration.power.observation_length =
        object.value(QStringLiteral("power_observation_length")).toDouble(1.0);
    if (!(page.configuration.power.observation_length > 0.0)) {
        page.configuration.power.observation_length = 1.0;
    }
    page.configuration.power.variance_method =
        object.value(QStringLiteral("power_variance_method"))
            .toString(QStringLiteral("pooled")).toStdString();
    page.configuration.power.sample_size_list =
        object.value(QStringLiteral("power_sample_size_list")).toString().toStdString();
    page.configuration.power.effect_size_list =
        object.value(QStringLiteral("power_effect_size_list")).toString().toStdString();
    page.configuration.power.doe_fraction_p = static_cast<std::size_t>(
        object.value(QStringLiteral("power_doe_fraction_p")).toInteger(0));
    page.configuration.power.doe_replicates = static_cast<std::size_t>(
        std::max<qint64>(1, object.value(QStringLiteral("power_doe_replicates")).toInteger(1)));
    page.configuration.power.equivalence_difference =
        object.value(QStringLiteral("power_equivalence_difference")).toDouble(0.0);
    if (object.contains(QStringLiteral("power_equivalence_lower"))) {
        page.configuration.power.equivalence_lower =
            object.value(QStringLiteral("power_equivalence_lower")).toDouble();
    }
    if (object.contains(QStringLiteral("power_equivalence_upper"))) {
        page.configuration.power.equivalence_upper =
            object.value(QStringLiteral("power_equivalence_upper")).toDouble();
    }
    page.configuration.msa.attribute_rating_column =
        read_optional_size(object.value(QStringLiteral("attribute_rating_column")));
    page.configuration.msa.attribute_part_column =
        read_optional_size(object.value(QStringLiteral("attribute_part_column")));
    page.configuration.msa.attribute_appraiser_column =
        read_optional_size(object.value(QStringLiteral("attribute_appraiser_column")));
    page.configuration.msa.attribute_standard_column =
        read_optional_size(object.value(QStringLiteral("attribute_standard_column")));
    page.configuration.msa.attribute_agreement_method =
        object.value(QStringLiteral("attribute_agreement_method"))
            .toString("kappa").toStdString();
    page.configuration.msa.ratings_are_ordinal =
        object.value(QStringLiteral("ratings_are_ordinal")).toBool(false);
    page.configuration.msa.kappa_weight_scheme =
        object.value(QStringLiteral("kappa_weight_scheme")).toString("none").toStdString();
    if (page.configuration.msa.kappa_weight_scheme.empty()) {
        page.configuration.msa.kappa_weight_scheme = "none";
    }
    page.configuration.time_series.seasonal_period =
        static_cast<std::size_t>(object.value(QStringLiteral("seasonal_period")).toInteger(1));
    page.configuration.time_series.seasonal_error_model =
        object.value(QStringLiteral("seasonal_error_model"))
            .toString("additive").toStdString();
    page.configuration.time_series.seasonal_trend_model =
        object.value(QStringLiteral("seasonal_trend_model"))
            .toString("additive").toStdString();
    page.configuration.time_series.seasonal_damped_trend =
        object.value(QStringLiteral("seasonal_damped_trend")).toBool(false);
    page.configuration.time_series.seasonal_beta =
        object.value(QStringLiteral("seasonal_beta")).toDouble(0.1);
    page.configuration.time_series.seasonal_damping_phi =
        object.value(QStringLiteral("seasonal_damping_phi")).toDouble(0.98);
    page.configuration.time_series.validation_initial_size =
        static_cast<std::size_t>(object.value(QStringLiteral("validation_initial_size"))
                                      .toInteger());
    page.configuration.time_series.validation_horizon =
        static_cast<std::size_t>(object.value(QStringLiteral("validation_horizon"))
                                      .toInteger(1));
    page.configuration.time_series.validation_step =
        static_cast<std::size_t>(object.value(QStringLiteral("validation_step"))
                                      .toInteger(1));
    page.configuration.pca.variable_columns =
        to_sizes(object.value(QStringLiteral("pca_variable_columns")).toArray());
    page.configuration.pca.mode =
        object.value(QStringLiteral("pca_mode")).toString("covariance").toStdString();
    page.configuration.pca.component_count =
        static_cast<std::size_t>(object.value(QStringLiteral("pca_component_count"))
                                      .toInteger());
    page.configuration.pca.anomaly_quantile =
        object.value(QStringLiteral("pca_anomaly_quantile")).toDouble(0.99);
    if (schema_version < 2) {
        page.configuration.selection.measurement_column = 0;
    }
    for (const QJsonValue& table_value : object.value(QStringLiteral("tables")).toArray()) {
        const QJsonObject table_object = table_value.toObject();
        domain::StatisticTable table;
        table.title = table_object.value(QStringLiteral("title")).toString().toStdString();
        table.headers = to_strings(table_object.value(QStringLiteral("headers")).toArray());
        for (const QJsonValue& row : table_object.value(QStringLiteral("rows")).toArray()) {
            table.rows.push_back(to_strings(row.toArray()));
        }
        table.column_kinds =
            to_strings(table_object.value(QStringLiteral("column_kinds")).toArray());
        for (const QJsonValue& id : table_object.value(QStringLiteral("row_ids")).toArray()) {
            table.row_ids.push_back(static_cast<domain::RowId>(id.toInteger()));
        }
        table.rule_ids = to_strings(table_object.value(QStringLiteral("rule_ids")).toArray());
        page.tables.push_back(table);
    }
    for (const QJsonValue& plot_value : object.value(QStringLiteral("plots")).toArray()) {
        const QJsonObject plot_object = plot_value.toObject();
        domain::PlotSpec plot;
        plot.schema_version = plot_object.value(QStringLiteral("schema_version")).toInt(1);
        plot.kind = static_cast<domain::PlotKind>(plot_object.value(QStringLiteral("kind")).toInt());
        plot.title = plot_object.value(QStringLiteral("title")).toString().toStdString();
        plot.x_axis_title = plot_object.value(QStringLiteral("x_axis_title")).toString().toStdString();
        plot.y_axis_title = plot_object.value(QStringLiteral("y_axis_title")).toString().toStdString();
        plot.center_label = plot_object.value(QStringLiteral("center_label")).toString().toStdString();
        plot.subtitle = plot_object.value(QStringLiteral("subtitle")).toString().toStdString();
        plot.show_grid = plot_object.value(QStringLiteral("show_grid")).toBool(true);
        plot.show_legend = plot_object.value(QStringLiteral("show_legend")).toBool(true);
        plot.line_width = plot_object.value(QStringLiteral("line_width")).toDouble(1.8);
        plot.legend_font_size = plot_object.value(QStringLiteral("legend_font_size")).toInt(8);
        plot.title_font_size = plot_object.value(QStringLiteral("title_font_size")).toInt(11);
        plot.axis_font_size = plot_object.value(QStringLiteral("axis_font_size")).toInt(9);
        plot.theme_preset =
            plot_object.value(QStringLiteral("theme_preset")).toString(QStringLiteral("default"))
                .toStdString();
        plot.y_min = read_optional(plot_object.value(QStringLiteral("y_min")));
        plot.y_max = read_optional(plot_object.value(QStringLiteral("y_max")));
        plot.x_min = read_optional(plot_object.value(QStringLiteral("x_min")));
        plot.x_max = read_optional(plot_object.value(QStringLiteral("x_max")));
        plot.data_region_fill = plot_object.value(QStringLiteral("data_region_fill"))
                                    .toString().toStdString();
        plot.grid_color = plot_object.value(QStringLiteral("grid_color"))
                              .toString(QStringLiteral("#e3e7eb")).toStdString();
        const auto read_series_style = [](const QJsonObject& source,
                                          domain::PlotSeriesStyle& style) {
            style.visible = source.value(QStringLiteral("visible")).toBool(style.visible);
            style.color = source.value(QStringLiteral("color"))
                              .toString(QString::fromStdString(style.color)).toStdString();
            style.fill_color = source.value(QStringLiteral("fill_color"))
                                   .toString(QString::fromStdString(style.fill_color))
                                   .toStdString();
            style.line_style = static_cast<domain::PlotLineStyle>(
                source.value(QStringLiteral("line_style"))
                    .toInt(static_cast<int>(style.line_style)));
            style.point_style = static_cast<domain::PlotPointStyle>(
                source.value(QStringLiteral("point_style"))
                    .toInt(static_cast<int>(style.point_style)));
            style.line_width = source.value(QStringLiteral("line_width"))
                                   .toDouble(style.line_width);
            style.point_size = source.value(QStringLiteral("point_size"))
                                   .toDouble(style.point_size);
            style.opacity = source.value(QStringLiteral("opacity")).toDouble(style.opacity);
        };
        const auto read_reference_style = [](const QJsonObject& source,
                                             domain::PlotReferenceStyle& style) {
            style.visible = source.value(QStringLiteral("visible")).toBool(style.visible);
            style.label = source.value(QStringLiteral("label"))
                              .toString(QString::fromStdString(style.label)).toStdString();
            style.color = source.value(QStringLiteral("color"))
                              .toString(QString::fromStdString(style.color)).toStdString();
            style.line_style = static_cast<domain::PlotLineStyle>(
                source.value(QStringLiteral("line_style"))
                    .toInt(static_cast<int>(style.line_style)));
            style.line_width = source.value(QStringLiteral("line_width"))
                                   .toDouble(style.line_width);
        };
        read_series_style(
            plot_object.value(QStringLiteral("value_style")).toObject(), plot.value_style);
        read_reference_style(
            plot_object.value(QStringLiteral("center_style")).toObject(), plot.center_style);
        read_reference_style(
            plot_object.value(QStringLiteral("lower_style")).toObject(), plot.lower_style);
        read_reference_style(
            plot_object.value(QStringLiteral("upper_style")).toObject(), plot.upper_style);
        plot.values = to_numbers(plot_object.value(QStringLiteral("values")).toArray());
        plot.x_values = to_numbers(plot_object.value(QStringLiteral("x_values")).toArray());
        plot.center = to_numbers(plot_object.value(QStringLiteral("center")).toArray());
        plot.lower = to_numbers(plot_object.value(QStringLiteral("lower")).toArray());
        plot.upper = to_numbers(plot_object.value(QStringLiteral("upper")).toArray());
        for (const QJsonValue& series_value :
             plot_object.value(QStringLiteral("series")).toArray()) {
            const QJsonObject series_object = series_value.toObject();
            domain::PlotSeries series;
            series.role = static_cast<domain::PlotSeriesRole>(
                series_object.value(QStringLiteral("role")).toInt());
            series.label = series_object.value(QStringLiteral("label")).toString().toStdString();
            series.values = to_numbers(series_object.value(QStringLiteral("values")).toArray());
            series.x_values = to_numbers(series_object.value(QStringLiteral("x_values")).toArray());
            series.lower = to_numbers(series_object.value(QStringLiteral("lower")).toArray());
            series.upper = to_numbers(series_object.value(QStringLiteral("upper")).toArray());
            series.style.line_width =
                series_object.value(QStringLiteral("line_width")).toDouble(1.8);
            series.line_width = series.style.line_width;
            series.show_points = series_object.value(QStringLiteral("show_points")).toBool(false);
            series.style.visible = series_object.value(QStringLiteral("visible")).toBool(true);
            series.style.color = series_object.value(QStringLiteral("color"))
                                     .toString(QStringLiteral("#455a64")).toStdString();
            series.style.fill_color = series_object.value(QStringLiteral("fill_color"))
                                          .toString().toStdString();
            series.style.line_style = static_cast<domain::PlotLineStyle>(
                series_object.value(QStringLiteral("line_style")).toInt(0));
            series.style.point_style = static_cast<domain::PlotPointStyle>(
                series_object.value(QStringLiteral("point_style")).toInt(
                    series.show_points ? 1 : 0));
            series.style.point_size = series_object.value(QStringLiteral("point_size"))
                                          .toDouble(3.5);
            series.style.opacity = series_object.value(QStringLiteral("opacity")).toDouble(1.0);
            plot.series.push_back(std::move(series));
        }
        plot.source_rows = to_sizes(plot_object.value(QStringLiteral("source_rows")).toArray());
        {
            const QJsonArray members =
                plot_object.value(QStringLiteral("member_source_rows")).toArray();
            plot.member_source_rows.reserve(static_cast<std::size_t>(members.size()));
            for (const QJsonValue& entry : members) {
                plot.member_source_rows.push_back(to_sizes(entry.toArray()));
            }
        }
        plot.sigma_z = plot_object.value(QStringLiteral("sigma_z")).toDouble();
        for (const QJsonValue& points : plot_object.value(
                 QStringLiteral("special_cause_points")).toArray()) {
            plot.special_cause_points.push_back(to_sizes(points.toArray()));
        }
        plot.triggered_tests = to_int_rows(
            plot_object.value(QStringLiteral("triggered_tests")).toArray());
        plot.primary_test_by_point = to_ints(
            plot_object.value(QStringLiteral("primary_test_by_point")).toArray());
        plot.signal_direction = to_ints(
            plot_object.value(QStringLiteral("signal_direction")).toArray());
        plot.histogram_edges = to_numbers(plot_object.value(QStringLiteral("histogram_edges")).toArray());
        plot.histogram_counts = to_numbers(plot_object.value(QStringLiteral("histogram_counts")).toArray());
        plot.lsl = read_optional(plot_object.value(QStringLiteral("lsl")));
        plot.usl = read_optional(plot_object.value(QStringLiteral("usl")));
        plot.target = read_optional(plot_object.value(QStringLiteral("target")));
        plot.process_mean = read_optional(plot_object.value(QStringLiteral("process_mean")));
        plot.within_sigma = read_optional(plot_object.value(QStringLiteral("within_sigma")));
        plot.overall_sigma = read_optional(plot_object.value(QStringLiteral("overall_sigma")));
        plot.categories = to_strings(plot_object.value(QStringLiteral("categories")).toArray());
        plot.category_values = to_numbers(plot_object.value(QStringLiteral("category_values")).toArray());
        plot.cumulative_percent = to_numbers(plot_object.value(QStringLiteral("cumulative_percent")).toArray());
        plot.box_min = to_numbers(plot_object.value(QStringLiteral("box_min")).toArray());
        plot.box_q1 = to_numbers(plot_object.value(QStringLiteral("box_q1")).toArray());
        plot.box_median = to_numbers(plot_object.value(QStringLiteral("box_median")).toArray());
        plot.box_q3 = to_numbers(plot_object.value(QStringLiteral("box_q3")).toArray());
        plot.box_max = to_numbers(plot_object.value(QStringLiteral("box_max")).toArray());
        plot.box_labels = to_strings(plot_object.value(QStringLiteral("box_labels")).toArray());
        plot.interval_lower =
            to_numbers(plot_object.value(QStringLiteral("interval_lower")).toArray());
        plot.interval_upper =
            to_numbers(plot_object.value(QStringLiteral("interval_upper")).toArray());
        plot.interval_counts =
            to_sizes(plot_object.value(QStringLiteral("interval_counts")).toArray());
        plot.point_labels =
            to_strings(plot_object.value(QStringLiteral("point_labels")).toArray());
        plot.point_groups =
            to_strings(plot_object.value(QStringLiteral("point_groups")).toArray());
        plot.bubble_sizes =
            to_numbers(plot_object.value(QStringLiteral("bubble_sizes")).toArray());
        plot.matrix_labels =
            to_strings(plot_object.value(QStringLiteral("matrix_labels")).toArray());
        for (const QJsonValue& row :
             plot_object.value(QStringLiteral("matrix_values")).toArray()) {
            plot.matrix_values.push_back(to_numbers(row.toArray()));
        }
        for (const QJsonValue& row :
             plot_object.value(QStringLiteral("matrix_counts")).toArray()) {
            plot.matrix_counts.push_back(to_sizes(row.toArray()));
        }
        for (const QJsonValue& row :
             plot_object.value(QStringLiteral("matrix_p_values")).toArray()) {
            plot.matrix_p_values.push_back(to_numbers(row.toArray()));
        }
        plot.histogram_edges_y =
            to_numbers(plot_object.value(QStringLiteral("histogram_edges_y")).toArray());
        plot.histogram_counts_y =
            to_numbers(plot_object.value(QStringLiteral("histogram_counts_y")).toArray());
        plot.contour_x = to_numbers(plot_object.value(QStringLiteral("contour_x")).toArray());
        plot.contour_y = to_numbers(plot_object.value(QStringLiteral("contour_y")).toArray());
        plot.contour_levels =
            to_numbers(plot_object.value(QStringLiteral("contour_levels")).toArray());
        plot.color_min = read_optional(plot_object.value(QStringLiteral("color_min")));
        plot.color_max = read_optional(plot_object.value(QStringLiteral("color_max")));
        page.plots.push_back(plot);
    }
    for (const QJsonValue& value : object.value(QStringLiteral("diagnostics")).toArray()) {
        const QJsonObject item = value.toObject();
        domain::DiagnosticMessage diagnostic;
        diagnostic.severity = static_cast<domain::DiagnosticMessage::Severity>(
            item.value(QStringLiteral("severity")).toInt());
        diagnostic.code = item.value(QStringLiteral("code")).toString().toStdString();
        diagnostic.message = item.value(QStringLiteral("message")).toString().toStdString();
        diagnostic.related_rows = to_rows(item.value(QStringLiteral("related_rows")).toArray());
        diagnostic.related_columns =
            to_sizes(item.value(QStringLiteral("related_columns")).toArray());
        diagnostic.related_plot_id =
            item.value(QStringLiteral("related_plot_id")).toString().toStdString();
        diagnostic.suggested_action =
            item.value(QStringLiteral("suggested_action")).toString().toStdString();
        page.diagnostics.push_back(diagnostic);
    }
    for (const QJsonValue& value : object.value(QStringLiteral("interpretation")).toArray()) {
        const QJsonObject item = value.toObject();
        domain::InterpretationSection section;
        section.heading = item.value(QStringLiteral("heading")).toString().toStdString();
        section.bullets = to_strings(item.value(QStringLiteral("bullets")).toArray());
        section.severity = static_cast<domain::DiagnosticMessage::Severity>(
            item.value(QStringLiteral("severity")).toInt());
        page.interpretation.push_back(section);
    }
    if (object.contains(QStringLiteral("worksheet_export"))
        && object.value(QStringLiteral("worksheet_export")).isObject()) {
        const QJsonObject export_object =
            object.value(QStringLiteral("worksheet_export")).toObject();
        domain::DataTable export_table;
        export_table.name =
            export_object.value(QStringLiteral("name")).toString().toStdString();
        export_table.source_path =
            export_object.value(QStringLiteral("source_path")).toString().toStdString();
        export_table.columns =
            to_strings(export_object.value(QStringLiteral("columns")).toArray());
        for (const QJsonValue& row_value :
             export_object.value(QStringLiteral("rows")).toArray()) {
            export_table.rows.push_back(to_strings(row_value.toArray()));
        }
        page.worksheet_export = std::move(export_table);
    }
    read_interpretation_facts(object, page.facts);
    for (const QJsonValue& value : object.value(QStringLiteral("computation_traces")).toArray()) {
        const QJsonObject item = value.toObject();
        domain::ComputationTrace trace;
        trace.formula_id = item.value(QStringLiteral("formula_id")).toString().toStdString();
        trace.title = item.value(QStringLiteral("title")).toString().toStdString();
        trace.plain_formula =
            item.value(QStringLiteral("plain_formula")).toString().toStdString();
        trace.substituted_text =
            item.value(QStringLiteral("substituted_text")).toString().toStdString();
        trace.result_symbol =
            item.value(QStringLiteral("result_symbol")).toString().toStdString();
        trace.result_value =
            item.value(QStringLiteral("result_value")).toString().toStdString();
        trace.evidence_type =
            item.value(QStringLiteral("evidence_type")).toString().toStdString();
        if (trace.evidence_type.empty()) {
            trace.evidence_type = "formula_reference";
        }
        trace.primary_url =
            item.value(QStringLiteral("primary_url")).toString().toStdString();
        trace.command_id =
            item.value(QStringLiteral("command_id")).toString().toStdString();
        for (const QJsonValue& binding_value : item.value(QStringLiteral("bindings")).toArray()) {
            const QJsonObject b = binding_value.toObject();
            domain::FormulaBinding binding;
            binding.symbol = b.value(QStringLiteral("symbol")).toString().toStdString();
            binding.label = b.value(QStringLiteral("label")).toString().toStdString();
            binding.value = b.value(QStringLiteral("value")).toString().toStdString();
            binding.role = b.value(QStringLiteral("role")).toString().toStdString();
            trace.bindings.push_back(std::move(binding));
        }
        for (const QJsonValue& step_value : item.value(QStringLiteral("steps")).toArray()) {
            const QJsonObject s = step_value.toObject();
            domain::ComputationStep step;
            step.order = s.value(QStringLiteral("order")).toInt(0);
            step.description =
                s.value(QStringLiteral("description")).toString().toStdString();
            step.expression_before =
                s.value(QStringLiteral("expression_before")).toString().toStdString();
            step.expression_after =
                s.value(QStringLiteral("expression_after")).toString().toStdString();
            step.value = s.value(QStringLiteral("value")).toString().toStdString();
            trace.steps.push_back(std::move(step));
        }
        page.computation_traces.push_back(std::move(trace));
    }
    return page;
}

}  // namespace datalab::infrastructure
