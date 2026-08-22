#include "application/database/database_import_service.h"
#include "domain/database/database_provider.h"
#include "infrastructure/database/database_provider_bootstrap.h"
#include "infrastructure/database/mysql_database_provider.h"
#include "infrastructure/database/odbc_database_provider.h"
#include "infrastructure/database/postgresql_database_provider.h"
#include "infrastructure/database/sqlite_database_provider.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

#include <set>

using datalab::application::DatabaseImportService;
using datalab::domain::DatabaseProviderRegistry;
using datalab::domain::IDatabaseProvider;

class DatabaseProviderRegistryTest final : public QObject {
    Q_OBJECT

private slots:
    void registersSqliteAndUnavailableProviders();
    void rejectsDuplicateAndEmptyFactory();
    void sqliteConnectListDescribeChineseAndReserved();
    void sqliteViewsAndPrimaryKeys();
};

void DatabaseProviderRegistryTest::registersSqliteAndUnavailableProviders()
{
    DatabaseProviderRegistry registry;
    datalab::infrastructure::register_builtin_database_providers(registry);
    const auto descriptors = registry.descriptors();
    QCOMPARE(descriptors.size(), std::size_t{4});

    const auto* sqlite = registry.find("sqlite");
    QVERIFY(sqlite != nullptr);
    QVERIFY(sqlite->available);
    QCOMPARE(sqlite->display_name, std::string("SQLite"));

    const auto* postgresql = registry.find("postgresql");
    QVERIFY(postgresql != nullptr);
    QString pg_reason;
    const bool pg_driver =
        datalab::infrastructure::postgresql_driver_available(&pg_reason);
    QCOMPARE(postgresql->available, pg_driver);
    if (!pg_driver) {
        QVERIFY(!postgresql->unavailable_reason.empty());
        auto created_pg = registry.create("postgresql");
        QVERIFY(!created_pg.ok);
    } else {
        auto created_pg = registry.create("postgresql");
        QVERIFY2(created_pg.ok, created_pg.error_message.c_str());
    }

    const auto* mysql = registry.find("mysql");
    QVERIFY(mysql != nullptr);
    QString mysql_reason;
    const bool mysql_driver =
        datalab::infrastructure::mysql_driver_available(&mysql_reason);
    QCOMPARE(mysql->available, mysql_driver);

    const auto* odbc = registry.find("odbc");
    QVERIFY(odbc != nullptr);
    QString odbc_reason;
    const bool odbc_driver =
        datalab::infrastructure::odbc_driver_available(&odbc_reason);
    QCOMPARE(odbc->available, odbc_driver);
}

void DatabaseProviderRegistryTest::rejectsDuplicateAndEmptyFactory()
{
    DatabaseProviderRegistry registry;
    datalab::domain::DatabaseProviderDescriptor first;
    first.id = "demo";
    first.display_name = "Demo";
    first.available = true;
    auto ok = registry.register_factory(first, [] {
        return std::unique_ptr<IDatabaseProvider>{};
    });
    QVERIFY(ok.ok);

    auto duplicate = registry.register_factory(first, [] {
        return std::unique_ptr<IDatabaseProvider>{};
    });
    QVERIFY(!duplicate.ok);
    QCOMPARE(duplicate.error_code, std::string("duplicate_provider_id"));

    datalab::domain::DatabaseProviderDescriptor empty_factory;
    empty_factory.id = "empty";
    empty_factory.available = true;
    auto empty = registry.register_factory(empty_factory, {});
    QVERIFY(!empty.ok);
    QCOMPARE(empty.error_code, std::string("empty_factory"));
}

void DatabaseProviderRegistryTest::sqliteConnectListDescribeChineseAndReserved()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString db_path = dir.filePath(QStringLiteral("meta.sqlite"));
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), QStringLiteral("seed_meta"));
        db.setDatabaseName(db_path);
        QVERIFY(db.open());
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE \"中文表\" ("
            "\"主键\" INTEGER PRIMARY KEY,"
            "\"中文列\" TEXT,"
            "\"space col\" REAL,"
            "\"select\" TEXT,"
            "\"空串\" TEXT,"
            "\"可空\" REAL)")));
        QVERIFY(query.exec(QStringLiteral(
            "INSERT INTO \"中文表\" VALUES (1, '甲', 1.5, 'x', '', NULL)")));
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("seed_meta"));

    DatabaseProviderRegistry registry;
    datalab::infrastructure::register_builtin_database_providers(registry);
    DatabaseImportService service(registry);

    datalab::domain::ConnectionOptions options;
    options.provider_id = "sqlite";
    options.file_path = db_path.toStdString();
    options.read_only = true;
    const auto connected = service.connect(options);
    QVERIFY2(connected.ok, connected.error_message.c_str());

    const auto objects = service.list_objects({});
    QVERIFY(objects.ok);
    bool found = false;
    for (const auto& object : objects.value) {
        if (object.ref.name == "中文表") {
            found = true;
            QCOMPARE(object.ref.kind, datalab::domain::DatabaseObjectKind::table);
        }
        QVERIFY(object.ref.name.find("sqlite_") == std::string::npos);
    }
    QVERIFY(found);

    datalab::domain::ObjectRef ref;
    ref.name = "中文表";
    ref.schema = "main";
    ref.kind = datalab::domain::DatabaseObjectKind::table;
    const auto described = service.describe(ref);
    QVERIFY2(described.ok, described.error_message.c_str());
    QCOMPARE(described.value.primary_key_columns.front(), std::string("主键"));

    std::set<std::string> names;
    for (const auto& column : described.value.columns) {
        names.insert(column.name);
        QVERIFY(!column.quoted_identifier.empty());
        QVERIFY(column.quoted_identifier.front() == '"');
    }
    QVERIFY(names.count("中文列") == 1);
    QVERIFY(names.count("space col") == 1);
    QVERIFY(names.count("select") == 1);
    service.disconnect();
}

void DatabaseProviderRegistryTest::sqliteViewsAndPrimaryKeys()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString db_path = dir.filePath(QStringLiteral("view.sqlite"));
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), QStringLiteral("seed_view"));
        db.setDatabaseName(db_path);
        QVERIFY(db.open());
        QSqlQuery query(db);
        QVERIFY(query.exec(QStringLiteral(
            "CREATE TABLE sample(id INTEGER PRIMARY KEY, value REAL)")));
        QVERIFY(query.exec(QStringLiteral(
            "CREATE VIEW sample_view AS SELECT id, value FROM sample")));
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("seed_view"));

    auto provider = datalab::infrastructure::create_sqlite_database_provider();
    datalab::domain::ConnectionOptions options;
    options.provider_id = "sqlite";
    options.file_path = db_path.toStdString();
    QVERIFY(provider->connect(options).ok);

    datalab::domain::ObjectFilter filter;
    filter.include_tables = true;
    filter.include_views = true;
    const auto objects = provider->list_objects(filter);
    QVERIFY(objects.ok);
    bool saw_view = false;
    for (const auto& object : objects.value) {
        if (object.ref.name == "sample_view") {
            saw_view = true;
            QCOMPARE(object.ref.kind, datalab::domain::DatabaseObjectKind::view);
        }
    }
    QVERIFY(saw_view);

    datalab::domain::ObjectRef table_ref;
    table_ref.name = "sample";
    table_ref.kind = datalab::domain::DatabaseObjectKind::table;
    const auto meta = provider->describe(table_ref);
    QVERIFY(meta.ok);
    QCOMPARE(meta.value.primary_key_columns.front(), std::string("id"));
    provider->disconnect();
}

QTEST_MAIN(DatabaseProviderRegistryTest)

#include "database_provider_registry_test.moc"
