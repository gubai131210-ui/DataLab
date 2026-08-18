#include "ui/analysis_commands.h"
#include "ui/analysis_setup_dialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>

class AnalysisSetupDialogTest final : public QObject {
    Q_OBJECT

private slots:
    void inputSchemaCreatesTypedControls();
    void invalidSubmissionKeepsDialogOpen();
    void commandSchemaHasStableIdentifiers();
    void runButtonUsesAnalysisLabel();
};

void AnalysisSetupDialogTest::inputSchemaCreatesTypedControls()
{
    AnalysisSetupDialog dialog(QStringLiteral("测试"), {QStringLiteral("C1  Value")});
    analysis_commands::InputSpec number{
        QStringLiteral("confidence"), QStringLiteral("置信水平"), QStringLiteral("95")};
    analysis_commands::InputSpec integer{
        QStringLiteral("periods"), QStringLiteral("预测期数"), QStringLiteral("3")};
    dialog.add_input(number);
    dialog.add_input(integer);
    QCOMPARE(dialog.line_text(QStringLiteral("confidence")), QStringLiteral("95"));
    QCOMPARE(dialog.line_int(QStringLiteral("periods")), std::optional<int>{3});
    QVERIFY(dialog.findChild<QDoubleSpinBox*>(QStringLiteral("confidence")) != nullptr);
    QVERIFY(dialog.findChild<QSpinBox*>(QStringLiteral("periods")) != nullptr);
    QVERIFY(dialog.findChild<QDoubleSpinBox*>(QStringLiteral("confidence"))->minimumHeight() >= 34);
}

void AnalysisSetupDialogTest::invalidSubmissionKeepsDialogOpen()
{
    AnalysisSetupDialog dialog(QStringLiteral("测试"), {QStringLiteral("C1  Value")});
    dialog.add_role(QStringLiteral("variables"), QStringLiteral("变量"), false, false);
    bool called = false;
    dialog.set_accept_validator([&called](QString* title, QString* message) {
        called = true;
        *title = QStringLiteral("参数错误");
        *message = QStringLiteral("请修正输入。");
        return false;
    });
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    const auto* buttons = dialog.findChild<QDialogButtonBox*>();
    QVERIFY(buttons != nullptr);
    QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton);
    QVERIFY(!called);
    QVERIFY(dialog.isVisible());
    QVERIFY(dialog.findChild<QLabel*>(QStringLiteral("error_banner"))->isVisible());
    dialog.close();
}

void AnalysisSetupDialogTest::commandSchemaHasStableIdentifiers()
{
    QVERIFY(!analysis_commands::all().empty());
    for (const auto& command : analysis_commands::all()) {
        QVERIFY2(!command.id.isEmpty(), "analysis command id must not be empty");
        for (const auto& role : command.roles) {
            QVERIFY2(!role.id.isEmpty(), "analysis role id must not be empty");
        }
        for (const auto& input : command.inputs) {
            QVERIFY2(!input.id.isEmpty(), "analysis input id must not be empty");
        }
        if (command.id == QStringLiteral("imr")
            || command.id == QStringLiteral("xbar_r")
            || command.id == QStringLiteral("ewma")
            || command.id == QStringLiteral("cusum")) {
            bool has_tests = false;
            for (const auto& input : command.inputs) {
                if (input.id == QStringLiteral("tests")) {
                    has_tests = true;
                    QCOMPARE(input.kind, analysis_commands::InputKind::special_cause_tests);
                }
            }
            QVERIFY(has_tests);
        }
    }
}

void AnalysisSetupDialogTest::runButtonUsesAnalysisLabel()
{
    AnalysisSetupDialog dialog(QStringLiteral("测试"), {QStringLiteral("C1  Value")});
    const auto* buttons = dialog.findChild<QDialogButtonBox*>();
    QVERIFY(buttons != nullptr);
    QVERIFY(buttons->button(QDialogButtonBox::Ok) != nullptr);
    QCOMPARE(buttons->button(QDialogButtonBox::Ok)->text(), QStringLiteral("运行分析"));
    QVERIFY(buttons->button(QDialogButtonBox::Cancel) != nullptr);
    QCOMPARE(buttons->button(QDialogButtonBox::Cancel)->text(), QStringLiteral("取消"));
    QVERIFY(dialog.findChild<QPushButton*>(QStringLiteral("run_button")) != nullptr);
}

QTEST_MAIN(AnalysisSetupDialogTest)
#include "analysis_setup_dialog_test.moc"
