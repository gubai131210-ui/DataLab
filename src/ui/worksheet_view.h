#pragma once

#include <QTableView>

class WorksheetView final : public QTableView {
    Q_OBJECT

public:
    explicit WorksheetView(QWidget* parent = nullptr);

    bool is_editing() const;
    void commit_editing();

signals:
    void active_cell_changed(const QModelIndex& index);

protected:
    void currentChanged(const QModelIndex& current,
                        const QModelIndex& previous) override;
};
