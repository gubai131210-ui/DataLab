#!/usr/bin/env python3
"""Wave-3 inference / ANOVA / regression datasets, generators, and overlay writers.

Catalog: wave3_catalog.json. Overlays: tutorial_overlays/<id>.json (56).
Import GENERATORS / WAVE3_DATASETS / ROLE_MAP_BY_DATASET from builders.
Run this module to regenerate overlay JSON from embedded rebuild helpers if needed;
primary overlays are already written beside this file.
"""
from __future__ import annotations

import json
import math
import random
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
OVERLAY_DIR = HERE / "tutorial_overlays"
_CATALOG = json.loads((HERE / "wave3_catalog.json").read_text(encoding="utf-8"))
WAVE3_DATASETS: dict[str, dict] = _CATALOG["datasets"]
ROLE_MAP_BY_DATASET: dict[str, dict[str, str]] = _CATALOG["role_maps"]


def _poisson(rng: random.Random, lam: float) -> int:
    L = math.exp(-lam)
    k = 0
    p = 1.0
    while p > L:
        k += 1
        p *= rng.random()
    return max(0, k - 1)


def gen_infer_one_sample_mean(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(301)
    rows = []
    for i in range(1, 51):
        y = 123.0 + rng.gauss(0, 2.0)
        rows.append([str(i), f"{y:.2f}", "相对目标120偏移"])
    return rows


def gen_infer_paired_shift(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(302)
    rows = []
    for i in range(1, 41):
        before = 100.0 + rng.gauss(0, 1.0)
        after = before + 2.5 + rng.gauss(0, 0.6)
        rows.append([str(i), f"{before:.2f}", f"{after:.2f}", "返工抬高"])
    return rows


def gen_infer_two_sample_location(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(303)
    rows = []
    for i in range(1, 31):
        a = 100.0 + rng.gauss(0, 1.0)
        b = 101.2 + rng.gauss(0, 1.0)
        rows.append([str(i), f"{a:.2f}", f"{b:.2f}", "B线偏高"])
    return rows


def gen_cat_shift_line(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(304)
    # Early: more balanced; Late: 虚焊 heavy
    bag = []
    for _ in range(60):
        bag.append(["早班", rng.choice(["虚焊", "偏移", "桥连", "其他", "偏移", "桥连"]), "基线"])
    for _ in range(60):
        bag.append(["晚班", rng.choice(["虚焊", "虚焊", "虚焊", "偏移", "桥连", "其他"]), "晚班虚焊↑"])
    rng.shuffle(bag)
    return bag


def gen_gof_category_bias(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(305)
    bag = ["虚焊"] * 45 + ["偏移"] * 20 + ["桥连"] * 20 + ["其他"] * 15
    rng.shuffle(bag)
    return [[c, "偏离均匀"] for c in bag]


def gen_cochran_three_repeat(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(306)
    rows = []
    for i in range(1, 37):
        a = 1 if rng.random() > 0.25 else 0
        b = 1 if rng.random() > 0.28 else 0
        c = 1 if rng.random() > 0.55 else 0  # stricter -> more 0
        rows.append([str(i), str(a), str(b), str(c)])
    return rows


def gen_corr_temp_offset_y(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(307)
    rows = []
    for i in range(1, 46):
        t = 240.0 + rng.gauss(0, 8.0)
        y = 0.35 * (t - 240.0) + 5.0 + rng.gauss(0, 1.2)
        note = "正相关"
        if 40 <= i <= 42:
            y += 4.0
            note = "轻微离群"
        rows.append([str(i), f"{t:.1f}", f"{y:.2f}", note])
    return rows


def gen_desc_unimodal_stable(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(308)
    return [[str(i), f"{100.0 + rng.gauss(0, 1.0):.2f}", "单峰稳定"] for i in range(1, 61)]


def gen_fisher_small_counts(_rng: random.Random) -> list[list[str]]:
    # Fixed small 2x2 recipe expanded to rows
    rows = []
    rows += [["A", "良", "小计数"]] * 10
    rows += [["A", "不良", "小计数"]] * 2
    rows += [["B", "良", "小计数"]] * 8
    rows += [["B", "不良", "小计数"]] * 8
    return rows


def gen_friedman_three_treat(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(309)
    rows = []
    for block in range(1, 13):
        for treat, delta in (("A", 0.0), ("B", 0.2), ("C", -1.5)):
            y = 8.0 + delta + rng.gauss(0, 0.4)
            note = "C偏低" if treat == "C" else "基线"
            rows.append([f"{y:.2f}", treat, str(block), note])
    return rows


def gen_kw_three_cavity(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(310)
    rows = []
    for cav in (1, 2, 3):
        mean = 98.0 if cav == 3 else 100.0
        note = "腔3偏低" if cav == 3 else "基线"
        for _ in range(15):
            rows.append([f"{mean + rng.gauss(0, 1.0):.2f}", str(cav), note])
    return rows


def gen_logit_pass_fail(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(311)
    rows = []
    for i in range(1, 51):
        x = rng.uniform(0, 12)
        p = 1 / (1 + math.exp(-(x - 6.5)))
        fail = 1 if rng.random() < p else 0
        note = "高偏移易失败" if x > 8 else "低偏移"
        rows.append([str(i), str(fail), f"{x:.2f}", note])
    return rows


def gen_mcnemar_paired_binary(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(312)
    rows = []
    for i in range(1, 41):
        before = 1 if rng.random() < 0.35 else 0
        # after more likely 1, especially if before 0
        if before == 0:
            after = 1 if rng.random() < 0.55 else 0
        else:
            after = 1 if rng.random() < 0.85 else 0
        rows.append([str(i), str(before), str(after), "后检出↑"])
    return rows


def gen_mood_two_group(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(313)
    rows = []
    for _ in range(20):
        rows.append([f"{100.0 + rng.gauss(0, 1.0):.2f}", "A", "基线"])
    for _ in range(20):
        rows.append([f"{102.5 + rng.gauss(0, 1.0):.2f}", "B", "B中位高"])
    return rows


def gen_norm_mild_skew(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(314)
    rows = []
    for i in range(1, 51):
        y = math.exp(rng.gauss(4.60, 0.18))
        note = "轻度右偏"
        if i >= 48:
            y *= 1.35
            note = "右尾"
        rows.append([str(i), f"{y:.2f}", note])
    return rows


def gen_pois_one_count(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(315)
    rows = []
    for i in range(1, 31):
        length = rng.choice([50, 80, 100])
        defects = _poisson(rng, 0.10 * length)
        rows.append([str(i), str(defects), str(length), "率偏高"])
    return rows


def gen_prop_one_lot(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(316)
    rows = []
    for i in range(1, 31):
        n = rng.choice([80, 100, 120])
        d = sum(1 for _ in range(n) if rng.random() < 0.06)
        rows.append([str(i), str(d), str(n), "比例偏高"])
    return rows


def gen_equiv_prop_one(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(317)
    rows = []
    for i in range(1, 29):
        n = rng.choice([100, 120, 150])
        d = sum(1 for _ in range(n) if rng.random() < 0.05)
        rows.append([str(i), str(d), str(n), "近目标"])
    return rows


def gen_equiv_one_near_target(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(318)
    return [[str(i), f"{100.1 + rng.gauss(0, 0.35):.2f}", "近目标"] for i in range(1, 41)]


def gen_anova_one_cavity(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(319)
    rows = []
    for cav in (1, 2, 3):
        mean = 98.0 if cav == 3 else 100.0
        note = "腔3偏低" if cav == 3 else "基线"
        for _ in range(15):
            rows.append([f"{mean + rng.gauss(0, 0.9):.2f}", str(cav), note])
    return rows


def gen_outlier_one_spike(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(320)
    rows = []
    for i in range(1, 41):
        if i == 28:
            rows.append([str(i), "112.00", "尖峰"])
        else:
            rows.append([str(i), f"{100.0 + rng.gauss(0, 1.0):.2f}", "基线"])
    return rows


def gen_equiv_paired_near(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(321)
    rows = []
    for i in range(1, 36):
        before = 100.0 + rng.gauss(0, 0.8)
        after = before + rng.gauss(0, 0.25)
        rows.append([str(i), f"{before:.2f}", f"{after:.2f}", "近零差"])
    return rows


def gen_regr_temp_strength(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(322)
    rows = []
    for i in range(1, 41):
        t = 150.0 + rng.uniform(0, 40)
        y = 20.0 + 0.15 * t + rng.gauss(0, 1.5)
        note = "正斜率"
        if i >= 38:
            y += rng.gauss(0, 3.0)
            note = "残差稍大"
        rows.append([str(i), f"{y:.2f}", f"{t:.1f}", note])
    return rows


def gen_runs_clustered(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(323)
    rows = []
    for i in range(1, 51):
        if 20 <= i <= 35:
            y = 100.8 + abs(rng.gauss(0, 0.15))
            note = "同侧游程"
        else:
            y = 100.0 + rng.gauss(0, 0.5)
            note = "基线"
        rows.append([str(i), f"{y:.2f}", note])
    return rows


def gen_anova_two_factor(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(324)
    rows = []
    for shift in ("早班", "晚班"):
        for pos in ("左", "中", "右"):
            for _ in range(8):
                y = 100.0 + rng.gauss(0, 0.7)
                if shift == "晚班":
                    y += 1.6
                note = "晚班抬高" if shift == "晚班" else "基线"
                rows.append([f"{y:.2f}", pos, shift, note])
    return rows


def gen_pois_two_count(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(325)
    rows = []
    for i in range(1, 25):
        la, lb = rng.choice([50, 80]), rng.choice([50, 80])
        da = _poisson(rng, 0.04 * la)
        db = _poisson(rng, 0.10 * lb)
        rows.append([str(i), str(da), str(la), str(db), str(lb), "B率高"])
    return rows


def gen_equiv_prop_two(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(326)
    rows = []
    for i in range(1, 25):
        na = rng.choice([100, 120])
        nb = rng.choice([100, 120])
        da = sum(1 for _ in range(na) if rng.random() < 0.05)
        db = sum(1 for _ in range(nb) if rng.random() < 0.05)
        rows.append([str(i), str(da), str(na), str(db), str(nb), "近相等"])
    return rows


def gen_prop_two_line(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(327)
    rows = []
    for i in range(1, 25):
        na = rng.choice([100, 120])
        nb = rng.choice([100, 120])
        da = sum(1 for _ in range(na) if rng.random() < 0.03)
        db = sum(1 for _ in range(nb) if rng.random() < 0.08)
        rows.append([str(i), str(da), str(na), str(db), str(nb), "B比例高"])
    return rows


def gen_equiv_two_near_equal(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(328)
    rows = []
    for i in range(1, 26):
        a = 100.0 + rng.gauss(0, 0.8)
        b = 100.1 + rng.gauss(0, 0.8)
        rows.append([str(i), f"{a:.2f}", f"{b:.2f}", "近相等"])
    return rows


def gen_equiv_ratio_near_one(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(329)
    rows = []
    for i in range(1, 21):
        ref = 100.0 + rng.gauss(0, 0.6)
        test = ref * (1.0 + rng.gauss(0, 0.02))
        rows.append([str(i), f"{test:.2f}", f"{ref:.2f}", "比近1"])
    return rows


def gen_var_two_line_unequal(_rng: random.Random) -> list[list[str]]:
    rng = random.Random(330)
    rows = []
    for i in range(1, 26):
        a = 100.0 + rng.gauss(0, 0.7)
        b = 100.0 + rng.gauss(0, 1.5)
        rows.append([str(i), f"{a:.2f}", f"{b:.2f}", "B方差大"])
    return rows


GENERATORS = {
    "infer_one_sample_mean": gen_infer_one_sample_mean,
    "infer_paired_shift": gen_infer_paired_shift,
    "infer_two_sample_location": gen_infer_two_sample_location,
    "cat_shift_line": gen_cat_shift_line,
    "gof_category_bias": gen_gof_category_bias,
    "cochran_three_repeat": gen_cochran_three_repeat,
    "corr_temp_offset_y": gen_corr_temp_offset_y,
    "desc_unimodal_stable": gen_desc_unimodal_stable,
    "fisher_small_counts": gen_fisher_small_counts,
    "friedman_three_treat": gen_friedman_three_treat,
    "kw_three_cavity": gen_kw_three_cavity,
    "logit_pass_fail": gen_logit_pass_fail,
    "mcnemar_paired_binary": gen_mcnemar_paired_binary,
    "mood_two_group": gen_mood_two_group,
    "norm_mild_skew": gen_norm_mild_skew,
    "pois_one_count": gen_pois_one_count,
    "prop_one_lot": gen_prop_one_lot,
    "equiv_prop_one": gen_equiv_prop_one,
    "equiv_one_near_target": gen_equiv_one_near_target,
    "anova_one_cavity": gen_anova_one_cavity,
    "outlier_one_spike": gen_outlier_one_spike,
    "equiv_paired_near": gen_equiv_paired_near,
    "regr_temp_strength": gen_regr_temp_strength,
    "runs_clustered": gen_runs_clustered,
    "anova_two_factor": gen_anova_two_factor,
    "pois_two_count": gen_pois_two_count,
    "equiv_prop_two": gen_equiv_prop_two,
    "prop_two_line": gen_prop_two_line,
    "equiv_two_near_equal": gen_equiv_two_near_equal,
    "equiv_ratio_near_one": gen_equiv_ratio_near_one,
    "var_two_line_unequal": gen_var_two_line_unequal,
}

WAVE3_COMMAND_IDS = sorted(GENERATORS)  # datasets; overlays are separate files


def write_overlays() -> None:
    """Polish Wave-3 overlay JSON (56) in place; other waves live in the same folder."""
    from copy_depth import polish_overlay

    wave3 = json.loads((HERE / "wave3_overlay_ids.json").read_text(encoding="utf-8"))
    missing = [cid for cid in wave3 if not (OVERLAY_DIR / f"{cid}.json").is_file()]
    if missing:
        raise SystemExit(f"missing overlays: {missing}")
    for cid in wave3:
        path = OVERLAY_DIR / f"{cid}.json"
        overlay = json.loads(path.read_text(encoding="utf-8"))
        path.write_text(
            json.dumps(polish_overlay(cid, overlay), ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
    print(f"Wrote {len(wave3)} Wave-3 overlays to {OVERLAY_DIR}")


if __name__ == "__main__":
    write_overlays()
