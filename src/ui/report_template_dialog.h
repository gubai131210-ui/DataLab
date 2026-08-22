#pragma once

#include "domain/report_types.h"

#include <QDialog>

class ReportTemplateDialog final : public QDialog {
    Q_OBJECT

public:
    explicit ReportTemplateDialog(QWidget* parent = nullptr);

    datalab::domain::ReportProfile selected_profile() const;

private:
    class QButtonGroup* template_group_ = nullptr;
    class QButtonGroup* language_group_ = nullptr;
};
