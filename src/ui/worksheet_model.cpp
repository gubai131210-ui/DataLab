#include "ui/worksheet_model.h"

#include "domain/column_extract.h"

#include <QColor>
#include <QFont>
#include <QModelIndexList>

#include <algorithm>

WorksheetModel::WorksheetModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

void WorksheetModel::set_table(const datalab::domain::DataTable& table)
{
    beginResetModel();
    table_ = table;
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

int WorksheetModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid()
        ? 0
        : std::max(minimum_rows_, static_cast<int>(table_.rows.size()));
}

int WorksheetModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid()
        ? 0
        : std::max(minimum_columns_, static_cast<int>(table_.columns.size()));
}

QVariant WorksheetModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    const std::size_t row = static_cast<std::size_t>(index.row());
    const std::size_t column = static_cast<std::size_t>(index.column());
    if (role == Qt::BackgroundRole && excluded_rows_.count(row) > 0) {
        return QColor("#fff3cd");
    }
    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return {};
    }
    if (row >= table_.rows.size() || column >= table_.rows[row].size()) {
        return QString();
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
    return orientation == Qt::Vertical ? section + 1 : QVariant{};
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

bool WorksheetModel::setHeaderData(
    int section, Qt::Orientation orientation, const QVariant& value, int role)
{
    if (orientation != Qt::Horizontal || role != Qt::EditRole || section < 0) {
        return false;
    }
    const std::size_t column = static_cast<std::size_t>(section);
    if (column >= table_.columns.size()) {
        table_.columns.resize(column + 1);
    }
    const QString name = value.toString().trimmed();
    table_.columns[column] = name.toStdString();
    emit headerDataChanged(Qt::Horizontal, section, section);
    emit table_changed(table_);
    return true;
}
