#include "infrastructure/report_serialization.h"

#include "infrastructure/output_serialization.h"

#include <QJsonArray>

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

std::vector<std::string> to_strings(const QJsonArray& array)
{
    std::vector<std::string> values;
    values.reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue& value : array) {
        values.push_back(value.toString().toStdString());
    }
    return values;
}

QJsonArray row_array(const std::vector<domain::RowId>& rows)
{
    QJsonArray array;
    for (domain::RowId row : rows) {
        array.append(static_cast<qint64>(row));
    }
    return array;
}

std::vector<domain::RowId> to_rows(const QJsonArray& array)
{
    std::vector<domain::RowId> rows;
    rows.reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue& value : array) {
        rows.push_back(static_cast<domain::RowId>(value.toVariant().toULongLong()));
    }
    return rows;
}

QJsonObject write_rule(const domain::RuleEvidence& rule)
{
    QJsonObject object;
    object.insert(QStringLiteral("id"), QString::fromStdString(rule.id));
    object.insert(QStringLiteral("status"), QString::fromStdString(rule.status));
    object.insert(QStringLiteral("message"), QString::fromStdString(rule.message));
    object.insert(QStringLiteral("related_rows"), row_array(rule.related_rows));
    object.insert(QStringLiteral("suggested_action"), QString::fromStdString(rule.suggested_action));
    object.insert(QStringLiteral("name"), QString::fromStdString(rule.name));
    object.insert(QStringLiteral("window"), QString::fromStdString(rule.window));
    object.insert(QStringLiteral("threshold"), QString::fromStdString(rule.threshold));
    object.insert(
        QStringLiteral("comparison_direction"),
        QString::fromStdString(rule.comparison_direction));
    QJsonArray plotted;
    for (std::size_t point : rule.plotted_points) {
        plotted.append(static_cast<int>(point));
    }
    object.insert(QStringLiteral("plotted_points"), plotted);
    object.insert(
        QStringLiteral("not_applicable_reason"),
        QString::fromStdString(rule.not_applicable_reason));
    object.insert(
        QStringLiteral("not_verified_reason"),
        QString::fromStdString(rule.not_verified_reason));
    object.insert(
        QStringLiteral("calculation_failed_reason"),
        QString::fromStdString(rule.calculation_failed_reason));
    return object;
}

domain::RuleEvidence read_rule(const QJsonObject& object)
{
    domain::RuleEvidence rule;
    rule.id = object.value(QStringLiteral("id")).toString().toStdString();
    rule.status =
        object.value(QStringLiteral("status")).toString("not_applicable").toStdString();
    rule.message = object.value(QStringLiteral("message")).toString().toStdString();
    rule.related_rows = to_rows(object.value(QStringLiteral("related_rows")).toArray());
    rule.suggested_action =
        object.value(QStringLiteral("suggested_action")).toString().toStdString();
    rule.name = object.value(QStringLiteral("name")).toString().toStdString();
    rule.window = object.value(QStringLiteral("window")).toString().toStdString();
    rule.threshold = object.value(QStringLiteral("threshold")).toString().toStdString();
    rule.comparison_direction =
        object.value(QStringLiteral("comparison_direction")).toString().toStdString();
    for (const QJsonValue& point : object.value(QStringLiteral("plotted_points")).toArray()) {
        rule.plotted_points.push_back(static_cast<std::size_t>(point.toInt()));
    }
    rule.not_applicable_reason =
        object.value(QStringLiteral("not_applicable_reason")).toString().toStdString();
    rule.not_verified_reason =
        object.value(QStringLiteral("not_verified_reason")).toString().toStdString();
    rule.calculation_failed_reason =
        object.value(QStringLiteral("calculation_failed_reason")).toString().toStdString();
    return rule;
}

QJsonObject write_locale(const domain::ReportLocaleSettings& locale)
{
    QJsonObject object;
    object.insert(QStringLiteral("language_tag"), QString::fromStdString(locale.language_tag));
    object.insert(QStringLiteral("timezone_id"), QString::fromStdString(locale.timezone_id));
    object.insert(
        QStringLiteral("number_format_locale"),
        QString::fromStdString(locale.number_format_locale));
    object.insert(
        QStringLiteral("date_format_locale"),
        QString::fromStdString(locale.date_format_locale));
    object.insert(QStringLiteral("use_percent_sign_localized"), locale.use_percent_sign_localized);
    return object;
}

domain::ReportLocaleSettings read_locale(const QJsonObject& object)
{
    domain::ReportLocaleSettings locale;
    locale.language_tag =
        object.value(QStringLiteral("language_tag")).toString("zh-CN").toStdString();
    locale.timezone_id =
        object.value(QStringLiteral("timezone_id")).toString("Asia/Shanghai").toStdString();
    locale.number_format_locale =
        object.value(QStringLiteral("number_format_locale")).toString("zh-CN").toStdString();
    locale.date_format_locale =
        object.value(QStringLiteral("date_format_locale")).toString("zh-CN").toStdString();
    locale.use_percent_sign_localized =
        object.value(QStringLiteral("use_percent_sign_localized")).toBool(true);
    return locale;
}

QJsonObject write_provenance(const domain::ReportProvenance& provenance)
{
    QJsonObject object;
    object.insert(QStringLiteral("report_id"), QString::fromStdString(provenance.report_id));
    object.insert(
        QStringLiteral("source_dataset_id"),
        QString::fromStdString(provenance.source_dataset_id));
    object.insert(QStringLiteral("source_path"), QString::fromStdString(provenance.source_path));
    object.insert(QStringLiteral("source_kind"), QString::fromStdString(provenance.source_kind));
    object.insert(
        QStringLiteral("import_plan_summary"),
        QString::fromStdString(provenance.import_plan_summary));
    object.insert(
        QStringLiteral("filter_summary"), QString::fromStdString(provenance.filter_summary));
    object.insert(QStringLiteral("row_count_n"), static_cast<int>(provenance.row_count_n));
    object.insert(QStringLiteral("column_count"), static_cast<int>(provenance.column_count));
    object.insert(
        QStringLiteral("excluded_row_count"), static_cast<int>(provenance.excluded_row_count));
    object.insert(
        QStringLiteral("hidden_row_count"), static_cast<int>(provenance.hidden_row_count));
    object.insert(
        QStringLiteral("algorithm_version"),
        QString::fromStdString(provenance.algorithm_version));
    object.insert(
        QStringLiteral("software_version"),
        QString::fromStdString(provenance.software_version));
    object.insert(
        QStringLiteral("generated_at_utc"),
        QString::fromStdString(provenance.generated_at_utc));
    object.insert(
        QStringLiteral("input_snapshot_hash"),
        QString::fromStdString(provenance.input_snapshot_hash));
    object.insert(QStringLiteral("facts_hash"), QString::fromStdString(provenance.facts_hash));
    object.insert(
        QStringLiteral("configuration_hash"),
        QString::fromStdString(provenance.configuration_hash));
    return object;
}

domain::ReportProvenance read_provenance(const QJsonObject& object)
{
    domain::ReportProvenance provenance;
    provenance.report_id = object.value(QStringLiteral("report_id")).toString().toStdString();
    provenance.source_dataset_id =
        object.value(QStringLiteral("source_dataset_id")).toString().toStdString();
    provenance.source_path = object.value(QStringLiteral("source_path")).toString().toStdString();
    provenance.source_kind =
        object.value(QStringLiteral("source_kind")).toString("worksheet_snapshot").toStdString();
    provenance.import_plan_summary =
        object.value(QStringLiteral("import_plan_summary")).toString().toStdString();
    provenance.filter_summary =
        object.value(QStringLiteral("filter_summary")).toString().toStdString();
    provenance.row_count_n =
        static_cast<std::size_t>(object.value(QStringLiteral("row_count_n")).toInt(0));
    provenance.column_count =
        static_cast<std::size_t>(object.value(QStringLiteral("column_count")).toInt(0));
    provenance.excluded_row_count =
        static_cast<std::size_t>(object.value(QStringLiteral("excluded_row_count")).toInt(0));
    provenance.hidden_row_count =
        static_cast<std::size_t>(object.value(QStringLiteral("hidden_row_count")).toInt(0));
    provenance.algorithm_version =
        object.value(QStringLiteral("algorithm_version")).toString().toStdString();
    provenance.software_version =
        object.value(QStringLiteral("software_version")).toString().toStdString();
    provenance.generated_at_utc =
        object.value(QStringLiteral("generated_at_utc")).toString().toStdString();
    provenance.input_snapshot_hash =
        object.value(QStringLiteral("input_snapshot_hash")).toString().toStdString();
    provenance.facts_hash = object.value(QStringLiteral("facts_hash")).toString().toStdString();
    provenance.configuration_hash =
        object.value(QStringLiteral("configuration_hash")).toString().toStdString();
    return provenance;
}

QJsonObject write_evidence_ref(const domain::EvidenceRef& ref)
{
    QJsonObject object;
    object.insert(QStringLiteral("evidence_id"), QString::fromStdString(ref.evidence_id));
    object.insert(QStringLiteral("kind"), QString::fromStdString(domain::evidence_kind_id(ref.kind)));
    object.insert(QStringLiteral("role"), QString::fromStdString(domain::evidence_role_id(ref.role)));
    object.insert(QStringLiteral("label_text_id"), QString::fromStdString(ref.label_text_id));
    object.insert(
        QStringLiteral("source_dataset_id"), QString::fromStdString(ref.source_dataset_id));
    object.insert(QStringLiteral("source_rows"), row_array(ref.source_rows));
    object.insert(QStringLiteral("parameter_keys"), string_array(ref.parameter_keys));
    object.insert(QStringLiteral("formula_ref_id"), QString::fromStdString(ref.formula_ref_id));
    object.insert(
        QStringLiteral("reference_impl_id"), QString::fromStdString(ref.reference_impl_id));
    object.insert(QStringLiteral("vendor_oracle_id"), QString::fromStdString(ref.vendor_oracle_id));
    object.insert(QStringLiteral("golden_id"), QString::fromStdString(ref.golden_id));
    object.insert(QStringLiteral("diagnostic_code"), QString::fromStdString(ref.diagnostic_code));
    object.insert(QStringLiteral("rule_id"), QString::fromStdString(ref.rule_id));
    object.insert(QStringLiteral("status"), QString::fromStdString(ref.status));
    object.insert(QStringLiteral("notes_text_id"), QString::fromStdString(ref.notes_text_id));
    return object;
}

domain::EvidenceRef read_evidence_ref(const QJsonObject& object)
{
    domain::EvidenceRef ref;
    ref.evidence_id = object.value(QStringLiteral("evidence_id")).toString().toStdString();
    ref.kind = domain::evidence_kind_from_id(
        object.value(QStringLiteral("kind")).toString("other").toStdString());
    ref.role = domain::evidence_role_from_id(
        object.value(QStringLiteral("role")).toString("supporting").toStdString());
    ref.label_text_id = object.value(QStringLiteral("label_text_id")).toString().toStdString();
    ref.source_dataset_id =
        object.value(QStringLiteral("source_dataset_id")).toString().toStdString();
    ref.source_rows = to_rows(object.value(QStringLiteral("source_rows")).toArray());
    ref.parameter_keys = to_strings(object.value(QStringLiteral("parameter_keys")).toArray());
    ref.formula_ref_id = object.value(QStringLiteral("formula_ref_id")).toString().toStdString();
    ref.reference_impl_id =
        object.value(QStringLiteral("reference_impl_id")).toString().toStdString();
    ref.vendor_oracle_id =
        object.value(QStringLiteral("vendor_oracle_id")).toString().toStdString();
    ref.golden_id = object.value(QStringLiteral("golden_id")).toString().toStdString();
    ref.diagnostic_code = object.value(QStringLiteral("diagnostic_code")).toString().toStdString();
    ref.rule_id = object.value(QStringLiteral("rule_id")).toString().toStdString();
    ref.status = object.value(QStringLiteral("status")).toString("present").toStdString();
    ref.notes_text_id = object.value(QStringLiteral("notes_text_id")).toString().toStdString();
    return ref;
}

QJsonObject write_method_metadata(const domain::MethodMetadata& metadata)
{
    QJsonObject object;
    object.insert(QStringLiteral("algorithm"), QString::fromStdString(metadata.algorithm));
    object.insert(QStringLiteral("version"), QString::fromStdString(metadata.version));
    object.insert(QStringLiteral("parameters"), QString::fromStdString(metadata.parameters));
    object.insert(QStringLiteral("missing_policy"), QString::fromStdString(metadata.missing_policy));
    object.insert(
        QStringLiteral("estimation_method"), QString::fromStdString(metadata.estimation_method));
    object.insert(QStringLiteral("source_rows"), row_array(metadata.source_rows));
    object.insert(QStringLiteral("diagnostic_codes"), string_array(metadata.diagnostic_codes));
    object.insert(
        QStringLiteral("assumption_status"), QString::fromStdString(metadata.assumption_status));
    object.insert(
        QStringLiteral("parameter_source"), QString::fromStdString(metadata.parameter_source));
    object.insert(QStringLiteral("valid_count"), static_cast<int>(metadata.valid_count));
    object.insert(QStringLiteral("missing_count"), static_cast<int>(metadata.missing_count));
    object.insert(
        QStringLiteral("not_computed_reason"),
        QString::fromStdString(metadata.not_computed_reason));
    return object;
}

domain::MethodMetadata read_method_metadata(const QJsonObject& object)
{
    domain::MethodMetadata metadata;
    metadata.algorithm = object.value(QStringLiteral("algorithm")).toString().toStdString();
    metadata.version = object.value(QStringLiteral("version")).toString("1").toStdString();
    metadata.parameters = object.value(QStringLiteral("parameters")).toString().toStdString();
    metadata.missing_policy =
        object.value(QStringLiteral("missing_policy")).toString("skip_missing").toStdString();
    metadata.estimation_method =
        object.value(QStringLiteral("estimation_method")).toString().toStdString();
    metadata.source_rows = to_rows(object.value(QStringLiteral("source_rows")).toArray());
    metadata.diagnostic_codes =
        to_strings(object.value(QStringLiteral("diagnostic_codes")).toArray());
    metadata.assumption_status =
        object.value(QStringLiteral("assumption_status")).toString("not_verified").toStdString();
    metadata.parameter_source =
        object.value(QStringLiteral("parameter_source")).toString().toStdString();
    metadata.valid_count =
        static_cast<std::size_t>(object.value(QStringLiteral("valid_count")).toInt(0));
    metadata.missing_count =
        static_cast<std::size_t>(object.value(QStringLiteral("missing_count")).toInt(0));
    metadata.not_computed_reason =
        object.value(QStringLiteral("not_computed_reason")).toString().toStdString();
    return metadata;
}

}  // namespace

QJsonObject report_profile_to_json(const domain::ReportProfile& profile)
{
    QJsonObject object;
    object.insert(QStringLiteral("schema_version"), profile.schema_version);
    object.insert(QStringLiteral("profile_id"), QString::fromStdString(profile.profile_id));
    object.insert(
        QStringLiteral("template_kind"),
        QString::fromStdString(domain::report_template_kind_id(profile.template_kind)));
    object.insert(
        QStringLiteral("template_version"), QString::fromStdString(profile.template_version));
    object.insert(QStringLiteral("locale"), write_locale(profile.locale));
    object.insert(QStringLiteral("include_executive_summary"), profile.include_executive_summary);
    object.insert(
        QStringLiteral("include_key_risks_and_limits"), profile.include_key_risks_and_limits);
    object.insert(QStringLiteral("include_plots"), profile.include_plots);
    object.insert(QStringLiteral("include_statistic_tables"), profile.include_statistic_tables);
    object.insert(QStringLiteral("include_parameters"), profile.include_parameters);
    object.insert(QStringLiteral("include_rule_evidence"), profile.include_rule_evidence);
    object.insert(QStringLiteral("include_diagnostics"), profile.include_diagnostics);
    object.insert(QStringLiteral("include_anomaly_rows"), profile.include_anomaly_rows);
    object.insert(QStringLiteral("include_input_snapshot"), profile.include_input_snapshot);
    object.insert(QStringLiteral("include_import_plan"), profile.include_import_plan);
    object.insert(QStringLiteral("include_filter_detail"), profile.include_filter_detail);
    object.insert(QStringLiteral("include_algorithm_versions"), profile.include_algorithm_versions);
    object.insert(QStringLiteral("include_source_hashes"), profile.include_source_hashes);
    object.insert(QStringLiteral("include_formula_references"), profile.include_formula_references);
    object.insert(
        QStringLiteral("include_full_evidence_appendix"), profile.include_full_evidence_appendix);
    object.insert(QStringLiteral("max_preview_rows"), static_cast<int>(profile.max_preview_rows));
    object.insert(QStringLiteral("max_evidence_rows"), static_cast<int>(profile.max_evidence_rows));
    object.insert(QStringLiteral("max_plots"), static_cast<int>(profile.max_plots));
    return object;
}

domain::ReportProfile report_profile_from_json(const QJsonObject& object)
{
    domain::ReportProfile profile = domain::make_report_profile(
        domain::report_template_kind_from_id(
            object.value(QStringLiteral("template_kind")).toString("engineer").toStdString()));
    profile.schema_version = object.value(QStringLiteral("schema_version")).toInt(1);
    profile.profile_id = object.value(QStringLiteral("profile_id")).toString().toStdString();
    if (profile.profile_id.empty()) {
        profile.profile_id = domain::report_template_kind_id(profile.template_kind);
    }
    profile.template_version =
        object.value(QStringLiteral("template_version")).toString("1").toStdString();
    if (object.contains(QStringLiteral("locale"))) {
        profile.locale = read_locale(object.value(QStringLiteral("locale")).toObject());
    }
    auto read_flag = [&](const char* key, bool fallback) {
        return object.value(QString::fromUtf8(key)).toBool(fallback);
    };
    profile.include_executive_summary =
        read_flag("include_executive_summary", profile.include_executive_summary);
    profile.include_key_risks_and_limits =
        read_flag("include_key_risks_and_limits", profile.include_key_risks_and_limits);
    profile.include_plots = read_flag("include_plots", profile.include_plots);
    profile.include_statistic_tables =
        read_flag("include_statistic_tables", profile.include_statistic_tables);
    profile.include_parameters = read_flag("include_parameters", profile.include_parameters);
    profile.include_rule_evidence =
        read_flag("include_rule_evidence", profile.include_rule_evidence);
    profile.include_diagnostics = read_flag("include_diagnostics", profile.include_diagnostics);
    profile.include_anomaly_rows = read_flag("include_anomaly_rows", profile.include_anomaly_rows);
    profile.include_input_snapshot =
        read_flag("include_input_snapshot", profile.include_input_snapshot);
    profile.include_import_plan = read_flag("include_import_plan", profile.include_import_plan);
    profile.include_filter_detail =
        read_flag("include_filter_detail", profile.include_filter_detail);
    profile.include_algorithm_versions =
        read_flag("include_algorithm_versions", profile.include_algorithm_versions);
    profile.include_source_hashes =
        read_flag("include_source_hashes", profile.include_source_hashes);
    profile.include_formula_references =
        read_flag("include_formula_references", profile.include_formula_references);
    profile.include_full_evidence_appendix =
        read_flag("include_full_evidence_appendix", profile.include_full_evidence_appendix);
    profile.max_preview_rows = static_cast<std::size_t>(
        object.value(QStringLiteral("max_preview_rows"))
            .toInt(static_cast<int>(profile.max_preview_rows)));
    profile.max_evidence_rows = static_cast<std::size_t>(
        object.value(QStringLiteral("max_evidence_rows"))
            .toInt(static_cast<int>(profile.max_evidence_rows)));
    profile.max_plots = static_cast<std::size_t>(
        object.value(QStringLiteral("max_plots")).toInt(static_cast<int>(profile.max_plots)));
    return profile;
}

QJsonObject evidence_bundle_to_json(const domain::EvidenceBundle& bundle)
{
    QJsonObject object;
    object.insert(QStringLiteral("schema_version"), bundle.schema_version);
    QJsonArray evidence;
    for (const domain::EvidenceRef& ref : bundle.evidence) {
        evidence.append(write_evidence_ref(ref));
    }
    object.insert(QStringLiteral("evidence"), evidence);
    QJsonArray rules;
    for (const domain::RuleEvidence& rule : bundle.rules) {
        rules.append(write_rule(rule));
    }
    object.insert(QStringLiteral("rules"), rules);
    object.insert(QStringLiteral("method_metadata"), write_method_metadata(bundle.method_metadata));
    QJsonObject quality;
    quality.insert(
        QStringLiteral("method_version"),
        QString::fromStdString(bundle.quality_evidence.method_version));
    quality.insert(QStringLiteral("valid_count"), static_cast<int>(bundle.quality_evidence.valid_count));
    quality.insert(
        QStringLiteral("missing_count"), static_cast<int>(bundle.quality_evidence.missing_count));
    quality.insert(
        QStringLiteral("omitted_count"), static_cast<int>(bundle.quality_evidence.omitted_count));
    quality.insert(
        QStringLiteral("source_rows"), row_array(bundle.quality_evidence.source_rows));
    quality.insert(
        QStringLiteral("parameter_source"),
        QString::fromStdString(bundle.quality_evidence.parameter_source));
    quality.insert(
        QStringLiteral("assumption_status"),
        QString::fromStdString(bundle.quality_evidence.assumption_status));
    quality.insert(
        QStringLiteral("not_computed_reason"),
        QString::fromStdString(bundle.quality_evidence.not_computed_reason));
    object.insert(QStringLiteral("quality_evidence"), quality);
    object.insert(QStringLiteral("provenance"), write_provenance(bundle.provenance));
    return object;
}

domain::EvidenceBundle evidence_bundle_from_json(const QJsonObject& object)
{
    domain::EvidenceBundle bundle;
    bundle.schema_version = object.value(QStringLiteral("schema_version")).toInt(1);
    for (const QJsonValue& value : object.value(QStringLiteral("evidence")).toArray()) {
        bundle.evidence.push_back(read_evidence_ref(value.toObject()));
    }
    for (const QJsonValue& value : object.value(QStringLiteral("rules")).toArray()) {
        bundle.rules.push_back(read_rule(value.toObject()));
    }
    bundle.method_metadata =
        read_method_metadata(object.value(QStringLiteral("method_metadata")).toObject());
    const QJsonObject quality = object.value(QStringLiteral("quality_evidence")).toObject();
    bundle.quality_evidence.method_version =
        quality.value(QStringLiteral("method_version")).toString("1").toStdString();
    bundle.quality_evidence.valid_count =
        static_cast<std::size_t>(quality.value(QStringLiteral("valid_count")).toInt(0));
    bundle.quality_evidence.missing_count =
        static_cast<std::size_t>(quality.value(QStringLiteral("missing_count")).toInt(0));
    bundle.quality_evidence.omitted_count =
        static_cast<std::size_t>(quality.value(QStringLiteral("omitted_count")).toInt(0));
    bundle.quality_evidence.source_rows =
        to_rows(quality.value(QStringLiteral("source_rows")).toArray());
    bundle.quality_evidence.parameter_source =
        quality.value(QStringLiteral("parameter_source")).toString("estimated").toStdString();
    bundle.quality_evidence.assumption_status =
        quality.value(QStringLiteral("assumption_status")).toString("not_verified").toStdString();
    bundle.quality_evidence.not_computed_reason =
        quality.value(QStringLiteral("not_computed_reason")).toString().toStdString();
    bundle.provenance = read_provenance(object.value(QStringLiteral("provenance")).toObject());
    return bundle;
}

QJsonObject report_document_to_json(const domain::ReportDocument& document)
{
    QJsonObject object;
    object.insert(QStringLiteral("schema_version"), document.schema_version);
    object.insert(QStringLiteral("profile"), report_profile_to_json(document.profile));
    object.insert(QStringLiteral("provenance"), write_provenance(document.provenance));
    object.insert(QStringLiteral("evidence"), evidence_bundle_to_json(document.evidence));
    object.insert(
        QStringLiteral("software_version"), QString::fromStdString(document.software_version));
    QJsonArray pages;
    for (const domain::ReportPageView& page : document.pages) {
        QJsonObject page_object;
        page_object.insert(QStringLiteral("source_page"), output_page_to_json(page.source_page));
        page_object.insert(
            QStringLiteral("truncated_evidence_count"),
            static_cast<int>(page.truncated_evidence_count));
        page_object.insert(
            QStringLiteral("truncated_table_row_count"),
            static_cast<int>(page.truncated_table_row_count));
        page_object.insert(QStringLiteral("show_parameter_summary"), page.show_parameter_summary);
        page_object.insert(QStringLiteral("show_method_metadata"), page.show_method_metadata);
        page_object.insert(QStringLiteral("show_provenance"), page.show_provenance);
        page_object.insert(QStringLiteral("show_hashes"), page.show_hashes);
        page_object.insert(QStringLiteral("show_evidence_appendix"), page.show_evidence_appendix);
        QJsonArray evidence;
        for (const domain::EvidenceRef& ref : page.visible_evidence) {
            evidence.append(write_evidence_ref(ref));
        }
        page_object.insert(QStringLiteral("visible_evidence"), evidence);
        pages.append(page_object);
    }
    object.insert(QStringLiteral("pages"), pages);
    return object;
}

domain::ReportDocument report_document_from_json(const QJsonObject& object)
{
    domain::ReportDocument document;
    document.schema_version = object.value(QStringLiteral("schema_version")).toInt(1);
    document.profile = report_profile_from_json(object.value(QStringLiteral("profile")).toObject());
    document.provenance = read_provenance(object.value(QStringLiteral("provenance")).toObject());
    document.evidence =
        evidence_bundle_from_json(object.value(QStringLiteral("evidence")).toObject());
    document.software_version =
        object.value(QStringLiteral("software_version")).toString("DataLab").toStdString();
    for (const QJsonValue& value : object.value(QStringLiteral("pages")).toArray()) {
        const QJsonObject page_object = value.toObject();
        domain::ReportPageView page;
        page.source_page =
            output_page_from_json(page_object.value(QStringLiteral("source_page")).toObject());
        page.truncated_evidence_count = static_cast<std::size_t>(
            page_object.value(QStringLiteral("truncated_evidence_count")).toInt(0));
        page.truncated_table_row_count = static_cast<std::size_t>(
            page_object.value(QStringLiteral("truncated_table_row_count")).toInt(0));
        page.show_parameter_summary =
            page_object.value(QStringLiteral("show_parameter_summary")).toBool(false);
        page.show_method_metadata =
            page_object.value(QStringLiteral("show_method_metadata")).toBool(false);
        page.show_provenance = page_object.value(QStringLiteral("show_provenance")).toBool(false);
        page.show_hashes = page_object.value(QStringLiteral("show_hashes")).toBool(false);
        page.show_evidence_appendix =
            page_object.value(QStringLiteral("show_evidence_appendix")).toBool(false);
        for (const QJsonValue& evidence_value :
             page_object.value(QStringLiteral("visible_evidence")).toArray()) {
            page.visible_evidence.push_back(read_evidence_ref(evidence_value.toObject()));
        }

        domain::ReportPageView view;
        view.source_page = page.source_page;
        view.visible_evidence = page.visible_evidence;
        view.truncated_evidence_count = page.truncated_evidence_count;
        view.truncated_table_row_count = page.truncated_table_row_count;
        view.show_parameter_summary = page.show_parameter_summary;
        view.show_method_metadata = page.show_method_metadata;
        view.show_provenance = page.show_provenance;
        view.show_hashes = page.show_hashes;
        view.show_evidence_appendix = page.show_evidence_appendix;
        if (document.profile.include_executive_summary) {
            view.visible_interpretation = page.source_page.interpretation;
        }
        if (document.profile.include_statistic_tables) {
            view.visible_tables = page.source_page.tables;
        }
        if (document.profile.include_plots) {
            view.visible_plots = page.source_page.plots;
        }
        if (document.profile.include_key_risks_and_limits
            || document.profile.include_diagnostics) {
            for (const domain::DiagnosticMessage& diagnostic : page.source_page.diagnostics) {
                const bool risk =
                    diagnostic.severity == domain::DiagnosticMessage::Severity::warning
                    || diagnostic.severity == domain::DiagnosticMessage::Severity::error;
                if (risk && document.profile.include_key_risks_and_limits) {
                    view.visible_diagnostics.push_back(diagnostic);
                } else if (
                    !risk && document.profile.include_diagnostics
                    && document.profile.template_kind != domain::ReportTemplateKind::customer) {
                    view.visible_diagnostics.push_back(diagnostic);
                }
            }
        }
        if (document.profile.include_rule_evidence) {
            view.visible_rules = document.evidence.rules;
        }
        document.pages.push_back(std::move(view));
    }
    return document;
}

QJsonObject report_export_manifest_to_json(const domain::ReportExportManifest& manifest)
{
    QJsonObject object;
    object.insert(QStringLiteral("schema_version"), manifest.schema_version);
    object.insert(QStringLiteral("report_id"), QString::fromStdString(manifest.report_id));
    object.insert(QStringLiteral("template_id"), QString::fromStdString(manifest.template_id));
    object.insert(
        QStringLiteral("template_version"), QString::fromStdString(manifest.template_version));
    object.insert(
        QStringLiteral("locale_language_tag"),
        QString::fromStdString(manifest.locale_language_tag));
    object.insert(QStringLiteral("timezone_id"), QString::fromStdString(manifest.timezone_id));
    object.insert(
        QStringLiteral("generated_at_utc"), QString::fromStdString(manifest.generated_at_utc));
    object.insert(
        QStringLiteral("software_version"), QString::fromStdString(manifest.software_version));
    object.insert(
        QStringLiteral("algorithm_version"), QString::fromStdString(manifest.algorithm_version));
    object.insert(QStringLiteral("facts_hash"), QString::fromStdString(manifest.facts_hash));
    object.insert(
        QStringLiteral("input_snapshot_hash"),
        QString::fromStdString(manifest.input_snapshot_hash));
    object.insert(
        QStringLiteral("pdf_relative_path"), QString::fromStdString(manifest.pdf_relative_path));
    object.insert(
        QStringLiteral("audit_json_relative_path"),
        QString::fromStdString(manifest.audit_json_relative_path));
    object.insert(QStringLiteral("title"), QString::fromStdString(manifest.title));
    object.insert(QStringLiteral("author"), QString::fromStdString(manifest.author));
    object.insert(
        QStringLiteral("analysis_page_ids"), QString::fromStdString(manifest.analysis_page_ids));
    object.insert(
        QStringLiteral("consistency_status"),
        QString::fromStdString(manifest.consistency_status));
    object.insert(
        QStringLiteral("pdfa_status"),
        QString::fromStdString(domain::pdf_compliance_status_id(manifest.pdfa_status)));
    object.insert(
        QStringLiteral("pdfua_status"),
        QString::fromStdString(domain::pdf_compliance_status_id(manifest.pdfua_status)));
    object.insert(QStringLiteral("validator_name"), QString::fromStdString(manifest.validator_name));
    object.insert(
        QStringLiteral("validator_version"), QString::fromStdString(manifest.validator_version));
    object.insert(
        QStringLiteral("validator_notes"), QString::fromStdString(manifest.validator_notes));
    object.insert(
        QStringLiteral("export_pipeline"), QString::fromStdString(manifest.export_pipeline));
    QJsonArray blockers;
    for (const auto& blocker : manifest.compliance_blockers) {
        blockers.append(QString::fromStdString(blocker));
    }
    object.insert(QStringLiteral("compliance_blockers"), blockers);
    return object;
}

domain::ReportExportManifest report_export_manifest_from_json(const QJsonObject& object)
{
    domain::ReportExportManifest manifest;
    manifest.schema_version = object.value(QStringLiteral("schema_version")).toInt(1);
    manifest.report_id = object.value(QStringLiteral("report_id")).toString().toStdString();
    manifest.template_id = object.value(QStringLiteral("template_id")).toString().toStdString();
    manifest.template_version =
        object.value(QStringLiteral("template_version")).toString("1").toStdString();
    manifest.locale_language_tag =
        object.value(QStringLiteral("locale_language_tag")).toString("zh-CN").toStdString();
    manifest.timezone_id =
        object.value(QStringLiteral("timezone_id")).toString("Asia/Shanghai").toStdString();
    manifest.generated_at_utc =
        object.value(QStringLiteral("generated_at_utc")).toString().toStdString();
    manifest.software_version =
        object.value(QStringLiteral("software_version")).toString().toStdString();
    manifest.algorithm_version =
        object.value(QStringLiteral("algorithm_version")).toString().toStdString();
    manifest.facts_hash = object.value(QStringLiteral("facts_hash")).toString().toStdString();
    manifest.input_snapshot_hash =
        object.value(QStringLiteral("input_snapshot_hash")).toString().toStdString();
    manifest.pdf_relative_path =
        object.value(QStringLiteral("pdf_relative_path")).toString().toStdString();
    manifest.audit_json_relative_path =
        object.value(QStringLiteral("audit_json_relative_path")).toString().toStdString();
    manifest.title = object.value(QStringLiteral("title")).toString().toStdString();
    manifest.author = object.value(QStringLiteral("author")).toString("DataLab").toStdString();
    manifest.analysis_page_ids =
        object.value(QStringLiteral("analysis_page_ids")).toString().toStdString();
    manifest.consistency_status =
        object.value(QStringLiteral("consistency_status")).toString("not_checked").toStdString();
    manifest.pdfa_status = domain::pdf_compliance_status_from_id(
        object.value(QStringLiteral("pdfa_status")).toString("not_validated").toStdString());
    manifest.pdfua_status = domain::pdf_compliance_status_from_id(
        object.value(QStringLiteral("pdfua_status")).toString("unsupported").toStdString());
    manifest.validator_name =
        object.value(QStringLiteral("validator_name")).toString().toStdString();
    manifest.validator_version =
        object.value(QStringLiteral("validator_version")).toString().toStdString();
    manifest.validator_notes =
        object.value(QStringLiteral("validator_notes")).toString().toStdString();
    manifest.export_pipeline =
        object.value(QStringLiteral("export_pipeline"))
            .toString(QStringLiteral("Qt QPdfWriter + QPainter"))
            .toStdString();
    for (const QJsonValue item : object.value(QStringLiteral("compliance_blockers")).toArray()) {
        manifest.compliance_blockers.push_back(item.toString().toStdString());
    }
    return manifest;
}

}  // namespace datalab::infrastructure
