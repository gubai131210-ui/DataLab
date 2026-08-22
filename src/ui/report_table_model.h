#pragma once

#include "domain/quality_types.h"

#include <QAbstractTableModel>

namespace datalab::ui {

enum ReportTableRole {
    ReportRowIdRole = Qt::UserRole + 21,
    ReportRuleIdRole = Qt::UserRole + 22,
    ReportColumnKindRole = Qt::UserRole + 23
};

class ReportTableModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit ReportTableModel(QObject* parent = nullptr);

    void set_table(const domain::StatisticTable& table);
    const domain::StatisticTable& table() const;

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    domain::StatisticTable table_;
};

}  // namespace datalab::ui
