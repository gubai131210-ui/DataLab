#include "ui/report_template_dialog.h"

#include "domain/report_text_catalog.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace {

QString ui_text(const char* id)
{
    // Dialog chrome follows UI default zh-CN; report language is chosen separately.
    return QString::fromStdString(
        datalab::domain::resolve_report_text(id, "zh-CN").text);
}

}  // namespace

ReportTemplateDialog::ReportTemplateDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(ui_text("ui.template_dialog.title"));
    resize(560, 520);
    auto* outer = new QVBoxLayout(this);
    auto* intro = new QLabel(ui_text("ui.template_dialog.intro"), this);
    intro->setWordWrap(true);
    outer->addWidget(intro);

    auto* group_box = new QGroupBox(ui_text("ui.template_dialog.group"), this);
    auto* form = new QVBoxLayout(group_box);
    template_group_ = new QButtonGroup(this);
    auto* customer = new QRadioButton(ui_text("ui.template_dialog.customer"), group_box);
    auto* engineer = new QRadioButton(ui_text("ui.template_dialog.engineer"), group_box);
    auto* audit = new QRadioButton(ui_text("ui.template_dialog.audit"), group_box);
    customer->setObjectName(QStringLiteral("report_template_customer"));
    engineer->setObjectName(QStringLiteral("report_template_engineer"));
    audit->setObjectName(QStringLiteral("report_template_audit"));
    template_group_->addButton(customer, 0);
    template_group_->addButton(engineer, 1);
    template_group_->addButton(audit, 2);
    engineer->setChecked(true);
    form->addWidget(customer);
    form->addWidget(engineer);
    form->addWidget(audit);
    outer->addWidget(group_box);

    auto* language_box = new QGroupBox(ui_text("ui.template_dialog.language_group"), this);
    auto* language_form = new QVBoxLayout(language_box);
    language_group_ = new QButtonGroup(this);
    auto* zh = new QRadioButton(ui_text("ui.template_dialog.lang_zh"), language_box);
    auto* en = new QRadioButton(ui_text("ui.template_dialog.lang_en"), language_box);
    zh->setObjectName(QStringLiteral("report_locale_zh_CN"));
    en->setObjectName(QStringLiteral("report_locale_en_US"));
    language_group_->addButton(zh, 0);
    language_group_->addButton(en, 1);
    zh->setChecked(true);
    language_form->addWidget(zh);
    language_form->addWidget(en);
    outer->addWidget(language_box);

    auto* note = new QLabel(ui_text("ui.template_dialog.note"), this);
    note->setWordWrap(true);
    outer->addWidget(note);
    outer->addStretch(1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(ui_text("ui.template_dialog.continue"));
    buttons->button(QDialogButtonBox::Cancel)->setText(ui_text("ui.template_dialog.cancel"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    outer->addWidget(buttons);
}

datalab::domain::ReportProfile ReportTemplateDialog::selected_profile() const
{
    datalab::domain::ReportProfile profile;
    switch (template_group_->checkedId()) {
    case 0:
        profile = datalab::domain::make_report_profile(
            datalab::domain::ReportTemplateKind::customer);
        break;
    case 2:
        profile = datalab::domain::make_report_profile(
            datalab::domain::ReportTemplateKind::audit);
        break;
    case 1:
    default:
        profile = datalab::domain::make_report_profile(
            datalab::domain::ReportTemplateKind::engineer);
        break;
    }
    if (language_group_->checkedId() == 1) {
        profile.locale.language_tag = "en-US";
        profile.locale.number_format_locale = "en-US";
        profile.locale.date_format_locale = "en-US";
        profile.locale.timezone_id = "UTC";
    } else {
        profile.locale.language_tag = "zh-CN";
        profile.locale.number_format_locale = "zh-CN";
        profile.locale.date_format_locale = "zh-CN";
        profile.locale.timezone_id = "Asia/Shanghai";
    }
    return profile;
}

#include "report_template_dialog.moc"
