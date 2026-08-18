#pragma once

#include "domain/quality_types.h"

#include <QTabWidget>

#include <vector>

class AnalysisChartWidget;
class QLabel;
class QResizeEvent;

class OutputWorkspace final : public QTabWidget {
    Q_OBJECT

public:
    explicit OutputWorkspace(QWidget* parent = nullptr);

    void add_page(const datalab::domain::OutputPage& page);
    void show_page(const QString& id);
    void clear_pages();
    std::vector<datalab::domain::OutputPage> pages() const;
    datalab::domain::OutputPage current_page() const;
    bool has_pages() const;
    void set_selected_source_rows(const std::vector<std::size_t>& rows);

signals:
    void rows_selected(const std::vector<std::size_t>& rows);
    void page_title_changed(const QString& id, const QString& title);
    void page_closed(const QString& id);
    void pages_changed();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void close_page_at(int index);
    void update_empty_state();

    std::vector<datalab::domain::OutputPage> pages_;
    QLabel* empty_label_ = nullptr;
};
