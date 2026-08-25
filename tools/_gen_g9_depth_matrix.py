#!/usr/bin/env python3
"""Generate g9-show-your-work-depth-matrix.md from coverage + deepen metadata."""

from __future__ import annotations

import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
COVERAGE = ROOT / "docs/research/g9-formula-substitution-coverage-matrix.md"
OUT = ROOT / "docs/research/g9-show-your-work-depth-matrix.md"

# L2 exceptions (<=15% A-class) with written reasons per Planner gate.
L2_EXCEPTIONS: dict[str, str] = {
    "bootstrap_mean": "Bootstrap 重采样路径全集不可稳定逐步展开",
    "bootstrap_two_sample": "Bootstrap 双样本重采样路径不可逐步展开",
    "random_forest": "袋装树路径摘要；非全树逐步",
    "cart_tree": "CART 分裂路径摘要；非全树逐步",
    "isolation_forest": "孤立路径摘要；非全路径逐步",
    "stepwise_regression": "逐步选择迭代；展示最终系数代入",
    "best_subsets_regression": "子集搜索组合爆炸；展示 Cp 最优子集",
    "arima": "ARIMA 拟合迭代；展示选定阶数参数代入",
    "time_series_smoothing": "指数平滑递推；展示 λ/水平一步代入",
    "time_series_decomposition": "分解滤波递推；展示季节/趋势摘要",
    "seasonal_forecasting": "季节模型递推；展示参数一步代入",
    "acf_pacf": "自相关序列摘要；非逐步谱分解",
    "ccf": "互相关序列摘要",
    "correlogram": "相关图序列摘要",
    "cluster_observations": "层次聚类树摘要",
    "km_interval": "Greenwood 递推区间；展示 KM 点估计代入",
}

D_CLASS = {
    "histogram", "eda_4plot", "boxplot", "pareto", "run_chart", "cause_and_effect",
    "density_plot", "hexbin_plot", "violin_plot", "bar_chart", "scatter_plot",
    "interval_plot", "correlation_plot", "bubble_plot", "probability_plot",
    "ecdf_plot", "matrix_plot", "marginal_plot", "parallel_plot", "heatmap_plot",
    "time_series_plot", "area_plot", "contour_plot", "pie_plot", "variability_chart",
}

B_CLASS = {
    "doe_plackett_burman", "taguchi_orthogonal_design", "doe_ccd", "doe_bbd",
    "doe_factorial", "doe_response", "rsm_response", "response_optimization",
    "acceptance_sampling", "distribution_identification", "box_cox", "multi_vari",
    "anom",
}

C_CLASS = {
    "distribution_calculator", "t_power", "pca", "kmeans", "cart_tree",
    "random_forest", "isolation_forest", "discriminant", "hotelling_t2",
}


def list_command_ids() -> list[str]:
    out = subprocess.check_output(
        [sys.executable, str(ROOT / "tools/_list_command_ids.py")],
        cwd=ROOT,
        text=True,
        encoding="utf-8",
    )
    ids: list[str] = []
    started = False
    for line in out.splitlines():
        if line.startswith("count="):
            started = True
            continue
        if started and line.strip():
            ids.append(line.strip())
    return ids


def parse_coverage() -> dict[str, str]:
    text = COVERAGE.read_text(encoding="utf-8")
    families: dict[str, str] = {}
    for m in re.finditer(
        r"^\|\s*([a-z0-9_]+)\s*\|\s*([^|]+)\|\s*([^|]+)\|\s*([^|]+)\|\s*([^|]+)\|",
        text,
        flags=re.M,
    ):
        families[m.group(1).strip()] = m.group(2).strip()
    return families


def depth_for(cid: str, family: str, status: str) -> tuple[str, str]:
    if cid in {"tests", "rule_policy"}:
        return "L0", "元命令豁免"
    if cid in D_CLASS or status == "display_summary":
        return "L1", "图形显示摘要；真实 N/规则"
    if cid in L2_EXCEPTIONS:
        return "L2", L2_EXCEPTIONS[cid]
    if cid in C_CLASS:
        if cid in {"distribution_calculator", "t_power", "hotelling_t2"}:
            return "L3", "关键方程分步验算"
        return "L2", "关键方程代入验算"
    if cid in B_CLASS:
        if cid in {
            "doe_factorial",
            "doe_response",
            "rsm_response",
            "taguchi_orthogonal_design",
        }:
            return "L3", "DOE 分析/设计规则分步验算"
        return "L2", "设计/生成规则代入验算"
    return "L3", "Facts 优先分步验算"


def main() -> None:
    families = parse_coverage()
    ids = list_command_ids()
    lines = [
        "# G9-D 验算轨迹深度矩阵",
        "",
        "> 访问日期：2026-08-24（UTC+8）",
        "> depth ∈ {L3, L2, L1, L0} · 权威命令源：`python tools/_list_command_ids.py`",
        "",
        "| command_id | family | depth | notes |",
        "|---|---|---|---|",
    ]
    for cid in ids:
        family = families.get(cid, "未分类")
        status = "实质绑定"
        if cid in D_CLASS:
            status = "display_summary"
        depth, notes = depth_for(cid, family, status)
        notes = notes.replace("|", "/")
        lines.append(f"| {cid} | {family} | {depth} | {notes} |")
    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {OUT} ({len(ids)} rows)")


if __name__ == "__main__":
    main()
