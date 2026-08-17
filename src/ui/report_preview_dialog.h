#pragma once

#include "domain/quality_types.h"

#include <QDialog>

class ReportPreviewDialog final : public QDialog {
    Q_OBJECT

public:
    ReportPreviewDialog(const std::vector<datalab::domain::OutputPage>& pages,
                        QWidget* parent = nullptr);
};
