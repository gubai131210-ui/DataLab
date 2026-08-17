#pragma once

#include "reporting/chart_model.h"

#include <QWidget>

#include <QPoint>

#include <optional>
#include <vector>

class AnalysisChartWidget final : public QWidget {
    Q_OBJECT

public:
    explicit AnalysisChartWidget(QWidget* parent = nullptr);

    void set_data(
        const std::vector<double>& values,
        const std::vector<double>& center,
        const std::vector<double>& lower,
        const std::vector<double>& upper);
    void set_model(const ChartModel& model);
    void set_source_rows(const std::vector<std::size_t>& rows);
    void set_selected_source_rows(const std::vector<std::size_t>& rows);
    const std::vector<std::size_t>& selected_rows() const;

signals:
    void rows_selected(const std::vector<std::size_t>& rows);
    void display_properties_changed(const ChartModel& model);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void fit_to_window();
    void edit_graph();
    void save_graph();
    void copy_graph();
    void emit_selected_rows();
    std::optional<std::size_t> hit_test(const QPoint& position) const;

    ChartModel model_;
    QPoint last_mouse_position_;
    QPoint selection_start_;
    QPoint selection_end_;
    bool panning_ = false;
    bool selecting_ = false;
    bool space_pressed_ = false;
};
