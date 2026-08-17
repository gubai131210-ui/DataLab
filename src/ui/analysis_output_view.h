#pragma once

#include "domain/quality_types.h"

#include <QTabWidget>

class AnalysisChartWidget;
class QTextEdit;

class AnalysisOutputView final : public QTabWidget {
    Q_OBJECT

public:
    explicit AnalysisOutputView(QWidget* parent = nullptr);

    void show_result(const datalab::domain::AnalysisResult& result);
    void set_source_rows(const std::vector<std::size_t>& rows);

signals:
    void rows_selected(const std::vector<std::size_t>& rows);

private:
    QTextEdit* summary_ = nullptr;
    AnalysisChartWidget* chart_ = nullptr;
};
