#include "ui/report_table_model.h"

namespace datalab::ui {

ReportTableModel::ReportTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

void ReportTableModel::set_table(const domain::StatisticTable& table)
{
    beginResetModel();
    table_ = table;
    endResetModel();
}

const domain::StatisticTable& ReportTableModel::table() const
{
    return table_;
}

int ReportTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(table_.rows.size());
}

int ReportTableModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(table_.headers.size());
}

QVariant ReportTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    const std::size_t row = static_cast<std::size_t>(index.row());
    const std::size_t column = static_cast<std::size_t>(index.column());
    if (role == ReportRowIdRole && row < table_.row_ids.size()) {
        return static_cast<qulonglong>(table_.row_ids[row]);
    }
    if (role == ReportRuleIdRole && row < table_.rule_ids.size()) {
        return QString::fromStdString(table_.rule_ids[row]);
    }
    if (role == ReportColumnKindRole && column < table_.column_kinds.size()) {
        return QString::fromStdString(table_.column_kinds[column]);
    }
    if (role == Qt::ToolTipRole) {
        QStringList parts;
        if (row < table_.row_ids.size()) {
            parts.append(QStringLiteral("RowId=%1").arg(table_.row_ids[row]));
        }
        if (row < table_.rule_ids.size() && !table_.rule_ids[row].empty()) {
            parts.append(QStringLiteral("rule_id=%1")
                             .arg(QString::fromStdString(table_.rule_ids[row])));
        }
        if (column < table_.column_kinds.size()) {
            parts.append(QStringLiteral("列类型=%1")
                             .arg(QString::fromStdString(table_.column_kinds[column])));
        }
        return parts.join(QStringLiteral("；"));
    }
    if (role == Qt::TextAlignmentRole) {
        QString kind = column < table_.column_kinds.size()
            ? QString::fromStdString(table_.column_kinds[column])
            : QString();
        if (kind == QStringLiteral("number")
            || kind == QStringLiteral("percent")
            || kind == QStringLiteral("p_value")
            || kind == QStringLiteral("row_id")) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }
    if (role != Qt::DisplayRole) {
        return {};
    }
    if (row >= table_.rows.size() || column >= table_.rows[row].size()) {
        return {};
    }
    return QString::fromStdString(table_.rows[row][column]);
}

QVariant ReportTableModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }
    if (orientation == Qt::Horizontal
        && section >= 0
        && section < static_cast<int>(table_.headers.size())) {
        return QString::fromStdString(table_.headers[static_cast<std::size_t>(section)]);
    }
    return {};
}

Qt::ItemFlags ReportTableModel::flags(const QModelIndex& index) const
{
    return index.isValid()
        ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable)
        : Qt::NoItemFlags;
}

}  // namespace datalab::ui
