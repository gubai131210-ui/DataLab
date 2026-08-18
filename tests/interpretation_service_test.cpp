#include "application/interpretation_service.h"

#include <QtTest/QtTest>

#include <algorithm>

class InterpretationServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void usesStructuredCapabilityFacts();
    void usesStructuredSpcFacts();
};

void InterpretationServiceTest::usesStructuredCapabilityFacts()
{
    datalab::domain::OutputPage page;
    page.method_name = "Capability";
    page.facts.capability = datalab::domain::CapabilityFacts{1.10, 1.42};

    datalab::application::InterpretationService::enrich(page);

    QVERIFY(!page.interpretation.empty());
    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(conclusion->bullets[0].find("Cpk = 1.100000") != std::string::npos);
    QVERIFY(conclusion->bullets[1].find("Ppk = 1.420000") != std::string::npos);
}

void InterpretationServiceTest::usesStructuredSpcFacts()
{
    datalab::domain::OutputPage page;
    page.method_name = "Xbar-R Chart";
    page.facts.spc = datalab::domain::SpcFacts{3, 1.25};

    datalab::application::InterpretationService::enrich(page);

    QVERIFY(!page.interpretation.empty());
    const auto conclusion = std::find_if(
        page.interpretation.cbegin(), page.interpretation.cend(),
        [](const auto& section) { return section.heading == "统计结论"; });
    QVERIFY(conclusion != page.interpretation.cend());
    QVERIFY(conclusion->bullets[0].find("3 个控制图超限点") != std::string::npos);
    QVERIFY(conclusion->bullets[1].find("Sigma Z = 1.250000") != std::string::npos);
}

QTEST_MAIN(InterpretationServiceTest)
#include "interpretation_service_test.moc"
