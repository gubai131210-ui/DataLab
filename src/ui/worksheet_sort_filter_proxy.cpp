#include "ui/worksheet_sort_filter_proxy.h"

#include "domain/column_extract.h"
#include "ui/worksheet_model.h"

namespace datalab::ui {

WorksheetSortFilterProxyModel::WorksheetSortFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

void WorksheetSortFilterProxyModel::set_sortable_columns(const std::set<int>& columns)
{
    sortable_columns_ = columns;
}

void WorksheetSortFilterProxyModel::set_text_filter(const QString& text)
{
    global_filter_ = text.trimmed();
    invalidateFilter();
}

bool WorksheetSortFilterProxyModel::set_column_filter(int column, const QString& text)
{
    if (column < 0) {
        return false;
    }
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        column_filters_.remove(column);
    } else {
        column_filters_.insert(column, trimmed);
    }
    invalidateFilter();
    return true;
}

bool WorksheetSortFilterProxyModel::lessThan(
    const QModelIndex& left,
    const QModelIndex& right) const
{
    if (!sortable_columns_.empty()
        && sortable_columns_.count(left.column()) == 0) {
        return left.row() < right.row();
    }
    const QVariant left_type = sourceModel()->data(left, ColumnTypeRole);
    const QVariant right_type = sourceModel()->data(right, ColumnTypeRole);
    if (left_type.isValid()
        && right_type.isValid()
        && left_type.toInt() == static_cast<int>(datalab::domain::ColumnType::numeric)
        && right_type.toInt() == static_cast<int>(datalab::domain::ColumnType::numeric)) {
        double left_number = 0.0;
        double right_number = 0.0;
        const bool left_ok = datalab::domain::parse_finite_number(
            left.data(Qt::EditRole).toString().toStdString(), left_number);
        const bool right_ok = datalab::domain::parse_finite_number(
            right.data(Qt::EditRole).toString().toStdString(), right_number);
        if (left_ok && right_ok) {
            return left_number < right_number;
        }
    }
    return QSortFilterProxyModel::lessThan(left, right);
}

bool WorksheetSortFilterProxyModel::filterAcceptsRow(
    int source_row,
    const QModelIndex& source_parent) const
{
    if (sourceModel() == nullptr) {
        return true;
    }
    if (!global_filter_.isEmpty()) {
        bool matched = false;
        for (int column = 0; column < sourceModel()->columnCount(source_parent); ++column) {
            const QModelIndex index = sourceModel()->index(source_row, column, source_parent);
            if (sourceModel()->data(index, Qt::DisplayRole).toString()
                    .contains(global_filter_, Qt::CaseInsensitive)) {
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
    }
    for (auto it = column_filters_.constBegin(); it != column_filters_.constEnd(); ++it) {
        const QModelIndex index =
            sourceModel()->index(source_row, it.key(), source_parent);
        if (!sourceModel()->data(index, Qt::DisplayRole).toString()
                 .contains(it.value(), Qt::CaseInsensitive)) {
            return false;
        }
    }
    return true;
}

}  // namespace datalab::ui
