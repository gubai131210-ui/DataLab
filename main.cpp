#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(app_resources);
    QApplication a(argc, argv);
    QTranslator translator;
    if (translator.load(QStringLiteral(":/i18n/DataLab_zh_CN.qm"))) {
        a.installTranslator(&translator);
    }
    MainWindow w;
    w.show();
    return QApplication::exec();
}
