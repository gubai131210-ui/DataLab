#pragma once

#include "domain/quality_types.h"

#include <QAbstractTableModel>
#include <QStringList>

#include <set>
#include <vector>

namespace datalab::ui {

enum WorksheetDataRole {
    RowIdRole = Qt::UserRole + 1,
    CellStateRole = Qt::UserRole + 2,
    ColumnTypeRole = Qt::UserRole + 3,
    SourceRowIndexRole = Qt::UserRole + 4
};

}  // namespace datalab::ui

class WorksheetModel final : public QAbstractTableModel {
    Q_OBJECT
public:
    static constexpr int kDefaultRows = 100;
    static constexpr int kDefaultColumns = 20;
    static constexpr int kRowSlack = 50;
    static constexpr int kColumnSlack = 16;
    static constexpr int kMaxRows = 1'048'576;
    static constexpr int kMaxColumns = 4000;

    explicit WorksheetModel(QObject* parent = nullptr);

    void set_table(const datalab::domain::DataTable& table);
    void replace_table(const datalab::domain::DataTable& table);
    void set_excluded_rows(const std::vector<std::size_t>& rows);
    void set_hidden_rows(const std::vector<std::size_t>& rows);
    const datalab::domain::DataTable& table() const;
    QStringList column_labels() const;
    bool setData(const QModelIndex& index, const QVariant& value,
                 int role = Qt::EditRole) override;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setHeaderData(int section, Qt::Orientation orientation,
                       const QVariant& value, int role = Qt::EditRole) override;
    bool clear_cells(const QModelIndexList& indexes);
    bool clear_columns(const QList<int>& column_indexes);
    bool remove_columns(const QList<int>& column_indexes);

    void set_column_hidden_flags(const std::vector<bool>& hidden);
    const std::vector<bool>& column_hidden_flags() const;
    QString selection_tsv(const QModelIndexList& indexes, bool include_headers) const;

    // Grows the visible grid when the cell is on the last displayed row/column.
    // Empty slack is display-only and is not written into DataTable.
    // Returns true if the displayed grid grew.
    bool grow_if_at_edge(int row, int column);

private:
    int used_column_count() const;
    int used_row_count() const;
    int target_column_count(int used_columns) const;
    int target_row_count(int used_rows) const;
    void refresh_display_extents();
    bool ensure_slack();
    bool extend_display_columns();
    bool extend_display_rows();

    datalab::domain::DataTable table_;
    std::set<std::size_t> excluded_rows_;
    std::set<std::size_t> hidden_rows_;
    std::vector<bool> column_hidden_;
    int minimum_rows_ = kDefaultRows;
    int minimum_columns_ = kDefaultColumns;

signals:
    void table_changed(const datalab::domain::DataTable& table);
    void display_limit_reached(Qt::Orientation orientation);
};
