#pragma once

#include <QHash>
#include <QSortFilterProxyModel>

#include <set>

namespace datalab::ui {

// Sorting is restricted to a whitelist of column indices (never free-form SQL).
class WorksheetSortFilterProxyModel final : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit WorksheetSortFilterProxyModel(QObject* parent = nullptr);

    void set_sortable_columns(const std::set<int>& columns);
    void set_text_filter(const QString& text);
    bool set_column_filter(int column, const QString& text);

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    std::set<int> sortable_columns_;
    QString global_filter_;
    QHash<int, QString> column_filters_;
};

}  // namespace datalab::ui
