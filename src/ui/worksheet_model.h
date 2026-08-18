#pragma once

#include "domain/quality_types.h"

#include <QAbstractTableModel>
#include <QStringList>

#include <set>
#include <vector>

class WorksheetModel final : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit WorksheetModel(QObject* parent = nullptr);

    void set_table(const datalab::domain::DataTable& table);
    void replace_table(const datalab::domain::DataTable& table);
    void set_excluded_rows(const std::vector<std::size_t>& rows);
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

private:
    datalab::domain::DataTable table_;
    std::set<std::size_t> excluded_rows_;
    int minimum_rows_ = 100;
    int minimum_columns_ = 20;

signals:
    void table_changed(const datalab::domain::DataTable& table);
};
