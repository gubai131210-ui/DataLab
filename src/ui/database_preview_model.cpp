#include "ui/database_preview_model.h"

#include "ui/worksheet_model.h"

namespace datalab::ui {

DatabasePreviewModel::DatabasePreviewModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

void DatabasePreviewModel::set_sample(const domain::DataTable& sample, bool truncated)
{
    beginResetModel();
    table_ = sample;
    truncated_ = truncated;
    paging_enabled_ = false;
    loader_ = {};
    keyset_columns_.clear();
    using_keyset_ = false;
    last_error_.clear();
    endResetModel();
}

void DatabasePreviewModel::configure_paging(
    domain::ImportPlan plan,
    std::uint64_t page_size,
    PageLoader loader,
    std::vector<std::string> keyset_columns)
{
    plan_ = std::move(plan);
    page_size_ = page_size == 0 ? 50 : page_size;
    loader_ = std::move(loader);
    keyset_columns_ = std::move(keyset_columns);
    using_keyset_ = !keyset_columns_.empty() && !keyset_column_indices().empty();
    paging_enabled_ = static_cast<bool>(loader_);
    truncated_ = true;
}

void DatabasePreviewModel::clear()
{
    beginResetModel();
    table_ = {};
    truncated_ = false;
    paging_enabled_ = false;
    loader_ = {};
    keyset_columns_.clear();
    using_keyset_ = false;
    last_error_.clear();
    endResetModel();
}

std::vector<int> DatabasePreviewModel::keyset_column_indices() const
{
    std::vector<int> indices;
    for (const std::string& name : keyset_columns_) {
        int found = -1;
        for (std::size_t index = 0; index < table_.columns.size(); ++index) {
            if (table_.columns[index] == name) {
                found = static_cast<int>(index);
                break;
            }
        }
        if (found < 0) {
            for (std::size_t index = 0; index < plan_.selected_columns.size(); ++index) {
                if (plan_.selected_columns[index] == name) {
                    found = static_cast<int>(index);
                    break;
                }
            }
        }
        if (found < 0) {
            return {};
        }
        indices.push_back(found);
    }
    return indices;
}

int DatabasePreviewModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(table_.rows.size());
}

int DatabasePreviewModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(table_.columns.size());
}

QVariant DatabasePreviewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    const std::size_t row = static_cast<std::size_t>(index.row());
    const std::size_t column = static_cast<std::size_t>(index.column());
    if (role == RowIdRole && row < table_.row_ids.size()) {
        return static_cast<qulonglong>(table_.row_ids[row]);
    }
    if (role == SourceRowIndexRole) {
        return static_cast<qulonglong>(row);
    }
    if (role == CellStateRole
        && row < table_.cell_states.size()
        && column < table_.cell_states[row].size()) {
        return static_cast<int>(table_.cell_states[row][column]);
    }
    if (role == ColumnTypeRole && column < table_.column_types.size()) {
        return static_cast<int>(table_.column_types[column]);
    }
    if (role == Qt::ToolTipRole) {
        QString text = QStringLiteral("源行=%1").arg(static_cast<qulonglong>(row));
        if (row < table_.row_ids.size()) {
            text += QStringLiteral("；RowId=%1").arg(table_.row_ids[row]);
        }
        if (using_keyset_) {
            text += QStringLiteral("；分页=keyset");
        }
        return text;
    }
    if (role != Qt::DisplayRole) {
        return {};
    }
    if (row >= table_.rows.size() || column >= table_.rows[row].size()) {
        return {};
    }
    if (row < table_.cell_states.size()
        && column < table_.cell_states[row].size()
        && table_.cell_states[row][column] == datalab::domain::CellState::missing) {
        return QStringLiteral("<NULL>");
    }
    return QString::fromStdString(table_.rows[row][column]);
}

QVariant DatabasePreviewModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }
    if (orientation == Qt::Horizontal
        && section >= 0
        && section < static_cast<int>(table_.columns.size())) {
        return QString::fromStdString(table_.columns[static_cast<std::size_t>(section)]);
    }
    if (orientation == Qt::Vertical) {
        if (section >= 0 && section < static_cast<int>(table_.row_ids.size())) {
            return QString::number(table_.row_ids[static_cast<std::size_t>(section)]);
        }
        return section + 1;
    }
    return {};
}

Qt::ItemFlags DatabasePreviewModel::flags(const QModelIndex& index) const
{
    return index.isValid()
        ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable)
        : Qt::NoItemFlags;
}

bool DatabasePreviewModel::canFetchMore(const QModelIndex& parent) const
{
    return !parent.isValid() && paging_enabled_ && truncated_ && !fetch_in_progress_
        && static_cast<bool>(loader_);
}

void DatabasePreviewModel::fetchMore(const QModelIndex& parent)
{
    if (!canFetchMore(parent)) {
        return;
    }
    fetch_in_progress_ = true;
    domain::ImportPlan page_plan = plan_;
    page_plan.row_limit = page_size_;
    page_plan.row_offset = std::nullopt;
    page_plan.keyset_after = std::nullopt;

    const std::vector<int> indices = keyset_column_indices();
    using_keyset_ = !indices.empty() && !table_.rows.empty();
    if (using_keyset_) {
        domain::KeysetCursor cursor;
        cursor.columns = keyset_columns_;
        const auto& last_row = table_.rows.back();
        for (int index : indices) {
            if (static_cast<std::size_t>(index) >= last_row.size()) {
                using_keyset_ = false;
                break;
            }
            cursor.after_values.push_back(last_row[static_cast<std::size_t>(index)]);
        }
        if (using_keyset_) {
            page_plan.keyset_after = cursor;
            if (page_plan.order_key.empty() && !cursor.columns.empty()) {
                page_plan.order_key = cursor.columns.front();
            }
        }
    }
    if (!using_keyset_) {
        page_plan.row_offset = static_cast<std::uint64_t>(table_.rows.size());
    }

    const std::uint64_t offset = page_plan.row_offset.value_or(0);
    auto loaded = loader_(page_plan, offset, page_size_);
    fetch_in_progress_ = false;
    if (!loaded.ok) {
        last_error_ = QString::fromStdString(loaded.error_message);
        truncated_ = false;
        return;
    }
    if (loaded.value.rows.empty()) {
        truncated_ = false;
        return;
    }
    const int first = static_cast<int>(table_.rows.size());
    const int count = static_cast<int>(loaded.value.rows.size());
    beginInsertRows({}, first, first + count - 1);
    for (std::size_t row = 0; row < loaded.value.rows.size(); ++row) {
        table_.rows.push_back(loaded.value.rows[row]);
        if (row < loaded.value.row_ids.size()) {
            table_.row_ids.push_back(loaded.value.row_ids[row]);
        }
        if (row < loaded.value.cell_states.size()) {
            table_.cell_states.push_back(loaded.value.cell_states[row]);
        }
    }
    if (table_.columns.empty()) {
        table_.columns = loaded.value.columns;
        table_.column_types = loaded.value.column_types;
    }
    endInsertRows();
    truncated_ = loaded.value.rows.size() >= page_size_;
}

bool DatabasePreviewModel::truncated() const
{
    return truncated_;
}

std::uint64_t DatabasePreviewModel::loaded_rows() const
{
    return static_cast<std::uint64_t>(table_.rows.size());
}

QString DatabasePreviewModel::last_error() const
{
    return last_error_;
}

bool DatabasePreviewModel::using_keyset_paging() const
{
    return using_keyset_;
}

}  // namespace datalab::ui
