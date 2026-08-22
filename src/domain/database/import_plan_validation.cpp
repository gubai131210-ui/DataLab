#include "domain/database_types.h"

#include <algorithm>
#include <set>
#include <sstream>

namespace datalab::domain {

ImportCancellationToken::ImportCancellationToken()
    : cancelled_(std::make_shared<std::atomic<bool>>(false))
{
}

void ImportCancellationToken::request_cancel()
{
    if (cancelled_) {
        cancelled_->store(true);
    }
}

bool ImportCancellationToken::is_cancellation_requested() const
{
    return cancelled_ && cancelled_->load();
}

namespace {

bool allowed_filter_op(const std::string& op)
{
    static const std::set<std::string> allowed = {
        "=", "!=", "<", "<=", ">", ">=", "LIKE", "IS NULL", "IS NOT NULL"};
    return allowed.count(op) == 1;
}

}  // namespace

std::string validate_import_plan(const ImportPlan& plan, const TableMetadata& metadata)
{
    if (plan.provider_id.empty()) {
        return "ImportPlan 缺少 provider_id。";
    }
    if (plan.object_ref.name.empty()) {
        return "ImportPlan 缺少对象名称。";
    }
    if (plan.selected_columns.empty()) {
        return "请至少选择一列导入。";
    }

    std::set<std::string> available;
    std::set<std::string> selectable;
    for (const ColumnMetadata& column : metadata.columns) {
        available.insert(column.name);
        if (column.selectable && !column.hidden) {
            selectable.insert(column.name);
        } else if (column.selectable) {
            selectable.insert(column.name);
        }
    }

    std::set<std::string> seen;
    for (const std::string& column : plan.selected_columns) {
        if (available.count(column) == 0) {
            return "所选列不存在：" + column;
        }
        if (!seen.insert(column).second) {
            return "所选列重复：" + column;
        }
    }

    if (!plan.column_order.empty()) {
        if (plan.column_order.size() != plan.selected_columns.size()) {
            return "column_order 长度必须与 selected_columns 一致。";
        }
        std::set<std::string> ordered(plan.column_order.begin(), plan.column_order.end());
        std::set<std::string> selected(plan.selected_columns.begin(), plan.selected_columns.end());
        if (ordered != selected) {
            return "column_order 必须与 selected_columns 是同一集合。";
        }
    }

    if (!plan.aliases.empty() && plan.aliases.size() != plan.selected_columns.size()) {
        return "aliases 长度必须与 selected_columns 一致。";
    }

    if (!plan.order_key.empty() && available.count(plan.order_key) == 0) {
        return "排序键不存在：" + plan.order_key;
    }

    if (plan.keyset_after.has_value()) {
        const KeysetCursor& cursor = *plan.keyset_after;
        if (cursor.columns.empty()) {
            return "keyset_after.columns 不能为空。";
        }
        if (cursor.columns.size() != cursor.after_values.size()) {
            return "keyset_after.columns 与 after_values 长度必须一致。";
        }
        for (const std::string& column : cursor.columns) {
            if (available.count(column) == 0) {
                return "keyset 列不存在：" + column;
            }
        }
        if (!plan.order_key.empty()) {
            if (cursor.columns.size() == 1) {
                if (plan.order_key != cursor.columns.front()) {
                    return "单列 keyset 必须与 order_key 一致。";
                }
            } else if (std::find(cursor.columns.begin(), cursor.columns.end(), plan.order_key)
                       == cursor.columns.end()) {
                return "复合 keyset 必须包含 order_key。";
            }
        }
        if (!metadata.primary_key_columns.empty()
            && cursor.columns.size() > 1
            && cursor.columns != metadata.primary_key_columns
            && plan.order_key.empty()) {
            // Allow composite keyset on explicit column list; prefer matching PK when provided.
        }
        if (plan.row_offset.has_value() && *plan.row_offset > 0) {
            return "不能同时使用 keyset_after 与 row_offset；优先 keyset。";
        }
        if (!plan.row_limit.has_value() || *plan.row_limit == 0) {
            return "keyset 分页需要设置 row_limit/page_size。";
        }
    }

    for (const StructuredFilter& filter : plan.structured_filter) {
        if (available.count(filter.column) == 0) {
            return "过滤列不存在：" + filter.column;
        }
        if (!allowed_filter_op(filter.op)) {
            return "不支持的过滤运算符：" + filter.op;
        }
        if ((filter.op == "IS NULL" || filter.op == "IS NOT NULL") && !filter.value.empty()) {
            return filter.op + " 不得带绑定值。";
        }
        if (filter.op != "IS NULL" && filter.op != "IS NOT NULL" && filter.value.empty()) {
            return "过滤运算符 " + filter.op + " 需要绑定值。";
        }
    }

    if (plan.null_policy == DatabaseNullPolicy::reject_null) {
        // validated at import time against actual rows
    }
    return {};
}

std::string summarize_import_filters(const ImportPlan& plan)
{
    if (plan.structured_filter.empty()) {
        return "无";
    }
    std::ostringstream stream;
    for (std::size_t index = 0; index < plan.structured_filter.size(); ++index) {
        if (index > 0) {
            stream << "; ";
        }
        const StructuredFilter& filter = plan.structured_filter[index];
        stream << filter.column << ' ' << filter.op;
        if (filter.op != "IS NULL" && filter.op != "IS NOT NULL") {
            stream << " ?";
        }
    }
    return stream.str();
}

}  // namespace datalab::domain
