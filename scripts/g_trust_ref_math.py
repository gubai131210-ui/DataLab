#!/usr/bin/env python3
"""Shared float64 math helpers for G-Trust reference_implementation scripts.

Mirrors domain formulas used by hypothesis_tests / anderson_darling
(regularized incomplete beta via continued fraction; Stephens AD p-value).

Dependencies: Python 3.10+ stdlib only (math). NOT vendor_oracle.
"""

from __future__ import annotations

import math


def beta_continued_fraction(a: float, b: float, x: float) -> float:
    max_iterations = 200
    epsilon = 3.0e-14
    tiny = 1.0e-300
    qab = a + b
    qap = a + 1.0
    qam = a - 1.0
    c = 1.0
    d = 1.0 - qab * x / qap
    d = tiny if abs(d) < tiny else d
    d = 1.0 / d
    h = d
    for iteration in range(1, max_iterations + 1):
        m = float(iteration)
        m2 = 2.0 * m
        numerator = m * (b - m) * x / ((qam + m2) * (a + m2))
        d = 1.0 + numerator * d
        d = tiny if abs(d) < tiny else d
        c = 1.0 + numerator / c
        c = tiny if abs(c) < tiny else c
        d = 1.0 / d
        h *= d * c
        numerator = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2))
        d = 1.0 + numerator * d
        d = tiny if abs(d) < tiny else d
        c = 1.0 + numerator / c
        c = tiny if abs(c) < tiny else c
        d = 1.0 / d
        delta = d * c
        h *= delta
        if abs(delta - 1.0) < epsilon:
            break
    return h


def regularized_beta(x: float, a: float, b: float) -> float:
    if x <= 0.0:
        return 0.0
    if x >= 1.0:
        return 1.0
    logarithm = (
        math.lgamma(a + b)
        - math.lgamma(a)
        - math.lgamma(b)
        + a * math.log(x)
        + b * math.log1p(-x)
    )
    factor = math.exp(logarithm)
    if x < (a + 1.0) / (a + b + 2.0):
        return factor * beta_continued_fraction(a, b, x) / a
    return 1.0 - factor * beta_continued_fraction(b, a, 1.0 - x) / b


def student_t_cdf(value: float, degrees_of_freedom: float) -> float:
    if not (degrees_of_freedom > 0.0) or not math.isfinite(value):
        return 1.0 if value > 0.0 else 0.0
    if value == 0.0:
        return 0.5
    x = degrees_of_freedom / (degrees_of_freedom + value * value)
    beta = regularized_beta(x, degrees_of_freedom / 2.0, 0.5)
    return 1.0 - 0.5 * beta if value > 0.0 else 0.5 * beta


def student_t_quantile(probability: float, degrees_of_freedom: float) -> float:
    if not (0.0 < probability < 1.0) or not (degrees_of_freedom > 0.0):
        return float("nan")
    lower = -100.0
    upper = 100.0
    for _ in range(160):
        middle = 0.5 * (lower + upper)
        if student_t_cdf(middle, degrees_of_freedom) < probability:
            lower = middle
        else:
            upper = middle
    return 0.5 * (lower + upper)


def two_sided_t_p_value(statistic: float, degrees_of_freedom: float) -> float:
    return max(
        0.0,
        min(1.0, 1.0 - student_t_cdf(abs(statistic), degrees_of_freedom)),
    ) * 2.0


def f_right_tail(value: float, numerator_df: float, denominator_df: float) -> float:
    if not (value >= 0.0) or not (numerator_df > 0.0) or not (denominator_df > 0.0):
        return float("nan")
    x = denominator_df / (denominator_df + numerator_df * value)
    return regularized_beta(x, denominator_df / 2.0, numerator_df / 2.0)


def sample_mean(values: list[float]) -> float:
    return sum(values) / len(values)


def sample_stdev(values: list[float]) -> float:
    n = len(values)
    if n < 2:
        return 0.0
    mean = sample_mean(values)
    return math.sqrt(sum((v - mean) ** 2 for v in values) / (n - 1))


def normal_cdf(value: float, mean: float, sd: float) -> float:
    return 0.5 * (1.0 + math.erf((value - mean) / (sd * math.sqrt(2.0))))


def anderson_darling_adjusted(statistic: float, n: int) -> float:
    nn = float(n)
    return statistic * (1.0 + 0.75 / nn + 2.25 / (nn * nn))


def anderson_darling_p_value_normal(adjusted: float) -> float:
    if adjusted > 0.6:
        return math.exp(1.2937 - 5.709 * adjusted + 0.0186 * adjusted * adjusted)
    if adjusted > 0.34:
        return math.exp(0.9177 - 4.279 * adjusted - 1.38 * adjusted * adjusted)
    if adjusted > 0.2:
        return 1.0 - math.exp(-8.318 + 42.796 * adjusted - 59.938 * adjusted * adjusted)
    return 1.0 - math.exp(-13.436 + 101.14 * adjusted - 223.73 * adjusted * adjusted)


def anderson_darling_normal(values: list[float]) -> tuple[float, float, float]:
    """Return (A2, A2_star, p_value) for normal AD with estimated mean/sd."""
    ordered = sorted(values)
    n = len(ordered)
    mean = sample_mean(ordered)
    sd = sample_stdev(ordered)
    eps = 1.0e-12
    total = 0.0
    for index, value in enumerate(ordered):
        cdf = max(eps, min(1.0 - eps, normal_cdf(value, mean, sd)))
        left_weight = float(2 * index + 1)
        right_weight = float(2 * n + 1 - 2 * (index + 1))
        total += left_weight * math.log(cdf) + right_weight * math.log(1.0 - cdf)
    statistic = -float(n) - total / float(n)
    adjusted = anderson_darling_adjusted(statistic, n)
    p_value = max(0.0, min(1.0, anderson_darling_p_value_normal(adjusted)))
    return statistic, adjusted, p_value


# ASTM/AIAG d2 table (same as SpcConstants; n=2..25 subset used by scripts).
D2_TABLE = {
    2: 1.128,
    3: 1.693,
    4: 2.059,
    5: 2.326,
    6: 2.534,
    7: 2.704,
    8: 2.847,
    9: 2.970,
    10: 3.078,
}


def estimate_within_subgroup_sigma(subgroups: list[list[float]]) -> tuple[float, str]:
    """Match ControlCharts::estimate_within_subgroup_sigma (n<=8 → Rbar/d2)."""
    if len(subgroups) < 2:
        raise ValueError("need >=2 subgroups")
    n = len(subgroups[0])
    if n < 2 or any(len(g) != n for g in subgroups):
        raise ValueError("equal subgroup size >=2 required")
    if n <= 8:
        d2 = D2_TABLE[n]
        range_sum = sum(max(g) - min(g) for g in subgroups)
        return (range_sum / len(subgroups)) / d2, "R̄ / d2(n)"
    raise ValueError(f"subgroup size {n} > 8 not used in G-Trust fixtures")
