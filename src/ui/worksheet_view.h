#pragma once

#include <QAbstractItemView>
#include <QAbstractProxyModel>
#include <QTableView>

#include <QPoint>

#include <cstddef>
#include <vector>

class QKeyEvent;
class WorksheetModel;

class WorksheetView final : public QTableView {
    Q_OBJECT

public:
    explicit WorksheetView(QWidget* parent = nullptr);

    bool is_editing() const;
    void commit_editing();
    void scroll_source_rows_into_view(
        QAbstractProxyModel* proxy,
        const std::vector<std::size_t>& source_rows);

signals:
    void active_cell_changed(const QModelIndex& index);
    void header_context_menu_requested(const QPoint& pos);

protected:
    void currentChanged(const QModelIndex& current,
                        const QModelIndex& previous) override;
    void keyPressEvent(QKeyEvent* event) override;
    QModelIndex moveCursor(CursorAction cursorAction,
                            Qt::KeyboardModifiers modifiers) override;

private:
    WorksheetModel* source_worksheet_model() const;
    QModelIndex map_to_source(const QModelIndex& index) const;
    void grow_grid_for_cursor(CursorAction cursorAction);
};
