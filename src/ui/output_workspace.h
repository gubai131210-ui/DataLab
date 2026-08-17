#pragma once

#include "domain/quality_types.h"

#include <QTabWidget>

#include <vector>

class AnalysisChartWidget;

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

private:
    bool eventFilter(QObject* watched, QEvent* event) override;
    QWidget* build_page_widget(const datalab::domain::OutputPage& page, std::size_t page_index);

    std::vector<datalab::domain::OutputPage> pages_;
};
