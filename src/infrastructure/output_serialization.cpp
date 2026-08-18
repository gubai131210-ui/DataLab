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
        array.append(object);
    }
    return array;
}

std::vector<domain::RuleEvidence> read_rules(const QJsonArray& array)
{
    std::vector<domain::RuleEvidence> rules;
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        rules.push_back({
            object.value(QStringLiteral("id")).toString().toStdString(),
            object.value(QStringLiteral("status")).toString("not_applicable").toStdString(),
            object.value(QStringLiteral("message")).toString().toStdString(),
            to_rows(object.value(QStringLiteral("related_rows")).toArray()),
            object.value(QStringLiteral("suggested_action")).toString().toStdString()});
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
        serialized.insert(QStringLiteral("capability"), capability);
    }
    if (facts.spc.has_value()) {
        QJsonObject spc;
        spc.insert(QStringLiteral("out_of_control_count"),
                   optional_size(facts.spc->out_of_control_count));
        spc.insert(QStringLiteral("sigma_z"), optional_number(facts.spc->sigma_z));
        serialized.insert(QStringLiteral("spc"), spc);
    }
    if (facts.regression.has_value()) {
        QJsonObject regression;
        regression.insert(QStringLiteral("r_squared"),
                          optional_number(facts.regression->r_squared));
        regression.insert(QStringLiteral("residual_normality_p"),
                          optional_number(facts.regression->residual_normality_p));
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
        msa.insert(QStringLiteral("ratings_are_ordinal"), facts.msa->ratings_are_ordinal);
        msa.insert(QStringLiteral("kendall_available"), facts.msa->kendall_available);
        msa.insert(QStringLiteral("kendall_w"), optional_number(facts.msa->kendall_w));
        msa.insert(QStringLiteral("kendall_w_p"), optional_number(facts.msa->kendall_w_p));
        msa.insert(QStringLiteral("kendall_tau"), optional_number(facts.msa->kendall_tau));
        msa.insert(QStringLiteral("kendall_tau_p"), optional_number(facts.msa->kendall_tau_p));
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
        serialized.insert(QStringLiteral("reliability"), reliability);
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
        serialized.insert(QStringLiteral("chi_square"), chi_square);
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
        pca.insert(QStringLiteral("converged"), facts.pca->converged);
        serialized.insert(QStringLiteral("pca"), pca);
    }
    if (facts.variance.has_value()) {
        QJsonObject variance;
        variance.insert(QStringLiteral("method"),
                        QString::fromStdString(facts.variance->method));
        variance.insert(QStringLiteral("statistic"),
                        optional_number(facts.variance->statistic));
        variance.insert(QStringLiteral("p_value"),
                        optional_number(facts.variance->p_value));
        variance.insert(QStringLiteral("group_count"),
                        static_cast<int>(facts.variance->group_count));
        serialized.insert(QStringLiteral("variance"), variance);
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
        facts.capability = domain::CapabilityFacts{
            read_optional(capability.value(QStringLiteral("cpk"))),
            read_optional(capability.value(QStringLiteral("ppk"))),
            read_optional(capability.value(QStringLiteral("cpm"))),
            read_optional(capability.value(QStringLiteral("z_bench"))),
            capability.value(QStringLiteral("assumption_status"))
                .toString("not_verified").toStdString(),
            capability.value(QStringLiteral("specification_mode")).toString().toStdString(),
            capability.value(QStringLiteral("method")).toString().toStdString(),
            capability.value(QStringLiteral("johnson_family")).toString().toStdString()};
    }
    const QJsonObject spc = serialized.value(QStringLiteral("spc")).toObject();
    if (!spc.isEmpty()) {
        facts.spc = domain::SpcFacts{
            read_optional_size(spc.value(QStringLiteral("out_of_control_count"))),
            read_optional(spc.value(QStringLiteral("sigma_z")))};
    }
    const QJsonObject regression = serialized.value(QStringLiteral("regression")).toObject();
    if (!regression.isEmpty()) {
        domain::RegressionFacts value;
        value.r_squared = read_optional(regression.value(QStringLiteral("r_squared")));
        value.residual_normality_p =
            read_optional(regression.value(QStringLiteral("residual_normality_p")));
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
        value.ratings_are_ordinal =
            msa.value(QStringLiteral("ratings_are_ordinal")).toBool(false);
        value.kendall_available =
            msa.value(QStringLiteral("kendall_available")).toBool(false);
        value.kendall_w = read_optional(msa.value(QStringLiteral("kendall_w")));
        value.kendall_w_p = read_optional(msa.value(QStringLiteral("kendall_w_p")));
        value.kendall_tau = read_optional(msa.value(QStringLiteral("kendall_tau")));
        value.kendall_tau_p = read_optional(msa.value(QStringLiteral("kendall_tau_p")));
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
        facts.reliability = std::move(value);
    }
    const QJsonObject forecast = serialized.value(QStringLiteral("forecast")).toObject();
    if (!forecast.isEmpty()) {
        facts.forecast = domain::ForecastFacts{
            read_optional(forecast.value(QStringLiteral("mape"))),
            read_optional(forecast.value(QStringLiteral("mase"))),
            read_optional(forecast.value(QStringLiteral("rolling_origin_mape"))),
            read_optional(forecast.value(QStringLiteral("rolling_origin_mase")))};
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
        facts.chi_square = std::move(value);
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
        value.converged = pca.value(QStringLiteral("converged")).toBool(false);
        facts.pca = std::move(value);
    }
    const QJsonObject variance = serialized.value(QStringLiteral("variance")).toObject();
    if (!variance.isEmpty()) {
        domain::VarianceFacts value;
        value.method = variance.value(QStringLiteral("method")).toString().toStdString();
        value.statistic = read_optional(variance.value(QStringLiteral("statistic")));
        value.p_value = read_optional(variance.value(QStringLiteral("p_value")));
        value.group_count = static_cast<std::size_t>(
            variance.value(QStringLiteral("group_count")).toInt(0));
        facts.variance = std::move(value);
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
    object.insert(QStringLiteral("lower_spec"), optional_number(page.configuration.specifications.lower));
    object.insert(QStringLiteral("upper_spec"), optional_number(page.configuration.specifications.upper));
    object.insert(QStringLiteral("target_spec"), optional_number(page.configuration.specifications.target));
    object.insert(QStringLiteral("by_column"), optional_size(page.configuration.by_column));
    object.insert(QStringLiteral("moving_range_length"), page.configuration.control.moving_range_length);
    object.insert(QStringLiteral("sigma_method"), QString::fromStdString(page.configuration.control.sigma_method));
    object.insert(QStringLiteral("historical_center"),
                  optional_number(page.configuration.control.historical_center));
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
    object.insert(QStringLiteral("hypothesis_mean"),
                  optional_number(page.configuration.inference.hypothesis_mean));
    object.insert(QStringLiteral("confidence_level"), page.configuration.inference.confidence_level);
    object.insert(QStringLiteral("alternative"),
                  QString::fromStdString(page.configuration.inference.alternative));
    object.insert(QStringLiteral("correlation_method"),
                  QString::fromStdString(page.configuration.inference.correlation_method));
    object.insert(QStringLiteral("variance_method"),
                  QString::fromStdString(page.configuration.inference.variance_method));
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
    object.insert(QStringLiteral("doe_low_levels"),
                  string_array(page.configuration.doe.low_levels));
    object.insert(QStringLiteral("doe_high_levels"),
                  string_array(page.configuration.doe.high_levels));
    object.insert(QStringLiteral("doe_center_point_count"),
                  static_cast<qint64>(page.configuration.doe.center_point_count));
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
    object.insert(QStringLiteral("msa_mode"),
                  QString::fromStdString(page.configuration.msa.mode));
    object.insert(QStringLiteral("reliability_time_column"),
                  optional_size(page.configuration.reliability.time_column));
    object.insert(QStringLiteral("reliability_event_column"),
                  optional_size(page.configuration.reliability.event_column));
    object.insert(QStringLiteral("reliability_group_column"),
                  optional_size(page.configuration.reliability.group_column));
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
    object.insert(QStringLiteral("power_variance_method"),
                  QString::fromStdString(page.configuration.power.variance_method));
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
    return object;
}

domain::OutputPage output_page_from_json(const QJsonObject& object)
{
    domain::OutputPage page;
    page.id = object.value(QStringLiteral("id")).toString().toStdString();
    page.title = object.value(QStringLiteral("title")).toString().toStdString();
    page.method_name = object.value(QStringLiteral("method_name")).toString().toStdString();
    page.parameter_summary = object.value(QStringLiteral("parameter_summary")).toString().toStdString();
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
    page.configuration.control.historical_center =
        read_optional(object.value(QStringLiteral("historical_center")));
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
    page.configuration.inference.hypothesis_mean =
        read_optional(object.value(QStringLiteral("hypothesis_mean")));
    page.configuration.inference.confidence_level =
        object.value(QStringLiteral("confidence_level")).toDouble(0.95);
    page.configuration.inference.alternative =
        object.value(QStringLiteral("alternative")).toString("two_sided").toStdString();
    page.configuration.inference.correlation_method =
        object.value(QStringLiteral("correlation_method")).toString("pearson").toStdString();
    page.configuration.inference.variance_method =
        object.value(QStringLiteral("variance_method")).toString("welch").toStdString();
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
    page.configuration.doe.low_levels =
        to_strings(object.value(QStringLiteral("doe_low_levels")).toArray());
    page.configuration.doe.high_levels =
        to_strings(object.value(QStringLiteral("doe_high_levels")).toArray());
    page.configuration.doe.center_point_count =
        static_cast<std::size_t>(object.value(QStringLiteral("doe_center_point_count")).toInteger());
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
    page.configuration.msa.mode =
        object.value(QStringLiteral("msa_mode")).toString(QStringLiteral("type1")).toStdString();
    page.configuration.reliability.time_column =
        read_optional_size(object.value(QStringLiteral("reliability_time_column")));
    page.configuration.reliability.event_column =
        read_optional_size(object.value(QStringLiteral("reliability_event_column")));
    page.configuration.reliability.group_column =
        read_optional_size(object.value(QStringLiteral("reliability_group_column")));
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
    page.configuration.power.variance_method =
        object.value(QStringLiteral("power_variance_method"))
            .toString(QStringLiteral("pooled")).toStdString();
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
    read_interpretation_facts(object, page.facts);
    return page;
}

}  // namespace datalab::infrastructure
