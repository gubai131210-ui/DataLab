#include "ui/page_renderer.h"
#include "ui/row_visibility_clipboard.h"

#include "ui/analysis_chart_widget.h"
#include "ui/report_table_model.h"
#include "ui/worksheet_sort_filter_proxy.h"
#include "reporting/chart_adapter.h"

#include <QAbstractItemView>
#include <QFile>
#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QShortcut>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableView>
#include <QTextStream>
#include <QVBoxLayout>

#include <set>
#include <memory>
#include <algorithm>
#include <utility>

namespace page_renderer {
namespace {

enum class PageContent { complete, charts, tables, diagnostics, method };

QString table_text(
    const datalab::domain::StatisticTable& table,
    QChar separator,
    const QString& visibility_footnote = {})
{
    QStringList lines;
    auto encode = [separator](const std::string& value) {
        QString text = QString::fromStdString(value);
        if (text.contains(separator) || text.contains('"') || text.contains('\n')) {
            text.replace('"', QStringLiteral("\"\""));
            text = QStringLiteral("\"") + text + QStringLiteral("\"");
        }
        return text;
    };
    QStringList headers;
    for (const auto& header : table.headers) {
        headers.push_back(encode(header));
    }
    lines.push_back(headers.join(separator));
    for (const auto& row : table.rows) {
        QStringList cells;
        for (std::size_t column = 0; column < table.headers.size(); ++column) {
            cells.push_back(encode(column < row.size() ? row[column] : std::string()));
        }
        lines.push_back(cells.join(separator));
    }
    if (!visibility_footnote.isEmpty()) {
        return datalab::ui::append_clipboard_footnote_comments(
            lines.join(QLatin1Char('\n')), visibility_footnote);
    }
    return lines.join(QLatin1Char('\n'));
}

datalab::domain::StatisticTable selected_table_rows(
    const datalab::domain::StatisticTable& table,
    const std::vector<int>& row_indexes)
{
    datalab::domain::StatisticTable selected;
    selected.title = table.title;
    selected.headers = table.headers;
    for (const int row_index : row_indexes) {
        if (row_index >= 0 && static_cast<std::size_t>(row_index) < table.rows.size()) {
            selected.rows.push_back(table.rows[static_cast<std::size_t>(row_index)]);
        }
    }
    return selected;
}

}  // namespace

QString icon_resource(const std::string& analysis_id)
{
    const QString candidate =
        QStringLiteral(":/icons/%1.svg").arg(QString::fromStdString(analysis_id));
    return QFile::exists(candidate) ? candidate : QStringLiteral(":/icons/report.svg");
}

static QWidget* build_page_content(
    const datalab::domain::OutputPage& page,
    QWidget* parent,
    const PageRenderOptions& options,
    const PageContent content,
    const std::optional<std::size_t> table_index = std::nullopt)
{
    auto* container = new QWidget(parent);
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(22, 18, 22, 24);
    layout->setSpacing(14);

    if (content == PageContent::complete) {
    auto* title_row = new QHBoxLayout();
    auto* title_icon = new QLabel(container);
    title_icon->setPixmap(QIcon(icon_resource(page.id)).pixmap(28, 28));
    title_row->addWidget(title_icon);
    auto* title = new QLabel(QString::fromStdString(page.title), container);
    title->setStyleSheet(QStringLiteral(
        "font-size: 21px; font-weight: 700; color: #20343d; padding: 2px 0 0;"));
    title_row->addWidget(title);
    title_row->addStretch();
    if (options.on_formula_substitution) {
        auto* formula_button = new QPushButton(QStringLiteral("公式代入"), container);
        QObject::connect(formula_button, &QPushButton::clicked, container,
                         [callback = options.on_formula_substitution, page]() {
                             callback(page);
                         });
        title_row->addWidget(formula_button);
    }
    layout->addLayout(title_row);
    }

    if ((content == PageContent::complete || content == PageContent::method)
        && options.include_method) {
        auto* method_card = new QFrame(container);
        method_card->setStyleSheet(QStringLiteral(
            "QFrame { background:#f3f8f9; border:1px solid #d8e7e9; border-radius:8px; }"
            "QLabel { color:#526a73; padding:2px 0; }"));
        auto* method_layout = new QVBoxLayout(method_card);
        method_layout->setContentsMargins(12, 9, 12, 9);
        auto* method = new QLabel(
            QStringLiteral("<b>%1</b><br/>%2")
                .arg(QString::fromStdString(page.method_name).toHtmlEscaped(),
                     QString::fromStdString(page.parameter_summary).toHtmlEscaped()),
            method_card);
        method->setTextFormat(Qt::RichText);
        method->setWordWrap(true);
        method_layout->addWidget(method);
        layout->addWidget(method_card);
    } else if ((content == PageContent::complete || content == PageContent::method)
               && !page.parameter_summary.empty()) {
        auto* summary = new QLabel(
            QString::fromStdString(page.parameter_summary), container);
        summary->setWordWrap(true);
        summary->setStyleSheet(QStringLiteral(
            "background:#f3f8f9; color:#526a73; border:1px solid #d8e7e9;"
            " border-radius:8px; padding:10px;"));
        layout->addWidget(summary);
    }

    const std::size_t excluded_rows = page.configuration.excluded_rows.size();
    const std::size_t hidden_rows = page.configuration.hidden_rows.size();
    std::size_t analysis_n = 0;
    std::size_t display_n = 0;
    if (page.facts.eda.has_value()) {
        analysis_n = page.facts.eda->analysis_n != 0
            ? page.facts.eda->analysis_n
            : page.facts.eda->analysis_eligible_n;
        display_n = page.facts.eda->n != 0 ? page.facts.eda->n : display_n;
    }
    if (display_n == 0 && !page.plots.empty()) {
        display_n = page.plots.front().source_rows.size();
    }
    const QString visibility_footnote = datalab::ui::row_visibility_footnote(
        excluded_rows, hidden_rows, analysis_n, display_n);

    if (content == PageContent::complete || content == PageContent::diagnostics) {
    for (const auto& section : page.interpretation) {
        auto* card = new QLabel(container);
        QString text = QStringLiteral("【%1】\n").arg(QString::fromStdString(section.heading));
        for (const auto& bullet : section.bullets) {
            text += QStringLiteral("• ") + QString::fromStdString(bullet) + QLatin1Char('\n');
        }
        card->setText(text.trimmed());
        card->setWordWrap(true);
        const QString color = section.severity == datalab::domain::DiagnosticMessage::Severity::error
            ? QStringLiteral("#fff0f0")
            : section.severity == datalab::domain::DiagnosticMessage::Severity::warning
                ? QStringLiteral("#fff8e6") : QStringLiteral("#f4f6f8");
        card->setStyleSheet(QStringLiteral(
            "background:%1; color:#49636d; padding:11px 13px; border:1px solid #d6e1e5;"
            " border-radius:8px;").arg(color));
        layout->addWidget(card);
    }
    }

    if (content == PageContent::complete || content == PageContent::tables) {
    for (std::size_t current_table = 0; current_table < page.tables.size(); ++current_table) {
        if (table_index.has_value() && *table_index != current_table) {
            continue;
        }
        const auto& table = page.tables[current_table];
        auto* caption = new QLabel(QString::fromStdString(table.title), container);
        caption->setStyleSheet(QStringLiteral("font-weight: 600; margin-top: 8px;"));
        layout->addWidget(caption);
        auto* model = new datalab::ui::ReportTableModel(container);
        model->set_table(table);
        auto* proxy = new datalab::ui::WorksheetSortFilterProxyModel(container);
        proxy->setSourceModel(model);
        std::set<int> sortable;
        for (int column = 0; column < static_cast<int>(table.headers.size()); ++column) {
            sortable.insert(column);
        }
        proxy->set_sortable_columns(sortable);
        auto* grid = new QTableView(container);
        grid->setModel(proxy);
        grid->setSortingEnabled(true);
        grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
        grid->setSelectionMode(QAbstractItemView::ContiguousSelection);
        grid->setSelectionBehavior(QAbstractItemView::SelectRows);
        grid->setContextMenuPolicy(Qt::ActionsContextMenu);
        const auto copy_table_text = [grid, table, proxy, visibility_footnote]() {
            std::vector<int> selected_rows;
            if (grid->selectionModel() != nullptr) {
                for (const QModelIndex& proxy_index : grid->selectionModel()->selectedRows()) {
                    selected_rows.push_back(proxy->mapToSource(proxy_index).row());
                }
            }
            const datalab::domain::StatisticTable payload =
                selected_rows.empty() ? table : selected_table_rows(table, selected_rows);
            QApplication::clipboard()->setText(
                table_text(payload, QLatin1Char('\t'), visibility_footnote));
        };
        auto* copy_table = new QAction(QStringLiteral("复制表格（TSV）"), grid);
        QObject::connect(copy_table, &QAction::triggered, grid, copy_table_text);
        grid->addAction(copy_table);
        auto* copy_shortcut = new QShortcut(QKeySequence::Copy, grid);
        copy_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(copy_shortcut, &QShortcut::activated, grid, copy_table_text);
        auto* export_table = new QAction(QStringLiteral("导出表格（CSV）"), grid);
        QObject::connect(export_table, &QAction::triggered, grid, [table, grid, visibility_footnote] {
            const QString path = QFileDialog::getSaveFileName(
                grid, QStringLiteral("导出统计表"),
                QString::fromStdString(table.title) + QStringLiteral(".csv"),
                QStringLiteral("CSV 文件 (*.csv)"));
            if (path.isEmpty()) {
                return;
            }
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream.setEncoding(QStringConverter::Utf8);
                stream << table_text(table, QLatin1Char(','), visibility_footnote);
            }
        });
        grid->addAction(export_table);
        grid->setWordWrap(true);
        grid->setShowGrid(false);
        grid->setMinimumHeight(42);
        grid->verticalHeader()->setDefaultSectionSize(28);
        grid->verticalHeader()->setVisible(false);
        grid->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        grid->setAlternatingRowColors(true);
        grid->setStyleSheet(QStringLiteral(
            "QTableView { background:#ffffff; alternate-background-color:#f6fafb;"
            " gridline-color:#dbe6e9; border:1px solid #d6e1e5; }"
            "QHeaderView::section { background:#e7f0f3; color:#29434e;"
            " border:0; border-right:1px solid #d6e1e5; padding:7px; font-weight:600; }"));
        grid->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        grid->horizontalHeader()->setMinimumSectionSize(64);
        grid->horizontalHeader()->setMaximumSectionSize(220);
        grid->horizontalHeader()->setStretchLastSection(false);
        if (options.interactive_charts && options.on_rows_selected) {
            QObject::connect(
                grid->selectionModel(),
                &QItemSelectionModel::selectionChanged,
                grid,
                [grid, model, proxy, callback = options.on_rows_selected]() {
                    std::vector<std::size_t> rows;
                    for (const QModelIndex& proxy_index :
                         grid->selectionModel()->selectedRows()) {
                        const QModelIndex index = proxy->mapToSource(proxy_index);
                        const QVariant row_id =
                            model->data(index, datalab::ui::ReportRowIdRole);
                        if (row_id.isValid()) {
                            rows.push_back(static_cast<std::size_t>(row_id.toULongLong()));
                        }
                    }
                    callback(rows);
                });
        }
        grid->resizeColumnsToContents();
        if (options.interactive_charts) {
            grid->horizontalHeader()->setResizeContentsPrecision(100);
            grid->setWordWrap(false);
            grid->setMinimumHeight(160);
        } else {
            grid->resizeRowsToContents();
            grid->setMinimumHeight(24 * static_cast<int>(table.rows.size() + 2));
        }
        layout->addWidget(grid);
    }
    }

    if ((content == PageContent::complete || content == PageContent::diagnostics)
        && !page.diagnostics.empty()) {
        auto* caption = new QLabel(QStringLiteral("诊断信息"), container);
        caption->setStyleSheet(QStringLiteral("font-weight: 600;"));
        layout->addWidget(caption);
        for (const auto& diagnostic : page.diagnostics) {
            auto* diagnostic_row = new QHBoxLayout();
            auto* diagnostic_icon = new QLabel(container);
            const QString icon = diagnostic.severity
                == datalab::domain::DiagnosticMessage::Severity::error
                ? QStringLiteral("error")
                : diagnostic.severity
                    == datalab::domain::DiagnosticMessage::Severity::warning
                    ? QStringLiteral("warning") : QStringLiteral("success");
            diagnostic_icon->setPixmap(
                QIcon(QStringLiteral(":/icons/%1.svg").arg(icon)).pixmap(20, 20));
            diagnostic_row->addWidget(diagnostic_icon);
            auto* message = new QLabel(QString::fromStdString(diagnostic.message), container);
            message->setWordWrap(true);
            const QString background =
                diagnostic.severity == datalab::domain::DiagnosticMessage::Severity::error
                    ? QStringLiteral("#fff0f0")
                    : diagnostic.severity == datalab::domain::DiagnosticMessage::Severity::warning
                        ? QStringLiteral("#fff8e6") : QStringLiteral("#eef8f3");
            message->setStyleSheet(QStringLiteral(
                "background:%1; color:#4d626a; border:1px solid #d6e1e5;"
                " border-radius:6px; padding:8px 10px;").arg(background));
            diagnostic_row->addWidget(message, 1);
            layout->addLayout(diagnostic_row);
        }
    }

    if (content == PageContent::complete || content == PageContent::charts) {
    QGridLayout* plot_grid = nullptr;
    if (page.method_name == "Capability Sixpack") {
        plot_grid = new QGridLayout();
        layout->addLayout(plot_grid);
    }
    for (std::size_t plot_index = 0; plot_index < page.plots.size(); ++plot_index) {
        const auto& plot = page.plots[plot_index];
        auto* chart = new AnalysisChartWidget(container);
        ChartModel model = chart_model_from_plot(plot);
        model.language_tag = options.chart_language_tag;
        chart->set_model(model);
        chart->set_row_visibility_summary(excluded_rows, hidden_rows, analysis_n, display_n);
        chart->setMinimumHeight(280);
        chart->setStyleSheet(QStringLiteral(
            "background:#ffffff; border:1px solid #d6e1e5; border-radius:8px;"));
        if (options.interactive_charts) {
            if (options.on_rows_selected) {
                QObject::connect(chart, &AnalysisChartWidget::rows_selected, chart,
                                 [handler = options.on_rows_selected](
                                     const std::vector<std::size_t>& rows) {
                                     handler(rows);
                                 });
            }
            if (options.on_display_properties_changed) {
                QObject::connect(
                    chart, &AnalysisChartWidget::display_properties_changed, chart,
                    [handler = options.on_display_properties_changed,
                     plot_index](const ChartModel& model) {
                        handler(plot_index, model);
                    });
            }
        }
        if (plot_grid != nullptr) {
            plot_grid->addWidget(chart, static_cast<int>(plot_index / 2),
                                 static_cast<int>(plot_index % 2));
        } else {
            layout->addWidget(chart, options.interactive_charts && page.plots.size() == 1 ? 1 : 0);
        }
    }
    }
    if (content != PageContent::tables
        && !(content == PageContent::charts && page.plots.size() == 1)) {
        layout->addStretch();
    }
    return container;
}

QWidget* build_page_widget(
    const datalab::domain::OutputPage& page,
    QWidget* parent,
    const PageRenderOptions& options)
{
    if (!options.interactive_charts) {
        return build_page_content(page, parent, options, PageContent::complete);
    }
    // The same immutable result feeds lazy interactive sections and complete reports.
    const auto result = std::make_shared<const datalab::domain::OutputPage>(page);
    auto* container = new QWidget(parent);
    container->setObjectName(QStringLiteral("interactive_analysis_page"));
    auto* layout = new QVBoxLayout(container);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(8);
    auto* heading = new QHBoxLayout();
    auto* title = new QLabel(QString::fromStdString(page.title), container);
    title->setTextFormat(Qt::PlainText);
    title->setWordWrap(true);
    QFont title_font = title->font();
    title_font.setPointSize(13);
    title_font.setBold(true);
    title->setFont(title_font);
    heading->addWidget(title, 1);
    if (options.on_formula_substitution) {
        auto* formula = new QPushButton(QStringLiteral("公式代入"), container);
        QObject::connect(formula, &QPushButton::clicked, container,
            [result, callback = options.on_formula_substitution] { callback(*result); });
        heading->addWidget(formula);
    }
    layout->addLayout(heading);
    QStringList summary;
    if (page.method_metadata.valid_count > 0) {
        summary << QStringLiteral("有效 N = %1").arg(
            static_cast<qulonglong>(page.method_metadata.valid_count));
    }
    if (page.facts.capability.has_value()) {
        const auto& facts = *page.facts.capability;
        for (const auto& metric : {std::make_pair("Cp", facts.cp),
                                  std::make_pair("Cpk", facts.cpk),
                                  std::make_pair("Ppk", facts.ppk)}) {
            if (metric.second.has_value()) {
                summary << QStringLiteral("%1 = %2").arg(
                    QString::fromLatin1(metric.first), QString::number(*metric.second, 'g', 6));
            }
        }
    }
    if (!summary.isEmpty()) {
        auto* metrics = new QLabel(summary.join(QStringLiteral("     ")), container);
        metrics->setTextFormat(Qt::PlainText);
        metrics->setWordWrap(true);
        layout->addWidget(metrics);
    }
    auto* sections = new QTabWidget(container);
    sections->setObjectName(QStringLiteral("analysis_result_sections"));
    sections->setDocumentMode(true);
    const auto add_section = [sections](const QString& name) {
        auto* host = new QWidget(sections);
        auto* host_layout = new QVBoxLayout(host);
        host_layout->setContentsMargins(0, 0, 0, 0);
        sections->addTab(host, name);
        return host;
    };
    if (!page.plots.empty()) {
        QWidget* charts = add_section(QStringLiteral("图表"));
        auto* scroll = new QScrollArea(charts);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidget(build_page_content(*result, scroll, options, PageContent::charts));
        charts->layout()->addWidget(scroll);
    }
    if (!page.tables.empty()) {
        QWidget* statistics = add_section(QStringLiteral("统计结果"));
        auto* selector = new QComboBox(statistics);
        selector->setObjectName(QStringLiteral("result_table_selector"));
        auto* stack = new QStackedWidget(statistics);
        for (const auto& table : page.tables) {
            selector->addItem(QString::fromStdString(table.title));
            auto* host = new QWidget(stack);
            auto* host_layout = new QVBoxLayout(host);
            host_layout->setContentsMargins(0, 0, 0, 0);
            stack->addWidget(host);
        }
        statistics->layout()->addWidget(selector);
        statistics->layout()->addWidget(stack);
        const auto show_table = [result, stack, options](int index) {
            if (index < 0 || index >= stack->count()) {
                return;
            }
            QWidget* host = stack->widget(index);
            if (!host->property("result_built").toBool()) {
                host->layout()->addWidget(build_page_content(
                    *result, host, options, PageContent::tables, static_cast<std::size_t>(index)));
                host->setProperty("result_built", true);
            }
            stack->setCurrentIndex(index);
        };
        QObject::connect(selector, &QComboBox::currentIndexChanged, statistics, show_table);
        QObject::connect(sections, &QTabWidget::currentChanged, statistics,
            [sections, statistics, selector, show_table](int) {
                if (sections->currentWidget() == statistics) {
                    show_table(selector->currentIndex());
                }
            });
        if (sections->currentWidget() == statistics) {
            show_table(0);
        }
    }
    int diagnostics_index = -1;
    if (!page.diagnostics.empty() || !page.interpretation.empty()) {
        QWidget* diagnostics = add_section(QStringLiteral("解读与诊断"));
        diagnostics_index = sections->indexOf(diagnostics);
        auto* scroll = new QScrollArea(diagnostics);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidget(build_page_content(*result, scroll, options, PageContent::diagnostics));
        diagnostics->layout()->addWidget(scroll);
    }
    QWidget* method = add_section(QStringLiteral("方法与参数"));
    auto* method_scroll = new QScrollArea(method);
    method_scroll->setWidgetResizable(true);
    method_scroll->setFrameShape(QFrame::NoFrame);
    method_scroll->setWidget(build_page_content(*result, method_scroll, options, PageContent::method));
    method->layout()->addWidget(method_scroll);
    QStringList warnings;
    QStringList errors;
    for (const auto& diagnostic : page.diagnostics) {
        if (diagnostic.severity != datalab::domain::DiagnosticMessage::Severity::info) {
            auto& messages = diagnostic.severity == datalab::domain::DiagnosticMessage::Severity::error
                ? errors : warnings;
            messages << QString::fromStdString(diagnostic.message);
        }
    }
    for (const auto& interpretation : page.interpretation) {
        if (interpretation.severity != datalab::domain::DiagnosticMessage::Severity::info) {
            for (const auto& bullet : interpretation.bullets) {
                auto& messages = interpretation.severity == datalab::domain::DiagnosticMessage::Severity::error
                    ? errors : warnings;
                messages << QString::fromStdString(bullet);
            }
        }
    }
    warnings.removeDuplicates();
    errors.removeDuplicates();
    const QStringList messages = errors + warnings;
    if (!messages.isEmpty()) {
        auto* warning_row = new QHBoxLayout();
        auto* banner = new QLabel(QStringLiteral("错误 %1 · 警告 %2：%3")
            .arg(errors.size()).arg(warnings.size()).arg(messages.front().simplified().left(180)), container);
        banner->setTextFormat(Qt::PlainText);
        banner->setWordWrap(true);
        banner->setObjectName(QStringLiteral("analysis_warning_summary"));
        banner->setToolTip(messages.join(QLatin1Char('\n')));
        banner->setStyleSheet(errors.isEmpty()
            ? QStringLiteral("padding:6px; color:#8a5a00; background:#fff8e6; border:0;")
            : QStringLiteral("padding:6px; color:#b3261e; background:#fff0f0; border:0;"));
        auto* details = new QPushButton(QStringLiteral("查看详情"), container);
        QObject::connect(details, &QPushButton::clicked, sections,
            [sections, diagnostics_index] { sections->setCurrentIndex(diagnostics_index); });
        warning_row->addWidget(banner, 1);
        warning_row->addWidget(details);
        layout->addLayout(warning_row);
    }
    layout->addWidget(sections, 1);
    return container;
}

}  // namespace page_renderer
