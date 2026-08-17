#include "ui/analysis_dialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>

AnalysisDialog::AnalysisDialog(const QStringList& columns, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("I-MR 控制图"));
    setModal(true);
    auto* layout = new QFormLayout(this);
    measurement_column_ = new QComboBox(this);
    measurement_column_->addItems(columns);
    lower_specification_ = new QLineEdit(this);
    upper_specification_ = new QLineEdit(this);
    lower_specification_->setPlaceholderText(QStringLiteral("可选，例如 73.95"));
    upper_specification_->setPlaceholderText(QStringLiteral("可选，例如 74.05"));
    layout->addRow(QStringLiteral("测量值列："), measurement_column_);
    layout->addRow(QStringLiteral("下规格限 LSL："), lower_specification_);
    layout->addRow(QStringLiteral("上规格限 USL："), upper_specification_);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

AnalysisDialog::Configuration AnalysisDialog::configuration() const
{
    bool lower_ok = false;
    bool upper_ok = false;
    const double lower = lower_specification_->text().toDouble(&lower_ok);
    const double upper = upper_specification_->text().toDouble(&upper_ok);
    return {
        measurement_column_->currentIndex(),
        lower_ok ? std::optional<double>(lower) : std::nullopt,
        upper_ok ? std::optional<double>(upper) : std::nullopt};
}
