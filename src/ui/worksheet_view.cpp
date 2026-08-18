#include "ui/worksheet_view.h"

#include <QAbstractItemModel>
#include <QHeaderView>
#include <QInputDialog>
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
}

#include <QAbstractItemDelegate>

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

void WorksheetView::currentChanged(const QModelIndex& current,
                                    const QModelIndex& previous)
{
    QTableView::currentChanged(current, previous);
    emit active_cell_changed(current);
}
