#include "ui/analysis_setup_dialog.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QCheckBox>
#include <QColor>
#include <QDoubleSpinBox>
#include <QDoubleValidator>
#include <QFocusEvent>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QIcon>
#include <QIntValidator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QList>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QStringList>
#include <QSpinBox>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>

#include "domain/statistics/control_charts.h"
#include "ui/app_ui_tr.h"

#include <algorithm>
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

QWidget* make_field_label(
    const QString& text,
    const QString& icon_resource,
    const bool optional,
    QWidget* parent)
{
    // `text` is already locale-resolved (ui_tr); optional suffix is translated separately.
    const QString caption = optional
        ? text + datalab::ui::ui_tr("（可选）")
        : text;
    auto* container = new QWidget(parent);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    auto* icon = new QLabel(container);
    icon->setPixmap(QIcon(icon_resource).pixmap(16, 16));
    icon->setFixedSize(16, 16);
    auto* caption_label = new QLabel(caption, container);
    caption_label->setWordWrap(true);
    caption_label->setStyleSheet(QStringLiteral(
        "font-size: 13px; font-weight: 600; color: #29434e;"));
    layout->addWidget(icon, 0, Qt::AlignTop);
    layout->addWidget(caption_label, 1);
    container->setToolTip(caption);
    container->setMinimumWidth(108);
    return container;
}

// Stable zh_CN tokens used for type matching (do not translate these values).
QString column_type_label(const datalab::domain::ColumnType type)
{
    switch (type) {
    case datalab::domain::ColumnType::numeric:
        return QStringLiteral("数值");
    case datalab::domain::ColumnType::categorical:
        return QStringLiteral("分类");
    case datalab::domain::ColumnType::time:
        return QStringLiteral("时间");
    case datalab::domain::ColumnType::unknown:
        break;
    }
    return QStringLiteral("未知");
}

QString column_type_display(const datalab::domain::ColumnType type)
{
    return datalab::ui::ui_tr(column_type_label(type));
}

QString translate_type_token(const QString& token)
{
    return datalab::ui::ui_tr(token);
}

class SpecialCauseTestsEditor final : public QWidget {
public:
    SpecialCauseTestsEditor(const QString& chart_kind, QWidget* parent)
        : QWidget(parent)
        , kind_(datalab::domain::statistics::control_chart_kind_from_name(
              chart_kind.toStdString()))
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);
        summary_ = new QLabel(this);
        layout->addWidget(summary_);
        auto* buttons = new QHBoxLayout();
        auto* select_all = new QPushButton(datalab::ui::ui_tr("全选"), this);
        auto* clear_all = new QPushButton(datalab::ui::ui_tr("清空"), this);
        auto* restore = new QPushButton(datalab::ui::ui_tr("恢复默认"), this);
        buttons->addWidget(select_all);
        buttons->addWidget(clear_all);
        buttons->addWidget(restore);
        buttons->addStretch();
        layout->addLayout(buttons);
        auto* list = new QWidget(this);
        auto* list_layout = new QVBoxLayout(list);
        list_layout->setContentsMargins(0, 0, 0, 0);
        list_layout->setSpacing(4);
        const auto applicable =
            datalab::domain::statistics::applicable_special_cause_tests(kind_);
        for (const auto& spec : datalab::domain::statistics::all_special_cause_tests()) {
            auto* check = new QCheckBox(
                QString::fromUtf8(spec.short_name),
                list);
            const bool allowed = std::find(applicable.begin(), applicable.end(), spec.number)
                != applicable.end();
            check->setToolTip(QString::fromUtf8(spec.description));
            check->setEnabled(allowed && kind_ != datalab::domain::statistics::ControlChartKind::cusum);
            check->setChecked(allowed);
            check->setProperty("defaultValue", allowed);
            check->setProperty("testNumber", spec.number);
            if (!allowed) {
                check->setToolTip(
                    QString::fromUtf8(spec.description)
                    + QLatin1Char('\n')
                    + datalab::ui::ui_tr("此规则不适用于当前控制图，已置灰。"));
            }
            list_layout->addWidget(check);
            checks_.push_back(check);
            connect(check, &QCheckBox::toggled, this, [this](bool) { refresh_summary(); });
        }
        auto* scroll = new QScrollArea(this);
        scroll->setWidget(list);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setMinimumHeight(180);
        layout->addWidget(scroll);
        if (kind_ == datalab::domain::statistics::ControlChartKind::cusum) {
            auto* note = new QLabel(
                datalab::ui::ui_tr(
                    "CUSUM 不使用 Shewhart 特殊原因规则（单点超出 3σ 控制限等），"
                    "改为报告上侧/下侧累计和的首次信号。"),
                this);
            note->setWordWrap(true);
            layout->addWidget(note);
            select_all->setEnabled(false);
            clear_all->setEnabled(false);
        }
        connect(select_all, &QPushButton::clicked, this, [this]() { set_all_applicable(true); });
        connect(clear_all, &QPushButton::clicked, this, [this]() { set_all_applicable(false); });
        connect(restore, &QPushButton::clicked, this, [this]() { restore_defaults(); });
        refresh_summary();
    }

    QString selected_text() const
    {
        if (kind_ == datalab::domain::statistics::ControlChartKind::cusum) {
            return QStringLiteral("none");
        }
        // 勾选与「全部适用」默认一致时返回空，由 rule_policy 决定默认策略。
        bool matches_all_applicable = true;
        bool any_enabled = false;
        for (const QCheckBox* check : checks_) {
            if (!check->isEnabled()) {
                continue;
            }
            any_enabled = true;
            if (check->isChecked() != check->property("defaultValue").toBool()) {
                matches_all_applicable = false;
                break;
            }
        }
        if (any_enabled && matches_all_applicable) {
            return {};
        }
        QStringList selected;
        for (const QCheckBox* check : checks_) {
            if (check->isEnabled() && check->isChecked()) {
                selected.append(QString::number(check->property("testNumber").toInt()));
            }
        }
        return selected.isEmpty() ? QStringLiteral("none") : selected.join(QLatin1Char(' '));
    }

    void restore_defaults()
    {
        for (QCheckBox* check : checks_) {
            check->setChecked(check->property("defaultValue").toBool());
        }
        refresh_summary();
    }

private:
    void set_all_applicable(bool checked)
    {
        for (QCheckBox* check : checks_) {
            if (check->isEnabled()) {
                check->setChecked(checked);
            }
        }
        refresh_summary();
    }

    void refresh_summary()
    {
        int enabled = 0;
        int selected = 0;
        for (const QCheckBox* check : checks_) {
            if (!check->isEnabled()) {
                continue;
            }
            ++enabled;
            selected += check->isChecked() ? 1 : 0;
        }
        if (kind_ == datalab::domain::statistics::ControlChartKind::cusum) {
            summary_->setText(datalab::ui::ui_tr(
                "CUSUM 使用专用信号，不勾选 Shewhart 特殊原因规则。"));
            return;
        }
        const bool uses_policy = selected_text().isEmpty();
        summary_->setText(
            uses_policy
                ? datalab::ui::ui_tr(
                      "勾选=全部适用默认 → 由「规则默认策略」决定"
                      "（all_applicable 或 minitab_like）。已选 %1 / %2。")
                      .arg(selected).arg(enabled)
                : datalab::ui::ui_tr(
                      "已显式勾选 %1 / %2 条（覆盖策略下拉）。"
                      "多规则提高误报风险。")
                      .arg(selected).arg(enabled));
    }

    datalab::domain::statistics::ControlChartKind kind_;
    QLabel* summary_ = nullptr;
    std::vector<QCheckBox*> checks_;
};

class ResponseObjectivesEditor final : public QWidget {
public:
    explicit ResponseObjectivesEditor(QWidget* parent)
        : QWidget(parent)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);
        auto* hint = new QLabel(
            datalab::ui::ui_tr("选择多个响应后，可为每个响应指定独立目标与权重。"), this);
        hint->setWordWrap(true);
        layout->addWidget(hint);
        table_ = new QTableWidget(this);
        table_->setColumnCount(6);
        table_->setHorizontalHeaderLabels({
            datalab::ui::ui_tr("响应"), datalab::ui::ui_tr("目标"), datalab::ui::ui_tr("下限"),
            datalab::ui::ui_tr("上限"), datalab::ui::ui_tr("目标值"), datalab::ui::ui_tr("权重")});
        table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        table_->verticalHeader()->setVisible(false);
        table_->setMinimumHeight(120);
        layout->addWidget(table_);
    }

    void set_response_list(QListWidget* list)
    {
        response_list_ = list;
        if (list != nullptr && list->model() != nullptr) {
            connect(list->model(), &QAbstractItemModel::rowsInserted,
                    this, [this]() { refresh(); });
            connect(list->model(), &QAbstractItemModel::rowsRemoved,
                    this, [this]() { refresh(); });
            connect(list->model(), &QAbstractItemModel::modelReset,
                    this, [this]() { refresh(); });
        }
        refresh();
    }

    QString selected_text() const
    {
        if (table_->rowCount() < 2) {
            return QString();
        }
        QJsonArray array;
        for (int row = 0; row < table_->rowCount(); ++row) {
            QJsonObject object;
            auto* combo = qobject_cast<QComboBox*>(table_->cellWidget(row, 1));
            object.insert(QStringLiteral("goal"),
                          combo != nullptr ? combo->currentData().toString()
                                           : QStringLiteral("maximize"));
            const QString lower = cell_text(row, 2);
            const QString upper = cell_text(row, 3);
            const QString target = cell_text(row, 4);
            if (!lower.isEmpty()) {
                object.insert(QStringLiteral("lower"), lower);
            }
            if (!upper.isEmpty()) {
                object.insert(QStringLiteral("upper"), upper);
            }
            if (!target.isEmpty()) {
                object.insert(QStringLiteral("target"), target);
            }
            auto* weight = qobject_cast<QDoubleSpinBox*>(table_->cellWidget(row, 5));
            object.insert(QStringLiteral("weight"),
                          weight != nullptr ? weight->value() : 1.0);
            array.append(object);
        }
        return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
    }

private:
    QString cell_text(const int row, const int column) const
    {
        auto* edit = qobject_cast<QLineEdit*>(table_->cellWidget(row, column));
        return edit != nullptr ? edit->text().trimmed() : QString();
    }

    void refresh()
    {
        QStringList names;
        if (response_list_ != nullptr) {
            for (int row = 0; row < response_list_->count(); ++row) {
                names.push_back(response_list_->item(row)->text());
            }
        }
        const QJsonArray previous =
            QJsonDocument::fromJson(selected_text().toUtf8()).array();
        table_->setRowCount(names.size());
        for (int row = 0; row < names.size(); ++row) {
            auto* name = new QTableWidgetItem(names[row]);
            name->setFlags(name->flags() & ~Qt::ItemIsEditable);
            table_->setItem(row, 0, name);
            auto* combo = new QComboBox(table_);
            combo->addItem(datalab::ui::ui_tr("最大化"), QStringLiteral("maximize"));
            combo->addItem(datalab::ui::ui_tr("最小化"), QStringLiteral("minimize"));
            combo->addItem(datalab::ui::ui_tr("目标值"), QStringLiteral("target"));
            auto* lower = new QLineEdit(table_);
            auto* upper = new QLineEdit(table_);
            auto* target = new QLineEdit(table_);
            auto* weight = new QDoubleSpinBox(table_);
            weight->setRange(0.1, 10.0);
            weight->setSingleStep(0.1);
            weight->setValue(1.0);
            if (row < previous.size()) {
                const QJsonObject object = previous.at(row).toObject();
                const int goal_index = combo->findData(
                    object.value(QStringLiteral("goal")).toString());
                combo->setCurrentIndex(goal_index >= 0 ? goal_index : 0);
                lower->setText(object.value(QStringLiteral("lower")).toString());
                upper->setText(object.value(QStringLiteral("upper")).toString());
                target->setText(object.value(QStringLiteral("target")).toString());
                if (object.contains(QStringLiteral("weight"))) {
                    weight->setValue(object.value(QStringLiteral("weight")).toDouble(1.0));
                }
            }
            table_->setCellWidget(row, 1, combo);
            table_->setCellWidget(row, 2, lower);
            table_->setCellWidget(row, 3, upper);
            table_->setCellWidget(row, 4, target);
            table_->setCellWidget(row, 5, weight);
        }
    }

    QListWidget* response_list_ = nullptr;
    QTableWidget* table_ = nullptr;
};

}  // namespace

AnalysisSetupDialog::AnalysisSetupDialog(
    const QString& title,
    const QStringList& column_labels,
    QWidget* parent,
    const QString& icon_resource,
    const std::vector<datalab::domain::ColumnType>& column_types)
    : QDialog(parent)
    , column_labels_(column_labels)
    , column_types_(column_types)
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
        "QDialog { background: #eef3f5; color: #29434e; }"
        "QLabel { color: #49636d; }"
        "QListWidget { background: #ffffff; border: 1px solid #cbd9de;"
        " border-radius: 5px; padding: 4px; color: #29434e; }"
        "QListWidget::item { padding: 6px; border-radius: 4px; }"
        "QListWidget::item:hover { background: #eaf6f6; }"
        "QListWidget::item:selected { background: #d8eeee; color: #147d85; }"
        "QPushButton { background: #ffffff; color: #39717a; border: 1px solid #b9d2d7;"
        " border-radius: 5px; padding: 7px 14px; }"
        "QPushButton:hover { background: #eaf6f6; border-color: #42aeb4; }"
        "QPushButton#run_button { background: #188a91; color: #ffffff; border: 0;"
        " font-weight: 600; }"
        "QPushButton#run_button:hover { background: #14777d; }"
        "QDialogButtonBox QPushButton { min-width: 84px; }"
        "QLineEdit { background: #ffffff; border: 1px solid #cbd9de;"
        " border-radius: 5px; padding: 7px 9px; }"
        "QLineEdit:focus { border-color: #42aeb4; }"
        "QLineEdit[validationError=\"true\"], QListWidget[validationError=\"true\"],"
        " QSpinBox[validationError=\"true\"], QDoubleSpinBox[validationError=\"true\"],"
        " QComboBox[validationError=\"true\"] { border: 2px solid #d32f2f; }"
        "QSpinBox, QDoubleSpinBox, QComboBox { min-height: 34px; }"
        "QCheckBox { min-height: 34px; }"
        "QFrame#dialog_card { background:#ffffff; border:1px solid #d7e3e6;"
        " border-radius:9px; }"
        "QToolButton#advanced_toggle { color:#2d6971; font-weight:600;"
        " padding:8px 4px; text-align:left; }"));

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
    error_banner_ = new QLabel(this);
    error_banner_->setObjectName(QStringLiteral("error_banner"));
    error_banner_->setWordWrap(true);
    error_banner_->setStyleSheet(QStringLiteral(
        "background:#fff0f0; color:#b3261e; border:1px solid #ef9a9a;"
        " border-radius:5px; padding:8px 10px;"));
    error_banner_->hide();
    root->addWidget(error_banner_);
    auto* content = new QHBoxLayout();
    content->setSpacing(14);
    auto* left_panel = new QFrame(this);
    left_panel->setObjectName(QStringLiteral("dialog_card"));
    auto* left = new QVBoxLayout(left_panel);
    left->setContentsMargins(16, 16, 16, 16);
    left->setSpacing(8);
    auto* source_title = new QLabel(datalab::ui::ui_tr("工作表列"), this);
    source_title->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 600; color: #29434e;"));
    left->addWidget(source_title);
    auto* search = new QLineEdit(this);
    search->setPlaceholderText(datalab::ui::ui_tr("搜索列名…"));
    search->setClearButtonEnabled(true);
    left->addWidget(search);
    available_ = new QListWidget(this);
    for (int index = 0; index < column_labels_.size(); ++index) {
        QString label = column_labels_.at(index);
        if (index >= 0 && index < static_cast<int>(column_types_.size())) {
            label += QStringLiteral("  [") + column_type_display(
                column_types_[static_cast<std::size_t>(index)]) + QStringLiteral("]");
        }
        auto* item = new QListWidgetItem(label, available_);
        item->setData(Qt::UserRole, index);
    }
    available_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    left->addWidget(available_);
    auto* select = new QPushButton(datalab::ui::ui_tr("选择 >"), this);
    select->setIcon(QIcon(QStringLiteral(":/icons/import-data.svg")));
    left->addWidget(select);
    auto* remove = new QPushButton(datalab::ui::ui_tr("移除选中列"), this);
    remove->setIcon(QIcon(QStringLiteral(":/icons/error.svg")));
    left->addWidget(remove);
    left->addWidget(new QLabel(
        datalab::ui::ui_tr("先点击右侧角色框，再点“选择 >”或双击左侧列。"), this));

    auto* right_panel = new QFrame(this);
    right_panel->setObjectName(QStringLiteral("dialog_card"));
    auto* right = new QVBoxLayout(right_panel);
    right->setContentsMargins(16, 16, 16, 16);
    right->setSpacing(8);
    auto* settings_title = new QLabel(datalab::ui::ui_tr("分析设置"), this);
    settings_title->setStyleSheet(QStringLiteral(
        "font-size: 15px; font-weight: 600; color: #29434e;"));
    right->addWidget(settings_title);
    auto* settings_content = new QWidget(this);
    auto* settings_layout = new QVBoxLayout(settings_content);
    settings_layout->setContentsMargins(8, 8, 8, 8);
    roles_layout_ = new QFormLayout();
    roles_layout_->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    roles_layout_->setFormAlignment(Qt::AlignTop);
    roles_layout_->setVerticalSpacing(10);
    roles_layout_->setHorizontalSpacing(12);
    roles_layout_->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    settings_layout->addLayout(roles_layout_);
    advanced_toggle_ = new QToolButton(this);
    advanced_toggle_->setText(datalab::ui::ui_tr("高级选项"));
    advanced_toggle_->setCheckable(true);
    advanced_toggle_->setObjectName(QStringLiteral("advanced_toggle"));
    advanced_toggle_->setChecked(false);
    advanced_toggle_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    advanced_toggle_->setArrowType(Qt::RightArrow);
    settings_layout->addWidget(advanced_toggle_);
    advanced_panel_ = new QWidget(this);
    advanced_layout_ = new QFormLayout(advanced_panel_);
    advanced_layout_->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    advanced_layout_->setFormAlignment(Qt::AlignTop);
    advanced_layout_->setVerticalSpacing(10);
    advanced_layout_->setHorizontalSpacing(12);
    advanced_layout_->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    advanced_panel_->setVisible(false);
    settings_layout->addWidget(advanced_panel_);
    settings_layout->addStretch();
    auto* settings_scroll = new QScrollArea(this);
    settings_scroll->setWidgetResizable(true);
    settings_scroll->setFrameShape(QFrame::NoFrame);
    settings_scroll->setWidget(settings_content);
    right->addWidget(settings_scroll, 1);

    content->addWidget(left_panel, 1);
    content->addWidget(right_panel, 2);
    root->addLayout(content, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* reset = new QPushButton(datalab::ui::ui_tr("重置默认值"), this);
    buttons->addButton(reset, QDialogButtonBox::ResetRole);
    root->addWidget(buttons);

    const auto ok_button = buttons->button(QDialogButtonBox::Ok);
    const auto cancel_button = buttons->button(QDialogButtonBox::Cancel);
    if (ok_button != nullptr) {
        ok_button->setObjectName(QStringLiteral("run_button"));
        ok_button->setText(datalab::ui::ui_tr("运行分析"));
        ok_button->setIcon(QIcon(QStringLiteral(":/icons/success.svg")));
    }
    if (cancel_button != nullptr) {
        cancel_button->setText(datalab::ui::ui_tr("取消"));
        cancel_button->setIcon(QIcon(QStringLiteral(":/icons/error.svg")));
    }

    connect(select, &QPushButton::clicked, this, &AnalysisSetupDialog::select_into_role);
    connect(remove, &QPushButton::clicked, this, &AnalysisSetupDialog::remove_from_role);
    connect(search, &QLineEdit::textChanged, this, [this](const QString& text) {
        for (int row = 0; row < available_->count(); ++row) {
            available_->item(row)->setHidden(
                !available_->item(row)->text().contains(text, Qt::CaseInsensitive));
        }
    });
    connect(available_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        select_into_role();
    });
    connect(buttons, &QDialogButtonBox::accepted,
            this, &AnalysisSetupDialog::validate_and_accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(reset, &QPushButton::clicked, this, &AnalysisSetupDialog::reset_defaults);
    connect(advanced_toggle_, &QToolButton::toggled, this, [this](bool expanded) {
        advanced_panel_->setVisible(expanded);
        advanced_toggle_->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
    });
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
    add_role(analysis_commands::RoleSpec{id, label, multi, optional});
}

void AnalysisSetupDialog::add_role(const analysis_commands::RoleSpec& spec)
{
    auto* list = new RoleListWidget(this);
    list->setMinimumHeight(spec.multi ? 96 : 48);
    list->setMaximumHeight(spec.multi ? 180 : 56);
    list->setObjectName(spec.id);
    list->setProperty("optional", spec.optional);
    list->setProperty("multi", spec.multi);
    list->setProperty("minimumCount",
                      spec.minimum_count > 0 ? spec.minimum_count
                                             : (spec.optional ? 0 : 1));
    list->setProperty("maximumCount", spec.maximum_count);
    QStringList allowed_types;
    for (const datalab::domain::ColumnType type : spec.allowed_types) {
        allowed_types.push_back(column_type_label(type));
    }
    if (allowed_types.isEmpty()) {
        const QString normalized_id = spec.id.toLower();
        if (normalized_id == QStringLiteral("variables")
            || normalized_id == QStringLiteral("response")
            || normalized_id == QStringLiteral("measurement")
            || normalized_id == QStringLiteral("x")
            || normalized_id == QStringLiteral("y")) {
            allowed_types.push_back(QStringLiteral("数值"));
        } else if (normalized_id == QStringLiteral("time")
                   || normalized_id == QStringLiteral("time_column")) {
            allowed_types.push_back(QStringLiteral("时间"));
        }
    }
    list->setProperty("allowedTypes", allowed_types);
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
    const QString display_label = datalab::ui::ui_tr(spec.label);
    const QString caption = spec.optional
        ? display_label + datalab::ui::ui_tr("（可选）")
        : display_label;
    roles_layout_->addRow(
        make_field_label(display_label, QStringLiteral(":/icons/data-table.svg"),
                         spec.optional, this),
        list);
    list->setToolTip(caption);
    list->setAccessibleName(caption);
}

QWidget* AnalysisSetupDialog::add_input(const analysis_commands::InputSpec& spec)
{
    QWidget* editor = nullptr;
    const bool optional = spec.placeholder.contains(QStringLiteral("可选"))
        || spec.placeholder.contains(QStringLiteral("留空"));
    if (spec.kind == analysis_commands::InputKind::integer && !optional) {
        auto* spin = new QSpinBox(this);
        spin->setRange(
            spec.minimum.has_value() ? static_cast<int>(*spec.minimum) : -1000000000,
            spec.maximum.has_value() ? static_cast<int>(*spec.maximum) : 1000000000);
        bool ok = false;
        const int default_value = spec.placeholder.toInt(&ok);
        if (ok) {
            spin->setValue(default_value);
        }
        spin->setProperty("defaultValue", spin->value());
        editor = spin;
    } else if ((spec.kind == analysis_commands::InputKind::number
                || spec.kind == analysis_commands::InputKind::percentage)
               && !optional) {
        auto* spin = new QDoubleSpinBox(this);
        spin->setDecimals(6);
        spin->setRange(
            spec.minimum.has_value() ? *spec.minimum : -1.0e12,
            spec.maximum.has_value() ? *spec.maximum : 1.0e12);
        spin->setSingleStep(spec.kind == analysis_commands::InputKind::percentage
                                ? 1.0 : 0.1);
        bool ok = false;
        const double default_value = spec.placeholder.toDouble(&ok);
        if (ok) {
            spin->setValue(default_value);
        }
        spin->setProperty("defaultValue", spin->value());
        editor = spin;
    } else if (spec.kind == analysis_commands::InputKind::choice
               && !spec.choices.empty()) {
        auto* combo = new QComboBox(this);
        for (const auto& [value, label] : spec.choices) {
            combo->addItem(datalab::ui::ui_tr(label), value);
        }
        combo->setCurrentIndex(std::max(0, combo->findData(spec.placeholder)));
        combo->setProperty("defaultIndex", combo->currentIndex());
        editor = combo;
    } else if (spec.kind == analysis_commands::InputKind::boolean) {
        auto* check = new QCheckBox(this);
        check->setChecked(spec.placeholder == QStringLiteral("true"));
        check->setProperty("defaultValue", check->isChecked());
        editor = check;
    } else if (spec.kind == analysis_commands::InputKind::special_cause_tests) {
        editor = new SpecialCauseTestsEditor(spec.placeholder, this);
    } else if (spec.kind == analysis_commands::InputKind::response_objectives) {
        auto* objectives = new ResponseObjectivesEditor(this);
        if (auto* list = findChild<QListWidget*>(QStringLiteral("response"))) {
            objectives->set_response_list(list);
        }
        editor = objectives;
    } else {
        auto* line = new QLineEdit(this);
        line->setPlaceholderText(spec.placeholder);
        const bool numeric = spec.kind == analysis_commands::InputKind::number
            || spec.kind == analysis_commands::InputKind::percentage;
        bool ok = false;
        if (numeric && !optional) {
            const double default_value = spec.placeholder.toDouble(&ok);
            if (ok) {
                line->setText(QString::number(default_value, 'g', 12));
            }
        }
        if (spec.kind == analysis_commands::InputKind::integer) {
            line->setValidator(new QIntValidator(
                spec.minimum.has_value() ? static_cast<int>(*spec.minimum) : -1000000000,
                spec.maximum.has_value() ? static_cast<int>(*spec.maximum) : 1000000000,
                line));
        } else if (numeric) {
            line->setValidator(new QDoubleValidator(
                spec.minimum.has_value() ? *spec.minimum : -1.0e12,
                spec.maximum.has_value() ? *spec.maximum : 1.0e12,
                8, line));
        }
        line->setProperty("defaultText", line->text());
        editor = line;
    }

    const QString display_label = datalab::ui::ui_tr(spec.label);
    editor->setObjectName(spec.id);
    editor->setToolTip(spec.help.isEmpty()
                           ? display_label + datalab::ui::ui_tr("，默认值：") + spec.placeholder
                           : spec.help);
    editor->setAccessibleName(display_label);
    editor->setMinimumHeight(34);
    QFormLayout* target_layout = spec.advanced ? advanced_layout_ : roles_layout_;
    target_layout->addRow(
        make_field_label(display_label, QStringLiteral(":/icons/settings.svg"), optional, this),
        editor);
    if (spec.id == QStringLiteral("historical_center")) {
        if (auto* sigma = findChild<QWidget*>(QStringLiteral("historical_sigma_z"))) {
            sigma->setEnabled(!line_text(spec.id).isEmpty());
        }
    } else if (spec.id == QStringLiteral("historical_sigma_z")) {
        if (auto* center = findChild<QLineEdit*>(QStringLiteral("historical_center"))) {
            editor->setEnabled(!center->text().trimmed().isEmpty());
            connect(center, &QLineEdit::textChanged, editor,
                    [editor](const QString& text) {
                        editor->setEnabled(!text.trimmed().isEmpty());
                    });
        }
    }
    return editor;
}

QWidget* AnalysisSetupDialog::add_line_edit(
    const QString& id,
    const QString& label,
    const QString& placeholder)
{
    analysis_commands::InputSpec spec{id, label, placeholder};
    return add_input(spec);
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
        const int column = item->data(Qt::UserRole).toInt();
        bool already_selected = false;
        for (int row = 0; row < target->count(); ++row) {
            already_selected = target->item(row)->data(Qt::UserRole).toInt() == column;
            if (already_selected) {
                break;
            }
        }
        if (already_selected) {
            continue;
        }
        auto* copy = new QListWidgetItem(item->text());
        copy->setData(Qt::UserRole, column);
        const QStringList allowed = target->property("allowedTypes").toStringList();
        if (!allowed.isEmpty() && column >= 0
            && column < static_cast<int>(column_types_.size())) {
            const QString actual = column_type_label(
                column_types_[static_cast<std::size_t>(column)]);
            if (!allowed.contains(actual)) {
                copy->setForeground(QColor(QStringLiteral("#a15c00")));
                QStringList allowed_display;
                for (const QString& token : allowed) {
                    allowed_display.push_back(translate_type_token(token));
                }
                const QString type_join = QStringLiteral(", ");
                copy->setToolTip(
                    datalab::ui::ui_tr(
                        "列类型为“%1”，与角色建议的类型（%2）不完全匹配；"
                        "命令仍会在提交时进行最终校验。")
                        .arg(translate_type_token(actual),
                             allowed_display.join(type_join)));
            }
        }
        target->addItem(copy);
        if (!multi) {
            break;
        }
    }
}

void AnalysisSetupDialog::remove_from_role()
{
    if (active_role_ == nullptr) {
        return;
    }
    const QList<QListWidgetItem*> selected = active_role_->selectedItems();
    for (QListWidgetItem* item : selected) {
        delete item;
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
    if (edit != nullptr) {
        return edit->text().trimmed();
    }
    if (const auto* spin = findChild<QSpinBox*>(id)) {
        return QString::number(spin->value());
    }
    if (const auto* spin = findChild<QDoubleSpinBox*>(id)) {
        return QString::number(spin->value(), 'g', 12);
    }
    if (const auto* combo = findChild<QComboBox*>(id)) {
        return combo->currentData().toString().isEmpty()
            ? combo->currentText() : combo->currentData().toString();
    }
    if (const auto* check = findChild<QCheckBox*>(id)) {
        return check->isChecked() ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (const auto* tests =
            dynamic_cast<const SpecialCauseTestsEditor*>(findChild<QWidget*>(id))) {
        return tests->selected_text();
    }
    if (const auto* objectives =
            dynamic_cast<const ResponseObjectivesEditor*>(findChild<QWidget*>(id))) {
        return objectives->selected_text();
    }
    return {};
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

void AnalysisSetupDialog::set_accept_validator(
    std::function<bool(QString*, QString*)> validator)
{
    accept_validator_ = std::move(validator);
}

void AnalysisSetupDialog::set_field_error(const QString& id, const QString& message)
{
    error_banner_->setText(message);
    error_banner_->show();
    QWidget* field = findChild<QWidget*>(id);
    if (field == nullptr) {
        return;
    }
    field->setProperty("validationError", true);
    field->setToolTip(message);
    field->style()->unpolish(field);
    field->style()->polish(field);
    field->setFocus();
}

void AnalysisSetupDialog::clear_errors()
{
    error_banner_->clear();
    error_banner_->hide();
    for (QWidget* field : findChildren<QWidget*>()) {
        if (field->property("validationError").toBool()) {
            field->setProperty("validationError", false);
            field->style()->unpolish(field);
            field->style()->polish(field);
        }
    }
}

void AnalysisSetupDialog::reset_defaults()
{
    clear_errors();
    for (QLineEdit* edit : findChildren<QLineEdit*>()) {
        edit->setText(edit->property("defaultText").toString());
    }
    for (QSpinBox* spin : findChildren<QSpinBox*>()) {
        spin->setValue(spin->property("defaultValue").toInt());
    }
    for (QDoubleSpinBox* spin : findChildren<QDoubleSpinBox*>()) {
        spin->setValue(spin->property("defaultValue").toDouble());
    }
    for (QComboBox* combo : findChildren<QComboBox*>()) {
        combo->setCurrentIndex(combo->property("defaultIndex").toInt());
    }
    for (QCheckBox* check : findChildren<QCheckBox*>()) {
        if (check->property("defaultValue").isValid()) {
            check->setChecked(check->property("defaultValue").toBool());
        }
    }
    for (QWidget* field : findChildren<QWidget*>()) {
        if (auto* tests = dynamic_cast<SpecialCauseTestsEditor*>(field)) {
            tests->restore_defaults();
        }
    }
}

void AnalysisSetupDialog::validate_and_accept()
{
    clear_errors();
    for (QListWidget* role : findChildren<QListWidget*>()) {
        if (role == available_) {
            continue;
        }
        const int minimum = role->property("minimumCount").toInt();
        const int maximum = role->property("maximumCount").toInt();
        if (role->count() < minimum) {
            set_field_error(
                role->objectName(),
                datalab::ui::ui_tr("此角色至少需要选择 %1 列。").arg(minimum));
            return;
        }
        if (maximum > 0 && role->count() > maximum) {
            set_field_error(
                role->objectName(),
                datalab::ui::ui_tr("此角色最多只能选择 %1 列。").arg(maximum));
            return;
        }
    }
    if (accept_validator_ == nullptr) {
        accept();
        return;
    }
    QString error_title;
    QString error_message;
    if (accept_validator_(&error_title, &error_message)) {
        accept();
        return;
    }
    error_banner_->setText(
        error_title.isEmpty() ? error_message : error_title + QStringLiteral("：") + error_message);
    error_banner_->show();
}
