#include "domain/column_extract.h"
#include "domain/statistics/control_charts.h"
#include "domain/statistics/hypothesis_tests.h"
#include "domain/statistics/normal_distribution.h"
#include "domain/statistics/regression.h"

#include <QtTest/QtTest>
#include <cmath>
#include <limits>

using namespace datalab::domain::statistics;

class AnalyticsCorrectnessRegressionTest final : public QObject {
    Q_OBJECT
private slots:
    void cauchyQuantilesHaveNoArtificialBound();
    void normalTailRemainsPositiveAtNineSigma();
    void vifIsInvariantUnderTranslationAndScale();
    void longSameSideRunAndPhaseBoundary();
    void timestampsPreserveTimeAndRejectInvalidDates();
};

void AnalyticsCorrectnessRegressionTest::cauchyQuantilesHaveNoArtificialBound()
{
    const double pi = std::acos(-1.0);
    for (double probability : {0.0005, 0.9995}) {
        const double expected = std::tan(pi * (probability - 0.5));
        const double actual = student_t_quantile(probability, 1.0);
        QVERIFY(std::abs(actual / expected - 1.0) < 1.0e-10);
    }
    const double extreme = student_t_quantile(1.0e-200, 1.0);
    QVERIFY(std::isfinite(extreme));
    QVERIFY(std::abs(extreme / (-1.0 / (pi * 1.0e-200)) - 1.0) < 1.0e-12);
    const double near_limit = student_t_quantile(3.0e-309, 1.0);
    QVERIFY(std::isfinite(near_limit));
    QVERIFY(std::abs(near_limit / (-1.0 / (pi * 3.0e-309)) - 1.0) < 1.0e-12);
    QVERIFY(std::isnan(student_t_cdf(std::numeric_limits<double>::quiet_NaN(), 1.0)));
}

void AnalyticsCorrectnessRegressionTest::normalTailRemainsPositiveAtNineSigma()
{
    const double reference = 1.1285884059538324e-19;
    QVERIFY(std::abs(standard_normal_cdf(-9.0) / reference - 1.0) < 1.0e-12);
    QVERIFY(std::abs(expected_ppm_above(0.0, 1.0, 9.0) / (reference * 1.0e6) - 1.0) < 1.0e-12);
}

void AnalyticsCorrectnessRegressionTest::vifIsInvariantUnderTranslationAndScale()
{
    for (double shift : {0.0, 100.0}) {
        for (double scale : {1.0, 10.0}) {
            std::vector<std::vector<double>> x;
            for (double value : {-2.0, -1.0, 0.0, 1.0, 2.0}) {
                x.push_back({shift + scale * value});
            }
            const auto fit = fit_linear_regression({1.0, 2.0, 2.5, 4.0, 4.5}, x, {"X"});
            QCOMPARE(fit.coefficients.size(), std::size_t(2));
            QVERIFY(fit.coefficients[1].vif.has_value());
            QVERIFY(std::abs(*fit.coefficients[1].vif - 1.0) < 1.0e-8);
        }
    }
}

void AnalyticsCorrectnessRegressionTest::longSameSideRunAndPhaseBoundary()
{
    ControlChartResult chart;
    chart.plotted_values.assign(10000, 1.0);
    chart.center_line.assign(10000, 0.0);
    chart.lower_control_limit.assign(10000, -3.0);
    chart.upper_control_limit.assign(10000, 3.0);
    apply_special_cause_tests(chart, ControlChartKind::individuals, {{2}, "custom"});
    QCOMPARE(chart.special_cause_points[1].size(), std::size_t(10000));
    // A large retained capacity would reveal recording all overlapping prefixes.
    QVERIFY(chart.special_cause_points[1].capacity() < 40000);
    chart.plotted_values.resize(18);
    chart.center_line.resize(18);
    chart.lower_control_limit.resize(18);
    chart.upper_control_limit.resize(18);
    chart.phase_labels.assign(18, "A");
    for (std::size_t i = 9; i < 18; ++i) chart.phase_labels[i] = "B";
    apply_special_cause_tests(chart, ControlChartKind::individuals, {{2}, "custom"});
    QCOMPARE(chart.special_cause_points[1].size(), std::size_t(18));
}

void AnalyticsCorrectnessRegressionTest::timestampsPreserveTimeAndRejectInvalidDates()
{
    double first = 0.0;
    double second = 0.0;
    std::string label;
    QVERIFY(datalab::domain::parse_time_cell_to_epoch("2026-11-11T08:30:00Z", first, label));
    QVERIFY(datalab::domain::parse_time_cell_to_epoch("2026-11-11T09:30:00Z", second, label));
    QCOMPARE(second - first, 3600.0);
    QVERIFY(datalab::domain::parse_time_cell_to_epoch("2026-11-11T16:30:00+08:00", second, label));
    QCOMPARE(second, first);
    QVERIFY(!datalab::domain::parse_time_cell_to_epoch("2026-02-30 12:00:00", second, label));
    QVERIFY(!datalab::domain::parse_time_cell_to_epoch("2026-11-11T08:30:00 garbage", second, label));
}

QTEST_APPLESS_MAIN(AnalyticsCorrectnessRegressionTest)
#include "analytics_correctness_regression_test.moc"
