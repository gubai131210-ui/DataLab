#pragma once

#include <QCoreApplication>
#include <QString>

// UI chrome / menu strings use Qt Linguist context "DataLabUi".
// Source language is zh_CN (product default); en_US comes from DataLab_en_US.qm.
// Report body localization remains ReportProfile/catalog (ADR 0009) — do not mix.
// File name intentionally avoids ui_*.h so Qt AUTOUIC does not expect a matching .ui.
namespace datalab::ui {

inline QString ui_tr(const char* source_zh)
{
    return QCoreApplication::translate("DataLabUi", source_zh);
}

inline QString ui_tr(const QString& source_zh)
{
    return QCoreApplication::translate("DataLabUi", source_zh.toUtf8().constData());
}

}  // namespace datalab::ui
