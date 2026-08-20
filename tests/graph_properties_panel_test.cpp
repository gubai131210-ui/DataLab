#include "ui/graph_properties_panel.h"

#include <QAbstractScrollArea>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTest>

class GraphPropertiesPanelTest final : public QObject {
    Q_OBJECT

private slots:
    void controlPanelShowsReferenceObjects();
    void scatterPanelHidesReferenceObjects();
    void panelHasScrollableContentAndFixedActions();
    void selectsSeriesFromPath();
    void writesSelectedSeriesColorAndWidth();
    void writesSelectedSeriesLineAndPointStyle();
    void writesAutoScaleAndFontSizes();
};

ChartModel make_model(const ChartKind kind)
{
    ChartModel model;
    model.kind = kind;
    model.values = {1.0, 2.0, 3.0};
    model.x_values = {1.0, 2.0, 3.0};
    if (kind == ChartKind::Control) {
        model.center = {2.0, 2.0, 2.0};
        model.lower = {1.0, 1.0, 1.0};
        model.upper = {3.0, 3.0, 3.0};
    }
    return model;
}

void GraphPropertiesPanelTest::controlPanelShowsReferenceObjects()
{
    GraphPropertiesPanel panel(make_model(ChartKind::Control));
    const auto* objects = panel.findChild<QListWidget*>(
        QStringLiteral("graph_object_list"));
    QVERIFY(objects != nullptr);
    QVERIFY(objects->findItems(QStringLiteral("控制线 / CL"), Qt::MatchExactly).size() == 1);
    QVERIFY(objects->findItems(QStringLiteral("控制线 / LCL"), Qt::MatchExactly).size() == 1);
    QVERIFY(objects->findItems(QStringLiteral("控制线 / UCL"), Qt::MatchExactly).size() == 1);
}

void GraphPropertiesPanelTest::scatterPanelHidesReferenceObjects()
{
    GraphPropertiesPanel panel(make_model(ChartKind::Scatter));
    const auto* objects = panel.findChild<QListWidget*>(
        QStringLiteral("graph_object_list"));
    QVERIFY(objects != nullptr);
    QVERIFY(objects->findItems(QStringLiteral("控制线 / CL"), Qt::MatchExactly).empty());
}

void GraphPropertiesPanelTest::panelHasScrollableContentAndFixedActions()
{
    GraphPropertiesPanel panel(make_model(ChartKind::Scatter));
    const auto* scroll = panel.findChild<QScrollArea*>(
        QStringLiteral("graph_properties_scroll"));
    QVERIFY(scroll != nullptr);
    QVERIFY(scroll->widgetResizable());
    QVERIFY(panel.findChild<QPushButton*>(QStringLiteral("close_panel")) != nullptr);
    QVERIFY(panel.findChild<QPushButton*>(QStringLiteral("restore_defaults")) != nullptr);
    QVERIFY(panel.findChild<QPushButton*>(QStringLiteral("apply_changes")) != nullptr);
}

void GraphPropertiesPanelTest::selectsSeriesFromPath()
{
    ChartModel model = make_model(ChartKind::Scatter);
    ChartSeries fitted;
    fitted.label = QStringLiteral("拟合");
    ChartSeries unlabeled;
    model.series.push_back(fitted);
    model.series.push_back(unlabeled);
    GraphPropertiesPanel panel(model);
    const auto* objects = panel.findChild<QListWidget*>(
        QStringLiteral("graph_object_list"));
    QVERIFY(objects != nullptr);
    QVERIFY(objects->findItems(QStringLiteral("数据系列 / 拟合"), Qt::MatchExactly).size() == 1);
    QVERIFY(objects->findItems(QStringLiteral("数据系列 / 系列 2"), Qt::MatchExactly).size() == 1);
    panel.set_selected_path(QStringLiteral("图形 > 数据系列 > 拟合"));
    QVERIFY(objects->currentItem() != nullptr);
    QVERIFY(objects->currentItem()->text().contains(QStringLiteral("拟合")));
    const auto* path = panel.findChild<QLabel*>(QStringLiteral("selection_path"));
    QVERIFY(path != nullptr);
    QCOMPARE(path->text(), QStringLiteral("图形 > 数据系列 > 拟合"));
}

void GraphPropertiesPanelTest::writesSelectedSeriesColorAndWidth()
{
    ChartModel model = make_model(ChartKind::Scatter);
    ChartSeries fitted;
    fitted.label = QStringLiteral("拟合");
    fitted.style.color = QStringLiteral("#455a64");
    fitted.style.line_width = 1.8;
    fitted.line_width = 1.8;
    ChartSeries actual;
    actual.label = QStringLiteral("观测");
    actual.style.color = QStringLiteral("#1b6b93");
    actual.style.line_width = 2.0;
    actual.line_width = 2.0;
    model.series.push_back(fitted);
    model.series.push_back(actual);
    GraphPropertiesPanel panel(model);
    panel.set_selected_path(QStringLiteral("图形 > 数据系列 > 拟合"));
    auto* width = panel.findChild<QDoubleSpinBox*>(QStringLiteral("series_line_width"));
    QVERIFY(width != nullptr);
    QVERIFY(width->isVisibleTo(&panel));
    width->setValue(4.5);
    QCOMPARE(panel.model().series[0].style.line_width, 4.5);
    QCOMPARE(panel.model().series[0].line_width, 4.5);
    QCOMPARE(panel.model().series[1].style.line_width, 2.0);
    QCOMPARE(panel.model().series[1].line_width, 2.0);

    auto* color = panel.findChild<QPushButton*>(QStringLiteral("series_color"));
    QVERIFY(color != nullptr);
    color->setProperty("chartColor", QStringLiteral("#ff0000"));
    auto* apply = panel.findChild<QPushButton*>(QStringLiteral("apply_changes"));
    QVERIFY(apply != nullptr);
    QTest::mouseClick(apply, Qt::LeftButton);
    QCOMPARE(panel.model().series[0].style.color, QStringLiteral("#ff0000"));
    QCOMPARE(panel.model().series[1].style.color, QStringLiteral("#1b6b93"));

    panel.set_selected_path(QStringLiteral("图形"));
    QVERIFY(!width->isVisibleTo(&panel));
}

void GraphPropertiesPanelTest::writesSelectedSeriesLineAndPointStyle()
{
    ChartModel model = make_model(ChartKind::Scatter);
    ChartSeries fitted;
    fitted.label = QStringLiteral("拟合");
    fitted.style.line_style = ChartLineStyle::Solid;
    fitted.style.point_style = ChartPointStyle::None;
    fitted.show_points = false;
    ChartSeries actual;
    actual.label = QStringLiteral("观测");
    actual.style.line_style = ChartLineStyle::Dash;
    actual.style.point_style = ChartPointStyle::Circle;
    actual.show_points = true;
    model.series.push_back(fitted);
    model.series.push_back(actual);
    GraphPropertiesPanel panel(model);
    panel.set_selected_path(QStringLiteral("图形 > 数据系列 > 拟合"));
    auto* line_style = panel.findChild<QComboBox*>(QStringLiteral("series_line_style"));
    auto* point_style = panel.findChild<QComboBox*>(QStringLiteral("series_point_style"));
    QVERIFY(line_style != nullptr);
    QVERIFY(point_style != nullptr);
    QVERIFY(line_style->isVisibleTo(&panel));
    line_style->setCurrentIndex(2);
    point_style->setCurrentIndex(2);
    QCOMPARE(panel.model().series[0].style.line_style, ChartLineStyle::Dot);
    QCOMPARE(panel.model().series[0].style.point_style, ChartPointStyle::Square);
    QVERIFY(panel.model().series[0].show_points);
    QCOMPARE(panel.model().series[1].style.line_style, ChartLineStyle::Dash);
    QCOMPARE(panel.model().series[1].style.point_style, ChartPointStyle::Circle);

    panel.set_selected_path(QStringLiteral("图形"));
    auto* width = panel.findChild<QDoubleSpinBox*>(QStringLiteral("default_line_width"));
    QVERIFY(width != nullptr);
    width->setValue(3.0);
    QCOMPARE(panel.model().line_width, 3.0);
    QCOMPARE(panel.model().series[0].style.line_style, ChartLineStyle::Dot);
    QCOMPARE(panel.model().series[1].style.line_style, ChartLineStyle::Dash);
}

void GraphPropertiesPanelTest::writesAutoScaleAndFontSizes()
{
    ChartModel model = make_model(ChartKind::Scatter);
    model.x_min = 1.0;
    model.y_max = 9.0;
    model.title_font_size = 11;
    model.axis_font_size = 9;
    model.legend_font_size = 8;
    GraphPropertiesPanel panel(model);
    auto* x_min_auto = panel.findChild<QCheckBox*>(QStringLiteral("x_min_auto"));
    auto* y_max_auto = panel.findChild<QCheckBox*>(QStringLiteral("y_max_auto"));
    auto* title_font = panel.findChild<QSpinBox*>(QStringLiteral("title_font_size"));
    QVERIFY(x_min_auto != nullptr);
    QVERIFY(y_max_auto != nullptr);
    QVERIFY(title_font != nullptr);
    QVERIFY(!x_min_auto->isChecked());
    QVERIFY(!y_max_auto->isChecked());
    x_min_auto->setChecked(true);
    QVERIFY(!panel.model().x_min.has_value());
    auto* y_max = panel.findChild<QDoubleSpinBox*>(QStringLiteral("y_max"));
    QVERIFY(y_max != nullptr);
    y_max->setValue(12.5);
    QCOMPARE(panel.model().y_max.value_or(0.0), 12.5);
    title_font->setValue(16);
    QCOMPARE(panel.model().title_font_size, 16);
    QCOMPARE(panel.model().theme_preset, QStringLiteral("default"));
}

QTEST_MAIN(GraphPropertiesPanelTest)
#include "graph_properties_panel_test.moc"
