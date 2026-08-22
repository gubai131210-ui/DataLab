#include "ui/analysis_commands.h"

#include <QtTest>

#include <QHash>
#include <QSet>
#include <QString>

#include <vector>

class UiMenuIaTrackTest final : public QObject {
    Q_OBJECT

private slots:
    void everyCommandHasPathAndGroup();
    void topLevelOnlyFourAnalysisMenus();
    void cascadeDepthIsOne();
    void waveAnchorsLandInExpectedGroups();
    void noDuplicateCommandIds();
};

void UiMenuIaTrackTest::everyCommandHasPathAndGroup()
{
    const auto& commands = analysis_commands::all();
    QVERIFY(commands.size() >= 130);
    for (const auto& command : commands) {
        QVERIFY2(!command.id.isEmpty(), "empty command id");
        QVERIFY2(
            !command.menu_path.trimmed().isEmpty(),
            qPrintable(QStringLiteral("empty menu_path: %1").arg(command.id)));
        QVERIFY2(
            !command.menu_group.trimmed().isEmpty(),
            qPrintable(QStringLiteral("empty menu_group: %1").arg(command.id)));
    }
}

void UiMenuIaTrackTest::topLevelOnlyFourAnalysisMenus()
{
    const QSet<QString> allowed = {
        QStringLiteral("统计"),
        QStringLiteral("控制图"),
        QStringLiteral("质量工具"),
        QStringLiteral("图形"),
    };
    for (const auto& command : analysis_commands::all()) {
        QVERIFY2(
            allowed.contains(command.menu_path),
            qPrintable(QStringLiteral("illegal top %1 for %2")
                           .arg(command.menu_path, command.id)));
    }
}

void UiMenuIaTrackTest::cascadeDepthIsOne()
{
    // Declarative model: top (menu_path) → one group → leaf. Group must not
    // itself contain "/" path segments that would imply a third level.
    for (const auto& command : analysis_commands::all()) {
        QVERIFY2(
            !command.menu_group.contains(QLatin1Char('>')),
            qPrintable(QStringLiteral("group looks nested: %1").arg(command.id)));
        QVERIFY2(
            command.menu_path.count(QLatin1Char('/')) == 0,
            qPrintable(QStringLiteral("path looks nested: %1").arg(command.id)));
    }
}

void UiMenuIaTrackTest::waveAnchorsLandInExpectedGroups()
{
    struct Anchor {
        const char* id;
        const char* path;
        const char* group;
    };
    const Anchor anchors[] = {
        {"cox_regression", "统计", "可靠性"},
        {"logistic_regression", "统计", "回归"},
        {"stepwise_regression", "统计", "回归"},
        {"nominal_logistic", "统计", "回归"},
        {"bootstrap_two_sample", "统计", "推断 / 仿真"},
        {"probit_reliability", "统计", "可靠性"},
        {"accelerated_life", "统计", "可靠性"},
        {"kmeans", "统计", "多变量"},
        {"cluster_observations", "统计", "多变量"},
        {"imr", "控制图", "计量图"},
        {"nonparametric_capability", "质量工具", "过程能力"},
        {"pareto", "质量工具", "质量图 / 规划"},
        {"histogram", "图形", "分布与单变量"},
        {"doe_factorial", "统计", "DOE"},
    };

    for (const Anchor& anchor : anchors) {
        const auto* command = analysis_commands::find(QString::fromUtf8(anchor.id));
        QVERIFY2(command != nullptr, anchor.id);
        QCOMPARE(command->menu_path, QString::fromUtf8(anchor.path));
        QCOMPARE(command->menu_group, QString::fromUtf8(anchor.group));
    }

    // 统计顶层不得出现「直接叶」：每个统计命令必须有非空 group（已在
    // everyCommandHasPathAndGroup 覆盖）；此处再断言统计叶子不扁平挂顶层。
    int flat_stat = 0;
    for (const auto& command : analysis_commands::all()) {
        if (command.menu_path == QStringLiteral("统计")
            && command.menu_group.trimmed().isEmpty()) {
            ++flat_stat;
        }
    }
    QCOMPARE(flat_stat, 0);
}

void UiMenuIaTrackTest::noDuplicateCommandIds()
{
    QSet<QString> seen;
    for (const auto& command : analysis_commands::all()) {
        QVERIFY2(!seen.contains(command.id), qPrintable(command.id));
        seen.insert(command.id);
    }
}

QTEST_MAIN(UiMenuIaTrackTest)
#include "ui_menu_ia_track_test.moc"
