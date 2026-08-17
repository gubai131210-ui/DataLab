#include "ui/analysis_setup_dialog.h"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QFocusEvent>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QList>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <QToolButton>

#include <functional>

namespace {

class RoleListWidget final : public QListWidget {
public:
    using QListWidget::QListWidget;

    void set_activation_handler(std::function<void(QListWidget*)> handler)
    {
        handler_ = std::move(handler);
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (handler_) {
            handler_(this);
        }
        QListWidget::mousePressEvent(event);
    }

    void focusInEvent(QFocusEvent* event) override
    {
        if (handler_) {
            handler_(this);
        }
        QListWidget::focusInEvent(event);
    }

private:
    std::function<void(QListWidget*)> handler_;
};

}  // namespace

AnalysisSetupDialog::AnalysisSetupDialog(
    const QString& title,
    const QStringList& column_labels,
    QWidget* parent,
    const QString& icon_resource)
    : QDialog(parent)
    , column_labels_(column_labels)
{
    setWindowTitle(title);
    // 图标由调用方（run_from_spec 传命令表 icon_file）提供，缺省回退应用图标。
    const QString effective_icon = icon_resource.isEmpty()
        ? QStringLiteral(":/icons/app-mark.svg")
        : icon_resource;
    setWindowIcon(QIcon(effective_icon));
    setModal(true);
    resize(820, 560);
    setMinimumSize(700, 480);
    setStyleSheet(QStringLiteral(
        "QDialog { background: #f4f7f9; color: #29434e; }"
        "QLabel { color: #49636d; }"
        "QListWidget { background: #ffffff; border: 1px solid #cbd9de;"
        " border-radius: 5px; padding: 4px; color: #29434e; }"
        "QListWidget::item { padding: 6px; border-radius: 4px; }"
        "QListWidget::item:hover { background: #eaf6f6; }"
        "QListWidget::item:selected { background: #d8eeee; color: #147d85; }"
        "QPushButton { background: #ffffff; color: #39717a; border: 1px solid #b9d2d7;"
        " border-radius: 5px; padding: 7px 14px; }"
        "QPushButton:hover { background: #eaf6f6; border-color: #42aeb4; }"
        "QDialogButtonBox QPushButton { min-width: 84px; }"
        "QLineEdit { background: #ffffff; border: 1px solid #cbd9de;"
        " border-radius: 5px; padding: 7px 9px; }"
        "QLineEdit:focus { border-color: #42aeb4; }"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 18, 20, 18);
    root->setSpacing(18);
    auto* header = new QHBoxLayout();
    auto* header_icon = new QLabel(this);
    header_icon->setPixmap(QIcon(effective_icon).pixmap(32, 32));
    header->addWidget(header_icon);
    auto* header_text = new QLabel(title, this);
    header_text->setStyleSheet(QStringLiteral(
        "font-size: 19px; font-weight: 700; color: #29434e;"));
    header->addWidget(header_text);
    header->addStretch();
    root->addLayout(header);
    auto* content = new QHBoxLayout();
    content->setSpacing(18);
    auto* left = new QVBoxLayout();
    left->setSpacing(8);
    auto* source_title = new QLabel(QStringLiteral("工作表列"), this);
    source_title->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 600; color: #29434e;"));
    left->addWidget(source_title);
    available_ = new QListWidget(this);
    available_->addItems(column_labels_);
    available_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    left->addWidget(available_);
    auto* select = new QPushButton(QStringLiteral("选择 >"), this);
    select->setIcon(QIcon(QStringLiteral(":/icons/import-data.svg")));
    left->addWidget(select);
    left->addWidget(new QLabel(
        QStringLiteral("先点击右侧角色框，再点“选择 >”或双击左侧列。"), this));

    auto* right = new QVBoxLayout();
    right->setSpacing(8);
    auto* settings_title = new QLabel(QStringLiteral("分析设置"), this);
    settings_title->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 600; color: #29434e;"));
    right->addWidget(settings_title);
    roles_layout_ = new QFormLayout();
    roles_layout_->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    roles_layout_->setFormAlignment(Qt::AlignTop);
    roles_layout_->setVerticalSpacing(10);
    roles_layout_->setHorizontalSpacing(10);
    right->addLayout(roles_layout_);
    right->addStretch();
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    right->addWidget(buttons);

    content->addLayout(left, 1);
    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Plain);
    separator->setStyleSheet(QStringLiteral("color: #d6e1e5;"));
    content->addWidget(separator);
    content->addLayout(right, 1);
    root->addLayout(content, 1);

    const auto ok_button = buttons->button(QDialogButtonBox::Ok);
    const auto cancel_button = buttons->button(QDialogButtonBox::Cancel);
    if (ok_button != nullptr) {
        ok_button->setIcon(QIcon(QStringLiteral(":/icons/success.svg")));
    }
    if (cancel_button != nullptr) {
        cancel_button->setIcon(QIcon(QStringLiteral(":/icons/error.svg")));
    }

    connect(select, &QPushButton::clicked, this, &AnalysisSetupDialog::select_into_role);
    connect(available_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        select_into_role();
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void AnalysisSetupDialog::set_active_role(QListWidget* list)
{
    if (list == nullptr) {
        return;
    }
    active_role_ = list;
    for (QListWidget* role : findChildren<QListWidget*>()) {
        if (role == available_) {
            continue;
        }
        role->setStyleSheet(role == list
            ? QStringLiteral(
                  "QListWidget { border: 2px solid #42aeb4; background: #f0fbfb; }")
            : QStringLiteral(
                  "QListWidget { border: 1px solid #cbd9de; background: #ffffff; }"));
    }
}

void AnalysisSetupDialog::add_role(
    const QString& id,
    const QString& label,
    bool multi,
    bool optional)
{
    auto* list = new RoleListWidget(this);
    list->setMinimumHeight(multi ? 96 : 48);
    list->setMaximumHeight(multi ? 120 : 56);
    list->setObjectName(id);
    list->setProperty("optional", optional);
    list->setProperty("multi", multi);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->set_activation_handler([this](QListWidget* role) {
        set_active_role(role);
    });
    connect(list, &QListWidget::itemSelectionChanged, this, [this, list] {
        set_active_role(list);
    });
    if (active_role_ == nullptr) {
        set_active_role(list);
    } else {
        list->setStyleSheet(
            QStringLiteral("QListWidget { border: 1px solid #cbd9de; background: #ffffff; }"));
    }
    auto* role_label = new QLabel(this);
    role_label->setPixmap(QIcon(QStringLiteral(":/icons/data-table.svg")).pixmap(16, 16));
    role_label->setToolTip(label + (optional ? QStringLiteral("（可选）") : QString()));
    roles_layout_->addRow(role_label, list);
    list->setToolTip(label + (optional ? QStringLiteral("（可选）") : QString()));
}

QWidget* AnalysisSetupDialog::add_line_edit(
    const QString& id,
    const QString& label,
    const QString& placeholder)
{
    auto* edit = new QLineEdit(this);
    edit->setObjectName(id);
    edit->setPlaceholderText(placeholder);
    auto* parameter_label = new QLabel(this);
    parameter_label->setPixmap(QIcon(QStringLiteral(":/icons/settings.svg")).pixmap(16, 16));
    parameter_label->setToolTip(label);
    roles_layout_->addRow(parameter_label, edit);
    edit->setToolTip(label);
    return edit;
}

QListWidget* AnalysisSetupDialog::role_list(const QString& id) const
{
    return findChild<QListWidget*>(id);
}

void AnalysisSetupDialog::select_into_role()
{
    QListWidget* target = active_role_;
    if (target == nullptr) {
        return;
    }
    const QList<QListWidgetItem*> selected = available_->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const bool multi = target->property("multi").toBool();
    if (!multi) {
        target->clear();
    }
    for (QListWidgetItem* item : selected) {
        const int column = available_->row(item);
        auto* copy = new QListWidgetItem(item->text());
        copy->setData(Qt::UserRole, column);
        target->addItem(copy);
        if (!multi) {
            break;
        }
    }
}

QList<int> AnalysisSetupDialog::role_indices(const QString& id) const
{
    QList<int> indices;
    QListWidget* list = role_list(id);
    if (list == nullptr) {
        return indices;
    }
    for (int row = 0; row < list->count(); ++row) {
        indices.append(list->item(row)->data(Qt::UserRole).toInt());
    }
    return indices;
}

int AnalysisSetupDialog::first_role_index(const QString& id) const
{
    const QList<int> indices = role_indices(id);
    return indices.isEmpty() ? -1 : indices.front();
}

QString AnalysisSetupDialog::line_text(const QString& id) const
{
    const QLineEdit* edit = findChild<QLineEdit*>(id);
    return edit == nullptr ? QString() : edit->text().trimmed();
}

std::optional<double> AnalysisSetupDialog::line_number(const QString& id) const
{
    bool ok = false;
    const double value = line_text(id).toDouble(&ok);
    return ok ? std::optional<double>(value) : std::nullopt;
}

std::optional<int> AnalysisSetupDialog::line_int(const QString& id) const
{
    bool ok = false;
    const int value = line_text(id).toInt(&ok);
    return ok ? std::optional<int>(value) : std::nullopt;
}
