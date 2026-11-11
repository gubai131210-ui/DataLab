#pragma once

#include "domain/quality_types.h"

#include <QTabWidget>

#include <vector>
#include <functional>
#include <memory>

class AnalysisChartWidget;
class QLabel;
class QResizeEvent;

class OutputWorkspace final : public QTabWidget {
    Q_OBJECT

public:
    explicit OutputWorkspace(QWidget* parent = nullptr);
    ~OutputWorkspace() override;

    void add_page(const datalab::domain::OutputPage& page);
    void show_page(const QString& id);
    void clear_pages();
    std::vector<datalab::domain::OutputPage> pages() const;
    datalab::domain::OutputPage current_page() const;
    bool has_pages() const;
    void set_selected_source_rows(const std::vector<std::size_t>& rows);
    void set_source_fingerprint_provider(std::function<std::string()> provider);
    void invalidate_source_selection();
    AnalysisChartWidget* chart_for_copy() const;

signals:
    void rows_selected(const std::vector<std::size_t>& rows);
    void page_title_changed(const QString& id, const QString& title);
    void page_closed(const QString& id);
    void pages_changed();
    void copy_chart_requested();
    void formula_substitution_requested(const datalab::domain::OutputPage& page);
    void source_link_unavailable(const QString& message);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void close_page_at(int index);
    void update_empty_state();
    std::string active_source_fingerprint() const;
    bool page_matches_source(std::size_t index, const std::string& fingerprint) const;
    void refresh_source_link_labels(const std::string& fingerprint);

    std::vector<datalab::domain::OutputPage> pages_;
    QLabel* empty_label_ = nullptr;
    AnalysisChartWidget* last_focused_chart_ = nullptr;
    std::function<std::string()> source_fingerprint_provider_;
    bool source_context_dirty_ = true;
    // Child signal handlers may outlive our members during QWidget teardown.
    std::shared_ptr<char> callback_lifetime_ = std::make_shared<char>();
};
