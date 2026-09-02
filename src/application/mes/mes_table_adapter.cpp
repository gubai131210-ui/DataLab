#include "application/mes/mes_table_adapter.h"

#include "domain/column_extract.h"

namespace datalab::application {
namespace {

constexpr char kSeqColumn[] = "序号";
constexpr char kInputSnColumn[] = "输入SN";
constexpr char kRouteTimeColumn[] = "过站时间";
constexpr char kTestItemSpecColumn[] = "TestItem/TestSpec";
constexpr char kTestValueColumn[] = "测试参数值";
constexpr char kUpperLimitColumn[] = "上限";
constexpr char kLowerLimitColumn[] = "下限";
constexpr char kQueryStatusColumn[] = "查询状态";
constexpr char kErrorInfoColumn[] = "错误信息";
constexpr char kWideUpperSuffix[] = "_上限";
constexpr char kWideLowerSuffix[] = "_下限";
constexpr char kWideStatusSuffix[] = "_状态";

bool is_test_value_api(const std::string& api_name)
{
    return api_name == "TestValueQuery" || api_name == "TimeRangeTestValueQuery";
}

bool ends_with(const std::string& text, const std::string& suffix)
{
    return text.size() >= suffix.size()
        && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool is_wide_table_header(const std::vector<std::string>& headers)
{
    for (const std::string& header : headers) {
        if (ends_with(header, kWideUpperSuffix)
            || ends_with(header, kWideLowerSuffix)
            || ends_with(header, kWideStatusSuffix)) {
            return true;
        }
    }
    return false;
}

bool should_keep_wide_column(const std::string& header)
{
    if (header == kInputSnColumn || header == kRouteTimeColumn) {
        return true;
    }
    if (ends_with(header, kWideUpperSuffix)
        || ends_with(header, kWideLowerSuffix)
        || ends_with(header, kWideStatusSuffix)) {
        return false;
    }
    return true;
}

bool should_keep_long_column(const std::string& header, const std::string& api_name)
{
    if (header == kInputSnColumn
        || header == kTestItemSpecColumn
        || header == kTestValueColumn) {
        return true;
    }
    if (header == kRouteTimeColumn && is_test_value_api(api_name)) {
        return true;
    }
    return false;
}

void slim_test_value_columns(
    std::vector<std::string>& headers,
    std::vector<std::vector<std::string>>& rows,
    const std::string& api_name)
{
    const bool wide = is_wide_table_header(headers);
    std::vector<std::size_t> keep_indices;
    for (std::size_t index = 0; index < headers.size(); ++index) {
        const bool keep = wide
            ? should_keep_wide_column(headers[index])
            : should_keep_long_column(headers[index], api_name);
        if (keep) {
            keep_indices.push_back(index);
        }
    }

    std::vector<std::string> new_headers;
    new_headers.push_back(kSeqColumn);
    for (const std::size_t index : keep_indices) {
        new_headers.push_back(headers[index]);
    }

    std::vector<std::vector<std::string>> new_rows;
    new_rows.reserve(rows.size());
    for (std::size_t row_index = 0; row_index < rows.size(); ++row_index) {
        std::vector<std::string> new_row;
        new_row.push_back(std::to_string(row_index + 1));
        for (const std::size_t col_index : keep_indices) {
            if (col_index < rows[row_index].size()) {
                new_row.push_back(rows[row_index][col_index]);
            } else {
                new_row.push_back("");
            }
        }
        new_rows.push_back(std::move(new_row));
    }

    headers = std::move(new_headers);
    rows = std::move(new_rows);
}

}  // namespace

MesTableImportResult materialize_mes_batch_table(const MesTableImportRequest& request)
{
    MesTableImportResult result;
    if (request.headers.empty()) {
        result.error_message = "MES 批量结果没有列标题。";
        return result;
    }
    if (request.rows.empty()) {
        result.error_message = "MES 批量结果没有数据行。";
        return result;
    }

    std::vector<std::string> headers = request.headers;
    std::vector<std::vector<std::string>> rows = request.rows;
    bool slimmed = false;
    if (request.slim_test_value_import && is_test_value_api(request.api_name)) {
        slim_test_value_columns(headers, rows, request.api_name);
        slimmed = true;
    }

    domain::DataTable table;
    table.name = request.api_name.empty()
        ? std::string("MES_Batch")
        : "MES_" + request.api_name;
    table.source_path = "mes_batch";
    table.columns = std::move(headers);
    table.rows = std::move(rows);

    table.import_metadata.provider_id = "mes_batch";
    table.import_metadata.source_object = request.api_name;
    table.import_metadata.object_kind = "batch_query";
    table.import_metadata.imported_at = "mes_batch_import";
    table.import_metadata.filter_summary = slimmed
        ? request.api_name + "; slim_test_value_v1"
        : request.api_name;

    populate_data_table_contract(table);
    const std::string validation_error = validate_data_table_contract(table);
    if (!validation_error.empty()) {
        result.error_message = validation_error;
        return result;
    }

    result.table = std::move(table);
    return result;
}

}  // namespace datalab::application
