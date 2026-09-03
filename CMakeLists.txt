cmake_minimum_required(VERSION 3.19)
project(DataLab LANGUAGES CXX C)

# MinGW cc1plus may crash with no diagnostic when %TEMP%/%TMP% lives under a
# non-ASCII user profile path (common on zh-CN Windows). Route compiler temps
# into the ASCII build directory instead.
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    configure_file(
        "${CMAKE_SOURCE_DIR}/cmake/compile_with_ascii_tmp.cmd.in"
        "${CMAKE_BINARY_DIR}/compile_with_ascii_tmp.cmd"
        @ONLY
    )
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_BINARY_DIR}/compile_with_ascii_tmp.cmd")
    set(CMAKE_C_COMPILER_LAUNCHER "${CMAKE_BINARY_DIR}/compile_with_ascii_tmp.cmd")
    # Large TUs (e.g. analysis_service.cpp) exceed default COFF section limits.
    add_compile_options(-Wa,-mbig-obj)
    message(STATUS "MinGW ASCII compiler temp: ${CMAKE_BINARY_DIR}/.compiler_tmp")
    message(STATUS "MinGW big object format: -Wa,-mbig-obj")
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Sql Svg Test Concurrent Network)

qt_standard_project_setup()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MES_FOR_Q_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Mes_For_Q")
if(EXISTS "${MES_FOR_Q_DIR}/CMakeLists.txt")
    set(MES_FOR_Q_BUILD_APP OFF CACHE BOOL "Build Mes_For_Q standalone app" FORCE)
    add_subdirectory("${MES_FOR_Q_DIR}" mes_for_q_build EXCLUDE_FROM_ALL)
    set(DATALAB_HAS_MES_CORE TRUE)
    message(STATUS "MES integration: mes_core from ${MES_FOR_Q_DIR}")
else()
    message(STATUS "MES integration: Mes_For_Q not found, skipping mes_core")
endif()

# 分层目标（依赖方向：ui → {application, infrastructure, reporting} → domain）
# 注意：include 根统一为 src/，分层由链接依赖 + tools/check_layering.ps1 约束。

# ---------------------------------------------------------------------------
# domain：纯 C++ 统计与领域模型，不依赖 Qt
# ---------------------------------------------------------------------------
add_library(datalab_domain STATIC
    src/domain/column_extract.cpp
    src/domain/column_extract.h
    src/domain/graph_assembly.cpp
    src/domain/row_visibility.cpp
    src/domain/row_visibility.h
    src/domain/graph_assembly.h
    src/domain/quality_types.h
    src/domain/report_types.h
    src/domain/report_text_catalog.cpp
    src/domain/report_text_catalog.h
    src/domain/report_text_catalog_part1.cpp
    src/domain/report_text_catalog_part2.cpp
    src/domain/report_text_catalog_part3.cpp
    src/domain/report_text_catalog_part4.cpp
    src/domain/report_text_catalog_part5.cpp
    src/domain/report_text_catalog_part6.cpp
    src/domain/report_text_catalog_part7.cpp
    src/domain/report_text_catalog_part8.cpp
    src/domain/report_text_catalog_part9.cpp
    src/domain/report_text_catalog_part10.cpp
    src/domain/report_text_catalog_part11.cpp
    src/domain/report_text_catalog_part12.cpp
    src/domain/report_text_catalog_part13.cpp
    src/domain/report_text_catalog_part14.cpp
    src/domain/report_text_catalog_part15.cpp
    src/domain/report_text_catalog_part16.cpp
    src/domain/report_text_catalog_part17.cpp
    src/domain/report_text_catalog_part18.cpp
    src/domain/report_text_catalog_part19.cpp
    src/domain/report_text_catalog_part20.cpp
    src/domain/report_text_catalog_part21.cpp
    src/domain/report_text_catalog_part22.cpp
    src/domain/report_text_catalog_part23.cpp
    src/domain/report_text_catalog_part24.cpp
    src/domain/report_text_catalog_part25.cpp
    src/domain/report_text_catalog_part26.cpp
    src/domain/report_text_catalog_part27.cpp
    src/domain/report_text_catalog_part28.cpp
    src/domain/report_text_catalog_part29.cpp
    src/domain/quality_diagnostics.cpp
    src/domain/quality_diagnostics.h
    src/domain/statistics/descriptive_statistics.cpp
    src/domain/statistics/descriptive_statistics.h
    src/domain/statistics/process_capability.cpp
    src/domain/statistics/process_capability.h
    src/domain/statistics/hartigan_dip.cpp
    src/domain/statistics/hartigan_dip.h
    src/domain/statistics/gaussian_mixture_2.cpp
    src/domain/statistics/gaussian_mixture_2.h
    src/domain/statistics/attribute_capability.cpp
    src/domain/statistics/attribute_capability.h
    src/domain/statistics/johnson_transform.cpp
    src/domain/statistics/johnson_transform.h
    src/domain/statistics/control_charts.cpp
    src/domain/statistics/control_charts.h
    src/domain/statistics/special_cause_rule_catalog.cpp
    src/domain/statistics/special_cause_rule_catalog.h
    src/domain/database_types.h
    src/domain/database/database_provider.h
    src/domain/database/database_provider_registry.cpp
    src/domain/database/import_plan_validation.cpp
    src/domain/database/keyset_sql.cpp
    src/domain/database/keyset_sql.h
    src/domain/statistics/multivariate_control.cpp
    src/domain/statistics/multivariate_control.h
    src/domain/statistics/quality_visuals.cpp
    src/domain/statistics/normal_probability.cpp
    src/domain/statistics/normality_test.cpp
    src/domain/statistics/anderson_darling.cpp
    src/domain/statistics/anderson_darling.h
    src/domain/statistics/distribution_identification.cpp
    src/domain/statistics/distribution_identification.h
    src/domain/statistics/quality_visuals.h
    src/domain/statistics/normality_test.h
    src/domain/statistics/spc_constants.cpp
    src/domain/statistics/spc_constants.h
    src/domain/statistics/normal_distribution.cpp
    src/domain/statistics/normal_distribution.h
    src/domain/statistics/quality_extensions.cpp
    src/domain/statistics/quality_extensions.h
    src/domain/statistics/correlation.cpp
    src/domain/statistics/correlation.h
    src/domain/statistics/hypothesis_tests.cpp
    src/domain/statistics/hypothesis_tests.h
    src/domain/statistics/inference_extensions.cpp
    src/domain/statistics/inference_extensions.h
    src/domain/statistics/analysis_rules.cpp
    src/domain/statistics/analysis_rules.h
    src/domain/statistics/regression.cpp
    src/domain/statistics/regression.h
    src/domain/statistics/two_factor_anova.cpp
    src/domain/statistics/two_factor_anova.h
    src/domain/statistics/box_cox.cpp
    src/domain/statistics/box_cox.h
    src/domain/statistics/gage_rr.cpp
    src/domain/statistics/expanded_gage_rr.cpp
    src/domain/statistics/expanded_gage_rr.h
    src/domain/statistics/gage_rr.h
    src/domain/statistics/msa_type1.cpp
    src/domain/statistics/msa_type1.h
    src/domain/statistics/reliability.cpp
    src/domain/statistics/reliability.h
    src/domain/statistics/t_power.cpp
    src/domain/statistics/t_power.h
    src/domain/statistics/autocorrelation.cpp
    src/domain/statistics/autocorrelation.h
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
    src/domain/statistics/trend_analysis.cpp
    src/domain/statistics/trend_analysis.h
    src/domain/statistics/doe_factorial.cpp
    src/domain/statistics/doe_factorial.h
    src/domain/statistics/doe_d_optimal.cpp
    src/domain/statistics/doe_d_optimal.h
    src/domain/statistics/rsm_analysis.cpp
    src/domain/statistics/rsm_analysis.h
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
    src/domain/statistics/kmeans.cpp
    src/domain/statistics/kmeans.h
    src/domain/statistics/cart_tree.cpp
    src/domain/statistics/cart_tree.h
    src/domain/statistics/random_forest.cpp
    src/domain/statistics/random_forest.h
    src/domain/statistics/weibayes.cpp
    src/domain/statistics/weibayes.h
    src/domain/statistics/taguchi_orthogonal.cpp
    src/domain/statistics/taguchi_orthogonal.h
    src/domain/statistics/distribution_calculator.cpp
    src/domain/statistics/distribution_calculator.h
    src/domain/statistics/taguchi_analyze.cpp
    src/domain/statistics/taguchi_analyze.h
    src/domain/statistics/mixture_design.cpp
    src/domain/statistics/mixture_design.h
    src/domain/statistics/nhpp_repairable.cpp
    src/domain/statistics/nhpp_repairable.h
    src/domain/statistics/reliability_test_plan.cpp
    src/domain/statistics/reliability_test_plan.h
    src/domain/statistics/mixture_analyze.cpp
    src/domain/statistics/mixture_analyze.h
    src/domain/statistics/glm_two_way.cpp
    src/domain/statistics/glm_two_way.h
    src/domain/statistics/analyze_variability.cpp
    src/domain/statistics/analyze_variability.h
    src/domain/statistics/factor_analysis.cpp
    src/domain/statistics/factor_analysis.h
    src/domain/statistics/binary_response_doe.cpp
    src/domain/statistics/binary_response_doe.h
    src/domain/statistics/cluster_variables.cpp
    src/domain/statistics/cluster_variables.h
    src/domain/statistics/glm_three_factor.cpp
    src/domain/statistics/glm_three_factor.h
    src/domain/statistics/life_data_regression.cpp
    src/domain/statistics/life_data_regression.h
    src/domain/statistics/expanded_gage_unbalanced.cpp
    src/domain/statistics/expanded_gage_unbalanced.h
    src/domain/statistics/split_plot_analyze.cpp
    src/domain/statistics/split_plot_analyze.h
    src/domain/statistics/mixture_process_variable.cpp
    src/domain/statistics/mixture_process_variable.h
    src/domain/statistics/manova_one_way.cpp
    src/domain/statistics/manova_one_way.h
    src/domain/statistics/general_manova.cpp
    src/domain/statistics/general_manova.h
    src/domain/statistics/mixed_effects_reml.cpp
    src/domain/statistics/mixed_effects_reml.h
    src/domain/statistics/binary_doe_probit.cpp
    src/domain/statistics/binary_doe_probit.h
    src/domain/statistics/life_data_lognormal.cpp
    src/domain/statistics/life_data_lognormal.h
    src/domain/statistics/simple_correspondence.cpp
    src/domain/statistics/simple_correspondence.h
    src/domain/statistics/multiple_correspondence.cpp
    src/domain/statistics/multiple_correspondence.h
    src/domain/statistics/nonlinear_regression.cpp
    src/domain/statistics/nonlinear_regression.h
    src/domain/statistics/split_plot_design.cpp
    src/domain/statistics/split_plot_design.h
    src/domain/statistics/pls_regression.cpp
    src/domain/statistics/pls_regression.h
    src/domain/statistics/definitive_screening_design.cpp
    src/domain/statistics/definitive_screening_design.h
    src/domain/statistics/analyze_definitive_screening.cpp
    src/domain/statistics/analyze_definitive_screening.h
    src/domain/statistics/mixture_extreme_vertices_design.cpp
    src/domain/statistics/mixture_extreme_vertices_design.h
    src/domain/statistics/adf_test.cpp
    src/domain/statistics/adf_test.h
    src/domain/statistics/poisson_regression.cpp
    src/domain/statistics/poisson_regression.h
    src/domain/statistics/isolation_forest.cpp
    src/domain/statistics/isolation_forest.h
    src/domain/statistics/bootstrap_mean.cpp
    src/domain/statistics/bootstrap_mean.h
    src/domain/statistics/orthogonal_regression.cpp
    src/domain/statistics/orthogonal_regression.h
    src/domain/statistics/bootstrap_two_sample.cpp
    src/domain/statistics/bootstrap_two_sample.h
    src/domain/statistics/randomization_test.cpp
    src/domain/statistics/randomization_test.h
    src/domain/statistics/probit_reliability.cpp
    src/domain/statistics/probit_reliability.h
    src/domain/statistics/hierarchical_cluster.cpp
    src/domain/statistics/hierarchical_cluster.h
    src/domain/statistics/ordinal_logistic.cpp
    src/domain/statistics/ordinal_logistic.h
    src/domain/statistics/nominal_logistic.cpp
    src/domain/statistics/nominal_logistic.h
    src/domain/statistics/nonparametric_capability.cpp
    src/domain/statistics/nonparametric_capability.h
    src/domain/statistics/accelerated_life.cpp
    src/domain/statistics/accelerated_life.h
    src/domain/statistics/discriminant.cpp
    src/domain/statistics/discriminant.h
    src/domain/statistics/ccf.cpp
    src/domain/statistics/ccf.h
    src/domain/statistics/stepwise_regression.cpp
    src/domain/statistics/stepwise_regression.h
    src/domain/statistics/best_subsets_regression.cpp
    src/domain/statistics/best_subsets_regression.h
    src/domain/statistics/batch_capability.cpp
    src/domain/statistics/batch_capability.h
    src/domain/statistics/km_interval.cpp
    src/domain/statistics/km_interval.h
    src/domain/statistics/aalen_johansen_cif.cpp
    src/domain/statistics/aalen_johansen_cif.h
    src/domain/statistics/cox_regression.cpp
    src/domain/statistics/cox_regression.h
    src/domain/statistics/cox_counting_process.cpp
    src/domain/statistics/cox_counting_process.h
    src/domain/statistics/gray_test.cpp
    src/domain/statistics/gray_test.h
    src/domain/statistics/fine_gray.cpp
    src/domain/statistics/fine_gray.h
    src/domain/statistics/plackett_burman.cpp
    src/domain/statistics/plackett_burman.h
    src/domain/statistics/response_surface_design.cpp
    src/domain/statistics/response_surface_design.h
    src/domain/statistics/censoring_contract.cpp
    src/domain/statistics/censoring_contract.h
    src/domain/statistics/graph_visuals.cpp
    src/domain/statistics/graph_visuals.h
    src/domain/statistics/eda_plots.cpp
    src/domain/statistics/eda_plots.h
    src/domain/statistics/multi_vari.cpp
    src/domain/statistics/multi_vari.h
    src/domain/statistics/tolerance_intervals.cpp
    src/domain/statistics/tolerance_intervals.h
    src/domain/statistics/proportion_test.cpp
    src/domain/statistics/proportion_test.h
    src/domain/statistics/poisson_rate.cpp
    src/domain/statistics/poisson_rate.h
    src/domain/statistics/equivalence_test.cpp
    src/domain/statistics/equivalence_test.h
    src/domain/statistics/grubbs_test.cpp
    src/domain/statistics/grubbs_test.h
    src/domain/license/license_types.h
    src/domain/license/license_encoding.cpp
    src/domain/license/license_encoding.h
    src/domain/license/license_fingerprint.cpp
    src/domain/license/license_fingerprint.h
    src/domain/license/license_public_key.h
    src/domain/license/license_token.cpp
    src/domain/license/license_token.h
)

add_library(datalab_monocypher STATIC
    third_party/monocypher/monocypher.c
)
target_include_directories(datalab_monocypher PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/monocypher)

target_include_directories(datalab_domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(datalab_domain PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/monocypher)
target_link_libraries(datalab_domain PRIVATE datalab_monocypher)

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
    src/application/computation_trace_attach.cpp
    src/application/computation_trace_attach.h
    src/application/computation_trace_attach_deep.cpp
    src/application/computation_trace_attach_deep.h
    src/application/computation_trace_helpers.cpp
    src/application/computation_trace_helpers.h
    src/application/database/database_import_service.cpp
    src/application/database/database_import_service.h
    src/application/report_assembly_service.cpp
    src/application/report_assembly_service.h
    src/application/report_export_service.cpp
    src/application/report_export_service.h
    src/application/report_localization.cpp
    src/application/report_localization.h
    src/application/command_recommendation_engine.cpp
    src/application/command_recommendation_engine.h
    src/application/mes/mes_table_adapter.cpp
    src/application/mes/mes_table_adapter.h
    src/application/license/license_service.cpp
    src/application/license/license_service.h
    src/application/learning/learning_dataset_store.cpp
    src/application/learning/learning_dataset_store.h
    src/application/learning/learning_tutorial_catalog.cpp
    src/application/learning/learning_tutorial_catalog.h
    src/application/learning/learning_types.h
    src/application/learning/worksheet_registry.cpp
    src/application/learning/worksheet_registry.h
)

target_include_directories(datalab_application PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(datalab_application
    PUBLIC
        datalab_domain
    PRIVATE
        Qt6::Sql
)

# ---------------------------------------------------------------------------
# reporting：图表渲染（纯 QPainter，无 QWidget 依赖；ui 与 infrastructure 共用）
# ---------------------------------------------------------------------------
add_library(datalab_reporting STATIC
    src/reporting/chart_model.h
    src/reporting/chart_interaction.cpp
    src/reporting/chart_interaction.h
    src/reporting/chart_geometry.h
    src/reporting/chart_plot_context.cpp
    src/reporting/chart_plot_context.h
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
# infrastructure：CSV/SQLite/PDF/Excel 等外部依赖适配
# ---------------------------------------------------------------------------
add_library(datalab_infrastructure STATIC
    src/infrastructure/csv_importer.cpp
    src/infrastructure/csv_importer.h
    src/infrastructure/data_import_service.cpp
    src/infrastructure/data_import_service.h
    src/infrastructure/excel_table_importer.cpp
    src/infrastructure/excel_table_importer.h
    src/infrastructure/project_repository.cpp
    src/infrastructure/project_repository.h
    src/infrastructure/output_serialization.cpp
    src/infrastructure/output_serialization.h
    src/infrastructure/database/database_provider_bootstrap.cpp
    src/infrastructure/database/database_provider_bootstrap.h
    src/infrastructure/database/sqlite_database_provider.cpp
    src/infrastructure/database/sqlite_database_provider.h
    src/infrastructure/database/postgresql_database_provider.cpp
    src/infrastructure/database/postgresql_database_provider.h
    src/infrastructure/database/mysql_database_provider.cpp
    src/infrastructure/database/mysql_database_provider.h
    src/infrastructure/database/odbc_database_provider.cpp
    src/infrastructure/database/odbc_database_provider.h
    src/infrastructure/database/import_plan_serialization.cpp
    src/infrastructure/database/import_plan_serialization.h
    src/infrastructure/pdf_report_writer.cpp
    src/infrastructure/pdf_report_writer.h
    src/infrastructure/report_serialization.cpp
    src/infrastructure/report_serialization.h
    src/infrastructure/report_export_writer.cpp
    src/infrastructure/report_export_writer.h
    src/infrastructure/report_layout_cursor.cpp
    src/infrastructure/report_layout_cursor.h
    src/infrastructure/report_painter.cpp
    src/infrastructure/report_painter.h
    src/infrastructure/license/license_store.cpp
    src/infrastructure/license/license_store.h
    src/infrastructure/license/machine_identity.cpp
    src/infrastructure/license/machine_identity.h
)

target_include_directories(datalab_infrastructure PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(datalab_infrastructure PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniz)

add_library(datalab_miniz STATIC
    third_party/miniz/miniz.c
    third_party/miniz/miniz_tdef.c
    third_party/miniz/miniz_tinfl.c
    third_party/miniz/miniz_zip.c
)
target_include_directories(datalab_miniz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniz)

target_link_libraries(datalab_infrastructure
    PUBLIC
        datalab_domain
        Qt6::Core
        Qt6::Gui
    PRIVATE
        datalab_reporting
        Qt6::Sql
        Qt6::Network
        datalab_miniz
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
    src/ui/worksheet_sort_filter_proxy.cpp
    src/ui/worksheet_sort_filter_proxy.h
    src/ui/database_preview_model.cpp
    src/ui/database_preview_model.h
    src/ui/report_table_model.cpp
    src/ui/report_table_model.h
    src/ui/worksheet_view.cpp
    src/ui/worksheet_view.h
    src/ui/report_preview_dialog.cpp
    src/ui/report_preview_dialog.h
    src/ui/report_template_dialog.cpp
    src/ui/report_template_dialog.h
    src/ui/graph_properties_panel.cpp
    src/ui/graph_properties_panel.h
    src/ui/graph_properties_dialog.cpp
    src/ui/graph_properties_dialog.h
    src/ui/algorithm_help_catalog.cpp
    src/ui/algorithm_help_catalog.h
    src/ui/input_option_catalog.cpp
    src/ui/input_option_catalog.h
    src/ui/algorithm_help_dialog.cpp
    src/ui/algorithm_help_dialog.h
    src/ui/learning_center_page.cpp
    src/ui/learning_center_page.h
    src/ui/database_import_wizard.cpp
    src/ui/database_import_wizard.h
    src/ui/mes_tool_dialog.cpp
    src/ui/mes_tool_dialog.h
    src/ui/usage_permission_dialog.cpp
    src/ui/usage_permission_dialog.h
    src/ui/command_wizard_dialog.cpp
    src/ui/command_wizard_dialog.h
    src/ui/formula_renderer.cpp
    src/ui/formula_renderer.h
    src/ui/formula_registry_dialog.cpp
    src/ui/formula_registry_dialog.h
    src/ui/formula_substitution_dialog.cpp
    src/ui/formula_substitution_dialog.h
    src/ui/taguchi_analyze_dialog.cpp
    src/ui/taguchi_analyze_dialog.h
    src/ui/mixture_design_dialog.cpp
    src/ui/mixture_design_dialog.h
    src/ui/nhpp_repairable_dialog.cpp
    src/ui/nhpp_repairable_dialog.h
    src/ui/reliability_test_plan_dialog.cpp
    src/ui/reliability_test_plan_dialog.h
    src/ui/mixture_analyze_dialog.cpp
    src/ui/mixture_analyze_dialog.h
    src/ui/glm_two_way_dialog.cpp
    src/ui/glm_two_way_dialog.h
    src/ui/analyze_variability_dialog.cpp
    src/ui/analyze_variability_dialog.h
    src/ui/factor_analysis_dialog.cpp
    src/ui/factor_analysis_dialog.h
    src/ui/binary_response_doe_dialog.cpp
    src/ui/binary_response_doe_dialog.h
    src/ui/cluster_variables_dialog.cpp
    src/ui/cluster_variables_dialog.h
    src/ui/glm_three_factor_dialog.cpp
    src/ui/glm_three_factor_dialog.h
    src/ui/life_data_regression_dialog.cpp
    src/ui/life_data_regression_dialog.h
    src/ui/expanded_gage_unbalanced_dialog.cpp
    src/ui/expanded_gage_unbalanced_dialog.h
    src/ui/split_plot_analyze_dialog.cpp
    src/ui/split_plot_analyze_dialog.h
    src/ui/mixture_process_variable_dialog.cpp
    src/ui/mixture_process_variable_dialog.h
    src/ui/manova_one_way_dialog.cpp
    src/ui/manova_one_way_dialog.h
    src/ui/general_manova_dialog.cpp
    src/ui/general_manova_dialog.h
    src/ui/mixed_effects_reml_dialog.cpp
    src/ui/mixed_effects_reml_dialog.h
    src/ui/binary_doe_probit_dialog.cpp
    src/ui/binary_doe_probit_dialog.h
    src/ui/life_data_lognormal_dialog.cpp
    src/ui/life_data_lognormal_dialog.h
    src/ui/simple_correspondence_dialog.cpp
    src/ui/simple_correspondence_dialog.h
    src/ui/multiple_correspondence_dialog.cpp
    src/ui/multiple_correspondence_dialog.h
    src/ui/nonlinear_regression_dialog.cpp
    src/ui/nonlinear_regression_dialog.h
    src/ui/split_plot_design_dialog.cpp
    src/ui/split_plot_design_dialog.h
    src/ui/pls_regression_dialog.cpp
    src/ui/pls_regression_dialog.h
    src/ui/definitive_screening_design_dialog.cpp
    src/ui/definitive_screening_design_dialog.h
    src/ui/analyze_definitive_screening_dialog.cpp
    src/ui/analyze_definitive_screening_dialog.h
    src/ui/mixture_extreme_vertices_design_dialog.cpp
    src/ui/mixture_extreme_vertices_design_dialog.h
    src/ui/bubble_plot_dialog.cpp
    src/ui/bubble_plot_dialog.h
    src/ui/dotplot_dialog.cpp
    src/ui/dotplot_dialog.h
    src/ui/simplex_design_plot_dialog.cpp
    src/ui/simplex_design_plot_dialog.h
    src/ui/ccd_design_dialog.cpp
    src/ui/ccd_design_dialog.h
    src/ui/cox_counting_process_dialog.cpp
    src/ui/cox_counting_process_dialog.h
    src/ui/fine_gray_regression_dialog.cpp
    src/ui/fine_gray_regression_dialog.h
    src/ui/bootstrap_two_sample_dialog.cpp
    src/ui/bootstrap_two_sample_dialog.h
    src/ui/randomization_test_dialog.cpp
    src/ui/randomization_test_dialog.h
    src/ui/bootstrap_mean_dialog.cpp
    src/ui/bootstrap_mean_dialog.h
    src/ui/orthogonal_regression_dialog.cpp
    src/ui/orthogonal_regression_dialog.h
    src/ui/trend_analysis_dialog.cpp
    src/ui/trend_analysis_dialog.h
    src/ui/doe_d_optimal_dialog.cpp
    src/ui/doe_d_optimal_dialog.h
    src/ui/anom_attribute_dialog.cpp
    src/ui/anom_attribute_dialog.h
    src/ui/bbd_design_dialog.cpp
    src/ui/bbd_design_dialog.h
    src/ui/mosaic_plot_dialog.cpp
    src/ui/mosaic_plot_dialog.h
    src/ui/graph_gallery_dialog.cpp
    src/ui/graph_gallery_dialog.h
    src/ui/chi_square_mosaic_link_dialog.cpp
    src/ui/chi_square_mosaic_link_dialog.h
    src/ui/response_optimization_dialog.cpp
    src/ui/response_optimization_dialog.h
    src/ui/best_subsets_regression_dialog.cpp
    src/ui/best_subsets_regression_dialog.h
    src/ui/parallel_plot_dialog.cpp
    src/ui/parallel_plot_dialog.h
    src/ui/row_visibility_clipboard.cpp
    src/ui/row_visibility_clipboard.h
    src/ui/algorithm_help_types.h
)

qt_add_resources(datalab_ui algorithm_help_resources
    PREFIX "/help"
    BASE resources/help
    FILES resources/help/algorithm_help.json
)

qt_add_resources(datalab_ui learning_center_resources
    PREFIX "/help"
    BASE resources/help
    FILES resources/help/learning_center.sqlite
)

qt_add_resources(datalab_ui input_option_catalog_resources
    PREFIX "/ui"
    BASE resources/ui
    FILES resources/ui/input_option_catalog.json
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

if(DATALAB_HAS_MES_CORE)
    target_link_libraries(datalab_ui PRIVATE mes_core Qt6::Network)
endif()

set(DATALAB_ICON_FILES
    resources/icons/Q_MES.ico
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
    resources/icons/outlier_test.svg
    resources/icons/correlation.svg
    resources/icons/one_sample_t.svg
    resources/icons/one_sample_z.svg
    resources/icons/two_sample_t.svg
    resources/icons/paired_t.svg
    resources/icons/one_way_anova.svg
    resources/icons/two_factor_anova.svg
    resources/icons/chi_square.svg
    resources/icons/two_proportions.svg
    resources/icons/one_proportion.svg
    resources/icons/one_poisson_rate.svg
    resources/icons/two_poisson_rate.svg
    resources/icons/equivalence_t.svg
    resources/icons/mann_whitney.svg
    resources/icons/wilcoxon_signed_rank.svg
    resources/icons/sign_test.svg
    resources/icons/mcnemar.svg
    resources/icons/cochran_q.svg
    resources/icons/mood_median.svg
    resources/icons/friedman.svg
    resources/icons/kruskal_wallis.svg
    resources/icons/regression.svg
    resources/icons/logistic_regression.svg
    resources/icons/box_cox.svg
    resources/icons/pca.svg
    resources/icons/imr.svg
    resources/icons/xbar_r.svg
    resources/icons/xbar_s.svg
    resources/icons/imr_rs.svg
    resources/icons/hotelling_t2.svg
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
    resources/icons/trend_analysis.svg
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
    resources/icons/time_series_plot.svg
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
)

if(WIN32)
    target_sources(DataLab PRIVATE app.rc)
endif()

qt_add_resources(DataLab translations
    PREFIX "/i18n"
    FILES
        translations/DataLab_zh_CN.qm
        translations/DataLab_en_US.qm
        translations/DataLab_zh_CN.ts
        translations/DataLab_en_US.ts
        translations/report_strings.json
        translations/ui_menu_strings.json
)

qt_add_resources(DataLab app_resources
    PREFIX "/"
    BASE resources
    FILES ${DATALAB_ICON_FILES}
)

target_link_libraries(DataLab
    PRIVATE
        datalab_ui
        Qt6::Widgets
        Qt6::Svg
)

add_custom_command(TARGET DataLab POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:DataLab>"
        "${CMAKE_SOURCE_DIR}/dist/$<TARGET_FILE_NAME:DataLab>"
    VERBATIM
)

add_executable(generate_license_keys
    tools/generate_license_keys.cpp
)
target_include_directories(generate_license_keys PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/monocypher)
target_link_libraries(generate_license_keys PRIVATE datalab_monocypher)

add_executable(DataLabLicenseAdmin WIN32
    tools/license_admin/main.cpp
    tools/license_admin/license_admin_window.cpp
    tools/license_admin/license_admin_window.h
    tools/license_admin/license_admin_store.cpp
    tools/license_admin/license_admin_store.h
    tools/license_admin/license_record_dialog.cpp
    tools/license_admin/license_record_dialog.h
)
target_include_directories(DataLabLicenseAdmin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/monocypher
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/license_admin
)
target_link_libraries(DataLabLicenseAdmin
    PRIVATE
        datalab_domain
        Qt6::Widgets
        Qt6::Sql
)
set_source_files_properties(
    tools/license_admin/license_admin_window.cpp
    tools/license_admin/license_record_dialog.cpp
    PROPERTIES SKIP_AUTOMOC ON
)

add_custom_command(TARGET DataLabLicenseAdmin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist-license-admin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:DataLabLicenseAdmin>"
        "${CMAKE_SOURCE_DIR}/dist-license-admin/$<TARGET_FILE_NAME:DataLabLicenseAdmin>"
    VERBATIM
)

add_custom_target(package_dist
    COMMAND powershell -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_SOURCE_DIR}/tools/package_dist.ps1"
        -BuildDir "${CMAKE_BINARY_DIR}"
        -SkipConfigure
        -SkipBuild
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS DataLab DataLabLicenseAdmin
    COMMENT "Package portable dist/ and dist-license-admin/"
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
add_datalab_test(report_contract_phase0_test tests/report_contract_phase0_test.cpp datalab_domain)
add_datalab_test(report_profile_phase1_test tests/report_profile_phase1_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(report_export_phase2_test tests/report_export_phase2_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(report_locale_phase3_test tests/report_locale_phase3_test.cpp datalab_domain datalab_application)
add_datalab_test(response_surface_design_phase4_test tests/response_surface_design_phase4_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(reliability_phase5_test tests/reliability_phase5_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(nonnormal_capability_phase6_test tests/nonnormal_capability_phase6_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(graph_visibility_phase7_test tests/graph_visibility_phase7_test.cpp datalab_domain datalab_application datalab_infrastructure)

add_datalab_test(column_extract_import_test tests/column_extract_import_test.cpp datalab_domain datalab_infrastructure)
add_datalab_test(mes_table_adapter_test tests/mes_table_adapter_test.cpp datalab_domain datalab_application)
add_datalab_test(license_fingerprint_test tests/license_fingerprint_test.cpp datalab_domain)
add_datalab_test(license_version_independence_test tests/license_version_independence_test.cpp datalab_domain)
add_datalab_test(license_token_test tests/license_token_test.cpp datalab_domain datalab_monocypher)
add_datalab_test(license_service_test tests/license_service_test.cpp datalab_domain datalab_application datalab_monocypher)
target_include_directories(license_token_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/monocypher)
target_include_directories(license_service_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/monocypher)
add_datalab_test(license_admin_store_test tests/license_admin_store_test.cpp)
target_sources(license_admin_store_test PRIVATE tools/license_admin/license_admin_store.cpp)
target_include_directories(license_admin_store_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/license_admin
)
target_link_libraries(license_admin_store_test PRIVATE Qt6::Sql)

add_datalab_test(quality_statistics_test tests/quality_statistics_test.cpp datalab_domain datalab_application)

add_datalab_test(regression_output_test tests/regression_output_test.cpp datalab_domain datalab_application)

add_datalab_test(reliability_msa_power_test tests/reliability_msa_power_test.cpp datalab_domain datalab_application)

add_datalab_test(doe_response_test tests/doe_response_test.cpp datalab_domain)
add_datalab_test(doe_factorial_output_test tests/doe_factorial_output_test.cpp datalab_domain datalab_application datalab_infrastructure)

add_datalab_test(graph_visuals_test tests/graph_visuals_test.cpp datalab_domain)
add_datalab_test(graph_service_test tests/graph_service_test.cpp datalab_application)
add_datalab_test(multi_vari_test tests/multi_vari_test.cpp datalab_domain datalab_application)
add_datalab_test(tolerance_interval_test tests/tolerance_interval_test.cpp datalab_domain datalab_application)
add_datalab_test(attribute_capability_test tests/attribute_capability_test.cpp datalab_domain datalab_application)
add_datalab_test(proportion_test tests/proportion_test.cpp datalab_domain datalab_application)
add_datalab_test(poisson_rate_test tests/poisson_rate_test.cpp datalab_domain datalab_application)
add_datalab_test(two_factor_anova_output_test tests/two_factor_anova_output_test.cpp datalab_domain datalab_application)
add_datalab_test(seasonal_forecasting_output_test tests/seasonal_forecasting_output_test.cpp datalab_domain datalab_application)
add_datalab_test(gage_rr_output_test tests/gage_rr_output_test.cpp datalab_domain datalab_application)
add_datalab_test(equivalence_test tests/equivalence_test.cpp datalab_domain datalab_application)
add_datalab_test(grubbs_test tests/grubbs_test.cpp datalab_domain datalab_application)
add_datalab_test(imr_rs_test tests/imr_rs_test.cpp datalab_domain datalab_application)
add_datalab_test(xbar_output_test tests/xbar_output_test.cpp datalab_domain datalab_application)
add_datalab_test(ewma_cusum_output_test tests/ewma_cusum_output_test.cpp datalab_domain datalab_application)
add_datalab_test(zone_zmr_ma_output_test tests/zone_zmr_ma_output_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(p2_doe_acf_power_test tests/p2_doe_acf_power_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(p3_batch1_kmeans_cart_adf_test tests/p3_batch1_kmeans_cart_adf_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(p3_batch2_poisson_iforest_bootstrap_hclust_test tests/p3_batch2_poisson_iforest_bootstrap_hclust_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(p3_batch3_ordinal_lda_ccf_correlogram_test tests/p3_batch3_ordinal_lda_ccf_correlogram_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(p3_track_h_stepwise_km_pb_test tests/p3_track_h_stepwise_km_pb_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_batch_2026_08_test tests/algorithm_batch_2026_08_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave2_track_test tests/algorithm_wave2_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave2_hardening_test tests/algorithm_wave2_hardening_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave3_track_test tests/algorithm_wave3_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave4_track_test tests/algorithm_wave4_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave5_track_test tests/algorithm_wave5_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave6_track_test tests/algorithm_wave6_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave7_track_test tests/algorithm_wave7_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave8_track_test tests/algorithm_wave8_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave9_track_test tests/algorithm_wave9_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave10_track_test tests/algorithm_wave10_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave11_track_test tests/algorithm_wave11_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave12_track_test tests/algorithm_wave12_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave13_track_test tests/algorithm_wave13_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave14_track_test tests/algorithm_wave14_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave15_track_test tests/algorithm_wave15_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave16_track_test tests/algorithm_wave16_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave17_track_test tests/algorithm_wave17_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(algorithm_wave18_track_test tests/algorithm_wave18_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(g9_formula_substitution_track_test tests/g9_formula_substitution_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(g9_show_your_work_deepen_track_test tests/g9_show_your_work_deepen_track_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(ui_menu_ia_track_test tests/ui_menu_ia_track_test.cpp datalab_ui)
add_datalab_test(
    g6_command_wizard_track_test
    tests/g6_command_wizard_track_test.cpp
    datalab_application
    datalab_ui
)
set_tests_properties(
    g6_command_wizard_track_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
add_datalab_test(
    g7_input_preset_track_test
    tests/g7_input_preset_track_test.cpp
    datalab_application
    datalab_ui
)
set_tests_properties(
    g7_input_preset_track_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
add_datalab_test(p2_rsm_special_cause_test tests/p2_rsm_special_cause_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(p2_eda_plots_test tests/p2_eda_plots_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(p2_eda4_crosstab_chi_resid_test tests/p2_eda4_crosstab_chi_resid_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(p2_t2_mewma_nelson_emp_test tests/p2_t2_mewma_nelson_emp_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(p2_gv_expanded_b4b5_test tests/p2_gv_expanded_b4b5_test.cpp datalab_domain datalab_application datalab_infrastructure)
add_datalab_test(capability_ci_gof_rare_event_power_test tests/capability_ci_gof_rare_event_power_test.cpp datalab_domain datalab_application)

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
add_datalab_test(chart_plot_context_test tests/chart_plot_context_test.cpp datalab_reporting)

add_datalab_test(chart_renderer_test tests/chart_renderer_test.cpp datalab_reporting)
add_datalab_test(chart_interaction_test tests/chart_interaction_test.cpp datalab_reporting datalab_application datalab_infrastructure datalab_domain)
set_tests_properties(
    chart_renderer_test
    chart_interaction_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

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

qt_add_executable(minitab_numerical_golden_test
    tests/minitab_numerical_golden_test.cpp
    tests/fixtures/minitab/golden_loader.cpp
)

target_include_directories(minitab_numerical_golden_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(minitab_numerical_golden_test
    PRIVATE
        datalab_application
        datalab_infrastructure
        Qt6::Test
)
target_compile_definitions(minitab_numerical_golden_test
    PRIVATE
        DATALAB_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME minitab_numerical_golden_test COMMAND minitab_numerical_golden_test)

add_datalab_test(import_state_reset_test tests/import_state_reset_test.cpp datalab_domain datalab_infrastructure)

add_datalab_test(worksheet_model_test tests/worksheet_model_test.cpp datalab_ui)
add_datalab_test(learning_center_store_test tests/learning_center_store_test.cpp datalab_application datalab_ui)
target_link_libraries(learning_center_store_test PRIVATE Qt6::Sql)
add_datalab_test(learning_center_worksheet_registry_test tests/learning_center_worksheet_registry_test.cpp datalab_application)
add_datalab_test(learning_center_analysis_sample_test tests/learning_center_analysis_sample_test.cpp datalab_application datalab_ui)
target_link_libraries(learning_center_analysis_sample_test PRIVATE Qt6::Sql)

add_datalab_test(worksheet_column_ops_test tests/worksheet_column_ops_test.cpp datalab_ui)

add_datalab_test(output_serialization_test tests/output_serialization_test.cpp datalab_infrastructure)

add_datalab_test(quality_diagnostics_test tests/quality_diagnostics_test.cpp datalab_domain datalab_infrastructure)

add_datalab_test(analysis_rule_evidence_test tests/analysis_rule_evidence_test.cpp datalab_domain)

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
    special_cause_rule_catalog_test
    tests/special_cause_rule_catalog_test.cpp
    datalab_domain
    datalab_application
    datalab_infrastructure
)

add_datalab_test(
    database_provider_registry_test
    tests/database_provider_registry_test.cpp
    datalab_application
    datalab_infrastructure
)
set_tests_properties(
    database_provider_registry_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_datalab_test(
    database_import_test
    tests/database_import_test.cpp
    datalab_application
    datalab_infrastructure
)
set_tests_properties(
    database_import_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_datalab_test(
    worksheet_interaction_test
    tests/worksheet_interaction_test.cpp
    datalab_ui
)
set_tests_properties(
    worksheet_interaction_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_datalab_test(
    graph_properties_dialog_test
    tests/graph_properties_dialog_test.cpp
    datalab_ui
)
set_tests_properties(
    graph_properties_dialog_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_datalab_test(
    graph_properties_panel_test
    tests/graph_properties_panel_test.cpp
    datalab_ui
)
set_tests_properties(
    graph_properties_panel_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_datalab_test(
    analysis_chart_widget_test
    tests/analysis_chart_widget_test.cpp
    datalab_ui
)
set_tests_properties(
    analysis_chart_widget_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_datalab_test(
    mainwindow_edit_commands_test
    tests/mainwindow_edit_commands_test.cpp
    datalab_ui
)
set_tests_properties(
    mainwindow_edit_commands_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_datalab_test(
    algorithm_help_catalog_test
    tests/algorithm_help_catalog_test.cpp
    datalab_ui
)

add_datalab_test(
    algorithm_help_dialog_test
    tests/algorithm_help_dialog_test.cpp
    datalab_ui
)
set_tests_properties(
    algorithm_help_dialog_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_datalab_test(
    formula_registry_dialog_test
    tests/formula_registry_dialog_test.cpp
    datalab_ui
)
set_tests_properties(
    formula_registry_dialog_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_datalab_test(
    row_visibility_clipboard_test
    tests/row_visibility_clipboard_test.cpp
    datalab_ui
)
set_tests_properties(
    row_visibility_clipboard_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_datalab_test(
    output_workspace_test
    tests/output_workspace_test.cpp
    datalab_ui
)
set_tests_properties(
    output_workspace_test
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

set(DATALAB_TRACK_G1_G2_TESTS_REGISTERED TRUE CACHE INTERNAL
    "Track G1+G2 tests registered (formula_registry_dialog_test, row_visibility_clipboard_test, analysis_chart_widget_test, output_workspace_test, algorithm_help_dialog_test)")
message(STATUS "Track G1+G2: 5 test targets registered (see tools/check_g1g2_build_ready.py)")

qt_add_executable(excel_table_importer_test
    tests/excel_table_importer_test.cpp
)

target_link_libraries(excel_table_importer_test
    PRIVATE
        datalab_infrastructure
        Qt6::Test
)
target_compile_definitions(excel_table_importer_test
    PRIVATE
        DATALAB_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME excel_table_importer_test COMMAND excel_table_importer_test)

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
