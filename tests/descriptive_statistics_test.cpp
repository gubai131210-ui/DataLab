#include "domain/statistics/descriptive_statistics.h"

#include <QtTest/QtTest>

#include <cmath>
#include <vector>

using datalab::domain::statistics::DescriptiveStatistics;

class DescriptiveStatisticsTest final : public QObject {
    Q_OBJECT

private slots:
    void calculatesMeanAndRange();
    void calculatesPopulationAndSampleStandardDeviation();
    void calculatesStandardErrorOfMean();
    void rejectsEmptyInput();
};

void DescriptiveStatisticsTest::calculatesMeanAndRange()
{
    const auto result = DescriptiveStatistics::calculate({1.0, 2.0, 3.0, 4.0});

    QVERIFY(result.has_value());
    QCOMPARE(result->count, std::size_t{4});
    QCOMPARE(result->mean, 2.5);
    QCOMPARE(result->minimum, 1.0);
    QCOMPARE(result->maximum, 4.0);
    QCOMPARE(result->median, 2.5);
    QCOMPARE(result->first_quartile, 1.75);
    QCOMPARE(result->third_quartile, 3.25);
}

void DescriptiveStatisticsTest::calculatesPopulationAndSampleStandardDeviation()
{
    const auto result = DescriptiveStatistics::calculate({1.0, 2.0, 3.0, 4.0});

    QVERIFY(result.has_value());
    QVERIFY(result->sample_standard_deviation.has_value());
    QVERIFY(qFuzzyCompare(result->population_standard_deviation, std::sqrt(1.25)));
    QVERIFY(qFuzzyCompare(*result->sample_standard_deviation, std::sqrt(5.0 / 3.0)));
}

void DescriptiveStatisticsTest::calculatesStandardErrorOfMean()
{
    const auto result = DescriptiveStatistics::calculate({1.0, 2.0, 3.0, 4.0});

    QVERIFY(result.has_value());
    QVERIFY(result->standard_error_of_mean.has_value());
    QVERIFY(qFuzzyCompare(*result->standard_error_of_mean,
                          std::sqrt(5.0 / 3.0) / 2.0));
    QCOMPARE(result->missing_count, std::size_t{0});
}

void DescriptiveStatisticsTest::rejectsEmptyInput()
{
    QVERIFY(!DescriptiveStatistics::calculate({}).has_value());
}

QTEST_APPLESS_MAIN(DescriptiveStatisticsTest)

#include "descriptive_statistics_test.moc"
