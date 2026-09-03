#include "ui/worksheet_model.h"

#include "domain/column_extract.h"

#include <QColor>
#include <QFont>
#include <QModelIndexList>

#include <algorithm>
#include <set>

WorksheetModel::WorksheetModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

void WorksheetModel::set_table(const datalab::domain::DataTable& table)
{
    beginResetModel();
    table_ = table;
    column_hidden_.assign(table_.columns.size(), false);
    refresh_display_extents();
    endResetModel();
}

void WorksheetModel::replace_table(const datalab::domain::DataTable& table)
{
    set_table(table);
    emit table_changed(table_);
}

void WorksheetModel::set_excluded_rows(const std::vector<std::size_t>& rows)
{
    beginResetModel();
    excluded_rows_.clear();
    excluded_rows_.insert(rows.begin(), rows.end());
    endResetModel();
}

void WorksheetModel::set_hidden_rows(const std::vector<std::size_t>& rows)
{
    beginResetModel();
    hidden_rows_.clear();
    hidden_rows_.insert(rows.begin(), rows.end());
    endResetModel();
}

const datalab::domain::DataTable& WorksheetModel::table() const
{
    return table_;
}

QStringList WorksheetModel::column_labels() const
{
    QStringList labels;
    for (std::size_t index = 0; index < table_.columns.size(); ++index) {
        labels.append(QString::fromStdString(datalab::domain::column_label(table_, index)));
    }
    return labels;
}

int WorksheetModel::used_column_count() const
{
    return static_cast<int>(table_.columns.size());
}

int WorksheetModel::used_row_count() const
{
    return static_cast<int>(table_.rows.size());
}

int WorksheetModel::target_column_count(int used_columns) const
{
    const int used = std::max(0, used_columns);
    const int padded = used == 0 ? kDefaultColumns : used + kColumnSlack;
    return std::min(kMaxColumns, std::max(kDefaultColumns, padded));
}

int WorksheetModel::target_row_count(int used_rows) const
{
    const int used = std::max(0, used_rows);
    const int padded = used == 0 ? kDefaultRows : used + kRowSlack;
    return std::min(kMaxRows, std::max(kDefaultRows, padded));
}

void WorksheetModel::refresh_display_extents()
{
    minimum_columns_ = target_column_count(used_column_count());
    minimum_rows_ = target_row_count(used_row_count());
}

bool WorksheetModel::extend_display_columns()
{
    const int current = columnCount();
    if (current >= kMaxColumns) {
        return false;
    }
    const int target = std::min(
        kMaxColumns,
        std::max(current + kColumnSlack, used_column_count() + kColumnSlack));
    if (target <= current) {
        return false;
    }
    beginInsertColumns({}, current, target - 1);
    minimum_columns_ = target;
    endInsertColumns();
    return true;
}

bool WorksheetModel::extend_display_rows()
{
    const int current = rowCount();
    if (current >= kMaxRows) {
        return false;
    }
    const int target = std::min(
        kMaxRows,
        std::max(current + kRowSlack, used_row_count() + kRowSlack));
    if (target <= current) {
        return false;
    }
    beginInsertRows({}, current, target - 1);
    minimum_rows_ = target;
    endInsertRows();
    return true;
}

bool WorksheetModel::ensure_slack()
{
    bool grew = false;
    const int column_target = target_column_count(used_column_count());
    const int current_columns = columnCount();
    if (column_target > current_columns) {
        beginInsertColumns({}, current_columns, column_target - 1);
        minimum_columns_ = column_target;
        endInsertColumns();
        grew = true;
    }
    const int row_target = target_row_count(used_row_count());
    const int current_rows = rowCount();
    if (row_target > current_rows) {
        beginInsertRows({}, current_rows, row_target - 1);
        minimum_rows_ = row_target;
        endInsertRows();
        grew = true;
    }
    return grew;
}

bool WorksheetModel::grow_if_at_edge(int row, int column)
{
    const int columns_before = columnCount();
    const int rows_before = rowCount();
    const bool at_last_column = column >= 0 && column >= columns_before - 1;
    const bool at_last_row = row >= 0 && row >= rows_before - 1;

    bool grew = ensure_slack();

    if (at_last_column) {
        if (columnCount() == columns_before) {
            grew = extend_display_columns() || grew;
        }
        if (columnCount() >= kMaxColumns && column >= kMaxColumns - 1) {
            emit display_limit_reached(Qt::Horizontal);
        }
    }
    if (at_last_row) {
        if (rowCount() == rows_before) {
            grew = extend_display_rows() || grew;
        }
        if (rowCount() >= kMaxRows && row >= kMaxRows - 1) {
            emit display_limit_reached(Qt::Vertical);
        }
    }
    return grew;
}

int WorksheetModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return std::min(kMaxRows, std::max(minimum_rows_, used_row_count()));
}

int WorksheetModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return std::min(kMaxColumns, std::max(minimum_columns_, used_column_count()));
}

QVariant WorksheetModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    const std::size_t row = static_cast<std::size_t>(index.row());
    const std::size_t column = static_cast<std::size_t>(index.column());
    if (role == Qt::BackgroundRole && excluded_rows_.count(row) > 0) {
        return QColor("#fff3cd");  // excluded: analysis-out
    }
    if (role == Qt::BackgroundRole && hidden_rows_.count(row) > 0) {
        return QColor("#e7f1ff");  // hidden: display-only omit (still analysis-eligible)
    }
    if (role == datalab::ui::SourceRowIndexRole) {
        return static_cast<qulonglong>(row);
    }
    if (role == datalab::ui::RowIdRole) {
        if (row < table_.row_ids.size()) {
            return static_cast<qulonglong>(table_.row_ids[row]);
        }
        return {};
    }
    if (role == datalab::ui::ColumnTypeRole) {
        if (column < table_.column_types.size()) {
            return static_cast<int>(table_.column_types[column]);
        }
        return static_cast<int>(datalab::domain::ColumnType::unknown);
    }
    if (role == datalab::ui::CellStateRole) {
        if (row < table_.cell_states.size()
            && column < table_.cell_states[row].size()) {
            return static_cast<int>(table_.cell_states[row][column]);
        }
        return static_cast<int>(datalab::domain::CellState::missing);
    }
    if (role == Qt::ToolTipRole) {
        QStringList parts;
        if (row < table_.row_ids.size()) {
            parts.append(QStringLiteral("RowId=%1").arg(table_.row_ids[row]));
        }
        parts.append(QStringLiteral("源行=%1").arg(static_cast<qulonglong>(row)));
        if (column < table_.column_types.size()) {
            const auto type = table_.column_types[column];
            parts.append(QStringLiteral("类型=%1").arg(
                type == datalab::domain::ColumnType::numeric ? QStringLiteral("数值")
                : type == datalab::domain::ColumnType::time ? QStringLiteral("时间")
                : type == datalab::domain::ColumnType::categorical ? QStringLiteral("类别")
                : QStringLiteral("未知")));
        }
        if (row < table_.cell_states.size()
            && column < table_.cell_states[row].size()) {
            const auto state = table_.cell_states[row][column];
            parts.append(QStringLiteral("状态=%1").arg(
                state == datalab::domain::CellState::valid ? QStringLiteral("有效")
                : state == datalab::domain::CellState::invalid ? QStringLiteral("无效数值")
                : QStringLiteral("缺失/NULL")));
        }
        return parts.join(QStringLiteral("；"));
    }
    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return {};
    }
    if (row >= table_.rows.size() || column >= table_.rows[row].size()) {
        return QString();
    }
    if (row < table_.cell_states.size()
        && column < table_.cell_states[row].size()
        && table_.cell_states[row][column] == datalab::domain::CellState::missing) {
        return role == Qt::EditRole ? QString() : QStringLiteral("*");
    }
    const std::string& cell = table_.rows[row][column];
    if (role == Qt::EditRole) {
        return QString::fromStdString(cell == "*" ? std::string() : cell);
    }
    if (cell.empty()) {
        return QString();
    }
    if (datalab::domain::is_missing_cell(cell)) {
        return QStringLiteral("*");
    }
    return QString::fromStdString(cell);
}

QVariant WorksheetModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role) const
{
    if (role == Qt::EditRole && orientation == Qt::Horizontal
        && section >= 0 && section < columnCount()) {
        return section < static_cast<int>(table_.columns.size())
            ? QString::fromStdString(table_.columns[static_cast<std::size_t>(section)])
            : QString();
    }
    if (role == Qt::FontRole && orientation == Qt::Horizontal) {
        QFont font;
        font.setPointSize(8);
        return font;
    }
    if (role == Qt::ToolTipRole && orientation == Qt::Vertical
        && section >= 0
        && section < static_cast<int>(table_.row_ids.size())) {
        return QStringLiteral("稳定 RowId=%1（非视觉行号）")
            .arg(table_.row_ids[static_cast<std::size_t>(section)]);
    }
    if (role == Qt::AccessibleDescriptionRole && orientation == Qt::Vertical
        && section >= 0
        && section < static_cast<int>(table_.row_ids.size())) {
        return QStringLiteral("RowId %1")
            .arg(table_.row_ids[static_cast<std::size_t>(section)]);
    }
    if (role != Qt::DisplayRole) {
        return {};
    }
    if (orientation == Qt::Horizontal
        && section >= 0
        && section < columnCount()) {
        const QString name = section < static_cast<int>(table_.columns.size())
            ? QString::fromStdString(table_.columns[static_cast<std::size_t>(section)])
            : QString();
        return QStringLiteral("C%1\n%2")
            .arg(section + 1)
            .arg(name);
    }
    if (orientation == Qt::Vertical) {
        if (section >= 0 && section < static_cast<int>(table_.row_ids.size())) {
            return QString::number(table_.row_ids[static_cast<std::size_t>(section)]);
        }
        return section + 1;
    }
    return {};
}

Qt::ItemFlags WorksheetModel::flags(const QModelIndex& index) const
{
    return index.isValid()
        ? Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable
        : Qt::NoItemFlags;
}

bool WorksheetModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || index.row() < 0 || index.column() < 0) {
        return false;
    }
    if (index.row() >= kMaxRows || index.column() >= kMaxColumns) {
        emit display_limit_reached(index.column() >= kMaxColumns
                                      ? Qt::Horizontal
                                      : Qt::Vertical);
        return false;
    }
    const std::size_t row = static_cast<std::size_t>(index.row());
    const std::size_t column = static_cast<std::size_t>(index.column());
    if (row >= table_.rows.size()) {
        table_.rows.resize(row + 1);
    }
    if (column >= table_.columns.size()) {
        table_.columns.resize(column + 1);
    }
    if (column >= table_.rows[row].size()) {
        table_.rows[row].resize(column + 1);
    }
    table_.rows[row][column] = value.toString().toStdString();
    if (table_.name.empty()) {
        table_.name = "工作表1";
    }
    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    grow_if_at_edge(index.row(), index.column());
    emit table_changed(table_);
    return true;
}

bool WorksheetModel::clear_cells(const QModelIndexList& indexes)
{
    if (indexes.isEmpty()) {
        return false;
    }
    int min_row = indexes.front().row();
    int max_row = min_row;
    int min_column = indexes.front().column();
    int max_column = min_column;
    for (const QModelIndex& index : indexes) {
        if (!index.isValid()) {
            continue;
        }
        min_row = std::min(min_row, index.row());
        max_row = std::max(max_row, index.row());
        min_column = std::min(min_column, index.column());
        max_column = std::max(max_column, index.column());
    }
    if (max_row < 0 || max_column < 0) {
        return false;
    }
    const std::size_t original_rows = table_.rows.size();
    const std::size_t original_columns = table_.columns.size();
    if (static_cast<std::size_t>(max_row) >= table_.rows.size()) {
        table_.rows.resize(static_cast<std::size_t>(max_row) + 1);
    }
    if (static_cast<std::size_t>(max_column) >= table_.columns.size()) {
        table_.columns.resize(static_cast<std::size_t>(max_column) + 1);
    }
    bool changed = table_.rows.size() != original_rows
        || table_.columns.size() != original_columns;
    for (const QModelIndex& index : indexes) {
        if (!index.isValid()) {
            continue;
        }
        const std::size_t row = static_cast<std::size_t>(index.row());
        const std::size_t column = static_cast<std::size_t>(index.column());
        if (column >= table_.rows[row].size()) {
            table_.rows[row].resize(column + 1);
        }
        if (!table_.rows[row][column].empty()) {
            table_.rows[row][column].clear();
            changed = true;
        }
    }
    if (!changed) {
        return false;
    }
    if (table_.name.empty()) {
        table_.name = "工作表1";
    }
    const QModelIndex top_left = index(min_row, min_column);
    const QModelIndex bottom_right = index(max_row, max_column);
    emit dataChanged(top_left, bottom_right, {Qt::DisplayRole, Qt::EditRole});
    emit table_changed(table_);
    return true;
}

bool WorksheetModel::clear_columns(const QList<int>& column_indexes)
{
    if (column_indexes.isEmpty()) {
        return false;
    }
    std::set<int> unique_columns;
    for (int column : column_indexes) {
        if (column >= 0) {
            unique_columns.insert(column);
        }
    }
    if (unique_columns.empty()) {
        return false;
    }
    bool changed = false;
    const int min_column = *unique_columns.begin();
    const int max_column = *unique_columns.rbegin();
    for (int column_index : unique_columns) {
        const std::size_t column = static_cast<std::size_t>(column_index);
        if (column >= table_.columns.size()) {
            continue;
        }
        for (auto& row : table_.rows) {
            if (column < row.size() && !row[column].empty()) {
                row[column].clear();
                changed = true;
            }
        }
    }
    if (!changed) {
        return false;
    }
    if (table_.name.empty()) {
        table_.name = "工作表1";
    }
    const QModelIndex top_left = index(0, min_column);
    const int last_row = std::max(0, rowCount() - 1);
    const QModelIndex bottom_right = index(last_row, max_column);
    emit dataChanged(top_left, bottom_right, {Qt::DisplayRole, Qt::EditRole});
    emit table_changed(table_);
    return true;
}

bool WorksheetModel::remove_columns(const QList<int>& column_indexes)
{
    if (column_indexes.isEmpty()) {
        return false;
    }
    std::set<int> unique_columns;
    for (int column : column_indexes) {
        if (column >= 0 && column < static_cast<int>(table_.columns.size())) {
            unique_columns.insert(column);
        }
    }
    if (unique_columns.empty()) {
        return false;
    }
    std::vector<std::size_t> remove_at;
    remove_at.reserve(unique_columns.size());
    for (int column : unique_columns) {
        remove_at.push_back(static_cast<std::size_t>(column));
    }
    std::sort(remove_at.begin(), remove_at.end(), std::greater<std::size_t>());
    for (std::size_t column : remove_at) {
        if (column >= table_.columns.size()) {
            continue;
        }
        table_.columns.erase(table_.columns.begin() + static_cast<std::ptrdiff_t>(column));
        if (column < table_.column_types.size()) {
            table_.column_types.erase(
                table_.column_types.begin() + static_cast<std::ptrdiff_t>(column));
        }
        for (auto& row : table_.rows) {
            if (column < row.size()) {
                row.erase(row.begin() + static_cast<std::ptrdiff_t>(column));
            }
        }
        for (auto& states : table_.cell_states) {
            if (column < states.size()) {
                states.erase(states.begin() + static_cast<std::ptrdiff_t>(column));
            }
        }
        if (column < column_hidden_.size()) {
            column_hidden_.erase(column_hidden_.begin() + static_cast<std::ptrdiff_t>(column));
        }
    }
    if (column_hidden_.size() < table_.columns.size()) {
        column_hidden_.resize(table_.columns.size(), false);
    }
    if (table_.name.empty()) {
        table_.name = "工作表1";
    }
    refresh_display_extents();
    beginResetModel();
    endResetModel();
    emit table_changed(table_);
    return true;
}

bool WorksheetModel::setHeaderData(
    int section, Qt::Orientation orientation, const QVariant& value, int role)
{
    if (orientation != Qt::Horizontal || role != Qt::EditRole || section < 0) {
        return false;
    }
    if (section >= kMaxColumns) {
        emit display_limit_reached(Qt::Horizontal);
        return false;
    }
    const std::size_t column = static_cast<std::size_t>(section);
    if (column >= table_.columns.size()) {
        table_.columns.resize(column + 1);
    }
    const QString name = value.toString().trimmed();
    table_.columns[column] = name.toStdString();
    emit headerDataChanged(Qt::Horizontal, section, section);
    grow_if_at_edge(0, section);
    emit table_changed(table_);
    return true;
}

void WorksheetModel::set_column_hidden_flags(const std::vector<bool>& hidden)
{
    column_hidden_ = hidden;
    if (column_hidden_.size() < table_.columns.size()) {
        column_hidden_.resize(table_.columns.size(), false);
    }
}

const std::vector<bool>& WorksheetModel::column_hidden_flags() const
{
    return column_hidden_;
}

QString WorksheetModel::selection_tsv(
    const QModelIndexList& indexes,
    bool include_headers) const
{
    if (indexes.isEmpty()) {
        return {};
    }
    int min_row = indexes.front().row();
    int max_row = min_row;
    int min_column = indexes.front().column();
    int max_column = min_column;
    for (const QModelIndex& index : indexes) {
        min_row = std::min(min_row, index.row());
        max_row = std::max(max_row, index.row());
        min_column = std::min(min_column, index.column());
        max_column = std::max(max_column, index.column());
    }
    QStringList lines;
    if (include_headers) {
        QStringList headers;
        for (int column = min_column; column <= max_column; ++column) {
            headers.append(headerData(column, Qt::Horizontal, Qt::EditRole).toString());
        }
        lines.append(headers.join(QLatin1Char('\t')));
    }
    for (int row = min_row; row <= max_row; ++row) {
        QStringList cells;
        for (int column = min_column; column <= max_column; ++column) {
            cells.append(data(index(row, column), Qt::DisplayRole).toString());
        }
        lines.append(cells.join(QLatin1Char('\t')));
    }
    return lines.join(QLatin1Char('\n'));
}
