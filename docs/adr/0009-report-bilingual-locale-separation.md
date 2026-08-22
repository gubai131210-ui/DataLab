# ADR 0009：报告双语与界面语言分离

- 状态：已接受
- 日期：2026-08-21
- 取代：部分取代 [ADR 0006](0006-simplified-chinese-single-language.md) 关于“禁止铺设翻译流程”的约束

## 背景

Phase 0–2 已冻结 `ReportProfile.locale` 与 `LocaleTextId`。产品需要中英报告，但不能把系统/UI locale 写进 domain Facts，也不能静默中英混排。

## 决策

1. **界面默认语言**仍为简体中文（zh-CN）；不要求本阶段完成全部菜单 `tr()` 覆盖。
2. **报告语言**由 `ReportProfile.locale.language_tag` 独立保存，支持至少 `zh-CN` 与 `en-US`。
3. 报告用户可见文案通过稳定文本 ID（`report_text_catalog`）解析；domain Facts 只保留数值与稳定 ID。
4. 缺失翻译必须产生诊断并回退到源语言文本，**禁止**静默混语。
5. 数字/日期格式跟随报告 locale 字段；**不得**用机器当前 locale 覆盖已保存的报告 locale。
6. Qt Linguist `.ts/.qm` 作为 UI/长期翻译管线资源；报告竖切以 catalog/JSON 为权威，与 `.ts` 同步维护（`tools/sync_report_linguist.py`；UI 可用 `DATALAB_UI_LANG=en-US` 加载对应 `.qm`）。

## 后果

- ADR 0006 仍解释“UI 默认中文”的产品定位，但不再阻止报告双语基础设施。
- PDF/预览/manifest 的 locale 必须与 `ReportProfile.locale` 一致。
- 规则 **ID** 不翻译；规则 **display name** 可翻译。
