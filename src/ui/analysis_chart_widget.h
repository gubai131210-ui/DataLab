#pragma once

#include "reporting/chart_model.h"
#include "reporting/chart_interaction.h"

#include <QWidget>

#include <QPoint>
#include <QRect>
#include <QSize>
#include <QTimer>

#include <optional>
#include <vector>

class GraphPropertiesPanel;
class QEvent;

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
    void set_row_visibility_summary(
        std::size_t excluded_count,
        std::size_t hidden_count,
        std::size_t analysis_n = 0,
        std::size_t display_n = 0);
    bool copy_to_clipboard();
signals:
    void rows_selected(const std::vector<std::size_t>& rows);
    void display_properties_changed(const ChartModel& model);
    void element_selected(const QString& path);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void apply_model_change(const ChartModel& model, bool record_undo);
    void undo_chart_edit();
    void redo_chart_edit();
    QPixmap render_chart_pixmap() const;
    void fit_to_window();
    void edit_graph();
    void open_full_editor();
    void enter_edit_mode(const QString& path);
    void leave_edit_mode();
    void save_graph();
    void copy_graph();
    void emit_selected_rows();
    void request_chart_update();
    QRect plot_bounds() const;
    QSize chart_render_size() const;
    std::optional<std::size_t> hit_test(const QPoint& position) const;

    ChartModel model_;
    QWidget* surface_ = nullptr;
    GraphPropertiesPanel* panel_ = nullptr;
    std::size_t excluded_count_ = 0;
    std::size_t hidden_count_ = 0;
    std::size_t analysis_n_ = 0;
    std::size_t display_n_ = 0;
    QPoint last_mouse_position_;
    QPoint selection_start_;
    QPoint selection_end_;
    bool panning_ = false;
    bool selecting_ = false;
    bool space_pressed_ = false;
    QTimer tooltip_timer_;
    QString pending_tooltip_;
    QPoint pending_tooltip_position_;
    std::vector<ChartModel> undo_history_;
    std::vector<ChartModel> redo_history_;
    bool applying_history_ = false;
};
