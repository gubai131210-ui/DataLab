#include "ui/learning_center_page.h"

#include "application/learning/learning_dataset_store.h"
#include "application/learning/learning_tutorial_catalog.h"

#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSplitter>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

using datalab::application::learning::LearningBuriedSignal;
using datalab::application::learning::LearningDatasetStore;
using datalab::application::learning::LearningDatasetSummary;
using datalab::application::learning::LearningDialogFillDetail;
using datalab::application::learning::LearningFadeLevel;
using datalab::application::learning::LearningGlossaryItem;
using datalab::application::learning::LearningMisconception;
using datalab::application::learning::LearningOutputGuideItem;
using datalab::application::learning::LearningPrereqItem;
using datalab::application::learning::LearningSelfExplain;
using datalab::application::learning::LearningTutorialCatalog;
using datalab::application::learning::LearningTutorialEntry;

namespace {

QString table_html(const QStringList& headers, const QVector<QStringList>& rows)
{
    QString html = QStringLiteral(
        "<table style=\"border-collapse:collapse; width:100%;\">"
        "<tr>");
    for (const QString& header : headers) {
        html += QStringLiteral(
                    "<th style=\"border:1px solid #cbd9de; background:#eef6f6; "
                    "padding:4px 8px; text-align:left;\">%1</th>")
                    .arg(header.toHtmlEscaped());
    }
    html += QStringLiteral("</tr>");
    for (const QStringList& row : rows) {
        html += QStringLiteral("<tr>");
        for (const QString& cell : row) {
            html += QStringLiteral(
                        "<td style=\"border:1px solid #cbd9de; padding:4px 8px; "
                        "vertical-align:top;\">%1</td>")
                        .arg(cell.toHtmlEscaped());
        }
        html += QStringLiteral("</tr>");
    }
    html += QStringLiteral("</table>");
    return html;
}

QLabel* rich_label(const QString& html, QWidget* parent)
{
    auto* label = new QLabel(html, parent);
    label->setTextFormat(Qt::RichText);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setOpenExternalLinks(true);
    return label;
}

QWidget* empty_block(const QString& message, QWidget* parent)
{
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    auto* label = new QLabel(message, wrap);
    label->setWordWrap(true);
    label->setStyleSheet(QStringLiteral("color:#5a6d73;"));
    layout->addWidget(label);
    return wrap;
}

QWidget* make_list_block(const QStringList& items, QWidget* parent)
{
    if (items.isEmpty()) {
        return empty_block(QStringLiteral("本课无此块"), parent);
    }
    QString html = QStringLiteral("<ol>");
    for (const QString& item : items) {
        html += QStringLiteral("<li>%1</li>").arg(item.toHtmlEscaped());
    }
    html += QStringLiteral("</ol>");
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(rich_label(html, wrap));
    return wrap;
}

class CollapsibleSection final : public QWidget {
public:
    CollapsibleSection(const QString& title, QWidget* body, bool expanded, QWidget* parent)
        : QWidget(parent)
        , toggle_(new QToolButton(this))
        , body_(body)
    {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 8);
        layout->setSpacing(2);

        toggle_->setObjectName(QStringLiteral("learning_section_toggle"));
        toggle_->setCheckable(true);
        toggle_->setChecked(expanded);
        toggle_->setText(title);
        toggle_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toggle_->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
        toggle_->setAutoRaise(true);
        toggle_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        toggle_->setStyleSheet(QStringLiteral(
            "QToolButton#learning_section_toggle { color:#2d6971; font-weight:600; "
            "text-align:left; padding:6px 4px; }"));

        body_->setParent(this);
        body_->setVisible(expanded);
        auto* frame = new QFrame(this);
        frame->setObjectName(QStringLiteral("learning_section_body"));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setStyleSheet(QStringLiteral(
            "QFrame#learning_section_body { background:#f7fbfb; border:1px solid #d5e3e6; }"));
        auto* frame_layout = new QVBoxLayout(frame);
        frame_layout->setContentsMargins(10, 8, 10, 8);
        frame_layout->addWidget(body_);
        frame->setVisible(expanded);

        layout->addWidget(toggle_);
        layout->addWidget(frame);

        connect(toggle_, &QToolButton::toggled, this, [this, frame](bool on) {
            body_->setVisible(on);
            frame->setVisible(on);
            toggle_->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
        });
    }

    QToolButton* toggle_button() const { return toggle_; }

    void set_expanded(bool expanded)
    {
        toggle_->setChecked(expanded);
    }

private:
    QToolButton* toggle_ = nullptr;
    QWidget* body_ = nullptr;
};

QWidget* glossary_block(const LearningTutorialEntry& entry, QWidget* parent)
{
    if (entry.glossary.isEmpty()) {
        return empty_block(QStringLiteral("本课无此块"), parent);
    }
    QVector<QStringList> rows;
    for (const LearningGlossaryItem& item : entry.glossary) {
        rows.push_back({item.term, item.plain, item.remember});
    }
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(rich_label(
        QStringLiteral("<p>首次出现的缩写先读这张表。控制限课请盯住 <b>UCL ≠ USL</b>。</p>%1")
            .arg(table_html({QStringLiteral("术语"), QStringLiteral("白话"), QStringLiteral("怎么记")},
                            rows)),
        wrap));
    return wrap;
}

QWidget* background_block(const LearningTutorialEntry& entry, QWidget* parent)
{
    if (entry.used_for.isEmpty() && entry.not_for.isEmpty() && entry.scenario.isEmpty()
        && entry.skill_mission.isEmpty()) {
        return empty_block(QStringLiteral("本课无此块"), parent);
    }
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    if (!entry.skill_mission.isEmpty()) {
        layout->addWidget(rich_label(
            QStringLiteral("<p><b>本课技能目标：</b>%1</p>").arg(entry.skill_mission.toHtmlEscaped()),
            wrap));
    }
    layout->addWidget(rich_label(
        QStringLiteral("<p><b>问题 / 用途：</b>%1</p>").arg(entry.used_for.toHtmlEscaped()), wrap));
    layout->addWidget(rich_label(
        QStringLiteral("<p><b>本课不回答：</b>%1</p>").arg(entry.not_for.toHtmlEscaped()), wrap));
    layout->addWidget(rich_label(
        QStringLiteral("<p><b>现场故事（Y / DMAIC 线索）：</b>%1</p>")
            .arg(entry.scenario.toHtmlEscaped()),
        wrap));
    return wrap;
}

QWidget* dataset_block(const LearningTutorialEntry& entry,
                       const QString& worksheet_name,
                       const LearningDatasetSummary* summary,
                       QWidget* parent)
{
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    if (!entry.dataset_id.has_value()) {
        layout->addWidget(empty_block(QStringLiteral("本命令无需导入演示数据。"), wrap));
        return wrap;
    }
    layout->addWidget(rich_label(
        QStringLiteral("<p>工作表显示名 <code>%1</code>（主键 <code>%2</code>）。"
                       "导入走学习中心按钮，不会覆盖当前工作表。</p>")
            .arg(worksheet_name.toHtmlEscaped(), entry.dataset_id.value().toHtmlEscaped()),
        wrap));
    if (summary != nullptr) {
        if (!summary->story.isEmpty()) {
            layout->addWidget(rich_label(
                QStringLiteral("<p>%1</p>").arg(summary->story.toHtmlEscaped()), wrap));
        }
        if (!summary->notes.isEmpty()) {
            layout->addWidget(rich_label(
                QStringLiteral("<p><b>数据集 notes：</b>%1</p>").arg(summary->notes.toHtmlEscaped()),
                wrap));
        }
        layout->addWidget(rich_label(
            QStringLiteral("<p>行数 %1。</p>").arg(summary->row_count), wrap));
    }
    if (!entry.dialog_fill.isEmpty()) {
        QVector<QStringList> role_rows;
        for (auto it = entry.dialog_fill.cbegin(); it != entry.dialog_fill.cend(); ++it) {
            role_rows.push_back({it.key(), it.value()});
        }
        layout->addWidget(rich_label(
            QStringLiteral("<p><b>列角色（dialog_fill）</b></p>%1")
                .arg(table_html({QStringLiteral("角色"), QStringLiteral("列名")}, role_rows)),
            wrap));
    }
    if (entry.buried_signals.isEmpty()) {
        layout->addWidget(empty_block(QStringLiteral("本课无埋点表"), wrap));
    } else {
        QVector<QStringList> signal_rows;
        for (const LearningBuriedSignal& signal : entry.buried_signals) {
            signal_rows.push_back({QString::number(signal.row), signal.what, signal.expect});
        }
        layout->addWidget(rich_label(
            QStringLiteral("<p><b>埋入信号（1-based 行号 / 片号）</b></p>%1")
                .arg(table_html(
                    {QStringLiteral("行"), QStringLiteral("埋了什么"), QStringLiteral("图上期望")},
                    signal_rows)),
            wrap));
    }
    return wrap;
}

QWidget* why_tool_block(const LearningTutorialEntry& entry, QWidget* parent)
{
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    if (entry.used_for.isEmpty() && entry.not_for.isEmpty() && entry.related_ids.isEmpty()) {
        layout->addWidget(empty_block(QStringLiteral("本课无此块"), wrap));
        return wrap;
    }
    layout->addWidget(rich_label(
        QStringLiteral("<p>%1</p><p><b>边界：</b>%2</p>")
            .arg(entry.used_for.toHtmlEscaped(), entry.not_for.toHtmlEscaped()),
        wrap));
    if (!entry.menu_path.isEmpty()) {
        layout->addWidget(rich_label(
            QStringLiteral("<p>菜单：%1</p>").arg(entry.menu_path.toHtmlEscaped()), wrap));
    }
    if (!entry.related_ids.isEmpty()) {
        layout->addWidget(rich_label(
            QStringLiteral("<p>对比课（不是共享表）：%1</p>")
                .arg(entry.related_ids.join(QStringLiteral("、")).toHtmlEscaped()),
            wrap));
    }
    return wrap;
}

QWidget* steps_block(const LearningTutorialEntry& entry, QWidget* parent)
{
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(make_list_block(entry.click_steps, wrap));
    if (entry.dialog_fill_detail.isEmpty()) {
        layout->addWidget(empty_block(QStringLiteral("本课无对话框详解表"), wrap));
        return wrap;
    }
    QVector<QStringList> rows;
    for (const LearningDialogFillDetail& item : entry.dialog_fill_detail) {
        rows.push_back({item.field, item.put, item.meaning});
    }
    layout->addWidget(rich_label(
        QStringLiteral("<p><b>对话框字段：填什么 / 代表什么</b></p>%1")
            .arg(table_html(
                {QStringLiteral("字段"), QStringLiteral("填什么"), QStringLiteral("代表什么")},
                rows)),
        wrap));
    return wrap;
}

QWidget* output_block(const LearningTutorialEntry& entry, QWidget* parent)
{
    if (entry.output_guide.isEmpty()) {
        return empty_block(QStringLiteral("本课无此块"), parent);
    }
    QVector<QStringList> rows;
    for (const LearningOutputGuideItem& item : entry.output_guide) {
        rows.push_back({item.name, item.meaning});
    }
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(rich_label(
        QStringLiteral("<p>对着埋点读图。禁止「过程合格 / 必须停线 / 已证明正态 / 点出 UCL=废品」。"
                       "</p>%1")
            .arg(table_html({QStringLiteral("输出"), QStringLiteral("怎么读")}, rows)),
        wrap));
    return wrap;
}

QWidget* prereq_block(const QVector<LearningPrereqItem>& items, QWidget* parent)
{
    if (items.isEmpty()) {
        return empty_block(QStringLiteral("本课无此块"), parent);
    }
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(new QLabel(QStringLiteral("选完对照反馈；选错请回到 §0 关键词。"), wrap));
    for (const LearningPrereqItem& item : items) {
        auto* card = new QFrame(wrap);
        auto* card_layout = new QVBoxLayout(card);
        auto* question = new QLabel(item.q, card);
        question->setWordWrap(true);
        card_layout->addWidget(question);
        auto* good_button = new QPushButton(item.good, card);
        auto* bad_button = new QPushButton(item.bad, card);
        if ((qHash(item.q) & 1U) != 0U) {
            card_layout->addWidget(bad_button);
            card_layout->addWidget(good_button);
        } else {
            card_layout->addWidget(good_button);
            card_layout->addWidget(bad_button);
        }
        auto* feedback = new QLabel(card);
        feedback->setWordWrap(true);
        card_layout->addWidget(feedback);
        QObject::connect(good_button, &QPushButton::clicked, wrap,
                         [feedback, good = item.good]() {
                             feedback->setText(
                                 QStringLiteral("正确。本课期望理解：%1。可继续读 §0 关键词。")
                                     .arg(good));
                             feedback->setStyleSheet(QStringLiteral("color:#1b6b4a;"));
                         });
        QObject::connect(bad_button, &QPushButton::clicked, wrap,
                         [feedback, bad = item.bad, good = item.good]() {
                             feedback->setText(
                                 QStringLiteral("请回到 §0 对照：常见误解是「%1」；"
                                                "更稳妥的理解是「%2」。")
                                     .arg(bad, good));
                             feedback->setStyleSheet(QStringLiteral("color:#a33b3b;"));
                         });
        layout->addWidget(card);
    }
    return wrap;
}

QString self_explain_hint(const LearningSelfExplain& item)
{
    if (item.prompt.contains(QStringLiteral("Nelson"))) {
        return QStringLiteral(
            "参考：本课关闭 Nelson estimate，避免过大 MR 被剔除后改写教学用 UCL/LCL。"
            "σ 仍走平均移动极差。作答留在本窗口，不写回数据库。");
    }
    return QStringLiteral(
        "参考：对照本题提示「%1」、§0 关键词、§4 参数表与 §5 读图。"
        "作答仅留在本窗口，不写回数据库。")
        .arg(item.prompt);
}

QWidget* self_explain_block(const QVector<LearningSelfExplain>& items, QWidget* parent)
{
    if (items.isEmpty()) {
        return empty_block(QStringLiteral("本课无此块"), parent);
    }
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    for (const LearningSelfExplain& item : items) {
        auto* card = new QFrame(wrap);
        auto* card_layout = new QVBoxLayout(card);
        auto* after = new QLabel(
            QStringLiteral("跟在「%1」之后").arg(item.after), card);
        after->setWordWrap(true);
        auto* prompt = new QLabel(item.prompt, card);
        prompt->setWordWrap(true);
        auto* edit = new QPlainTextEdit(card);
        edit->setPlaceholderText(QStringLiteral("先自己写，再点揭晓参考"));
        edit->setFixedHeight(72);
        auto* reveal = new QPushButton(QStringLiteral("揭晓参考"), card);
        auto* hint = new QLabel(card);
        hint->setWordWrap(true);
        hint->hide();
        const QString hint_text = self_explain_hint(item);
        QObject::connect(reveal, &QPushButton::clicked, wrap, [hint, hint_text]() {
            hint->setText(hint_text);
            hint->show();
        });
        card_layout->addWidget(after);
        card_layout->addWidget(prompt);
        card_layout->addWidget(edit);
        card_layout->addWidget(reveal);
        card_layout->addWidget(hint);
        layout->addWidget(card);
    }
    return wrap;
}

QWidget* fade_block(const QVector<LearningFadeLevel>& items, QWidget* parent)
{
    if (items.isEmpty()) {
        return empty_block(QStringLiteral("本课无此块"), parent);
    }
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    auto* combo = new QComboBox(wrap);
    auto* student = new QLabel(wrap);
    auto* scaffold = new QLabel(wrap);
    student->setWordWrap(true);
    scaffold->setWordWrap(true);
    for (const LearningFadeLevel& item : items) {
        combo->addItem(QStringLiteral("level %1").arg(item.level), QVariant::fromValue(item.level));
    }
    auto refresh = [combo, student, scaffold, items]() {
        const int level = combo->currentData().toInt();
        for (const LearningFadeLevel& item : items) {
            if (item.level == level) {
                student->setText(QStringLiteral("学员任务：%1").arg(item.student));
                scaffold->setText(QStringLiteral("仍保留的脚手架：%1").arg(item.scaffold));
                return;
            }
        }
    };
    QObject::connect(
        combo,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        wrap,
        [refresh](int) { refresh(); });
    layout->addWidget(new QLabel(QStringLiteral("0 完整例 → 1 完成题 → 2 独立练（换练习表）"), wrap));
    layout->addWidget(combo);
    layout->addWidget(student);
    layout->addWidget(scaffold);
    combo->setCurrentIndex(0);
    refresh();
    return wrap;
}

QWidget* retrieval_block(const QStringList& items, QWidget* parent)
{
    if (items.isEmpty()) {
        return empty_block(QStringLiteral("本课无此块"), parent);
    }
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    int index = 1;
    for (const QString& question : items) {
        auto* card = new QFrame(wrap);
        auto* card_layout = new QVBoxLayout(card);
        auto* label = new QLabel(QStringLiteral("%1. %2").arg(index).arg(question), card);
        label->setWordWrap(true);
        auto* edit = new QPlainTextEdit(card);
        edit->setPlaceholderText(QStringLiteral("合上教程作答（不写回数据库）"));
        edit->setFixedHeight(64);
        auto* reveal = new QPushButton(QStringLiteral("揭晓对照方向"), card);
        auto* hint = new QLabel(card);
        hint->setWordWrap(true);
        hint->hide();
        QObject::connect(reveal, &QPushButton::clicked, wrap, [hint, question]() {
            hint->setText(
                QStringLiteral("对照本题「%1」与 §0 关键词、§2 埋点行号、§5 读图；"
                               "控制限课请再核对 UCL≠USL。答案不写回 sqlite。")
                    .arg(question));
            hint->show();
        });
        card_layout->addWidget(label);
        card_layout->addWidget(edit);
        card_layout->addWidget(reveal);
        card_layout->addWidget(hint);
        layout->addWidget(card);
        ++index;
    }
    return wrap;
}

QWidget* misconception_block(const QVector<LearningMisconception>& items, QWidget* parent)
{
    if (items.isEmpty()) {
        return empty_block(QStringLiteral("本课无此块"), parent);
    }
    auto* wrap = new QWidget(parent);
    auto* layout = new QVBoxLayout(wrap);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(new QLabel(QStringLiteral("先看错念，点击展开纠正。"), wrap));
    for (const LearningMisconception& item : items) {
        auto* inner = new QWidget(wrap);
        auto* inner_layout = new QVBoxLayout(inner);
        inner_layout->setContentsMargins(4, 4, 4, 4);
        inner_layout->addWidget(rich_label(
            QStringLiteral("<p>%1</p>").arg(item.right.toHtmlEscaped()), inner));
        auto* section = new CollapsibleSection(
            QStringLiteral("错念：%1").arg(item.wrong), inner, false, wrap);
        layout->addWidget(section);
    }
    return wrap;
}

bool matches_filter(const LearningTutorialEntry& entry, const QString& filter)
{
    if (filter.isEmpty()) {
        return true;
    }
    const auto contains = [&](const QString& text) {
        return text.contains(filter, Qt::CaseInsensitive);
    };
    if (contains(entry.command_id) || contains(entry.title) || contains(entry.category)
        || contains(entry.menu_path) || contains(entry.implemented_status)
        || contains(entry.used_for) || contains(entry.not_for) || contains(entry.scenario)
        || contains(entry.skill_mission)) {
        return true;
    }
    for (const QString& step : entry.click_steps) {
        if (contains(step)) {
            return true;
        }
    }
    for (const QString& mistake : entry.common_mistakes) {
        if (contains(mistake)) {
            return true;
        }
    }
    for (const LearningGlossaryItem& item : entry.glossary) {
        if (contains(item.term) || contains(item.plain)) {
            return true;
        }
    }
    return false;
}

}  // namespace

LearningCenterPage::LearningCenterPage(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("学习中心"));
    resize(1080, 720);

    auto* root_layout = new QVBoxLayout(this);
    auto* intro = new QLabel(QStringLiteral(
        "按命令浏览黑带级场景说明；可一键导入演示数据到工作区（新建工作表，不覆盖当前表）。"));
    intro->setWordWrap(true);
    root_layout->addWidget(intro);

    search_edit_ = new QLineEdit(this);
    search_edit_->setPlaceholderText(QStringLiteral("搜索命令、场景、菜单路径…"));
    root_layout->addWidget(search_edit_);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    tree_ = new QTreeWidget(splitter);
    tree_->setHeaderHidden(true);
    tree_->setMinimumWidth(260);
    detail_scroll_ = new QScrollArea(splitter);
    detail_scroll_->setWidgetResizable(true);
    detail_scroll_->setFrameShape(QFrame::NoFrame);
    splitter->addWidget(tree_);
    splitter->addWidget(detail_scroll_);
    splitter->setStretchFactor(1, 1);
    root_layout->addWidget(splitter, 1);

    auto* button_row = new QHBoxLayout();
    import_button_ = new QPushButton(QStringLiteral("导入测试数据到工作区"), this);
    formula_button_ = new QPushButton(QStringLiteral("打开公式说明"), this);
    auto* export_button = new QPushButton(QStringLiteral("导出 SQLite 数据库…"), this);
    button_row->addWidget(import_button_);
    button_row->addWidget(formula_button_);
    button_row->addStretch();
    button_row->addWidget(export_button);
    root_layout->addLayout(button_row);

    connect(search_edit_, &QLineEdit::textChanged, this, &LearningCenterPage::on_search_text_changed);
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, &LearningCenterPage::on_tree_selection_changed);
    connect(import_button_, &QPushButton::clicked, this, &LearningCenterPage::import_current_dataset);
    connect(formula_button_, &QPushButton::clicked, this, &LearningCenterPage::open_formula_help);
    connect(export_button, &QPushButton::clicked, this, &LearningCenterPage::export_database);

    QString error;
    const auto loaded = LearningTutorialCatalog::load_all(&error);
    entries_.clear();
    entries_.reserve(static_cast<int>(loaded.size()));
    for (const LearningTutorialEntry& entry : loaded) {
        entries_.push_back(entry);
    }
    if (!error.isEmpty()) {
        load_error_ = error;
    } else {
        const QString version = LearningDatasetStore::catalog_version(&error);
        if (!error.isEmpty()) {
            load_error_ = error;
        } else if (version != QString::fromLatin1(LearningDatasetStore::kExpectedCatalogVersion)) {
            load_error_ = QStringLiteral("学习中心数据库版本不匹配：%1").arg(version);
        }
    }
    if (load_error_.isEmpty()) {
        QString dataset_error;
        const auto datasets = LearningDatasetStore::list_datasets(&dataset_error);
        dataset_summaries_.clear();
        dataset_summaries_.reserve(static_cast<int>(datasets.size()));
        for (const LearningDatasetSummary& summary : datasets) {
            dataset_summaries_.push_back(summary);
        }
        if (!dataset_error.isEmpty() && load_error_.isEmpty()) {
            load_error_ = dataset_error;
        }
    }

    rebuild_tree(QString());
    if (!load_error_.isEmpty()) {
        show_load_error();
        import_button_->setEnabled(false);
    }
}

void LearningCenterPage::select_entry(const QString& command_id)
{
    QTreeWidgetItemIterator iterator(tree_);
    while (*iterator != nullptr) {
        QTreeWidgetItem* item = *iterator;
        if (item->data(0, Qt::UserRole).toString() == command_id) {
            tree_->setCurrentItem(item);
            tree_->scrollToItem(item);
            return;
        }
        ++iterator;
    }
}

void LearningCenterPage::on_search_text_changed(const QString& text)
{
    rebuild_tree(text.trimmed());
}

void LearningCenterPage::on_tree_selection_changed()
{
    const QList<QTreeWidgetItem*> selected = tree_->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const QString command_id = selected.front()->data(0, Qt::UserRole).toString();
    if (command_id.isEmpty()) {
        return;
    }
    for (const LearningTutorialEntry& entry : entries_) {
        if (entry.command_id == command_id) {
            show_entry(entry);
            return;
        }
    }
}

void LearningCenterPage::import_current_dataset()
{
    if (!load_error_.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("学习中心"), load_error_);
        return;
    }
    if (current_dataset_id_.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("学习中心"),
                                 QStringLiteral("本命令无需导入演示数据，请直接按步骤打开菜单。"));
        return;
    }
    emit import_demo_requested(current_dataset_id_, current_worksheet_name_);
}

void LearningCenterPage::open_formula_help()
{
    if (current_command_id_.isEmpty()) {
        return;
    }
    emit open_formula_help_requested(current_command_id_);
}

void LearningCenterPage::export_database()
{
    const QString target_path = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出学习中心数据库"), QStringLiteral("learning_center.sqlite"),
        QStringLiteral("SQLite 数据库 (*.sqlite *.db)"));
    if (target_path.isEmpty()) {
        return;
    }
    QFile resource(QString::fromLatin1(LearningDatasetStore::kResourcePath));
    if (!resource.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
                               QStringLiteral("无法读取嵌入数据库。"));
        return;
    }
    QFile target(target_path);
    if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
                               QStringLiteral("无法写入目标文件。"));
        return;
    }
    if (target.write(resource.readAll()) < 0) {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
                               QStringLiteral("写入目标文件失败。"));
        return;
    }
    QMessageBox::information(this, QStringLiteral("导出完成"),
                             QStringLiteral("已导出到：%1").arg(target_path));
}

void LearningCenterPage::rebuild_tree(const QString& filter)
{
    tree_->clear();
    QMap<QString, QTreeWidgetItem*> path_nodes;
    for (const LearningTutorialEntry& entry : entries_) {
        if (!matches_filter(entry, filter)) {
            continue;
        }

        QString path = entry.menu_path.trimmed();
        if (path.isEmpty()) {
            path = entry.category.isEmpty() ? QStringLiteral("其他") : entry.category;
        }
        const QStringList segments = path.split(
            QStringLiteral(">"), Qt::SkipEmptyParts);
        QTreeWidgetItem* parent = nullptr;
        QString accumulated;
        for (int index = 0; index < segments.size(); ++index) {
            const QString segment = segments.at(index).trimmed();
            if (segment.isEmpty()) {
                continue;
            }
            accumulated = accumulated.isEmpty() ? segment : accumulated + QStringLiteral(" > ") + segment;
            QTreeWidgetItem*& node = path_nodes[accumulated];
            if (node == nullptr) {
                node = parent == nullptr
                    ? new QTreeWidgetItem(tree_, {segment})
                    : new QTreeWidgetItem(parent, {segment});
            }
            parent = node;
        }
        if (parent == nullptr) {
            parent = new QTreeWidgetItem(tree_, {QStringLiteral("其他")});
        }
        auto* item = new QTreeWidgetItem(parent, {entry.title});
        item->setData(0, Qt::UserRole, entry.command_id);
    }
    tree_->expandToDepth(1);
}

void LearningCenterPage::show_entry(const LearningTutorialEntry& entry)
{
    current_command_id_ = entry.command_id;
    current_dataset_id_ = entry.dataset_id.value_or(QString());
    current_worksheet_name_ = current_dataset_id_.isEmpty()
        ? QString()
        : QStringLiteral("demo_%1").arg(current_dataset_id_);
    import_button_->setEnabled(load_error_.isEmpty() && !current_dataset_id_.isEmpty());
    rebuild_detail(entry);
}

void LearningCenterPage::show_load_error()
{
    auto* host = new QWidget(detail_scroll_);
    auto* layout = new QVBoxLayout(host);
    layout->addWidget(rich_label(
        QStringLiteral("<h3>加载失败</h3><p>%1</p>").arg(load_error_.toHtmlEscaped()), host));
    layout->addStretch();
    detail_scroll_->setWidget(host);
}

void LearningCenterPage::rebuild_detail(const LearningTutorialEntry& entry)
{
    auto* host = new QWidget(detail_scroll_);
    auto* layout = new QVBoxLayout(host);
    layout->setContentsMargins(8, 8, 12, 8);
    layout->setSpacing(6);

    auto* header = rich_label(
        QStringLiteral("<h2 style=\"margin:0;\">%1</h2><p><b>状态：</b>%2 &nbsp; <b>id：</b>%3</p>")
            .arg(entry.title.toHtmlEscaped(),
                 status_label(entry.implemented_status).toHtmlEscaped(),
                 entry.command_id.toHtmlEscaped()),
        host);
    layout->addWidget(header);

    layout->addWidget(new CollapsibleSection(
        QStringLiteral("0. 本课关键词"), glossary_block(entry, host), true, host));
    layout->addWidget(new CollapsibleSection(
        QStringLiteral("1. 背景"), background_block(entry, host), true, host));
    layout->addWidget(new CollapsibleSection(
        QStringLiteral("2. 专用数据"),
        dataset_block(entry, current_worksheet_name_,
                      find_dataset_summary(current_dataset_id_), host),
        true, host));
    layout->addWidget(new CollapsibleSection(
        QStringLiteral("3. 为何此工具"), why_tool_block(entry, host), true, host));
    layout->addWidget(new CollapsibleSection(
        QStringLiteral("4. 逐步 + 参数表"), steps_block(entry, host), true, host));
    layout->addWidget(new CollapsibleSection(
        QStringLiteral("5. 读输出"), output_block(entry, host), true, host));
    layout->addWidget(new CollapsibleSection(
        QStringLiteral("6. 误用"), make_list_block(entry.common_mistakes, host), true, host));

    auto* practice = new QWidget(host);
    auto* practice_layout = new QVBoxLayout(practice);
    practice_layout->setContentsMargins(0, 0, 0, 0);
    QVector<CollapsibleSection*> practice_sections;
    practice_sections.push_back(new CollapsibleSection(
        QStringLiteral("7A. 先修检查"), prereq_block(entry.prereq_quiz, practice), false, practice));
    practice_sections.push_back(new CollapsibleSection(
        QStringLiteral("7B. 步间自解释"), self_explain_block(entry.self_explain, practice),
        false, practice));
    practice_sections.push_back(new CollapsibleSection(
        QStringLiteral("7C. 褪脚手架"), fade_block(entry.fade_levels, practice), false, practice));
    practice_sections.push_back(new CollapsibleSection(
        QStringLiteral("7D. 检索小测"), retrieval_block(entry.retrieval_quiz, practice),
        false, practice));
    practice_sections.push_back(new CollapsibleSection(
        QStringLiteral("7E. 错念纠正"), misconception_block(entry.misconceptions, practice),
        false, practice));
    for (CollapsibleSection* section : practice_sections) {
        practice_layout->addWidget(section);
    }
    for (CollapsibleSection* section : practice_sections) {
        connect(section->toggle_button(), &QToolButton::toggled, this,
                [practice_sections, section](bool on) {
                    if (!on) {
                        return;
                    }
                    for (CollapsibleSection* other : practice_sections) {
                        if (other != section) {
                            other->set_expanded(false);
                        }
                    }
                });
    }
    layout->addWidget(new CollapsibleSection(
        QStringLiteral("7+. 练习闭环"), practice, false, host));
    layout->addStretch();
    detail_scroll_->setWidget(host);
}

const LearningDatasetSummary* LearningCenterPage::find_dataset_summary(
    const QString& dataset_id) const
{
    if (dataset_id.isEmpty()) {
        return nullptr;
    }
    for (const LearningDatasetSummary& summary : dataset_summaries_) {
        if (summary.dataset_id == dataset_id) {
            return &summary;
        }
    }
    return nullptr;
}

QString LearningCenterPage::status_label(const QString& status) const
{
    if (status == QStringLiteral("implemented")) {
        return QStringLiteral("已实现菜单");
    }
    if (status == QStringLiteral("formula_reference")) {
        return QStringLiteral("公式参考（菜单可能未实现）");
    }
    if (status == QStringLiteral("partial")) {
        return QStringLiteral("部分实现");
    }
    if (status == QStringLiteral("graph_reference")) {
        return QStringLiteral("图形参考");
    }
    if (status == QStringLiteral("orchestration")) {
        return QStringLiteral("编排/流程参考");
    }
    return status;
}
