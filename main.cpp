#include "ui/mainwindow.h"

#include "infrastructure/database/database_provider_bootstrap.h"

#include <QApplication>
#include <QString>
#include <QTranslator>

#include <cstdlib>

namespace {

QString resolve_ui_language_tag()
{
    if (const char* env = std::getenv("DATALAB_UI_LANG")) {
        const QString tag = QString::fromUtf8(env).trimmed();
        if (!tag.isEmpty()) {
            return tag;
        }
    }
    // Default product UI remains Simplified Chinese (ADR 0006); English opt-in via env.
    return QStringLiteral("zh-CN");
}

bool language_is_english(const QString& tag)
{
    const QString normalized = tag.toLower();
    return normalized == QStringLiteral("en")
        || normalized == QStringLiteral("en-us")
        || normalized == QStringLiteral("en_us")
        || normalized.startsWith(QStringLiteral("en-"))
        || normalized.startsWith(QStringLiteral("en_"));
}

QString translator_resource_path(const QString& language_tag)
{
    if (language_is_english(language_tag)) {
        return QStringLiteral(":/i18n/DataLab_en_US.qm");
    }
    return QStringLiteral(":/i18n/DataLab_zh_CN.qm");
}

}  // namespace

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(app_resources);
    Q_INIT_RESOURCE(translations);
    QApplication a(argc, argv);
    datalab::infrastructure::ensure_default_database_providers_registered();

    const QString ui_language = resolve_ui_language_tag();
    QTranslator translator;
    const QString qm_path = translator_resource_path(ui_language);
    if (translator.load(qm_path)) {
        a.installTranslator(&translator);
    }
    // Report locale stays on ReportProfile (ADR 0009) and is independent of this UI translator.

    MainWindow w;
    w.show();
    return QApplication::exec();
}
