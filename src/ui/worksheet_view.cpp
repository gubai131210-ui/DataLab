#include "ui/worksheet_view.h"

#include "ui/worksheet_model.h"

#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QHeaderView>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QStyledItemDelegate>

namespace {

class CompactCellDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void updateEditorGeometry(
        QWidget* editor,
        const QStyleOptionViewItem& option,
        const QModelIndex&) const override
    {
        if (editor != nullptr) {
            editor->setGeometry(option.rect);
        }
    }
};

}  // namespace

WorksheetView::WorksheetView(QWidget* parent)
    : QTableView(parent)
{
    setItemDelegate(new CompactCellDelegate(this));
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setShowGrid(true);
    setGridStyle(Qt::SolidLine);
    setAlternatingRowColors(true);
    setWordWrap(false);
    setCornerButtonEnabled(true);
    setStyleSheet(QStringLiteral(
        "QTableView { background: #ffffff; alternate-background-color: #f6fafb;"
        " gridline-color: #dbe6e9; selection-background-color: #d8eeee;"
        " selection-color: #29434e; border: 1px solid #d6e1e5; }"
        "QTableView QLineEdit { min-height: 0; padding: 0 4px; margin: 0;"
        " border: 1px solid #35a6aa; border-radius: 0; }"
        "QHeaderView::section { background: #e7f0f3; color: #29434e;"
        " border: 0; border-right: 1px solid #d6e1e5; border-bottom: 1px solid #d6e1e5;"
        " padding: 5px 8px; font-weight: 600; }"));
    horizontalHeader()->setDefaultSectionSize(84);
    horizontalHeader()->setMinimumHeight(34);
    horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    horizontalHeader()->setSectionsClickable(true);
    horizontalHeader()->setStretchLastSection(false);
    verticalHeader()->setDefaultSectionSize(26);
    verticalHeader()->setVisible(true);
    verticalHeader()->setMinimumWidth(56);
    verticalHeader()->setDefaultAlignment(Qt::AlignRight | Qt::AlignVCenter);
    verticalHeader()->setSectionsClickable(false);
    connect(horizontalHeader(), &QHeaderView::sectionDoubleClicked,
            this, [this](int section) {
                if (model() == nullptr) {
                    return;
                }
                const QString current = model()->headerData(
                    section, Qt::Horizontal, Qt::EditRole).toString();
                bool accepted = false;
                const QString name = QInputDialog::getText(
                    this, QStringLiteral("编辑列名"), QStringLiteral("列名："),
                    QLineEdit::Normal, current, &accepted);
                if (accepted) {
                    model()->setHeaderData(section, Qt::Horizontal, name);
                }
            });
    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(horizontalHeader(), &QHeaderView::customContextMenuRequested,
            this, [this](const QPoint& pos) {
                const int section = horizontalHeader()->logicalIndexAt(pos);
                if (section >= 0) {
                    selectColumn(section);
                }
                emit header_context_menu_requested(pos);
            });
}

bool WorksheetView::is_editing() const
{
    return state() == EditingState;
}

void WorksheetView::commit_editing()
{
    if (state() != EditingState) {
        return;
    }
    if (QWidget* editor = indexWidget(currentIndex())) {
        closeEditor(editor, QAbstractItemDelegate::SubmitModelCache);
    }
}

void WorksheetView::scroll_source_rows_into_view(
    QAbstractProxyModel* proxy,
    const std::vector<std::size_t>& source_rows)
{
    if (proxy == nullptr || source_rows.empty()) {
        return;
    }
    QModelIndex anchor;
    for (const std::size_t row : source_rows) {
        const QModelIndex source = proxy->sourceModel()->index(static_cast<int>(row), 0);
        const QModelIndex mapped = proxy->mapFromSource(source);
        if (!mapped.isValid()) {
            continue;
        }
        scrollTo(mapped, QAbstractItemView::PositionAtCenter);
        if (!anchor.isValid()) {
            anchor = mapped;
        }
    }
    if (anchor.isValid()) {
        setCurrentIndex(anchor);
    }
}

void WorksheetView::currentChanged(const QModelIndex& current,
                                    const QModelIndex& previous)
{
    QTableView::currentChanged(current, previous);
    emit active_cell_changed(current);
}

WorksheetModel* WorksheetView::source_worksheet_model() const
{
    QAbstractItemModel* current = model();
    while (auto* proxy = qobject_cast<QAbstractProxyModel*>(current)) {
        current = proxy->sourceModel();
    }
    return qobject_cast<WorksheetModel*>(current);
}

QModelIndex WorksheetView::map_to_source(const QModelIndex& index) const
{
    QModelIndex mapped = index;
    QAbstractItemModel* current = model();
    while (auto* proxy = qobject_cast<QAbstractProxyModel*>(current)) {
        mapped = proxy->mapToSource(mapped);
        current = proxy->sourceModel();
    }
    return mapped;
}

void WorksheetView::grow_grid_for_cursor(CursorAction cursorAction)
{
    WorksheetModel* source = source_worksheet_model();
    if (source == nullptr) {
        return;
    }
    const QModelIndex current = currentIndex();
    if (!current.isValid()) {
        return;
    }
    const QModelIndex source_index = map_to_source(current);
    switch (cursorAction) {
    case MoveNext:
    case MoveRight:
    case MoveDown:
        source->grow_if_at_edge(source_index.row(), source_index.column());
        break;
    default:
        break;
    }
}

void WorksheetView::keyPressEvent(QKeyEvent* event)
{
    if (event != nullptr && (event->modifiers() & Qt::ControlModifier) == 0) {
        const int key = event->key();
        if (key == Qt::Key_Tab || key == Qt::Key_Right) {
            grow_grid_for_cursor(MoveNext);
        } else if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Down) {
            grow_grid_for_cursor(MoveDown);
        }
    }
    QTableView::keyPressEvent(event);
}

QModelIndex WorksheetView::moveCursor(CursorAction cursorAction,
                                      Qt::KeyboardModifiers modifiers)
{
    grow_grid_for_cursor(cursorAction);
    return QTableView::moveCursor(cursorAction, modifiers);
}
