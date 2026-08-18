#!/usr/bin/env python3
"""Dump DataLab-equivalent golden values by mirroring domain ARIMA/regression logic."""

from __future__ import annotations

import csv
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = ROOT / "tests" / "fixtures" / "minitab" / "expected"
K_NORMAL95 = 1.959963984540054
K_MIN_VAR = 1.0e-12


def lstsq(design, response):
    k = len(design[0])
    normal = [[0.0] * k for _ in range(k)]
    rhs = [0.0] * k
    for idx, row in enumerate(design):
        for i in range(k):
            rhs[i] += row[i] * response[idx]
            for j in range(k):
                normal[i][j] += row[i] * row[j]
    aug = [normal[i] + [rhs[i]] for i in range(k)]
    for col in range(k):
        pivot = max(range(col, k), key=lambda r: abs(aug[r][col]))
        aug[col], aug[pivot] = aug[pivot], aug[col]
        div = aug[col][col]
        aug[col] = [v / div for v in aug[col]]
        for r in range(k):
            if r == col:
                continue
            factor = aug[r][col]
            aug[r] = [aug[r][c] - factor * aug[col][c] for c in range(k + 1)]
    return [aug[i][k] for i in range(k)]


def information_criteria(sse: float, n: int, parameters: int) -> tuple[float, float, float]:
    variance = max(sse / n, K_MIN_VAR)
    log_likelihood = -0.5 * n * (math.log(2 * math.pi) + 1 + math.log(variance))
    aic = -2 * log_likelihood + 2 * parameters
    bic = -2 * log_likelihood + parameters * math.log(n)
    denominator = n - parameters - 1
    aicc = aic + 2 * parameters * (parameters + 1) / denominator if denominator > 0 else aic
    return aic, aicc, bic


def difference_series(values: list[float], d: int) -> list[float]:
    current = values[:]
    for _ in range(d):
        current = [current[i] - current[i - 1] for i in range(1, len(current))]
    return current


def fit_ar_ols(series: list[float], p: int) -> tuple[list[float], float, float]:
    n = len(series) - p
    design = []
    response = []
    for row in range(p, len(series)):
        design.append([1.0] + [series[row - lag] for lag in range(1, p + 1)])
        response.append(series[row])
    beta = lstsq(design, response)
    intercept = beta[0]
    ar = beta[1:]
    sse = 0.0
    for row in range(p, len(series)):
        fitted = intercept + sum(ar[lag - 1] * series[row - lag] for lag in range(1, p + 1))
        sse += (series[row] - fitted) ** 2
    return ar, intercept, sse


def fit_random_walk_drift(observations: list[float], forecast_periods: int) -> dict:
    n = len(observations) - 1
    drift = (observations[-1] - observations[0]) / n
    sse = sum(
        (observations[i] - (observations[i - 1] + drift)) ** 2 for i in range(1, len(observations))
    )
    aic, aicc, bic = information_criteria(sse, n, 2)
    variance = max(sse / n, K_MIN_VAR)
    forecasts = [observations[-1] + h * drift for h in range(1, forecast_periods + 1)]
    lowers, uppers = [], []
    for h, fc in enumerate(forecasts, 1):
        margin = K_NORMAL95 * math.sqrt(variance * h)
        lowers.append(fc - margin)
        uppers.append(fc + margin)
    return {
        "order": (0, 1, 0),
        "sse": sse,
        "aic": aic,
        "aicc": aicc,
        "bic": bic,
        "forecasts": forecasts,
        "lower": lowers,
        "upper": uppers,
    }


def fit_arima_order(observations: list[float], p: int, d: int, q: int, forecast_periods: int) -> dict | None:
    if p > 0 and q > 0:
        return None
    if p == 0 and q == 0 and d == 1:
        return fit_random_walk_drift(observations, forecast_periods)
    if p == 0 and q == 0 and d == 0:
        return None
    differenced = difference_series(observations, d) if d > 0 else observations[:]
    effective_n = len(differenced) - max(p, 1)
    if p > 0 and q == 0:
        ar, intercept, sse = fit_ar_ols(differenced, p)
        extended = differenced[:]
        diff_forecasts = []
        for _ in range(forecast_periods):
            nxt = intercept + sum(ar[lag - 1] * extended[-lag] for lag in range(1, p + 1))
            diff_forecasts.append(nxt)
            extended.append(nxt)
    elif q > 0 and p == 0:
        return None  # skip MA grid in dump
    else:
        return None
    if d > 0:
        last_levels = [observations[-d - 1]]
        for step in range(d - 1, -1, -1):
            last_levels.append(observations[-step - 1])
        levels = last_levels[:]
        forecasts = []
        for increment in diff_forecasts:
            levels.append(levels[-1] + increment)
            forecasts.append(levels[-1])
    else:
        forecasts = diff_forecasts
    params = 1 + p + q if p > 0 or q > 0 else (2 if d > 0 else 1)
    aic, aicc, bic = information_criteria(sse, max(1, effective_n), params)
    variance = max(sse / max(1, effective_n), K_MIN_VAR)
    lowers, uppers = [], []
    for h, fc in enumerate(forecasts, 1):
        multiplier = h if q == 0 else 1.0 + 0.25 * h
        margin = K_NORMAL95 * math.sqrt(variance * multiplier)
        lowers.append(fc - margin)
        uppers.append(fc + margin)
    return {
        "order": (p, d, q),
        "sse": sse,
        "aic": aic,
        "aicc": aicc,
        "bic": bic,
        "forecasts": forecasts,
        "lower": lowers,
        "upper": uppers,
    }


def fit_arima_candidates(values: list[float], forecast_periods: int, max_d: int = 1) -> list[dict]:
    candidates = []
    for d in range(0, max_d + 1):
        for p in range(0, 4):
            for q in range(0, 4):
                if p == 0 and q == 0 and d == 0:
                    continue
                if p > 0 and q > 0:
                    continue
                fitted = fit_arima_order(values, p, d, q, forecast_periods)
                if fitted is not None:
                    candidates.append(fitted)
    return candidates


def write_regression_golden() -> None:
    rows = list(csv.DictReader((ROOT / "samples/statistical_inference/regression.csv").open()))
    y = [float(r["Response"]) for r in rows]
    temp = [float(r["Temperature"]) for r in rows]
    press = [float(r["Pressure"]) for r in rows]
    n = len(y)
    p = 3
    design = [[1.0, temp[i], press[i]] for i in range(n)]
    beta = lstsq(design, y)
    fitted = [beta[0] + beta[1] * temp[i] + beta[2] * press[i] for i in range(n)]
    resid = [y[i] - fitted[i] for i in range(n)]
    sse = sum(r * r for r in resid)
    ybar = sum(y) / n
    sst = sum((yi - ybar) ** 2 for yi in y)
    s = math.sqrt(sse / (n - p))
    r2 = 1 - sse / sst
    adj_r2 = 1 - (1 - r2) * (n - 1) / (n - p)

    def rss(cols):
        d = [[1.0] + [design[i][c] for c in cols] for i in range(n)]
        b = lstsq(d, y)
        return sum((y[i] - sum(b[j] * d[i][j] for j in range(len(b)))) ** 2 for i in range(n))

    ss_seq_temp = sst - rss([1])
    ss_seq_press = rss([1]) - sse
    adj_temp = rss([2]) - sse
    adj_press = rss([1]) - sse
    de = [resid[i] - resid[i - 1] for i in range(1, n)]
    dw = sum(d * d for d in de) / sse
    lines = [
        "# source: OLS formulas per docs/statistical-methodology.md (verify against Minitab export)",
        "# config: response_col=Response",
        "# config: predictor_cols=Temperature,Pressure",
        "# config: confidence=0.95",
        "# section: summary",
        "Metric\tValue",
        f"S\t{s}",
        f"R-sq\t{r2}",
        f"R-sq(adj)\t{adj_r2}",
        f"Durbin-Watson\t{dw}",
        "# section: anova",
        "Term\tSeq_SS\tAdj_SS\tDF",
        f"Temperature\t{ss_seq_temp}\t{adj_temp}\t1",
        f"Pressure\t{ss_seq_press}\t{adj_press}\t1",
        "# section: coefficients",
        "Term\tCoef",
        f"Constant\t{beta[0]}",
        f"Temperature\t{beta[1]}",
        f"Pressure\t{beta[2]}",
    ]
    (EXPECTED / "regression_golden.tsv").write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_arima_golden() -> None:
    values = []
    with (ROOT / "samples/time_series/arima/arima_trend.csv").open() as f:
        for row in csv.DictReader(f):
            values.append(float(row["Value"]))
    candidates = fit_arima_candidates(values, 4, max_d=1)
    best = min(candidates, key=lambda item: item["aicc"])
    p, d, q = best["order"]
    lines = [
        "# source: mirrors src/domain/statistics/arima.cpp candidate grid (verify against Minitab export)",
        "# config: time_col=Period",
        "# config: value_col=Value",
        "# config: criterion=aicc",
        "# config: forecast_periods=4",
        "# config: arima_differencing=1",
        "# section: candidates",
        "Order\tSSE\tAIC\tAICc\tBIC",
    ]
    for candidate in sorted(candidates, key=lambda item: item["aicc"]):
        cp, cd, cq = candidate["order"]
        lines.append(
            f"ARIMA({cp},{cd},{cq})\t{candidate['sse']}\t{candidate['aic']}\t{candidate['aicc']}\t{candidate['bic']}"
        )
    lines += [
        "# section: forecast",
        "Period\tForecast\tLower\tUpper",
    ]
    for idx, (fc, lo, up) in enumerate(
        zip(best["forecasts"], best["lower"], best["upper"]), 1
    ):
        lines.append(f"{idx}\t{fc}\t{lo}\t{up}")
    lines += [
        "# section: best",
        "Key\tValue",
        f"best_order\tARIMA({p},{d},{q})",
    ]
    (EXPECTED / "arima_trend_golden.tsv").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Best ARIMA order: ({p},{d},{q}) AICc={best['aicc']}")


if __name__ == "__main__":
    EXPECTED.mkdir(parents=True, exist_ok=True)
    write_regression_golden()
    write_arima_golden()
