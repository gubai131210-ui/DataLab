#include "application/report_assembly_service.h"
#include "application/report_localization.h"
#include "application/interpretation_service.h"
#include "domain/quality_types.h"
#include "domain/report_text_catalog.h"
#include "domain/report_types.h"
#include "ui/app_ui_tr.h"

#include <QtTest/QtTest>

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTranslator>

using datalab::application::facts_fingerprint;
using datalab::application::build_report_document;
using datalab::application::localize_report_document;
using datalab::domain::CapabilityFacts;
using datalab::domain::DataTable;
using datalab::domain::DiagnosticMessage;
using datalab::domain::OutputPage;
using datalab::domain::ReportTemplateKind;
using datalab::domain::make_report_profile;
using datalab::domain::report_text_coverage;
using datalab::domain::resolve_report_text;

namespace {

DataTable sample_table()
{
    DataTable table;
    table.name = "PinLength";
    table.source_path = "samples/capability/PinLength.csv";
    table.import_metadata.dataset_id = "phase0_report_capability_pin";
    table.columns = {"Length"};
    table.rows = {{"10.1"}, {"10.2"}};
    table.row_ids = {11, 12};
    return table;
}

OutputPage sample_page()
{
    OutputPage page;
    page.id = "cap-i18n";
    page.title = "过程能力";
    page.method_name = "Capability Analysis";
    page.method_metadata.algorithm = "capability_normal";
    page.method_metadata.version = "2";
    page.method_metadata.valid_count = 2;
    page.method_metadata.source_rows = {11, 12};
    CapabilityFacts capability;
    capability.cp = 1.33;
    capability.cpk = 1.21;
    capability.assumption_status = "not_verified";
    capability.method = "normal";
    page.facts.capability = capability;
    return page;
}

}  // namespace

class ReportLocalePhase3Test final : public QObject {
    Q_OBJECT

private slots:
    void zh_and_en_preserve_facts_and_row_ids()
    {
        auto zh_profile = make_report_profile(ReportTemplateKind::engineer);
        zh_profile.locale.language_tag = "zh-CN";
        auto en_profile = make_report_profile(ReportTemplateKind::engineer);
        en_profile.locale.language_tag = "en-US";
        en_profile.locale.number_format_locale = "en-US";

        const auto zh_doc = build_report_document(sample_table(), {sample_page()}, zh_profile);
        const auto en_doc = build_report_document(sample_table(), {sample_page()}, en_profile);
        const auto zh_localized = localize_report_document(zh_doc);
        const auto en_localized = localize_report_document(en_doc);

        QCOMPARE(
            facts_fingerprint(zh_localized.document.pages[0].source_page.facts),
            facts_fingerprint(en_localized.document.pages[0].source_page.facts));
        QCOMPARE(
            *zh_localized.document.pages[0].source_page.facts.capability->cpk,
            *en_localized.document.pages[0].source_page.facts.capability->cpk);
        QCOMPARE(
            zh_localized.document.pages[0].source_page.method_metadata.source_rows,
            en_localized.document.pages[0].source_page.method_metadata.source_rows);
        QCOMPARE(zh_localized.document.profile.locale.language_tag, std::string("zh-CN"));
        QCOMPARE(en_localized.document.profile.locale.language_tag, std::string("en-US"));
    }

    void translated_labels_differ_but_ids_stable()
    {
        const auto zh = resolve_report_text("report.title", "zh-CN");
        const auto en = resolve_report_text("report.title", "en-US");
        QVERIFY(zh.text.find("质量") != std::string::npos);
        QVERIFY(en.text.find("Quality") != std::string::npos);
        QVERIFY(zh.text != en.text);

        const auto rule_zh = resolve_report_text("report.rule_line", "zh-CN");
        const auto rule_en = resolve_report_text("report.rule_line", "en-US");
        QCOMPARE(rule_zh.text, std::string("规则"));
        QCOMPARE(rule_en.text, std::string("Rule"));
    }

    void coverage_is_complete_for_catalog_languages()
    {
        const auto zh = report_text_coverage("zh-CN");
        const auto en = report_text_coverage("en-US");
        QVERIFY(zh.catalog_size > 0);
        QCOMPARE(zh.missing_count, static_cast<std::size_t>(0));
        QCOMPARE(en.missing_count, static_cast<std::size_t>(0));
        QCOMPARE(zh.coverage_ratio, 1.0);
        QCOMPARE(en.coverage_ratio, 1.0);
    }

    void missing_translation_is_diagnosed_not_silent()
    {
        std::vector<datalab::domain::MissingTranslation> missing;
        const auto resolved =
            resolve_report_text("report.does_not_exist_xyz", "en-US", &missing);
        QVERIFY(resolved.used_fallback);
        QCOMPARE(missing.size(), static_cast<std::size_t>(1));
        QCOMPARE(missing.front().text_id, std::string("report.does_not_exist_xyz"));

        auto profile = make_report_profile(ReportTemplateKind::customer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {sample_page()}, profile);
        // Force a missing ID path through localize by ensuring diagnostic path works on empty pages
        // (required IDs exist). Verify localize keeps facts intact.
        const auto localized = localize_report_document(document);
        QCOMPARE(
            facts_fingerprint(localized.document.pages[0].source_page.facts),
            facts_fingerprint(document.pages[0].source_page.facts));
    }

    void system_locale_does_not_override_saved_report_locale()
    {
        auto profile = make_report_profile(ReportTemplateKind::audit);
        profile.locale.language_tag = "en-US";
        profile.locale.timezone_id = "UTC";
        const auto document = build_report_document(sample_table(), {sample_page()}, profile);
        QCOMPARE(document.profile.locale.language_tag, std::string("en-US"));
        QCOMPARE(document.profile.locale.timezone_id, std::string("UTC"));
        // Report locale is stored on the profile; export/PDF must read profile, not QLocale::system().
        QVERIFY(document.profile.locale.language_tag != "zh-CN"
                || profile.locale.language_tag == "zh-CN");
        QCOMPARE(document.profile.locale.language_tag, profile.locale.language_tag);
    }

    void visibility_limitation_bullets_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "配置排除了 2 行；结论只适用于纳入分析的数据。",
            "配置隐藏了 3 行；隐藏行仍计入分析口径，仅影响显示。不得将 hidden 与 excluded 合并叙述。"};
        page.interpretation.push_back(limitations);

        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {"示例结论。"};
        page.interpretation.push_back(conclusion);

        datalab::domain::InterpretationSection advice;
        advice.heading = "工程建议";
        advice.bullets = {"示例建议。"};
        page.interpretation.push_back(advice);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());
        QVERIFY(localized.document.pages[0].visible_interpretation.size() >= 3);
        const auto& bullets = localized.document.pages[0].visible_interpretation.front().bullets;
        QCOMPARE(bullets.size(), static_cast<std::size_t>(2));
        QVERIFY(bullets[0].find("excluded 2 rows") != std::string::npos);
        QVERIFY(bullets[1].find("hid 3 rows") != std::string::npos);
        QVERIFY(bullets[1].find("Do not merge") != std::string::npos);
        QCOMPARE(
            localized.document.pages[0].visible_interpretation[0].heading,
            std::string("Limitations and data quality"));
        QCOMPARE(
            localized.document.pages[0].visible_interpretation[1].heading,
            std::string("Statistical conclusion"));
        QCOMPARE(
            localized.document.pages[0].visible_interpretation[2].heading,
            std::string("Engineering advice"));
        QCOMPARE(
            facts_fingerprint(localized.document.pages[0].source_page.facts),
            facts_fingerprint(document.pages[0].source_page.facts));
    }

    void fixed_reliability_doe_capability_interp_bullets_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "存在右删失；尾部生存率由较少的风险集支持，不能把删失时间当作失效时间。",
            "无重复编码点：未报告纯误差/失拟；不得用残差 MS 冒充纯误差。",
            "不得写成过程合格（非混合模型 / 非 Hartigan dip）。",
            "不是 vendor_oracle；不得写成商业软件对齐。",
            "Cp/Cpk = 1.33（研究/预览指数；门禁禁止写成过程合格判定）。"};
        page.interpretation.push_back(limitations);
        datalab::domain::InterpretationSection advice;
        advice.heading = "工程建议";
        advice.bullets = {
            "报告模型参数和删失处理，并用现场失效机理验证模型假设；不要把分位寿命当成单件保证寿命。",
            "不得把统计预测写成法律/质量承诺；分母与时间窗口必须一并阅读。"};
        page.interpretation.push_back(advice);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& lim = localized.document.pages[0].visible_interpretation[0].bullets;
        QCOMPARE(lim.size(), static_cast<std::size_t>(5));
        QVERIFY(lim[0].find("Right censoring") != std::string::npos);
        QVERIFY(lim[1].find("pure error") != std::string::npos);
        QVERIFY(lim[2].find("Hartigan") != std::string::npos);
        QVERIFY(lim[3].find("vendor_oracle") != std::string::npos);
        QVERIFY(lim[4].find("research/preview") != std::string::npos);
        QVERIFY(lim[4].find("Cp/Cpk = 1.33") != std::string::npos);
        const auto& adv = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(adv[0].find("percentile life") != std::string::npos);
        QVERIFY(adv[1].find("legal/quality") != std::string::npos);
        QCOMPARE(
            facts_fingerprint(localized.document.pages[0].source_page.facts),
            facts_fingerprint(document.pages[0].source_page.facts));
    }

    void dynamic_doe_msa_reliability_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "在 α = 0.05 下显著的项：A、B。",
            "设计包含 3 个因子、4 个中心点；实施时保持随机化/区组记录，避免把运行顺序效应误认为因子效应。",
            "量具稳定性图发现 2 个超限点，统计上存在失控信号。",
            "偏倚回归斜率 = 0.12，低/高参考点偏倚 = -0.1 / 0.2。",
            "Kaplan-Meier 有效观测 40，失效 12，删失 8。",
            "已对 2 个失效模式做 cause-specific 分模式拟合（scheme=cause_specific）。",
            "已计算 2 个失效模式的 Aalen–Johansen 累计发生函数 CIF（algorithm=aalen_johansen_cif）。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "残差图供调查残差形态；直方图不用于证明正态。",
            "分模式可靠度为 formula_reference（竞争失效作右删失），不是 vendor_oracle / golden；阈值模型与 pinned R 对齐仍未冻结。",
            "CIF 为 formula_reference（Aalen–Johansen），不是 Fine-Gray 回归，也不是 cause-specific 可靠度或 vendor_oracle。",
            "按目标任务时间读取生存概率和置信区间；比较方案时使用分层或 Log-rank。",
            "可靠性结果不可识别（insufficient failures），不能估计寿命分位数。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("Significant terms") != std::string::npos);
        QVERIFY(conc[0].find("0.05") != std::string::npos);
        QVERIFY(conc[0].find("A、B") != std::string::npos || conc[0].find("A") != std::string::npos);
        QVERIFY(conc[0].find("显著的项") == std::string::npos);
        QVERIFY(conc[1].find("Design has") != std::string::npos);
        QVERIFY(conc[1].find("3") != std::string::npos);
        QVERIFY(conc[1].find("4") != std::string::npos);
        QVERIFY(conc[1].find("个因子") == std::string::npos);
        QVERIFY(conc[2].find("Gage stability") != std::string::npos);
        QVERIFY(conc[2].find("2") != std::string::npos);
        QVERIFY(conc[2].find("超限点") == std::string::npos);
        QVERIFY(conc[3].find("Bias regression slope") != std::string::npos);
        QVERIFY(conc[3].find("0.12") != std::string::npos);
        QVERIFY(conc[4].find("Kaplan-Meier") != std::string::npos);
        QVERIFY(conc[4].find("40") != std::string::npos);
        QVERIFY(conc[4].find("有效观测") == std::string::npos);
        QVERIFY(conc[5].find("cause-specific") != std::string::npos);
        QVERIFY(conc[5].find("2") != std::string::npos);
        QVERIFY(conc[5].find("已对") == std::string::npos);
        QVERIFY(conc[6].find("Aalen") != std::string::npos
                || conc[6].find("CIF") != std::string::npos);
        QVERIFY(conc[6].find("2") != std::string::npos);
        QVERIFY(conc[6].find("aalen_johansen_cif") != std::string::npos);
        QVERIFY(conc[6].find("已计算") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(lim[0].find("Residual plots") != std::string::npos);
        QVERIFY(lim[1].find("formula_reference") != std::string::npos);
        QVERIFY(lim[1].find("vendor_oracle") != std::string::npos);
        QVERIFY(lim[1].find("分模式可靠度") == std::string::npos);
        QVERIFY(lim[2].find("CIF") != std::string::npos
                || lim[2].find("Aalen") != std::string::npos);
        QVERIFY(lim[2].find("Fine-Gray") != std::string::npos);
        QVERIFY(lim[2].find("不是 Fine-Gray") == std::string::npos);
        QVERIFY(lim[3].find("Log-rank") != std::string::npos);
        QVERIFY(lim[3].find("按目标任务时间") == std::string::npos);
        QVERIFY(lim[4].find("non-identifiable") != std::string::npos);
        QVERIFY(lim[4].find("insufficient failures") != std::string::npos);
        QVERIFY(lim[4].find("不可识别") == std::string::npos);
    }

    void dynamic_spc_contour_pareto_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "发现 5 个控制图超限点。",
            "特殊原因规则证据：已触发 2 / 8 条；详见「特殊原因规则证据」表（含 rule_id、窗口、阈值与状态）。",
            "特殊原因策略 = all_applicable（多规则提高灵敏度也提高误报风险）",
            "效应 Pareto 最大项为 A*B；参考线方法为 未指定。条越过参考线只提供统计证据，不表示过程合格。",
            "等值线/曲面轴为 Temp 与 Pressure；其余因子编码 hold=0：Time、Speed。",
            "等值线/曲面轴为 X1 与 X2；其余因子实际单位 hold：X3=10（编码 0）。",
            "等值线/曲面轴为 A 与 B。",
            "在 α = 0.05 下显著的项：A、B、A*B。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection advice;
        advice.heading = "工程建议";
        advice.bullets = {
            "规则「beyond_control_limit」已触发。",
            "规则「trend」已触发：检查漂移。",
            "规则「单点超出 3σ 控制限」已触发：复核测量、批次、设备或取样条件，并关联原始观测行。",
            "规则「连续 9 点位于中心线同侧」已触发：调查均值偏移、分层或阶段变化，检查窗口首尾相关行。",
            "建议回查对应原始行并确认特殊原因。"};
        page.interpretation.push_back(advice);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "EWMA 仅启用「单点超出 3σ 控制限」；其余特殊原因规则不适用，超限点需结合原始观测调查。",
            "等值线/曲面按前两因子绘制，其余因子编码 hold=0；用于探索响应形状。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("Found") != std::string::npos);
        QVERIFY(conc[0].find("5") != std::string::npos);
        QVERIFY(conc[0].find("发现") == std::string::npos);
        QVERIFY(conc[1].find("triggered") != std::string::npos);
        QVERIFY(conc[1].find("2") != std::string::npos);
        QVERIFY(conc[1].find("8") != std::string::npos);
        QVERIFY(conc[2].find("Special-cause policy") != std::string::npos);
        QVERIFY(conc[2].find("all_applicable") != std::string::npos);
        QVERIFY(conc[3].find("Pareto") != std::string::npos);
        QVERIFY(conc[3].find("A*B") != std::string::npos);
        QVERIFY(conc[3].find("unspecified") != std::string::npos);
        QVERIFY(conc[3].find("未指定") == std::string::npos);
        QVERIFY(conc[4].find("Contour/surface") != std::string::npos);
        QVERIFY(conc[4].find("Temp") != std::string::npos);
        QVERIFY(conc[4].find("coded 0") != std::string::npos);
        QVERIFY(conc[4].find("Time, Speed") != std::string::npos);
        QVERIFY(conc[4].find("、") == std::string::npos);
        QVERIFY(conc[4].find("等值线") == std::string::npos);
        QVERIFY(conc[5].find("actual units") != std::string::npos);
        QVERIFY(conc[5].find("X3=10") != std::string::npos);
        QVERIFY(conc[5].find("(coded 0)") != std::string::npos);
        QVERIFY(conc[5].find("（编码") == std::string::npos);
        QVERIFY(conc[6].find("A") != std::string::npos);
        QVERIFY(conc[6].find("B") != std::string::npos);
        QVERIFY(conc[7].find("Significant") != std::string::npos
                || conc[7].find("significant") != std::string::npos);
        QVERIFY(conc[7].find("A, B, A*B") != std::string::npos);
        QVERIFY(conc[7].find("、") == std::string::npos);
        const auto& adv = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(adv[0].find("Rule") != std::string::npos);
        QVERIFY(adv[0].find("beyond_control_limit") != std::string::npos);
        QVERIFY(adv[1].find("trend") != std::string::npos);
        QVERIFY(adv[1].find("检查漂移") != std::string::npos);
        QVERIFY(adv[2].find("1 point beyond 3σ") != std::string::npos);
        QVERIFY(adv[2].find("Recheck measurement") != std::string::npos);
        QVERIFY(adv[2].find("单点超出") == std::string::npos);
        QVERIFY(adv[2].find("复核测量") == std::string::npos);
        QVERIFY(adv[3].find("9 points in a row") != std::string::npos);
        QVERIFY(adv[3].find("Investigate mean shift") != std::string::npos);
        QVERIFY(adv[3].find("连续 9 点") == std::string::npos);
        QVERIFY(adv[4].find("Review") != std::string::npos || adv[4].find("special") != std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[2].bullets;
        QVERIFY(lim[0].find("EWMA") != std::string::npos);
        QVERIFY(lim[0].find("仅启用") == std::string::npos);
        QVERIFY(lim[1].find("Contour/surface") != std::string::npos);
        QVERIFY(lim[1].find("按前两因子") == std::string::npos);
    }

    void dynamic_sigma_ttest_normality_attr_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "组内 σ = 0.12，组间 σ = 0.34，σ_BW = 0.36（imr_rs）。这些标准差只描述当前子组分解，不是规格判定。",
            "单样本 Z 检验（已知 σ=1.5），Z = 2.1，P = 0.036，差值 = 0.5。P 值与置信区间只描述当前样本证据，统计显著不等于工程差异具有实际重要性，也不是过程合格判定。",
            "双样本 t 检验（welch），P = 0.04，差值 = -1.2。P 值与置信区间只描述当前样本证据，统计显著不等于工程差异具有实际重要性，也不是过程合格判定。",
            "配对 t 检验，P = 0.01，差值 = 0.3。P 值与置信区间只描述当前样本证据，统计显著不等于工程差异具有实际重要性，也不是过程合格判定。",
            "单样本 t 检验，P = 0.2。P 值与置信区间只描述当前样本证据，统计显著不等于工程差异具有实际重要性，也不是过程合格判定。",
            "Anderson-Darling 正态性检验判定为 fail_to_reject，P = 0.25。在 alpha 下未拒绝正态假设，不能写成数据已正态。",
            "Ryan–Joiner 正态性检验判定为 reject，R = 0.95，P = 0.01。证据反对正态假设，这不是规格判定。",
            "%Defective = 2.5（Average P = 0.025）。这是当前样本的不合格品率估计，不是过程合格判定。",
            "Process Z = 1.96，由 Average P 的标准正态右尾得到。",
            "Mean DPU = 0.12。这是当前样本的单位缺陷率估计，不是过程合格判定。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "Z 检验要求已知总体标准差且正态/大样本近似成立（assumption_status=not_verified）；样本 StDev 仅展示，不参与 Z/CI。",
            "t 检验正态与独立假设未验证（assumption_status=not_verified）。",
            "正态假设状态为 not_verified。",
            "二项过程能力未验证独立性、恒定 p 与稳定性（assumption_status=not_verified），不能写成过程合格。",
            "泊松过程能力未验证独立性、恒定 DPU 与稳定性（assumption_status=not_verified），不能写成过程合格。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("Within") != std::string::npos);
        QVERIFY(conc[0].find("0.12") != std::string::npos);
        QVERIFY(conc[0].find("0.34") != std::string::npos);
        QVERIFY(conc[0].find("0.36") != std::string::npos);
        QVERIFY(conc[0].find("imr_rs") != std::string::npos);
        QVERIFY(conc[0].find("组内") == std::string::npos);
        QVERIFY(conc[1].find("One-sample Z") != std::string::npos);
        QVERIFY(conc[1].find("1.5") != std::string::npos);
        QVERIFY(conc[1].find("2.1") != std::string::npos);
        QVERIFY(conc[1].find("difference") != std::string::npos);
        QVERIFY(conc[1].find("单样本 Z") == std::string::npos);
        QVERIFY(conc[2].find("Two-sample t") != std::string::npos);
        QVERIFY(conc[2].find("welch") != std::string::npos);
        QVERIFY(conc[2].find("双样本") == std::string::npos);
        QVERIFY(conc[3].find("Paired t") != std::string::npos);
        QVERIFY(conc[3].find("配对") == std::string::npos);
        QVERIFY(conc[4].find("One-sample t") != std::string::npos);
        QVERIFY(conc[4].find("0.2") != std::string::npos);
        QVERIFY(conc[5].find("Anderson-Darling") != std::string::npos);
        QVERIFY(conc[5].find("fail_to_reject") != std::string::npos);
        QVERIFY(conc[5].find("prove") != std::string::npos
                || conc[5].find("normal") != std::string::npos);
        QVERIFY(conc[5].find("不能写成") == std::string::npos);
        QVERIFY(conc[6].find("Ryan") != std::string::npos);
        QVERIFY(conc[6].find("0.95") != std::string::npos);
        QVERIFY(conc[6].find("证据反对") == std::string::npos);
        QVERIFY(conc[7].find("%Defective") != std::string::npos);
        QVERIFY(conc[7].find("0.025") != std::string::npos);
        QVERIFY(conc[7].find("不合格品率") == std::string::npos);
        QVERIFY(conc[8].find("Process Z") != std::string::npos);
        QVERIFY(conc[8].find("1.96") != std::string::npos);
        QVERIFY(conc[8].find("由 Average") == std::string::npos);
        QVERIFY(conc[9].find("Mean DPU") != std::string::npos);
        QVERIFY(conc[9].find("单位缺陷率") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(lim[0].find("Z test requires") != std::string::npos);
        QVERIFY(lim[0].find("not_verified") != std::string::npos);
        QVERIFY(lim[0].find("要求已知") == std::string::npos);
        QVERIFY(lim[1].find("t-test") != std::string::npos
                || lim[1].find("t test") != std::string::npos
                || lim[1].find("unverified") != std::string::npos);
        QVERIFY(lim[1].find("正态与独立") == std::string::npos);
        QVERIFY(lim[2].find("Normality assumption") != std::string::npos);
        QVERIFY(lim[2].find("正态假设状态") == std::string::npos);
        QVERIFY(lim[3].find("Binomial") != std::string::npos);
        QVERIFY(lim[3].find("二项过程") == std::string::npos);
        QVERIFY(lim[4].find("Poisson") != std::string::npos);
        QVERIFY(lim[4].find("泊松过程") == std::string::npos);
    }

    void dynamic_outlier_corr_oc_equiv_anom_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "Grubbs 异常值检验 G = 2.5，P = 0.04。在 α 下拒绝“无异常值”假设，嫌疑观测需工程调查，"
            "P 值只描述与正态假设下的一致性，勿当作删点依据，也勿当成异常已核实。",
            "Dixon r10 异常值检验 r = 0.55。未拒绝“无异常值”假设，不能证明数据中没有异常值。",
            "Pearson 相关系数表示变量关联方向与强度；P-Value 反映在零相关假设下的证据强度，"
            "不能单独证明因果关系。未拒绝零相关不能写成已证明无关。",
            "二项 OC 曲线描述在 n=50、c=1 计划下，不同不合格品率 p 的接收概率 Pa(p)；"
            "OC 用于评估抽样方案风险，不能替代对具体批次的接收判定。",
            "等价性检验比较差值与界限 [-0.1, 0.1]；CI(tost_1_minus_2alpha) 为 [-0.05, 0.04]，"
            "区间落在等价界限内。 双单侧 P 值分别为 0.01 与 0.02。这只陈述界限关系，不能写成已证明等价。",
            "ANOM 在 α=0.05 下比较各组均值与总体均值；超出 UDL/LDL 的组数 = 1。"
            "这标记值得进一步调查的组，不能写成组间无差异或应剔除该组。",
            "ANOVA 的总体 F 检验显著时，只能说明至少存在一组均值差异；"
            "需要后续多重比较确定具体组对。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "Grubbs 要求近似正态且至多一个异常值（assumption_status=unverified）。",
            "Dixon r10 要求近似正态、至多一个异常值，且 P 可能为临界值插值近似"
            "（assumption_status=approximate）。",
            "相关分析使用 complete-case 对齐（N = 40），assumption_status=ok。",
            "协方差矩阵基于 complete-case 样本协方差；对角线为各变量方差。",
            "偏相关在控制其余变量后给出 Pearson 偏系数；不能写成已排除混杂或已证明因果。",
            "Pa(AQL) = 0.95 只反映 AQL 风险点，不是生产判定结论。",
            "Pa(RQL) = 0.1 只反映 RQL 风险点，不是拒收证明。",
            "批大小 N = 1000 仅作摘要；本轮 OC 仍用二项（无限批）近似。",
            "TOST 依赖正态与独立样本假设（assumption_status=unverified）。",
            "决策限方法 = nelson（正态近似）；二项/泊松计数不适用本命令。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("Grubbs") != std::string::npos);
        QVERIFY(conc[0].find("2.5") != std::string::npos);
        QVERIFY(conc[0].find("0.04") != std::string::npos);
        QVERIFY(conc[0].find("异常值检验") == std::string::npos);
        QVERIFY(conc[1].find("Dixon") != std::string::npos);
        QVERIFY(conc[1].find("0.55") != std::string::npos);
        QVERIFY(conc[1].find("未拒绝") == std::string::npos);
        QVERIFY(conc[2].find("Pearson") != std::string::npos);
        QVERIFY(conc[2].find("causation") != std::string::npos
                || conc[2].find("correlation") != std::string::npos);
        QVERIFY(conc[2].find("不能单独") == std::string::npos);
        QVERIFY(conc[3].find("50") != std::string::npos);
        QVERIFY(conc[3].find("1") != std::string::npos);
        QVERIFY(conc[3].find("OC") != std::string::npos);
        QVERIFY(conc[3].find("不合格品率") == std::string::npos);
        QVERIFY(conc[4].find("Equivalence") != std::string::npos
                || conc[4].find("equivalence") != std::string::npos);
        QVERIFY(conc[4].find("-0.1") != std::string::npos);
        QVERIFY(conc[4].find("0.1") != std::string::npos);
        QVERIFY(conc[4].find("CI(") != std::string::npos);
        QVERIFY(conc[4].find("tost_1_minus_2alpha") != std::string::npos);
        QVERIFY(conc[4].find("-0.05") != std::string::npos);
        QVERIFY(conc[4].find("0.01") != std::string::npos);
        QVERIFY(conc[4].find("0.02") != std::string::npos);
        QVERIFY(conc[4].find("P-values") != std::string::npos
                || conc[4].find("P-value") != std::string::npos
                || conc[4].find("one-sided") != std::string::npos);
        QVERIFY(conc[4].find("为") == std::string::npos);
        QVERIFY(conc[4].find("已证明等价") == std::string::npos);
        QVERIFY(conc[4].find("双单侧") == std::string::npos);
        QVERIFY(conc[5].find("ANOM") != std::string::npos);
        QVERIFY(conc[5].find("0.05") != std::string::npos);
        QVERIFY(conc[5].find("应剔除") == std::string::npos);
        QVERIFY(conc[6].find("ANOVA") != std::string::npos);
        QVERIFY(conc[6].find("多重比较") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(lim[0].find("Grubbs") != std::string::npos);
        QVERIFY(lim[0].find("unverified") != std::string::npos);
        QVERIFY(lim[0].find("要求近似") == std::string::npos);
        QVERIFY(lim[1].find("Dixon") != std::string::npos);
        QVERIFY(lim[1].find("approximate") != std::string::npos);
        QVERIFY(lim[2].find("40") != std::string::npos);
        QVERIFY(lim[2].find("complete-case") != std::string::npos);
        QVERIFY(lim[2].find("相关分析") == std::string::npos);
        QVERIFY(lim[3].find("covariance") != std::string::npos
                || lim[3].find("Covariance") != std::string::npos);
        QVERIFY(lim[3].find("对角线") == std::string::npos);
        QVERIFY(lim[4].find("Partial") != std::string::npos
                || lim[4].find("partial") != std::string::npos);
        QVERIFY(lim[4].find("偏相关") == std::string::npos);
        QVERIFY(lim[5].find("0.95") != std::string::npos);
        QVERIFY(lim[5].find("生产判定") == std::string::npos);
        QVERIFY(lim[6].find("0.1") != std::string::npos);
        QVERIFY(lim[6].find("拒收证明") == std::string::npos);
        QVERIFY(lim[7].find("1000") != std::string::npos);
        QVERIFY(lim[7].find("批大小") == std::string::npos);
        QVERIFY(lim[8].find("TOST") != std::string::npos);
        QVERIFY(lim[8].find("unverified") != std::string::npos);
        QVERIFY(lim[8].find("正态与独立") == std::string::npos);
        QVERIFY(lim[9].find("nelson") != std::string::npos);
        QVERIFY(lim[9].find("决策限") == std::string::npos);
    }

    void dynamic_prop_poisson_boxcox_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "两比例检验方法为 normal，p̂1 = 0.2，p̂2 = 0.35，差值 = -0.15，Wald P = 0.04。"
            "P 值只描述两组比例差异的证据强度，不是规格判定。",
            "单比例检验方法为 exact（CI=clopper_pearson），p̂ = 0.25，H0: p = 0.3，P = 0.2。"
            "P 值只描述与假设比例的证据强度，不是规格判定。",
            "双样本泊松率方法为 exact，比较量=率比 ρ=λ1/λ2，ρ̂ = 1.5，P = 0.03。"
            "P 值只描述与假设发生率的证据强度，不是规格判定。",
            "单样本泊松率方法为 exact，λ̂ = 2.5，H0: λ = 2，P = 0.1。"
            "P 值只描述与假设发生率的证据强度，不是规格判定。",
            "Box-Cox 选定 λ = 0.5，有效观测 N = 40，变换后标准化 SD = 1.1。"
            "概率图只是诊断，不能写成数据已正态。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "比例假设未验证独立性与恒定 p（assumption_status=unverified）。",
            "泊松率假设未验证独立同质发生率（assumption_status=unverified）。",
            "变换后能力指数（若出现）不是过程合格判定（assumption_status=diagnostic）。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("Two-proportion") != std::string::npos
                || conc[0].find("two-proportion") != std::string::npos
                || conc[0].find("proportion") != std::string::npos);
        QVERIFY(conc[0].find("0.2") != std::string::npos);
        QVERIFY(conc[0].find("规格判定") == std::string::npos);
        QVERIFY(conc[1].find("One-proportion") != std::string::npos
                || conc[1].find("one-proportion") != std::string::npos
                || conc[1].find("proportion") != std::string::npos);
        QVERIFY(conc[1].find("clopper_pearson") != std::string::npos);
        QVERIFY(conc[1].find("单比例") == std::string::npos);
        QVERIFY(conc[2].find("Poisson") != std::string::npos);
        QVERIFY(conc[2].find("1.5") != std::string::npos);
        QVERIFY(conc[2].find("比较量") == std::string::npos);
        QVERIFY(conc[3].find("2.5") != std::string::npos);
        QVERIFY(conc[3].find("单样本泊松") == std::string::npos);
        QVERIFY(conc[4].find("Box-Cox") != std::string::npos);
        QVERIFY(conc[4].find("0.5") != std::string::npos);
        QVERIFY(conc[4].find("不能写成") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(lim[0].find("Proportion") != std::string::npos
                || lim[0].find("proportion") != std::string::npos);
        QVERIFY(lim[0].find("恒定") == std::string::npos);
        QVERIFY(lim[1].find("Poisson") != std::string::npos);
        QVERIFY(lim[1].find("独立同质") == std::string::npos);
        QVERIFY(lim[2].find("transformation") != std::string::npos
                || lim[2].find("capability") != std::string::npos);
        QVERIFY(lim[2].find("合格判定") == std::string::npos);
    }

    void dynamic_power_forecast_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "在效应量 = 0.5、α = 0.05 下，估计实际功效 = 0.82。",
            // Matches InterpretationService generic power line (single bullet).
            "在效应量 = 0.8、目标功效 = 0.9 下，估计实际功效 = 0.91，对应样本量 n = 64。"
            "数值是假设条件下的计算值。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection advice;
        advice.heading = "工程建议";
        advice.bullets = {
            "将效应量预先定义为具有工程意义的最小差异，并同时考虑实际脱落、分组和方差不确定性。",
            "样本量结果是模型和效应量假设的条件结果；试验前应进行敏感性分析并向上取整到可执行的分组方案。",
            "MASE > 1，当前模型不优于朴素基准；上线前应比较替代模型或扩大验证窗口。"};
        page.interpretation.push_back(advice);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "功效/样本量结果表为空，不能给出设计建议。",
            "缺少 Actual Power 或 Effect Size；结果不能用于样本量决策。",
            "效应量或 α 配置不在有效范围，不能把数值当作正式设计依据。",
            "预测结果表为空，不能评价预测或未来期。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc.size() >= 2);
        QVERIFY(conc[0].find("0.5") != std::string::npos);
        QVERIFY(conc[0].find("0.05") != std::string::npos);
        QVERIFY(conc[0].find("0.82") != std::string::npos);
        QVERIFY(conc[0].find("effect") != std::string::npos
                || conc[0].find("Effect") != std::string::npos
                || conc[0].find("power") != std::string::npos);
        QVERIFY(conc[0].find("效应量") == std::string::npos);
        QVERIFY(conc[0].find("实际功效") == std::string::npos);
        QVERIFY(conc[1].find("0.8") != std::string::npos);
        QVERIFY(conc[1].find("0.9") != std::string::npos);
        QVERIFY(conc[1].find("0.91") != std::string::npos);
        QVERIFY(conc[1].find("64") != std::string::npos);
        QVERIFY(conc[1].find("assumptions") != std::string::npos
                || conc[1].find("Assumptions") != std::string::npos
                || conc[1].find("target power") != std::string::npos);
        QVERIFY(conc[1].find("效应量") == std::string::npos);
        QVERIFY(conc[1].find("目标功效") == std::string::npos);
        QVERIFY(conc[1].find("假设条件") == std::string::npos);
        QVERIFY(conc[1].find("对应样本量") == std::string::npos);
        const auto& adv = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(adv.size() >= 3);
        QVERIFY(adv[0].find("effect size") != std::string::npos
                || adv[0].find("Effect") != std::string::npos
                || adv[0].find("engineering") != std::string::npos);
        QVERIFY(adv[0].find("预先定义") == std::string::npos);
        QVERIFY(adv[0].find("脱落") == std::string::npos);
        QVERIFY(adv[1].find("Sample-size") != std::string::npos
                || adv[1].find("sample-size") != std::string::npos
                || adv[1].find("conditional") != std::string::npos);
        QVERIFY(adv[1].find("样本量结果") == std::string::npos);
        QVERIFY(adv[2].find("MASE") != std::string::npos);
        QVERIFY(adv[2].find("naïve") != std::string::npos
                || adv[2].find("naive") != std::string::npos
                || adv[2].find("baseline") != std::string::npos);
        QVERIFY(adv[2].find("朴素") == std::string::npos);
        QVERIFY(adv[2].find("上线前") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[2].bullets;
        QVERIFY(lim.size() >= 4);
        QVERIFY(lim[0].find("empty") != std::string::npos
                || lim[0].find("Empty") != std::string::npos
                || lim[0].find("power") != std::string::npos);
        QVERIFY(lim[0].find("功效/") == std::string::npos);
        QVERIFY(lim[0].find("设计建议") == std::string::npos);
        QVERIFY(lim[1].find("Actual Power") != std::string::npos);
        QVERIFY(lim[1].find("不能用于") == std::string::npos);
        QVERIFY(lim[1].find("样本量决策") == std::string::npos);
        QVERIFY(lim[2].find("α") != std::string::npos
                || lim[2].find("Effect") != std::string::npos);
        QVERIFY(lim[2].find("正式设计") == std::string::npos);
        QVERIFY(lim[3].find("Forecast") != std::string::npos
                || lim[3].find("forecast") != std::string::npos
                || lim[3].find("empty") != std::string::npos);
        QVERIFY(lim[3].find("预测结果") == std::string::npos);
    }

    void dynamic_forecast_mape_regression_anova_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "预测误差指标已计算；MAPE = 12.5。",
            "R² = 0.91 只描述当前样本拟合程度，不能单独判定模型合格。",
            "当前有统计证据的项：A、B、A*B。",
            "Tukey 同时置信水平 = 0.95，显著比较对数 = 2；区间含 0 不显著。"
            "同字母仅表示在本产品 Tukey 近似规则下未显著不同，不能写成已证明相同。",
            "在当前显著性水平下，没有可报告为显著的 ANOVA 项；这不等于各组完全相同。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection advice;
        advice.heading = "工程建议";
        advice.bullets = {
            "最大 VIF = 8.2，提示共线性调查，不会自动删除预测变量。",
            "结合残差对拟合值图、残差顺序图、拟合线图置信/预测带、杠杆值和 Cook's D 后再解释模型。"};
        page.interpretation.push_back(advice);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "存在打标观测，请结合异常观测表调查；解释层不会自动删除这些观测。",
            "残差假设检查提供了需要调查的证据；不能把系数显著性直接写成因果关系。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("Forecast") != std::string::npos
                || conc[0].find("forecast") != std::string::npos
                || conc[0].find("MAPE") != std::string::npos);
        QVERIFY(conc[0].find("12.5") != std::string::npos);
        QVERIFY(conc[0].find("预测误差") == std::string::npos);
        QVERIFY(conc[1].find("0.91") != std::string::npos);
        QVERIFY(conc[1].find("只描述") == std::string::npos);
        QVERIFY(conc[2].find("A") != std::string::npos);
        QVERIFY(conc[2].find("统计证据") == std::string::npos);
        QVERIFY(conc[3].find("Tukey") != std::string::npos);
        QVERIFY(conc[3].find("0.95") != std::string::npos);
        QVERIFY(conc[3].find("已证明相同") == std::string::npos);
        QVERIFY(conc[4].find("ANOVA") != std::string::npos);
        QVERIFY(conc[4].find("完全相同") == std::string::npos);
        const auto& adv = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(adv[0].find("8.2") != std::string::npos);
        QVERIFY(adv[0].find("共线性") == std::string::npos);
        QVERIFY(adv[1].find("Cook") != std::string::npos
                || adv[1].find("residual") != std::string::npos
                || adv[1].find("Residual") != std::string::npos);
        QVERIFY(adv[1].find("后再解释") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[2].bullets;
        QVERIFY(lim[0].find("Flagged") != std::string::npos
                || lim[0].find("flagged") != std::string::npos
                || lim[0].find("auto-delete") != std::string::npos);
        QVERIFY(lim[0].find("打标") == std::string::npos);
        QVERIFY(lim[1].find("causation") != std::string::npos
                || lim[1].find("Residual") != std::string::npos
                || lim[1].find("assumption") != std::string::npos);
        QVERIFY(lim[1].find("因果关系") == std::string::npos);
    }

    void dynamic_doe_rsm_desirability_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "设计类型 = fractional；运行数 = 16；分辨度 = 4。"
            "可将设计矩阵写入工作表后用 DOE 响应分析衔接。",
            "多响应 Desirability 优化覆盖 2 个响应（Y1、Y2）；总体 D 为几何平均。",
            "最佳候选总体 Desirability = 0.812345；这是在编码 ±1 设计空间内枚举得到的结果。",
            "RSM 二次模型（编码单位）：因子数 = 3；项数 = 10；R² = 0.95。",
            "当前 |t| 最大项 = A*A（仅描述相对大小，不是工程最优）。",
            "设计来源 ID = ccd_demo（ccd）；编码模式 = design_bounds。",
            "失拟检验可用（纯误差 DF = 3；失拟 DF = 5；P = 0.21）；"
            "证据类型 formula_reference，不是 vendor_oracle。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection advice;
        advice.heading = "工程建议";
        advice.bullets = {
            "生成器：D=ABC。别名结构仅说明混淆模式，不是显著性结论。",
            "多响应冲突时优先核对各响应 Desirability 与权重；"
            "确认性试验应覆盖总体 D 靠前且工程上可实施的组合。",
            "响应优化在编码 ±1 空间枚举候选组合；结果不能外推到设计空间之外。"};
        page.interpretation.push_back(advice);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("Design type") != std::string::npos);
        QVERIFY(conc[0].find("fractional") != std::string::npos);
        QVERIFY(conc[0].find("16") != std::string::npos);
        QVERIFY(conc[0].find("resolution") != std::string::npos);
        QVERIFY(conc[0].find("设计类型") == std::string::npos);
        QVERIFY(conc[1].find("desirability") != std::string::npos
                || conc[1].find("Desirability") != std::string::npos);
        QVERIFY(conc[1].find("Y1") != std::string::npos);
        QVERIFY(conc[1].find("个响应") == std::string::npos);
        QVERIFY(conc[2].find("0.812345") != std::string::npos);
        QVERIFY(conc[2].find("枚举得到") == std::string::npos);
        QVERIFY(conc[3].find("RSM") != std::string::npos);
        QVERIFY(conc[3].find("0.95") != std::string::npos);
        QVERIFY(conc[3].find("二次模型") == std::string::npos);
        QVERIFY(conc[4].find("A*A") != std::string::npos);
        QVERIFY(conc[4].find("工程最优") == std::string::npos);
        QVERIFY(conc[5].find("ccd_demo") != std::string::npos);
        QVERIFY(conc[5].find("design_bounds") != std::string::npos);
        QVERIFY(conc[5].find("设计来源") == std::string::npos);
        QVERIFY(conc[6].find("Lack-of-fit") != std::string::npos
                || conc[6].find("lack-of-fit") != std::string::npos
                || conc[6].find("formula_reference") != std::string::npos);
        QVERIFY(conc[6].find("0.21") != std::string::npos);
        QVERIFY(conc[6].find("失拟检验") == std::string::npos);
        QVERIFY(conc[6].find("vendor_oracle") != std::string::npos);
        const auto& adv = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(adv[0].find("Generators") != std::string::npos
                || adv[0].find("D=ABC") != std::string::npos);
        QVERIFY(adv[0].find("别名结构") == std::string::npos);
        QVERIFY(adv[1].find("conflict") != std::string::npos
                || adv[1].find("desirability") != std::string::npos
                || adv[1].find("Desirability") != std::string::npos);
        QVERIFY(adv[1].find("多响应冲突") == std::string::npos);
        QVERIFY(adv[2].find("coded") != std::string::npos
                || adv[2].find("±1") != std::string::npos);
        QVERIFY(adv[2].find("不能外推") == std::string::npos);
    }

    void dynamic_msa_emp_ndc_type1_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "EMP Crossed：ICC(with interaction) = 0.82，分级 = First Class。"
            "这是 Wheeler 监控能力分级，不是 AIAG 合格判定，不能写成量具合格。",
            "ndc = 3，小于 5，提示测量系统分辨力需要调查。",
            "ndc = 8；ndc 只描述当前研究中零件间变异相对 Gage 变异的分辨力。",
            "Total Gage R&R %Study Var = 12.5；%Contribution 与 %Study Var 口径不同，不能混用。",
            "Average Bias = 0.01。",
            "Average Bias p = 0.40。",
            "Constant p = 0.12。",
            "%Linearity = 4.5；基于用户提供的过程变差（6σ）。",
            "偏倚检验 P = 0.03，与参考值差异具有统计证据。",
            "偏倚检验 P = 0.40，未发现与参考值差异的统计证据。",
            "Cgk = 1.45；应结合组织规定的能力门槛解释，而非套用单一通用阈值。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection advice;
        advice.heading = "工程建议";
        advice.bullets = {
            "%Tolerance = 18.2；请与产品公差及风险等级核对后决定校准、维修或放行策略。",
            "Part×Operator 交互 p>0.25，传统流程可考虑缩减模型；当前结果仍保留完整交互。"};
        page.interpretation.push_back(advice);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "Probable Error = 0.05；用于对照测量增量是否物理合理，不是公差合格证明。",
            "ndc<5 不是量具不合格的绝对结论，需要结合 %Study Var 和公差风险。",
            "斜率检验 p≤0.05，提示线性显著；应分别解读各参考水平的偏倚，不宜只用平均偏倚。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("Wheeler") != std::string::npos);
        QVERIFY(conc[0].find("0.82") != std::string::npos);
        QVERIFY(conc[0].find("First Class") != std::string::npos);
        QVERIFY(conc[0].find("量具合格") == std::string::npos);
        QVERIFY(conc[1].find("3") != std::string::npos);
        QVERIFY(conc[1].find("below 5") != std::string::npos
                || conc[1].find("investigation") != std::string::npos);
        QVERIFY(conc[1].find("小于 5") == std::string::npos);
        QVERIFY(conc[2].find("8") != std::string::npos);
        QVERIFY(conc[2].find("只描述") == std::string::npos);
        QVERIFY(conc[3].find("12.5") != std::string::npos);
        QVERIFY(conc[3].find("口径不同") == std::string::npos);
        QVERIFY(conc[4].find("0.01") != std::string::npos);
        QVERIFY(conc[5].find("0.40") != std::string::npos);
        QVERIFY(conc[6].find("0.12") != std::string::npos);
        QVERIFY(conc[7].find("4.5") != std::string::npos);
        QVERIFY(conc[7].find("过程变差") == std::string::npos);
        QVERIFY(conc[8].find("0.03") != std::string::npos);
        QVERIFY(conc[8].find("statistical evidence") != std::string::npos
                || conc[8].find("evidence") != std::string::npos);
        QVERIFY(conc[8].find("具有统计证据") == std::string::npos);
        QVERIFY(conc[9].find("0.40") != std::string::npos);
        QVERIFY(conc[9].find("未发现") == std::string::npos);
        QVERIFY(conc[10].find("1.45") != std::string::npos);
        QVERIFY(conc[10].find("通用阈值") == std::string::npos);
        const auto& adv = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(adv[0].find("18.2") != std::string::npos);
        QVERIFY(adv[0].find("放行策略") == std::string::npos);
        QVERIFY(adv[1].find("Part") != std::string::npos);
        QVERIFY(adv[1].find("缩减模型") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[2].bullets;
        QVERIFY(lim[0].find("0.05") != std::string::npos);
        QVERIFY(lim[0].find("公差合格证明") == std::string::npos);
        QVERIFY(lim[1].find("ndc") != std::string::npos);
        QVERIFY(lim[1].find("绝对结论") == std::string::npos);
        QVERIFY(lim[2].find("Slope") != std::string::npos
                || lim[2].find("slope") != std::string::npos
                || lim[2].find("linearity") != std::string::npos);
        QVERIFY(lim[2].find("不宜只用") == std::string::npos);
    }

    void dynamic_nonparametric_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "kruskal_wallis 使用 normal 近似，P = 0.012，已做 ties 修正。"
            "未拒绝原假设不能证明两组或各组分布相同。",
            "Walsh 估计中位数为 1.5，置信区间 [0.2, 2.8]。"
            "区间只描述中位数的可能范围，不能写成已证明等于假设值。",
            "位置差异估计为 0.8，置信区间 [-0.1, 1.7]。区间只描述位置差的可能范围。",
            "Dunn–Bonferroni 成对比较共 3 对；Grouping 字母只反映该规则下的显著矩阵，"
            "不能写成组间已证明相同或不同。",
            "Steel–Dwass（近似）成对比较共 3 对；Grouping 字母只反映该规则下的显著矩阵，"
            "不能写成组间已证明相同或不同。",
            "Nemenyi（近似）成对比较共 6 对；Grouping 字母只反映该规则下的显著矩阵，"
            "不能写成处理间已证明相同或不同。",
            "Friedman S（调整后）= 12.4，P = 0.002。"
            "这只陈述区组设计下处理间秩差异证据，不能写成已证明相同或不同。",
            "符号检验（二项精确）P = 0.31。未拒绝原假设不能证明中位数等于假设值。",
            "Sign 中位数置信区间为 [1.0, 3.0]，只描述位置不确定性，不能写成已证明等于假设值。",
            "游程检验 P = 0.44。结果只陈述相对比较准则 K 的顺序随机性证据，"
            "不能写成已证明过程受控或失控。",
            "单样本 Wilcoxon 符号秩 P = 0.11。"
            "未拒绝原假设不能证明中位数等于假设值；Walsh 估计与区间只描述位置。",
            "配对 Wilcoxon 符号秩 P = 0.09。未拒绝原假设不能证明配对差分中位数为 0。",
            "Mood 中位数检验 χ² = 5.2，P = 0.07。"
            "未拒绝原假设不能证明各组中位数相同；各组 Sign CI 只描述组内位置。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "箱线图与个体值图基于 24 个有效观测，缺失单元格未进入图形。",
            "比较准则 K = 10.5；等号归入 ≤K 侧。",
            "存在小样本组，近似 P 值只作提示。",
            "Walsh 估计中位数为 2.0。点估计不能写成已证明等于假设值。",
            "Walsh 估计中位数为 2.0，置信下界 1.1。",
            "Walsh 估计中位数为 2.0，置信上界 2.9。",
            "位置差异估计为 0.5，置信下界 -0.2。",
            "位置差异估计为 0.5，置信上界 1.2。",
            "游程检验 未计算出 P。结果只陈述相对比较准则 K 的顺序随机性证据，"
            "不能写成已证明过程受控或失控。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("kruskal_wallis") != std::string::npos);
        QVERIFY(conc[0].find("0.012") != std::string::npos);
        QVERIFY(conc[0].find("ties") != std::string::npos);
        QVERIFY(conc[0].find("未拒绝") == std::string::npos);
        QVERIFY(conc[1].find("Walsh") != std::string::npos);
        QVERIFY(conc[1].find("1.5") != std::string::npos);
        QVERIFY(conc[1].find("已证明等于") == std::string::npos);
        QVERIFY(conc[2].find("0.8") != std::string::npos);
        QVERIFY(conc[2].find("位置差") == std::string::npos);
        QVERIFY(conc[3].find("3") != std::string::npos);
        QVERIFY(conc[3].find("组间已证明") == std::string::npos);
        QVERIFY(conc[4].find("Steel") != std::string::npos);
        QVERIFY(conc[5].find("Nemenyi") != std::string::npos);
        QVERIFY(conc[5].find("处理间已证明") == std::string::npos);
        QVERIFY(conc[6].find("Friedman") != std::string::npos);
        QVERIFY(conc[6].find("12.4") != std::string::npos);
        QVERIFY(conc[7].find("0.31") != std::string::npos);
        QVERIFY(conc[7].find("符号检验") == std::string::npos);
        QVERIFY(conc[8].find("1.0") != std::string::npos);
        QVERIFY(conc[9].find("0.44") != std::string::npos);
        QVERIFY(conc[9].find("受控或失控") == std::string::npos);
        QVERIFY(conc[10].find("0.11") != std::string::npos);
        QVERIFY(conc[11].find("0.09") != std::string::npos);
        QVERIFY(conc[12].find("5.2") != std::string::npos);
        QVERIFY(conc[12].find("各组中位数相同") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(lim[0].find("24") != std::string::npos);
        QVERIFY(lim[0].find("缺失单元格") == std::string::npos);
        QVERIFY(lim[1].find("10.5") != std::string::npos);
        QVERIFY(lim[1].find("等号归入") == std::string::npos);
        QVERIFY(lim[2].find("Small-sample") != std::string::npos
                || lim[2].find("approximate") != std::string::npos
                || lim[2].find("hint") != std::string::npos);
        QVERIFY(lim[2].find("只作提示") == std::string::npos);
        QVERIFY(lim[3].find("2.0") != std::string::npos);
        QVERIFY(lim[3].find("点估计不能") == std::string::npos);
        QVERIFY(lim[4].find("1.1") != std::string::npos);
        QVERIFY(lim[4].find("置信下界") == std::string::npos);
        QVERIFY(lim[5].find("2.9") != std::string::npos);
        QVERIFY(lim[5].find("置信上界") == std::string::npos);
        QVERIFY(lim[6].find("-0.2") != std::string::npos);
        QVERIFY(lim[6].find("置信下界") == std::string::npos);
        QVERIFY(lim[7].find("1.2") != std::string::npos);
        QVERIFY(lim[8].find("did not compute") != std::string::npos
                || lim[8].find("Runs") != std::string::npos
                || lim[8].find("runs") != std::string::npos);
        QVERIFY(lim[8].find("未计算出") == std::string::npos);
    }

    void dynamic_chi2_association_mcnemar_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "Fisher 精确检验 P = 0.041，优势比 OR = 2.5。"
            "P 值只描述当前 2×2 表与独立性假设的一致程度，不能写成已证明存在或不存在关联。",
            "Pearson χ² = 8.1，P = 0.017。"
            "P 值只描述当前列联表与独立性假设的一致程度，不能证明因果关系。",
            "最大 |调整残差| = 3.2，贡献最大单元格: A×B。"
            "这些量只帮助定位偏离，不能写成已证明关联或无关联。",
            "交叉表汇总 3 × 4 分类，N = 120。"
            "本输出只提供频数与百分比，不做独立性检验；检验请使用列联表卡方。",
            "泊松拟合优度 Pearson χ² = 4.4，P = 0.22，DF = 3，λ̂ = 1.2。"
            "P 值只描述观察计数与泊松模型的偏离程度，不能证明总体服从泊松分布。",
            "拟合优度 Pearson χ² = 6.0，P = 0.11，DF = 4。"
            "P 值只描述观察频数与指定比例的一致程度，不能证明总体比例等于假设。",
            "McNemar（Edwards 连续性校正）χ² = 5.0，配对有效 N = 40，不一致对数 b+c = 12，P = 0.025。"
            "结果只陈述边际比例差异证据，不能写成已证明相同或不同。",
            "Cochran Q = 9.8，DF = 3，P = 0.02。"
            "结果只陈述配对二元处理间差异证据，不能写成已证明阳性率相同或不同。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "GOF 有效性状态为 caution（最小期望频数 = 2.5）。",
            "缺失 N* = 7，未进入类别计数。",
            "调整残差热图只描述单元格相对独立假设的偏离方向与相对大小，不是因果证据。",
            "McNemar 未计算出统计量（非二元、无不一致对或输入不足）。"
            "这只说明当前配对表不支持该检验，不能写成前后比例已证明相同或不同。",
            "Cochran Q 未计算出统计量（例如列数不足、编码失败或分母退化）。"
            "两列配对请用 McNemar；不得写成已证明各处理阳性率相同或不同。",
            "存在期望频数过小的单元格，卡方近似可能不可靠。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("Fisher") != std::string::npos);
        QVERIFY(conc[0].find("0.041") != std::string::npos);
        QVERIFY(conc[0].find("2.5") != std::string::npos);
        QVERIFY(conc[0].find("已证明存在") == std::string::npos);
        QVERIFY(conc[1].find("8.1") != std::string::npos);
        QVERIFY(conc[1].find("因果关系") == std::string::npos);
        QVERIFY(conc[2].find("3.2") != std::string::npos);
        QVERIFY(conc[2].find("A×B") != std::string::npos
                || conc[2].find("A") != std::string::npos);
        QVERIFY(conc[2].find("已证明关联") == std::string::npos);
        QVERIFY(conc[3].find("120") != std::string::npos);
        QVERIFY(conc[3].find("列联表卡方") == std::string::npos);
        QVERIFY(conc[4].find("Poisson") != std::string::npos
                || conc[4].find("poisson") != std::string::npos
                || conc[4].find("1.2") != std::string::npos);
        QVERIFY(conc[4].find("服从泊松") == std::string::npos);
        QVERIFY(conc[5].find("6.0") != std::string::npos);
        QVERIFY(conc[5].find("总体比例等于") == std::string::npos);
        QVERIFY(conc[6].find("McNemar") != std::string::npos);
        QVERIFY(conc[6].find("0.025") != std::string::npos);
        QVERIFY(conc[6].find("已证明相同") == std::string::npos);
        QVERIFY(conc[7].find("Cochran") != std::string::npos);
        QVERIFY(conc[7].find("9.8") != std::string::npos);
        QVERIFY(conc[7].find("阳性率相同") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(lim[0].find("caution") != std::string::npos);
        QVERIFY(lim[0].find("2.5") != std::string::npos);
        QVERIFY(lim[0].find("有效性状态") == std::string::npos);
        QVERIFY(lim[1].find("7") != std::string::npos);
        QVERIFY(lim[1].find("未进入") == std::string::npos);
        QVERIFY(lim[2].find("causal") != std::string::npos
                || lim[2].find("residual") != std::string::npos
                || lim[2].find("Adjusted") != std::string::npos);
        QVERIFY(lim[2].find("不是因果") == std::string::npos);
        QVERIFY(lim[3].find("McNemar") != std::string::npos);
        QVERIFY(lim[3].find("前后比例") == std::string::npos);
        QVERIFY(lim[4].find("Cochran") != std::string::npos);
        QVERIFY(lim[4].find("各处理阳性率") == std::string::npos);
        QVERIFY(lim[5].find("expected") != std::string::npos
                || lim[5].find("chi-square") != std::string::npos
                || lim[5].find("unreliable") != std::string::npos);
        QVERIFY(lim[5].find("期望频数") == std::string::npos);
    }

    void dynamic_ml_multivariate_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "二元 Logistic 已收敛，在 α = 0.05 下未拒绝拟合不足（Hosmer–Lemeshow P = 0.40）。"
            "未拒绝拟合不足不能说明模型已充分；检测到 2 个高杠杆观测；最大 VIF = 3.1。"
            "系数解释依赖事件编码和 complete-case 样本。",
            "PCA 模式为 correlation，保留 3 个主成分，有效观测 50，检测到 4 个 T²/Q 异常观测。"
            "异常阈值只作诊断，T²/Q 超限不是过程合格或失控判定。",
            "K-Means：k = 3，N = 90，变量数 = 4，迭代 = 12（已收敛）；总簇内平方和 ≈ 18.5。"
            "簇标签只描述相对邻近结构，不能写成过程或批次判定。",
            "CART 单树任务 = classification，N = 100，预测变量 = 5，结点 = 11，叶 = 6；"
            "主导分裂变量 = Temp；训练集准确率 ≈ 0.88。训练集指标不能外推为过程合格结论。",
            "ADF（constant）：N = 80，滞后 = 2，有效回归行 = 77；τ ≈ -3.2；5% 临界值 ≈ -2.86；"
            "相对 5% 临界值有拒绝单位根的证据。单位根结论只描述差分平稳性证据，不能外推为工艺判定。",
            "Poisson 回归（log 链）：N = 60，预测变量 = 2，迭代 = 8（已收敛）；偏差 ≈ 45.0；AIC ≈ 70.0。"
            "系数显著性只描述计数均值与预测变量的关联证据。",
            "Isolation Forest：N = 200，变量 = 3，树数 = 100，标记异常 = 10；分数阈值 ≈ 0.62。"
            "分数高只提示相对孤立，与单变量 outlier_test 分流，不能写成工艺判定。",
            "有效观测 N = 25，缺失 N* = 1，均值为 12.3。描述统计不检验分布假设，也不能写成过程合格。",
            "Bootstrap 均值（percentile）：N = 40，B = 1000；均值 ≈ 5.5；CI ≈ [4.1, 6.8]。"
            "区间只描述重抽样不确定性。",
            "层次聚类（complete）：N = 30，k = 4，合并步 = 26；已标准化。"
            "簇标签只描述相对邻近结构。",
            "有序 Logistic（比例优势 logit）：N = 55，水平 = 3，预测变量 = 2；已收敛；AIC ≈ 120.0。"
            "系数描述有序累积对数优势的关联证据。",
            "线性判别：N = 70，类数 = 3，预测变量 = 4；训练准确率 ≈ 0.91。"
            "训练集准确率不能外推为工艺判定。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "模型出现完全分离，极大似然估计可能不存在，不对系数给出稳定解释。",
            "IRLS 未收敛，不对发散系数给出稳定解释。",
            "初始质心取前 k 个观测；结果依赖尺度（已标准化）；非 Minitab golden。",
            "自研二叉 CART；非 Minitab TreeNet/Random Forests 对齐；本轮无成本复杂度剪枝。",
            "临界值为大样本 MacKinnon 风格常数；非 Minitab 导出；宜与 ACF/PACF 一并阅读。",
            "本轮无负二项/零膨胀/偏移列；非 Minitab golden。",
            "自研实现；非 sklearn 运行时；非 TreeNet/RF。",
            "本轮为百分位法，非 BCa。",
            "本轮仅 complete linkage；非 K-Means；非 Minitab golden。",
            "本轮仅 logit 链；非名义 Logistic；非 Minitab golden。",
            "等协方差 LDA；不做 QDA；非 Minitab golden。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("logistic") != std::string::npos
                || conc[0].find("Logistic") != std::string::npos
                || conc[0].find("Hosmer") != std::string::npos);
        QVERIFY(conc[0].find("0.40") != std::string::npos);
        QVERIFY(conc[0].find("3.1") != std::string::npos);
        QVERIFY(conc[0].find("未拒绝拟合不足不能说明") == std::string::npos);
        QVERIFY(conc[1].find("PCA") != std::string::npos);
        QVERIFY(conc[1].find("correlation") != std::string::npos);
        QVERIFY(conc[1].find("过程合格或失控") == std::string::npos);
        QVERIFY(conc[2].find("K-Means") != std::string::npos);
        QVERIFY(conc[2].find("18.5") != std::string::npos);
        QVERIFY(conc[2].find("批次判定") == std::string::npos);
        QVERIFY(conc[3].find("CART") != std::string::npos);
        QVERIFY(conc[3].find("Temp") != std::string::npos);
        QVERIFY(conc[3].find("过程合格结论") == std::string::npos);
        QVERIFY(conc[4].find("ADF") != std::string::npos);
        QVERIFY(conc[4].find("-3.2") != std::string::npos);
        QVERIFY(conc[4].find("工艺判定") == std::string::npos);
        QVERIFY(conc[5].find("Poisson") != std::string::npos);
        QVERIFY(conc[5].find("70.0") != std::string::npos);
        QVERIFY(conc[5].find("关联证据") == std::string::npos);
        QVERIFY(conc[6].find("Isolation") != std::string::npos);
        QVERIFY(conc[6].find("0.62") != std::string::npos);
        QVERIFY(conc[6].find("工艺判定") == std::string::npos);
        QVERIFY(conc[7].find("25") != std::string::npos);
        QVERIFY(conc[7].find("12.3") != std::string::npos);
        QVERIFY(conc[7].find("过程合格") == std::string::npos);
        QVERIFY(conc[8].find("Bootstrap") != std::string::npos);
        QVERIFY(conc[8].find("5.5") != std::string::npos);
        QVERIFY(conc[8].find("重抽样") == std::string::npos);
        QVERIFY(conc[9].find("Hierarchical") != std::string::npos
                || conc[9].find("complete") != std::string::npos);
        QVERIFY(conc[9].find("30") != std::string::npos);
        QVERIFY(conc[9].find("相对邻近") == std::string::npos);
        QVERIFY(conc[10].find("Ordinal") != std::string::npos
                || conc[10].find("ordinal") != std::string::npos
                || conc[10].find("logit") != std::string::npos);
        QVERIFY(conc[10].find("120.0") != std::string::npos);
        QVERIFY(conc[10].find("累积对数优势") == std::string::npos);
        QVERIFY(conc[11].find("discriminant") != std::string::npos
                || conc[11].find("Linear") != std::string::npos
                || conc[11].find("0.91") != std::string::npos);
        QVERIFY(conc[11].find("工艺判定") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(lim[0].find("Complete separation") != std::string::npos
                || lim[0].find("complete separation") != std::string::npos
                || lim[0].find("MLE") != std::string::npos);
        QVERIFY(lim[0].find("完全分离") == std::string::npos);
        QVERIFY(lim[1].find("IRLS") != std::string::npos);
        QVERIFY(lim[1].find("不对发散") == std::string::npos);
        QVERIFY(lim[2].find("standardized") != std::string::npos
                || lim[2].find("centroids") != std::string::npos);
        QVERIFY(lim[2].find("初始质心") == std::string::npos);
        QVERIFY(lim[3].find("CART") != std::string::npos
                || lim[3].find("TreeNet") != std::string::npos);
        QVERIFY(lim[3].find("自研二叉") == std::string::npos);
        QVERIFY(lim[4].find("MacKinnon") != std::string::npos);
        QVERIFY(lim[4].find("临界值为") == std::string::npos);
        QVERIFY(lim[5].find("negative-binomial") != std::string::npos
                || lim[5].find("golden") != std::string::npos);
        QVERIFY(lim[5].find("本轮无负二项") == std::string::npos);
        QVERIFY(lim[6].find("sklearn") != std::string::npos
                || lim[6].find("TreeNet") != std::string::npos);
        QVERIFY(lim[6].find("自研实现") == std::string::npos);
        QVERIFY(lim[7].find("percentile") != std::string::npos
                || lim[7].find("BCa") != std::string::npos);
        QVERIFY(lim[7].find("百分位法") == std::string::npos);
        QVERIFY(lim[8].find("complete linkage") != std::string::npos
                || lim[8].find("K-Means") != std::string::npos);
        QVERIFY(lim[8].find("本轮仅 complete") == std::string::npos);
        QVERIFY(lim[9].find("logit") != std::string::npos
                || lim[9].find("nominal") != std::string::npos);
        QVERIFY(lim[9].find("名义 Logistic") == std::string::npos);
        QVERIFY(lim[10].find("LDA") != std::string::npos
                || lim[10].find("QDA") != std::string::npos);
        QVERIFY(lim[10].find("等协方差") == std::string::npos);
    }

    void dynamic_eda_graph_turnbull_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "密度图：N = 120；Silverman 带宽 h = 0.35。"
            "曲线是平滑估计，不能据此写成正态性结论。",
            "二维分箱：N = 200；格 = 20×15（矩形格，产品名 Hexbin）。",
            "小提琴图：组数 = 4；形状来自分组 KDE，箱线为五数摘要。",
            "通用条形图：类别数 = 8；未按计数排序，无累积百分比（与柏拉图分流）。",
            "NIST EDA 四图：N = 64；同页包含 run sequence、lag-1、histogram、normal probability。"
            "四图用于检查位置/散布/随机性/分布形态假设，不能写成受控结论或正态已成立。",
            "受控分面：面板 = 6/9（max=8）；已截断 3 个水平。"
            "分面是多面板编排，不是自由拼版；by/分组仍是图内着色。",
            "因果图效应为“泄漏”，共 6 个类别、18 条原因。"
            "这是结构化头脑风暴摘要，不是统计检验，不能写成已证明根因。",
            "区间删失 KM（Turnbull）：N = 40，左/区间/右 = 2/5/8；中位寿命 ≈ 100.5。",
            "ACF/PACF 基于 N = 80，最大滞后 = 20；默认置信带方法 = nist_white_noise，半宽 ≈ 0.219。"
            "越过带宽只提示相对白噪声零假设的相关证据，不能写成过程失控判定。",
            "Ljung–Box P ≈ 0.03；小 P 提示序列存在总体自相关结构，应结合 ACF/PACF 图与工艺时序解释。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "EDA 图用于探索形态与频数，不是假设检验结论。",
            "Turnbull NPMLE（简化网格）；evidence_type=formula_reference；"
            "algorithm_id=turnbull_npmle_simplified_grid；非右删失 product-limit；"
            "非参数寿命模型；不得写成 vendor_oracle/golden。",
            "本批观测均为精确失效（无左/区间/右删失）；仍走区间 NPMLE 路径，"
            "不自动等价替换为经典右删失 KM 报告。",
            "默认带宽是 NIST 白噪声固定 ±z/√n（独立性检验），"
            "不是 ARIMA 识别用的 Bartlett 变带宽；亦非 Minitab golden。",
            "若要比较原因频次优先级，请改用柏拉图并结合现场验证。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("Density") != std::string::npos
                || conc[0].find("density") != std::string::npos
                || conc[0].find("Silverman") != std::string::npos);
        QVERIFY(conc[0].find("0.35") != std::string::npos);
        QVERIFY(conc[0].find("正态性结论") == std::string::npos);
        QVERIFY(conc[1].find("200") != std::string::npos);
        QVERIFY(conc[1].find("Hexbin") != std::string::npos);
        QVERIFY(conc[1].find("二维分箱") == std::string::npos);
        QVERIFY(conc[2].find("4") != std::string::npos);
        QVERIFY(conc[2].find("小提琴") == std::string::npos);
        QVERIFY(conc[3].find("8") != std::string::npos);
        QVERIFY(conc[3].find("柏拉图") == std::string::npos);
        QVERIFY(conc[4].find("NIST") != std::string::npos);
        QVERIFY(conc[4].find("64") != std::string::npos);
        QVERIFY(conc[4].find("受控结论") == std::string::npos);
        QVERIFY(conc[5].find("6") != std::string::npos);
        QVERIFY(conc[5].find("自由拼版") == std::string::npos);
        QVERIFY(conc[6].find("泄漏") != std::string::npos
                || conc[6].find("18") != std::string::npos);
        QVERIFY(conc[6].find("已证明根因") == std::string::npos);
        QVERIFY(conc[7].find("Turnbull") != std::string::npos);
        QVERIFY(conc[7].find("100.5") != std::string::npos);
        QVERIFY(conc[7].find("区间删失") == std::string::npos);
        QVERIFY(conc[8].find("ACF") != std::string::npos);
        QVERIFY(conc[8].find("0.219") != std::string::npos);
        QVERIFY(conc[8].find("过程失控判定") == std::string::npos);
        QVERIFY(conc[9].find("0.03") != std::string::npos);
        QVERIFY(conc[9].find("工艺时序") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(lim[0].find("hypothesis") != std::string::npos
                || lim[0].find("EDA") != std::string::npos);
        QVERIFY(lim[0].find("假设检验结论") == std::string::npos);
        QVERIFY(lim[1].find("vendor_oracle") != std::string::npos);
        QVERIFY(lim[1].find("formula_reference") != std::string::npos);
        QVERIFY(lim[1].find("不得写成") == std::string::npos);
        QVERIFY(lim[2].find("NPMLE") != std::string::npos
                || lim[2].find("exact") != std::string::npos
                || lim[2].find("classic") != std::string::npos);
        QVERIFY(lim[2].find("精确失效") == std::string::npos);
        QVERIFY(lim[3].find("NIST") != std::string::npos
                || lim[3].find("Bartlett") != std::string::npos);
        QVERIFY(lim[3].find("独立性检验") == std::string::npos);
        QVERIFY(lim[4].find("Pareto") != std::string::npos
                || lim[4].find("field") != std::string::npos);
        QVERIFY(lim[4].find("柏拉图") == std::string::npos);
    }

    void dynamic_spc_doe_design_warranty_strata_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "Z-MR 图 N = 48，组数 = 6，Z 图超限 = 2；短流程标准化依赖 样本估计 μ/σ，"
            "超限点需要调查，不能直接当作过程失控或稳定结论。",
            "移动平均图窗宽 w = 5，「单点超出 3σ 控制限」触发 = 1；MA 平滑后的出限是信号提示，"
            "需要结合工艺调查，不能将统计信号直接等同于过程失控判定或稳定结论。",
            "运行图 N = 30，中位数 = 10.5；关于中位数游程 = 8，上升/下降游程 = 7。"
            "四模式 P 只提示相对随机性的偏离方向，不能写成已证明过程受控或失控。",
            "区域图 Jaehn 累计得分达到阈值 8 的点有 3 个；这是统计信号提示，需要调查特殊原因，"
            "不能将统计信号直接等同于过程失控判定或稳定结论。",
            "Plackett–Burman：因子 = 7，运行 = 12，中心点 = 2。设计矩阵仅供筛选实验排布。",
            "Box–Behnken：因子 = 3，边中点 = 12，中心点 = 3，总运行 = 15。"
            "不包含所有因子同时极端的角点。",
            "CCD（circumscribed）：因子 = 3，立方点 = 8，星点 = 6，中心点 = 6，α = 1.682。",
            "已按 product 追溯 4 个分层分母；部分/全部分层 expected_failures 使用 "
            "cause-specific 分模式 R(T_w)。",
            "已按 plant 追溯 2 个分层分母；分层 expected_failures 使用池化 R(T_w)。",
            "CCF：N = 100，max|lag| = 20；lag0 ≈ 0.42。"
            "越过带宽只提示相对独立性零假设的相关证据。",
            "Correlogram（pearson）：变量数 = 5，成对数 = 10。热图只展示两两相关结构。",
            "逐步回归（forward）：N = 60，候选 = 8，选入 = 3；R² ≈ 0.81。"
            "选入项只描述相对拟合证据。",
            "Multi-Vari 图用 2 个因子分层显示 40 个有效测量的均值（A、B）。",
            "变异性图按 2 个因子汇总 6 个单元、40 个有效测量的均值与标准差，总均值 = 12.3，"
            "单元 StDev 平均 = 1.1。图只用于探索变异结构，不是显著性检验或过程判定。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection advice;
        advice.heading = "行动建议";
        advice.bullets = {
            "对照 P clustering / mixtures / trends / oscillation 与现场时序，"
            "小 P 只表示该模式方向上的偏离证据。",
            "结合因子均值表回查占主导的因子水平；需要检验显著性时使用 ANOVA 或回归。",
            "先看均值面板中极差较大的单元，再对照标准差面板定位离散度来源。"};
        page.interpretation.push_back(advice);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "未提供完整历史 μ/σ 表；本轮用各组样本均值与全序列 MR/d2 估计 σ。",
            "移动平均图与 EWMA 不同；仅完整窗 MA 点参与判读，不是 Minitab golden。",
            "区域图采用 Jaehn 1/2/4 计分（formula_reference），"
            "不是完整 Shewhart 特殊原因规则（beyond_control_limit 等）的替代品。",
            "分辨率 III；非 CCD/BBD；非 Minitab golden。",
            "证据类型 formula_reference；未宣称全域预测最优；非 vendor_oracle。",
            "证据类型 formula_reference；非 vendor_oracle / 商业软件对齐。",
            "分模式 R 为 cause-specific（竞争失效作右删失）formula_reference，"
            "不是 vendor_oracle；不得写成商业软件对齐。",
            "分层未估计分模式可靠度（uses_pooled_reliability=true）；"
            "比例分摊暴露量须标注 proportional_scalar。",
            "与 correlation_plot 分流；非 Graph Builder。",
            "α 逐步；非 Best subsets；选择不稳定时勿外推。",
            "已跳过 3 个缺失或不完整行。",
            "变异性图与 Multi-Vari 语义不同：本命令输出均值+极差与标准差双面板，不替代 ANOVA。",
            "分层均值图只用于探索变异来源，不能把结果写成过程判定。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("Z-MR") != std::string::npos);
        QVERIFY(conc[0].find("48") != std::string::npos);
        QVERIFY(conc[0].find("sample") != std::string::npos
                || conc[0].find("μ/σ") != std::string::npos);
        QVERIFY(conc[0].find("样本估计") == std::string::npos);
        QVERIFY(conc[1].find("5") != std::string::npos);
        QVERIFY(conc[1].find("Moving") != std::string::npos
                || conc[1].find("moving") != std::string::npos
                || conc[1].find("window") != std::string::npos);
        QVERIFY(conc[1].find("移动平均") == std::string::npos);
        QVERIFY(conc[2].find("30") != std::string::npos);
        QVERIFY(conc[2].find("10.5") != std::string::npos);
        QVERIFY(conc[2].find("中位数") == std::string::npos);
        QVERIFY(conc[3].find("Jaehn") != std::string::npos);
        QVERIFY(conc[3].find("3") != std::string::npos);
        QVERIFY(conc[3].find("区域图") == std::string::npos);
        QVERIFY(conc[4].find("Plackett") != std::string::npos);
        QVERIFY(conc[4].find("7") != std::string::npos);
        QVERIFY(conc[4].find("筛选实验") == std::string::npos);
        QVERIFY(conc[5].find("Box") != std::string::npos
                || conc[5].find("Behnken") != std::string::npos);
        QVERIFY(conc[5].find("角点") == std::string::npos);
        QVERIFY(conc[6].find("CCD") != std::string::npos);
        QVERIFY(conc[6].find("1.682") != std::string::npos);
        QVERIFY(conc[6].find("立方点") == std::string::npos);
        QVERIFY(conc[7].find("product") != std::string::npos);
        QVERIFY(conc[7].find("4") != std::string::npos);
        QVERIFY(conc[7].find("分层分母") == std::string::npos);
        QVERIFY(conc[8].find("plant") != std::string::npos);
        QVERIFY(conc[8].find("pooled") != std::string::npos
                || conc[8].find("2") != std::string::npos);
        QVERIFY(conc[8].find("池化") == std::string::npos);
        QVERIFY(conc[9].find("CCF") != std::string::npos);
        QVERIFY(conc[9].find("0.42") != std::string::npos);
        QVERIFY(conc[9].find("独立性") == std::string::npos);
        QVERIFY(conc[10].find("Correlogram") != std::string::npos);
        QVERIFY(conc[10].find("5") != std::string::npos);
        QVERIFY(conc[10].find("热图") == std::string::npos);
        QVERIFY(conc[11].find("Stepwise") != std::string::npos
                || conc[11].find("stepwise") != std::string::npos
                || conc[11].find("forward") != std::string::npos);
        QVERIFY(conc[11].find("0.81") != std::string::npos);
        QVERIFY(conc[11].find("选入项") == std::string::npos);
        QVERIFY(conc[12].find("Multi-Vari") != std::string::npos
                || conc[12].find("40") != std::string::npos);
        QVERIFY(conc[12].find("分层显示") == std::string::npos);
        QVERIFY(conc[13].find("12.3") != std::string::npos);
        QVERIFY(conc[13].find("1.1") != std::string::npos);
        QVERIFY(conc[13].find("变异性图") == std::string::npos);
        const auto& adv = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(adv[0].find("clustering") != std::string::npos);
        QVERIFY(adv[0].find("现场时序") == std::string::npos);
        QVERIFY(adv[1].find("ANOVA") != std::string::npos
                || adv[1].find("factor") != std::string::npos);
        QVERIFY(adv[1].find("因子均值表") == std::string::npos);
        QVERIFY(adv[2].find("SD") != std::string::npos
                || adv[2].find("dispersion") != std::string::npos
                || adv[2].find("mean") != std::string::npos);
        QVERIFY(adv[2].find("离散度来源") == std::string::npos);
        const auto& lim2 = localized.document.pages[0].visible_interpretation[2].bullets;
        QVERIFY(lim2[0].find("historical") != std::string::npos
                || lim2[0].find("MR/d2") != std::string::npos);
        QVERIFY(lim2[0].find("未提供") == std::string::npos);
        QVERIFY(lim2[1].find("EWMA") != std::string::npos);
        QVERIFY(lim2[1].find("移动平均") == std::string::npos);
        QVERIFY(lim2[2].find("Jaehn") != std::string::npos);
        QVERIFY(lim2[2].find("替代品") == std::string::npos);
        QVERIFY(lim2[3].find("Resolution") != std::string::npos
                || lim2[3].find("III") != std::string::npos);
        QVERIFY(lim2[3].find("分辨率") == std::string::npos);
        QVERIFY(lim2[4].find("formula_reference") != std::string::npos);
        QVERIFY(lim2[4].find("未宣称") == std::string::npos);
        QVERIFY(lim2[5].find("vendor_oracle") != std::string::npos);
        QVERIFY(lim2[5].find("商业软件对齐") == std::string::npos);
        QVERIFY(lim2[6].find("cause-specific") != std::string::npos);
        QVERIFY(lim2[6].find("不得写成") == std::string::npos);
        QVERIFY(lim2[7].find("uses_pooled_reliability=true") != std::string::npos
                || lim2[7].find("proportional_scalar") != std::string::npos);
        QVERIFY(lim2[7].find("分层未估计") == std::string::npos);
        QVERIFY(lim2[8].find("correlation_plot") != std::string::npos
                || lim2[8].find("Graph Builder") != std::string::npos);
        QVERIFY(lim2[8].find("分流") == std::string::npos);
        QVERIFY(lim2[9].find("Best subsets") != std::string::npos
                || lim2[9].find("stepwise") != std::string::npos
                || lim2[9].find("Stepwise") != std::string::npos);
        QVERIFY(lim2[9].find("勿外推") == std::string::npos);
        QVERIFY(lim2[10].find("3") != std::string::npos);
        QVERIFY(lim2[10].find("已跳过") == std::string::npos);
        QVERIFY(lim2[11].find("ANOVA") != std::string::npos
                || lim2[11].find("Multi-Vari") != std::string::npos);
        QVERIFY(lim2[11].find("双面板") == std::string::npos);
        QVERIFY(lim2[12].find("process") != std::string::npos
                || lim2[12].find("explore") != std::string::npos
                || lim2[12].find("means") != std::string::npos);
        QVERIFY(lim2[12].find("过程判定") == std::string::npos);
    }

    void dynamic_tolerance_distrib_variance_capability_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "容差区间方法为 Howe（normal），覆盖率 0.95，置信水平 0.95，"
            "achieved confidence 0.94，下限 1.2，上限 9.8。"
            "区间只描述当前样本覆盖，不是规格覆盖或过程判定。",
            "个体分布识别按 Anderson-Darling 升序比较四族二参数分布；当前表内最优为 "
            "Weibull（AD = 0.42）。排序结果不证明数据服从该分布，也不自动改写过程能力默认方法。",
            "等方差检验方法为 Bonett，P = 0.21。"
            "未拒绝原假设不能证明方差相等。"
            "Bonett 为连续分布下的标准差比路径，与中位数 Levene 不同；"
            "极偏/重尾小样本时宜对照 Levene。",
            "Cpk = 1.10，低于项目提示基准 1.33，需要调查过程能力。",
            "Cpk = 1.45，达到项目提示基准 1.33；这不是已验证的过程合格结论。",
            "Ppk = 1.05，整体过程表现低于 1.33 提示基准。",
            "Ppk = 1.40，整体过程表现达到 1.33 提示基准。",
            "Cpk 置信区间 [1.01, 1.29]（Bissell 近似，公式参考）。"
            "区间描述抽样不确定性，不是合格判定。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection advice;
        advice.heading = "行动建议";
        advice.bullets = {
            "结合直方图与方法族诊断调查覆盖区间；需要对照规格时使用过程能力分析。"};
        page.interpretation.push_back(advice);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "非参数容差依赖序统计与连续分布假设；achieved confidence 低于目标时，"
            "只说明样本量不足，不是计算错误。",
            "本方法假设测量近似正态且未在本页验证（assumption_status=not_verified）；"
            "单侧使用 Natrella 近似，双侧使用 Howe 近似。",
            "已跳过 2 个缺失或非法数值。",
            "过程能力未验证稳定性与正态性（assumption_status=not_verified），"
            "不能把 Cpk/Ppk 直接写成合格判定。",
            "Anderson-Darling P = 0.18；未拒绝正态假设不等于已证明正态分布。",
            "Johnson 变换后的 Pp/Ppk 是变换尺度上的 overall 指数；"
            "未拒绝变换后正态假设不等于原始数据服从正态分布，也不能写成过程合格。",
            "非正态 Z-score Pp/Ppk 依赖所选分布的 CDF；拟合未拒绝假设不等于已证明"
            "过程服从该分布，也不能写成过程合格。",
            "Johnson 变换 AD P = 0.55；这是变换尺度上的拟合证据，不是原始数据正态性证明。",
            "组间/组内能力使用 σ_BW 计算 Cp/Cpk、样本标准差计算 Pp/Ppk；"
            "未验证稳定性不等于过程合格。",
            "gate_status=research_preview：当前为研究/预览，证据类型 formula_reference；"
            "未满足 golden/尾部验收前不得开放合格判定。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("Tolerance") != std::string::npos
                || conc[0].find("tolerance") != std::string::npos
                || conc[0].find("Howe") != std::string::npos);
        QVERIFY(conc[0].find("9.8") != std::string::npos);
        QVERIFY(conc[0].find("容差区间") == std::string::npos);
        QVERIFY(conc[1].find("Anderson-Darling") != std::string::npos);
        QVERIFY(conc[1].find("Weibull") != std::string::npos);
        QVERIFY(conc[1].find("0.42") != std::string::npos);
        QVERIFY(conc[1].find("个体分布") == std::string::npos);
        QVERIFY(conc[2].find("Bonett") != std::string::npos);
        QVERIFY(conc[2].find("0.21") != std::string::npos);
        QVERIFY(conc[2].find("等方差") == std::string::npos);
        QVERIFY(conc[3].find("1.10") != std::string::npos);
        QVERIFY(conc[3].find("低于项目") == std::string::npos);
        QVERIFY(conc[4].find("1.45") != std::string::npos);
        QVERIFY(conc[4].find("已验证的过程合格") == std::string::npos);
        QVERIFY(conc[5].find("1.05") != std::string::npos);
        QVERIFY(conc[5].find("整体过程表现") == std::string::npos);
        QVERIFY(conc[6].find("1.40") != std::string::npos);
        QVERIFY(conc[6].find("提示基准") == std::string::npos);
        QVERIFY(conc[7].find("1.01") != std::string::npos);
        QVERIFY(conc[7].find("Bissell") != std::string::npos);
        QVERIFY(conc[7].find("合格判定") == std::string::npos);
        const auto& adv = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(adv[0].find("capability") != std::string::npos
                || adv[0].find("histogram") != std::string::npos
                || adv[0].find("specification") != std::string::npos);
        QVERIFY(adv[0].find("过程能力分析") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[2].bullets;
        QVERIFY(lim[0].find("Nonparametric") != std::string::npos
                || lim[0].find("achieved confidence") != std::string::npos);
        QVERIFY(lim[0].find("非参数容差") == std::string::npos);
        QVERIFY(lim[1].find("Natrella") != std::string::npos);
        QVERIFY(lim[1].find("Howe") != std::string::npos);
        QVERIFY(lim[1].find("本方法假设") == std::string::npos);
        QVERIFY(lim[2].find("2") != std::string::npos);
        QVERIFY(lim[2].find("已跳过") == std::string::npos);
        QVERIFY(lim[3].find("assumption_status=not_verified") != std::string::npos);
        QVERIFY(lim[3].find("不能把") == std::string::npos);
        QVERIFY(lim[4].find("0.18") != std::string::npos);
        QVERIFY(lim[4].find("已证明正态分布") == std::string::npos);
        QVERIFY(lim[5].find("Johnson") != std::string::npos);
        QVERIFY(lim[5].find("过程合格") == std::string::npos);
        QVERIFY(lim[6].find("Z-score") != std::string::npos
                || lim[6].find("CDF") != std::string::npos);
        QVERIFY(lim[6].find("不能写成") == std::string::npos);
        QVERIFY(lim[7].find("0.55") != std::string::npos);
        QVERIFY(lim[7].find("原始数据正态性") == std::string::npos);
        QVERIFY(lim[8].find("σ_BW") != std::string::npos
                || lim[8].find("Between") != std::string::npos
                || lim[8].find("between") != std::string::npos);
        QVERIFY(lim[8].find("组间") == std::string::npos);
        QVERIFY(lim[9].find("research") != std::string::npos
                || lim[9].find("preview") != std::string::npos
                || lim[9].find("formula_reference") != std::string::npos);
        QVERIFY(lim[9].find("不得开放") == std::string::npos);
    }

    void dynamic_pareto_and_capability_multi_screen_gate_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "最大类别为“泄漏”，计数 42，单项占比 35.5%。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection advice;
        advice.heading = "行动建议";
        advice.bullets = {
            "前 2 个类别累计占比 61.0%；应优先结合现场原因验证，而不是直接假设存在 80/20 规律。"};
        page.interpretation.push_back(advice);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "gate_status=blocked_stability；stability_screen=signals（OOC=3）；"
            "bimodality_screen=clear（peaks=1）；hartigan_dip=consistent（D=0.02）；"
            "mixture=not_preferred（k=1，ΔBIC=0.0）：I-MR Rule-1 / 直方图双峰 / "
            "Hartigan dip / 高斯混合门禁筛查不等于完整稳定性或单峰验收。",
            "二维高斯混合为 preferred_2comp：单一分布能力指数仅供调查，"
            "不得写成过程合格（formula_reference / gaussian_mixture_k_bic；非 vendor_oracle）。",
            "Hartigan dip 为 evidence_against：单一分布能力指数仅供调查，"
            "不得写成过程合格（formula_reference；非 vendor_oracle）。",
            "双峰初筛为 suspected：单一分布能力指数仅供调查，"
            "不得写成过程合格（直方图初筛 ≠ 混合模型证明）。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("泄漏") != std::string::npos
                || conc[0].find("Largest") != std::string::npos);
        QVERIFY(conc[0].find("42") != std::string::npos);
        QVERIFY(conc[0].find("35.5") != std::string::npos);
        QVERIFY(conc[0].find("最大类别") == std::string::npos);
        const auto& adv = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(adv[0].find("61.0") != std::string::npos);
        QVERIFY(adv[0].find("80/20") != std::string::npos);
        QVERIFY(adv[0].find("累计占比") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[2].bullets;
        QVERIFY(lim[0].find("blocked_stability") != std::string::npos);
        QVERIFY(lim[0].find("OOC=3") != std::string::npos);
        QVERIFY(lim[0].find("ΔBIC=0.0") != std::string::npos
                || lim[0].find("0.0") != std::string::npos);
        QVERIFY(lim[0].find("直方图双峰") == std::string::npos);
        QVERIFY(lim[0].find("单峰验收") == std::string::npos);
        QVERIFY(lim[1].find("preferred_2comp") != std::string::npos);
        QVERIFY(lim[1].find("不得写成") == std::string::npos);
        QVERIFY(lim[2].find("evidence_against") != std::string::npos);
        QVERIFY(lim[2].find("不得写成") == std::string::npos);
        QVERIFY(lim[3].find("suspected") != std::string::npos);
        QVERIFY(lim[3].find("混合模型证明") == std::string::npos);
    }

    void dynamic_forecast_attr_spc_honesty_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "Grouping Information 使用 4 个字母；字母来自成对显著矩阵，不改 Studentized Range 近似。",
            "属性一致性报告观察一致率与 Kappa；≥3 名评估者使用 Fleiss，两两比较仍用 Cohen。",
            "有序评级已计算 Kendall 系数；拒绝 W=0 或 τ=0 不等于已证明有序一致。",
            "Durbin-Watson = 1.82，判定区 = inconclusive（按输入顺序与 α=0.05 近似 dL/dU）。"
            "不能写成已证明无自相关或存在自相关。",
            "I-MR σ 估计方法 = average_moving_range；Nelson estimate 剔除过大 MR 2 个。"
            "σ 估计变化只影响控制限宽度，不能单独写成稳定结论。",
            "控制限使用了历史 μ/σ；分阶段估计表仅供对照，不会自动改写全局限。",
            "已按阶段列汇总估计（阶段数 = 3）；当前输出仍使用全样本估计限，不是逐阶段独立控制图。",
            "Hotelling T²：m = 40，p = 3，超限 = 2。"
            "超限是统计信号提示，需要结合工艺调查，不能单独写成失控或稳定结论。"
            " 本图不是 PCA 经验分位 T²。",
            "MEWMA：m = 30，p = 4，超限 = 0。"
            "超限是统计信号提示，需要结合工艺调查，不能单独写成失控或稳定结论。"
            " 默认 UCL 可能是渐近近似，不是 ARL 仿真校准。",
            "广义方差 |S|：m = 20，p = 2，子组数 = 10，超限 = 1。"
            "超限是统计信号提示，需要结合工艺调查，不能单独写成失控或稳定结论。"
            " Montgomery |S| 子组图；个体路径与 NIST 争议说明见诊断。",
            "Laney P' Sigma Z = 1.35，存在过度离散，传统控制限可能过窄。",
            "当前 Sigma Z = 0.90，控制限已按离散程度进行调整。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection advice;
        advice.heading = "工程建议";
        advice.bullets = {
            "预测区间反映模型不确定性，不是规格上下限；应通过滚动验证检查不同预测期的稳定性。",
            "乘法分解要求观测值为正；季节指数与趋势外推对结构变化敏感，应核对残差与移动平均边界。",
            "加法分解的季节指数与线性趋势外推对结构变化敏感；应核对残差与非等间隔时间诊断。",
            "未来预测依赖平滑/ARIMA 模型假设；应检查残差、自相关和结构变化，避免外推超出历史范围。",
            "区间图是各组均值的个体置信区间（pooled MSE），不是 Tukey 同时比较。",
            "名义评级看 Kappa；有序评级请设置 ordinal=true 以计算 Kendall W/τ。"};
        page.interpretation.push_back(advice);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "残差正态或方差齐性检查提供了需要调查的证据，不能把 p 值直接写成工程差异。",
            "拒绝 Kappa=0 不等于已证明评估者一致；Kappa 只描述超出偶然的绝对一致率。",
            "已请求有序评级，但 Kendall 不可识别或等级不足；未伪造 W=1。",
            "属性一致性缺少 Facts，不能从表头反解析 Kappa 或 Kendall。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("4") != std::string::npos);
        QVERIFY(conc[0].find("个字母") == std::string::npos);
        QVERIFY(conc[1].find("Kappa") != std::string::npos);
        QVERIFY(conc[1].find("属性一致性") == std::string::npos);
        QVERIFY(conc[2].find("Kendall") != std::string::npos);
        QVERIFY(conc[2].find("有序一致") == std::string::npos);
        QVERIFY(conc[3].find("1.82") != std::string::npos);
        QVERIFY(conc[3].find("inconclusive") != std::string::npos);
        QVERIFY(conc[3].find("已证明无自相关") == std::string::npos);
        QVERIFY(conc[4].find("average_moving_range") != std::string::npos);
        QVERIFY(conc[4].find("2") != std::string::npos);
        QVERIFY(conc[4].find("excluded") != std::string::npos);
        QVERIFY(conc[4].find("个") == std::string::npos);
        QVERIFY(conc[4].find("稳定结论") == std::string::npos);
        QVERIFY(conc[5].find("historical") != std::string::npos
                || conc[5].find("μ/σ") != std::string::npos);
        QVERIFY(conc[5].find("不会自动改写") == std::string::npos);
        QVERIFY(conc[6].find("3") != std::string::npos);
        QVERIFY(conc[6].find("逐阶段") == std::string::npos);
        QVERIFY(conc[7].find("Hotelling") != std::string::npos);
        QVERIFY(conc[7].find("40") != std::string::npos);
        QVERIFY(conc[7].find("PCA") != std::string::npos);
        QVERIFY(conc[7].find("本图不是") == std::string::npos);
        QVERIFY(conc[8].find("MEWMA") != std::string::npos);
        QVERIFY(conc[8].find("30") != std::string::npos);
        QVERIFY(conc[8].find("ARL") != std::string::npos);
        QVERIFY(conc[8].find("默认 UCL") == std::string::npos);
        QVERIFY(conc[9].find("Generalized") != std::string::npos
                || conc[9].find("|S|") != std::string::npos);
        QVERIFY(conc[9].find("Montgomery") != std::string::npos);
        QVERIFY(conc[9].find("广义方差") == std::string::npos);
        QVERIFY(conc[10].find("1.35") != std::string::npos);
        QVERIFY(conc[10].find("overdispersion") != std::string::npos
                || conc[10].find("narrow") != std::string::npos);
        QVERIFY(conc[10].find("过度离散") == std::string::npos);
        QVERIFY(conc[11].find("0.90") != std::string::npos);
        QVERIFY(conc[11].find("Current") != std::string::npos);
        QVERIFY(conc[11].find("adjusted") != std::string::npos);
        QVERIFY(conc[11].find("当前") == std::string::npos);
        QVERIFY(conc[11].find("离散程度") == std::string::npos);
        const auto& adv = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(adv[0].find("Prediction") != std::string::npos
                || adv[0].find("specification") != std::string::npos);
        QVERIFY(adv[0].find("规格上下限") == std::string::npos);
        QVERIFY(adv[1].find("Multiplicative") != std::string::npos
                || adv[1].find("positive") != std::string::npos);
        QVERIFY(adv[1].find("乘法分解") == std::string::npos);
        QVERIFY(adv[2].find("Additive") != std::string::npos
                || adv[2].find("additive") != std::string::npos);
        QVERIFY(adv[2].find("加法分解") == std::string::npos);
        QVERIFY(adv[3].find("ARIMA") != std::string::npos);
        QVERIFY(adv[3].find("外推超出") == std::string::npos);
        QVERIFY(adv[4].find("Tukey") != std::string::npos);
        QVERIFY(adv[4].find("区间图") == std::string::npos);
        QVERIFY(adv[5].find("ordinal=true") != std::string::npos);
        QVERIFY(adv[5].find("名义评级") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[2].bullets;
        QVERIFY(lim[0].find("Residual") != std::string::npos
                || lim[0].find("engineering") != std::string::npos);
        QVERIFY(lim[0].find("工程差异") == std::string::npos);
        QVERIFY(lim[1].find("Kappa=0") != std::string::npos
                || lim[1].find("chance") != std::string::npos);
        QVERIFY(lim[1].find("已证明评估者") == std::string::npos);
        QVERIFY(lim[2].find("W=1") != std::string::npos);
        QVERIFY(lim[2].find("未伪造") == std::string::npos);
        QVERIFY(lim[3].find("Facts") != std::string::npos);
        QVERIFY(lim[3].find("反解析") == std::string::npos);
    }

    void dynamic_reliability_life_models_and_doe_opt_interval_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "Weibull 形状参数 β = 2.5，提示失效率随时间上升。",
            "Weibull 形状参数 β = 0.7，提示早期失效型失效率随时间下降。",
            "Weibull 形状参数 β = 1.0，接近恒定失效率。",
            "阈值 λ = 10.5；百分位寿命按 t_p = λ + α[-ln(1-p)]^(1/β) 计算。",
            "两参数指数估计了阈值 λ = 5.0 后的恒定失效率。",
            "三参数对数正态估计了阈值 λ = 2.2；分位寿命按 λ + exp(μ + σ Φ⁻¹(p)) 计算。",
            "对数正态模型估计了右删失下的位置/尺度参数；分位寿命由 exp(μ + σ Φ⁻¹(p)) 给出。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "缺协方差或残差自由度不足，置信/预测区间不可用；不要仅凭点预测确定最优设置。",
            "三参数拟合未拒绝假设不等于已证明寿命服从三参数 Weibull。",
            "指数拟合未拒绝假设不等于已证明寿命服从指数分布。",
            "对数正态拟合未拒绝假设并不等于已证明寿命服从对数正态。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("2.5") != std::string::npos);
        QVERIFY(conc[0].find("rises") != std::string::npos
                || conc[0].find("rise") != std::string::npos);
        QVERIFY(conc[0].find("失效率随时间上升") == std::string::npos);
        QVERIFY(conc[1].find("0.7") != std::string::npos);
        QVERIFY(conc[1].find("early") != std::string::npos
                || conc[1].find("decreases") != std::string::npos);
        QVERIFY(conc[1].find("早期失效") == std::string::npos);
        QVERIFY(conc[2].find("1.0") != std::string::npos);
        QVERIFY(conc[2].find("constant") != std::string::npos);
        QVERIFY(conc[2].find("接近恒定") == std::string::npos);
        QVERIFY(conc[3].find("10.5") != std::string::npos);
        QVERIFY(conc[3].find("Threshold") != std::string::npos
                || conc[3].find("t_p") != std::string::npos);
        QVERIFY(conc[3].find("百分位寿命") == std::string::npos);
        QVERIFY(conc[4].find("5.0") != std::string::npos);
        QVERIFY(conc[4].find("Two-parameter") != std::string::npos
                || conc[4].find("exponential") != std::string::npos);
        QVERIFY(conc[4].find("两参数指数") == std::string::npos);
        QVERIFY(conc[5].find("2.2") != std::string::npos);
        QVERIFY(conc[5].find("Three-parameter") != std::string::npos
                || conc[5].find("lognormal") != std::string::npos);
        QVERIFY(conc[5].find("三参数对数正态") == std::string::npos);
        QVERIFY(conc[6].find("Φ⁻¹") != std::string::npos
                || conc[6].find("percentile") != std::string::npos
                || conc[6].find("location") != std::string::npos);
        QVERIFY(conc[6].find("右删失下的位置") == std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(lim[0].find("Covariance") != std::string::npos
                || lim[0].find("prediction") != std::string::npos
                || lim[0].find("unavailable") != std::string::npos);
        QVERIFY(lim[0].find("缺协方差") == std::string::npos);
        QVERIFY(lim[0].find("点预测") == std::string::npos);
        QVERIFY(lim[1].find("three-parameter") != std::string::npos
                || lim[1].find("Weibull") != std::string::npos);
        QVERIFY(lim[1].find("不等于已证明") == std::string::npos);
        QVERIFY(lim[2].find("exponential") != std::string::npos);
        QVERIFY(lim[2].find("不等于已证明") == std::string::npos);
        QVERIFY(lim[3].find("lognormal") != std::string::npos);
        QVERIFY(lim[3].find("并不等于") == std::string::npos);
    }

    void dynamic_km_warranty_evidence_interp_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection conclusion;
        conclusion.heading = "统计结论";
        conclusion.bullets = {
            "Kaplan-Meier 曲线基于 12 个时间点；删失数合计 3。",
            "保修摘要：T_w = 365 day，暴露量 = 1000，claims/1000 = 12.5（口径 = units）。"};
        page.interpretation.push_back(conclusion);
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {"证据类型 formula_reference；非 vendor_oracle。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& conc = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(conc[0].find("12") != std::string::npos);
        QVERIFY(conc[0].find("3") != std::string::npos);
        QVERIFY(conc[0].find("Kaplan-Meier") != std::string::npos);
        QVERIFY(conc[0].find("时间点") == std::string::npos);
        QVERIFY(conc[1].find("365") != std::string::npos);
        QVERIFY(conc[1].find("1000") != std::string::npos);
        QVERIFY(conc[1].find("12.5") != std::string::npos);
        QVERIFY(conc[1].find("Warranty summary") != std::string::npos);
        const auto& lim = localized.document.pages[0].visible_interpretation[1].bullets;
        QVERIFY(lim[0].find("formula_reference") != std::string::npos);
        QVERIFY(lim[0].find("Evidence type") != std::string::npos);
        QVERIFY(lim[0].find("证据类型") == std::string::npos);
    }

    void dynamic_error_limitation_weibull_prefix_templates_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：样本量不足。",
            "分析限制：存在大量删失。",
            "Weibull 形状参数 β = 2.5。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);
        const auto& lim = localized.document.pages[0].visible_interpretation[0].bullets;
        QVERIFY(lim[0].find("Analysis error") != std::string::npos);
        QVERIFY(lim[0].find("样本量不足") != std::string::npos);
        QVERIFY(lim[1].find("Analysis limitation") != std::string::npos);
        QVERIFY(lim[1].find("存在大量删失") != std::string::npos);
        QVERIFY(lim[2].find("Weibull shape") != std::string::npos);
        QVERIFY(lim[2].find("2.5") != std::string::npos);
        QVERIFY(lim[2].find("形状参数") == std::string::npos);
    }

    void visible_table_titles_and_gate_diagnostics_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::StatisticTable warranty;
        warranty.title = "保修摘要";
        warranty.headers = {"Property", "Value"};
        warranty.rows = {{"Exposure", "1000"}};
        page.tables.push_back(warranty);
        page.diagnostics.push_back({
            datalab::domain::DiagnosticMessage::Severity::warning,
            "johnson_capability_gated",
            "Johnson 能力为研究/预览结果：可显示拟合与变换尺度指数，"
            "但不得输出过程合格判定（门禁：缺 golden/尾部/人工解释验收）。"});

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());
        QCOMPARE(
            localized.document.pages[0].visible_tables.front().title,
            std::string("Warranty summary"));
        QCOMPARE(
            localized.document.pages[0].visible_tables.front().headers[0],
            std::string("Property"));
        QCOMPARE(
            localized.document.pages[0].visible_tables.front().headers[1],
            std::string("Value"));
        // Source tables must remain Chinese production text (visible layer only).
        QCOMPARE(document.pages[0].source_page.tables.front().title, std::string("保修摘要"));

        bool saw_en_gate = false;
        bool saw_honesty = false;
        for (const auto& diagnostic : localized.document.pages[0].visible_diagnostics) {
            if (diagnostic.code == "johnson_capability_gated") {
                QVERIFY(diagnostic.message.find("research/preview") != std::string::npos);
                saw_en_gate = true;
            }
            if (diagnostic.code == "report_body_partial_bilingual") {
                saw_honesty = true;
            }
        }
        QVERIFY(saw_en_gate);
        QVERIFY(saw_honesty);
        QCOMPARE(
            facts_fingerprint(localized.document.pages[0].source_page.facts),
            facts_fingerprint(document.pages[0].source_page.facts));
    }

    void dual_line_core_table_plot_page_and_graph_captions_localize_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "正态过程能力分析";
        datalab::domain::StatisticTable coeffs;
        coeffs.title = "系数与效应";
        coeffs.headers = {"项", "Coef", "SE Coef", "T", "P-Value"};
        page.tables.push_back(coeffs);
        datalab::domain::StatisticTable anova;
        anova.title = "方差分析";
        anova.headers = {"来源", "DF", "SS", "MS", "F", "P-Value"};
        page.tables.push_back(anova);
        datalab::domain::StatisticTable imr;
        imr.title = "I-MR 参数";
        imr.headers = {"指标", "数值"};
        page.tables.push_back(imr);
        datalab::domain::StatisticTable gage;
        gage.title = "Gage R&R 摘要";
        gage.headers = {"项目", "估计", "下限", "上限"};
        page.tables.push_back(gage);
        datalab::domain::PlotSpec residual;
        residual.title = "残差与拟合值";
        residual.subtitle =
            "显示 N = 40    分析 N(水平) = 12    分析 N = 48    分面 = Batch"
            "    excluded = 2    hidden = 6";
        page.plots.push_back(residual);
        datalab::domain::PlotSpec panel;
        panel.title = "箱线图";
        panel.subtitle = "facet = A    分析 N(水平) = 5    显示 N = 5";
        page.plots.push_back(panel);
        datalab::domain::PlotSpec hist;
        hist.title = "过程能力直方图";
        page.plots.push_back(hist);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].source_page.title = page.title;
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QCOMPARE(
            localized.document.pages[0].source_page.title,
            std::string("Normal process capability analysis"));
        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(tables[0].title, std::string("Coefficients and effects"));
        QCOMPARE(tables[0].headers[0], std::string("Term"));
        QCOMPARE(tables[1].title, std::string("Analysis of variance"));
        QCOMPARE(tables[1].headers[0], std::string("Source"));
        QCOMPARE(tables[2].title, std::string("I-MR parameters"));
        QCOMPARE(tables[2].headers[0], std::string("Metric"));
        QCOMPARE(tables[2].headers[1], std::string("Value"));
        QCOMPARE(tables[3].title, std::string("Gage R&R summary"));
        QCOMPARE(tables[3].headers[0], std::string("Item"));
        QCOMPARE(tables[3].headers[1], std::string("Estimate"));
        QCOMPARE(tables[3].headers[2], std::string("Lower"));
        QCOMPARE(tables[3].headers[3], std::string("Upper"));
        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(plots[0].title, std::string("Residuals vs fits"));
        QVERIFY(plots[0].subtitle.find("Display N = 40") != std::string::npos);
        QVERIFY(plots[0].subtitle.find("Analysis N (level) = 12") != std::string::npos);
        QVERIFY(plots[0].subtitle.find("Analysis N = 48") != std::string::npos);
        QVERIFY(plots[0].subtitle.find("Facet = Batch") != std::string::npos);
        QVERIFY(plots[0].subtitle.find("分析 N(水平)") == std::string::npos);
        QVERIFY(plots[0].subtitle.find("显示 N") == std::string::npos);
        QVERIFY(plots[0].subtitle.find("分面 =") == std::string::npos);
        QCOMPARE(plots[1].title, std::string("Boxplot"));
        QVERIFY(plots[1].subtitle.find("Facet = A") != std::string::npos);
        QVERIFY(plots[1].subtitle.find("Analysis N (level) = 5") != std::string::npos);
        QVERIFY(plots[1].subtitle.find("facet =") == std::string::npos);
        QCOMPARE(plots[2].title, std::string("Process capability histogram"));
        QCOMPARE(page.title, std::string("正态过程能力分析"));
        QCOMPARE(page.tables[0].title, std::string("系数与效应"));
    }

    void dual_line_doe_rsm_msa_capability_titles_localize_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "响应曲面分析";
        datalab::domain::StatisticTable coded;
        coded.title = "系数（编码单位）";
        page.tables.push_back(coded);
        datalab::domain::StatisticTable binomial;
        binomial.title = "二项能力";
        page.tables.push_back(binomial);
        datalab::domain::StatisticTable poisson;
        poisson.title = "泊松能力";
        page.tables.push_back(poisson);
        datalab::domain::StatisticTable km;
        km.title = "Kaplan-Meier 生存表";
        page.tables.push_back(km);
        datalab::domain::StatisticTable expo2;
        expo2.title = "两参数指数参数";
        page.tables.push_back(expo2);
        datalab::domain::StatisticTable laney;
        laney.title = "Laney 图逐子组统计";
        page.tables.push_back(laney);

        datalab::domain::PlotSpec main_fx;
        main_fx.title = "主效应图 - Temp";
        page.plots.push_back(main_fx);
        datalab::domain::PlotSpec interaction;
        interaction.title = "交互作用图 - A*B";
        page.plots.push_back(interaction);
        datalab::domain::PlotSpec cube;
        cube.title = "立方图（方形）";
        page.plots.push_back(cube);
        datalab::domain::PlotSpec pareto;
        pareto.title = "标准化效应 Pareto";
        page.plots.push_back(pareto);
        datalab::domain::PlotSpec contour;
        contour.title = "等值线图 - X vs Y";
        page.plots.push_back(contour);
        datalab::domain::PlotSpec surface;
        surface.title = "响应曲面图 - X vs Y";
        page.plots.push_back(surface);
        datalab::domain::PlotSpec static_surface;
        static_surface.title = "静态响应曲面图（非可旋转 3D）- X vs Y";
        page.plots.push_back(static_surface);
        datalab::domain::PlotSpec msa;
        msa.title = "操作者×零件交互";
        page.plots.push_back(msa);
        datalab::domain::PlotSpec type1;
        type1.title = "Type 1 Gage 直方图";
        page.plots.push_back(type1);
        datalab::domain::PlotSpec last25;
        last25.title = "最后 25 个子组";
        page.plots.push_back(last25);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].source_page.title = page.title;
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QCOMPARE(
            localized.document.pages[0].source_page.title,
            std::string("Response surface analysis"));
        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(tables[0].title, std::string("Coefficients (coded units)"));
        QCOMPARE(tables[1].title, std::string("Binomial capability"));
        QCOMPARE(tables[2].title, std::string("Poisson capability"));
        QCOMPARE(tables[3].title, std::string("Kaplan-Meier survival table"));
        QCOMPARE(tables[4].title, std::string("2-parameter exponential parameters"));
        QCOMPARE(tables[5].title, std::string("Laney chart subgroup statistics"));
        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(plots[0].title, std::string("Main effects plot - Temp"));
        QCOMPARE(plots[1].title, std::string("Interaction plot - A*B"));
        QCOMPARE(plots[2].title, std::string("Cube plot (square)"));
        QCOMPARE(plots[3].title, std::string("Standardized effects Pareto"));
        QCOMPARE(plots[4].title, std::string("Contour plot - X vs Y"));
        QCOMPARE(plots[5].title, std::string("Response surface plot - X vs Y"));
        QCOMPARE(
            plots[6].title,
            std::string("Static response surface (non-rotatable 3D) - X vs Y"));
        QCOMPARE(plots[7].title, std::string("Operator × part interaction"));
        QCOMPARE(plots[8].title, std::string("Type 1 Gage histogram"));
        QCOMPARE(plots[9].title, std::string("Last 25 subgroups"));
        QVERIFY(plots[0].title.find("主效应图") == std::string::npos);
        QVERIFY(plots[5].title.find("响应曲面图") == std::string::npos);
        QVERIFY(plots[6].title.find("静态响应") == std::string::npos);

        OutputPage design_page = sample_page();
        design_page.title = "中心复合设计 (CCD)";
        auto design_doc = build_report_document(sample_table(), {design_page}, profile);
        design_doc.pages[0].source_page.title = design_page.title;
        const auto design_localized = localize_report_document(design_doc);
        QCOMPARE(
            design_localized.document.pages[0].source_page.title,
            std::string("Central composite design (CCD)"));

        OutputPage attr_page = sample_page();
        attr_page.title = "二项过程能力";
        auto attr_doc = build_report_document(sample_table(), {attr_page}, profile);
        attr_doc.pages[0].source_page.title = attr_page.title;
        const auto attr_localized = localize_report_document(attr_doc);
        QCOMPARE(
            attr_localized.document.pages[0].source_page.title,
            std::string("Binomial process capability"));
    }

    void dual_line_reliability_turnbull_cif_titles_localize_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "可靠性分析";
        datalab::domain::StatisticTable mode;
        mode.title = "分模式可靠度（cause-specific）";
        page.tables.push_back(mode);
        datalab::domain::StatisticTable cif;
        cif.title = "累计发生函数 CIF（Aalen-Johansen）";
        page.tables.push_back(cif);
        datalab::domain::StatisticTable fg_multi;
        fg_multi.title = "Fine-Gray 子分布风险（多协变量）";
        page.tables.push_back(fg_multi);
        datalab::domain::StatisticTable fg_cont;
        fg_cont.title = "Fine-Gray 子分布风险（连续协变量）";
        page.tables.push_back(fg_cont);
        datalab::domain::StatisticTable fg_bin;
        fg_bin.title = "Fine-Gray 子分布风险（二分类 group）";
        page.tables.push_back(fg_bin);
        datalab::domain::StatisticTable estimates;
        estimates.title = "生存估计";
        page.tables.push_back(estimates);

        datalab::domain::PlotSpec km;
        km.title = "Kaplan-Meier 生存曲线";
        km.x_axis_title = "时间";
        datalab::domain::PlotSeries fitted;
        fitted.label = "拟合 S(t)";
        km.series.push_back(fitted);
        page.plots.push_back(km);
        datalab::domain::PlotSpec turnbull;
        turnbull.title = "Turnbull 生存曲线";
        page.plots.push_back(turnbull);
        datalab::domain::PlotSpec weibull3;
        weibull3.title = "三参数 Weibull 生存曲线";
        page.plots.push_back(weibull3);
        datalab::domain::PlotSpec expo2;
        expo2.title = "两参数指数 概率图";
        page.plots.push_back(expo2);
        datalab::domain::PlotSpec ln3;
        ln3.title = "三参数对数正态 生存曲线";
        page.plots.push_back(ln3);
        datalab::domain::PlotSpec unknown;
        unknown.title = "未知模型XYZ 生存曲线";
        page.plots.push_back(unknown);
        datalab::domain::PlotSpec legacy_km;
        legacy_km.title = "Kaplan-Meier Survival Curve";
        page.plots.push_back(legacy_km);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].source_page.title = page.title;
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QCOMPARE(
            localized.document.pages[0].source_page.title,
            std::string("Reliability analysis"));
        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(
            tables[0].title, std::string("Mode-specific reliability (cause-specific)"));
        QCOMPARE(
            tables[1].title,
            std::string("Cumulative incidence CIF (Aalen–Johansen)"));
        QCOMPARE(
            tables[2].title,
            std::string("Fine-Gray subdistribution hazard (multi-covariate)"));
        QCOMPARE(
            tables[3].title,
            std::string("Fine-Gray subdistribution hazard (continuous covariate)"));
        QCOMPARE(
            tables[4].title,
            std::string("Fine-Gray subdistribution hazard (binary group)"));
        QCOMPARE(tables[5].title, std::string("Survival estimates"));
        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(plots[0].title, std::string("Kaplan-Meier survival curve"));
        QCOMPARE(plots[0].x_axis_title, std::string("Time"));
        QCOMPARE(plots[0].series[0].label, std::string("Fitted S(t)"));
        QCOMPARE(plots[1].title, std::string("Turnbull survival curve"));
        QCOMPARE(plots[2].title, std::string("3-parameter Weibull Survival curve"));
        QCOMPARE(plots[3].title, std::string("2-parameter exponential Probability plot"));
        QCOMPARE(plots[4].title, std::string("3-parameter lognormal Survival curve"));
        // Unknown model: leave whole title unchanged (no half-translate).
        QCOMPARE(plots[5].title, std::string("未知模型XYZ 生存曲线"));
        QCOMPARE(plots[6].title, std::string("Kaplan-Meier survival curve"));
        QVERIFY(plots[2].title.find("生存曲线") == std::string::npos);
        QVERIFY(plots[2].title.find("三参数") == std::string::npos);

        OutputPage turnbull_page = sample_page();
        turnbull_page.title = "区间删失 Kaplan–Meier（Turnbull）";
        auto turnbull_doc =
            build_report_document(sample_table(), {turnbull_page}, profile);
        turnbull_doc.pages[0].source_page.title = turnbull_page.title;
        const auto turnbull_localized = localize_report_document(turnbull_doc);
        QCOMPARE(
            turnbull_localized.document.pages[0].source_page.title,
            std::string("Interval-censored Kaplan–Meier (Turnbull)"));

        OutputPage legacy_page = sample_page();
        legacy_page.title = "Reliability Analysis";
        auto legacy_doc = build_report_document(sample_table(), {legacy_page}, profile);
        legacy_doc.pages[0].source_page.title = legacy_page.title;
        const auto legacy_localized = localize_report_document(legacy_doc);
        QCOMPARE(
            legacy_localized.document.pages[0].source_page.title,
            std::string("Reliability analysis"));
    }

    void dual_line_graph_eda_page_and_facet_titles_localize_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "散点图（分面）";
        datalab::domain::PlotSpec scatter;
        scatter.title = "散点图";
        page.plots.push_back(scatter);
        datalab::domain::PlotSpec density;
        density.title = "密度图（分面）";
        page.plots.push_back(density);
        datalab::domain::PlotSpec hexbin;
        hexbin.title = "Hexbin / 二维分箱";
        page.plots.push_back(hexbin);
        datalab::domain::PlotSpec hex_facet;
        hex_facet.title = "Hexbin（分面）";
        page.plots.push_back(hex_facet);
        datalab::domain::PlotSpec violin;
        violin.title = "小提琴图";
        page.plots.push_back(violin);
        datalab::domain::PlotSpec bar;
        bar.title = "条形图（分面）";
        page.plots.push_back(bar);
        datalab::domain::PlotSpec panel;
        panel.title = "散点图 · BatchA";
        page.plots.push_back(panel);
        datalab::domain::PlotSpec multi;
        multi.title = "Multi-Vari 图";
        page.plots.push_back(multi);
        datalab::domain::PlotSpec cause;
        cause.title = "因果图";
        page.plots.push_back(cause);
        datalab::domain::PlotSpec hex_panel;
        hex_panel.title = "二维分箱 · Panel1";
        page.plots.push_back(hex_panel);
        datalab::domain::PlotSpec npp_panel;
        npp_panel.title = "正态概率图 · LotA";
        page.plots.push_back(npp_panel);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].source_page.title = page.title;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QCOMPARE(
            localized.document.pages[0].source_page.title,
            std::string("Scatterplot (faceted)"));
        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(plots[0].title, std::string("Scatterplot"));
        QCOMPARE(plots[1].title, std::string("Density plot (faceted)"));
        QCOMPARE(plots[2].title, std::string("Hexbin / 2D binning"));
        QCOMPARE(plots[3].title, std::string("Hexbin (faceted)"));
        QCOMPARE(plots[4].title, std::string("Violin plot"));
        QCOMPARE(plots[5].title, std::string("Bar chart (faceted)"));
        QCOMPARE(plots[6].title, std::string("Scatterplot · BatchA"));
        QCOMPARE(plots[7].title, std::string("Multi-Vari chart"));
        QCOMPARE(plots[8].title, std::string("Cause-and-effect diagram"));
        QCOMPARE(plots[9].title, std::string("2D binning · Panel1"));
        QCOMPARE(plots[10].title, std::string("Normal probability plot · LotA"));
        QVERIFY(plots[1].title.find("密度图") == std::string::npos);
        QVERIFY(plots[1].title.find("分面") == std::string::npos);

        OutputPage heatmap = sample_page();
        heatmap.title = "热图";
        auto heat_doc = build_report_document(sample_table(), {heatmap}, profile);
        heat_doc.pages[0].source_page.title = heatmap.title;
        QCOMPARE(
            localize_report_document(heat_doc).document.pages[0].source_page.title,
            std::string("Heatmap"));

        OutputPage variability = sample_page();
        variability.title = "变异性图";
        auto var_doc = build_report_document(sample_table(), {variability}, profile);
        var_doc.pages[0].source_page.title = variability.title;
        QCOMPARE(
            localize_report_document(var_doc).document.pages[0].source_page.title,
            std::string("Variability chart"));

        OutputPage npp = sample_page();
        npp.title = "正态概率图（分面）";
        auto npp_doc = build_report_document(sample_table(), {npp}, profile);
        npp_doc.pages[0].source_page.title = npp.title;
        QCOMPARE(
            localize_report_document(npp_doc).document.pages[0].source_page.title,
            std::string("Normal probability plot (faceted)"));
    }

    void dual_line_inference_association_and_ml_page_titles_localize_to_en_us()
    {
        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        const auto localize_title = [&](const std::string& zh) {
            OutputPage page = sample_page();
            page.title = zh;
            auto document = build_report_document(sample_table(), {page}, profile);
            document.pages[0].source_page.title = page.title;
            return localize_report_document(document).document.pages[0].source_page.title;
        };

        QCOMPARE(localize_title("Pearson 相关"), std::string("Pearson correlation"));
        QCOMPARE(
            localize_title("Spearman 秩相关"), std::string("Spearman rank correlation"));
        QCOMPARE(localize_title("单比例检验"), std::string("One-proportion test"));
        QCOMPARE(localize_title("两比例检验"), std::string("Two-proportions test"));
        QCOMPARE(
            localize_title("单样本泊松率"), std::string("One-sample Poisson rate"));
        QCOMPARE(
            localize_title("双样本泊松率"), std::string("Two-sample Poisson rates"));
        QCOMPARE(
            localize_title("列联表卡方"), std::string("Chi-square contingency"));
        QCOMPARE(localize_title("交叉表"), std::string("Crosstabulation"));
        QCOMPARE(
            localize_title("卡方拟合优度"),
            std::string("Chi-square goodness of fit"));
        QCOMPARE(
            localize_title("泊松拟合优度"), std::string("Poisson goodness of fit"));
        QCOMPARE(
            localize_title("Box-Cox 变换"), std::string("Box-Cox transformation"));
        QCOMPARE(localize_title("双因素 ANOVA"), std::string("Two-way ANOVA"));
        QCOMPARE(
            localize_title("方差检验"), std::string("Test for equal variances"));
        QCOMPARE(
            localize_title("正态容差区间"), std::string("Normal tolerance interval"));
        QCOMPARE(
            localize_title("非参数容差区间"),
            std::string("Nonparametric tolerance interval"));
        QCOMPARE(
            localize_title("T 功效与样本量"), std::string("t power and sample size"));
        QCOMPARE(
            localize_title("个体分布识别"),
            std::string("Individual distribution identification"));
        QCOMPARE(
            localize_title("均值分析 (ANOM)"),
            std::string("Analysis of means (ANOM)"));
        QCOMPARE(
            localize_title("属性一次抽样"), std::string("Attribute single sampling"));
        QCOMPARE(
            localize_title("属性一致性分析"),
            std::string("Attribute agreement analysis"));
        QCOMPARE(localize_title("Mann-Whitney 检验"), std::string("Mann-Whitney test"));
        QCOMPARE(
            localize_title("Wilcoxon 符号秩检验"),
            std::string("Wilcoxon signed-rank test"));
        QCOMPARE(localize_title("Kruskal-Wallis 检验"), std::string("Kruskal-Wallis test"));
        QCOMPARE(localize_title("Friedman 检验"), std::string("Friedman test"));
        QCOMPARE(
            localize_title("二元 Logistic 回归"),
            std::string("Binary logistic regression"));
        QCOMPARE(
            localize_title("主成分分析"),
            std::string("Principal component analysis"));
        QCOMPARE(localize_title("K-Means 聚类"), std::string("K-Means clustering"));
        QCOMPARE(localize_title("逐步回归"), std::string("Stepwise regression"));
        QCOMPARE(localize_title("自相关 / 偏自相关"), std::string("ACF / PACF"));
        QCOMPARE(
            localize_title("互相关（CCF）"), std::string("Cross-correlation (CCF)"));
        QCOMPARE(
            localize_title("Correlogram（相关热图）"),
            std::string("Correlogram (correlation heatmap)"));
        QCOMPARE(
            localize_title("时间序列分解"), std::string("Time series decomposition"));
        QCOMPARE(localize_title("EDA 四图"), std::string("EDA four-plot"));
        QCOMPARE(
            localize_title("I-MR-R/S 控制图"), std::string("I-MR-R/S control chart"));
        QCOMPARE(
            localize_title("广义方差图"), std::string("Generalized variance chart"));
        QCOMPARE(localize_title("Hotelling T²"), std::string("Hotelling T²"));
        QCOMPARE(localize_title("MEWMA"), std::string("MEWMA"));
        QCOMPARE(localize_title("Isolation Forest"), std::string("Isolation Forest"));
        QCOMPARE(localize_title("Crossed Gage R&R"), std::string("Crossed Gage R&R"));
        QVERIFY(localize_title("列联表卡方").find("卡方") == std::string::npos);
    }

    void dual_line_orphan_table_plot_titles_localize_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "Hotelling T²";
        datalab::domain::StatisticTable special;
        special.title = "特殊原因规则证据";
        page.tables.push_back(special);
        datalab::domain::StatisticTable doe_anova;
        doe_anova.title = "DOE ANOVA";
        page.tables.push_back(doe_anova);
        datalab::domain::StatisticTable two_sample;
        two_sample.title = "双样本描述统计";
        page.tables.push_back(two_sample);
        datalab::domain::StatisticTable test;
        test.title = "检验结果";
        page.tables.push_back(test);
        datalab::domain::StatisticTable counts;
        counts.title = "观察频数";
        page.tables.push_back(counts);
        datalab::domain::StatisticTable chi;
        chi.title = "卡方检验";
        page.tables.push_back(chi);
        datalab::domain::StatisticTable cells;
        cells.title = "单元格统计";
        page.tables.push_back(cells);
        datalab::domain::StatisticTable grouping;
        grouping.title = "Grouping Information";
        page.tables.push_back(grouping);
        datalab::domain::StatisticTable dw;
        dw.title = "Durbin-Watson";
        page.tables.push_back(dw);
        datalab::domain::StatisticTable assumptions;
        assumptions.title = "假设检查";
        page.tables.push_back(assumptions);
        datalab::domain::StatisticTable fit;
        fit.title = "拟合与诊断";
        page.tables.push_back(fit);
        datalab::domain::StatisticTable paired;
        paired.title = "配对差值统计";
        page.tables.push_back(paired);
        datalab::domain::StatisticTable process;
        process.title = "Process Data";
        page.tables.push_back(process);
        datalab::domain::StatisticTable overall;
        overall.title = "Overall Capability";
        page.tables.push_back(overall);

        datalab::domain::PlotSpec paired_plot;
        paired_plot.title = "配对测量散点图";
        page.plots.push_back(paired_plot);
        datalab::domain::PlotSpec residual_pred;
        residual_pred.title = "残差与预测变量 - Temp";
        page.plots.push_back(residual_pred);
        datalab::domain::PlotSpec heat;
        heat.title = "观察频数热图";
        page.plots.push_back(heat);
        datalab::domain::PlotSpec adj;
        adj.title = "调整残差热图";
        page.plots.push_back(adj);
        datalab::domain::PlotSpec gage;
        gage.title = "Gage Run Chart";
        page.plots.push_back(gage);
        datalab::domain::PlotSpec equiv;
        equiv.title = "等价性区间";
        page.plots.push_back(equiv);
        datalab::domain::PlotSpec hotelling;
        hotelling.title = "Hotelling T² 图";
        page.plots.push_back(hotelling);
        datalab::domain::PlotSpec mewma;
        mewma.title = "MEWMA T² 图";
        page.plots.push_back(mewma);
        datalab::domain::PlotSpec isolation;
        isolation.title = "Isolation 分数";
        page.plots.push_back(isolation);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].source_page.title = page.title;
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QCOMPARE(
            localized.document.pages[0].source_page.title, std::string("Hotelling T²"));
        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(tables[0].title, std::string("Special-cause rule evidence"));
        QCOMPARE(tables[1].title, std::string("DOE ANOVA"));
        QCOMPARE(tables[2].title, std::string("Two-sample descriptive statistics"));
        QCOMPARE(tables[3].title, std::string("Test results"));
        QCOMPARE(tables[4].title, std::string("Observed counts"));
        QCOMPARE(tables[5].title, std::string("Chi-square test"));
        QCOMPARE(tables[6].title, std::string("Cell statistics"));
        QCOMPARE(tables[7].title, std::string("Grouping Information"));
        QCOMPARE(tables[8].title, std::string("Durbin-Watson"));
        QCOMPARE(tables[9].title, std::string("Assumption checks"));
        QCOMPARE(tables[10].title, std::string("Fits and diagnostics"));
        QCOMPARE(tables[11].title, std::string("Paired-difference statistics"));
        QCOMPARE(tables[12].title, std::string("Process Data"));
        QCOMPARE(tables[13].title, std::string("Overall Capability"));
        QVERIFY(tables[0].title.find("特殊") == std::string::npos);
        QVERIFY(tables[2].title.find("双样本") == std::string::npos);
        QVERIFY(tables[9].title.find("假设") == std::string::npos);
        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(plots[0].title, std::string("Paired-measurement scatterplot"));
        QCOMPARE(plots[1].title, std::string("Residuals vs predictor - Temp"));
        QCOMPARE(plots[2].title, std::string("Observed-count heatmap"));
        QCOMPARE(plots[3].title, std::string("Adjusted-residual heatmap"));
        QCOMPARE(plots[4].title, std::string("Gage Run Chart"));
        QCOMPARE(plots[5].title, std::string("Equivalence interval"));
        QCOMPARE(plots[6].title, std::string("Hotelling T² chart"));
        QCOMPARE(plots[7].title, std::string("MEWMA T² chart"));
        QCOMPARE(plots[8].title, std::string("Isolation scores"));
        QVERIFY(plots[1].title.find("残差与预测") == std::string::npos);
        QVERIFY(plots[6].title.find("图") == std::string::npos);

        auto profile_zh = make_report_profile(ReportTemplateKind::engineer);
        profile_zh.locale.language_tag = "zh-CN";
        auto zh_doc = build_report_document(sample_table(), {page}, profile_zh);
        zh_doc.pages[0].visible_tables = {overall};
        const auto zh_localized = localize_report_document(zh_doc);
        QCOMPARE(
            zh_localized.document.pages[0].visible_tables[0].title,
            std::string("总体能力"));
    }

    void dual_line_spc_rule_evidence_headers_and_status_cells_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::StatisticTable table;
        table.title = "特殊原因规则证据";
        table.headers = {
            "规则ID", "规则名称", "状态", "判定窗口", "阈值", "比较方向",
            "解释", "触发图点", "原始行", "建议动作",
            "不适用原因", "未验证原因", "计算失败原因"};
        table.rows.push_back({
            "beyond_control_limit",
            "单点超出 3σ 控制限",
            "已触发",
            "单点",
            "LCL / UCL（约 3σ）",
            "y < LCL 或 y > UCL（严格）",
            "任一点低于 LCL 或高于 UCL；有限控制限使用严格越界比较。表示该点与当前控制模型不一致，不等于根因已确认。"
                " 触发图点序号（1-based）: 12。",
            "12",
            "12",
            "复核测量、批次、设备或取样条件，并关联原始观测行。",
            "",
            "",
            ""});
        table.rows.push_back({
            "nine_same_side",
            "连续 9 点位于中心线同侧",
            "未触发",
            "连续 9 点",
            "同侧 9 点",
            "全部位于中心线同一侧（中心线上点打断）",
            "连续 9 个可用点全部在中心线同一侧；跨缺失或阶段断点不形成窗口。提示均值偏移、分层或阶段变化。"
                " 当前未触发。",
            "",
            "",
            "调查均值偏移、分层或阶段变化，检查窗口首尾相关行。",
            "",
            "规则未启用，本次分析未验证该特殊原因模式。",
            ""});
        table.rows.push_back({
            "unknown_rule",
            "未知自定义规则",
            "计算失败",
            "未知窗口",
            "未知阈值",
            "未知比较",
            "未知解释保持中文。",
            "",
            "",
            "自定义动作保持中文。",
            "此规则不适用于控制图类型 individuals。",
            "",
            "没有可绘制的控制图点，无法判定规则。"});
        page.tables.push_back(table);

        datalab::domain::StatisticTable point_stats;
        point_stats.title = "I-MR 逐点统计";
        point_stats.headers = {
            "原始行", "阶段", "观测值", "触发规则", "主要规则", "子组", "信号", "方向"};
        page.tables.push_back(point_stats);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_tables = page.tables;
        const auto localized = localize_report_document(document);
        const auto& evidence = localized.document.pages[0].visible_tables[0];
        QCOMPARE(evidence.title, std::string("Special-cause rule evidence"));
        QCOMPARE(evidence.headers[0], std::string("Rule ID"));
        QCOMPARE(evidence.headers[1], std::string("Rule name"));
        QCOMPARE(evidence.headers[2], std::string("Status"));
        QCOMPARE(evidence.headers[3], std::string("Decision window"));
        QCOMPARE(evidence.headers[4], std::string("Threshold"));
        QCOMPARE(evidence.headers[5], std::string("Comparison direction"));
        QCOMPARE(evidence.headers[6], std::string("Explanation"));
        QCOMPARE(evidence.headers[7], std::string("Triggered plot points"));
        QCOMPARE(evidence.headers[8], std::string("Source row"));
        QCOMPARE(evidence.headers[9], std::string("Suggested action"));
        QCOMPARE(evidence.headers[10], std::string("Not-applicable reason"));
        QCOMPARE(evidence.headers[11], std::string("Not-verified reason"));
        QCOMPARE(evidence.headers[12], std::string("Calculation-failed reason"));
        QVERIFY(evidence.headers[0].find("规则") == std::string::npos);
        QCOMPARE(evidence.rows[0][1], std::string("1 point beyond 3σ control limits"));
        QCOMPARE(evidence.rows[0][2], std::string("Triggered"));
        QCOMPARE(evidence.rows[0][3], std::string("1 point"));
        QCOMPARE(evidence.rows[0][4], std::string("LCL / UCL (~3σ)"));
        QCOMPARE(evidence.rows[0][5], std::string("y < LCL or y > UCL (strict)"));
        QVERIFY(evidence.rows[0][6].find("Triggered plot point indices") != std::string::npos);
        QVERIFY(evidence.rows[0][6].find("12。") != std::string::npos
                || evidence.rows[0][6].find("12.") != std::string::npos);
        QVERIFY(evidence.rows[0][6].find("任一点") == std::string::npos);
        QVERIFY(evidence.rows[0][6].find("触发图点") == std::string::npos);
        QCOMPARE(
            evidence.rows[0][9],
            std::string(
                "Recheck measurement, lot, equipment, or sampling conditions and link source rows."));
        QCOMPARE(evidence.rows[1][1], std::string("9 points in a row on the same side of the center line"));
        QCOMPARE(evidence.rows[1][2], std::string("Not triggered"));
        QCOMPARE(evidence.rows[1][3], std::string("9 points in a row"));
        QCOMPARE(evidence.rows[1][4], std::string("9 points on the same side"));
        QVERIFY(evidence.rows[1][6].find("Currently not triggered.") != std::string::npos);
        QVERIFY(evidence.rows[1][6].find("当前未触发") == std::string::npos);
        QCOMPARE(
            evidence.rows[1][9],
            std::string(
                "Investigate mean shift, stratification, or stage change; check window endpoints."));
        QCOMPARE(
            evidence.rows[1][11],
            std::string(
                "Rule not enabled; this special-cause pattern was not verified in this analysis."));
        QCOMPARE(evidence.rows[2][1], std::string("未知自定义规则"));
        QCOMPARE(evidence.rows[2][2], std::string("Calculation failed"));
        QCOMPARE(evidence.rows[2][3], std::string("未知窗口"));
        QCOMPARE(evidence.rows[2][6], std::string("未知解释保持中文。"));
        QCOMPARE(evidence.rows[2][9], std::string("自定义动作保持中文。"));
        QCOMPARE(
            evidence.rows[2][10],
            std::string("This rule does not apply to control chart type individuals."));
        QCOMPARE(
            evidence.rows[2][12],
            std::string("No plottable control-chart points; rule cannot be evaluated."));
        QVERIFY(evidence.rows[0][1].find("单点") == std::string::npos);
        QVERIFY(evidence.rows[0][9].find("复核") == std::string::npos);

        const auto& points = localized.document.pages[0].visible_tables[1];
        QCOMPARE(points.headers[0], std::string("Source row"));
        QCOMPARE(points.headers[1], std::string("Stage"));
        QCOMPARE(points.headers[2], std::string("Observed value"));
        QCOMPARE(points.headers[3], std::string("Triggered rules"));
        QCOMPARE(points.headers[4], std::string("Primary rule"));
        QCOMPARE(points.headers[5], std::string("Subgroup"));
        QCOMPARE(points.headers[6], std::string("Signal"));
        QCOMPARE(points.headers[7], std::string("Direction"));
        QVERIFY(points.headers[3].find("触发") == std::string::npos);
    }

    void dual_line_anova_regression_contingency_headers_localize_to_en_us()
    {
        OutputPage page = sample_page();

        datalab::domain::StatisticTable tukey;
        tukey.title = "Tukey 成对比较";
        tukey.headers = {
            "对比", "差值", "下限", "上限", "Adjusted P-Value", "显著",
            "族置信水平", "误差 DF", "MSE", "方法"};
        page.tables.push_back(tukey);

        datalab::domain::StatisticTable diagnostics;
        diagnostics.title = "回归诊断";
        diagnostics.headers = {
            "观测", "原始行", "响应", "拟合值", "残差", "标准化残差",
            "内部标准化残差", "学生化残差", "删除学生化残差",
            "杠杆值", "Cook 距离", "DFITS", "诊断标记"};
        page.tables.push_back(diagnostics);

        datalab::domain::StatisticTable unusual;
        unusual.headers = {
            "观测", "原始行", "响应", "拟合值", "残差", "标准化残差",
            "杠杆", "Cook", "DFITS", "标记"};
        page.tables.push_back(unusual);

        datalab::domain::StatisticTable gof;
        gof.headers = {
            "N", "N*", "DF", "Chi-Sq", "P-Value", "最小期望频数", "<5 类别数", "有效性"};
        page.tables.push_back(gof);

        datalab::domain::StatisticTable cells;
        cells.headers = {"行", "列", "Observed", "Expected", "Raw Residual"};
        page.tables.push_back(cells);

        datalab::domain::StatisticTable fisher;
        fisher.headers = {"方法", "P-Value", "优势比 OR", "a", "b", "c", "d"};
        page.tables.push_back(fisher);

        datalab::domain::StatisticTable mcnemar;
        mcnemar.headers = {"单元格", "计数"};
        mcnemar.rows.push_back({"合计", "10"});
        page.tables.push_back(mcnemar);

        datalab::domain::StatisticTable edwards;
        edwards.headers = {"χ² (Edwards)", "DF", "P-Value", "连续性校正", "方法"};
        page.tables.push_back(edwards);

        datalab::domain::StatisticTable twoway;
        twoway.headers = {
            "来源", "Seq SS", "Adj SS", "DF", "MS", "F", "P-Value", "可估计性"};
        page.tables.push_back(twoway);

        datalab::domain::StatisticTable interaction;
        interaction.headers = {"因子 A", "因子 B", "N", "均值"};
        page.tables.push_back(interaction);

        datalab::domain::StatisticTable ranks;
        ranks.headers = {
            "对比", "平均秩差", "SE", "Z", "未调整 P", "Bonferroni P", "显著"};
        page.tables.push_back(ranks);

        datalab::domain::StatisticTable poisson;
        poisson.headers = {"N", "N*", "λ̂", "类别数", "期望<5 数", "有效性"};
        page.tables.push_back(poisson);

        datalab::domain::StatisticTable unknown;
        unknown.headers = {"未知自定义列 XYZ"};
        page.tables.push_back(unknown);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_tables = page.tables;
        const auto localized = localize_report_document(document);
        const auto& tables = localized.document.pages[0].visible_tables;

        QCOMPARE(tables[0].headers[0], std::string("Contrast"));
        QCOMPARE(tables[0].headers[5], std::string("Significant"));
        QCOMPARE(tables[0].headers[6], std::string("Family confidence level"));
        QCOMPARE(tables[0].headers[7], std::string("Error DF"));
        QVERIFY(tables[0].headers[5].find("显著") == std::string::npos);

        QCOMPARE(tables[1].headers[6], std::string("Internally standardized residual"));
        QCOMPARE(tables[1].headers[7], std::string("Studentized residual"));
        QCOMPARE(tables[1].headers[8], std::string("Deleted studentized residual"));
        QCOMPARE(tables[1].headers[9], std::string("Leverage"));
        QCOMPARE(tables[1].headers[10], std::string("Cook's distance"));
        QCOMPARE(tables[1].headers[12], std::string("Diagnostic flags"));
        QVERIFY(tables[1].headers[9].find("杠杆") == std::string::npos);

        QCOMPARE(tables[2].headers[6], std::string("Leverage"));
        QCOMPARE(tables[2].headers[7], std::string("Cook"));
        QCOMPARE(tables[2].headers[9], std::string("Flag"));

        QCOMPARE(tables[3].headers[5], std::string("Minimum expected count"));
        QCOMPARE(tables[3].headers[6], std::string("Categories with expected <5"));
        QCOMPARE(tables[3].headers[7], std::string("Validity"));
        QVERIFY(tables[3].headers[5].find("期望") == std::string::npos);

        QCOMPARE(tables[4].headers[0], std::string("Row"));
        QCOMPARE(tables[4].headers[1], std::string("Column"));

        QCOMPARE(tables[5].headers[2], std::string("Odds ratio (OR)"));

        QCOMPARE(tables[6].headers[0], std::string("Cell"));
        QCOMPARE(tables[6].headers[1], std::string("Count"));
        QCOMPARE(tables[6].rows[0][0], std::string("Total"));

        QCOMPARE(tables[7].headers[3], std::string("Continuity correction"));

        QCOMPARE(tables[8].headers[7], std::string("Estimability"));
        QVERIFY(tables[8].headers[7].find("可估计") == std::string::npos);

        QCOMPARE(tables[9].headers[0], std::string("Factor A"));
        QCOMPARE(tables[9].headers[1], std::string("Factor B"));

        QCOMPARE(tables[10].headers[1], std::string("Mean rank difference"));
        QCOMPARE(tables[10].headers[4], std::string("Unadjusted P"));
        QCOMPARE(tables[10].headers[5], std::string("Bonferroni P"));

        QCOMPARE(tables[11].headers[3], std::string("Category count"));
        QCOMPARE(tables[11].headers[4], std::string("Count with expected <5"));

        QCOMPARE(tables[12].headers[0], std::string("未知自定义列 XYZ"));
    }

    void dual_line_anova_source_row_labels_and_related_headers_localize_to_en_us()
    {
        OutputPage page = sample_page();

        datalab::domain::StatisticTable anova;
        anova.title = "方差分析";
        anova.headers = {"来源", "DF", "Adj SS", "MS", "F", "P-Value"};
        anova.rows.push_back({"组间", "2", "10", "5", "4", "0.05"});
        anova.rows.push_back({"回归", "3", "12", "4", "3", "0.04"});
        anova.rows.push_back({"失拟", "1", "1", "1", "0.5", "0.5"});
        anova.rows.push_back({"纯误差", "2", "2", "1", "", ""});
        anova.rows.push_back({"误差", "10", "8", "0.8", "", ""});
        anova.rows.push_back({"合计", "15", "20", "", "", ""});
        anova.rows.push_back({"未知来源 XYZ", "1", "1", "1", "", ""});
        page.tables.push_back(anova);

        datalab::domain::StatisticTable factors;
        factors.title = "交互均值";
        factors.headers = {"因子", "编码水平", "实际水平", "组合", "组中位数", "CI 下限", "CI 上限"};
        factors.rows.push_back({"第一组", "1", "10", "A1", "5", "4", "6"});
        factors.rows.push_back({"第二组", "−1", "0", "A2", "3", "2", "4"});
        page.tables.push_back(factors);

        datalab::domain::StatisticTable mcnemar;
        mcnemar.headers = {"单元格", "计数"};
        mcnemar.rows.push_back({"不一致对数 b+c", "7"});
        mcnemar.rows.push_back({"有效配对数", "40"});
        page.tables.push_back(mcnemar);

        datalab::domain::StatisticTable ts;
        ts.headers = {"序号", "原始行", "实际值", "拟合值", "残差"};
        page.tables.push_back(ts);

        datalab::domain::PlotSpec residual;
        residual.title = "交互均值图";
        residual.y_axis_title = "残差";
        residual.x_axis_title = "因子 A";
        page.plots.push_back(residual);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        const auto& tables = localized.document.pages[0].visible_tables;

        QCOMPARE(tables[0].title, std::string("Analysis of variance"));
        QCOMPARE(tables[0].rows[0][0], std::string("Between groups"));
        QCOMPARE(tables[0].rows[1][0], std::string("Regression"));
        QCOMPARE(tables[0].rows[2][0], std::string("Lack of fit"));
        QCOMPARE(tables[0].rows[3][0], std::string("Pure error"));
        QCOMPARE(tables[0].rows[4][0], std::string("Error"));
        QCOMPARE(tables[0].rows[5][0], std::string("Total"));
        QCOMPARE(tables[0].rows[6][0], std::string("未知来源 XYZ"));
        QVERIFY(tables[0].rows[0][0].find("组间") == std::string::npos);

        QCOMPARE(tables[1].title, std::string("Interaction means"));
        QCOMPARE(tables[1].headers[0], std::string("Factor"));
        QCOMPARE(tables[1].headers[1], std::string("Coded level"));
        QCOMPARE(tables[1].headers[2], std::string("Actual level"));
        QCOMPARE(tables[1].headers[3], std::string("Combination"));
        QCOMPARE(tables[1].headers[4], std::string("Group median"));
        QCOMPARE(tables[1].headers[5], std::string("CI lower"));
        QCOMPARE(tables[1].headers[6], std::string("CI upper"));
        QCOMPARE(tables[1].rows[0][0], std::string("First group"));
        QCOMPARE(tables[1].rows[1][0], std::string("Second group"));

        QCOMPARE(tables[2].rows[0][0], std::string("Discordant pairs b+c"));
        QCOMPARE(tables[2].rows[1][0], std::string("Valid pair count"));

        QCOMPARE(tables[3].headers[0], std::string("Index"));
        QCOMPARE(tables[3].headers[2], std::string("Actual"));
        QCOMPARE(tables[3].headers[4], std::string("Residual"));

        const auto& plot = localized.document.pages[0].visible_plots[0];
        QCOMPARE(plot.title, std::string("Interaction means plot"));
        QCOMPARE(plot.y_axis_title, std::string("Residual"));
        QCOMPARE(plot.x_axis_title, std::string("Factor A"));
    }

    void capability_gate_evidence_labels_localize_to_en_us()
    {
        const char* ids[] = {
            "evidence.johnson_capability_gated",
            "evidence.capability_stability_prerequisite",
            "evidence.reliability_all_censored",
            "evidence.km_not_long_term_guarantee",
            "evidence.warranty_not_legal_promise",
            "evidence.warranty_prediction_not_observation",
            "evidence.pass_fail_not_allowed",
        };
        for (const char* id : ids) {
            const auto en = resolve_report_text(id, "en-US");
            const auto zh = resolve_report_text(id, "zh-CN");
            QVERIFY(!en.text.empty());
            QVERIFY(!zh.text.empty());
            QVERIFY2(
                en.text.find("门禁") == std::string::npos
                    && en.text.find("合格") == std::string::npos
                    && en.text.find("删失") == std::string::npos
                    && en.text.find("保修") == std::string::npos,
                id);
            QVERIFY(zh.text != en.text);
        }
        QCOMPARE(
            resolve_report_text("evidence.johnson_capability_gated", "en-US").text,
            std::string(
                "Johnson capability gate (research preview; pass/fail blocked)"));
        QCOMPARE(
            resolve_report_text("evidence.rsm_insufficient_pure_error", "en-US").text,
            std::string(
                "RSM insufficient pure error; residual MS must not fake pure error"));
        QCOMPARE(
            resolve_report_text("evidence.rsm_lof_formula_reference", "en-US").text,
            std::string(
                "RSM lack-of-fit ANOVA (formula_reference; not vendor_oracle)"));
        QVERIFY(
            resolve_report_text("evidence.rsm_lof_formula_reference", "en-US")
                .text.find("失拟")
            == std::string::npos);
        QCOMPARE(
            resolve_report_text("evidence.design_formula_reference_only", "en-US").text,
            std::string(
                "DOE design generation is formula_reference only; not vendor_oracle / commercial alignment"));
        QCOMPARE(
            resolve_report_text("evidence.ccd_beyond_range", "en-US").text,
            std::string(
                "CCD axial points beyond cube range; not an executable process claim"));
        QCOMPARE(
            resolve_report_text("evidence.bbd_no_corners", "en-US").text,
            std::string(
                "BBD has no corner points; must not claim domain-wide prediction optimality"));
        QVERIFY(
            resolve_report_text("evidence.bbd_no_corners", "en-US").text.find("角点")
            == std::string::npos);
        QCOMPARE(
            resolve_report_text("evidence.cif_not_fine_gray", "en-US").text,
            std::string(
                "Aalen–Johansen CIF is not Fine-Gray; not cause-specific reliability / vendor_oracle"));
        QCOMPARE(
            resolve_report_text("evidence.fine_gray_formula_reference_only", "en-US").text,
            std::string(
                "Fine-Gray is formula_reference IPCW only; not cause-specific Cox / vendor_oracle / pinned R"));
        QCOMPARE(
            resolve_report_text("evidence.warranty_strata_exposure_honesty", "en-US").text,
            std::string(
                "Warranty strata exposure includes proportional allocation; must not claim measured denominators"));
        QVERIFY(
            resolve_report_text("evidence.warranty_strata_reliability_basis", "en-US")
                .text.find("池化")
            == std::string::npos);
        QCOMPARE(
            resolve_report_text("evidence.hexbin_rectangular_bins", "en-US").text,
            std::string(
                "Hexbin uses rectangular 2D bins; not a regular hexagonal tessellation"));
        QCOMPARE(
            resolve_report_text("evidence.density_curve_not_discrete_marks", "en-US").text,
            std::string(
                "Density KDE curves are not discrete row marks (continuous grid)"));
        QCOMPARE(
            resolve_report_text("diag.density_curve_not_discrete_marks", "en-US").text,
            std::string(
                "Density curves are continuous KDE grids, not discrete observation marks; "
                "selecting the curve does not map to individual worksheet rows "
                "(hidden/excluded still count separately for analysis vs display N)."));
        QCOMPARE(
            resolve_report_text("diag.hexbin_rectangular_bins", "en-US").text,
            std::string(
                "Using rectangular 2D bins (binned scatter); not a regular hexagonal tessellation."));
        QVERIFY(
            resolve_report_text("diag.hexbin_rectangular_bins", "en-US")
                .text.find("六边形")
            == std::string::npos);
    }

    void common_plot_axes_msa_doe_diag_localize_to_en_us()
    {
        OutputPage page = sample_page();
        datalab::domain::PlotSpec residual;
        residual.title = "残差与观测顺序";
        residual.x_axis_title = "观测顺序";
        residual.y_axis_title = "残差";
        datalab::domain::PlotSeries zero;
        zero.label = "残差 = 0";
        residual.series.push_back(zero);
        page.plots.push_back(residual);

        datalab::domain::PlotSpec qq;
        qq.x_axis_title = "理论分位数";
        qq.y_axis_title = "拟合值";
        page.plots.push_back(qq);

        datalab::domain::PlotSpec hist;
        hist.y_axis_title = "频数";
        hist.x_axis_title = "样本";
        page.plots.push_back(hist);

        datalab::domain::PlotSpec msa;
        msa.x_axis_title = "零件";
        msa.y_axis_title = "单元格均值";
        page.plots.push_back(msa);

        datalab::domain::PlotSpec bias;
        bias.x_axis_title = "参考值";
        datalab::domain::PlotSeries observed;
        observed.label = "观测偏倚";
        bias.series.push_back(observed);
        page.plots.push_back(bias);

        datalab::domain::PlotSpec normality;
        normality.x_axis_title = "理论标准正态分位数";
        normality.y_axis_title = "变换后测量";
        page.plots.push_back(normality);

        datalab::domain::PlotSpec std_norm;
        std_norm.x_axis_title = "标准正态分位数";
        std_norm.y_axis_title = "观测";
        page.plots.push_back(std_norm);

        datalab::domain::PlotSpec tukey;
        tukey.x_axis_title = "成对比较";
        tukey.y_axis_title = "均值差";
        page.plots.push_back(tukey);

        datalab::domain::PlotSpec coded;
        coded.x_axis_title = "A（编码）";
        coded.y_axis_title = "B（编码）";
        datalab::domain::PlotSeries edge;
        edge.label = "棱";
        coded.series.push_back(edge);
        page.plots.push_back(coded);

        datalab::domain::PlotSpec unknown;
        unknown.x_axis_title = "未知轴";
        unknown.y_axis_title = "未知轴";
        page.plots.push_back(unknown);

        datalab::domain::PlotSpec box;
        box.x_axis_title = "分组";
        box.y_axis_title = "观测序号";
        page.plots.push_back(box);

        datalab::domain::PlotSpec outlier;
        outlier.x_axis_title = "观测序";
        page.plots.push_back(outlier);

        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "cube_plot_requires_2_or_3_factors",
            "立方图仅支持 2 或 3 个因子（当前 5 个）；请用主效应图、交互图、等值线/曲面图查看高维设计。"});
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::warning,
            "invalid_contour_factors",
            "等值线/曲面的 X/Y 因子无效或相同；请指定两个不同的因子名。"});
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "contour_requires_two_factors",
            "等值线/曲面图需要至少两个连续因子。"});
        page.diagnostics.push_back({
            DiagnosticMessage::Severity::info,
            "doe_worksheet_export_ready",
            "已生成可写入工作表的设计矩阵（实际水平 + 空 Response 列）；"
            "UI 可一键替换活动工作表以录入响应。"});

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_plots = page.plots;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        const auto& plots = localized.document.pages[0].visible_plots;

        QCOMPARE(plots[0].title, std::string("Residuals vs observation order"));
        QCOMPARE(plots[0].x_axis_title, std::string("Observation order"));
        QCOMPARE(plots[0].y_axis_title, std::string("Residual"));
        QCOMPARE(plots[0].series[0].label, std::string("Residual = 0"));
        QCOMPARE(plots[1].x_axis_title, std::string("Theoretical quantile"));
        QCOMPARE(plots[1].y_axis_title, std::string("Fitted"));
        QCOMPARE(plots[2].y_axis_title, std::string("Frequency"));
        QCOMPARE(plots[2].x_axis_title, std::string("Sample"));
        QCOMPARE(plots[3].x_axis_title, std::string("Part"));
        QCOMPARE(plots[3].y_axis_title, std::string("Cell mean"));
        QCOMPARE(plots[4].x_axis_title, std::string("Reference"));
        QCOMPARE(plots[4].series[0].label, std::string("Observed bias"));
        QCOMPARE(
            plots[5].x_axis_title, std::string("Theoretical standard normal quantile"));
        QCOMPARE(plots[5].y_axis_title, std::string("Transformed measurement"));
        QCOMPARE(plots[6].x_axis_title, std::string("Standard normal quantile"));
        QCOMPARE(plots[6].y_axis_title, std::string("Observation"));
        QCOMPARE(plots[7].x_axis_title, std::string("Pairwise comparison"));
        QCOMPARE(plots[7].y_axis_title, std::string("Mean difference"));
        QCOMPARE(plots[8].x_axis_title, std::string("A (coded)"));
        QCOMPARE(plots[8].y_axis_title, std::string("B (coded)"));
        QCOMPARE(plots[8].series[0].label, std::string("Edge"));
        QCOMPARE(plots[9].x_axis_title, std::string("未知轴"));
        QCOMPARE(plots[10].x_axis_title, std::string("Group"));
        QCOMPARE(plots[10].y_axis_title, std::string("Observation index"));
        QCOMPARE(plots[11].x_axis_title, std::string("Obs. order"));
        QVERIFY(plots[0].x_axis_title.find("观测") == std::string::npos);
        QVERIFY(plots[8].x_axis_title.find("编码") == std::string::npos);

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(
            diags[0].message,
            std::string(
                "Cube plots support only 2 or 3 factors (currently 5); use main-effects, interaction, and contour/surface plots for higher dimensions."));
        QCOMPARE(
            diags[1].message,
            std::string(
                "Contour/surface X/Y factors are invalid or identical; specify two different factor names."));
        QCOMPARE(
            diags[2].message,
            std::string(
                "Contour/surface plots require at least two continuous factors."));
        QCOMPARE(
            diags[3].message,
            std::string(
                "Design matrix ready for worksheet write-back (actual levels + empty Response column); UI can replace the active worksheet to enter responses."));
        QVERIFY(diags[0].message.find("立方") == std::string::npos);
        QVERIFY(diags[3].message.find("工作表") == std::string::npos);
    }

    void dual_line_spc_attribute_shewhart_panel_titles_and_axes_localize_to_en_us()
    {
        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        const auto localize_page = [&](const std::string& title) {
            OutputPage page = sample_page();
            page.title = title;
            auto document = build_report_document(sample_table(), {page}, profile);
            document.pages[0].source_page.title = page.title;
            return localize_report_document(document).document.pages[0].source_page.title;
        };
        QCOMPARE(localize_page("P 图"), std::string("P chart"));
        QCOMPARE(localize_page("NP 图"), std::string("NP chart"));
        QCOMPARE(localize_page("C 图"), std::string("C chart"));
        QCOMPARE(localize_page("U 图"), std::string("U chart"));
        QCOMPARE(localize_page("G 图（几何间隔）"), std::string("G chart (geometric interval)"));
        QCOMPARE(localize_page("T 图（时间间隔）"), std::string("T chart (time interval)"));
        QVERIFY(localize_page("P 图").find("图") == std::string::npos);

        OutputPage page = sample_page();
        page.title = "I-MR 控制图";
        datalab::domain::PlotSpec individuals;
        individuals.title = "单值图 (I)";
        individuals.y_axis_title = "测量值";
        page.plots.push_back(individuals);
        datalab::domain::PlotSpec mr;
        mr.title = "移动极差图 (MR)";
        mr.y_axis_title = "移动极差";
        page.plots.push_back(mr);
        datalab::domain::PlotSpec i_sub;
        i_sub.title = "I 图（子组均值）";
        i_sub.y_axis_title = "子组均值";
        page.plots.push_back(i_sub);
        datalab::domain::PlotSpec r_chart;
        r_chart.title = "R 图";
        r_chart.y_axis_title = "子组极差";
        page.plots.push_back(r_chart);
        datalab::domain::PlotSpec s_chart;
        s_chart.title = "S 图";
        s_chart.y_axis_title = "子组标准差";
        page.plots.push_back(s_chart);
        datalab::domain::PlotSpec p_chart;
        p_chart.title = "P 图";
        p_chart.y_axis_title = "不合格品率";
        page.plots.push_back(p_chart);
        datalab::domain::PlotSpec u_chart;
        u_chart.title = "U 图";
        u_chart.y_axis_title = "单位缺陷数";
        page.plots.push_back(u_chart);
        datalab::domain::PlotSpec unknown;
        unknown.title = "未知自定义图 XYZ";
        unknown.y_axis_title = "未知轴";
        page.plots.push_back(unknown);

        datalab::domain::StatisticTable attr;
        attr.title = "P 图";
        attr.headers = {"原始行", "子组", "检验数", "单位数", "不合格品率", "不合格品数", "缺陷数", "单位缺陷数"};
        page.tables.push_back(attr);

        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].source_page.title = page.title;
        document.pages[0].visible_plots = page.plots;
        document.pages[0].visible_tables = page.tables;
        const auto localized = localize_report_document(document);
        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(plots[0].title, std::string("Individuals chart (I)"));
        QCOMPARE(plots[0].y_axis_title, std::string("Measurement"));
        QCOMPARE(plots[1].title, std::string("Moving range chart (MR)"));
        QCOMPARE(plots[1].y_axis_title, std::string("Moving range"));
        QCOMPARE(plots[2].title, std::string("I chart (subgroup mean)"));
        QCOMPARE(plots[2].y_axis_title, std::string("Subgroup mean"));
        QCOMPARE(plots[3].title, std::string("R chart"));
        QCOMPARE(plots[3].y_axis_title, std::string("Subgroup range"));
        QCOMPARE(plots[4].title, std::string("S chart"));
        QCOMPARE(plots[4].y_axis_title, std::string("Subgroup StDev"));
        QCOMPARE(plots[5].title, std::string("P chart"));
        QCOMPARE(plots[5].y_axis_title, std::string("Proportion defective"));
        QCOMPARE(plots[6].title, std::string("U chart"));
        QCOMPARE(plots[6].y_axis_title, std::string("Defects per unit"));
        QCOMPARE(plots[7].title, std::string("未知自定义图 XYZ"));
        QCOMPARE(plots[7].y_axis_title, std::string("未知轴"));
        QVERIFY(plots[0].title.find("单值") == std::string::npos);
        QVERIFY(plots[5].y_axis_title.find("不合格") == std::string::npos);

        const auto& table = localized.document.pages[0].visible_tables[0];
        QCOMPARE(table.title, std::string("P chart"));
        QCOMPARE(table.headers[2], std::string("Inspected count"));
        QCOMPARE(table.headers[3], std::string("Unit count"));
        QCOMPARE(table.headers[4], std::string("Proportion defective"));
        QCOMPARE(table.headers[5], std::string("Number defective"));
        QCOMPARE(table.headers[6], std::string("Defect count"));
        QCOMPARE(table.headers[7], std::string("Defects per unit"));
        QVERIFY(table.headers[3].find("单位") == std::string::npos);
        QVERIFY(table.headers[4].find("不合格") == std::string::npos);
    }

    void dual_line_table_titles_and_stability_diagnostics_localize()
    {
        OutputPage page = sample_page();
        datalab::domain::StatisticTable mode_denom;
        mode_denom.title = "失效模式分母追溯";
        mode_denom.headers = {"属性", "值"};
        page.tables.push_back(mode_denom);
        datalab::domain::StatisticTable group_denom;
        group_denom.title = "分组分母追溯";
        page.tables.push_back(group_denom);
        datalab::domain::StatisticTable lof;
        lof.title = "纯误差与失拟";
        page.tables.push_back(lof);
        datalab::domain::StatisticTable johnson;
        johnson.title = "Johnson 变换";
        page.tables.push_back(johnson);

        page.diagnostics.push_back({
            datalab::domain::DiagnosticMessage::Severity::warning,
            "capability_stability_screen_signals",
            "I-MR Rule-1 初筛检出 2 个超限点；能力指数仅供调查，不得写成过程合格。"});
        page.diagnostics.push_back({
            datalab::domain::DiagnosticMessage::Severity::warning,
            "capability_stability_screen_clear_not_verified",
            "I-MR Rule-1 初筛未检出超限点；这不是完整控制图验收，"
            "也不等于已验证稳定性/正态性，不得自动开放合格判定。"});
        page.diagnostics.push_back({
            datalab::domain::DiagnosticMessage::Severity::warning,
            "capability_stability_prerequisite",
            "能力分析稳定性前置：当前仅做公式参考级 I-MR Rule-1 初筛；"
            "合格判定保持关闭，直至独立稳定性与正态性验收工作流落地。"});
        page.diagnostics.push_back({
            datalab::domain::DiagnosticMessage::Severity::warning,
            "capability_pass_fail_blocked_by_stability_prerequisite",
            "正态能力未满足稳定性/正态性验收前置：禁止过程合格判定"
            "（pass_fail_judgment_allowed=false）。"});
        page.diagnostics.push_back({
            datalab::domain::DiagnosticMessage::Severity::warning,
            "warranty_stratum_exposure_proportional",
            "分层暴露量按 valid_count 比例分摊标量总暴露量；"
            "这不是实测分母，报告须标注 exposure_attribution=proportional_scalar。"});
        page.diagnostics.push_back({
            datalab::domain::DiagnosticMessage::Severity::error,
            "warranty_stratum_kind_mixed",
            "同一次分层摘要不得混用 failure_mode 与 group。"});

        datalab::domain::StatisticTable box_cap;
        box_cap.title = "变换后过程能力";
        page.tables.push_back(box_cap);

        datalab::domain::PlotSpec lambda_plot;
        lambda_plot.title = "Box-Cox λ 选择诊断";
        page.plots.push_back(lambda_plot);

        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "Cpk=1.2（稳定性前置未验收；禁止写成过程合格判定）。",
            "无重复编码点：未报告纯误差/失拟；不得用残差 MS 冒充纯误差。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_diagnostics = page.diagnostics;
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        QCOMPARE(
            localized.document.pages[0].visible_tables[0].title,
            std::string("Failure-mode denominator trace"));
        QCOMPARE(
            localized.document.pages[0].visible_tables[0].headers[0],
            std::string("Property"));
        QCOMPARE(
            localized.document.pages[0].visible_tables[1].title,
            std::string("Group denominator trace"));
        QCOMPARE(
            localized.document.pages[0].visible_tables[2].title,
            std::string("Pure error and lack of fit"));
        QCOMPARE(
            localized.document.pages[0].visible_tables[3].title,
            std::string("Johnson transform"));
        QCOMPARE(
            localized.document.pages[0].visible_tables[4].title,
            std::string("Capability after transform"));
        QCOMPARE(
            localized.document.pages[0].visible_plots.front().title,
            std::string("Box-Cox λ selection diagnostic"));

        bool saw_signals = false;
        bool saw_clear = false;
        bool saw_prereq = false;
        bool saw_blocked = false;
        bool saw_proportional = false;
        bool saw_kind_mixed = false;
        for (const auto& diagnostic : localized.document.pages[0].visible_diagnostics) {
            if (diagnostic.code == "capability_stability_screen_signals") {
                QVERIFY(diagnostic.message.find("found 2 out-of-control") != std::string::npos);
                saw_signals = true;
            }
            if (diagnostic.code == "capability_stability_screen_clear_not_verified") {
                QVERIFY(diagnostic.message.find("not a full control-chart") != std::string::npos);
                saw_clear = true;
            }
            if (diagnostic.code == "capability_stability_prerequisite") {
                QVERIFY(diagnostic.message.find("pass/fail stays closed") != std::string::npos);
                saw_prereq = true;
            }
            if (diagnostic.code == "capability_pass_fail_blocked_by_stability_prerequisite") {
                QVERIFY(diagnostic.message.find("pass/fail") != std::string::npos);
                saw_blocked = true;
            }
            if (diagnostic.code == "warranty_stratum_exposure_proportional") {
                QVERIFY(diagnostic.message.find("proportional_scalar") != std::string::npos);
                saw_proportional = true;
            }
            if (diagnostic.code == "warranty_stratum_kind_mixed") {
                QVERIFY(diagnostic.message.find("must not mix") != std::string::npos);
                saw_kind_mixed = true;
            }
        }
        QVERIFY(saw_signals);
        QVERIFY(saw_clear);
        QVERIFY(saw_prereq);
        QVERIFY(saw_blocked);
        QVERIFY(saw_proportional);
        QVERIFY(saw_kind_mixed);

        QVERIFY(!localized.document.pages[0].visible_interpretation.empty());
        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QCOMPARE(bullets.size(), static_cast<std::size_t>(2));
        QVERIFY(bullets[0].find("stability prerequisite not accepted") != std::string::npos);
        QVERIFY(bullets[0].find("Cpk=1.2") != std::string::npos);
        QVERIFY(bullets[1].find("residual MS") != std::string::npos);
        QVERIFY(bullets[1].find("pure error") != std::string::npos);

        // Source layer stays Chinese after localization.
        QCOMPARE(
            localized.document.pages[0].source_page.tables[0].title,
            std::string("失效模式分母追溯"));
        QCOMPARE(
            localized.document.pages[0].source_page.plots.front().title,
            std::string("Box-Cox λ 选择诊断"));
        QCOMPARE(
            facts_fingerprint(localized.document.pages[0].source_page.facts),
            facts_fingerprint(document.pages[0].source_page.facts));
    }

    void parameter_summary_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.parameter_summary =
            "变量 = Length    响应 = Y    方法 = Spearman    "
            "测量值 = M    因子数 k = 3    运行数 = 12    分辨度 = 5    "
            "变体 = inscribed    有效观测 = 20    "
            "分布 = 二项分布    p̄ = Σ不合格品数 / Σ检验数    "
            "过程变差(6σ) = 1.2    "
            "颜色范围固定为相关系数 [-1, 1]    方法 = Pearson    "
            "单元格为组内均值    "
            "面积表示相邻观测之间的数值区间，不是置信区间    "
            "类别列: Type    效应: A    模型 = 二项 OC    "
            "c̄ = 缺陷数均值    ū = Σ缺陷数 / Σ单位数";
        page.method_metadata.parameters = page.parameter_summary;

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Variable = Length") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Response = Y") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Method = Spearman") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Measurement = M") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Factor count k = 3") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Run count = 12") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Resolution = 5") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Variant = inscribed") != std::string::npos, summary.c_str());
        QVERIFY2(
            summary.find("Valid observations = 20") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Distribution = Binomial") != std::string::npos, summary.c_str());
        QVERIFY2(
            summary.find("p̄ = Σdefectives / Σinspections") != std::string::npos,
            summary.c_str());
        QVERIFY2(
            summary.find("Process variation (6σ) = 1.2") != std::string::npos, summary.c_str());
        QVERIFY2(
            summary.find("Color scale fixed to correlation [-1, 1]") != std::string::npos,
            summary.c_str());
        QVERIFY2(summary.find("Method = Pearson") != std::string::npos, summary.c_str());
        QVERIFY2(
            summary.find("Cells are within-group means") != std::string::npos, summary.c_str());
        QVERIFY2(
            summary.find("not a confidence interval") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Category column: Type") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Effect: A") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Model = Binomial OC") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("c̄ = mean defect count") != std::string::npos, summary.c_str());
        QVERIFY2(
            summary.find("ū = Σdefects / Σunits") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("变量") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("响应") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("测量值") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("二项分布") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("组内均值") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("不合格品数") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("缺陷数均值") == std::string::npos, summary.c_str());

        QCOMPARE(
            localized.document.pages[0].source_page.method_metadata.parameters, summary);

        // zh-CN keeps Chinese chrome.
        profile.locale.language_tag = "zh-CN";
        auto zh_document = build_report_document(sample_table(), {page}, profile);
        const auto zh_localized = localize_report_document(zh_document);
        const std::string& zh_summary =
            zh_localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(zh_summary.find("变量 = Length") != std::string::npos, zh_summary.c_str());
        QVERIFY2(zh_summary.find("二项分布") != std::string::npos, zh_summary.c_str());
        QVERIFY2(zh_summary.find("Variable =") == std::string::npos, zh_summary.c_str());
    }

    void chi_square_gof_expected_count_validity_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "卡方拟合优度";
        datalab::domain::InterpretationSection advice;
        advice.heading = "建议";
        advice.bullets = {
            "建议合并相邻类别后复算；当前 P 值可作为探索性证据。",
            "期望频数过低，建议先调整分组（合并类别）再进行拟合优度检验。",
        };
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析限制：存在期望频数小于 5 的类别，卡方近似可能不可靠。",
            "分析限制：低期望频数比例偏高，建议合并类别并谨慎解释 P 值。",
            "分析限制：期望频数过低，卡方近似可靠性较差。",
            "分析错误，以下结论不可用于决策：拟合优度至少需要两个类别，且类别与计数长度相同。",
            "分析限制：列联表至少需要两行两列，且标签数量必须匹配。",
            "存在期望频数小于 1 的单元格，卡方近似 P 值不显示。",
            "列联表总计数必须大于 0。",
        };
        page.interpretation = {advice, limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "expected_count_below_five",
             "存在期望频数小于 5 的类别，卡方近似可能不可靠。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "gof_validity_caution",
             "低期望频数比例偏高，建议合并类别并谨慎解释 P 值。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "gof_validity_poor",
             "期望频数过低，卡方近似可靠性较差。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_contingency_table",
             "列联表至少需要两行两列，且标签数量必须匹配。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "expected_count_below_one",
             "存在期望频数小于 1 的单元格，卡方近似 P 值不显示。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& sections = localized.document.pages[0].visible_interpretation;
        QCOMPARE(sections.size(), static_cast<std::size_t>(2));
        QCOMPARE(
            sections[0].bullets[0],
            std::string(
                "Consider merging adjacent categories and recomputing; "
                "the current P-value is exploratory evidence only."));
        QCOMPARE(
            sections[0].bullets[1],
            std::string(
                "Expected counts are too low; adjust grouping (merge categories) "
                "before goodness-of-fit testing."));
        QVERIFY2(
            sections[1].bullets[0].find("expected count < 5") != std::string::npos,
            sections[1].bullets[0].c_str());
        QVERIFY2(
            sections[1].bullets[0].find("Analysis limitation:") != std::string::npos,
            sections[1].bullets[0].c_str());
        QVERIFY2(
            sections[1].bullets[1].find("high share of low expected") != std::string::npos,
            sections[1].bullets[1].c_str());
        QVERIFY2(
            sections[1].bullets[2].find("poorly supported") != std::string::npos,
            sections[1].bullets[2].c_str());
        QVERIFY2(
            sections[1].bullets[3].find("at least two categories") != std::string::npos,
            sections[1].bullets[3].c_str());
        QVERIFY2(
            sections[1].bullets[3].find("Analysis error;") != std::string::npos,
            sections[1].bullets[3].c_str());
        QVERIFY2(
            sections[1].bullets[4].find("two rows and two columns") != std::string::npos,
            sections[1].bullets[4].c_str());
        QCOMPARE(
            sections[1].bullets[5],
            std::string(
                "Some cells have expected count < 1; "
                "chi-square approximate P-values are not shown."));
        QCOMPARE(
            sections[1].bullets[6],
            std::string("Contingency-table total count must be greater than 0."));

        // Unknown diagnostic detail must stay fully Chinese (no EN prefix + ZH body).
        limitations.bullets.push_back(
            "分析限制：尚未登记的诊断原文应整句保留中文。");
        document.pages[0].visible_interpretation = {advice, limitations};
        const auto mixed_guard = localize_report_document(document);
        const auto& guarded =
            mixed_guard.document.pages[0].visible_interpretation[1].bullets.back();
        QCOMPARE(
            guarded,
            std::string("分析限制：尚未登记的诊断原文应整句保留中文。"));
        QVERIFY2(guarded.find("Analysis limitation:") == std::string::npos, guarded.c_str());

        for (const auto& bullet : sections[0].bullets) {
            QVERIFY2(bullet.find("建议") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("期望频数") == std::string::npos, bullet.c_str());
        }
        for (const auto& bullet : sections[1].bullets) {
            QVERIFY2(bullet.find("期望频数") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("列联表") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("拟合优度") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(
            diags[0].message,
            std::string(
                "Some categories have expected count < 5; "
                "the chi-square approximation may be unreliable."));
        QCOMPARE(
            diags[1].message,
            std::string(
                "A high share of low expected counts; consider merging categories "
                "and interpret P-values cautiously."));
        QCOMPARE(
            diags[2].message,
            std::string(
                "Expected counts are too low; "
                "the chi-square approximation is poorly supported."));
        QCOMPARE(
            diags[3].message,
            std::string(
                "The contingency table needs at least two rows and two columns, "
                "with matching label counts."));
        QCOMPARE(
            diags[4].message,
            std::string(
                "Some cells have expected count < 1; "
                "chi-square approximate P-values are not shown."));
    }

    void association_mcnemar_cochran_proportion_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "McNemar";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：McNemar 要求两列配对标签等长且非空。",
            "分析限制：存在全失败或全成功的处理列，解读需谨慎。",
            "分析错误，以下结论不可用于决策：Cochran Q 要求至少 3 个处理列；两列配对请用 McNemar。",
            "分析限制：事件数或非事件数小于 5，正态近似可能不准确；请参考 Fisher 精确检验。",
            "无不一致对（b+c=0），McNemar 统计量不可计算。",
            "比例标准误为 0，无法计算正态近似检验。",
            "已将多行事件数/试验数求和后再做单比例检验。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "mcnemar_input_mismatch",
             "McNemar 要求两列配对标签等长且非空。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "cochran_sparse_column",
             "存在全失败或全成功的处理列，解读需谨慎。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "cochran_use_mcnemar",
             "Cochran Q 要求至少 3 个处理列；两列配对请用 McNemar。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "small_count_normal_approximation",
             "事件数或非事件数小于 5，正态近似可能不准确；请参考 Fisher 精确检验。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "small_count_normal_approximation",
             "n p0 或 n(1-p0) 小于 5，正态近似可能不准确。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "summarized_from_multiple_rows",
             "已将多行缺陷数/观测长度求和后再做单样本泊松率检验。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "fisher_empty_table",
             "2×2 表总计数必须大于 0。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("Analysis error;") != std::string::npos, bullets[0].c_str());
        QVERIFY2(
            bullets[0].find("equal, non-empty length") != std::string::npos, bullets[0].c_str());
        QVERIFY2(
            bullets[1].find("Analysis limitation:") != std::string::npos, bullets[1].c_str());
        QVERIFY2(
            bullets[1].find("all failures or all successes") != std::string::npos,
            bullets[1].c_str());
        QVERIFY2(bullets[2].find("at least three treatment") != std::string::npos, bullets[2].c_str());
        QVERIFY2(bullets[3].find("Fisher") != std::string::npos, bullets[3].c_str());
        QCOMPARE(
            bullets[4],
            std::string(
                "No discordant pairs (b+c=0); the McNemar statistic cannot be computed."));
        QCOMPARE(
            bullets[5],
            std::string(
                "Proportion standard error is 0; "
                "the normal-approximation test cannot be computed."));
        QCOMPARE(
            bullets[6],
            std::string(
                "Events/trials from multiple rows were summed before the one-proportion test."));

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("McNemar 要求") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("全失败") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("标准误") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(
            diags[0].message,
            std::string(
                "McNemar requires two paired label columns of equal, non-empty length."));
        QCOMPARE(
            diags[1].message,
            std::string(
                "Some treatment columns are all failures or all successes; "
                "interpret with caution."));
        QCOMPARE(
            diags[2].message,
            std::string(
                "Cochran Q needs at least three treatment columns; "
                "for two paired columns use McNemar."));
        QVERIFY2(diags[3].message.find("Fisher") != std::string::npos, diags[3].message.c_str());
        QCOMPARE(
            diags[4].message,
            std::string(
                "n·p0 or n·(1−p0) is less than 5; "
                "the normal approximation may be inaccurate."));
        QVERIFY2(
            diags[5].message.find("Poisson rate") != std::string::npos, diags[5].message.c_str());
        QCOMPARE(
            diags[6].message,
            std::string("The 2×2 table total count must be greater than 0."));

        // Shared diagnostic.code must not invent a wrong English body.
        QVERIFY(diags[3].message != diags[4].message);
    }

    void nonparametric_family_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "Mann-Whitney";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：Mann–Whitney 检验要求两组均有有效观测。",
            "分析限制：存在样本量小于 10 的组，Mann–Whitney 正态近似只作提示。",
            "分析错误，以下结论不可用于决策：Friedman 检验至少需要 2 个处理与 2 个区组。",
            "排除小组后不足两个组，无法计算 Mood 中位数检验。",
            "游程检验至少需要 2 个有限观测。",
            "存在相邻相等点：按产品锁定计入下行游程（平坦差分归下行）。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_samples",
             "Mann–Whitney 检验要求两组均有有效观测。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "small_sample_normal_approximation",
             "存在样本量小于 10 的组，Mann–Whitney 正态近似只作提示。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "small_sample_normal_approximation",
             "非零差值少于 10，Wilcoxon 正态近似只作提示。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "friedman_insufficient_levels",
             "Friedman 检验至少需要 2 个处理与 2 个区组。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "mood_insufficient_groups",
             "排除小组后不足两个组，无法计算 Mood 中位数检验。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "runs_insufficient_n",
             "游程检验至少需要 2 个有限观测。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "run_chart_flat_as_down",
             "存在相邻相等点：按产品锁定计入下行游程（平坦差分归下行）。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "zero_rank_variance",
             "秩方差为 0，无法计算 Mann–Whitney P 值。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "zero_rank_variance",
             "符号秩方差为 0，无法计算 Wilcoxon P 值。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("Analysis error;") != std::string::npos, bullets[0].c_str());
        QVERIFY2(
            bullets[0].find("valid observations in both groups") != std::string::npos,
            bullets[0].c_str());
        QVERIFY2(bullets[1].find("advisory only") != std::string::npos, bullets[1].c_str());
        QVERIFY2(
            bullets[2].find("2 treatments and 2 blocks") != std::string::npos, bullets[2].c_str());
        QCOMPARE(
            bullets[3],
            std::string(
                "Fewer than two groups remain after dropping small groups; "
                "Mood median test cannot be computed."));
        QCOMPARE(
            bullets[4],
            std::string("Runs test needs at least 2 finite observations."));
        QVERIFY2(bullets[5].find("flat diffs as down") != std::string::npos, bullets[5].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("检验要求") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("游程") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(
            diags[0].message,
            std::string("Mann–Whitney requires valid observations in both groups."));
        QVERIFY2(diags[1].message.find("Mann–Whitney") != std::string::npos, diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("Wilcoxon") != std::string::npos, diags[2].message.c_str());
        QVERIFY(diags[1].message != diags[2].message);
        QCOMPARE(
            diags[3].message,
            std::string("Friedman needs at least 2 treatments and 2 blocks."));
        QVERIFY2(diags[4].message.find("Mood median") != std::string::npos, diags[4].message.c_str());
        QCOMPARE(
            diags[5].message,
            std::string("Runs test needs at least 2 finite observations."));
        QVERIFY2(diags[6].message.find("down runs") != std::string::npos, diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("Mann–Whitney P-value") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("Wilcoxon P-value") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY(diags[7].message != diags[8].message);
    }

    void msa_gage_rr_family_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "Gage R&R";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：Crossed Gage R&R 至少需要两个零件和两个操作员。",
            "分析限制：ndc<5 只作为调查提示，不是量具不合格的绝对结论。",
            "分析限制：ndc<5 只作为调查提示。",
            "分析错误，以下结论不可用于决策：Nested Gage R&R 中每个零件必须只属于一个操作员。",
            "方差分量原始估计为负，已截断为 0。",
            "本轮 Expanded 仅支持完整平衡交叉；缺失格子请用交叉 Gage 或后续 GLM。",
            "重复次数=1：三阶交互用作重复性误差估计。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_gage_levels",
             "Crossed Gage R&R 至少需要两个零件和两个操作员。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "ndc_investigation",
             "ndc<5 只作为调查提示，不是量具不合格的绝对结论。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "ndc_investigation",
             "ndc<5 只作为调查提示。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "part_has_multiple_operators",
             "Nested Gage R&R 中每个零件必须只属于一个操作员。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "negative_variance_component",
             "方差分量原始估计为负，已截断为 0。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "unbalanced_or_incomplete",
             "本轮 Expanded 仅支持完整平衡交叉；缺失格子请用交叉 Gage 或后续 GLM。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "three_way_as_error",
             "重复次数=1：三阶交互用作重复性误差估计。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "expanded_scoped_3factor",
             "本命令为平衡三因子随机 Expanded；不平衡/固定效应/嵌套 GLM 仍延后。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_tolerance",
             "公差必须为有限非负数；NaN、无穷或负数不可用。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_tolerance",
             "公差必须为有限非负数。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("two parts and two operators") != std::string::npos,
                 bullets[0].c_str());
        QVERIFY2(bullets[1].find("not an absolute gage-fail") != std::string::npos,
                 bullets[1].c_str());
        QCOMPARE(
            bullets[2],
            std::string("Analysis limitation: ndc<5 is an investigation hint only."));
        QVERIFY2(bullets[3].find("only one operator") != std::string::npos, bullets[3].c_str());
        QCOMPARE(
            bullets[4],
            std::string("Negative variance-component estimate(s) were truncated to 0."));
        QVERIFY2(bullets[5].find("complete balanced crossed") != std::string::npos,
                 bullets[5].c_str());
        QVERIFY2(bullets[6].find("three-way interaction") != std::string::npos, bullets[6].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("量具不合格") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("方差分量") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("调查提示") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY2(diags[0].message.find("Crossed Gage R&R") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("absolute gage-fail") != std::string::npos,
                 diags[1].message.c_str());
        QCOMPARE(diags[2].message, std::string("ndc<5 is an investigation hint only."));
        QVERIFY(diags[1].message != diags[2].message);
        QVERIFY2(diags[3].message.find("Nested Gage R&R") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("truncated to 0") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("Expanded") != std::string::npos, diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("Replicates=1") != std::string::npos, diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("remains deferred") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("NaN") != std::string::npos, diags[8].message.c_str());
        QCOMPARE(
            diags[9].message,
            std::string("Tolerance must be a finite non-negative number."));
        QVERIFY(diags[8].message != diags[9].message);
    }

    void msa_type1_bias_linearity_stability_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "MSA Type 1";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：Type 1 Gage 至少需要两条有限测量值，参考值和公差必须有效。",
            "分析限制：重复性为零时 t、p 和置信区间不可用，不输出 p=0。",
            "Bias/Linearity 要求至少三组且参考值与测量值长度一致。",
            "未提供过程变差（6×过程标准差），Linearity / %Linearity / %Bias 未计算。",
            "Stability 至少需要三条测量值。",
            "请选择测量值列，并在配置中提供参考值。",
            "请选择测量值列。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_msa_type1_input",
             "Type 1 Gage 至少需要两条有限测量值，参考值和公差必须有效。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "zero_repeatability",
             "重复性为零时 t、p 和置信区间不可用，不输出 p=0。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_bias_linearity_shape",
             "Bias/Linearity 要求至少三组且参考值与测量值长度一致。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "process_variation_not_provided",
             "未提供过程变差（6×过程标准差），Linearity / %Linearity / %Bias 未计算。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_stability_input",
             "Stability 至少需要三条测量值。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_msa_type1_value",
             "测量值必须为有限数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "zero_reference_range",
             "参考值必须包含至少两个不同水平。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_process_variation",
             "过程变差必须为有限正数（6×过程标准差）。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("Type 1 Gage requires") != std::string::npos, bullets[0].c_str());
        QVERIFY2(bullets[1].find("p=0 is not reported") != std::string::npos, bullets[1].c_str());
        QVERIFY2(bullets[2].find("at least three pairs") != std::string::npos, bullets[2].c_str());
        QVERIFY2(bullets[3].find("were not computed") != std::string::npos, bullets[3].c_str());
        QCOMPARE(
            bullets[4],
            std::string("Stability requires at least three measurements."));
        QVERIFY2(
            bullets[5].find("measurement column") != std::string::npos, bullets[5].c_str());
        QCOMPARE(bullets[6], std::string("Select a measurement column."));

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("测量值") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("过程变差") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("重复性") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY2(diags[0].message.find("Type 1 Gage requires") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("repeatability is zero") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("Bias/Linearity") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("not provided") != std::string::npos,
                 diags[3].message.c_str());
        QCOMPARE(
            diags[4].message,
            std::string("Stability requires at least three measurements."));
        QCOMPARE(diags[5].message, std::string("Measurements must be finite."));
        QVERIFY2(diags[6].message.find("two distinct levels") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("6× process standard deviation") != std::string::npos,
                 diags[7].message.c_str());
    }

    void attribute_agreement_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "属性一致性分析";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：属性一致性至少需要两个项目和两个评估者。",
            "分析限制：评估者间 Kendall W 不可识别；不伪造 W=1。",
            "请选择评级、部件和评估者列。",
            "加权 Kappa 需要可排序的数值评级；已回退未加权 Cohen。",
            "期望一致率 P_expected=1，Kappa 不可识别，不计算无限标准误。",
            "Fleiss overall Kappa 保持未加权；linear/quadratic 仅用于两两 Cohen。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_attribute_levels",
             "属性一致性至少需要两个项目和两个评估者。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "kendall_w_unidentifiable",
             "评估者间 Kendall W 不可识别；不伪造 W=1。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_attribute_agreement_shape",
             "属性一致性要求列长度一致，标准列为空或与记录数一致，置信度必须在 0 与 1 之间。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "ordinal_ratings_unranked",
             "加权 Kappa 需要可排序的数值评级；已回退未加权 Cohen。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "kappa_unidentifiable",
             "期望一致率 P_expected=1，Kappa 不可识别，不计算无限标准误。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "fleiss_unweighted",
             "Fleiss overall Kappa 保持未加权；linear/quadratic 仅用于两两 Cohen。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "unequal_replicates",
             "评估者重复次数不一致；不等长配对已排除，不会静默截断。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "unknown_kappa_weight_scheme",
             "未知的 kappa_weight_scheme，已回退为 none。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("two items and two appraisers") != std::string::npos,
                 bullets[0].c_str());
        QVERIFY2(bullets[1].find("W=1 is not fabricated") != std::string::npos,
                 bullets[1].c_str());
        QCOMPARE(
            bullets[2],
            std::string("Select rating, part, and appraiser columns."));
        QVERIFY2(bullets[3].find("unweighted Cohen") != std::string::npos, bullets[3].c_str());
        QVERIFY2(bullets[4].find("not identifiable") != std::string::npos, bullets[4].c_str());
        QVERIFY2(bullets[5].find("Fleiss overall Kappa") != std::string::npos, bullets[5].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("属性一致性") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("不伪造") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("评估者") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY2(diags[0].message.find("two items and two appraisers") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("not fabricated") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("equal column lengths") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("Weighted Kappa") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("P_expected=1") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("pairwise Cohen") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("silent truncation") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("fell back to none") != std::string::npos,
                 diags[7].message.c_str());
    }

    void linear_regression_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "线性回归";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：回归至少需要两个观测、一个响应列和一个预测列。",
            "分析限制：VIF>5 提示共线性调查，不会自动删除预测变量。",
            "设计矩阵秩亏，已拒绝拟合。",
            "秩亏时不要解释系数；先检查常量列或完全共线预测变量。",
            "Durbin-Watson 临界界不可用（需要 15≤n≤100 且 1≤k'≤5）。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_regression_shape",
             "回归至少需要两个观测、一个响应列和一个预测列。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "collinearity_investigation",
             "VIF>5 提示共线性调查，不会自动删除预测变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "rank_deficient_design",
             "设计矩阵存在完全共线或常量预测变量。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "durbin_watson_bounds_unavailable",
             "Durbin-Watson 临界界不可用（需要 15≤n≤100 且 1≤k'≤5）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "no_error_degrees_of_freedom",
             "误差自由度 N-p-1 ≤ 0，不输出 t、F 与 P。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "regression_seq_adj_may_differ",
             "多预测变量回归应同时查看 Seq SS 与 Adj SS；两者不一致常见于不平衡或共线设计。"},
        };

        datalab::domain::StatisticTable assumptions;
        assumptions.title = "假设检查";
        assumptions.headers = {"检查项", "状态", "统计量", "P-Value", "说明"};
        assumptions.rows.push_back({
            "residual_independence", "not_computed", "*", "*",
            "Durbin-Watson 按输入顺序；判定区对照 α=0.05 近似 dL/dU，不能写成已证明无自相关。"});
        assumptions.rows.push_back({
            "homoscedasticity", "not_verified", "*", "*",
            "方差齐性主要依据残差对拟合值图，当前不单独给出数值判定。"});
        page.tables.push_back(assumptions);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        document.pages[0].visible_tables = page.tables;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("at least two observations") != std::string::npos,
                 bullets[0].c_str());
        QVERIFY2(bullets[1].find("not removed automatically") != std::string::npos,
                 bullets[1].c_str());
        QCOMPARE(
            bullets[2],
            std::string("The design matrix is rank-deficient; the fit was rejected."));
        QVERIFY2(bullets[3].find("rank-deficient") != std::string::npos, bullets[3].c_str());
        QVERIFY2(bullets[4].find("15≤n≤100") != std::string::npos, bullets[4].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("回归至少") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("共线性") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("秩亏") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY2(diags[0].message.find("one predictor column") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("VIF>5") != std::string::npos, diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("perfect collinearity") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("Durbin") != std::string::npos, diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("N-p-1") != std::string::npos, diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("Seq SS") != std::string::npos, diags[5].message.c_str());

        QVERIFY(!localized.document.pages[0].visible_tables.empty());
        const auto& cells = localized.document.pages[0].visible_tables.front().rows;
        QVERIFY2(
            cells[0][4].find("must not be written as proven no autocorrelation")
                != std::string::npos,
            cells[0][4].c_str());
        QVERIFY2(
            cells[1][4].find("residual-vs-fitted") != std::string::npos, cells[1][4].c_str());
        QVERIFY2(cells[0][4].find("不能写成") == std::string::npos, cells[0][4].c_str());
        QVERIFY2(cells[1][4].find("方差齐性") == std::string::npos, cells[1][4].c_str());
    }

    void anova_tukey_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "单因素方差分析";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：单因素 ANOVA 至少需要两个有效组。",
            "分析限制：因子组合的重复数不平衡；Seq SS 与 Adj SS 可能不同。",
            "请选择因子/分组列。",
            "不要把逐比较 alpha 当成家族错误率。",
            "当前 Tukey 调整使用 Studentized range 的保守 t 分布近似。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_groups",
             "单因素 ANOVA 至少需要两个有效组。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "unbalanced_design",
             "因子组合的重复数不平衡；Seq SS 与 Adj SS 可能不同。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_groups",
             "Tukey 比较至少需要两个且标签数量必须匹配的有效组。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "tukey_studentized_range_approximation",
             "当前 Tukey 调整使用 Studentized range 的保守 t 分布近似。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "interval_unavailable",
             "误差自由度或 MSE 不可用，未输出组均值区间。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "zero_error_ss",
             "组内误差平方和为 0，无法计算有限 F 统计量。"},
        };

        datalab::domain::StatisticTable assumptions;
        assumptions.title = "假设检查";
        assumptions.headers = {"检查项", "状态", "统计量", "P-Value", "说明"};
        assumptions.rows.push_back({
            "residual_normality", "not_verified", "*", "*",
            "ANOVA 残差正态性只能拒绝或未拒绝假设，不能证明组内误差正态。"});
        assumptions.rows.push_back({
            "homogeneity", "not_verified", "*", "*",
            "Levene 检验只作为方差齐性调查证据，不能单独决定模型是否可用。"});
        page.tables.push_back(assumptions);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        document.pages[0].visible_tables = page.tables;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("at least two valid groups") != std::string::npos,
                 bullets[0].c_str());
        QVERIFY2(bullets[1].find("Seq SS and Adj SS") != std::string::npos, bullets[1].c_str());
        QCOMPARE(bullets[2], std::string("Select a factor / grouping column."));
        QVERIFY2(bullets[3].find("family-wise error rate") != std::string::npos,
                 bullets[3].c_str());
        QVERIFY2(bullets[4].find("studentized range") != std::string::npos, bullets[4].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("单因素") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("因子组合") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("家族错误率") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY2(diags[0].message.find("One-way ANOVA") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("unbalanced") != std::string::npos, diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("Tukey") != std::string::npos, diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("conservative t-distribution") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("group-mean intervals") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("finite F statistic") != std::string::npos,
                 diags[5].message.c_str());

        const auto& cells = localized.document.pages[0].visible_tables.front().rows;
        QVERIFY2(cells[0][4].find("does not prove") != std::string::npos, cells[0][4].c_str());
        QVERIFY2(cells[1][4].find("investigative evidence") != std::string::npos,
                 cells[1][4].c_str());
        QVERIFY2(cells[0][4].find("不能证明") == std::string::npos, cells[0][4].c_str());
    }

    void doe_factorial_ccd_bbd_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "DOE 响应分析";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：生成器格式应为 D=ABC;E=ABD。",
            "分析限制：没有重复运行，无法估计纯误差和失拟。",
            "BBD 不包含所有因素同时处于极端水平的角点；这是设计空间边界，不是实现缺陷。",
            "请选择响应列和至少一个设计因子列。",
            "CCC 星点超出原始因素范围；请允许超范围星点，或改用 CCI/CCF。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_generator_format",
             "生成器格式应为 D=ABC;E=ABD。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "no_pure_error",
             "没有重复运行，无法估计纯误差和失拟。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "bbd_no_corners",
             "BBD 不包含所有因素同时处于极端水平的角点；这是设计空间边界，不是实现缺陷。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "ccd_ccc_beyond_range",
             "CCC 星点超出原始因素范围；请允许超范围星点，或改用 CCI/CCF。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "ccd_ccc_beyond_allowed",
             "CCC 星点超出原始 low/high；已按允许超范围策略生成，实验可行性需人工确认。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "bbd_factor_count",
             "BBD 第一阶段支持 3–7 个连续因素；2 因素不接受。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "rsd_categorical_blocked",
             "第一阶段只接受连续因素；分类因素不得静默编码。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "lof_df_zero",
             "纯误差占用了全部残差自由度，无法检验失拟。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "center_imported",
             "已按 PointType/中心水平导入中心点运行，可用于曲率与纯误差。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("D=ABC;E=ABD") != std::string::npos, bullets[0].c_str());
        QVERIFY2(bullets[1].find("pure error and lack of fit") != std::string::npos,
                 bullets[1].c_str());
        QVERIFY2(bullets[2].find("design-space boundary") != std::string::npos,
                 bullets[2].c_str());
        QVERIFY2(bullets[2].find("not an implementation defect") != std::string::npos,
                 bullets[2].c_str());
        QCOMPARE(
            bullets[3],
            std::string("Select a response column and at least one design factor column."));
        QVERIFY2(bullets[4].find("CCI/CCF") != std::string::npos, bullets[4].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("生成器") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("纯误差") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("实现缺陷") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY2(diags[0].message.find("Generator format") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("No replicate runs") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("BBD omits corners") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("beyond-range") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("human confirmation") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("3–7 continuous") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("not be silently coded") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("lack of fit cannot be tested") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("PointType") != std::string::npos, diags[8].message.c_str());
    }

    void rsm_analysis_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "响应曲面分析";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：RSM 需要响应与因子行数一致，且至少两个因子。",
            "分析限制：没有重复编码点，无法估计纯误差和失拟；不得用残差 MS 冒充纯误差。",
            "失拟 ANOVA 证据类型 formula_reference；不是 vendor_oracle。",
            "因子已按设计 low/high/center 编码（与 CCD/BBD 一致）。",
            "等值线需要两个不同因子且模型已拟合。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "rsm_invalid_shape",
             "RSM 需要响应与因子行数一致，且至少两个因子。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "rsm_insufficient_pure_error",
             "没有重复编码点，无法估计纯误差和失拟；不得用残差 MS 冒充纯误差。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "rsm_lof_formula_reference",
             "失拟 ANOVA 证据类型 formula_reference；不是 vendor_oracle。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "rsm_design_bounds_coded",
             "因子已按设计 low/high/center 编码（与 CCD/BBD 一致）。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "rsm_invalid_grid",
             "等值线需要两个不同因子且模型已拟合。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "rsm_already_coded",
             "因子值均在 [-1,1]，按已编码单位拟合 RSM。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "rsm_pe_exceeds_residual",
             "纯误差自由度大于残差自由度，跳过失拟分解（可能模型过参或重复点过多）。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "rsm_replicated_points",
             "检测到重复编码点，已用于纯误差 / 失拟估计。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("at least two factors") != std::string::npos,
                 bullets[0].c_str());
        QVERIFY2(bullets[1].find("residual MS as pure error") != std::string::npos,
                 bullets[1].c_str());
        QVERIFY2(bullets[2].find("formula_reference") != std::string::npos, bullets[2].c_str());
        QVERIFY2(bullets[2].find("not vendor_oracle") != std::string::npos, bullets[2].c_str());
        QVERIFY2(bullets[3].find("CCD/BBD") != std::string::npos, bullets[3].c_str());
        QVERIFY2(bullets[4].find("two distinct factors") != std::string::npos, bullets[4].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("残差 MS") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("不是 vendor_oracle") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("等值线需要") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY2(diags[0].message.find("matching response") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("No replicated coded points") != std::string::npos,
                 diags[1].message.c_str());
        QCOMPARE(
            diags[2].message,
            std::string(
                "Lack-of-fit ANOVA evidence_type is formula_reference; not vendor_oracle."));
        QVERIFY2(diags[3].message.find("low/high/center") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("fitted model") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("[-1,1]") != std::string::npos, diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("skipping lack-of-fit") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("pure-error") != std::string::npos, diags[7].message.c_str());
    }

    void reliability_km_weibull_logrank_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "可靠性分析";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：全为删失，无法识别 Weibull 参数。",
            "分析限制：最大观测为删失，尾部生存函数不可估计到 0。",
            "三参数 Weibull 似然无界（常见于形状 ≤ 1）；不估计阈值，也不伪造参数。",
            "Log-rank 检验至少需要两个非空分组。",
            "两参数指数未能找到有限阈值估计；不伪造参数。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "weibull_all_censored",
             "全为删失，无法识别 Weibull 参数。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "km_tail_censored",
             "最大观测为删失，尾部生存函数不可估计到 0。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "weibull3_unbounded",
             "三参数 Weibull 似然无界（常见于形状 ≤ 1）；不估计阈值，也不伪造参数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "logrank_need_groups",
             "Log-rank 检验至少需要两个非空分组。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "exp2_no_threshold",
             "两参数指数未能找到有限阈值估计；不伪造参数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "rel_shape",
             "寿命和删失指示列长度必须一致且至少包含两条记录。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "km_all_censored",
             "全为删失，无法识别失效分布或中位寿命。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "lognormal3_unbounded",
             "三参数对数正态似然无界；不估计阈值，也不伪造参数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "weibull_one_failure",
             "只有一条失效记录时 Weibull 形状参数通常不可稳定识别。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("Weibull parameters are not identifiable")
                     != std::string::npos,
                 bullets[0].c_str());
        QVERIFY2(bullets[1].find("may not reach 0") != std::string::npos, bullets[1].c_str());
        QVERIFY2(bullets[2].find("parameters are not invented") != std::string::npos,
                 bullets[2].c_str());
        QVERIFY2(bullets[3].find("two non-empty groups") != std::string::npos, bullets[3].c_str());
        QVERIFY2(bullets[4].find("not invented") != std::string::npos, bullets[4].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("全为删失") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("不伪造参数") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("尾部生存") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY2(diags[0].message.find("not identifiable") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("largest observation is censored") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("not invented") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("Log-rank") != std::string::npos, diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("Two-parameter exponential") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("at least two records") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("median life") != std::string::npos, diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("lognormal likelihood is unbounded") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("only one failure") != std::string::npos,
                 diags[8].message.c_str());
    }

    void glm_logistic_poisson_stepwise_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "广义线性模型";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：二元 Logistic 回归至少需要三个观测和一个预测变量。",
            "分析限制：IRLS 在最大迭代次数内未达到收敛容差。",
            "Hosmer–Lemeshow 在样本量不足、未收敛或完全分离时不计算。",
            "α_remove < α_enter 可能导致振荡；仍继续。",
            "未选入任何预测变量（仅截距）。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::info,
             "hosmer_lemeshow_not_computed",
             "Hosmer–Lemeshow 在样本量不足、未收敛或完全分离时不计算。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "hosmer_lemeshow_not_computed",
             "Hosmer–Lemeshow 分组期望方差过小，检验不可用。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "hosmer_lemeshow_not_computed",
             "Hosmer–Lemeshow 有效组数不足，检验不可用。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "logistic_need_obs",
             "二元 Logistic 回归至少需要三个观测和一个预测变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "logistic_options",
             "置信水平、收敛容差和最大迭代次数必须有效。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "logistic_labels",
             "预测变量标签数量必须与预测变量列数一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "logistic_binary",
             "响应变量必须全部为 0 或 1。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "logistic_obs_params",
             "Logistic 回归需要多于参数数量的观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "logistic_rank",
             "Logistic 信息矩阵秩亏，无法进行 IRLS 更新。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "logistic_eta",
             "线性预测量达到数值稳定性边界，概率已进行安全截断。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "logistic_irls",
             "IRLS 在最大迭代次数内未达到收敛容差。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "logistic_sep",
             "预测变量完全分离了 0/1 响应，极大似然估计可能不存在。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "logistic_quasi",
             "预测概率达到边界，模型可能存在准分离。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "logistic_cov",
             "无法计算 Logistic 系数协方差矩阵。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "poisson_need",
             "Poisson 回归至少需要三个观测和一个预测变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "poisson_resp",
             "响应必须为非负有限值，且预测矩阵规整。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "poisson_pred",
             "预测变量含非有限值。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "poisson_singular",
             "IRLS 信息矩阵奇异。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "max_iter",
             "已达最大迭代次数。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "poisson_cov",
             "无法计算系数协方差。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "stepwise_need",
             "逐步回归需要 ≥4 观测与 ≥2 候选预测变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "stepwise_alpha",
             "α_enter / α_remove 必须在 (0,1)。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "stepwise_order",
             "α_remove < α_enter 可能导致振荡；仍继续。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "stepwise_intercept",
             "未选入任何预测变量（仅截距）。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "ordinal_need",
             "有序 Logistic 需要 ≥10 观测、≥3 有序水平与 ≥1 预测变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "ordinal_code",
             "响应类别编码越界。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "ordinal_singular",
             "有序 Logistic 信息矩阵奇异。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("at least three observations") != std::string::npos,
                 bullets[0].c_str());
        QVERIFY2(bullets[1].find("convergence tolerance") != std::string::npos,
                 bullets[1].c_str());
        QVERIFY2(bullets[2].find("Hosmer") != std::string::npos, bullets[2].c_str());
        QVERIFY2(bullets[3].find("oscillation") != std::string::npos, bullets[3].c_str());
        QVERIFY2(bullets[4].find("intercept only") != std::string::npos, bullets[4].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("二元 Logistic") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("未选入") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("检验不可用") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY(diags.size() == 27);
        QVERIFY2(diags[0].message.find("not computed") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("too small") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("Too few usable") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("Binary logistic") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[10].message.find("IRLS did not reach") != std::string::npos,
                 diags[10].message.c_str());
        QVERIFY2(diags[11].message.find("completely separate") != std::string::npos,
                 diags[11].message.c_str());
        QVERIFY2(diags[14].message.find("Poisson regression") != std::string::npos,
                 diags[14].message.c_str());
        QVERIFY2(diags[17].message.find("singular") != std::string::npos,
                 diags[17].message.c_str());
        QVERIFY2(diags[18].message.find("Maximum iterations") != std::string::npos,
                 diags[18].message.c_str());
        QVERIFY2(diags[20].message.find("Stepwise regression") != std::string::npos,
                 diags[20].message.c_str());
        QVERIFY2(diags[24].message.find("Ordinal logistic") != std::string::npos,
                 diags[24].message.c_str());
        QVERIFY2(diags[26].message.find("singular") != std::string::npos,
                 diags[26].message.c_str());

        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("不可用") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("秩亏") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("截距") == std::string::npos, diag.message.c_str());
        }
    }

    void univariate_spc_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "单变量控制图";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：I-MR 不允许 NaN 或无穷观测。",
            "分析限制：MSSD 无偏常数 c4' 使用近似式，不是完整 Minitab 表查表。",
            "检测到 1 点超出 3σ 控制限，建议复核该观测、测量和记录过程。",
            "未提供完整历史 μ/σ；本轮用各组样本均值与全序列 MR/d2 估计 σ。",
            "CUSUM 不使用 Shewhart 特殊原因规则（beyond_control_limit 等），改用上/下侧累计和首次信号。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "test1",
             "检测到 1 点超出 3σ 控制限，建议复核该观测、测量和记录过程。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "test2",
             "检测到连续 9 点位于中心线同侧，建议复核阶段、设备或批次因素。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "test3",
             "检测到连续 6 点单调上升或下降，建议复核趋势、刀具磨损或过程漂移。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "test4",
             "检测到连续 14 点上下交替，建议复核系统性周期或两台设备交替影响。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "test5",
             "检测到 3 点中有 2 点同侧超过 2σ，提示可能存在较小的过程偏移。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "test6",
             "检测到 5 点中有 4 点同侧超过 1σ，提示可能存在较小的过程偏移。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "test7",
             "检测到连续 15 点落在 1σ 以内，控制限可能过宽或数据存在分层。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "test8",
             "检测到连续 8 点落在 1σ 以外，提示可能存在混合总体或双群模式。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "non_finite_input",
             "I-MR 不允许 NaN 或无穷观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_mssd",
             "MSSD 需要至少一对相继差分。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "mssd_c4_prime_approx",
             "MSSD 无偏常数 c4' 使用近似式，不是完整 Minitab 表查表。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "nelson_estimate_ignored",
             "Nelson estimate 仅适用于平均移动极差法；当前 MSSD 方法已忽略该选项。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "nelson_estimate_empty",
             "Nelson estimate 剔除后无剩余移动极差，回退到未剔除估计。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "nelson_estimate_ignored",
             "Nelson estimate 仅适用于平均移动极差法；当前方法已忽略该选项。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "unbalanced_design",
             "Xbar-R 要求各组样本量相等，不能用最后一组的 d2/A2 代替。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "non_finite_input",
             "Xbar-R 子组不允许 NaN 或无穷观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_subgroups",
             "I-MR-R/S 至少需要两个子组。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_subgroup_size",
             "各子组必须至少包含两个观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "non_finite_input",
             "I-MR-R/S 子组不允许 NaN 或无穷观测。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "between_variance_truncated",
             "估计的组间方差为负，已截断为 0；σ_B 可能低估。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_ewma_options",
             "EWMA 要求非空数据、lambda 位于 (0,1] 且控制限倍数大于 0。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_ewma_sigma",
             "EWMA 的过程标准差必须大于 0。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_cusum_options",
             "CUSUM 要求非空数据、sigma/h 大于 0 且 k 不小于 0。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "zone_insufficient_n",
             "区域图至少需要 2 个观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "zone_non_finite",
             "区域图不允许非有限观测。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "zone_jaehn_scoring",
             "区域得分采用 Jaehn 1/2/4 权重、累计阈值 8（formula_reference），不是 Minitab 自定义权重 golden。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "zmr_sample_parameters",
             "未提供完整历史 μ/σ；本轮用各组样本均值与全序列 MR/d2 估计 σ。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "cusum_signals_only",
             "CUSUM 不使用 Shewhart 特殊原因规则（beyond_control_limit 等），改用上/下侧累计和首次信号。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("I-MR does not allow") != std::string::npos,
                 bullets[0].c_str());
        QVERIFY2(bullets[1].find("approximation") != std::string::npos, bullets[1].c_str());
        QVERIFY2(bullets[2].find("beyond the 3") != std::string::npos, bullets[2].c_str());
        QVERIFY2(bullets[3].find("historical") != std::string::npos, bullets[3].c_str());
        QVERIFY2(bullets[4].find("does not use Shewhart") != std::string::npos,
                 bullets[4].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("不允许") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("检测到") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("未提供") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 28);
        QVERIFY2(diags[0].message.find("beyond the 3") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[8].message.find("I-MR does not allow") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[11].message.find("MSSD method ignored") != std::string::npos,
                 diags[11].message.c_str());
        QVERIFY2(diags[13].message.find("current method ignored") != std::string::npos,
                 diags[13].message.c_str());
        QVERIFY2(diags[14].message.find("Xbar-R requires equal") != std::string::npos,
                 diags[14].message.c_str());
        QVERIFY2(diags[19].message.find("truncated to 0") != std::string::npos,
                 diags[19].message.c_str());
        QVERIFY2(diags[25].message.find("formula_reference") != std::string::npos,
                 diags[25].message.c_str());
        QVERIFY2(diags[26].message.find("Complete historical") != std::string::npos,
                 diags[26].message.c_str());
        QVERIFY2(diags[27].message.find("does not use Shewhart") != std::string::npos,
                 diags[27].message.c_str());

        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("检测到") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("不允许") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("区域图") == std::string::npos, diag.message.c_str());
        }
    }

    void censoring_warranty_contract_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "删失与保修";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：删失契约至少需要一条观测。",
            "分析限制：输入为全删失；预测摘要仍可计算，但不得宣称为观察失效率。",
            "删失契约证据类型 formula_reference；右删失不得当作失效。",
            "claims/1000 = 1000*(1-R(T_w))；证据类型 formula_reference，非 vendor_oracle。",
            "无带 failure_mode 标签的 exact 失效，跳过分模式拟合。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "censoring_worksheet_empty",
             "删失工作表为空。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "censoring_worksheet_missing_columns",
             "删失工作表需要 censoring_type 与 time 列。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "censoring_worksheet_missing_interval_bounds",
             "interval 行需要 interval_left / interval_right 列。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "censoring_empty",
             "删失契约至少需要一条观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "censoring_negative_or_nonfinite_time",
             "时间必须为有限非负数；负时间阻止分析。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "censoring_zero_failure_time",
             "出现时间为 0 的失效；请确认单位与记录口径。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "censoring_interval_nonfinite",
             "区间删失边界必须为有限非负数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "censoring_interval_reversed",
             "区间删失要求左端严格小于右端；反向区间阻止。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "censoring_invalid_exposure",
             "暴露量必须为有限非负数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "censoring_time_unit_conflict",
             "观测时间单位不一致，阻止合并分析。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "censoring_zero_failures",
             "零失效（全删失或无 exact 事件）；生存/参数估计可能不可识别。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "censoring_all_failures",
             "全部为失效事件，无删失。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "censoring_all_censored",
             "全部删失；中位寿命等点估计通常不可得。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "censoring_formula_reference",
             "删失契约证据类型 formula_reference；右删失不得当作失效。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "warranty_invalid_window",
             "保修窗口 T_w 必须为正有限数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "warranty_missing_time_unit",
             "保修摘要需要明确时间单位。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "warranty_invalid_reliability",
             "R(T_w) 必须落在 [0,1]。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "warranty_all_censored_input",
             "输入为全删失；预测摘要仍可计算，但不得宣称为观察失效率。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "warranty_formula_reference",
             "claims/1000 = 1000*(1-R(T_w))；证据类型 formula_reference，非 vendor_oracle。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "warranty_prediction_label",
             "当前摘要标记为 prediction，不得与观察失效计数混读为同一口径。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "warranty_strata_mixed_reliability",
             "部分 failure_mode 层使用 cause-specific R(T_w)，其余仍用池化 R；证据类型 formula_reference，不是 vendor_oracle。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "warranty_strata_mode_specific_reliability",
             "分层 expected_failures = 层暴露量 * F_mode(T_w)，F_mode 来自 cause-specific 分模式拟合（竞争失效作右删失）；formula_reference，不是 vendor_oracle。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "mode_fit_threshold_model_unsupported",
             "分模式拟合当前仅支持二参数 Weibull / Lognormal / Exponential / KM；阈值模型请用总体拟合。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "mode_fit_no_labeled_failures",
             "无带 failure_mode 标签的 exact 失效，跳过分模式拟合。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "mode_fit_cause_specific_scope",
             "分模式可靠度 = cause-specific：目标模式 exact 为失效，其他已标注模式的 exact 作为右删失，原始 right 仍为右删失；evidence_type=formula_reference，algorithm_id=cause_specific_right_censored_competing；不是 vendor_oracle。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_event_value",
             "事件列只接受明确的失效/删失编码；未知值不会被静默当作删失。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_censoring_type_value",
             "删失类型列无法解析，或与事件列冲突；未知值不会被静默改写。left/interval 若出现在数据中，将由删失契约拒绝经典 KM 路径。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "censoring_contract_failed",
             "删失契约校验失败。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("at least one observation") != std::string::npos,
                 bullets[0].c_str());
        QVERIFY2(bullets[1].find("observed failure rate") != std::string::npos,
                 bullets[1].c_str());
        QVERIFY2(bullets[2].find("formula_reference") != std::string::npos,
                 bullets[2].c_str());
        QVERIFY2(bullets[3].find("not vendor_oracle") != std::string::npos,
                 bullets[3].c_str());
        QVERIFY2(bullets[4].find("per-mode fitting skipped") != std::string::npos,
                 bullets[4].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("删失契约") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("不得宣称为") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("跳过") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 28);
        QVERIFY2(diags[0].message.find("empty") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[3].message.find("at least one observation") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[13].message.find("right-censored") != std::string::npos,
                 diags[13].message.c_str());
        QVERIFY2(diags[18].message.find("not vendor_oracle") != std::string::npos,
                 diags[18].message.c_str());
        QVERIFY2(diags[20].message.find("cause-specific") != std::string::npos,
                 diags[20].message.c_str());
        QVERIFY2(diags[24].message.find("cause_specific_right_censored_competing")
                     != std::string::npos,
                 diags[24].message.c_str());
        QVERIFY2(diags[25].message.find("not silently treated as censored")
                     != std::string::npos,
                 diags[25].message.c_str());
        QVERIFY2(diags[27].message.find("validation failed") != std::string::npos,
                 diags[27].message.c_str());

        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("删失工作表") == std::string::npos,
                     diag.message.c_str());
            QVERIFY2(diag.message.find("保修窗口") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("分模式拟合") == std::string::npos,
                     diag.message.c_str());
        }
    }

    void multivariate_spc_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "多元控制图";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：样本协方差奇异，无法计算 T²。",
            "分析限制：默认 UCL 使用渐近 χ² 近似，不是仿真 ARL 校准常数；可手工指定 ucl。",
            "本命令是正式多元 Hotelling T² 控制图，不是 PCA 经验分位 T²。",
            "MEWMA 是多元向量平滑；不要与单变量 EWMA 图混淆。",
            "EMP 分级基于 Wheeler ICC，不是 AIAG %Study Var 合格判定。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "empty_matrix",
             "需要至少一行多元观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "need_multivariate",
             "Hotelling T² 至少需要两个变量列。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_observations",
             "个体 T² Phase I 需要 m > p+1。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "source_row_mismatch",
             "source_rows 长度必须匹配观测数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_matrix",
             "无法估计均值/协方差（非有限或不矩形）。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "singular_covariance",
             "样本协方差奇异，无法计算 T²。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "not_pca_empirical_t2",
             "本命令是正式多元 Hotelling T² 控制图，不是 PCA 经验分位 T²。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "need_multivariate",
             "MEWMA 至少需要两个变量列。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_observations",
             "MEWMA 至少需要 3 个观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_matrix",
             "无法估计均值/协方差。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "ragged_matrix",
             "观测矩阵必须矩形。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "singular_mewma_cov",
             "MEWMA 协方差在当前步奇异。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "mewma_ucl_not_arl_calibrated",
             "默认 UCL 使用渐近 χ² 近似，不是仿真 ARL 校准常数；可手工指定 ucl。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "not_univariate_ewma",
             "MEWMA 是多元向量平滑；不要与单变量 EWMA 图混淆。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "empty_subgroups",
             "需要至少一个子组。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "empty_subgroup",
             "子组不能为空。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "need_multivariate",
             "GV 图至少需要两个变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "subgroup_too_small",
             "广义方差图要求每个子组大小 n > 变量数 p。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "unequal_subgroups",
             "广义方差图要求等量子组。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "ragged_subgroup",
             "子组观测必须具有相同变量数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_b_constants",
             "b1/b2 常数无效。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "non_finite",
             "子组含非有限值。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "gv_montgomery_subgroup",
             "广义方差图按 Montgomery |S| 子组公式；个体观测路径不做假 |S|。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "gv_variability_chart_caveat",
             "NIST 指出多元变差图存在争议；本输出仅作 |S| 探索信号，不是唯一变差判定。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "zero_repeatability",
             "重复性方差为 0，Probable Error 为 0；请检查设计。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "emp_not_aiag_pass_fail",
             "EMP 分级基于 Wheeler ICC，不是 AIAG %Study Var 合格判定。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("singular") != std::string::npos, bullets[0].c_str());
        QVERIFY2(bullets[1].find("ARL-calibrated") != std::string::npos, bullets[1].c_str());
        QVERIFY2(bullets[2].find("not PCA") != std::string::npos, bullets[2].c_str());
        QVERIFY2(bullets[3].find("univariate EWMA") != std::string::npos, bullets[3].c_str());
        QVERIFY2(bullets[4].find("not an AIAG") != std::string::npos, bullets[4].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("无法计算") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("不要与") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("合格判定") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 26);
        QVERIFY2(diags[1].message.find("Hotelling") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[4].message.find("non-rectangular") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[7].message.find("MEWMA requires at least two") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[9].message == "Unable to estimate mean/covariance.",
                 diags[9].message.c_str());
        QVERIFY2(diags[12].message.find("ARL-calibrated") != std::string::npos,
                 diags[12].message.c_str());
        QVERIFY2(diags[16].message.find("GV chart") != std::string::npos,
                 diags[16].message.c_str());
        QVERIFY2(diags[22].message.find("Montgomery") != std::string::npos,
                 diags[22].message.c_str());
        QVERIFY2(diags[25].message.find("Wheeler ICC") != std::string::npos,
                 diags[25].message.c_str());

        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("至少需要") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("广义方差") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("合格判定") == std::string::npos, diag.message.c_str());
        }
    }

    void competing_risk_cif_fine_gray_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "竞争风险";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：Fine-Gray 信息量退化，未能估计 β。",
            "分析限制：不得把本 Fine-Gray 结果写成商业软件对齐或 pinned R survival::finegray 黄金标准。",
            "无带 failure_mode 标签的 exact 失效，跳过 Aalen–Johansen CIF。",
            "Aalen–Johansen CIF 本身不是 Fine-Gray 回归；不得把 CIF 表写成 Fine-Gray 或商业软件对齐。",
            "已指定多协变量列：Fine-Gray 使用 multi IPCW，不与二分类 group Fine-Gray 同时运行。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::info,
             "cif_no_labeled_failures",
             "无带 failure_mode 标签的 exact 失效，跳过 Aalen–Johansen CIF。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "cif_aalen_johansen_scope",
             "累计发生函数 CIF = Aalen–Johansen（formula_reference / aalen_johansen_cif）：总体生存把任一标注失效当作事件；CIF_k 为原因 k 的累计发生概率。不是 Fine-Gray 多协变量回归，不是 cause-specific（竞争删失）可靠度，不是 vendor_oracle。二分类 group 的 Fine-Gray 另有门禁路径。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "cif_not_fine_gray_multivar",
             "Aalen–Johansen CIF 本身不是 Fine-Gray 回归；不得把 CIF 表写成 Fine-Gray 或商业软件对齐。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "fine_gray_not_vendor_oracle",
             "不得把本 Fine-Gray 结果写成商业软件对齐或 pinned R survival::finegray 黄金标准。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "fine_gray_singular",
             "Fine-Gray 信息量退化，未能估计 β。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "fine_gray_diverged",
             "Fine-Gray β 迭代发散。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "fine_gray_not_converged",
             "Fine-Gray 未在迭代上限内收敛。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "fine_gray_singular",
             "Fine-Gray 信息量不可逆，未能估计 SE。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "fine_gray_covariate_length_mismatch",
             "Fine-Gray 协变量矩阵行数与观测不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "fine_gray_no_labeled_failures",
             "Fine-Gray 需要带 failure_mode 的 exact 失效。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "fine_gray_ragged_matrix",
             "Fine-Gray 协变量矩阵行长度不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "fine_gray_need_multi",
             "fine_gray_multi 需要至少两个协变量；单列请用 fine_gray_continuous。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "fine_gray_covariate_length_mismatch",
             "Fine-Gray 连续协变量长度与观测不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "fine_gray_target_one_group_only",
             "目标原因失效只出现在一个分组中；估计仍可运行但对比识别较弱。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "fine_gray_covariate_priority",
             "已指定多协变量列：Fine-Gray 使用 multi IPCW，不与二分类 group Fine-Gray 同时运行。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "fine_gray_covariate_priority",
             "已指定连续协变量列：Fine-Gray 使用 continuous IPCW，不与二分类 group Fine-Gray 同时运行。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("could not be estimated") != std::string::npos,
                 bullets[0].c_str());
        QVERIFY2(bullets[1].find("pinned R") != std::string::npos, bullets[1].c_str());
        QVERIFY2(bullets[2].find("skipping Aalen") != std::string::npos, bullets[2].c_str());
        QVERIFY2(bullets[3].find("not Fine-Gray regression") != std::string::npos,
                 bullets[3].c_str());
        QVERIFY2(bullets[4].find("multi IPCW") != std::string::npos, bullets[4].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("跳过") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("不得把") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("已指定") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 16);
        QVERIFY2(diags[0].message.find("skipping Aalen") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("not vendor_oracle") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[4].message.find("β could not be estimated") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[7].message.find("SE could not be estimated") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("row count") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[12].message.find("continuous covariate length") != std::string::npos,
                 diags[12].message.c_str());
        QVERIFY2(diags[14].message.find("multi IPCW") != std::string::npos,
                 diags[14].message.c_str());
        QVERIFY2(diags[15].message.find("continuous IPCW") != std::string::npos,
                 diags[15].message.c_str());

        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("未能估计") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("不得把") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("已指定") == std::string::npos, diag.message.c_str());
        }
    }

    void ml_cluster_cart_mixture_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "机器学习与聚类";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：合并协方差奇异。",
            "分析限制：已达最大迭代次数，分配可能尚未完全稳定。",
            "自研 Isolation Forest；多元异常辅助；非单变量 Grubbs/Dixon；非 TreeNet。",
            "自研 CART 单树；非 Minitab TreeNet/Random Forests 数值对齐；本轮无成本复杂度剪枝。",
            "观测不足（n<30），无法做二维高斯混合 EM；不得把不足样本写成已排除混合。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "iforest_empty",
             "Isolation Forest 需要非空矩阵与 tree_count ≥ 1。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "iforest_shape",
             "至少需要 2 个观测与 2 个变量。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "iforest_scope",
             "自研 Isolation Forest；多元异常辅助；非单变量 Grubbs/Dixon；非 TreeNet。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "kmeans_k",
             "K-Means 需要 k ≥ 2。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "kmeans_empty",
             "K-Means 需要至少一行有效数值观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "n_lt_k",
             "有效观测数必须 ≥ k。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "kmeans_init",
             "初始质心取前 k 个有效观测（分析尺度）；Lloyd 迭代（分配→更新质心）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "kmeans_max_iter",
             "已达最大迭代次数，分配可能尚未完全稳定。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "cart_empty",
             "CART 需要预测矩阵与响应对齐且非空。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "cart_predictors",
             "至少需要一个数值预测变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "cart_min_leaf",
             "有效观测过少，无法按 min_leaf 分裂。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "cart_scope",
             "自研 CART 单树；非 Minitab TreeNet/Random Forests 数值对齐；本轮无成本复杂度剪枝。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "lda_invalid",
             "线性判别需要 ≥2 类、≥1 预测变量与足够观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "lda_row",
             "类别编码或预测行列不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "lda_class",
             "每个类至少需要 2 个观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "lda_df",
             "合并协方差自由度不足。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "lda_singular",
             "合并协方差奇异。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "lda_scope",
             "线性判别（等协方差）；非 Minitab golden；不做 QDA。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "hclust_invalid",
             "层次聚类需要非空矩阵且 k≥2。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "hclust_linkage",
             "Complete linkage + 欧氏距离；非 Minitab golden。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "mixture2_n",
             "观测不足（n<30），无法做二维高斯混合 EM；不得把不足样本写成已排除混合。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "mixture2_degen",
             "二维高斯混合 EM 密度退化，未能收敛；不得伪造混合拟合。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "mixture2_iter",
             "二维高斯混合 EM 未在迭代上限内收敛；不得把未收敛结果写成已确认混合。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "mixture_k_n",
             "观测不足（n<30），无法做多 k 高斯混合 BIC 搜索；不得把不足样本写成已排除混合。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "mixture_k_fail",
             "多 k 高斯混合 BIC 搜索未得到可用拟合；不得伪造混合结论。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("singular") != std::string::npos, bullets[0].c_str());
        QVERIFY2(bullets[1].find("not be fully stable") != std::string::npos,
                 bullets[1].c_str());
        QVERIFY2(bullets[2].find("not TreeNet") != std::string::npos, bullets[2].c_str());
        QVERIFY2(bullets[3].find("cost-complexity") != std::string::npos, bullets[3].c_str());
        QVERIFY2(bullets[4].find("ruled out a mixture") != std::string::npos,
                 bullets[4].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("合并协方差") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("自研") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("不得把") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 25);
        QVERIFY2(diags[0].message.find("Isolation Forest") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[5].message.find("must be ≥ k") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[7].message.find("fully stable") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[11].message.find("numerically aligned") != std::string::npos,
                 diags[11].message.c_str());
        QVERIFY2(diags[16].message.find("singular") != std::string::npos,
                 diags[16].message.c_str());
        QVERIFY2(diags[19].message.find("Euclidean") != std::string::npos,
                 diags[19].message.c_str());
        QVERIFY2(diags[20].message.find("two-component") != std::string::npos,
                 diags[20].message.c_str());
        QVERIFY2(diags[23].message.find("multi-k") != std::string::npos,
                 diags[23].message.c_str());
        QVERIFY2(diags[24].message.find("fabricate mixture") != std::string::npos,
                 diags[24].message.c_str());

        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("至少需要") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("不得伪造") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("非空矩阵") == std::string::npos, diag.message.c_str());
        }
    }

    void capability_gate_johnson_boxcox_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "过程能力门禁";
        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：规格限落在 Johnson 变换定义域外，无法计算 Pp/Ppk。",
            "分析限制：能力指标未验证过程稳定性和正态性；数值仅供调查，不能单独作为过程合格结论。",
            "Johnson 能力为研究/预览结果：可显示拟合与变换尺度指数，但不得输出过程合格判定（门禁：缺 golden/尾部/人工解释验收）。",
            "观测不足（n<30），无法做多 k 高斯混合门禁；不得把不足样本写成已排除混合。",
            "Box-Cox 变换要求所有观测严格大于 0。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "assumption_not_verified",
             "能力指标未验证过程稳定性和正态性；数值仅供调查，不能单独作为过程合格结论。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "assumption_not_verified",
             "能力指标未验证过程稳定性；数值仅供调查，不能单独作为过程合格结论。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_stability_insufficient_n",
             "观测不足，无法做能力分析前的 I-MR 稳定性初筛。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_stability_screen_clear_not_verified",
             "I-MR Rule-1 初筛未检出超限点；这不是完整控制图验收，也不等于已验证稳定性/正态性，不得自动开放合格判定。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_stability_prerequisite",
             "能力分析稳定性前置：当前仅做公式参考级 I-MR Rule-1 初筛；合格判定保持关闭，直至独立稳定性与正态性验收工作流落地。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_mixture_insufficient_n",
             "观测不足（n<30），无法做多 k 高斯混合门禁；不得把不足样本写成已排除混合。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_mixture_failed",
             "多 k 高斯混合门禁未得到可用拟合；不得伪造混合结论或开放合格判定。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "johnson_capability_gated",
             "Johnson 能力为研究/预览结果：可显示拟合与变换尺度指数，但不得输出过程合格判定（门禁：缺 golden/尾部/人工解释验收）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_pass_fail_blocked_by_stability_prerequisite",
             "正态能力未满足稳定性/正态性验收前置：禁止过程合格判定（pass_fail_judgment_allowed=false）。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "johnson_spec_outside_support",
             "规格限落在 Johnson 变换定义域外，无法计算 Pp/Ppk。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "johnson_spec_outside_support",
             "至少一个规格限落在变换定义域外；越界侧的百分位回推未实现，仅输出仍可变换规格的 overall 指数。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "within_not_applicable_after_johnson",
             "Johnson 变换路径只报告 overall Pp/Ppk，不报告 within Cp/Cpk。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "johnson_insufficient_n",
             "Johnson 变换至少需要 8 个有限观测。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "johnson_transform_not_found",
             "样本量不足以估计 Johnson 分位匹配所需的尾部分位数。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "johnson_formula_reference",
             "Johnson 变换按 Chou et al. (1998) 分位匹配与 AD p 值选择；数值是公式参考，不是 Minitab Individual Distribution Identification 导出。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "nonnormal_z_score_formula_reference",
             "非正态能力使用拟合分布 CDF 的 Z-score 法计算 Pp/Ppk；不报告 Cp/Cpk。数值是公式参考，不是 Minitab 导出。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_observations",
             "Box-Cox 变换至少需要两个有效观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "nonpositive_observation",
             "Box-Cox 变换要求所有观测严格大于 0。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "lambda_out_of_range",
             "Box-Cox lambda 必须位于 [-5, 5]。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "assumption_not_verified",
             "n<8 时正态性近似较粗糙，未拒绝正态假设不能当作已验证正态。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "assumption_not_verified",
             "二项过程能力未验证独立性、恒定 p 与稳定性，不能写成过程合格。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "assumption_not_verified",
             "泊松过程能力未验证独立性、恒定 DPU 与稳定性，不能写成过程合格。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_subgroups",
             "组间/组内能力至少需要两个子组。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_hartigan_dip_consistent_not_proof",
             "Hartigan dip 门禁筛查未拒绝 Uniform 零假设下的单峰（formula_reference）；这不是过程单峰证明，也不得开放合格判定。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_bimodality_clear_not_verified",
             "直方图双峰初筛未检出可分离峰；这不是单峰证明，不得据此开放过程合格判定。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("outside the Johnson") != std::string::npos,
                 bullets[0].c_str());
        QVERIFY2(bullets[1].find("stability and normality") != std::string::npos,
                 bullets[1].c_str());
        QVERIFY2(bullets[2].find("research/preview") != std::string::npos,
                 bullets[2].c_str());
        QVERIFY2(bullets[3].find("mixture gate cannot run") != std::string::npos,
                 bullets[3].c_str());
        QVERIFY2(bullets[4].find("strictly greater than 0") != std::string::npos,
                 bullets[4].c_str());

        for (const auto& bullet : bullets) {
            QVERIFY2(bullet.find("过程合格") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("不得输出") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("严格大于") == std::string::npos, bullet.c_str());
        }

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 25);
        QVERIFY2(diags[0].message.find("stability and normality") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("process stability") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[5].message.find("mixture gate cannot run") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[7].message.find("research/preview") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[9].message.find("outside the Johnson") != std::string::npos,
                 diags[9].message.c_str());
        QVERIFY2(diags[14].message.find("Chou et al.") != std::string::npos,
                 diags[14].message.c_str());
        QVERIFY2(diags[16].message.find("at least two valid") != std::string::npos,
                 diags[16].message.c_str());
        QVERIFY2(diags[19].message.find("n<8") != std::string::npos, diags[19].message.c_str());
        QVERIFY2(diags[20].message.find("Binomial") != std::string::npos,
                 diags[20].message.c_str());
        QVERIFY2(diags[22].message.find("Between/within") != std::string::npos,
                 diags[22].message.c_str());

        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("不能写成过程合格") == std::string::npos,
                     diag.message.c_str());
            QVERIFY2(diag.message.find("合格判定保持关闭") == std::string::npos,
                     diag.message.c_str());
            QVERIFY2(diag.message.find("不得伪造") == std::string::npos, diag.message.c_str());
        }
    }

    void box_cox_spec_limit_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "Box-Cox 变换";
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "box_cox_invalid_spec_limit",
             "规格下限无法变换（须为正有限数）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "box_cox_invalid_spec_limit",
             "规格上限无法变换（须为正有限数）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "box_cox_invalid_spec_limit",
             "规格目标无法变换（须为正有限数）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "box_cox_spec_limits_order",
             "变换后规格限顺序无效；请检查 LSL/USL 与 λ。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 4);
        QVERIFY2(diags[0].message.find("Lower spec limit") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("Upper spec limit") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("Target spec") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("out of order") != std::string::npos,
                 diags[3].message.c_str());
        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("规格") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("变换") == std::string::npos, diag.message.c_str());
        }

        OutputPage interp_page = sample_page();
        interp_page.title = "Box-Cox 变换";
        datalab::domain::BoxCoxFacts facts;
        facts.lambda = 0.5;
        facts.n = 4;
        facts.assumption_status = "not_verified";
        interp_page.facts.box_cox = facts;
        interp_page.diagnostics = page.diagnostics;
        datalab::application::InterpretationService::enrich(interp_page);
        auto interp_doc = build_report_document(sample_table(), {interp_page}, profile);
        interp_doc.pages[0].visible_interpretation = interp_page.interpretation;
        const auto interp_localized = localize_report_document(interp_doc);
        bool saw_gate_bullet = false;
        for (const auto& section :
             interp_localized.document.pages[0].visible_interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("post-transform capability table was skipped")
                        != std::string::npos) {
                    saw_gate_bullet = true;
                }
                QVERIFY2(
                    bullet.find("已跳过变换后过程能力表") == std::string::npos,
                    "localized interpretation must not leak Chinese spec-limit gate");
            }
        }
        QVERIFY2(saw_gate_bullet, "en-US must localize Box-Cox spec-limit gate bullet");
    }

    void warranty_exposure_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "保修摘要";
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_exposure_value",
             "暴露量列必须为有限非负数；缺失或非法值不会被静默补齐。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_exposure_value",
             "暴露量列必须为有限非负数；缺失或非法值不会被静默当作 0 或 1。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "warranty_zero_exposure",
             "暴露量列求和后必须为正有限数；不得用标量默认值静默补齐。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "warranty_exposure_column_overrides_scalar",
             "同时提供了暴露量列与标量暴露量；摘要使用列求和，标量被忽略。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 4);
        QVERIFY2(
            diags[0].message.find("not silently imputed") != std::string::npos,
            diags[0].message.c_str());
        QVERIFY2(
            diags[1].message.find("not silently treated as 0 or 1") != std::string::npos,
            diags[1].message.c_str());
        QVERIFY2(
            diags[2].message.find("silent fallback") != std::string::npos,
            diags[2].message.c_str());
        QVERIFY2(
            diags[3].message.find("column sum and ignores the scalar") != std::string::npos,
            diags[3].message.c_str());
        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("暴露量") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("静默") == std::string::npos, diag.message.c_str());
        }

        OutputPage interp_page = sample_page();
        interp_page.title = "保修摘要";
        interp_page.method_name = "Warranty Summary";
        interp_page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "warranty_zero_exposure",
             "暴露量列求和后必须为正有限数；不得用标量默认值静默补齐。"},
        };
        datalab::application::InterpretationService::enrich(interp_page);
        auto interp_doc = build_report_document(sample_table(), {interp_page}, profile);
        interp_doc.pages[0].visible_interpretation = interp_page.interpretation;
        const auto interp_localized = localize_report_document(interp_doc);
        bool saw_gate_bullet = false;
        for (const auto& section :
             interp_localized.document.pages[0].visible_interpretation) {
            for (const auto& bullet : section.bullets) {
                if (bullet.find("warranty summary metrics were skipped")
                        != std::string::npos) {
                    saw_gate_bullet = true;
                }
                QVERIFY2(
                    bullet.find("已跳过保修摘要指标") == std::string::npos,
                    "localized interpretation must not leak Chinese exposure gate bullet");
            }
        }
        QVERIFY2(saw_gate_bullet, "en-US must localize warranty exposure gate bullet");
    }

    void doe_eda_multivariate_nested_titles_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "嵌套标题切片";
        const char* table_titles[] = {
            "响应目标",
            "最佳组合",
            "候选组合",
            "响应预测",
            "因子均值",
            "单元均值",
            "单元统计",
            "四图说明",
            "箱线统计",
            "结构摘要",
            "关于中位数的游程",
            "上升/下降游程",
            "区域图参数",
            "广义方差摘要",
            "逐子组 |S|",
            "T² 摘要",
            "逐点 T²",
            "逐点 MEWMA T²",
        };
        for (const char* title : table_titles) {
            datalab::domain::StatisticTable table;
            table.title = title;
            page.tables.push_back(table);
        }
        const char* plot_titles[] = {
            "候选组合总体 Desirability",
            "均值与极差",
            "标准差图",
            "能力图",
            "区域累计得分",
            "广义方差 |S| 图",
        };
        for (const char* title : plot_titles) {
            datalab::domain::PlotSpec plot;
            plot.title = title;
            page.plots.push_back(plot);
        }

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(static_cast<int>(tables.size()), 18);
        QCOMPARE(QString::fromStdString(tables[0].title), QStringLiteral("Response objectives"));
        QCOMPARE(QString::fromStdString(tables[1].title), QStringLiteral("Best combination"));
        QCOMPARE(QString::fromStdString(tables[2].title),
                 QStringLiteral("Candidate combinations"));
        QCOMPARE(QString::fromStdString(tables[3].title), QStringLiteral("Response predictions"));
        QCOMPARE(QString::fromStdString(tables[4].title), QStringLiteral("Factor means"));
        QCOMPARE(QString::fromStdString(tables[5].title), QStringLiteral("Cell means"));
        QCOMPARE(QString::fromStdString(tables[6].title), QStringLiteral("Cell statistics"));
        QCOMPARE(QString::fromStdString(tables[7].title), QStringLiteral("Four-plot notes"));
        QCOMPARE(QString::fromStdString(tables[8].title), QStringLiteral("Boxplot statistics"));
        QCOMPARE(QString::fromStdString(tables[9].title), QStringLiteral("Structure summary"));
        QCOMPARE(QString::fromStdString(tables[10].title),
                 QStringLiteral("Runs about the median"));
        QCOMPARE(QString::fromStdString(tables[11].title), QStringLiteral("Runs up/down"));
        QCOMPARE(QString::fromStdString(tables[12].title),
                 QStringLiteral("Zone chart parameters"));
        QCOMPARE(QString::fromStdString(tables[13].title),
                 QStringLiteral("Generalized variance summary"));
        QCOMPARE(QString::fromStdString(tables[14].title), QStringLiteral("Subgroup |S|"));
        QCOMPARE(QString::fromStdString(tables[15].title), QStringLiteral("T² summary"));
        QCOMPARE(QString::fromStdString(tables[16].title), QStringLiteral("Pointwise T²"));
        QCOMPARE(QString::fromStdString(tables[17].title), QStringLiteral("Pointwise MEWMA T²"));

        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(static_cast<int>(plots.size()), 6);
        QCOMPARE(QString::fromStdString(plots[0].title),
                 QStringLiteral("Overall desirability of candidates"));
        QCOMPARE(QString::fromStdString(plots[1].title), QStringLiteral("Mean and range"));
        QCOMPARE(QString::fromStdString(plots[2].title),
                 QStringLiteral("Standard deviation chart"));
        QCOMPARE(QString::fromStdString(plots[3].title), QStringLiteral("Capability chart"));
        QCOMPARE(QString::fromStdString(plots[4].title),
                 QStringLiteral("Zone cumulative score"));
        QCOMPARE(QString::fromStdString(plots[5].title),
                 QStringLiteral("Generalized variance |S| chart"));

        for (const auto& table : tables) {
            QVERIFY2(table.title.find("响应") == std::string::npos, table.title.c_str());
            QVERIFY2(table.title.find("游程") == std::string::npos, table.title.c_str());
        }
        for (const auto& plot : plots) {
            QVERIFY2(plot.title.find("能力图") == std::string::npos, plot.title.c_str());
            QVERIFY2(plot.title.find("标准差图") == std::string::npos, plot.title.c_str());
        }
    }

    void classic_inference_param_summary_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "经典推断参数摘要切片";
        page.parameter_summary =
            "变量 = Length    假设均值 = 0    备择：总体均值 不等于 假设均值    "
            "方法 = 合并方差    置信水平 = 0.95    "
            "变量: Length    方法: Anderson-Darling    缺失值 N* = 2    "
            "备择: two-sided    "
            "事件 = Ev    试验 = Tr    目标比例 = 0.5    假设比例 = 0.5    "
            "缺陷 = Def    假设发生率 = 1.2    比较 = difference    "
            "行: RowCat    列: ColCat    "
            "变换 = log    检验 = TestCol    参考 = RefCol    "
            "（本命令不做卡方检验；关联检验请用列联表卡方）";

        datalab::domain::StatisticTable pct;
        pct.title = "行百分比";
        pct.headers = {"", "A", "合计"};
        page.tables.push_back(pct);
        datalab::domain::StatisticTable col_pct;
        col_pct.title = "列百分比";
        page.tables.push_back(col_pct);
        datalab::domain::StatisticTable tot_pct;
        tot_pct.title = "合计百分比";
        page.tables.push_back(tot_pct);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());
        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Hypothesized mean = 0") != std::string::npos,
                 summary.c_str());
        QVERIFY2(
            summary.find(
                "Alternative: population mean not equal to hypothesized mean")
                != std::string::npos,
            summary.c_str());
        QVERIFY2(summary.find("Method = Pooled variance") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Method: Anderson-Darling") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Missing N* = 2") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Alternative: two-sided") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Trials = Tr") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Target proportion = 0.5") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Hypothesized proportion = 0.5") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Hypothesized rate = 1.2") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Comparison = difference") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Row: RowCat") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Column: ColCat") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Transform = log") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Test = TestCol") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Reference = RefCol") != std::string::npos, summary.c_str());
        QVERIFY2(
            summary.find(
                "(This command does not run a chi-square test; use Chi-Square "
                "Association for association tests)")
                != std::string::npos,
            summary.c_str());
        QVERIFY2(summary.find("假设均值") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("合并方差") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("备择") == std::string::npos, summary.c_str());

        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(QString::fromStdString(tables[0].title), QStringLiteral("Row percent"));
        QCOMPARE(QString::fromStdString(tables[1].title), QStringLiteral("Column percent"));
        QCOMPARE(QString::fromStdString(tables[2].title), QStringLiteral("Total percent"));
    }

    void graph_builder_param_summary_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "Hexbin（分面）";
        page.parameter_summary =
            "X = A    Y = B    分面 = Site    面板 = 2/3    "
            "显示 N = 10    分析 N = 12    分析 N(水平) = 8    "
            "分组 = Batch    置信水平 = 0.95    大小 = Size    数值 = Y    "
            "显示组数 = 3    显示类别数 = 5    分析类别数 = 6    "
            "小类别合并阈值 = 5%    "
            "有效变量数 = 4    "
            "坐标已按各变量最小-最大范围标准化    "
            "直线性不能单独作为正态性证明；显示拟合省略 hidden，分析拟合保留 hidden";

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());
        QCOMPARE(QString::fromStdString(localized.document.pages[0].source_page.title),
                 QStringLiteral("Hexbin (faceted)"));
        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Facet = Site") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Panels = 2/3") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Display N = 10") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Analysis N = 12") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Analysis N (level) = 8") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Group = Batch") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Confidence level = 0.95") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Size = Size") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Value = Y") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Displayed groups = 3") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Displayed categories = 5") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Analysis categories = 6") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Small-category merge threshold = 5%") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Valid variables = 4") != std::string::npos, summary.c_str());
        QVERIFY2(
            summary.find("Coordinates scaled to each variable's min–max range")
                != std::string::npos,
            summary.c_str());
        QVERIFY2(
            summary.find(
                "Linearity alone is not proof of normality; display fit omits hidden, "
                "analysis fit keeps hidden")
                != std::string::npos,
            summary.c_str());
        QVERIFY2(summary.find("分面") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("显示 N") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("置信水平") == std::string::npos, summary.c_str());

        datalab::domain::PlotSpec plot;
        plot.title = "ECDF";
        plot.y_axis_title = "累计比例";
        document.pages[0].visible_plots = {plot};
        const auto localized2 = localize_report_document(document);
        QCOMPARE(QString::fromStdString(
                     localized2.document.pages[0].visible_plots[0].y_axis_title),
                 QStringLiteral("Cumulative proportion"));
    }

    void spc_capability_param_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "SPC / 能力参数表 chrome";
        datalab::domain::StatisticTable process;
        process.title = "Process Data";
        process.headers = {"项目", "数值"};
        process.rows = {
            {"规格模式", "bilateral"},
            {"Within σ 来源", "Rbar"},
            {"Between σ 来源", "Sbar"},
            {"Between/Within σ 来源", "pooled"},
            {"Overall σ 方法", "s"},
            {"AD 判定", "在 alpha 下拒绝正态假设"},
            {"假设状态", "unverified"},
        };
        page.tables.push_back(process);
        datalab::domain::StatisticTable ad;
        ad.title = "AD";
        ad.headers = {"项目", "数值"};
        ad.rows = {
            {"AD 判定", "无法计算"},
            {"AD 判定", "在 alpha 下未拒绝正态假设"},
        };
        page.tables.push_back(ad);
        datalab::domain::StatisticTable ppm;
        ppm.title = "Performance (PPM)";
        ppm.headers = {"", "观测", "期望 Within", "期望 Overall"};
        ppm.rows = {
            {"低于 LSL", "1", "2", "3"},
            {"高于 USL", "4", "5", "6"},
            {"合计", "5", "7", "9"},
        };
        page.tables.push_back(ppm);
        datalab::domain::StatisticTable t2;
        t2.title = "T² 摘要";
        t2.headers = {"指标", "数值"};
        t2.rows = {
            {"限方法", "empirical"},
            {"超 UCL 点数", "2"},
        };
        page.tables.push_back(t2);
        datalab::domain::StatisticTable t2_pts;
        t2_pts.title = "逐点 T²";
        t2_pts.headers = {"观测顺序", "T²", "信号"};
        t2_pts.rows = {{"1", "10", "超限"}, {"2", "1", ""}};
        page.tables.push_back(t2_pts);
        datalab::domain::StatisticTable cusum;
        cusum.title = "CUSUM 参数";
        cusum.headers = {"指标", "数值"};
        cusum.rows = {
            {"目标 T", "0"},
            {"上侧首次信号点", "3"},
            {"下侧首次信号点", "无（不套用 Tests 1–8）"},
            {"信号总数", "1"},
            {"判定口径",
             "上侧/下侧累计和超过决策间隔 hσ 记为信号，原因待调查。"},
        };
        page.tables.push_back(cusum);
        datalab::domain::StatisticTable zone;
        zone.title = "区域图参数";
        zone.headers = {"指标", "数值"};
        zone.rows = {
            {"移动极差长度", "2"},
            {"Jaehn 累计阈值", "8"},
            {"Jaehn 信号点数", "1"},
            {"计分规则",
             "Jaehn 1/2/4 权重；不是完整 Western Electric Tests 1–8。"},
        };
        page.tables.push_back(zone);
        datalab::domain::StatisticTable zmr;
        zmr.title = "Z-MR 参数";
        zmr.headers = {"指标", "数值"};
        zmr.rows = {
            {"Z 图「单点超出 3σ 控制限」触发", "2"},
            {"参数来源", "样本估计（未提供完整历史 μ/σ）"},
            {"适用测试", "Z 图 Tests 1–4；MR 图无 Shewhart 测试。"},
            {"参数来源", "历史 μ/σ"},
        };
        page.tables.push_back(zmr);
        datalab::domain::StatisticTable hist;
        hist.title = "历史参数与分阶段估计";
        hist.headers = {"项", "值"};
        hist.rows = {
            {"历史 μ", "（未指定）"},
            {"历史 σ", "1.2"},
            {"控制限参数来源", "历史参数优先"},
            {"控制限参数来源", "当前数据估计"},
            {"阶段数", "2"},
            {"阶段 A（N / 均值 / σ̂_MR）", "5 / 1.0 / 0.2"},
            {"阶段列", "未指定"},
            {"说明",
             "分阶段估计仅作对照；全局控制限仍由历史参数或全样本估计决定，不自动按阶段切换限。"},
            {"Nelson 剔除 MR 数", "1"},
        };
        page.tables.push_back(hist);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());
        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(QString::fromStdString(tables[0].rows[0][0]),
                 QStringLiteral("Specification mode"));
        QCOMPARE(QString::fromStdString(tables[0].rows[1][0]),
                 QStringLiteral("Within σ source"));
        QCOMPARE(QString::fromStdString(tables[0].rows[5][1]),
                 QStringLiteral("Reject normality at alpha"));
        QCOMPARE(QString::fromStdString(tables[1].rows[0][1]),
                 QStringLiteral("Unable to compute"));
        QCOMPARE(QString::fromStdString(tables[1].rows[1][1]),
                 QStringLiteral("Fail to reject normality at alpha"));
        QCOMPARE(QString::fromStdString(tables[2].headers[1]),
                 QStringLiteral("Observation"));
        QCOMPARE(QString::fromStdString(tables[2].headers[2]),
                 QStringLiteral("Expected within"));
        QCOMPARE(QString::fromStdString(tables[2].headers[3]),
                 QStringLiteral("Expected overall"));
        QCOMPARE(QString::fromStdString(tables[2].rows[0][0]),
                 QStringLiteral("Below LSL"));
        QCOMPARE(QString::fromStdString(tables[2].rows[2][0]),
                 QStringLiteral("Total"));
        QCOMPARE(QString::fromStdString(tables[3].rows[0][0]),
                 QStringLiteral("Limit method"));
        QCOMPARE(QString::fromStdString(tables[4].rows[0][2]),
                 QStringLiteral("Out of control"));
        QCOMPARE(QString::fromStdString(tables[5].rows[0][0]),
                 QStringLiteral("Target T"));
        QCOMPARE(QString::fromStdString(tables[6].rows[0][0]),
                 QStringLiteral("Moving-range length"));
        QCOMPARE(QString::fromStdString(tables[6].rows[3][1]),
                 QStringLiteral(
                     "Jaehn 1/2/4 weights; not full Western Electric Tests 1–8."));
        QCOMPARE(QString::fromStdString(tables[7].rows[0][0]),
                 QStringLiteral("Z chart “1 point beyond 3σ” triggers"));
        QCOMPARE(QString::fromStdString(tables[7].rows[1][1]),
                 QStringLiteral(
                     "Sample estimates (complete historical μ/σ not provided)"));
        QCOMPARE(QString::fromStdString(tables[8].rows[0][1]),
                 QStringLiteral("(not specified)"));
        QCOMPARE(QString::fromStdString(tables[8].rows[2][1]),
                 QStringLiteral("Historical parameters preferred"));
        QCOMPARE(QString::fromStdString(tables[8].rows[5][0]),
                 QStringLiteral("Stage A (N / mean / σ̂_MR)"));
        QCOMPARE(QString::fromStdString(tables[8].rows[6][1]),
                 QStringLiteral("Not specified"));
        QCOMPARE(QString::fromStdString(tables[8].rows[8][0]),
                 QStringLiteral("Nelson excluded MR count"));
    }

    void equivalence_tost_nested_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "等价性 TOST 嵌套 chrome";
        page.parameter_summary = "α = 0.05    界限 = [-0.5, 0.5]";
        page.diagnostics = {
            {DiagnosticMessage::Severity::error, "analysis",
             "请指定等价下限和上限。"},
            {DiagnosticMessage::Severity::error, "analysis",
             "请指定比值等价下限和上限。"},
            {DiagnosticMessage::Severity::error, "analysis",
             "请指定目标比例。"},
        };
        datalab::domain::StatisticTable groups;
        groups.title = "描述统计";
        groups.headers = {"组", "N", "Mean", "StDev"};
        groups.rows = {
            {"样本 1", "10", "1.0", "0.2"},
            {"样本 2", "10", "1.1", "0.2"},
            {"配对差值", "10", "0.1", "0.1"},
            {"参考样本", "10", "1.0", "0.2"},
            {"检验样本", "10", "1.05", "0.2"},
        };
        page.tables.push_back(groups);
        datalab::domain::StatisticTable prop;
        prop.title = "描述统计";
        prop.headers = {"组", "事件数", "试验数", "比例"};
        prop.rows = {
            {"样本", "5", "20", "0.25"},
            {"目标比例", "*", "*", "0.3"},
        };
        page.tables.push_back(prop);
        datalab::domain::StatisticTable test;
        test.title = "等价性检验";
        test.headers = {"差值", "下限 t", "下限 P", "上限 t", "上限 P", "α", "CI",
                        "CI 方法", "界限", "结论"};
        test.rows = {{"0.1", "1.2", "0.1", "2.3", "0.01", "0.05", "[-0.1, 0.3]",
                      "t", "[-0.5, 0.5]", "within_limits"}};
        page.tables.push_back(test);
        datalab::domain::StatisticTable ratio;
        ratio.title = "等价性检验";
        ratio.headers = {"比值", "下限 z", "下限 P", "上限 z", "上限 P", "α", "CI",
                         "CI 方法", "界限", "结论"};
        ratio.rows = {{"1.05", "1.0", "0.2", "1.1", "0.1", "0.05", "[0.9, 1.2]",
                       "z", "[0.8, 1.25]", "not_within_limits"}};
        page.tables.push_back(ratio);

        datalab::domain::PlotSpec interval;
        interval.title = "等价性区间";
        interval.x_axis_title = "比例差";
        interval.y_axis_title = "差值";
        datalab::domain::PlotSeries lower;
        lower.label = "等价下限";
        datalab::domain::PlotSeries upper;
        upper.label = "等价上限";
        interval.series = {lower, upper};
        page.plots.push_back(interval);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());
        QCOMPARE(QString::fromStdString(
                     localized.document.pages[0].source_page.parameter_summary),
                 QStringLiteral("α = 0.05    Limits = [-0.5, 0.5]"));
        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(QString::fromStdString(diags[0].message),
                 QStringLiteral("Specify the equivalence lower and upper limits."));
        QCOMPARE(QString::fromStdString(diags[1].message),
                 QStringLiteral(
                     "Specify the ratio equivalence lower and upper limits."));
        QCOMPARE(QString::fromStdString(diags[2].message),
                 QStringLiteral("Specify the target proportion."));
        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(QString::fromStdString(tables[0].rows[0][0]),
                 QStringLiteral("Sample 1"));
        QCOMPARE(QString::fromStdString(tables[0].rows[2][0]),
                 QStringLiteral("Paired difference"));
        QCOMPARE(QString::fromStdString(tables[0].rows[3][0]),
                 QStringLiteral("Reference sample"));
        QCOMPARE(QString::fromStdString(tables[0].rows[4][0]),
                 QStringLiteral("Test sample"));
        QCOMPARE(QString::fromStdString(tables[1].rows[1][0]),
                 QStringLiteral("Target proportion"));
        QCOMPARE(QString::fromStdString(tables[2].headers[1]),
                 QStringLiteral("Lower t"));
        QCOMPARE(QString::fromStdString(tables[2].headers[7]),
                 QStringLiteral("CI method"));
        QCOMPARE(QString::fromStdString(tables[2].headers[8]),
                 QStringLiteral("Limits"));
        QCOMPARE(QString::fromStdString(tables[2].headers[9]),
                 QStringLiteral("Conclusion"));
        QCOMPARE(QString::fromStdString(tables[3].headers[0]),
                 QStringLiteral("Ratio"));
        QCOMPARE(QString::fromStdString(tables[3].headers[1]),
                 QStringLiteral("Lower z"));
        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(QString::fromStdString(plots[0].x_axis_title),
                 QStringLiteral("Proportion difference"));
        QCOMPARE(QString::fromStdString(plots[0].y_axis_title),
                 QStringLiteral("Difference"));
        QCOMPARE(QString::fromStdString(plots[0].series[0].label),
                 QStringLiteral("Equivalence lower"));
        QCOMPARE(QString::fromStdString(plots[0].series[1].label),
                 QStringLiteral("Equivalence upper"));
    }

    void nonparametric_nested_headers_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "非参数嵌套表头切片";
        datalab::domain::StatisticTable wilcoxon;
        wilcoxon.title = "符号秩检验";
        wilcoxon.headers = {"非零差值 N", "η0", "正秩和", "负秩和", "Z", "P-Value",
                            "Ties 修正", "连续性修正", "近似方法", "小样本警告"};
        page.tables.push_back(wilcoxon);
        datalab::domain::StatisticTable sign_summary;
        sign_summary.title = "符号摘要";
        sign_summary.headers = {
            "有效符号 N", "结数", "n+", "n-", "样本中位数", "假设中位数 η0"};
        page.tables.push_back(sign_summary);
        datalab::domain::StatisticTable sign_ci;
        sign_ci.title = "中位数置信区间";
        sign_ci.headers = {
            "估计中位数", "CI 下限", "CI 上限", "名义置信水平", "达到水平", "方法"};
        page.tables.push_back(sign_ci);
        datalab::domain::StatisticTable runs_crit;
        runs_crit.title = "比较准则";
        runs_crit.headers = {"准则", "K", "N", "≤K (B)", ">K (A)"};
        page.tables.push_back(runs_crit);
        datalab::domain::StatisticTable runs_test;
        runs_test.title = "Runs检验";
        runs_test.headers = {"Observed", "Expected", "Z", "P-Value", "近似", "小样本警告"};
        page.tables.push_back(runs_test);
        datalab::domain::StatisticTable cochran;
        cochran.title = "处理成功计数";
        cochran.headers = {"处理", "成功数", "成功率", "N"};
        page.tables.push_back(cochran);
        datalab::domain::StatisticTable mood;
        mood.title = "Mood 中位数检验";
        mood.headers = {"总体中位数 M", "χ²", "DF", "P-Value", "近似方法"};
        page.tables.push_back(mood);
        datalab::domain::StatisticTable kw;
        kw.title = "Kruskal-Wallis 结果";
        kw.headers = {"组别", "N", "中位数", "平均秩", "Z"};
        page.tables.push_back(kw);
        datalab::domain::StatisticTable kw_stat;
        kw_stat.title = "检验统计量";
        kw_stat.headers = {"H", "调整后 H", "DF", "P-Value", "未调整 P", "Ties 修正",
                           "小样本警告", "近似方法", "效应量"};
        page.tables.push_back(kw_stat);
        datalab::domain::StatisticTable steel;
        steel.title = "Steel-Dwass 成对比较";
        steel.headers = {"对比", "Z", "未调整 P", "Bonferroni P", "显著"};
        page.tables.push_back(steel);
        datalab::domain::StatisticTable dunn;
        dunn.title = "Dunn 成对比较";
        dunn.headers = {
            "对比", "平均秩差", "SE", "Z", "未调整 P", "Bonferroni P", "显著"};
        page.tables.push_back(dunn);
        datalab::domain::StatisticTable friedman;
        friedman.title = "Friedman 检验";
        friedman.headers = {"S", "调整后 S", "DF", "P-Value", "Ties 修正", "区组数",
                            "处理数"};
        page.tables.push_back(friedman);

        datalab::domain::PlotSpec runs_plot;
        runs_plot.title = "Length 游程序列图";
        runs_plot.x_axis_title = "观测序号";
        page.plots.push_back(runs_plot);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());
        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(static_cast<int>(tables.size()), 12);
        QCOMPARE(QString::fromStdString(tables[0].headers[0]),
                 QStringLiteral("Nonzero differences N"));
        QCOMPARE(QString::fromStdString(tables[0].headers[2]),
                 QStringLiteral("Positive rank sum"));
        QCOMPARE(QString::fromStdString(tables[1].headers[0]),
                 QStringLiteral("Valid signs N"));
        QCOMPARE(QString::fromStdString(tables[1].headers[1]), QStringLiteral("Ties"));
        QCOMPARE(QString::fromStdString(tables[1].headers[5]),
                 QStringLiteral("Hypothesized median η0"));
        QCOMPARE(QString::fromStdString(tables[2].headers[0]),
                 QStringLiteral("Estimated median"));
        QCOMPARE(QString::fromStdString(tables[2].headers[3]),
                 QStringLiteral("Nominal confidence level"));
        QCOMPARE(QString::fromStdString(tables[3].headers[0]),
                 QStringLiteral("Criterion"));
        QCOMPARE(QString::fromStdString(tables[4].headers[4]),
                 QStringLiteral("Approximation"));
        QCOMPARE(QString::fromStdString(tables[5].headers[1]),
                 QStringLiteral("Successes"));
        QCOMPARE(QString::fromStdString(tables[6].headers[0]),
                 QStringLiteral("Overall median M"));
        QCOMPARE(QString::fromStdString(tables[7].headers[2]), QStringLiteral("Median"));
        QCOMPARE(QString::fromStdString(tables[8].headers[1]),
                 QStringLiteral("Adjusted H"));
        QCOMPARE(QString::fromStdString(tables[9].title),
                 QStringLiteral("Steel-Dwass pairwise comparisons"));
        QCOMPARE(QString::fromStdString(tables[10].title),
                 QStringLiteral("Dunn pairwise comparisons"));
        QCOMPARE(QString::fromStdString(tables[11].headers[1]),
                 QStringLiteral("Adjusted S"));
        QCOMPARE(QString::fromStdString(tables[11].headers[5]),
                 QStringLiteral("Blocks"));
        QCOMPARE(QString::fromStdString(tables[11].headers[6]),
                 QStringLiteral("Treatments"));
        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(static_cast<int>(plots.size()), 1);
        QCOMPARE(QString::fromStdString(plots[0].title),
                 QStringLiteral("Length Runs series plot"));
        QCOMPARE(QString::fromStdString(plots[0].x_axis_title),
                 QStringLiteral("Observation index"));
    }

    void arima_fixed_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "ARIMA / Best ARIMA 固定诊断";
        page.diagnostics = {
            {DiagnosticMessage::Severity::warning, "aicc_unavailable",
             "有效样本不足以进行 AICc 小样本修正，已返回未修正的 AIC。"},
            {DiagnosticMessage::Severity::warning, "near_zero_residual_variance",
             "残差平方和接近零，信息准则使用数值下限稳定计算。"},
            {DiagnosticMessage::Severity::error, "ar_fit_failed", "AR(1) 拟合失败。"},
            {DiagnosticMessage::Severity::error, "ma_fit_failed", "MA(1) 拟合失败。"},
            {DiagnosticMessage::Severity::error, "insufficient_differenced_samples",
             "差分后样本不足以拟合 ARIMA 阶数。"},
            {DiagnosticMessage::Severity::error, "ar_fit_failed",
             "AR 拟合失败或设计矩阵秩亏。"},
            {DiagnosticMessage::Severity::error, "ma_fit_failed", "MA 拟合失败。"},
            {DiagnosticMessage::Severity::error, "unsupported_arima_order",
             "当前仅支持 AR(p,d,0) 与 MA(0,d,q) 候选，不含混合 ARMA 阶。"},
            {DiagnosticMessage::Severity::error, "insufficient_samples",
             "ARIMA 候选要求更多的有限观测值才能拟合。"},
            {DiagnosticMessage::Severity::error, "invalid_forecast_horizon",
             "预测期数必须大于零。"},
            {DiagnosticMessage::Severity::error, "non_finite_observation",
             "观测序列必须全部为有限数值。"},
            {DiagnosticMessage::Severity::error, "non_finite_result",
             "模型计算产生了非有限结果，请检查数据尺度后重试。"},
            {DiagnosticMessage::Severity::error, "unsupported_arima_order",
             "仅支持 p/q≤3、d≤2 的有限阶 ARIMA。"},
            {DiagnosticMessage::Severity::error, "degenerate_arima_order",
             "ARIMA(0,0,0) 不在候选网格内。"},
            {DiagnosticMessage::Severity::error, "unsupported_arima_order",
             "混合 ARMA 阶尚未纳入 Best ARIMA 网格。"},
            {DiagnosticMessage::Severity::warning, "arima_css_approximation",
             "Best ARIMA 候选基于条件最小二乘与差分尺度 AICc；与 Minitab 迭代最小二乘（Meeker TSERIES）的最优阶可能不同。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());
        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 16);
        QCOMPARE(QString::fromStdString(diags[0].message),
                 QStringLiteral(
                     "Effective sample size is too small for AICc small-sample correction; "
                     "uncorrected AIC is returned."));
        QCOMPARE(QString::fromStdString(diags[2].message),
                 QStringLiteral("AR(1) fit failed."));
        QCOMPARE(QString::fromStdString(diags[5].message),
                 QStringLiteral(
                     "AR fit failed or the design matrix is rank-deficient."));
        QCOMPARE(QString::fromStdString(diags[8].message),
                 QStringLiteral(
                     "More finite observations are required to fit this ARIMA candidate."));
        QCOMPARE(QString::fromStdString(diags[13].message),
                 QStringLiteral("ARIMA(0,0,0) is not in the candidate grid."));
        QCOMPARE(QString::fromStdString(diags[15].message),
                 QStringLiteral(
                     "Best ARIMA candidates use conditional least squares and "
                     "differenced-scale AICc; the selected order may differ from Minitab "
                     "iterative least squares (Meeker TSERIES)."));
        for (const auto& diagnostic : diags) {
            QVERIFY2(diagnostic.message.find("拟合失败") == std::string::npos,
                     diagnostic.message.c_str());
            QVERIFY2(diagnostic.message.find("。") == std::string::npos,
                     diagnostic.message.c_str());
        }
    }

    void rsm_prefix_acceptance_poisson_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "RSM / 前缀 / 验收 / 泊松 GOF 诊断";
        page.diagnostics = {
            {DiagnosticMessage::Severity::error, "rsd_empty_factor_id",
             "因素 ID 不能为空。"},
            {DiagnosticMessage::Severity::error, "rsd_nonfinite_bounds",
             "因素低/高水平必须有限。"},
            {DiagnosticMessage::Severity::error, "rsd_invalid_center",
             "中心值必须有限且落在 [low, high] 内。"},
            {DiagnosticMessage::Severity::error, "ccd_invalid_alpha",
             "alpha 必须为正有限数。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "相关分析：跳过 2 个缺失或非法单元格。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "单样本 t：跳过 1 个缺失或非法单元格。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "单样本 Z：跳过 1 个缺失或非法单元格。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "双样本 t：跳过 3 个缺失或非法单元格。"},
            {DiagnosticMessage::Severity::error, "invalid_specification",
             "能力分析: LSL 必须为有限数。"},
            {DiagnosticMessage::Severity::warning, "prediction",
             "响应优化预测：缺失响应运行未参与拟合。"},
            {DiagnosticMessage::Severity::error, "acceptance_n_zero",
             "样本量 n 必须 ≥ 1。"},
            {DiagnosticMessage::Severity::error, "acceptance_c_too_large",
             "接收数 c 不能大于样本量 n。"},
            {DiagnosticMessage::Severity::warning, "acceptance_lot_lt_n",
             "批大小 N < n；本轮仍用二项 OC（无限批近似）。"},
            {DiagnosticMessage::Severity::warning, "anom_unequal_n",
             "各组样本量不等；决策限仍用总 N 近似，解读需谨慎。"},
            {DiagnosticMessage::Severity::warning, "anom_limits_approx",
             "决策限采用正态/多重比较近似（formula_reference），不是 Minitab exact h 表 golden。"},
            {DiagnosticMessage::Severity::error, "poisson_gof_non_integer",
             "泊松拟合优度要求非负整数计数。"},
            {DiagnosticMessage::Severity::error, "poisson_gof_insufficient_n",
             "至少需要 2 个有效计数。"},
            {DiagnosticMessage::Severity::error, "poisson_gof_too_few_bins",
             "合并后类别不足，无法计算拟合优度。"},
            {DiagnosticMessage::Severity::error, "poisson_gof_df",
             "自由度 < 1（类别过少）。"},
            {DiagnosticMessage::Severity::warning, "poisson_gof_small_expected",
             "存在期望频数 < 5，卡方近似需谨慎。"},
            {DiagnosticMessage::Severity::error, "design_gen",
             "设计生成失败。"},
            {DiagnosticMessage::Severity::error, "chart_empty",
             "Xbar 图没有可显示的数据。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());
        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 22);
        QCOMPARE(QString::fromStdString(diags[0].message),
                 QStringLiteral("Factor ID must not be empty."));
        QCOMPARE(QString::fromStdString(diags[3].message),
                 QStringLiteral("alpha must be a positive finite number."));
        QCOMPARE(QString::fromStdString(diags[4].message),
                 QStringLiteral("Correlation: Skipped 2 missing or invalid cell(s)."));
        QCOMPARE(QString::fromStdString(diags[5].message),
                 QStringLiteral("One-sample t: Skipped 1 missing or invalid cell(s)."));
        QCOMPARE(QString::fromStdString(diags[8].message),
                 QStringLiteral("Capability: LSL must be a finite number."));
        QCOMPARE(QString::fromStdString(diags[9].message),
                 QStringLiteral(
                     "Response optimization prediction: Runs with missing responses were "
                     "not included in the fit."));
        QCOMPARE(QString::fromStdString(diags[10].message),
                 QStringLiteral("Sample size n must be ≥ 1."));
        QCOMPARE(QString::fromStdString(diags[14].message),
                 QStringLiteral(
                     "Decision limits use a normal/multiple-comparison approximation "
                     "(formula_reference), not a Minitab exact h-table golden."));
        QCOMPARE(QString::fromStdString(diags[20].message),
                 QStringLiteral("Design generation failed."));
        QCOMPARE(QString::fromStdString(diags[21].message),
                 QStringLiteral("Xbar 图 has no displayable data."));
        for (const auto& diagnostic : diags) {
            QVERIFY2(diagnostic.message.find("因素") == std::string::npos,
                     diagnostic.message.c_str());
            QVERIFY2(diagnostic.message.find("相关分析") == std::string::npos,
                     diagnostic.message.c_str());
            QVERIFY2(diagnostic.message.find("没有可显示的数据。") == std::string::npos,
                     diagnostic.message.c_str());
        }
    }

    void spc_capability_anom_gate_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "SPC σ / 能力 / ANOM 门禁诊断";
        page.diagnostics = {
            {DiagnosticMessage::Severity::error, "zone_sigma",
             "无法估计过程 σ（移动极差）。"},
            {DiagnosticMessage::Severity::error, "zone_sigma",
             "过程 σ 必须大于 0。"},
            {DiagnosticMessage::Severity::error, "zmr_sigma", "无法估计 σ。"},
            {DiagnosticMessage::Severity::error, "zmr_sigma", "存在组 σ ≤ 0。"},
            {DiagnosticMessage::Severity::error, "ma_non_finite",
             "移动平均图不允许非有限观测。"},
            {DiagnosticMessage::Severity::error, "ma_sigma", "无法估计过程 σ。"},
            {DiagnosticMessage::Severity::error, "ma_sigma",
             "σ 与控制限倍数必须大于 0。"},
            {DiagnosticMessage::Severity::error, "within_sigma",
             "组内 σ 至少需要两个子组。"},
            {DiagnosticMessage::Severity::error, "within_sigma",
             "各子组必须具有相同观测数。"},
            {DiagnosticMessage::Severity::error, "within_sigma",
             "子组不允许 NaN 或无穷观测。"},
            {DiagnosticMessage::Severity::error, "within_sigma",
             "子组大小超出无偏常数表范围。"},
            {DiagnosticMessage::Severity::error, "within_sigma",
             "无法计算该子组大小的 c4。"},
            {DiagnosticMessage::Severity::error, "invalid_specification",
             "LSL 必须为有限数。"},
            {DiagnosticMessage::Severity::error, "invalid_specification",
             "USL 必须为有限数。"},
            {DiagnosticMessage::Severity::error, "invalid_mean",
             "过程均值必须为有限数。"},
            {DiagnosticMessage::Severity::error, "bw_capability",
             "组间/组内能力需要有效数值观测。"},
            {DiagnosticMessage::Severity::error, "anom_groups",
             "ANOM 至少需要两个组。"},
            {DiagnosticMessage::Severity::error, "anom_groups",
             "每组至少需要 2 个观测。"},
            {DiagnosticMessage::Severity::error, "anom_var",
             "无法估计组内方差。"},
            {DiagnosticMessage::Severity::error, "anom_var",
             "组内标准差为 0。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());
        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 20);
        QCOMPARE(QString::fromStdString(diags[0].message),
                 QStringLiteral("Unable to estimate process σ (moving range)."));
        QCOMPARE(QString::fromStdString(diags[5].message),
                 QStringLiteral("Unable to estimate process σ."));
        QCOMPARE(QString::fromStdString(diags[8].message),
                 QStringLiteral(
                     "All subgroups must have the same number of observations."));
        QCOMPARE(QString::fromStdString(diags[12].message),
                 QStringLiteral("LSL must be a finite number."));
        QCOMPARE(QString::fromStdString(diags[16].message),
                 QStringLiteral("ANOM requires at least two groups."));
        QCOMPARE(QString::fromStdString(diags[19].message),
                 QStringLiteral("Within-group standard deviation is 0."));
        for (const auto& diagnostic : diags) {
            QVERIFY2(diagnostic.message.find("无法") == std::string::npos,
                     diagnostic.message.c_str());
            QVERIFY2(diagnostic.message.find("必须") == std::string::npos,
                     diagnostic.message.c_str());
        }
    }

    void skip_complete_case_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "跳过与 complete-case 诊断切片";
        page.diagnostics = {
            {DiagnosticMessage::Severity::warning, "missing_values",
             "双样本等价性检验跳过缺失或非法单元格。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "均值比等价性检验跳过缺失或非法单元格。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "配对等价性检验按 complete-case 对齐，已跳过不成对的缺失或非法单元格。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "两比例等价性检验跳过了缺失或非法单元格。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "回归使用 complete-case，跳过响应缺失、预测变量缺失或无效的行。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "两比例检验跳过了缺失或非法单元格。"},
            {DiagnosticMessage::Severity::error, "runs_interior_missing",
             "序列中间存在缺失或非法值，游程检验要求完整连续观测，未计算检验。"},
            {DiagnosticMessage::Severity::info, "missing_values",
             "平滑跳过缺失或无效的数值单元格。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "Logistic 回归使用 complete-case，缺失或非法行已排除。"},
            {DiagnosticMessage::Severity::info, "missing_values",
             "缺失、* 或非法数值未进入容差区间计算。"},
            {DiagnosticMessage::Severity::warning, "incomplete_time_value_pairs",
             "时间列与值列未配对的行已跳过，分解仅使用可对齐的完整观测。"},
            {DiagnosticMessage::Severity::info, "missing_values",
             "分解跳过缺失或无效的时间/数值单元格。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "Nested Gage R&R 跳过缺失或非法的测量/零件/操作者单元格。"},
            {DiagnosticMessage::Severity::error, "run_chart_interior_missing",
             "序列中间存在缺失或非法值，运行图随机性检验要求完整连续观测，未计算四模式 P。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "跳过了缺失或非法单元格。"},
            {DiagnosticMessage::Severity::warning, "missing_row_excluded",
             "包含缺失或非有限值的观测行已被排除。"},
            {DiagnosticMessage::Severity::warning, "missing_doe_responses",
             "缺失或非有限响应未参与 DOE 效应摘要。"},
            {DiagnosticMessage::Severity::warning, "missing_doe_response",
             "缺失响应运行未参与拟合。"},
            {DiagnosticMessage::Severity::warning, "violin_group_too_small",
             "某分组观测少于 2，已跳过密度形状。"},
            {DiagnosticMessage::Severity::error, "insufficient_observations",
             "没有可用于 Multi-Vari 图的 complete-case 观测。"},
            {DiagnosticMessage::Severity::error, "invalid_target",
             "Target 必须为有限数，不能静默跳过。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "跳过 3 个缺失或非法单元格。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "跳过 2 个缺失或非法单元格（含 *）。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "跳过 1 个缺失或非法单元格（计入 N*）。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "描述统计跳过 4 个缺失或非法单元格（含 *）。"},
            {DiagnosticMessage::Severity::warning, "missing_values",
             "泊松拟合优度跳过 5 个缺失或非法单元格。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());
        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 26);
        QCOMPARE(QString::fromStdString(diags[0].message),
                 QStringLiteral("Two-sample equivalence skipped missing or invalid cells."));
        QCOMPARE(QString::fromStdString(diags[2].message),
                 QStringLiteral(
                     "Paired equivalence used complete-case alignment and skipped unpaired "
                     "missing or invalid cells."));
        QCOMPARE(QString::fromStdString(diags[4].message),
                 QStringLiteral(
                     "Regression used complete-case and skipped rows with missing response, "
                     "missing predictors, or invalid values."));
        QCOMPARE(QString::fromStdString(diags[8].message),
                 QStringLiteral(
                     "Logistic regression used complete-case; missing or invalid rows were "
                     "excluded."));
        QCOMPARE(QString::fromStdString(diags[12].message),
                 QStringLiteral(
                     "Nested Gage R&R skipped missing or invalid measurement/part/operator "
                     "cells."));
        QCOMPARE(QString::fromStdString(diags[19].message),
                 QStringLiteral(
                     "No complete-case observations are available for the Multi-Vari chart."));
        QCOMPARE(QString::fromStdString(diags[20].message),
                 QStringLiteral("Target must be finite; it cannot be silently skipped."));
        QCOMPARE(QString::fromStdString(diags[21].message),
                 QStringLiteral("Skipped 3 missing or invalid cell(s)."));
        QCOMPARE(QString::fromStdString(diags[22].message),
                 QStringLiteral("Skipped 2 missing or invalid cell(s) (including *)."));
        QCOMPARE(QString::fromStdString(diags[23].message),
                 QStringLiteral("Skipped 1 missing or invalid cell(s) (counted in N*)."));
        QCOMPARE(QString::fromStdString(diags[24].message),
                 QStringLiteral(
                     "Descriptive statistics skipped 4 missing or invalid cell(s) "
                     "(including *)."));
        QCOMPARE(QString::fromStdString(diags[25].message),
                 QStringLiteral(
                     "Poisson goodness-of-fit skipped 5 missing or invalid cell(s)."));
        for (const auto& diagnostic : diags) {
            QVERIFY2(diagnostic.message.find("跳过") == std::string::npos,
                     diagnostic.message.c_str());
            QVERIFY2(diagnostic.message.find("。") == std::string::npos,
                     diagnostic.message.c_str());
        }
    }

    void nested_chrome_leftovers_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "嵌套 chrome 余量切片";
        const char* table_titles[] = {
            "ANOVA 表",
            "方差分量（交叉 ANOVA）",
            "因子 A 均值",
            "因子 B 均值",
            "拟合优度",
            "系数与 Odds Ratio",
            "拟合与残差",
            "参数估计",
            "方差检验结果",
            "摘要",
            "I-MR-R/S 逐子组统计",
            "缺陷计数",
            "泊松参数",
        };
        for (const char* title : table_titles) {
            datalab::domain::StatisticTable table;
            table.title = title;
            page.tables.push_back(table);
        }
        datalab::domain::StatisticTable gof;
        gof.title = "拟合优度";
        gof.headers = {"检验", "组数", "状态"};
        gof.rows = {{"Hosmer-Lemeshow", "10", "ok"}};
        page.tables.push_back(gof);
        datalab::domain::StatisticTable desir;
        desir.title = "最佳组合";
        desir.headers = {"因子", "总体 D"};
        desir.rows = {{"A", "0.9"}};
        page.tables.push_back(desir);

        datalab::domain::PlotSpec diff;
        diff.title = "差值置信区间";
        diff.x_axis_title = "对比";
        datalab::domain::PlotSpec rate;
        rate.title = "率比置信区间";
        rate.x_axis_title = "率比";
        datalab::domain::PlotSpec pred;
        pred.title = "预测概率";
        pred.y_axis_title = "事件概率";
        datalab::domain::PlotSpec tol;
        tol.title = "容差区间直方图";
        datalab::domain::PlotSpec pareto;
        pareto.title = "Defect 的 Pareto 图";
        pareto.x_axis_title = "类别";
        pareto.y_axis_title = "计数";
        datalab::domain::PlotSpec cat_idx;
        cat_idx.title = "条形图";
        cat_idx.x_axis_title = "类别序号";
        datalab::domain::PlotSpec transform;
        transform.title = "Box-Cox λ 选择诊断";
        transform.y_axis_title = "标准化变换 SD";
        datalab::domain::PlotSpec tv;
        tv.title = "Johnson 变换";
        tv.y_axis_title = "变换值";
        datalab::domain::PlotSpec rank;
        rank.title = "候选组合总体 Desirability";
        rank.x_axis_title = "候选排序";
        datalab::domain::PlotSpec meas;
        meas.title = "Gage Stability Run Chart";
        meas.x_axis_title = "测量序号";
        datalab::domain::PlotSpec cause;
        cause.title = "因果图";
        cause.y_axis_title = "原因数";
        page.plots = {diff, rate, pred, tol, pareto, cat_idx, transform, tv, rank, meas,
                      cause};

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(static_cast<int>(tables.size()), 15);
        QCOMPARE(QString::fromStdString(tables[0].title), QStringLiteral("ANOVA table"));
        QCOMPARE(QString::fromStdString(tables[1].title),
                 QStringLiteral("Variance components (crossed ANOVA)"));
        QCOMPARE(QString::fromStdString(tables[2].title), QStringLiteral("Factor A means"));
        QCOMPARE(QString::fromStdString(tables[4].title), QStringLiteral("Goodness of fit"));
        QCOMPARE(QString::fromStdString(tables[5].title),
                 QStringLiteral("Coefficients and odds ratios"));
        QCOMPARE(QString::fromStdString(tables[9].title), QStringLiteral("Summary"));
        QCOMPARE(QString::fromStdString(tables[10].title),
                 QStringLiteral("I-MR-R/S subgroup statistics"));
        QCOMPARE(QString::fromStdString(tables[11].title), QStringLiteral("Defect counts"));
        QCOMPARE(QString::fromStdString(tables[12].title), QStringLiteral("Poisson parameters"));
        QCOMPARE(QString::fromStdString(tables[13].headers[0]), QStringLiteral("Test"));
        QCOMPARE(QString::fromStdString(tables[13].headers[1]),
                 QStringLiteral("Number of groups"));
        QCOMPARE(QString::fromStdString(tables[14].headers[1]), QStringLiteral("Overall D"));

        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(static_cast<int>(plots.size()), 11);
        QCOMPARE(QString::fromStdString(plots[0].title),
                 QStringLiteral("Difference confidence interval"));
        QCOMPARE(QString::fromStdString(plots[1].title),
                 QStringLiteral("Rate-ratio confidence interval"));
        QCOMPARE(QString::fromStdString(plots[1].x_axis_title), QStringLiteral("Rate ratio"));
        QCOMPARE(QString::fromStdString(plots[2].title), QStringLiteral("Predicted probability"));
        QCOMPARE(QString::fromStdString(plots[2].y_axis_title),
                 QStringLiteral("Event probability"));
        QCOMPARE(QString::fromStdString(plots[3].title),
                 QStringLiteral("Tolerance-interval histogram"));
        QCOMPARE(QString::fromStdString(plots[4].title), QStringLiteral("Defect Pareto chart"));
        QCOMPARE(QString::fromStdString(plots[4].x_axis_title), QStringLiteral("Category"));
        QCOMPARE(QString::fromStdString(plots[5].x_axis_title),
                 QStringLiteral("Category index"));
        QCOMPARE(QString::fromStdString(plots[6].y_axis_title),
                 QStringLiteral("Standardized transformed SD"));
        QCOMPARE(QString::fromStdString(plots[7].y_axis_title),
                 QStringLiteral("Transformed value"));
        QCOMPARE(QString::fromStdString(plots[8].x_axis_title),
                 QStringLiteral("Candidate rank"));
        QCOMPARE(QString::fromStdString(plots[9].x_axis_title),
                 QStringLiteral("Measurement index"));
        QCOMPARE(QString::fromStdString(plots[10].y_axis_title),
                 QStringLiteral("Number of causes"));
    }

    void msa_aa_rule_evidence_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "规则证据切片";
        datalab::domain::StatisticTable rules;
        rules.title = "规则证据";
        rules.headers = {"规则", "名称", "状态", "证据", "关联行", "建议"};
        rules.rows = {
            {"ndc_not_computed", "ndc_not_computed", "not_computed",
             "ndc 不可估计；不能据此评价测量系统分辨力。", "",
             "先检查零总变异、零 Gage 标准差或无效容差后再解释 ndc。"},
            {"ndc_investigation", "ndc 调查", "triggered",
             "ndc < 5，提示测量系统对零件间差异的分辨力需要调查。", "",
             "ndc 小于 5 只是调查提示，不是量具不合格的绝对结论。"},
            {"ndc_investigation", "ndc 调查", "not_triggered",
             "ndc ≥ 5；这只说明当前研究中零件间变异相对 Gage 变异较大。", "",
             "仍需结合 %Study Var、%Tolerance 和现场公差风险解释。"},
            {"design_balance", "设计平衡", "triggered",
             "零件×操作员单元重复次数不一致。", "",
             "交叉设计需要平衡重复后才能解释方差分量。"},
            {"design_balance", "设计平衡", "triggered",
             "嵌套设计零件重复次数不一致。", "",
             "先补齐平衡重复后再解释方差分量。"},
            {"design_balance", "设计平衡", "not_triggered",
             "交叉设计单元重复次数平衡。", "",
             "平衡设计是 ANOVA 方差分量解释的前提。"},
            {"interaction_model", "interaction_model", "triggered",
             "交互项 p>0.25，可考虑缩减，但当前保留完整模型。", "",
             "交互是否缩减必须回显；本实现不自动并入重复性。"},
            {"percent_metrics", "percent_metrics", "not_triggered",
             "%Contribution 基于方差，%Study Var 基于标准差，口径不同。", "",
             "不要把 %Contribution 与 %Study Var 当成同一个百分比。"},
            {"invalid_tolerance", "invalid_tolerance", "triggered",
             "未提供有效公差，%Tolerance 不可用。", "",
             "只有有限正公差才能计算 %Tolerance。"},
            {"kappa_interpretation", "kappa_interpretation", "not_triggered",
             "拒绝 Kappa=0 不等于已证明评估者一致。", "",
             "Kappa 只描述超出偶然的绝对一致率，不能写成测量系统合格。"},
            {"weighted_kappa_not_minitab_aaa", "weighted_kappa_not_minitab_aaa",
             "not_triggered",
             "Weighted Kappa 是 DataLab 可选 Cohen 加权，不是 Minitab AAA 默认输出。", "",
             "Minitab 有序评级路径使用 Kendall；不要把加权 κ 写成 Minitab AAA 结果。"},
            {"kendall_interpretation", "kendall_interpretation", "not_triggered",
             "有序评级已计算 Kendall W/τ；拒绝系数为 0 不等于已证明有序一致。", "",
             "不要把 Kendall 写成加权 Kappa，也不要把未拒绝原假设写成已证明一致。"},
            {"kendall_interpretation", "kendall_interpretation", "triggered",
             "已请求有序评级，但 Kendall 不可识别或等级不足。", "",
             "不要把 Kendall 写成加权 Kappa，也不要把未拒绝原假设写成已证明一致。"},
            {"negative_variance", "negative_variance", "triggered",
             "存在负方差分量，已截断为 0，并保留原始估计。", "",
             "截断后的分量用于 %Contribution；解释时同时查看原始值。"},
        };
        page.tables = {rules};

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& table = localized.document.pages[0].visible_tables.front();
        QCOMPARE(QString::fromStdString(table.title), QStringLiteral("Rule evidence"));
        QCOMPARE(QString::fromStdString(table.headers[0]), QStringLiteral("Rule"));
        QCOMPARE(QString::fromStdString(table.headers[1]), QStringLiteral("Name"));
        QCOMPARE(QString::fromStdString(table.headers[2]), QStringLiteral("Status"));
        QCOMPARE(QString::fromStdString(table.headers[3]), QStringLiteral("Evidence"));
        QCOMPARE(QString::fromStdString(table.headers[4]), QStringLiteral("Related rows"));
        QCOMPARE(QString::fromStdString(table.headers[5]), QStringLiteral("Suggestion"));
        QCOMPARE(static_cast<int>(table.rows.size()), 14);
        QVERIFY2(table.rows[0][3].find("cannot be estimated") != std::string::npos,
                 table.rows[0][3].c_str());
        QVERIFY2(table.rows[0][5].find("zero Gage SD") != std::string::npos,
                 table.rows[0][5].c_str());
        QVERIFY2(table.rows[1][3].find("ndc < 5") != std::string::npos,
                 table.rows[1][3].c_str());
        QCOMPARE(QString::fromStdString(table.rows[1][1]),
                 QStringLiteral("ndc investigation"));
        QVERIFY2(table.rows[2][3].find("ndc ≥ 5") != std::string::npos,
                 table.rows[2][3].c_str());
        QVERIFY2(table.rows[3][3].find("inconsistent") != std::string::npos,
                 table.rows[3][3].c_str());
        QVERIFY2(table.rows[4][3].find("Nested-design") != std::string::npos,
                 table.rows[4][3].c_str());
        QCOMPARE(QString::fromStdString(table.rows[5][1]), QStringLiteral("Design balance"));
        QVERIFY2(table.rows[6][3].find("Interaction p>0.25") != std::string::npos,
                 table.rows[6][3].c_str());
        QVERIFY2(table.rows[7][3].find("%Contribution is variance-based") != std::string::npos,
                 table.rows[7][3].c_str());
        QVERIFY2(table.rows[9][3].find("does not prove rater agreement") != std::string::npos,
                 table.rows[9][3].c_str());
        QVERIFY2(table.rows[10][3].find("not Minitab AAA") != std::string::npos,
                 table.rows[10][3].c_str());
        QVERIFY2(table.rows[11][3].find("Kendall W/τ") != std::string::npos,
                 table.rows[11][3].c_str());
        QVERIFY2(table.rows[12][3].find("not identifiable") != std::string::npos,
                 table.rows[12][3].c_str());
        QVERIFY2(table.rows[13][3].find("truncated to 0") != std::string::npos,
                 table.rows[13][3].c_str());

        for (const auto& row : table.rows) {
            QVERIFY2(row[3].find("不可估计") == std::string::npos, row[3].c_str());
            QVERIFY2(row[3].find("评估者") == std::string::npos, row[3].c_str());
            QVERIFY2(row[5].find("量具") == std::string::npos, row[5].c_str());
            QVERIFY2(row[5].find("不要把") == std::string::npos, row[5].c_str());
        }
    }

    void ml_nested_adf_poisson_iforest_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "ML 嵌套 chrome 切片";
        page.parameter_summary =
            "回归 = const    预测变量 = 3    链 = log    异常数 = 2    "
            "类数 = 4    准确率 ≈ 0.91    水平 = 5";

        const char* table_titles[] = {
            "ADF 检验",
            "ADF 回归系数",
            "拟合与 Pearson 残差",
            "异常分数",
            "Bootstrap 摘要",
            "合并历程",
            "簇分配（切 k）",
            "系数（阈值与斜率）",
            "类均值",
            "相关矩阵",
            "模型摘要",
        };
        for (const char* title : table_titles) {
            datalab::domain::StatisticTable table;
            table.title = title;
            page.tables.push_back(table);
        }
        datalab::domain::StatisticTable poisson_summary;
        poisson_summary.title = "模型摘要";
        poisson_summary.headers = {"N", "迭代", "收敛", "LogLik"};
        poisson_summary.rows = {{"20", "8", "是", "1.2"}};
        page.tables.push_back(poisson_summary);

        datalab::domain::PlotSpec cart_plot;
        cart_plot.title = "观测 vs 拟合";
        cart_plot.x_axis_title = "观测";
        cart_plot.y_axis_title = "拟合";
        datalab::domain::PlotSpec adf_plot;
        adf_plot.title = "序列";
        datalab::domain::PlotSpec pearson_plot;
        pearson_plot.title = "拟合 vs Pearson 残差";
        datalab::domain::PlotSpec iforest_plot;
        iforest_plot.title = "Isolation 分数";
        datalab::domain::PlotSeries threshold;
        threshold.label = "阈值";
        iforest_plot.series = {threshold};
        datalab::domain::PlotSpec bootstrap_plot;
        bootstrap_plot.title = "Bootstrap 均值分布";
        datalab::domain::PlotSpec hclust_plot;
        hclust_plot.title = "层次聚类散点（前两列）";
        datalab::domain::PlotSpec lda_plot;
        lda_plot.title = "LD 投影（示意）";
        datalab::domain::PlotSpec ccf_plot;
        ccf_plot.title = "CCF";
        datalab::domain::PlotSeries upper;
        upper.label = "上置信限";
        datalab::domain::PlotSeries lower;
        lower.label = "下置信限";
        ccf_plot.series = {upper, lower};
        datalab::domain::PlotSpec corr_plot;
        corr_plot.title = "相关热图";
        corr_plot.x_axis_title = "变量";
        corr_plot.y_axis_title = "变量";
        page.plots = {cart_plot, adf_plot, pearson_plot, iforest_plot, bootstrap_plot,
                      hclust_plot, lda_plot, ccf_plot, corr_plot};

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].source_page.parameter_summary = page.parameter_summary;
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(static_cast<int>(tables.size()), 12);
        QCOMPARE(QString::fromStdString(tables[0].title), QStringLiteral("ADF test"));
        QCOMPARE(QString::fromStdString(tables[1].title),
                 QStringLiteral("ADF regression coefficients"));
        QCOMPARE(QString::fromStdString(tables[2].title),
                 QStringLiteral("Fitted and Pearson residuals"));
        QCOMPARE(QString::fromStdString(tables[3].title), QStringLiteral("Anomaly scores"));
        QCOMPARE(QString::fromStdString(tables[4].title), QStringLiteral("Bootstrap summary"));
        QCOMPARE(QString::fromStdString(tables[5].title), QStringLiteral("Merge history"));
        QCOMPARE(QString::fromStdString(tables[6].title),
                 QStringLiteral("Cluster assignment (cut k)"));
        QCOMPARE(QString::fromStdString(tables[7].title),
                 QStringLiteral("Coefficients (thresholds and slopes)"));
        QCOMPARE(QString::fromStdString(tables[8].title), QStringLiteral("Class means"));
        QCOMPARE(QString::fromStdString(tables[9].title), QStringLiteral("Correlation matrix"));
        QCOMPARE(QString::fromStdString(tables[11].headers[1]), QStringLiteral("Iterations"));
        QCOMPARE(QString::fromStdString(tables[11].headers[2]), QStringLiteral("Converged"));
        QCOMPARE(QString::fromStdString(tables[11].rows[0][2]), QStringLiteral("Yes"));

        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(static_cast<int>(plots.size()), 9);
        QCOMPARE(QString::fromStdString(plots[0].title), QStringLiteral("Observed vs fitted"));
        QCOMPARE(QString::fromStdString(plots[0].x_axis_title), QStringLiteral("Observation"));
        QCOMPARE(QString::fromStdString(plots[0].y_axis_title), QStringLiteral("Fitted"));
        QCOMPARE(QString::fromStdString(plots[1].title), QStringLiteral("Series"));
        QCOMPARE(QString::fromStdString(plots[2].title),
                 QStringLiteral("Fitted vs Pearson residual"));
        QCOMPARE(QString::fromStdString(plots[3].series[0].label), QStringLiteral("Threshold"));
        QCOMPARE(QString::fromStdString(plots[4].title),
                 QStringLiteral("Bootstrap mean distribution"));
        QCOMPARE(QString::fromStdString(plots[5].title),
                 QStringLiteral("Hierarchical cluster scatter (first two columns)"));
        QCOMPARE(QString::fromStdString(plots[6].title),
                 QStringLiteral("LD projection (schematic)"));
        QCOMPARE(QString::fromStdString(plots[7].series[0].label),
                 QStringLiteral("Upper confidence limit"));
        QCOMPARE(QString::fromStdString(plots[7].series[1].label),
                 QStringLiteral("Lower confidence limit"));
        QCOMPARE(QString::fromStdString(plots[8].title), QStringLiteral("Correlation heatmap"));
        QCOMPARE(QString::fromStdString(plots[8].x_axis_title), QStringLiteral("Variable"));
        QCOMPARE(QString::fromStdString(plots[8].y_axis_title), QStringLiteral("Variable"));

        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Regression = ") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Predictors = ") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Link = ") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Anomalies = ") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Classes = ") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Accuracy ≈ ") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Levels = ") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("预测变量") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("异常数") == std::string::npos, summary.c_str());
    }

    void equivalence_forecast_pca_ml_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "单样本等价性检验";
        const char* table_titles[] = {
            "拟合与预测明细",
            "预测准确度",
            "候选模型比较",
            "模型摘要与预测",
            "季节指数",
            "SARIMA 候选模型比较",
            "Rolling-origin 评估",
            "Rolling-origin 明细",
            "特征值与解释率",
            "主成分系数",
            "相关载荷",
            "主成分得分",
            "T² 与 Q 阈值",
            "T² 与 Q 残差",
            "簇摘要",
            "质心（分析尺度）",
            "簇分配",
            "树结点",
            "变量重要性",
            "训练集混淆矩阵",
            "逐步步骤",
            "选入项",
            "终模型系数",
            "功效与样本量",
            "抽样计划",
            "OC 曲线",
            "决策限",
            "组均值",
        };
        for (const char* title : table_titles) {
            datalab::domain::StatisticTable table;
            table.title = title;
            page.tables.push_back(table);
        }
        datalab::domain::StatisticTable pca_limits;
        pca_limits.title = "T² 与 Q 阈值";
        pca_limits.headers = {"分位数", "T² 限", "Q 限", "口径"};
        pca_limits.rows = {{"0.95", "1", "2", "经验分位（非 Minitab T² 控制图 UCL）"}};
        page.tables.push_back(pca_limits);
        datalab::domain::StatisticTable selected;
        selected.title = "选入项";
        selected.headers = {"Term"};
        selected.rows = {{"(仅截距)"}};
        page.tables.push_back(selected);
        datalab::domain::StatisticTable arima_headers;
        arima_headers.title = "候选模型比较";
        arima_headers.headers = {"模型", "最优模型", "截距", "系数/漂移", "预测期"};
        page.tables.push_back(arima_headers);

        datalab::domain::PlotSpec forecast_plot;
        forecast_plot.title = "实际值、拟合值与预测区间";
        datalab::domain::PlotSeries actual;
        actual.label = "实际值";
        datalab::domain::PlotSeries forecast;
        forecast.label = "预测值";
        datalab::domain::PlotSeries band;
        band.label = "预测区间";
        forecast_plot.series = {actual, forecast, band};
        datalab::domain::PlotSpec arima_plot;
        arima_plot.title = "ARIMA 拟合与预测";
        datalab::domain::PlotSpec seasonal_plot;
        seasonal_plot.title = "季节性拟合与预测";
        datalab::domain::PlotSpec pca_plot;
        pca_plot.title = "PCA 得分图";
        datalab::domain::PlotSpec kmeans_plot;
        kmeans_plot.title = "K-Means 散点（前两列）";
        datalab::domain::PlotSpec power_plot;
        power_plot.title = "功效曲线";
        datalab::domain::PlotSeries power;
        power.label = "功效";
        power_plot.series = {power};
        datalab::domain::PlotSpec oc_plot;
        oc_plot.title = "OC 曲线（二项）";
        oc_plot.x_axis_title = "不合格品率 p";
        oc_plot.y_axis_title = "接收概率 Pa(p)";
        datalab::domain::PlotSpec anom_plot;
        anom_plot.title = "Length ANOM 图";
        page.plots = {forecast_plot, arima_plot, seasonal_plot, pca_plot, kmeans_plot,
                      power_plot, oc_plot, anom_plot};

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].source_page.title = page.title;
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        QCOMPARE(QString::fromStdString(localized.document.pages[0].source_page.title),
                 QStringLiteral("1-Sample Equivalence Test"));

        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(static_cast<int>(tables.size()), 31);
        QCOMPARE(QString::fromStdString(tables[0].title),
                 QStringLiteral("Fit and forecast detail"));
        QCOMPARE(QString::fromStdString(tables[1].title), QStringLiteral("Forecast accuracy"));
        QCOMPARE(QString::fromStdString(tables[8].title),
                 QStringLiteral("Eigenvalues and explained variance"));
        QCOMPARE(QString::fromStdString(tables[14].title), QStringLiteral("Cluster summary"));
        QCOMPARE(QString::fromStdString(tables[17].title), QStringLiteral("Tree nodes"));
        QCOMPARE(QString::fromStdString(tables[20].title), QStringLiteral("Stepwise steps"));
        QCOMPARE(QString::fromStdString(tables[24].title), QStringLiteral("Sampling plan"));
        QCOMPARE(QString::fromStdString(tables[26].title), QStringLiteral("Decision limits"));
        QCOMPARE(QString::fromStdString(tables[28].rows[0][3]),
                 QStringLiteral("Empirical quantile (not a Minitab T² chart UCL)"));
        QCOMPARE(QString::fromStdString(tables[29].rows[0][0]),
                 QStringLiteral("(Intercept only)"));
        QCOMPARE(QString::fromStdString(tables[30].headers[0]), QStringLiteral("Model"));
        QCOMPARE(QString::fromStdString(tables[30].headers[3]),
                 QStringLiteral("Coefficient/drift"));

        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(static_cast<int>(plots.size()), 8);
        QCOMPARE(QString::fromStdString(plots[0].title),
                 QStringLiteral("Actual, fitted, and prediction interval"));
        QCOMPARE(QString::fromStdString(plots[0].series[0].label), QStringLiteral("Actual"));
        QCOMPARE(QString::fromStdString(plots[0].series[1].label), QStringLiteral("Forecast"));
        QCOMPARE(QString::fromStdString(plots[1].title),
                 QStringLiteral("ARIMA fit and forecast"));
        QCOMPARE(QString::fromStdString(plots[3].title), QStringLiteral("PCA score plot"));
        QCOMPARE(QString::fromStdString(plots[5].series[0].label), QStringLiteral("Power"));
        QCOMPARE(QString::fromStdString(plots[6].x_axis_title),
                 QStringLiteral("Proportion defective p"));
        QCOMPARE(QString::fromStdString(plots[6].y_axis_title),
                 QStringLiteral("Acceptance probability Pa(p)"));
        QCOMPARE(QString::fromStdString(plots[7].title), QStringLiteral("Length ANOM chart"));

        for (const auto& table : tables) {
            QVERIFY2(table.title.find("拟合") == std::string::npos, table.title.c_str());
            QVERIFY2(table.title.find("主成分") == std::string::npos, table.title.c_str());
            QVERIFY2(table.title.find("簇") == std::string::npos, table.title.c_str());
        }
    }

    void spc_parameter_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "SPC 参数 chrome 切片";
        page.parameter_summary =
            "子组数 = 20    子组大小 = 5    σ = 1.2（估计）    μ = 10（历史参数）";

        datalab::domain::StatisticTable ewma;
        ewma.title = "EWMA 参数";
        ewma.headers = {"指标", "数值"};
        ewma.rows = {
            {"规则策略", "用户指定"},
            {"启用测试", "Test 1"},
            {"参数来源", "估计"},
            {"控制限倍数", "3"},
            {"「单点超出 3σ 控制限」触发点数", "2"},
            {"适用性",
             "EWMA 只开放「单点超出 3σ 控制限」；其余特殊原因规则不附加到 EWMA。"},
        };

        datalab::domain::StatisticTable xbar;
        xbar.title = "Xbar-R 参数";
        xbar.headers = {"指标", "数值"};
        xbar.rows = {
            {"子组数", "12"},
            {"规则策略", "minitab_like（仅「单点超出 3σ 控制限」）"},
            {"Xbar「单点超出 3σ 控制限」触发点数", "1"},
            {"R「单点超出 3σ 控制限」触发点数", "0"},
            {"Xbar 启用测试", "Test 1"},
            {"R 适用规则",
             "单点超出 3σ 控制限、连续 9 点同侧、连续 6 点趋势、连续 14 点交替"},
        };

        datalab::domain::StatisticTable attr;
        attr.title = "P 图 方法与参数";
        attr.headers = {"指标", "数值"};
        attr.rows = {
            {"有效子组数", "8"},
            {"规则策略", "all_applicable（全部适用）"},
            {"启用规则", "Test 1"},
            {"判定口径",
             "特殊原因规则 beyond_control_limit…eight_beyond_1sigma；超过 kσ "
             "使用严格大于，窗口不跨阶段或缺失断点"},
        };

        datalab::domain::StatisticTable imrrs;
        imrrs.title = "I-MR-R/S 参数";
        imrrs.headers = {"指标", "数值"};
        imrrs.rows = {
            {"子组大小", "4"},
            {"I 图启用测试", "Test 1"},
            {"MR 图适用规则",
             "单点超出 3σ 控制限 / 连续 9 点同侧 / 连续 6 点趋势 / 连续 14 "
             "点交替（Minitab 不在 MR 图上启用后四条规则）"},
            {"参数来源", "历史参数"},
        };

        datalab::domain::StatisticTable laney;
        laney.title = "Laney P' 图 参数";
        laney.headers = {"指标", "数值"};
        laney.rows = {{"有效子组数", "5"}};

        page.tables = {ewma, xbar, attr, imrrs, laney};

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].source_page.parameter_summary = page.parameter_summary;
        document.pages[0].visible_tables = page.tables;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(static_cast<int>(tables.size()), 5);
        QCOMPARE(QString::fromStdString(tables[0].title), QStringLiteral("EWMA parameters"));
        QCOMPARE(QString::fromStdString(tables[0].rows[0][0]), QStringLiteral("Rule policy"));
        QCOMPARE(QString::fromStdString(tables[0].rows[0][1]), QStringLiteral("User-specified"));
        QCOMPARE(QString::fromStdString(tables[0].rows[2][1]), QStringLiteral("Estimate"));
        QVERIFY2(tables[0].rows[5][1].find("EWMA enables only") != std::string::npos,
                 tables[0].rows[5][1].c_str());

        QCOMPARE(QString::fromStdString(tables[1].rows[1][1]),
                 QStringLiteral("minitab_like (Rule 1 only)"));
        QCOMPARE(QString::fromStdString(tables[1].rows[2][0]),
                 QStringLiteral("Xbar points beyond 3σ limits"));
        QCOMPARE(QString::fromStdString(tables[1].rows[3][0]),
                 QStringLiteral("R points beyond 3σ limits"));

        QVERIFY2(tables[2].title.find("method and parameters") != std::string::npos,
                 tables[2].title.c_str());
        QVERIFY2(tables[2].title.find("方法") == std::string::npos, tables[2].title.c_str());
        QCOMPARE(QString::fromStdString(tables[2].rows[0][0]), QStringLiteral("Valid subgroups"));
        QCOMPARE(QString::fromStdString(tables[2].rows[1][1]),
                 QStringLiteral("all_applicable (all applicable)"));

        QCOMPARE(QString::fromStdString(tables[3].title),
                 QStringLiteral("I-MR-R/S parameters"));
        QCOMPARE(QString::fromStdString(tables[3].rows[2][0]),
                 QStringLiteral("MR chart applicable rules"));
        QVERIFY2(tables[3].rows[2][1].find("Minitab does not enable") != std::string::npos,
                 tables[3].rows[2][1].c_str());

        QVERIFY2(tables[4].title.find("parameters") != std::string::npos, tables[4].title.c_str());
        QVERIFY2(tables[4].title.find(" 参数") == std::string::npos, tables[4].title.c_str());

        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Subgroups = ") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Subgroup size = ") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find(" (estimated)") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find(" (historical)") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("子组数") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("（估计）") == std::string::npos, summary.c_str());
    }

    void nonparametric_attribute_agreement_titles_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "非参数与属性一致性标题切片";
        page.parameter_summary =
            "部件数 = 10    评估者数 = 3    Kappa权重 = fleiss";
        const char* table_titles[] = {
            "秩和检验",
            "符号秩检验",
            "位置估计（Walsh）",
            "符号摘要",
            "中位数置信区间",
            "比较准则",
            "Runs检验",
            "Fisher精确检验",
            "2×2 交叉表",
            "处理成功计数",
            "各处理阳性率",
            "各组 Above/Below",
            "Kruskal-Wallis 结果",
            "检验统计量",
            "处理摘要",
            "Nemenyi 成对比较",
            "评估者内一致性",
            "评估者间一致性",
            "评估者间总体 Kappa（Fleiss）",
            "评估者间 Kendall W",
            "评估者内 Kendall W",
            "评估者 vs 标准 Kendall τ",
            "全体 vs 标准 Kendall τ",
            "评估者×零件一致率",
            "评估者一致率",
            "Cochran Q 检验",
            "Mood 中位数检验",
            "Friedman 检验",
        };
        for (const char* title : table_titles) {
            datalab::domain::StatisticTable table;
            table.title = title;
            page.tables.push_back(table);
        }
        datalab::domain::StatisticTable mw_headers;
        mw_headers.title = "秩和检验";
        mw_headers.headers = {"第一组 N", "第二组 N", "秩和", "位置差异", "Ties 修正",
                              "连续性修正", "近似方法", "小样本警告", "效应量"};
        mw_headers.rows = {{"12", "10", "1.2", "0.3", "是", "否", "normal", "是", "0.4"}};
        page.tables.push_back(mw_headers);

        datalab::domain::PlotSpec positive_rate_plot;
        positive_rate_plot.title = "各处理阳性率";
        positive_rate_plot.x_axis_title = "处理";
        positive_rate_plot.y_axis_title = "阳性率";
        datalab::domain::PlotSpec evaluator_plot;
        evaluator_plot.title = "评估者一致率";
        evaluator_plot.x_axis_title = "评估者";
        evaluator_plot.y_axis_title = "Agreement %";
        page.plots = {positive_rate_plot, evaluator_plot};

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].source_page.parameter_summary = page.parameter_summary;
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(static_cast<int>(tables.size()), 29);
        QCOMPARE(QString::fromStdString(tables[0].title), QStringLiteral("Rank-sum test"));
        QCOMPARE(QString::fromStdString(tables[1].title), QStringLiteral("Signed-rank test"));
        QCOMPARE(QString::fromStdString(tables[7].title), QStringLiteral("Fisher exact test"));
        QCOMPARE(QString::fromStdString(tables[12].title),
                 QStringLiteral("Kruskal–Wallis results"));
        QCOMPARE(QString::fromStdString(tables[16].title),
                 QStringLiteral("Within-evaluator agreement"));
        QCOMPARE(QString::fromStdString(tables[18].title),
                 QStringLiteral("Overall between-evaluator Kappa (Fleiss)"));
        QCOMPARE(QString::fromStdString(tables[24].title),
                 QStringLiteral("Evaluator agreement rate"));
        QCOMPARE(QString::fromStdString(tables[25].title), QStringLiteral("Cochran Q test"));
        QCOMPARE(QString::fromStdString(tables[26].title), QStringLiteral("Mood median test"));
        QCOMPARE(QString::fromStdString(tables[27].title), QStringLiteral("Friedman test"));

        const auto& mw = tables[28];
        QCOMPARE(QString::fromStdString(mw.headers[0]), QStringLiteral("First group N"));
        QCOMPARE(QString::fromStdString(mw.headers[2]), QStringLiteral("Rank sum"));
        QCOMPARE(QString::fromStdString(mw.headers[4]), QStringLiteral("Ties correction"));
        QCOMPARE(QString::fromStdString(mw.headers[8]), QStringLiteral("Effect size"));
        QCOMPARE(QString::fromStdString(mw.rows[0][4]), QStringLiteral("Yes"));
        QCOMPARE(QString::fromStdString(mw.rows[0][5]), QStringLiteral("No"));
        QCOMPARE(QString::fromStdString(mw.rows[0][0]), QStringLiteral("12"));

        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(static_cast<int>(plots.size()), 2);
        QCOMPARE(QString::fromStdString(plots[0].title),
                 QStringLiteral("Positive rate by treatment"));
        QCOMPARE(QString::fromStdString(plots[0].x_axis_title), QStringLiteral("Treatment"));
        QCOMPARE(QString::fromStdString(plots[0].y_axis_title), QStringLiteral("Positive rate"));
        QCOMPARE(QString::fromStdString(plots[1].title),
                 QStringLiteral("Evaluator agreement rate"));
        QCOMPARE(QString::fromStdString(plots[1].x_axis_title), QStringLiteral("Evaluator"));

        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Evaluators = ") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Kappa weight = ") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Part count = ") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("评估者数") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Kappa权重") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("部件数") == std::string::npos, summary.c_str());

        for (std::size_t i = 0; i < 28; ++i) {
            QVERIFY2(tables[i].title.find("评估者") == std::string::npos,
                     tables[i].title.c_str());
            QVERIFY2(tables[i].title.find("一致性") == std::string::npos,
                     tables[i].title.c_str());
            QVERIFY2(tables[i].title.find("秩和") == std::string::npos, tables[i].title.c_str());
            QVERIFY2(tables[i].title.find("交叉表") == std::string::npos,
                     tables[i].title.c_str());
        }
    }

    void graph_faceted_chrome_and_eda_gate_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "区域图（分面）";
        datalab::domain::PlotSpec scatter_panel;
        scatter_panel.title = "散点图 · A";
        datalab::domain::PlotSpec interval_panel;
        interval_panel.title = "区间散点图 · B";
        datalab::domain::PlotSpec hex_panel;
        hex_panel.title = "二维分箱 · C";
        datalab::domain::PlotSpec area_panel;
        area_panel.title = "区域图 · D";
        page.plots = {scatter_panel, interval_panel, hex_panel, area_panel};

        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析错误，以下结论不可用于决策：散点图至少需要两个有效的完整观测行。",
            "分析限制：密度估计至少需要两个有限观测。",
        };
        page.interpretation = {limitations};
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "scatter_need",
             "散点图至少需要两个有效的完整观测行。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "interval_se",
             "区间图存在有效观测数少于 2 的分组，无法估计标准误。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "correlogram_need",
             "相关图至少需要选择两个连续变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "np_need",
             "正态概率图至少需要三个有效观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "matrix_need",
             "矩阵图至少需要两个连续变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "parallel_need",
             "平行坐标图至少需要两个连续变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "density_need",
             "密度估计至少需要两个有限观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "hexbin_need",
             "二维分箱至少需要两个 complete-case 点。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].source_page.title = page.title;
        document.pages[0].visible_plots = page.plots;
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        QCOMPARE(QString::fromStdString(localized.document.pages[0].source_page.title),
                 QStringLiteral("Faceted Area Plot"));
        QVERIFY2(localized.document.pages[0].source_page.title.find("Zone") == std::string::npos,
                 localized.document.pages[0].source_page.title.c_str());

        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(static_cast<int>(plots.size()), 4);
        QVERIFY2(plots[0].title.rfind(" · A") != std::string::npos, plots[0].title.c_str());
        QVERIFY2(plots[0].title.find("散点图") == std::string::npos, plots[0].title.c_str());
        QVERIFY2(plots[1].title.find("Interval") != std::string::npos, plots[1].title.c_str());
        QVERIFY2(plots[2].title.find("2D") != std::string::npos
                    || plots[2].title.find("bin") != std::string::npos,
                 plots[2].title.c_str());
        QVERIFY2(plots[3].title.find("Area") != std::string::npos, plots[3].title.c_str());
        QVERIFY2(plots[3].title.find("Zone") == std::string::npos, plots[3].title.c_str());

        const auto& bullets =
            localized.document.pages[0].visible_interpretation.front().bullets;
        QVERIFY2(bullets[0].find("complete-case") != std::string::npos, bullets[0].c_str());
        QVERIFY2(bullets[1].find("Density estimation") != std::string::npos,
                 bullets[1].c_str());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 8);
        QVERIFY2(diags[0].message.find("complete-case") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("SE cannot be estimated") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[6].message.find("Density estimation") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("2D binning") != std::string::npos,
                 diags[7].message.c_str());

        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("至少需要") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("无法估计") == std::string::npos, diag.message.c_str());
        }
    }

    void method_skip_diag_and_param_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "METHOD跳过与参数chrome";
        page.parameter_summary =
            "方法 = Pearson    偏相关 = 是    "
            "测量值 = Y    零件 = Part    操作员 = Op    "
            "Z 超限 = 2    窗宽 w = 3    控制限倍数 = 3    "
            "选择准则 = AICc    周期 = 12    操作者数 = 4    "
            "得分阈值 = 8    深度上限 = 5    叶数 = 6    选入 = 2    "
            "带宽 = ±0.12    分箱规则 = Sturges    "
            "须线 = Tukey 1.5×IQR    组数 = 3    "
            "设计来源 ID = ccd_k2    ；设计族 = ccd。";

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "相关分析 complete-case 跳过 3 个含缺失或非法单元格的行。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "双样本 t 检验跳过缺失或非法单元格；组 1 = 2，组 2 = 1。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "ANOVA 跳过 4 个缺失或非法响应值。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "Box-Cox 跳过 5 个缺失或非法观测。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "Mann–Whitney 跳过 6 个缺失或非法单元格（含 *）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "Wilcoxon 跳过 1 个缺失或非法单元格（含 *）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "符号检验跳过 2 个缺失或非法单元格（含 *）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "游程检验跳过两端缺失或非法单元格 N* = 7。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "Fisher 精确检验按 complete-case 跳过 8 个缺失单元格。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "McNemar 按 complete-case 跳过 9 个缺失或非法单元格（含 *）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "Cochran Q 按 complete-case 跳过 10 行缺失（含 *）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "Mood 跳过 11 个缺失或非法单元格（含 *）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "Kruskal–Wallis 跳过 12 个缺失或非法单元格（含 *）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "Friedman 按 complete-case 跳过 13 个缺失或非法单元格（含 *）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "测量或因子缺失、* 或非法单元格已跳过 14 行。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "Bias/Linearity 跳过 15 个缺失或不完整观测。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "Type 1 Gage 跳过 16 个缺失或非法观测。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "运行图跳过两端缺失或非法单元格 N* = 17。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "因果图跳过 18 个空类别或空原因单元格。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "ANOM 跳过 19 个缺失或非法响应值。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "Length 跳过 20 个缺失或非法单元格。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "missing_values",
             "Nested Gage R&R 跳过缺失或非法的测量/零件/操作者单元格。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Partial correlation = yes") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Part = Part") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Operator = Op") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Z out-of-control = 2") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Window w = 3") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Control-limit multiplier = 3") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Selection criterion = AICc") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Period = 12") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Operator count = 4") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Score threshold = 8") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Max depth = 5") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Leaf count = 6") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Selected = 2") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Bandwidth = ±0.12") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Binning rule = Sturges") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Whiskers = Tukey 1.5×IQR") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Design source ID = ccd_k2") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("; design family = ccd") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("偏相关") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("零件 =") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("操作员 =") == std::string::npos, summary.c_str());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY(diags.size() >= 22);
        QVERIFY2(diags[0].message.find("Correlation complete-case skipped 3")
                     != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("group 1 = 2, group 2 = 1") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("ANOVA skipped 4") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("Box-Cox skipped 5") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("Mann–Whitney skipped 6") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[7].message.find("Runs test skipped") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[7].message.find("N* = 7") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[10].message.find("Cochran Q complete-case skipped 10")
                     != std::string::npos,
                 diags[10].message.c_str());
        QVERIFY2(diags[12].message.find("Kruskal–Wallis skipped 12") != std::string::npos,
                 diags[12].message.c_str());
        QVERIFY2(diags[17].message.find("Run chart skipped") != std::string::npos,
                 diags[17].message.c_str());
        QVERIFY2(diags[18].message.find("Cause-and-effect skipped 18") != std::string::npos,
                 diags[18].message.c_str());
        QVERIFY2(diags[19].message.find("ANOM skipped 19") != std::string::npos,
                 diags[19].message.c_str());
        QVERIFY2(diags[20].message.find("Length skipped 20") != std::string::npos,
                 diags[20].message.c_str());
        QVERIFY2(diags[21].message.find("Nested Gage R&R skipped") != std::string::npos,
                 diags[21].message.c_str());
        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("跳过") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("个缺失") == std::string::npos, diag.message.c_str());
        }
    }

    void analysis_plot_title_suffix_and_residual_param_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "分析图题后缀与残渣参数";
        page.parameter_summary =
            "响应 = Y    处理 = Treat    区组 = Block    "
            "缺失值 N* = 1    缺失 = 3    预测期数 = 12    "
            "因子 A = A1    因子 B = B2    "
            "移动极差长度 = 2    正态概率图相关系数 = 0.98";

        datalab::domain::StatisticTable components;
        components.title = "方差分量";
        components.headers = {"来源", "截断", "N（可选）"};
        components.rows.push_back({"Part", "是", "*"});
        page.tables.push_back(components);

        datalab::domain::PlotSpec normal_plot;
        normal_plot.title = "Length 的正态概率图";
        normal_plot.x_axis_title = "标准正态分位数";
        page.plots.push_back(normal_plot);

        datalab::domain::PlotSpec hist_plot;
        hist_plot.title = "Length 的直方图";
        hist_plot.y_axis_title = "频数";
        page.plots.push_back(hist_plot);

        datalab::domain::PlotSpec indiv_plot;
        indiv_plot.title = "Length 的个体值图";
        indiv_plot.point_labels = {"嫌疑点", ""};
        datalab::domain::PlotSeries suspect;
        suspect.label = "嫌疑观测";
        indiv_plot.series.push_back(suspect);
        page.plots.push_back(indiv_plot);

        datalab::domain::PlotSpec scatter_plot;
        scatter_plot.title = "X 与 Y 的散点图";
        page.plots.push_back(scatter_plot);

        datalab::domain::PlotSpec run_plot;
        run_plot.title = "Length 运行图";
        page.plots.push_back(run_plot);

        datalab::domain::PlotSpec multi_plot;
        multi_plot.title = "Multi-Vari 图";
        multi_plot.x_axis_title = "因子 1";
        page.plots.push_back(multi_plot);

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error, "fisher_not_2x2",
             "Fisher 精确检验要求恰好 2×2 水平；当前行水平数 = 3，列水平数 = 4。"},
            {datalab::domain::DiagnosticMessage::Severity::info, "doe_fractional_resolution",
             "部分析因分辨度 IV（字长最短非 I 词）。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "need_units",
             "Laney U' 图需要单位数列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "need_units",
             "自定义图需要单位数列。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Response = Y") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Treatment = Treat") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Block = Block") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Missing N* = 1") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Missing = 3") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Forecast periods = 12") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Factor A = A1") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Factor B = B2") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Moving-range length = 2") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Normal probability plot correlation = 0.98")
                     != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("处理 =") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("区组 =") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("缺失值") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("缺失 =") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("预测期数") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("移动极差长度") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("正态概率图相关系数") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Missing Missing") == std::string::npos, summary.c_str());

        const auto& tables = localized.document.pages[0].visible_tables;
        QVERIFY(!tables.empty());
        QCOMPARE(QString::fromStdString(tables[0].headers[1]), QStringLiteral("Truncated"));
        QCOMPARE(QString::fromStdString(tables[0].headers[2]),
                 QStringLiteral("N (optional)"));

        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(static_cast<int>(plots.size()), 6);
        QCOMPARE(QString::fromStdString(plots[0].title),
                 QStringLiteral("Length Normal probability plot"));
        QCOMPARE(QString::fromStdString(plots[0].x_axis_title),
                 QStringLiteral("Standard normal quantile"));
        QCOMPARE(QString::fromStdString(plots[1].title),
                 QStringLiteral("Length Histogram"));
        QCOMPARE(QString::fromStdString(plots[1].y_axis_title), QStringLiteral("Frequency"));
        QCOMPARE(QString::fromStdString(plots[2].title),
                 QStringLiteral("Length Individual value plot"));
        QCOMPARE(QString::fromStdString(plots[2].point_labels[0]),
                 QStringLiteral("Suspect point"));
        QCOMPARE(QString::fromStdString(plots[2].series[0].label),
                 QStringLiteral("Suspect observation"));
        QCOMPARE(QString::fromStdString(plots[3].title),
                 QStringLiteral("X and Y Scatterplot"));
        QCOMPARE(QString::fromStdString(plots[4].title),
                 QStringLiteral("Length Run chart"));
        QCOMPARE(QString::fromStdString(plots[5].x_axis_title),
                 QStringLiteral("Factor 1"));
        QVERIFY2(plots[0].title.find("的") == std::string::npos, plots[0].title.c_str());
        QVERIFY2(plots[3].title.find("与") == std::string::npos, plots[3].title.c_str());
        QVERIFY2(plots[4].title.find("运行图") == std::string::npos, plots[4].title.c_str());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 4);
        QVERIFY2(diags[0].message.find("Fisher exact test requires exactly 2×2")
                     != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[0].message.find("row levels = 3") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[0].message.find("column levels = 4") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("Fractional factorial resolution IV")
                     != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("Laney U' chart requires a units column")
                     != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("requires a units column") != std::string::npos,
                 diags[3].message.c_str());
        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("部分析因") == std::string::npos,
                     diag.message.c_str());
            QVERIFY2(diag.message.find("行水平数") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("需要单位数列") == std::string::npos,
                     diag.message.c_str());
        }
    }

    void residual_param_summary_eq_tokens_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "残渣参数摘要eq";
        page.parameter_summary =
            "分类列 = Cat    N = 100    N* = 2    比例 = equal    "
            "测量值 = Y；总体中位数 M = 5.5    "
            "测量值 = M    零件 = P    操作员 = O    "
            "（Wheeler EMP；非全量 Expanded Gage）    "
            "测量值 = M2    零件 / 操作员 / Batch    "
            "（平衡三因子随机；非全量 GLM）    "
            "变量 = Y    周期 = 12    误差模型 = additive    "
            "趋势模型 = multiplicative    "
            "k = 3    N = 40    迭代 = 8    标准化    "
            "变量: Length    N = 20    组数 = 5    分箱规则 = 手工 8    "
            "变量: Length    须线 = Tukey 1.5×IQR    组数 = 2    "
            "Length    中位数 = 10.2    N = 30    "
            "模型 = 二项 OC    n = 50    c = 1    N = 200（信息）";

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Category column = Cat") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Proportion = equal") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("; overall median M = 5.5") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("(Wheeler EMP; not full Expanded Gage)") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Part / Operator / Batch") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("(balanced 3-factor random; not full GLM)")
                     != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Error model = additive") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Trend model = multiplicative") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Iterations = 8") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("standardized") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Group count = 5") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Binning rule = Manual 8") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Group count = 2") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Median = 10.2") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("N = 200 (info)") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("比例 =") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("组数 =") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("迭代 =") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("标准化") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("中位数 =") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("总体中位数") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Wheeler EMP；") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("零件 / 操作员") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("平衡三因子") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("误差模型") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("趋势模型") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("手工 ") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("（信息）") == std::string::npos, summary.c_str());
    }

    void spc_msa_assembly_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "SPC/MSA组装chrome";

        datalab::domain::StatisticTable attr_subgroup;
        attr_subgroup.title = "P 图逐子组统计";
        attr_subgroup.headers = {"原始行", "间隔", "CL", "LCL", "UCL", "触发规则", "主要规则"};
        attr_subgroup.rows.push_back({"1", "2.5", "0.05", "0.01", "0.10", "*", "*"});
        page.tables.push_back(attr_subgroup);

        datalab::domain::StatisticTable cusum_signals;
        cusum_signals.title = "CUSUM 信号";
        cusum_signals.headers = {"方向", "原始行", "观测序号", "累计值"};
        cusum_signals.rows = {
            {"上侧", "3", "3", "1.25"},
            {"下侧", "7", "7", "0.88"},
            {"无", "*", "*", "*"},
        };
        page.tables.push_back(cusum_signals);

        datalab::domain::PlotSpec part_xbar;
        part_xbar.title = "按零件 Xbar";
        part_xbar.x_axis_title = "子组均值";
        datalab::domain::PlotSpec part_r;
        part_r.title = "按零件 R";
        part_r.x_axis_title = "子组极差";
        datalab::domain::PlotSpec oper_xbar;
        oper_xbar.title = "按操作者 Xbar";
        datalab::domain::PlotSpec oper_r;
        oper_r.title = "按操作者 R";
        page.plots = {part_xbar, part_r, oper_xbar, oper_r};

        datalab::domain::InterpretationSection residual_normality;
        residual_normality.heading = "残差正态性";
        residual_normality.bullets = {
            "Anderson-Darling = 0.42，在 alpha 下未拒绝残差正态假设，不能据此宣称模型合格。",
            "Anderson-Darling = 1.08，请结合正态概率图和 P 值判断残差正态性，不能只看 R²。",
        };
        page.interpretation = {residual_normality};

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error, "stage_missing",
             "阶段列存在缺失标签，请补齐原始数据。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "insufficient_replicates",
             "重复次数小于 2，无法绘制按零件 R 图。"},
            {datalab::domain::DiagnosticMessage::Severity::warning, "insufficient_replicates",
             "重复次数小于 2，无法绘制按操作者 R 图。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        document.pages[0].visible_interpretation = page.interpretation;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(static_cast<int>(tables.size()), 2);
        QCOMPARE(QString::fromStdString(tables[0].title),
                 QStringLiteral("P chart subgroup statistics"));
        QCOMPARE(QString::fromStdString(tables[0].headers[1]), QStringLiteral("Interval"));
        QCOMPARE(QString::fromStdString(tables[1].headers[0]), QStringLiteral("Direction"));
        QCOMPARE(QString::fromStdString(tables[1].rows[0][0]), QStringLiteral("Upper"));
        QCOMPARE(QString::fromStdString(tables[1].rows[1][0]), QStringLiteral("Lower"));
        QCOMPARE(QString::fromStdString(tables[1].rows[2][0]), QStringLiteral("None"));

        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(static_cast<int>(plots.size()), 4);
        QCOMPARE(QString::fromStdString(plots[0].title), QStringLiteral("Xbar by part"));
        QCOMPARE(QString::fromStdString(plots[0].x_axis_title),
                 QStringLiteral("Subgroup mean"));
        QCOMPARE(QString::fromStdString(plots[1].title), QStringLiteral("R by part"));
        QCOMPARE(QString::fromStdString(plots[1].x_axis_title),
                 QStringLiteral("Subgroup range"));
        QCOMPARE(QString::fromStdString(plots[2].title), QStringLiteral("Xbar by operator"));
        QCOMPARE(QString::fromStdString(plots[3].title), QStringLiteral("R by operator"));

        const auto& interp = localized.document.pages[0].visible_interpretation;
        QVERIFY(!interp.empty());
        QCOMPARE(QString::fromStdString(interp[0].heading),
                 QStringLiteral("Residual normality"));
        QVERIFY2(interp[0].bullets[0].find("failure to reject residual normality")
                     != std::string::npos,
                 interp[0].bullets[0].c_str());
        QVERIFY2(interp[0].bullets[0].find("0.42") != std::string::npos,
                 interp[0].bullets[0].c_str());
        QVERIFY2(interp[0].bullets[1].find("normal probability plot and P value")
                     != std::string::npos,
                 interp[0].bullets[1].c_str());
        QVERIFY2(interp[0].bullets[1].find("1.08") != std::string::npos,
                 interp[0].bullets[1].c_str());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 3);
        QVERIFY2(diags[0].message.find("Stage column has missing labels")
                     != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("R chart by part") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("R chart by operator") != std::string::npos,
                 diags[2].message.c_str());

        for (const auto& table : tables) {
            for (const auto& header : table.headers) {
                QVERIFY2(header.find("间隔") == std::string::npos, header.c_str());
                QVERIFY2(header.find("方向") == std::string::npos, header.c_str());
            }
            for (const auto& row : table.rows) {
                for (const auto& cell : row) {
                    QVERIFY2(cell.find("上侧") == std::string::npos, cell.c_str());
                    QVERIFY2(cell.find("下侧") == std::string::npos, cell.c_str());
                    QVERIFY2(cell == "无" || cell.find("无") == std::string::npos, cell.c_str());
                }
            }
            QVERIFY2(table.title.find("逐子组统计") == std::string::npos,
                     table.title.c_str());
        }
        for (const auto& plot : plots) {
            QVERIFY2(plot.title.find("按零件") == std::string::npos, plot.title.c_str());
            QVERIFY2(plot.title.find("按操作者") == std::string::npos, plot.title.c_str());
        }
        for (const auto& bullet : interp[0].bullets) {
            QVERIFY2(bullet.find("残差正态") == std::string::npos, bullet.c_str());
            QVERIFY2(bullet.find("不能只看") == std::string::npos, bullet.c_str());
        }
        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("阶段列") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("重复次数") == std::string::npos, diag.message.c_str());
        }
    }

    void reliability_power_gt_point_stats_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "可靠性/功效/GT逐点";

        datalab::domain::StatisticTable gt_points;
        gt_points.title = "G 图 逐点统计";
        gt_points.headers = {"原始行", "间隔", "CL", "LCL", "UCL", "触发规则", "主要规则"};
        gt_points.rows.push_back({"1", "3", "5", "1", "9", "*", "*"});
        page.tables.push_back(gt_points);

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::info, "two_param_weibull",
             "当前为二参数 Weibull。含阈值的三参数模型请选择 model=weibull3。"},
            {datalab::domain::DiagnosticMessage::Severity::info, "one_param_exponential",
             "当前为一参数指数。含阈值的两参数模型请选择 model=exponential2。"},
            {datalab::domain::DiagnosticMessage::Severity::info, "two_param_lognormal",
             "当前为二参数对数正态。含阈值的三参数模型请选择 model=lognormal3。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "invalid_target_power",
             "目标功效必须位于 0 和 1 之间。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "sample_size_limit",
             "在允许的最大样本量内无法达到目标功效。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "doe_factorial_sample_size_limit",
             "在允许的最大重复数内无法达到目标功效。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "tolerance_sample_size_limit",
             "在允许的最大 n 内 Howe k 仍大于目标上限。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& tables = localized.document.pages[0].visible_tables;
        QVERIFY(!tables.empty());
        QCOMPARE(QString::fromStdString(tables[0].title),
                 QStringLiteral("G chart point statistics"));
        QVERIFY2(tables[0].title.find("逐点统计") == std::string::npos,
                 tables[0].title.c_str());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 7);
        QVERIFY2(diags[0].message.find("2-parameter Weibull") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[0].message.find("model=weibull3") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("1-parameter exponential") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[1].message.find("model=exponential2") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("2-parameter lognormal") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[2].message.find("model=lognormal3") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("Target power must be between 0 and 1")
                     != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("maximum sample size") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("maximum replicates") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("Howe k still exceeds") != std::string::npos,
                 diags[6].message.c_str());

        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("当前为") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("目标功效") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("在允许的最大") == std::string::npos, diag.message.c_str());
        }
    }

    void analysis_visible_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "分析可见chrome";

        datalab::domain::PlotSpec part_plot;
        part_plot.title = "按零件";
        part_plot.x_axis_title = "Temperature（按 Pressure 分组）";
        part_plot.y_axis_title = "测量值";
        datalab::domain::PlotSeries part_mean;
        part_mean.label = "零件均值";
        part_plot.series.push_back(part_mean);

        datalab::domain::PlotSpec cusum_upper;
        cusum_upper.title = "上侧 CUSUM";
        datalab::domain::PlotSpec cusum_lower;
        cusum_lower.title = "下侧 CUSUM";
        page.plots = {part_plot, cusum_upper, cusum_lower};

        datalab::domain::StatisticTable rules;
        rules.title = "特殊原因规则证据";
        rules.headers = {"主要规则", "触发规则"};
        rules.rows.push_back({"未知特殊原因规则", "Test 9"});
        page.tables.push_back(rules);

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error, "analysis",
             "时间序列没有有效数值观测。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "censoring_worksheet_export_ready",
             "已生成逐观测删失状态工作表（128 行；censoring_type=exact|right|left|interval）。"
             "可写回活动表以便审计或再导入；不是 vendor_oracle。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_plots = page.plots;
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(static_cast<int>(plots.size()), 3);
        QCOMPARE(QString::fromStdString(plots[0].series[0].label),
                 QStringLiteral("Part mean"));
        QCOMPARE(QString::fromStdString(plots[0].x_axis_title),
                 QStringLiteral("Temperature(grouped by Pressure)"));
        QCOMPARE(QString::fromStdString(plots[0].y_axis_title),
                 QStringLiteral("Measurement"));
        QCOMPARE(QString::fromStdString(plots[1].title), QStringLiteral("Upper CUSUM"));
        QCOMPARE(QString::fromStdString(plots[2].title), QStringLiteral("Lower CUSUM"));

        const auto& tables = localized.document.pages[0].visible_tables;
        QVERIFY(!tables.empty());
        QCOMPARE(QString::fromStdString(tables[0].rows[0][0]),
                 QStringLiteral("Unknown special-cause rule"));

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY(diags.size() >= 2);
        bool saw_time_series = false;
        bool saw_censoring = false;
        bool saw_honesty = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                saw_honesty = true;
                continue;
            }
            if (diag.message.find("Time series has no valid numeric observations")
                != std::string::npos) {
                saw_time_series = true;
            }
            if (diag.message.find("Per-observation censoring-status worksheet")
                != std::string::npos) {
                saw_censoring = true;
                QVERIFY2(diag.message.find("128 rows") != std::string::npos,
                         diag.message.c_str());
                QVERIFY2(diag.message.find("not vendor_oracle") != std::string::npos,
                         diag.message.c_str());
            }
        }
        QVERIFY(saw_time_series);
        QVERIFY(saw_censoring);
        QVERIFY(saw_honesty);

        for (const auto& plot : plots) {
            QVERIFY2(plot.title.find("上侧") == std::string::npos, plot.title.c_str());
            QVERIFY2(plot.title.find("下侧") == std::string::npos, plot.title.c_str());
            for (const auto& series : plot.series) {
                QVERIFY2(series.label.find("零件均值") == std::string::npos,
                         series.label.c_str());
            }
            QVERIFY2(plot.x_axis_title.find("按") == std::string::npos,
                     plot.x_axis_title.c_str());
            QVERIFY2(plot.x_axis_title.find("分组") == std::string::npos,
                     plot.x_axis_title.c_str());
        }
        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("时间序列") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("已生成") == std::string::npos, diag.message.c_str());
        }
    }

    void domain_spc_policy_and_rule_catalog_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "SPC策略与规则目录";

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::info,
             "special_cause_policy_minitab_like",
             "特殊原因策略=minitab_like：默认仅启用「单点超出 3σ 控制限」（与 Minitab 常见默认接近）。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "special_cause_policy_all_applicable",
             "特殊原因策略=all_applicable：启用该图种全部适用特殊原因规则；多规则提高灵敏度也提高误报风险，与 Minitab 默认仅「单点超出 3σ 控制限」不同。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "test_not_applicable",
             "已忽略不适用于此控制图的特殊原因规则：连续 9 点位于中心线同侧, 连续 6 点持续单调趋势。"},
        };

        datalab::domain::StatisticTable catalog;
        catalog.title = "规则证据";
        catalog.headers = {"规则", "名称", "状态", "证据", "关联行", "建议"};
        catalog.rows = {
            {"error_df", "误差自由度", "not_applicable",
             "N-p-1 必须为正才能输出 t、F 与 P。", "", ""},
            {"rank_deficiency", "秩亏/共线", "not_applicable",
             "设计矩阵秩亏时拒绝拟合，不输出伪造推断。", "", ""},
            {"residual_normality", "残差正态性", "not_applicable",
             "Anderson-Darling 只能拒绝或未拒绝正态假设。", "", ""},
            {"estimability", "可估计性", "not_applicable",
             "秩亏或无误差自由度时不输出伪造 F/P。", "", ""},
            {"family_error_rate", "家族错误率", "not_applicable",
             "Tukey 必须回显同时置信水平和调整后 p 值。", "", ""},
            {"design_balance", "设计平衡", "not_applicable",
             "零件×操作员单元重复次数必须一致。", "", ""},
            {"negative_variance", "负方差分量", "not_applicable",
             "保留截断前后方差分量，截断后用于 %Contribution。", "", ""},
            {"event_encoding", "事件编码", "not_applicable",
             "只接受明确的失效/删失语义，拒绝未知编码。", "", ""},
            {"convergence", "参数收敛", "not_applicable",
             "Weibull 必须回显收敛、边界命中和估计方法。", "", ""},
        };
        page.tables = {catalog};

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        bool saw_minitab = false;
        bool saw_all_applicable = false;
        bool saw_ignored = false;
        bool saw_honesty = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                saw_honesty = true;
                continue;
            }
            if (diag.code == "special_cause_policy_minitab_like") {
                saw_minitab = true;
                QVERIFY2(diag.message.find("minitab_like") != std::string::npos,
                         diag.message.c_str());
                QVERIFY2(diag.message.find("1 point beyond 3") != std::string::npos,
                         diag.message.c_str());
            }
            if (diag.code == "special_cause_policy_all_applicable") {
                saw_all_applicable = true;
                QVERIFY2(diag.message.find("all_applicable") != std::string::npos,
                         diag.message.c_str());
                QVERIFY2(diag.message.find("false-alarm") != std::string::npos,
                         diag.message.c_str());
            }
            if (diag.code == "test_not_applicable") {
                saw_ignored = true;
                QVERIFY2(diag.message.find("Ignored special-cause rules") != std::string::npos,
                         diag.message.c_str());
                QVERIFY2(diag.message.find("9 points in a row") != std::string::npos,
                         diag.message.c_str());
                QVERIFY2(diag.message.find("6 points in a row") != std::string::npos,
                         diag.message.c_str());
            }
            QVERIFY2(diag.message.find("特殊原因策略") == std::string::npos,
                     diag.message.c_str());
            QVERIFY2(diag.message.find("已忽略") == std::string::npos, diag.message.c_str());
        }
        QVERIFY(saw_minitab);
        QVERIFY(saw_all_applicable);
        QVERIFY(saw_ignored);
        QVERIFY(saw_honesty);

        const auto& table = localized.document.pages[0].visible_tables.front();
        QCOMPARE(QString::fromStdString(table.title), QStringLiteral("Rule evidence"));
        QCOMPARE(QString::fromStdString(table.rows[0][1]),
                 QStringLiteral("Error degrees of freedom"));
        QVERIFY2(table.rows[0][3].find("N-p-1 must be positive") != std::string::npos,
                 table.rows[0][3].c_str());
        QCOMPARE(QString::fromStdString(table.rows[1][1]),
                 QStringLiteral("Rank deficiency / collinearity"));
        QVERIFY2(table.rows[2][3].find("Anderson-Darling can only reject") != std::string::npos,
                 table.rows[2][3].c_str());
        QCOMPARE(QString::fromStdString(table.rows[3][1]), QStringLiteral("Estimability"));
        QCOMPARE(QString::fromStdString(table.rows[4][1]), QStringLiteral("Family error rate"));
        QVERIFY2(table.rows[5][3].find("part×operator cell") != std::string::npos,
                 table.rows[5][3].c_str());
        QVERIFY2(table.rows[6][3].find("%Contribution") != std::string::npos,
                 table.rows[6][3].c_str());
        QVERIFY2(table.rows[7][3].find("failure/censor semantics") != std::string::npos,
                 table.rows[7][3].c_str());
        QVERIFY2(table.rows[8][3].find("Weibull must echo convergence") != std::string::npos,
                 table.rows[8][3].c_str());

        for (const auto& row : table.rows) {
            for (const auto& cell : row) {
                QVERIFY2(cell.find("必须") == std::string::npos, cell.c_str());
                QVERIFY2(cell.find("秩亏") == std::string::npos, cell.c_str());
                QVERIFY2(cell.find("调查") == std::string::npos, cell.c_str());
            }
        }
    }

    void gt_zmr_ma_graph_visuals_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "G/T/Z-MR/MA与Graph诊断";

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "zmr_insufficient_n", "Z-MR 至少需要 2 个观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "zmr_non_finite", "Z-MR 不允许非有限观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "ma_insufficient_n", "移动平均图需要至少 w 个观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_g_points", "G 图至少需要两个有限间隔。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_g_interval", "G 图间隔必须为非负有限数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_g_probability", "无法从间隔均值估计几何分布参数 p。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_t_interval", "T 图间隔必须为非负有限数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_t_points", "T 图至少需要两个正间隔。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "t_chart_weibull_failed",
             "Weibull 参数无法识别，T 图控制限未计算。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "zero_interval_regression_used",
             "存在 0 间隔，已排除后用 log-log 回归估计 Weibull 参数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "t_chart_regression_failed", "0 间隔回归无法估计 Weibull 参数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "t_chart_regression_failed", "0 间隔回归得到非正形状参数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "no_valid_groups", "区间图没有可用于计算的有效分组。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "negative_bubble_size", "气泡大小不能为负数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "no_valid_bubbles", "气泡图没有可用于绘制的完整观测行。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_points", "经验累积分布图至少需要一个有效观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "no_valid_cells", "热图没有可用于聚合的完整观测。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "duplicate_time",
             "存在重复时间点；图形按时间排序后保留全部观测。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "irregular_interval",
             "时间间隔不规则；图形按时间排序，但不把间隔当作等距。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_points", "时间序列图至少需要两个有效观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "irregular_grid", "等值线图需要完整的规则 X/Y/Z 网格。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "negative_weight", "饼图权重不能为负数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "zero_total", "饼图各类别合计必须大于 0。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 24);
        QVERIFY2(diags[0].message.find("Z-MR requires at least 2") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("does not allow non-finite") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("Moving-average chart requires") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("G chart requires at least two finite") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[8].message.find("T chart requires at least two positive") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[9].message.find("Weibull parameters not identifiable") != std::string::npos,
                 diags[9].message.c_str());
        QVERIFY2(diags[10].message.find("log-log regression") != std::string::npos,
                 diags[10].message.c_str());
        QVERIFY2(diags[12].message.find("non-positive shape") != std::string::npos,
                 diags[12].message.c_str());
        QVERIFY2(diags[13].message.find("Interval plot has no valid groups") != std::string::npos,
                 diags[13].message.c_str());
        QVERIFY2(diags[15].message.find("complete observation rows") != std::string::npos,
                 diags[15].message.c_str());
        QVERIFY2(diags[17].message.find("Heatmap has no complete") != std::string::npos,
                 diags[17].message.c_str());
        QVERIFY2(diags[18].message.find("Duplicate time points") != std::string::npos,
                 diags[18].message.c_str());
        QVERIFY2(diags[19].message.find("Irregular time spacing") != std::string::npos,
                 diags[19].message.c_str());
        QVERIFY2(diags[21].message.find("complete regular X/Y/Z grid") != std::string::npos,
                 diags[21].message.c_str());
        QVERIFY2(diags[22].message.find("totals must be greater than 0") != std::string::npos,
                 diags[22].message.c_str());

        bool saw_honesty = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                saw_honesty = true;
                continue;
            }
            QVERIFY2(diag.message.find("至少需要") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("不允许") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("无法") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("存在") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("热图") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("饼图") == std::string::npos, diag.message.c_str());
        }
        QVERIFY(saw_honesty);
    }

    void quality_visuals_import_contract_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "质量图与导入契约";

        datalab::domain::StatisticTable cause_table;
        cause_table.title = "结构摘要";
        cause_table.headers = {"效果", "原因数"};
        cause_table.rows.push_back({"效果（未命名）", "0"});
        page.tables = {cause_table};

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "cause_effect_empty",
             "未找到有效的类别-原因行。请提供非空类别列与原因列。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "cause_effect_skipped_rows", "已跳过 2 个空类别或空原因单元格。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "variability_length_mismatch", "测量列与因子 A 长度不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "variability_length_mismatch", "测量列与因子 B 长度不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "variability_empty", "没有有效的测量值可用于变异性图。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "variability_singleton_cell", "单元「A | B」仅 1 个点，SD 记为 0。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "nelson_estimate_applied",
             "Nelson estimate 剔除了 3 个过大移动极差后重估 σ。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_contract", "导入结果的列类型数量与列名不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_contract", "导入结果的 RowId 数量与数据行不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_contract", "导入结果的单元格状态行数与数据行不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_contract", "导入元数据中的原始行数与数据行不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_contract", "导入元数据中的列数与列名不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_contract", "第 4 行的字段数与列数不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_contract", "第 7 行的单元格状态数与列数不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_contract", "导入结果包含重复的 RowId。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& table = localized.document.pages[0].visible_tables.front();
        QCOMPARE(QString::fromStdString(table.rows[0][0]), QStringLiteral("Unnamed effect"));

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 16);
        QVERIFY2(diags[0].message.find("No valid category-cause rows") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("Skipped 2 empty category") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("factor A") != std::string::npos, diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("factor B") != std::string::npos, diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("variability chart") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("Cell \"A | B\"") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("excluded 3 oversized moving range") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("column-type count") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("RowId count") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[9].message.find("cell-state row count") != std::string::npos,
                 diags[9].message.c_str());
        QVERIFY2(diags[10].message.find("original row count") != std::string::npos,
                 diags[10].message.c_str());
        QVERIFY2(diags[11].message.find("column count") != std::string::npos,
                 diags[11].message.c_str());
        QVERIFY2(diags[12].message.find("Row 4 field count") != std::string::npos,
                 diags[12].message.c_str());
        QVERIFY2(diags[13].message.find("Row 7 cell-state count") != std::string::npos,
                 diags[13].message.c_str());
        QVERIFY2(diags[14].message.find("duplicate RowId") != std::string::npos,
                 diags[14].message.c_str());

        bool saw_honesty = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                saw_honesty = true;
                continue;
            }
            QVERIFY2(diag.message.find("未找到") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("已跳过") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("测量列") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("导入") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("单元") == std::string::npos, diag.message.c_str());
        }
        QVERIFY(saw_honesty);
    }

    void adf_eda_facet_import_msa_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "ADF/EDA/分面/导入/MSA";

        datalab::domain::StatisticTable rules;
        rules.title = "规则证据";
        rules.headers = {"规则", "名称", "状态", "证据", "关联行", "建议"};
        rules.rows.push_back(
            {"stability_signals", "stability_signals", "triggered",
             "量具稳定性图存在超限点，需要调查特殊原因。", "",
             "超限只是调查提示，不能直接判定量具合格或不合格。"});
        page.tables = {rules};

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::info,
             "adf_default_lags",
             "默认滞后 p = floor((T-1)^(1/3)) = 4。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "adf_lags_too_large", "滞后过大，有效回归行不足。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "adf_critical_source",
             "临界值为大样本 MacKinnon 风格常数表；非 Minitab 导出 golden。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "kde_silverman_bandwidth",
             "默认带宽为 Silverman：h=0.9·min(s,IQR/1.34)·n^(-1/5)；高斯核。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "facet_levels_truncated",
             "分面水平数 = 12，受控 Graph Builder 最多显示 6 个面板；已截断 6 个水平（非自由拼版）。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "facet_controlled_panels",
             "使用受控分面面板（facet），不是自由像素拼版；by/分组仍是图内着色，与分面列不同。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_plan", "ImportPlan 缺少 provider_id。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_plan", "所选列不存在：LotID"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_plan", "不能同时使用 keyset_after 与 row_offset；优先 keyset。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_plan", "IS NULL 不得带绑定值。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "import_plan", "过滤运算符 >= 需要绑定值。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& table = localized.document.pages[0].visible_tables.front();
        QVERIFY2(table.rows[0][3].find("out-of-control stability-chart") != std::string::npos,
                 table.rows[0][3].c_str());
        QVERIFY2(table.rows[0][5].find("investigation hint only") != std::string::npos,
                 table.rows[0][5].c_str());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 12);
        QVERIFY2(diags[0].message.find("Default lag p = floor") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("Lag order is too large") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("MacKinnon-style constants") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("Silverman") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("Facet levels = 12") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[4].message.find("truncated 6 level") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("controlled facet panels") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("missing provider_id") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("LotID") != std::string::npos, diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("keyset takes priority") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[9].message.find("IS NULL must not include") != std::string::npos,
                 diags[9].message.c_str());
        QVERIFY2(diags[10].message.find("Filter operator >=") != std::string::npos,
                 diags[10].message.c_str());

        bool saw_honesty = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                saw_honesty = true;
                continue;
            }
            QVERIFY2(diag.message.find("默认") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("分面") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("ImportPlan") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("不得") == std::string::npos, diag.message.c_str());
        }
        QVERIFY(saw_honesty);
    }

    void forecast_normality_distid_doe_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "时间序列/正态性/分布识别/DOE";

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_time_series",
             "单指数平滑要求至少两个观测、alpha 位于 (0,1] 且预测期数大于 0。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_time_series",
             "双指数平滑要求至少三个观测，alpha/gamma 位于 (0,1] 且预测期数大于 0。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "insufficient_data", "正态性检验需要至少一个有效数值观测。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "insufficient_data",
             "正态性检验至少需要 2 个观测；当前仅返回概率图点。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "zero_variance", "常量样本无法计算正态性检验统计量。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "insufficient_data",
             "正态性检验至少需要 3 个有效观测；n<3 时不计算统计量与 p 值。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "ryan_joiner_not_computed",
             "Ryan–Joiner 统计量无法计算（方差或正态得分为零）。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "ryan_joiner_p_gt_0_10",
             "Ryan–Joiner：R 高于 α=0.10 临界，报告 p>0.10（存储 0.10）。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "ryan_joiner_p_lt_0_01",
             "Ryan–Joiner：R 不高于 α=0.01 临界，报告 p<0.01（存储 0.01）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "insufficient_data", "个体分布识别至少需要 3 个有效数值观测。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "non_positive_values",
             "存在非正值；Weibull、Lognormal 与 Exponential 仅对全部正值计算 AD。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "doe_factor_count_overflow", "因子数量过大，无法验证全因子运行数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_doe_run_shape",
             "每个 DOE 运行必须包含一个对应每个因子的编码。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_doe_center_point", "中心点的所有编码必须为零。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_doe_factor_level", "全因子运行的编码只能为 -1 或 +1。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "incomplete_doe_factorial_coverage",
             "设计未完整覆盖每个 2 水平因子组合，或存在重复运行。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "hold_out_of_range",
             "hold 实际值超出高低水平，已 clamp 到编码 [-1,1]。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "invalid_hold_levels", "因子高低水平相等，hold 回退编码 0。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "invalid_hold_value",
             "hold 实际值无法匹配高低水平，该因子回退编码 0。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "contour_factors_held_at_actual",
             "未作图的因子按指定实际单位 hold（已转换为编码）求值。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "contour_factors_held_at_zero", "未作图的因子在编码 0 处保持不变。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 22);
        QVERIFY2(diags[0].message.find("Single exponential smoothing") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("Double exponential smoothing") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("at least one valid numeric") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("probability-plot points") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("Constant sample") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("at least 3 valid observations") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("Ryan–Joiner statistic cannot") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("α=0.10 critical") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("α=0.01 critical") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[9].message.find("Individual distribution identification")
                     != std::string::npos,
                 diags[9].message.c_str());
        QVERIFY2(diags[10].message.find("Non-positive values") != std::string::npos,
                 diags[10].message.c_str());
        QVERIFY2(diags[11].message.find("Too many factors") != std::string::npos,
                 diags[11].message.c_str());
        QVERIFY2(diags[12].message.find("one coded level per factor") != std::string::npos,
                 diags[12].message.c_str());
        QVERIFY2(diags[13].message.find("center points must be zero") != std::string::npos,
                 diags[13].message.c_str());
        QVERIFY2(diags[14].message.find("-1 or +1") != std::string::npos,
                 diags[14].message.c_str());
        QVERIFY2(diags[15].message.find("two-level factor combination") != std::string::npos,
                 diags[15].message.c_str());
        QVERIFY2(diags[16].message.find("clamped to coded") != std::string::npos,
                 diags[16].message.c_str());
        QVERIFY2(diags[17].message.find("low and high levels are equal") != std::string::npos,
                 diags[17].message.c_str());
        QVERIFY2(diags[18].message.find("does not match low/high") != std::string::npos,
                 diags[18].message.c_str());
        QVERIFY2(diags[19].message.find("specified actual units") != std::string::npos,
                 diags[19].message.c_str());
        QVERIFY2(diags[20].message.find("held at coded 0") != std::string::npos,
                 diags[20].message.c_str());

        bool saw_honesty = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                saw_honesty = true;
                continue;
            }
            QVERIFY2(diag.message.find("要求") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("正态性") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("因子") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("编码") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("水平") == std::string::npos, diag.message.c_str());
        }
        QVERIFY(saw_honesty);
    }

    void doe_factorial_remaining_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "DOE 因子设计剩余诊断";

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "doe_factor_count_overflow", "基设计因子过多，无法生成运行数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "doe_factor_count_overflow", "因子数量过大，无法生成运行数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "doe_center_point_count_overflow",
             "中心点数量过大，无法生成 DOE 运行列表。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_doe_run_count", "DOE 运行数与设计内容不一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_doe_block", "DOE 区组编号必须从 1 开始。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_doe_response_shape", "响应值数量必须与 DOE 运行数一致。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_doe_effect_data",
             "计算 DOE 效应时正负对比组必须都有有效响应。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "missing_doe_responses", "缺失或非有限响应未参与 DOE 效应摘要。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "doe_factor_count_overflow",
             "因子数量过大，无法诊断完整 2 水平组合。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "missing_doe_runs", "设计缺少一个或多个 2 水平因子组合。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "duplicate_doe_runs", "设计包含重复的因子组合。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "block_terms_included", "区组项已作为模型中的分类项纳入拟合。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "replicated_doe_runs", "检测到重复因子组合，已用于纯误差估计。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "lenth_pse_unreplicated",
             "误差自由度为 0，标准化效应 Pareto 使用 Lenth PSE 参考线。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "contour_requires_two_factors",
             "等值线/曲面图需要至少两个连续因子。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "invalid_contour_factors", "等值线因子索引无效。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "missing_factorial_coefficients", "没有可用于求值的析因系数。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "factorial_contour_no_quadratic",
             "二水平模型无平方项，等值线为双线性面，不能表示曲率。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "hold_ignored_axis_factor", "轴因子的 hold 条目已忽略。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "unknown_hold_factor", "hold 中出现未知因子名，已忽略。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 21);
        QVERIFY2(diags[0].message.find("Too many base-design factors") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("Too many factors to generate run count")
                     != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("Center-point count too large") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("run count does not match") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("block numbers must start at 1") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("Response count must match") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("Both positive and negative contrast") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("Missing or non-finite responses") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("diagnose complete two-level") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[9].message.find("missing one or more two-level") != std::string::npos,
                 diags[9].message.c_str());
        QVERIFY2(diags[10].message.find("duplicate factor combinations") != std::string::npos,
                 diags[10].message.c_str());
        QVERIFY2(diags[11].message.find("Block terms included") != std::string::npos,
                 diags[11].message.c_str());
        QVERIFY2(diags[12].message.find("Duplicate factor combinations detected")
                     != std::string::npos,
                 diags[12].message.c_str());
        QVERIFY2(diags[13].message.find("Lenth PSE reference line") != std::string::npos,
                 diags[13].message.c_str());
        QVERIFY2(diags[14].message.find("Contour/surface plots require") != std::string::npos,
                 diags[14].message.c_str());
        QVERIFY2(diags[15].message.find("Contour factor index is invalid") != std::string::npos,
                 diags[15].message.c_str());
        QVERIFY2(diags[16].message.find("No factorial coefficients") != std::string::npos,
                 diags[16].message.c_str());
        QVERIFY2(diags[17].message.find("bilinear surface") != std::string::npos,
                 diags[17].message.c_str());
        QVERIFY2(diags[18].message.find("Hold entry for axis factor") != std::string::npos,
                 diags[18].message.c_str());
        QVERIFY2(diags[19].message.find("Unknown factor name in hold") != std::string::npos,
                 diags[19].message.c_str());

        bool saw_honesty = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                saw_honesty = true;
                continue;
            }
            QVERIFY2(diag.message.find("因子") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("设计") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("运行") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("等值线") == std::string::npos, diag.message.c_str());
        }
        QVERIFY(saw_honesty);
    }

    void cif_km_interval_gaussian_mixture_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "CIF/KM区间/高斯混合";

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "cif_unlabeled_exact_excluded",
             "有 3 条 exact 失效缺少 failure_mode，已从 CIF 排除。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "cif_left_interval_omitted",
             "CIF 省略 left/interval 行 2 条；请用 km_interval 路径处理。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "cif_aalen_johansen_scope",
             "累计发生函数 CIF = Aalen–Johansen（formula_reference / aalen_johansen_cif）："
             "总体生存把任一标注失效当作事件；CIF_k 为原因 k 的累计发生概率。"
             "不是 Fine-Gray 多协变量回归，不是 cause-specific（竞争删失）可靠度，不是 vendor_oracle。"
             "二分类 group 的 Fine-Gray 另有门禁路径。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "cif_not_fine_gray_multivar",
             "Aalen–Johansen CIF 本身不是 Fine-Gray 回归；不得把 CIF 表写成 Fine-Gray 或商业软件对齐。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "km_interval_n", "区间删失 KM 至少需要约 3 条观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "km_interval_order", "存在 R < L 的无效区间。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "km_interval_clean", "有效区间过少。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "km_interval_grid", "无法构造 Turnbull 时间网格。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "km_interval_mass", "无候选质量点。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "km_interval_collapse", "Turnbull 质量塌缩。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "km_interval_max_iter", "Turnbull 已达最大迭代。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "km_interval_scope",
             "Turnbull NPMLE（简化网格）；非右删失 product-limit；非 Minitab golden。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "km_interval_evidence",
             "证据类型 = formula_reference（algorithm_id=turnbull_npmle_simplified_grid）；"
             "不得写成 vendor_oracle / golden / 商业软件对齐。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "mixture2_scope",
             "二维高斯混合 = formula_reference / gaussian_mixture_2_em（EM + BIC vs 单正态）；"
             "多 k 搜索见 gaussian_mixture_k_bic；不是 vendor_oracle，不得写成过程合格判定。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "mixture_k_skipped_sparse",
             "跳过 k=3（n < 15k）；不得把跳过写成已排除该 k。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "mixture_k_em_failed",
             "k=2 高斯混合 EM 失败（singular）；该 k 不参与 BIC 优选。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "mixture_k_not_converged",
             "k=4 高斯混合 EM 未收敛；该 k 不参与 BIC 优选。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "mixture_k_scope",
             "多 k 高斯混合 = formula_reference / gaussian_mixture_k_bic（EM + BIC，k=1..5；"
             "选定 k=2）；不是非高斯混合，不是 vendor_oracle，不得写成过程合格判定。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 19);
        QVERIFY2(diags[0].message.find("3 exact failure") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("omitted 2 left/interval") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("Aalen–Johansen") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("not Fine-Gray regression") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("about 3 or more observations") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("R < L") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("Too few valid intervals") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("Turnbull time grid") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("candidate mass points") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[9].message.find("mass collapsed") != std::string::npos,
                 diags[9].message.c_str());
        QVERIFY2(diags[10].message.find("maximum iterations") != std::string::npos,
                 diags[10].message.c_str());
        QVERIFY2(diags[11].message.find("Turnbull NPMLE") != std::string::npos,
                 diags[11].message.c_str());
        QVERIFY2(diags[12].message.find("turnbull_npmle_simplified_grid") != std::string::npos,
                 diags[12].message.c_str());
        QVERIFY2(diags[13].message.find("Two-component Gaussian mixture") != std::string::npos,
                 diags[13].message.c_str());
        QVERIFY2(diags[14].message.find("Skipped k=3") != std::string::npos,
                 diags[14].message.c_str());
        QVERIFY2(diags[15].message.find("k=2 Gaussian mixture EM failed") != std::string::npos,
                 diags[15].message.c_str());
        QVERIFY2(diags[16].message.find("k=4 Gaussian mixture EM did not converge")
                     != std::string::npos,
                 diags[16].message.c_str());
        QVERIFY2(diags[17].message.find("selected k=2") != std::string::npos,
                 diags[17].message.c_str());

        bool saw_honesty = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                saw_honesty = true;
                continue;
            }
            QVERIFY2(diag.message.find("失效") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("区间") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("高斯") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("不得") == std::string::npos, diag.message.c_str());
        }
        QVERIFY(saw_honesty);
    }

    void capability_gate_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "能力门禁诊断";

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_stability_screen_signals",
             "I-MR Rule-1 初筛检出 2 个超限点；能力指数仅供调查，不得写成过程合格。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_bimodality_suspected",
             "直方图双峰初筛检出约 3 个可分离峰；单一分布能力指数仅供调查，"
             "不得写成过程合格（直方图初筛 ≠ Hartigan；≠ 混合模型证明）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_hartigan_dip_evidence_against",
             "Hartigan dip 门禁筛查（formula_reference / hartigan_dip_1985）：dip=0.042，"
             "Uniform 零假设 MC p≈0.031（reps=199）；提示偏离单峰，单一分布能力指数仅供调查，"
             "不得写成过程合格（非 vendor_oracle；二维高斯混合另有门禁）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_mixture_preferred_kcomp",
             "高斯混合门禁（formula_reference / gaussian_mixture_k_bic）：BIC 更支持 k=3"
             "（ΔBIC≈12.5；k_max=4）；单一分布能力指数仅供调查，"
             "不得写成过程合格（非 vendor_oracle）。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "between_variance_truncated",
             "估计的组间方差为负，已截断为 0；σ_B 可能低估。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "ci_df_used_sample_n",
             "组间/组内能力区间自由度使用 N−1，不是 Minitab Rbar/Sbar 调整 ν。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "johnson_spec_outside_support",
             "至少一个规格限落在变换定义域外；越界侧的百分位回推未实现，"
             "仅输出仍可变换规格的 overall 指数。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "capability_hartigan_dip_consistent_not_proof",
             "Hartigan dip 门禁筛查未拒绝 Uniform 零假设下的单峰（formula_reference）；"
             "这不是过程单峰证明，也不得开放合格判定。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 9);
        QVERIFY2(diags[0].message.find("2 out-of-control point") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("about 3 separable peak") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("dip=0.042") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[2].message.find("Uniform-null MC p≈0.031") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[2].message.find("reps=199") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("BIC favors k=3") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[3].message.find("ΔBIC≈12.5") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("truncated to 0") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("uses N−1") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("outside transform support") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("did not reject unimodality") != std::string::npos,
                 diags[7].message.c_str());

        bool saw_honesty = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                saw_honesty = true;
                continue;
            }
            QVERIFY2(diag.message.find("不得") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("门禁") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("双峰") == std::string::npos, diag.message.c_str());
        }
        QVERIFY(saw_honesty);
    }

    void censoring_warranty_mode_fit_dynamic_diag_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "删失保修分模式动态";

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "censoring_left_interval_not_for_classic_km",
             "经典 Kaplan–Meier 路径不接受左删失/区间删失；请改用区间删失 KM，"
             "或先过滤为 exact/right。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "censoring_worksheet_import_ok",
             "已从工作表导入 42 条删失观测；证据类型仍为 formula_reference，不是 vendor_oracle。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "censoring_worksheet_invalid_type",
             "第 5 行 censoring_type 无法解析。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "mode_fit_unlabeled_exact_excluded",
             "有 2 条 exact 失效缺少 failure_mode，已从分模式拟合排除"
             "（不作任一模式的竞争删失）。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "mode_fit_left_interval_omitted",
             "分模式拟合省略 left/interval 行 3 条；请用 km_interval / 总体路径处理。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "mode_fit_model_unsupported",
             "分模式拟合不支持模型 'weibull3'。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "mode_fit_no_failures",
             "模式 Wear 无 exact 失效，无法拟合。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "mode_fit_reliability_unavailable",
             "模式 Corrosion 已拟合但未能在 T_w 计算 R。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "warranty_strata_empty",
             "未提供分层输入；仅输出总体保修摘要。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "warranty_strata_pooled_reliability",
             "分层 expected_failures = 层暴露量 * F(T_w)，F 来自总体池化 R(T_w)；"
             "未使用分模式可靠度，不是 vendor_oracle。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "warranty_stratum_invalid_mode_reliability",
             "分层 East 提供的分模式 R(T_w) 非法，已回退到池化 R。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 12);
        QVERIFY2(diags[0].message.find("left/interval censoring") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("Imported 42 censoring observation") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("Row 5 censoring_type") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("2 exact failure") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("omitted 3 left/interval") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("weibull3") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("Mode Wear") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("Mode Corrosion") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[7].message.find("T_w") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("overall warranty summary only") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[9].message.find("pooled R(T_w)") != std::string::npos,
                 diags[9].message.c_str());
        QVERIFY2(diags[10].message.find("Stratum East") != std::string::npos,
                 diags[10].message.c_str());
        QVERIFY2(diags[10].message.find("fell back to pooled R") != std::string::npos,
                 diags[10].message.c_str());

        bool saw_honesty = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                saw_honesty = true;
                continue;
            }
            QVERIFY2(diag.message.find("删失") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("分层") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("不得") == std::string::npos, diag.message.c_str());
        }
        QVERIFY(saw_honesty);
    }

    void tukey_computed_rule_evidence_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "Tukey计算规则证据";

        datalab::domain::StatisticTable rules;
        rules.title = "规则证据";
        rules.headers = {"规则", "名称", "状态", "证据", "关联行", "建议"};
        rules.rows.push_back(
            {"family_error_rate", "家族错误率", "not_triggered",
             "Tukey 同时置信水平 = 0.95；显著性由同时置信区间是否包含 0 决定。",
             "",
             "不要把逐比较 alpha 当成家族错误率。"});
        page.tables = {rules};

        datalab::domain::InterpretationSection limitations;
        limitations.heading = "限制与数据质量";
        limitations.bullets = {
            "分析限制：Tukey 同时置信水平 = 0.90；显著性由同时置信区间是否包含 0 决定。"};
        page.interpretation.push_back(limitations);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_interpretation = page.interpretation;
        const auto localized = localize_report_document(document);

        const auto& cell =
            localized.document.pages[0].visible_tables.front().rows[0][3];
        QVERIFY2(cell.find("0.95") != std::string::npos, cell.c_str());
        QVERIFY2(cell.find("simultaneous confidence") != std::string::npos,
                 cell.c_str());
        QVERIFY2(cell.find("同时置信") == std::string::npos, cell.c_str());

        const auto& lim =
            localized.document.pages[0].visible_interpretation.front().bullets.front();
        QVERIFY2(lim.find("0.90") != std::string::npos, lim.c_str());
        QVERIFY2(lim.find("simultaneous confidence") != std::string::npos,
                 lim.c_str());
        QVERIFY2(lim.find("Analysis limitation") != std::string::npos, lim.c_str());
        QVERIFY2(lim.find("同时置信") == std::string::npos, lim.c_str());
    }

    void application_error_page_fallback_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "应用层错误页回退";

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "imr_rs_cannot_compute",
             "无法计算 I-MR-R/S 控制图。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "between_within_cannot_compute",
             "无法计算组间/组内能力指标。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 3);
        QVERIFY2(diags[0].message.find("Cannot compute I-MR-R/S control chart")
                      != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("Cannot compute between/within capability")
                      != std::string::npos,
                 diags[1].message.c_str());

        bool saw_honesty = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                saw_honesty = true;
                continue;
            }
            QVERIFY2(diag.message.find("无法计算") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("组间/组内") == std::string::npos, diag.message.c_str());
        }
        QVERIFY(saw_honesty);
    }

    void graph_builder_faceted_page_titles_localize_to_en_us()
    {
        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        const auto localize_title = [&](const std::string& zh) {
            OutputPage page = sample_page();
            page.title = zh;
            auto document = build_report_document(sample_table(), {page}, profile);
            document.pages[0].source_page.title = page.title;
            return localize_report_document(document).document.pages[0].source_page.title;
        };

        struct TitleCase {
            const char* zh;
            const char* en;
        };
        static const TitleCase cases[] = {
            {"散点图（分面）", "Scatterplot (faceted)"},
            {"区间散点图（分面）", "Interval scatterplot (faceted)"},
            {"相关图（分面）", "Correlogram (faceted)"},
            {"气泡图（分面）", "Bubble chart (faceted)"},
            {"正态概率图（分面）", "Normal probability plot (faceted)"},
            {"经验累积分布图（分面）", "Empirical CDF (faceted)"},
            {"矩阵图（分面）", "Matrix plot (faceted)"},
            {"边际图（分面）", "Marginal plot (faceted)"},
            {"平行坐标图（分面）", "Parallel coordinates (faceted)"},
            {"热图（分面）", "Heatmap (faceted)"},
            {"时间序列图（分面）", "Time series plot (faceted)"},
            {"等值线图（分面）", "Contour plot (faceted)"},
            {"密度图（分面）", "Density plot (faceted)"},
            {"Hexbin（分面）", "Hexbin (faceted)"},
            {"小提琴图（分面）", "Violin plot (faceted)"},
            {"条形图（分面）", "Bar chart (faceted)"},
        };

        for (const auto& item : cases) {
            const std::string localized = localize_title(item.zh);
            QCOMPARE(QString::fromStdString(localized), QString::fromUtf8(item.en));
            QVERIFY2(localized.find("（分面）") == std::string::npos, localized.c_str());
            QVERIFY2(localized.find("(faceted)") != std::string::npos, localized.c_str());
        }
    }

    void param_true_audit_leftovers_localize_to_en_us()
    {
        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";

        OutputPage page = sample_page();
        page.title = "param_true 审计残渣";
        page.parameter_summary =
            "响应 = Y    处理 = T    区组 = B    测量 = Length    因子 = A, B    "
            "有效观测 = 100    缺失 = 2";
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "missing_values",
             "跳过 11 个缺失或非法单元格（计入 N*）。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "fisher_not_2x2",
             "Fisher 精确检验要求恰好 2×2 水平；当前行水平数 = 3，列水平数 = 4。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "missing_values",
             "双样本 t 检验跳过缺失或非法单元格；组 1 = 2，组 2 = 1。"},
        };
        datalab::domain::StatisticTable rules;
        rules.title = "特殊原因规则证据";
        rules.headers = {"规则", "名称", "状态", "证据", "关联行", "建议"};
        rules.rows.push_back(
            {"beyond_control_limit",
             "单点超出 3σ 控制限",
             "未触发",
             "任一点低于 LCL 或高于 UCL；有限控制限使用严格越界比较。"
             "表示该点与当前控制模型不一致，不等于根因已确认。 当前未触发。",
             "",
             "复核测量、批次、设备或取样条件，并关联原始观测行。"});
        page.tables = {rules};

        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].source_page.parameter_summary = page.parameter_summary;
        document.pages[0].visible_diagnostics = page.diagnostics;
        document.pages[0].visible_tables = page.tables;
        const auto localized = localize_report_document(document);

        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Block = B") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Missing = 2") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("区组") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("缺失 =") == std::string::npos, summary.c_str());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 4);
        bool saw_skip_nstar = false;
        bool saw_fisher = false;
        bool saw_two_sample = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                continue;
            }
            if (diag.message.find("Skipped 11") != std::string::npos
                && diag.message.find("counted in N*") != std::string::npos) {
                saw_skip_nstar = true;
            }
            if (diag.message.find("Fisher exact test requires exactly 2×2 levels")
                    != std::string::npos
                && diag.message.find("row levels = 3") != std::string::npos
                && diag.message.find("column levels = 4") != std::string::npos) {
                saw_fisher = true;
            }
            if (diag.message.find("Two-sample t skipped") != std::string::npos
                && diag.message.find("group 1 = 2") != std::string::npos
                && diag.message.find("group 2 = 1") != std::string::npos) {
                saw_two_sample = true;
            }
            QVERIFY2(diag.message.find("跳过") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("列水平数") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("组 2") == std::string::npos, diag.message.c_str());
        }
        QVERIFY(saw_skip_nstar);
        QVERIFY(saw_fisher);
        QVERIFY(saw_two_sample);

        const auto& evidence =
            localized.document.pages[0].visible_tables.front().rows[0][3];
        QVERIFY2(evidence.find("Currently not triggered") != std::string::npos,
                 evidence.c_str());
        QVERIFY2(evidence.find("LCL") != std::string::npos, evidence.c_str());
        QVERIFY2(evidence.find("当前未触发") == std::string::npos, evidence.c_str());
        QVERIFY2(evidence.find("任一点") == std::string::npos, evidence.c_str());
    }

    void proportion_group_error_prefix_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "比例组错误前缀";
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "analysis",
             "第一组：没有可用于比例检验的 complete-case 行。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "analysis",
             "第二组：事件数和试验数必须为非负整数，且事件数不能超过试验数。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QCOMPARE(static_cast<int>(diags.size()), 3);
        bool saw_group1 = false;
        bool saw_group2 = false;
        for (const auto& diag : diags) {
            if (diag.code == "report_body_partial_bilingual") {
                continue;
            }
            if (diag.message.find("Group 1:") != std::string::npos
                && diag.message.find("No complete-case rows") != std::string::npos) {
                saw_group1 = true;
            }
            if (diag.message.find("Group 2:") != std::string::npos
                && diag.message.find("Events and trials must be") != std::string::npos) {
                saw_group2 = true;
            }
            QVERIFY2(diag.message.find("第一组") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("第二组") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("比例检验") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("事件数和试验数") == std::string::npos, diag.message.c_str());
        }
        QVERIFY(saw_group1);
        QVERIFY(saw_group2);
    }

    void dual_line_remaining_qing_gate_diags_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "双线剩余请选择门禁";
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择测量值、零件和操作员列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择测量值、零件、操作员与附加因子列；无附加因子时请用交叉 Gage R&R。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择测量值、部件和操作者列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择响应、处理与区组列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请至少选择两个数值变量列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择时间序列数值列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择一个数值序列列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择两个数值序列列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择二元响应列和至少一个预测变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择时间序列值列并输入正整数季节周期。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择响应列与至少两个连续因子列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择至少一个响应列和已导入因子列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择寿命列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择失效/删失指示列，或逐行删失类型列（exact/right/left/interval）。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择计数响应列与至少一个数值预测列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择响应列与至少一个数值预测列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择响应列与至少两个候选预测列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择一个数值列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择有序响应列与至少一个数值预测列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择类别响应与至少一个数值预测列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请提供至少一个因子名称。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请输入 LSL 或 USL。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择一列数值观测（子组大小=1）。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请指定样本量 n ≥ 1。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择一列非负整数计数。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY(diags.size() >= 25);
        QVERIFY2(diags[0].message.find("measurement, part, and operator")
                     != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[0].message.find("additional-factor") == std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("additional-factor") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[1].message.find("crossed Gage R&R") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("measurement, part, and operator")
                     != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("response, treatment, and block")
                     != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("at least two numeric variable")
                     != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("time-series numeric column") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("one numeric series") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[6].message.find("two numeric") == std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("two numeric series") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("binary response") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[9].message.find("seasonal period") != std::string::npos,
                 diags[9].message.c_str());
        QVERIFY2(diags[10].message.find("at least two continuous factor")
                     != std::string::npos,
                 diags[10].message.c_str());
        QVERIFY2(diags[11].message.find("imported factor") != std::string::npos,
                 diags[11].message.c_str());
        QVERIFY2(diags[12].message.find("lifetime column") != std::string::npos,
                 diags[12].message.c_str());
        QVERIFY2(diags[13].message.find("exact/right/left/interval") != std::string::npos,
                 diags[13].message.c_str());
        QVERIFY2(diags[14].message.find("count-response") != std::string::npos,
                 diags[14].message.c_str());
        QVERIFY2(diags[15].message.find(
                     "Select a response column and at least one numeric predictor")
                     != std::string::npos,
                 diags[15].message.c_str());
        QVERIFY2(diags[15].message.find("count-response") == std::string::npos,
                 diags[15].message.c_str());
        QVERIFY2(diags[15].message.find("ordinal") == std::string::npos,
                 diags[15].message.c_str());
        QVERIFY2(diags[15].message.find("class-response") == std::string::npos,
                 diags[15].message.c_str());
        QVERIFY2(diags[15].message.find("two candidate") == std::string::npos,
                 diags[15].message.c_str());
        QVERIFY2(diags[16].message.find("at least two candidate predictor")
                     != std::string::npos,
                 diags[16].message.c_str());
        QVERIFY2(diags[17].message.find("one numeric column") != std::string::npos,
                 diags[17].message.c_str());
        QVERIFY2(diags[18].message.find("ordinal response") != std::string::npos,
                 diags[18].message.c_str());
        QVERIFY2(diags[19].message.find("class-response") != std::string::npos,
                 diags[19].message.c_str());
        QVERIFY2(diags[20].message.find("factor name") != std::string::npos,
                 diags[20].message.c_str());
        QVERIFY2(diags[21].message.find("LSL or USL") != std::string::npos,
                 diags[21].message.c_str());
        QVERIFY2(diags[22].message.find("subgroup size = 1") != std::string::npos,
                 diags[22].message.c_str());
        QVERIFY2(diags[23].message.find("sample size n ≥ 1") != std::string::npos,
                 diags[23].message.c_str());
        QVERIFY2(diags[24].message.find("non-negative integer count") != std::string::npos,
                 diags[24].message.c_str());
        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("请选择") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("请指定") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("请输入") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("请提供") == std::string::npos, diag.message.c_str());
        }
    }

    void classic_and_nonparametric_gate_diags_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "经典推断门禁切片";
        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择至少两列数值变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请指定假设均值。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请指定已知总体标准差 σ（必须为正）。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择两列独立样本变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择正好两列独立样本。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择两列独立样本。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择检验列与参考列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择两列配对测量值。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择两组事件数和试验数列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择事件数列和试验数列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择一个响应变量和至少一个预测变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请指定假设比例。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择缺陷计数列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择观测长度列或输入观测长度常数。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请指定假设发生率。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择两组缺陷计数和观测长度列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择行分类列和列分类列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择分类列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择第一样本列或测量列。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "所选列没有足够的数值观测。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "没有可用于比例检验的 complete-case 行。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "事件数和试验数必须为非负整数，且事件数不能超过试验数。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择一列（相对 η0）或两列配对样本。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择一列（单样本）或两列（配对）。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择一列数值序列（按行序）。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择正好两列分类变量以构建 2×2 表。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择正好两列配对二元结果。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择至少两列配对二元结果（k≥3 才计算；k=2 请用 McNemar）。"},
            {datalab::domain::DiagnosticMessage::Severity::error, "gate",
             "请选择测量列和分组列。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY(diags.size() >= 29);
        QVERIFY2(diags[0].message.find("at least two numeric") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("hypothesized mean") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("standard deviation σ") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("two independent sample columns")
                     != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[3].message.find("exactly") == std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("exactly two independent sample")
                     != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("two independent samples") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[5].message.find("exactly") == std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("test column and a reference") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("paired measurement") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("two pairs of events and trials")
                     != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[9].message.find("events column and a trials") != std::string::npos,
                 diags[9].message.c_str());
        QVERIFY2(diags[9].message.find("two pairs") == std::string::npos,
                 diags[9].message.c_str());
        QVERIFY2(diags[10].message.find("response variable and at least one predictor")
                     != std::string::npos,
                 diags[10].message.c_str());
        QVERIFY2(diags[11].message.find("hypothesized proportion") != std::string::npos,
                 diags[11].message.c_str());
        QVERIFY2(diags[12].message.find("defect-count column") != std::string::npos,
                 diags[12].message.c_str());
        QVERIFY2(diags[13].message.find("observation-length") != std::string::npos,
                 diags[13].message.c_str());
        QVERIFY2(diags[14].message.find("hypothesized rate") != std::string::npos,
                 diags[14].message.c_str());
        QVERIFY2(diags[15].message.find("two pairs of defect-count") != std::string::npos,
                 diags[15].message.c_str());
        QVERIFY2(diags[16].message.find("row-category column and a column-category")
                     != std::string::npos,
                 diags[16].message.c_str());
        QVERIFY2(diags[17].message.find("category column") != std::string::npos,
                 diags[17].message.c_str());
        QVERIFY2(diags[18].message.find("first-sample column") != std::string::npos,
                 diags[18].message.c_str());
        QVERIFY2(diags[19].message.find("enough numeric observations") != std::string::npos,
                 diags[19].message.c_str());
        QVERIFY2(diags[20].message.find("complete-case rows") != std::string::npos,
                 diags[20].message.c_str());
        QVERIFY2(diags[21].message.find("cannot exceed trials") != std::string::npos,
                 diags[21].message.c_str());
        QVERIFY2(diags[22].message.find("relative to η0") != std::string::npos,
                 diags[22].message.c_str());
        QVERIFY2(diags[23].message.find("one-sample") != std::string::npos,
                 diags[23].message.c_str());
        QVERIFY2(diags[24].message.find("numeric sequence") != std::string::npos,
                 diags[24].message.c_str());
        QVERIFY2(diags[25].message.find("2×2 table") != std::string::npos,
                 diags[25].message.c_str());
        QVERIFY2(diags[26].message.find("paired binary outcome") != std::string::npos,
                 diags[26].message.c_str());
        QVERIFY2(diags[26].message.find("exactly") != std::string::npos,
                 diags[26].message.c_str());
        QVERIFY2(diags[26].message.find("k≥3") == std::string::npos,
                 diags[26].message.c_str());
        QVERIFY2(diags[26].message.find("at least") == std::string::npos,
                 diags[26].message.c_str());
        QVERIFY2(diags[27].message.find("k≥3") != std::string::npos,
                 diags[27].message.c_str());
        QVERIFY2(diags[28].message.find("measurement column and a grouping")
                     != std::string::npos,
                 diags[28].message.c_str());
        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("请选择") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("请指定") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("所选列") == std::string::npos, diag.message.c_str());
        }
    }

    void graph_builder_need_var_and_honesty_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "散点图";
        page.parameter_summary = "方法 = pearson";

        datalab::domain::PlotSpec corr_facet;
        corr_facet.title = "相关图 · SiteA";
        corr_facet.subtitle =
            "facet = SiteA    分析 N(水平) = 8    "
            "相关矩阵热图单元格不是观测层；不伪造 per-cell member_source_rows";
        page.plots.push_back(corr_facet);

        datalab::domain::PlotSpec heatmap_facet;
        heatmap_facet.title = "热图 · SiteB";
        heatmap_facet.subtitle =
            "facet = SiteB    分析 N(水平) = 5    相关矩阵单元格不是观测层";
        page.plots.push_back(heatmap_facet);

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "graph_input",
             "请选择 X 变量和 Y 变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "graph_input",
             "请选择响应变量和分类变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "graph_input",
             "请至少选择两个连续变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "graph_input",
             "请选择 X、Y 和气泡大小变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "graph_input",
             "请选择连续变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "graph_input",
             "请选择多个连续变量，或行类别、列类别和数值。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "graph_input",
             "请选择数值变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "graph_input",
             "请选择 X、Y 和 Z 变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "graph_input",
             "请选择分类变量。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "graph_input",
             "请选择响应变量。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "bar_hidden_excluded_distinct",
             "条形图显示计数省略 hidden；分析口径计数保留 hidden、仅省略 excluded。"
             "两者不得合并为一个 bool。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_plots = page.plots;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& plots = localized.document.pages[0].visible_plots;
        QVERIFY2(
            plots[0].subtitle.find(
                "Correlation-matrix heatmap cells are not an observation layer")
                != std::string::npos,
            plots[0].subtitle.c_str());
        QVERIFY2(
            plots[0].subtitle.find("Analysis N (level) = 8") != std::string::npos,
            plots[0].subtitle.c_str());
        QVERIFY2(plots[0].subtitle.find("相关矩阵") == std::string::npos,
                 plots[0].subtitle.c_str());
        QVERIFY2(plots[0].subtitle.find("不伪造") == std::string::npos,
                 plots[0].subtitle.c_str());
        QVERIFY2(
            plots[1].subtitle.find(
                "Correlation-matrix cells are not an observation layer")
                != std::string::npos,
            plots[1].subtitle.c_str());
        QVERIFY2(plots[1].subtitle.find("相关矩阵") == std::string::npos,
                 plots[1].subtitle.c_str());

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY(diags.size() >= 11);
        QVERIFY2(diags[0].message.find("X variable and a Y variable") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("response variable and a categorical")
                     != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("at least two continuous") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("bubble-size") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("continuous variable") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("row category, column category") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[6].message.find("numeric variable") != std::string::npos,
                 diags[6].message.c_str());
        QVERIFY2(diags[7].message.find("X, Y, and Z") != std::string::npos,
                 diags[7].message.c_str());
        QVERIFY2(diags[8].message.find("categorical variable") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[9].message.find("response variable") != std::string::npos,
                 diags[9].message.c_str());
        QCOMPARE(
            QString::fromStdString(diags[10].message),
            QStringLiteral(
                "Bar display counts omit hidden; analysis counts keep hidden and "
                "only omit excluded. Do not merge these into one bool."));
        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("请选择") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("条形图显示") == std::string::npos, diag.message.c_str());
        }
    }

    void dist_id_logistic_tolerance_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "个体分布识别";
        page.parameter_summary =
            "响应 = PassFail    事件水平 = 1    预测变量数 = 3    "
            "测量 = Length    覆盖率 = 0.95    方向 = two-sided";

        datalab::domain::StatisticTable logistic_summary;
        logistic_summary.title = "模型摘要";
        logistic_summary.headers = {
            "N", "迭代次数", "收敛", "Log-Likelihood", "Deviance", "AIC", "BIC"};
        page.tables.push_back(logistic_summary);

        datalab::domain::StatisticTable goodness;
        goodness.title = "拟合优度";
        goodness.headers = {"分布", "AD", "AD*", "P-Value", "判定", "状态"};
        goodness.rows = {
            {"Normal", "*", "*", "*", "在 alpha 下拒绝", "ok"},
            {"Weibull", "*", "*", "*", "在 alpha 下未拒绝", "ok"},
        };
        page.tables.push_back(goodness);

        datalab::domain::StatisticTable hl;
        hl.title = "拟合优度";
        hl.headers = {"检验", "卡方", "DF", "组数", "P-Value", "状态"};
        page.tables.push_back(hl);

        datalab::domain::StatisticTable parameters;
        parameters.title = "参数估计";
        parameters.headers = {"分布", "位置/形状", "尺度", "状态"};
        page.tables.push_back(parameters);

        datalab::domain::StatisticTable tolerance;
        tolerance.title = "正态容差区间";
        tolerance.headers = {
            "方法", "方法族", "覆盖率", "目标置信水平", "Achieved", "k", "下限", "上限"};
        page.tables.push_back(tolerance);

        datalab::domain::PlotSpec fitted;
        fitted.title = "拟合线图";
        datalab::domain::PlotSeries fit_line;
        fit_line.label = "拟合线";
        fitted.series.push_back(fit_line);
        page.plots.push_back(fitted);

        datalab::domain::PlotSpec lambda;
        lambda.title = "Box-Cox λ";
        datalab::domain::PlotSeries selected;
        selected.label = "选定 λ";
        lambda.series.push_back(selected);
        page.plots.push_back(lambda);

        datalab::domain::PlotSpec outlier;
        outlier.title = "Grubbs";
        datalab::domain::PlotSeries suspect;
        suspect.label = "嫌疑观测";
        outlier.series.push_back(suspect);
        page.plots.push_back(outlier);

        datalab::domain::PlotSpec tukey;
        tukey.title = "Tukey 差值同时区间";
        datalab::domain::PlotSeries zero;
        zero.label = "差 = 0";
        tukey.series.push_back(zero);
        page.plots.push_back(tukey);

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Event level = 1") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Predictor count = 3") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Coverage = 0.95") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Direction = two-sided") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Predictors count") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("事件水平") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("预测变量数") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("覆盖率 =") == std::string::npos, summary.c_str());

        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(QString::fromStdString(tables[0].headers[1]), QStringLiteral("Iterations"));
        QCOMPARE(QString::fromStdString(tables[1].headers[0]), QStringLiteral("Distribution"));
        QCOMPARE(QString::fromStdString(tables[1].headers[4]), QStringLiteral("Decision"));
        QCOMPARE(QString::fromStdString(tables[1].rows[0][4]), QStringLiteral("Reject at alpha"));
        QCOMPARE(QString::fromStdString(tables[1].rows[1][4]),
                 QStringLiteral("Fail to reject at alpha"));
        QCOMPARE(QString::fromStdString(tables[2].headers[1]), QStringLiteral("Chi-square"));
        QCOMPARE(QString::fromStdString(tables[3].headers[1]),
                 QStringLiteral("Location / shape"));
        QCOMPARE(QString::fromStdString(tables[3].headers[2]), QStringLiteral("Scale"));
        QCOMPARE(QString::fromStdString(tables[4].headers[1]), QStringLiteral("Method family"));
        QCOMPARE(QString::fromStdString(tables[4].headers[2]), QStringLiteral("Coverage"));
        QCOMPARE(QString::fromStdString(tables[4].headers[3]),
                 QStringLiteral("Target confidence"));

        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(QString::fromStdString(plots[0].series[0].label),
                 QStringLiteral("Fitted line"));
        QCOMPARE(QString::fromStdString(plots[1].series[0].label),
                 QStringLiteral("Selected λ"));
        QCOMPARE(QString::fromStdString(plots[2].series[0].label),
                 QStringLiteral("Suspect observation"));
        QCOMPARE(QString::fromStdString(plots[3].series[0].label),
                 QStringLiteral("Difference = 0"));
    }

    void eda_four_plot_boxplot_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "EDA 四图";
        datalab::domain::StatisticTable notes;
        notes.title = "四图说明";
        notes.headers = {"图", "检查假设"};
        notes.rows = {
            {"Run Sequence", "位置是否漂移；散布是否大致恒定"},
            {"Lag-1", "相邻观测是否呈结构（随机性）"},
            {"Histogram", "分布形态（是否近似钟形）"},
            {"Normal Probability", "正态分位是否近似直线"},
        };
        page.tables.push_back(notes);

        datalab::domain::StatisticTable box;
        box.title = "箱线统计";
        box.headers = {
            "分组", "N", "最小值", "Q1", "中位数", "Q3", "最大值", "IQR", "下须", "上须",
            "异常点数"};
        page.tables.push_back(box);

        datalab::domain::PlotSpec plot;
        plot.title = "箱线图";
        datalab::domain::PlotSeries outliers;
        outliers.label = "异常点";
        plot.series.push_back(outliers);
        page.plots.push_back(plot);

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::info,
             "eda_4plot_exploratory",
             "EDA 四图用于探索位置/散布/随机性/分布形态假设，不能写成过程受控或分布已正态。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "no_numeric",
             "所选列没有数值观测。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(QString::fromStdString(tables[0].title), QStringLiteral("Four-plot notes"));
        QCOMPARE(QString::fromStdString(tables[0].headers[0]), QStringLiteral("Plot"));
        QCOMPARE(QString::fromStdString(tables[0].headers[1]),
                 QStringLiteral("Hypothesis to check"));
        QVERIFY2(tables[0].rows[0][1].find("location drifts") != std::string::npos,
                 tables[0].rows[0][1].c_str());
        QVERIFY2(tables[0].rows[1][1].find("randomness") != std::string::npos,
                 tables[0].rows[1][1].c_str());
        QVERIFY2(tables[0].rows[2][1].find("bell-shaped") != std::string::npos,
                 tables[0].rows[2][1].c_str());
        QVERIFY2(tables[0].rows[3][1].find("approximately linear") != std::string::npos,
                 tables[0].rows[3][1].c_str());
        QCOMPARE(QString::fromStdString(tables[1].headers[2]), QStringLiteral("Minimum"));
        QCOMPARE(QString::fromStdString(tables[1].headers[6]), QStringLiteral("Maximum"));
        QCOMPARE(QString::fromStdString(tables[1].headers[8]),
                 QStringLiteral("Lower whisker"));
        QCOMPARE(QString::fromStdString(tables[1].headers[9]),
                 QStringLiteral("Upper whisker"));
        QCOMPARE(QString::fromStdString(tables[1].headers[10]),
                 QStringLiteral("Outlier count"));

        QCOMPARE(
            QString::fromStdString(
                localized.document.pages[0].visible_plots[0].series[0].label),
            QStringLiteral("Outliers"));

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY2(diags[0].message.find("must not be written as process-in-control")
                     != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("no numeric observations") != std::string::npos,
                 diags[1].message.c_str());
        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("不能写成") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("所选列") == std::string::npos, diag.message.c_str());
        }
        QVERIFY2(tables[0].rows[0][1].find("位置是否") == std::string::npos,
                 tables[0].rows[0][1].c_str());
    }

    void reliability_exposure_interval_km_graph_axes_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "区间删失 KM";
        page.parameter_summary =
            "N = 40    精确 = 10    左 = 2    区间 = 5    右 = 23    "
            "暴露量合计 = 1000（列求和，12 行）    "
            "T_w = 12 month    暴露量 = 500（column_sum）";

        datalab::domain::PlotSpec parallel;
        parallel.title = "平行坐标图";
        parallel.y_axis_title = "标准化值";
        page.plots.push_back(parallel);
        datalab::domain::PlotSpec pie;
        pie.title = "饼图";
        pie.y_axis_title = "组成比例";
        page.plots.push_back(pie);
        datalab::domain::PlotSpec density;
        density.title = "密度图";
        density.y_axis_title = "密度";
        page.plots.push_back(density);

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "need_interval_columns",
             "请选择区间左端与右端列（右删失可用空/Inf）。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_plots = page.plots;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        QCOMPARE(QString::fromStdString(localized.document.pages[0].source_page.title),
                 QStringLiteral("Interval-censored KM"));

        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Exact = 10") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Left = 2") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Interval = 5") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Right = 23") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Total exposure = 1000") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("(column sum, 12 rows)") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Exposure = 500") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("暴露量") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("精确 =") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("列求和") == std::string::npos, summary.c_str());

        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(QString::fromStdString(plots[0].y_axis_title),
                 QStringLiteral("Standardized value"));
        QCOMPARE(QString::fromStdString(plots[1].y_axis_title),
                 QStringLiteral("Composition proportion"));
        QCOMPARE(QString::fromStdString(plots[2].y_axis_title),
                 QStringLiteral("Density"));

        QVERIFY2(
            localized.document.pages[0].visible_diagnostics[0].message.find(
                "interval left and right columns")
                != std::string::npos,
            localized.document.pages[0].visible_diagnostics[0].message.c_str());
        QVERIFY2(
            localized.document.pages[0].visible_diagnostics[0].message.find("请选择")
                == std::string::npos,
            localized.document.pages[0].visible_diagnostics[0].message.c_str());
    }

    void doe_rsm_response_opt_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "DOE 响应优化";
        page.parameter_summary =
            "响应 = Yield, Strength    有效运行数 = 20    "
            "响应 = Yield    因子数 = 3    有效观测 = 15    "
            "模型 = 线性+交互+纯二次（编码单位）";

        datalab::domain::StatisticTable objectives;
        objectives.title = "响应目标";
        objectives.headers = {"响应", "目标", "下限", "上限", "目标值", "权重"};
        objectives.rows = {{"Yield", "maximize", "10", "20", "*", "1"}};
        page.tables.push_back(objectives);

        datalab::domain::StatisticTable best;
        best.title = "最佳组合";
        best.headers = {
            "因子", "编码水平", "实际水平", "最佳预测", "单响应 D", "总体 D"};
        page.tables.push_back(best);

        datalab::domain::StatisticTable best_multi;
        best_multi.title = "最佳组合";
        best_multi.headers = {
            "因子", "编码水平", "实际水平", "Yield 预测", "Yield D", "总体 D"};
        page.tables.push_back(best_multi);

        datalab::domain::StatisticTable candidates;
        candidates.title = "候选组合";
        candidates.headers = {
            "排序", "组合", "实际水平", "预测值", "单响应 D", "总体 D"};
        page.tables.push_back(candidates);

        datalab::domain::StatisticTable prediction;
        prediction.title = "响应预测";
        prediction.headers = {
            "组合", "实际水平", "响应", "预测", "置信下限", "置信上限",
            "预测下限", "预测上限"};
        page.tables.push_back(prediction);

        datalab::domain::PlotSpec pareto;
        pareto.title = "效应 Pareto";
        pareto.y_axis_title = "|效应|";
        page.plots.push_back(pareto);

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "rsm_no_center_points",
             "未检测到中心点（编码全 0）；纯误差/失拟诊断受限。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "response_optimizer_covariance_unavailable",
             "缺少回归系数协方差矩阵，响应优化表中的置信区间与预测区间显示为 *。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "ccd_formula_reference",
             "CCD 点集按 NIST Response Surface / CCD 定义生成；证据类型 formula_reference，"
             "非 vendor_oracle，未冻结为商业软件对齐 golden。"},
            {datalab::domain::DiagnosticMessage::Severity::info,
             "bbd_formula_reference",
             "BBD 点集按 NIST Box–Behnken 定义生成；证据类型 formula_reference，"
             "非 vendor_oracle。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "insufficient_complete_case",
             "complete-case 有效行不足，无法拟合二次模型。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "factor_coding_failed",
             "因子编码失败。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Effective runs = 20") != std::string::npos, summary.c_str());
        QVERIFY2(
            summary.find("linear + interaction + pure quadratic (coded units)")
                != std::string::npos,
            summary.c_str());
        QVERIFY2(summary.find("有效运行数") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("线性+交互") == std::string::npos, summary.c_str());

        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(QString::fromStdString(tables[0].headers[1]), QStringLiteral("Goal"));
        QCOMPARE(QString::fromStdString(tables[0].headers[4]),
                 QStringLiteral("Target value"));
        QCOMPARE(QString::fromStdString(tables[0].headers[5]), QStringLiteral("Weight"));
        QCOMPARE(QString::fromStdString(tables[1].headers[3]),
                 QStringLiteral("Best prediction"));
        QCOMPARE(QString::fromStdString(tables[1].headers[4]),
                 QStringLiteral("Single-response D"));
        QCOMPARE(QString::fromStdString(tables[2].headers[3]),
                 QStringLiteral("Yield prediction"));
        QCOMPARE(QString::fromStdString(tables[2].headers[4]), QStringLiteral("Yield D"));
        QCOMPARE(QString::fromStdString(tables[3].headers[0]), QStringLiteral("Rank"));
        QCOMPARE(QString::fromStdString(tables[3].headers[3]),
                 QStringLiteral("Predicted value"));
        QCOMPARE(QString::fromStdString(tables[4].headers[3]), QStringLiteral("Prediction"));
        QCOMPARE(QString::fromStdString(tables[4].headers[4]),
                 QStringLiteral("Confidence lower"));
        QCOMPARE(QString::fromStdString(tables[4].headers[5]),
                 QStringLiteral("Confidence upper"));
        QCOMPARE(QString::fromStdString(tables[4].headers[6]),
                 QStringLiteral("Prediction lower"));
        QCOMPARE(QString::fromStdString(tables[4].headers[7]),
                 QStringLiteral("Prediction upper"));

        QCOMPARE(QString::fromStdString(
                     localized.document.pages[0].visible_plots[0].y_axis_title),
                 QStringLiteral("|Effect|"));

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY(diags.size() >= 6);
        QVERIFY2(diags[0].message.find("No center points detected") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("covariance is unavailable") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("NIST Response Surface") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[2].message.find("vendor_oracle") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[3].message.find("Box–Behnken") != std::string::npos,
                 diags[3].message.c_str());
        QVERIFY2(diags[4].message.find("complete-case") != std::string::npos,
                 diags[4].message.c_str());
        QVERIFY2(diags[5].message.find("Factor coding failed") != std::string::npos,
                 diags[5].message.c_str());
        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("未检测到中心点") == std::string::npos,
                     diag.message.c_str());
            QVERIFY2(diag.message.find("缺少回归系数") == std::string::npos,
                     diag.message.c_str());
            QVERIFY2(diag.message.find("因子编码失败") == std::string::npos,
                     diag.message.c_str());
        }
    }

    void attribute_capability_spc_pareto_chrome_localizes_to_en_us()
    {
        OutputPage page = sample_page();
        page.title = "二项过程能力";
        page.parameter_summary =
            "分布 = 二项    子组数 = 12    "
            "分布 = 二项分布    np̄_i = n_i p̄    "
            "「单点超出 3σ 控制限」= 超出 3σ 控制限的点（rule_id=beyond_control_limit）    "
            "分布 = 泊松    "
            "分布 = 泊松分布    C 图要求每个子组单位数相同    "
            "类别列: DefectType    总计数 = 40    Other 阈值 = 5.0000%    "
            "效应: Leak    显示类别数 = 5    分析类别数 = 6    类别数 = 3    原因数 = 9";

        datalab::domain::StatisticTable process;
        process.title = "过程数据";
        process.headers = {"指标", "数值"};
        process.rows = {
            {"不合格品合计", "8"},
            {"检验数合计", "200"},
            {"缺陷合计", "15"},
            {"单位数合计", "180"},
            {"目标不合格品率", "0.01"},
            {"目标 DPU", "0.02"},
        };
        page.tables.push_back(process);

        datalab::domain::StatisticTable structure;
        structure.title = "结构摘要";
        structure.headers = {"类别", "原因数", "原因"};
        structure.rows = {{"Man", "2", "A; B"}};
        page.tables.push_back(structure);

        datalab::domain::PlotSpec cum_p;
        cum_p.title = "累计 %Defective";
        page.plots.push_back(cum_p);
        datalab::domain::PlotSpec cum_dpu;
        cum_dpu.title = "累计 DPU";
        page.plots.push_back(cum_dpu);
        datalab::domain::PlotSpec fishbone;
        fishbone.title = "Leak — 类别原因计数";
        fishbone.x_axis_title = "类别";
        fishbone.y_axis_title = "原因数";
        page.plots.push_back(fishbone);
        datalab::domain::PlotSpec pareto;
        pareto.title = "DefectType 的 Pareto 图";
        page.plots.push_back(pareto);

        page.diagnostics = {
            {datalab::domain::DiagnosticMessage::Severity::error,
             "need_defect_column",
             "请选择不合格品/缺陷数列。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "need_inspected",
             "请指定检验数/单位数列或常数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "defectives_nonneg",
             "不合格品数必须是非负整数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "defectives_inspected_nonneg",
             "不合格品数和检验数必须是非负整数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "need_defectives_and_inspected",
             "请指定不合格品数列和检验数（常数或列）。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "np_need_inspected",
             "NP 图需要固定检验数或检验数列。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "defects_nonneg",
             "缺陷数必须是非负整数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "defects_nonneg_shi",
             "缺陷数必须为非负整数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "u_need_units",
             "U 图需要单位数列。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "defects_units_nonneg",
             "缺陷数和单位数必须是非负整数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "defects_units_valid",
             "缺陷数和单位数必须是有效的非负整数列。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "need_inspected_col",
             "请指定检验数列或检验数常数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "pareto_empty",
             "没有可用于柏拉图的类别数据。"},
            {datalab::domain::DiagnosticMessage::Severity::warning,
             "pareto_ignored",
             "已忽略负数或非整数缺陷计数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "inspected_nonneg",
             "检验数必须是非负整数。"},
            {datalab::domain::DiagnosticMessage::Severity::error,
             "cause_effect_cols",
             "请选择类别列与原因列。"},
        };

        auto profile = make_report_profile(ReportTemplateKind::engineer);
        profile.locale.language_tag = "en-US";
        auto document = build_report_document(sample_table(), {page}, profile);
        QVERIFY(!document.pages.empty());
        document.pages[0].visible_tables = page.tables;
        document.pages[0].visible_plots = page.plots;
        document.pages[0].visible_diagnostics = page.diagnostics;
        const auto localized = localize_report_document(document);
        QVERIFY(!localized.document.pages.empty());

        QCOMPARE(QString::fromStdString(localized.document.pages[0].source_page.title),
                 QStringLiteral("Binomial process capability"));

        const std::string& summary =
            localized.document.pages[0].source_page.parameter_summary;
        QVERIFY2(summary.find("Distribution = Binomial") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Distribution = Poisson") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Binomial分布") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Poisson分布") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("二项") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("泊松") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("np̄_i = n_i p̄") != std::string::npos, summary.c_str());
        QVERIFY2(
            summary.find(
                "\"Beyond 3σ limits\" = points beyond 3σ limits "
                "(rule_id=beyond_control_limit)")
                != std::string::npos,
            summary.c_str());
        QVERIFY2(
            summary.find("C chart requires equal units per subgroup")
                != std::string::npos,
            summary.c_str());
        QVERIFY2(summary.find("Total count = 40") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Other threshold = 5.0000%") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Displayed categories = 5") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("Analysis categories = 6") != std::string::npos,
                 summary.c_str());
        QVERIFY2(summary.find("显示Category") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("分析Category") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Category count = 3") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("Cause count = 9") != std::string::npos, summary.c_str());
        QVERIFY2(summary.find("分布 =") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("总计数") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("原因数 =") == std::string::npos, summary.c_str());
        QVERIFY2(summary.find("类别数 =") == std::string::npos, summary.c_str());

        const auto& tables = localized.document.pages[0].visible_tables;
        QCOMPARE(QString::fromStdString(tables[0].title), QStringLiteral("Process data"));
        QCOMPARE(QString::fromStdString(tables[0].rows[0][0]),
                 QStringLiteral("Total defectives"));
        QCOMPARE(QString::fromStdString(tables[0].rows[1][0]),
                 QStringLiteral("Total inspected"));
        QCOMPARE(QString::fromStdString(tables[0].rows[2][0]),
                 QStringLiteral("Total defects"));
        QCOMPARE(QString::fromStdString(tables[0].rows[3][0]),
                 QStringLiteral("Total units"));
        QCOMPARE(QString::fromStdString(tables[0].rows[4][0]),
                 QStringLiteral("Target proportion defective"));
        QCOMPARE(QString::fromStdString(tables[0].rows[5][0]),
                 QStringLiteral("Target DPU"));
        QCOMPARE(QString::fromStdString(tables[1].title),
                 QStringLiteral("Structure summary"));
        QCOMPARE(QString::fromStdString(tables[1].headers[1]),
                 QStringLiteral("Number of causes"));
        QCOMPARE(QString::fromStdString(tables[1].headers[2]), QStringLiteral("Causes"));

        const auto& plots = localized.document.pages[0].visible_plots;
        QCOMPARE(QString::fromStdString(plots[0].title),
                 QStringLiteral("Cumulative %Defective"));
        QCOMPARE(QString::fromStdString(plots[1].title), QStringLiteral("Cumulative DPU"));
        QVERIFY2(plots[2].title.find("category cause counts") != std::string::npos,
                 plots[2].title.c_str());
        QVERIFY2(plots[2].title.find("Leak") != std::string::npos, plots[2].title.c_str());
        QVERIFY2(plots[2].title.find("类别原因") == std::string::npos, plots[2].title.c_str());
        QVERIFY2(plots[3].title.find("Pareto chart") != std::string::npos,
                 plots[3].title.c_str());
        QCOMPARE(QString::fromStdString(plots[2].y_axis_title),
                 QStringLiteral("Number of causes"));

        const auto& diags = localized.document.pages[0].visible_diagnostics;
        QVERIFY(diags.size() >= 16);
        QVERIFY2(diags[0].message.find("defectives/defects column") != std::string::npos,
                 diags[0].message.c_str());
        QVERIFY2(diags[1].message.find("inspected/units") != std::string::npos,
                 diags[1].message.c_str());
        QVERIFY2(diags[2].message.find("non-negative integer") != std::string::npos,
                 diags[2].message.c_str());
        QVERIFY2(diags[5].message.find("NP chart") != std::string::npos,
                 diags[5].message.c_str());
        QVERIFY2(diags[8].message.find("U chart") != std::string::npos,
                 diags[8].message.c_str());
        QVERIFY2(diags[12].message.find("Pareto chart") != std::string::npos,
                 diags[12].message.c_str());
        QVERIFY2(diags[13].message.find("ignored") != std::string::npos,
                 diags[13].message.c_str());
        QVERIFY2(diags[14].message.find("Inspected count must be a non-negative integer")
                    != std::string::npos,
                 diags[14].message.c_str());
        QVERIFY2(diags[15].message.find("category column and a cause column")
                    != std::string::npos,
                 diags[15].message.c_str());
        for (const auto& diag : diags) {
            QVERIFY2(diag.message.find("请选择") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("不合格品") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("柏拉图") == std::string::npos, diag.message.c_str());
            QVERIFY2(diag.message.find("检验数必须") == std::string::npos, diag.message.c_str());
        }
    }

    void linguist_mirror_matches_catalog_and_qm_loads()
    {
        const QString translations_dir =
            QFileInfo(QString::fromUtf8(__FILE__)).absolutePath()
            + QStringLiteral("/../translations");
        const QString json_path = translations_dir + QStringLiteral("/report_strings.json");
        const QString en_ts = translations_dir + QStringLiteral("/DataLab_en_US.ts");
        const QString en_qm = translations_dir + QStringLiteral("/DataLab_en_US.qm");
        const QString zh_qm = translations_dir + QStringLiteral("/DataLab_zh_CN.qm");
        QVERIFY2(QFileInfo::exists(json_path), qPrintable(json_path));
        QVERIFY2(QFileInfo::exists(en_ts), qPrintable(en_ts));
        QVERIFY2(QFileInfo::exists(en_qm), qPrintable(en_qm));
        QVERIFY2(QFileInfo::exists(zh_qm), qPrintable(zh_qm));
        QVERIFY(QFileInfo(en_qm).size() > 1000);
        QVERIFY(QFileInfo(zh_qm).size() > 1000);

        QFile json_file(json_path);
        QVERIFY(json_file.open(QIODevice::ReadOnly));
        const QJsonObject root = QJsonDocument::fromJson(json_file.readAll()).object();
        QCOMPARE(root.value(QStringLiteral("authority")).toString(),
                 QStringLiteral("src/domain/report_text_catalog.cpp"));
        QVERIFY(root.value(QStringLiteral("note")).toString().contains(
            QStringLiteral("vendor_oracle")));
        const QJsonArray entries = root.value(QStringLiteral("entries")).toArray();
        QVERIFY(entries.size() > 0);
        QCOMPARE(static_cast<int>(entries.size()),
                 static_cast<int>(datalab::domain::report_text_catalog().size()));

        for (const auto& catalog_entry : datalab::domain::report_text_catalog()) {
            bool found = false;
            for (const QJsonValue& value : entries) {
                const QJsonObject object = value.toObject();
                if (object.value(QStringLiteral("id")).toString().toStdString()
                    == catalog_entry.id) {
                    QCOMPARE(object.value(QStringLiteral("zh_cn")).toString().toStdString(),
                             catalog_entry.zh_cn);
                    QCOMPARE(object.value(QStringLiteral("en_us")).toString().toStdString(),
                             catalog_entry.en_us);
                    found = true;
                    break;
                }
            }
            QVERIFY2(found, catalog_entry.id.c_str());
        }

        QFile ts_file(en_ts);
        QVERIFY(ts_file.open(QIODevice::ReadOnly));
        const QString ts_text = QString::fromUtf8(ts_file.readAll());
        QVERIFY(ts_text.contains(QStringLiteral("ReportCatalog")));
        QVERIFY(ts_text.contains(QStringLiteral("DataLabUi")));
        QVERIFY(ts_text.contains(QStringLiteral("id=\"report.title\"")));
        QVERIFY(ts_text.contains(QStringLiteral("id=\"report.pdfa_banner\"")));
        QVERIFY(ts_text.contains(QStringLiteral("id=\"menu.file\"")));
        QVERIFY(ts_text.contains(QStringLiteral("<source>文件</source>")));
        QVERIFY(ts_text.contains(QStringLiteral("<translation>File</translation>")));

        const QString ui_json_path =
            translations_dir + QStringLiteral("/ui_menu_strings.json");
        QVERIFY2(QFileInfo::exists(ui_json_path), qPrintable(ui_json_path));
        QFile ui_json_file(ui_json_path);
        QVERIFY(ui_json_file.open(QIODevice::ReadOnly));
        const QJsonObject ui_root =
            QJsonDocument::fromJson(ui_json_file.readAll()).object();
        QCOMPARE(ui_root.value(QStringLiteral("context")).toString(),
                 QStringLiteral("DataLabUi"));
        QVERIFY(ui_root.value(QStringLiteral("entries")).toArray().size() >= 400);

        QTranslator en_translator;
        QVERIFY(en_translator.load(en_qm));
        qApp->installTranslator(&en_translator);
        QCOMPARE(datalab::ui::ui_tr("文件"), QStringLiteral("File"));
        QCOMPARE(datalab::ui::ui_tr("统计"), QStringLiteral("Stat"));
        QCOMPARE(datalab::ui::ui_tr("新建项目"), QStringLiteral("New Project"));
        QCOMPARE(datalab::ui::ui_tr("散点图"), QStringLiteral("Scatterplot"));
        QCOMPARE(datalab::ui::ui_tr("可靠性分析（Kaplan-Meier / Weibull）"),
                 QStringLiteral("Reliability Analysis (Kaplan-Meier / Weibull)"));
        QCOMPARE(datalab::ui::ui_tr("中心复合设计 CCD"),
                 QStringLiteral("Central Composite Design (CCD)"));
        QCOMPARE(datalab::ui::ui_tr("等值线图"), QStringLiteral("Contour Plot"));
        // Analysis setup dialog roles / chrome (Phase 3 dialog-role deepen)
        QCOMPARE(datalab::ui::ui_tr("响应变量"), QStringLiteral("Response variable"));
        QCOMPARE(datalab::ui::ui_tr("因子/分组列"),
                 QStringLiteral("Factor / grouping column"));
        QCOMPARE(datalab::ui::ui_tr("分析设置"), QStringLiteral("Analysis settings"));
        QCOMPARE(datalab::ui::ui_tr("运行分析"), QStringLiteral("Run analysis"));
        QCOMPARE(datalab::ui::ui_tr("工作表列"), QStringLiteral("Worksheet columns"));
        QCOMPARE(datalab::ui::ui_tr("特殊原因测试"),
                 QStringLiteral("Special-cause tests"));
        QCOMPARE(datalab::ui::ui_tr("寿命/时间"), QStringLiteral("Lifetime / time"));
        qApp->removeTranslator(&en_translator);

        QTranslator zh_translator;
        QVERIFY(zh_translator.load(zh_qm));
        qApp->installTranslator(&zh_translator);
        QCOMPARE(datalab::ui::ui_tr("文件"), QStringLiteral("文件"));
        qApp->removeTranslator(&zh_translator);
    }
};

QTEST_MAIN(ReportLocalePhase3Test)
#include "report_locale_phase3_test.moc"
