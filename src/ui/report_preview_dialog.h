#pragma once

#include "domain/quality_types.h"
#include "domain/report_types.h"

#include <QDialog>

class ReportPreviewDialog final : public QDialog {
    Q_OBJECT

public:
    ReportPreviewDialog(
        const datalab::domain::ReportDocument& document,
        QWidget* parent = nullptr);

    // Legacy convenience: engineer-like preview of raw pages.
    ReportPreviewDialog(
        const std::vector<datalab::domain::OutputPage>& pages,
        QWidget* parent = nullptr);
};
