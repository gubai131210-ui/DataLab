cmake_minimum_required(VERSION 3.19)
project(DataLab LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Sql Svg Test Concurrent)

qt_standard_project_setup()

# 分层目标（依赖方向：ui → {application, infrastructure, reporting} → domain）
# 注意：include 根统一为 src/，分层由链接依赖 + tools/check_layering.ps1 约束。

# ---------------------------------------------------------------------------
# domain：纯 C++ 统计与领域模型，不依赖 Qt
# ---------------------------------------------------------------------------
add_library(datalab_domain STATIC
    src/domain/column_extract.cpp
    src/domain/column_extract.h
    src/domain/graph_assembly.cpp
    src/domain/graph_assembly.h
    src/domain/quality_types.h
    src/domain/quality_diagnostics.cpp
    src/domain/quality_diagnostics.h
    src/domain/statistics/descriptive_statistics.cpp
    src/domain/statistics/descriptive_statistics.h
    src/domain/statistics/process_capability.cpp
    src/domain/statistics/process_capability.h
    src/domain/statistics/control_charts.cpp
    src/domain/statistics/control_charts.h
    src/domain/statistics/quality_visuals.cpp
    src/domain/statistics/normal_probability.cpp
    src/domain/statistics/normality_test.cpp
    src/domain/statistics/quality_visuals.h
    src/domain/statistics/normality_test.h
    src/domain/statistics/spc_constants.cpp
    src/domain/statistics/spc_constants.h
    src/domain/statistics/normal_distribution.cpp
    src/domain/statistics/normal_distribution.h
    src/domain/statistics/correlation.cpp
    src/domain/statistics/correlation.h
    src/domain/statistics/hypothesis_tests.cpp
    src/domain/statistics/hypothesis_tests.h
    src/domain/statistics/inference_extensions.cpp
    src/domain/statistics/inference_extensions.h
    src/domain/statistics/regression.cpp
    src/domain/statistics/regression.h
    src/domain/statistics/two_factor_anova.cpp
    src/domain/statistics/two_factor_anova.h
    src/domain/statistics/box_cox.cpp
    src/domain/statistics/box_cox.h
    src/domain/statistics/gage_rr.cpp
    src/domain/statistics/gage_rr.h
    src/domain/statistics/msa_type1.cpp
    src/domain/statistics/msa_type1.h
    src/domain/statistics/reliability.cpp
    src/domain/statistics/reliability.h
    src/domain/statistics/t_power.cpp
    src/domain/statistics/t_power.h
    src/domain/statistics/nonparametric_tests.cpp
    src/domain/statistics/nonparametric_tests.h
    src/domain/statistics/time_series.cpp
    src/domain/statistics/time_series.h
    src/domain/statistics/arima.cpp
    src/domain/statistics/arima.h
    src/domain/statistics/logistic_regression.cpp
    src/domain/statistics/logistic_regression.h
    src/domain/statistics/variance_tests.cpp
    src/domain/statistics/variance_tests.h
    src/domain/statistics/time_series_decomposition.cpp
    src/domain/statistics/time_series_decomposition.h
    src/domain/statistics/doe_factorial.cpp
    src/domain/statistics/doe_factorial.h
    src/domain/statistics/response_optimization.cpp
    src/domain/statistics/response_optimization.h
    src/domain/statistics/nested_gage_rr.cpp
    src/domain/statistics/nested_gage_rr.h
    src/domain/statistics/attribute_agreement.cpp
    src/domain/statistics/attribute_agreement.h
    src/domain/statistics/seasonal_forecasting.cpp
    src/domain/statistics/seasonal_forecasting.h
    src/domain/statistics/pca.cpp
    src/domain/statistics/pca.h
    src/domain/statistics/graph_visuals.cpp
    src/domain/statistics/graph_visuals.h
)

target_include_directories(datalab_domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---------------------------------------------------------------------------
# application：分析编排（AnalysisService / InterpretationService / 构建器）
# ---------------------------------------------------------------------------
add_library(datalab_application STATIC
    src/application/analysis_intent.cpp
    src/application/analysis_intent.h
    src/application/analysis_service.cpp
    src/application/analysis_service.h
    src/application/interpretation_service.cpp
    src/application/interpretation_service.h
    src/application/output_builder.cpp
    src/application/output_builder.h
    src/application/column_assembly.cpp
    src/application/column_assembly.h
    src/application/chart_pages.cpp
    src/application/chart_pages.h
    src/application/graph_service.cpp
    src/application/graph_service.h
    src/application/doe_pages.cpp
    src/application/doe_pages.h
)

target_include_directories(datalab_application PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(datalab_application
    PUBLIC
        datalab_domain
)

# ---------------------------------------------------------------------------
# reporting：图表渲染（纯 QPainter，无 QWidget 依赖；ui 与 infrastructure 共用）
# ---------------------------------------------------------------------------
add_library(datalab_reporting STATIC
    src/reporting/chart_model.h
    src/reporting/chart_geometry.h
    src/reporting/chart_renderer.cpp
    src/reporting/chart_renderer.h
    src/reporting/chart_adapter.cpp
    src/reporting/chart_adapter.h
    src/reporting/chart_coordinate_mapper.cpp
    src/reporting/chart_coordinate_mapper.h
)

target_include_directories(datalab_reporting PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(datalab_reporting
    PUBLIC
        datalab_domain
        Qt6::Gui
)

# ---------------------------------------------------------------------------
# infrastructure：CSV/SQLite/PDF/Python 桥等外部依赖适配
# ---------------------------------------------------------------------------
add_library(datalab_infrastructure STATIC
    src/infrastructure/csv_importer.cpp
    src/infrastructure/csv_importer.h
    src/infrastructure/data_import_service.cpp
    src/infrastructure/data_import_service.h
    src/infrastructure/project_repository.cpp
    src/infrastructure/project_repository.h
    src/infrastructure/output_serialization.cpp
    src/infrastructure/output_serialization.h
    src/infrastructure/pdf_report_writer.cpp
    src/infrastructure/pdf_report_writer.h
    src/infrastructure/report_layout_cursor.cpp
    src/infrastructure/report_layout_cursor.h
    src/infrastructure/report_painter.cpp
    src/infrastructure/report_painter.h
    src/infrastructure/python_table_importer.cpp
    src/infrastructure/python_table_importer.h
)

target_include_directories(datalab_infrastructure PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(datalab_infrastructure
    PUBLIC
        datalab_domain
        Qt6::Core
        Qt6::Gui
    PRIVATE
        datalab_reporting
        Qt6::Sql
)

# ---------------------------------------------------------------------------
# ui：Qt Widgets 界面（含 mainwindow）
# ---------------------------------------------------------------------------
add_library(datalab_ui STATIC
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/mainwindow.ui
    src/ui/analysis_chart_widget.cpp
    src/ui/analysis_chart_widget.h
    src/ui/analysis_commands.cpp
    src/ui/analysis_commands.h
    src/ui/analysis_setup_dialog.cpp
    src/ui/analysis_setup_dialog.h
    src/ui/output_workspace.cpp
    src/ui/output_workspace.h
    src/ui/page_renderer.cpp
    src/ui/page_renderer.h
    src/ui/command_registry.cpp
    src/ui/command_registry.h
    src/ui/project_navigator.cpp
    src/ui/project_navigator.h
    src/ui/worksheet_model.cpp
    src/ui/worksheet_model.h
    src/ui/worksheet_view.cpp
    src/ui/worksheet_view.h
    src/ui/report_preview_dialog.cpp
    src/ui/report_preview_dialog.h
    src/ui/graph_properties_dialog.cpp
    src/ui/graph_properties_dialog.h
)

target_include_directories(datalab_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(datalab_ui
    PUBLIC
        datalab_domain
        Qt6::Widgets
    PRIVATE
        datalab_application
        datalab_infrastructure
        datalab_reporting
        Qt6::Concurrent
)

set(DATALAB_ICON_FILES
    resources/icons/app-mark.svg
    resources/icons/new.svg
    resources/icons/open.svg
    resources/icons/save.svg
    resources/icons/import.svg
    resources/icons/undo.svg
    resources/icons/redo.svg
    resources/icons/run-analysis.svg
    resources/icons/data-table.svg
    resources/icons/report.svg
    resources/icons/export-pdf.svg
    resources/icons/settings.svg
    resources/icons/warning.svg
    resources/icons/error.svg
    resources/icons/success.svg
    resources/icons/descriptive.svg
    resources/icons/normality_test.svg
    resources/icons/correlation.svg
    resources/icons/one_sample_t.svg
    resources/icons/two_sample_t.svg
    resources/icons/paired_t.svg
    resources/icons/one_way_anova.svg
    resources/icons/two_factor_anova.svg
    resources/icons/chi_square.svg
    resources/icons/two_proportions.svg
    resources/icons/mann_whitney.svg
    resources/icons/wilcoxon_signed_rank.svg
    resources/icons/kruskal_wallis.svg
    resources/icons/regression.svg
    resources/icons/logistic_regression.svg
    resources/icons/box_cox.svg
    resources/icons/pca.svg
    resources/icons/imr.svg
    resources/icons/xbar_r.svg
    resources/icons/xbar_s.svg
    resources/icons/p_chart.svg
    resources/icons/np_chart.svg
    resources/icons/c_chart.svg
    resources/icons/u_chart.svg
    resources/icons/laney_p_chart.svg
    resources/icons/laney_u_chart.svg
    resources/icons/ewma.svg
    resources/icons/cusum.svg
    resources/icons/time_series_smoothing.svg
    resources/icons/time_series_decomposition.svg
    resources/icons/seasonal_forecasting.svg
    resources/icons/arima.svg
    resources/icons/doe_factorial.svg
    resources/icons/gage_rr.svg
    resources/icons/nested_gage_rr.svg
    resources/icons/attribute_agreement.svg
    resources/icons/variance_test.svg
    resources/icons/capability.svg
    resources/icons/capability_sixpack.svg
    resources/icons/histogram.svg
    resources/icons/boxplot.svg
    resources/icons/pareto.svg
    resources/icons/scatter.svg
    resources/icons/interval.svg
    resources/icons/bubble.svg
    resources/icons/probability.svg
    resources/icons/ecdf.svg
    resources/icons/matrix.svg
    resources/icons/marginal.svg
    resources/icons/parallel.svg
    resources/icons/heatmap.svg
    resources/icons/time_series.svg
    resources/icons/area.svg
    resources/icons/contour.svg
    resources/icons/pie.svg
    resources/icons/import-data.svg
    resources/icons/export-report.svg
)

# ---------------------------------------------------------------------------
# 可执行目标
# ---------------------------------------------------------------------------
qt_add_executable(DataLab
    WIN32 MACOSX_BUNDLE
    main.cpp
    translations/translations.qrc
)

qt_add_resources(DataLab app_resources
    PREFIX "/"
    BASE resources
    FILES ${DATALAB_ICON_FILES}
)

qt_add_resources(DataLab import_script_resource
    PREFIX "/"
    BASE .
    FILES
        tools/import_table.py
)

target_link_libraries(DataLab
    PRIVATE
        datalab_ui
        Qt6::Widgets
        Qt6::Svg
)

# ---------------------------------------------------------------------------
# 测试
# ---------------------------------------------------------------------------
enable_testing()

function(add_datalab_test name source)
    qt_add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE ${ARGN} Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()
add_datalab_test(descriptive_statistics_test tests/descriptive_statistics_test.cpp datalab_domain)

add_datalab_test(column_extract_import_test tests/column_extract_import_test.cpp datalab_domain datalab_infrastructure)

add_datalab_test(quality_statistics_test tests/quality_statistics_test.cpp datalab_domain datalab_application)

add_datalab_test(reliability_msa_power_test tests/reliability_msa_power_test.cpp datalab_domain datalab_application)

add_datalab_test(doe_response_test tests/doe_response_test.cpp datalab_domain)

add_datalab_test(graph_visuals_test tests/graph_visuals_test.cpp datalab_domain)
add_datalab_test(graph_service_test tests/graph_service_test.cpp datalab_application)

qt_add_executable(minitab_fixture_test
    tests/minitab_fixture_test.cpp
)

target_link_libraries(minitab_fixture_test
    PRIVATE
        datalab_infrastructure
        Qt6::Test
)
target_compile_definitions(minitab_fixture_test
    PRIVATE
        DATALAB_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME minitab_fixture_test COMMAND minitab_fixture_test)

add_datalab_test(chart_geometry_test tests/chart_geometry_test.cpp datalab_reporting)

add_datalab_test(
    chart_model_roundtrip_test
    tests/chart_model_roundtrip_test.cpp
    datalab_reporting
)

add_datalab_test(pdf_layout_test tests/pdf_layout_test.cpp datalab_infrastructure datalab_reporting)

qt_add_executable(minitab_formula_golden_test
    tests/minitab_formula_golden_test.cpp
)

target_link_libraries(minitab_formula_golden_test
    PRIVATE
        datalab_application
        datalab_infrastructure
        Qt6::Test
)
target_compile_definitions(minitab_formula_golden_test
    PRIVATE
        DATALAB_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME minitab_formula_golden_test COMMAND minitab_formula_golden_test)

add_datalab_test(worksheet_model_test tests/worksheet_model_test.cpp datalab_ui)

add_datalab_test(output_serialization_test tests/output_serialization_test.cpp datalab_infrastructure)

add_datalab_test(quality_diagnostics_test tests/quality_diagnostics_test.cpp datalab_domain datalab_infrastructure)

add_datalab_test(
    interpretation_service_test
    tests/interpretation_service_test.cpp
    datalab_application
)

add_datalab_test(
    analysis_intent_test
    tests/analysis_intent_test.cpp
    datalab_ui
)

add_datalab_test(
    analysis_setup_dialog_test
    tests/analysis_setup_dialog_test.cpp
    datalab_ui
)
set_tests_properties(
    analysis_setup_dialog_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_datalab_test(
    special_cause_rule_test
    tests/special_cause_rule_test.cpp
    datalab_ui
)
set_tests_properties(
    special_cause_rule_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_datalab_test(
    python_table_importer_test
    tests/python_table_importer_test.cpp
    datalab_infrastructure
)

add_datalab_test(
    project_repository_test
    tests/project_repository_test.cpp
    datalab_infrastructure
    Qt6::Sql
)

qt_add_executable(icon_resource_test
    tests/icon_resource_test.cpp
)

qt_add_resources(icon_resource_test icon_resources
    PREFIX "/"
    BASE resources
    FILES ${DATALAB_ICON_FILES}
)

target_link_libraries(icon_resource_test
    PRIVATE
        Qt6::Widgets
        Qt6::Svg
        Qt6::Test
)

add_test(NAME icon_resource_test COMMAND icon_resource_test)

install(TARGETS DataLab
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET DataLab
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
