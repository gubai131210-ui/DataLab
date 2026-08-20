#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QtTest/QtTest>

class IconResourceTest final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void loadsAllBundledIcons();
};

void IconResourceTest::initTestCase()
{
    Q_INIT_RESOURCE(icon_resources);
}

void IconResourceTest::loadsAllBundledIcons()
{
    const QStringList names = {
        QStringLiteral("app-mark"), QStringLiteral("new"), QStringLiteral("open"),
        QStringLiteral("save"), QStringLiteral("import"), QStringLiteral("undo"),
        QStringLiteral("redo"), QStringLiteral("run-analysis"),
        QStringLiteral("data-table"), QStringLiteral("report"),
        QStringLiteral("export-pdf"), QStringLiteral("settings"),
        QStringLiteral("warning"), QStringLiteral("error"), QStringLiteral("success"),
        QStringLiteral("descriptive"), QStringLiteral("normality_test"),
        QStringLiteral("outlier_test"),
        QStringLiteral("correlation"), QStringLiteral("one_sample_t"),
        QStringLiteral("two_sample_t"), QStringLiteral("one_way_anova"),
        QStringLiteral("paired_t"), QStringLiteral("regression"),
        QStringLiteral("two_proportions"), QStringLiteral("one_proportion"),
        QStringLiteral("one_poisson_rate"), QStringLiteral("two_poisson_rate"),
        QStringLiteral("equivalence_t"), QStringLiteral("chi_square"),
        QStringLiteral("mann_whitney"), QStringLiteral("wilcoxon_signed_rank"),
        QStringLiteral("kruskal_wallis"), QStringLiteral("box_cox"),
        QStringLiteral("gage_rr"), QStringLiteral("imr"), QStringLiteral("xbar_r"),
        QStringLiteral("xbar_s"), QStringLiteral("imr_rs"), QStringLiteral("p_chart"), QStringLiteral("np_chart"),
        QStringLiteral("c_chart"), QStringLiteral("u_chart"),
        QStringLiteral("laney_p_chart"), QStringLiteral("laney_u_chart"),
        QStringLiteral("ewma"), QStringLiteral("cusum"),
        QStringLiteral("time_series_smoothing"), QStringLiteral("arima"),
        QStringLiteral("two_factor_anova"), QStringLiteral("logistic_regression"),
        QStringLiteral("variance_test"), QStringLiteral("time_series_decomposition"),
        QStringLiteral("seasonal_forecasting"), QStringLiteral("pca"),
        QStringLiteral("doe_factorial"), QStringLiteral("nested_gage_rr"),
        QStringLiteral("attribute_agreement"), QStringLiteral("capability"),
        QStringLiteral("capability_sixpack"), QStringLiteral("histogram"),
        QStringLiteral("boxplot"), QStringLiteral("pareto"),
        QStringLiteral("scatter"), QStringLiteral("interval"),
        QStringLiteral("bubble"), QStringLiteral("probability"),
        QStringLiteral("ecdf"), QStringLiteral("matrix"),
        QStringLiteral("marginal"), QStringLiteral("parallel"),
        QStringLiteral("heatmap"), QStringLiteral("time_series"),
        QStringLiteral("area"), QStringLiteral("contour"),
        QStringLiteral("pie"),
        QStringLiteral("import-data"), QStringLiteral("export-report")};
    for (const QString& name : names) {
        const QString path = QStringLiteral(":/icons/%1.svg").arg(name);
        QVERIFY2(QFile::exists(path), qPrintable(path));
        QVERIFY2(!QIcon(path).isNull(), qPrintable(path));
    }
}

QTEST_MAIN(IconResourceTest)

#include "icon_resource_test.moc"
