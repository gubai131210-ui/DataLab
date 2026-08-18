#include "infrastructure/project_repository.h"

#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryFile>
#include <QtTest/QtTest>

class ProjectRepositoryTest final : public QObject {
    Q_OBJECT

private slots:
    void initializesSchemaVersion();
};

void ProjectRepositoryTest::initializesSchemaVersion()
{
    QTemporaryFile project(QDir::tempPath() + QStringLiteral("/datalab-project-XXXXXX.dlab"));
    project.setAutoRemove(false);
    QVERIFY(project.open());
    const QString path = project.fileName();
    project.close();

    datalab::infrastructure::ProjectRepository repository;
    datalab::domain::DataTable table;
    QString error;
    QVERIFY2(repository.save(path, table, {}, &error), qPrintable(error));

    const QString connection_name = QStringLiteral("schema_check");
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), connection_name);
        database.setDatabaseName(path);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral("PRAGMA user_version")));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 1);
        database.close();
    }
    QSqlDatabase::removeDatabase(connection_name);
    QFile::remove(path);
}

QTEST_MAIN(ProjectRepositoryTest)
#include "project_repository_test.moc"
