#include "application/database/database_import_service.h"
#include "domain/column_extract.h"
#include "domain/database/database_provider.h"
#include "domain/database_types.h"
#include "infrastructure/database/database_provider_bootstrap.h"
#include "infrastructure/database/import_plan_serialization.h"
#include "infrastructure/database/sqlite_database_provider.h"

#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

#include <set>

using datalab::application::DatabaseImportService;
using datalab::domain::DatabaseProviderRegistry;
using datalab::domain::ImportCancellationToken;

class DatabaseImportTest final : public QObject {
    Q_OBJECT

private slots:
    void importSelectedColumnsPreservesOrderAndChineseNames();
    void distinguishesNullAndEmptyString();
    void invalidNumberGetsInvalidState();
    void syntheticRowIdWhenNoPrimaryKey();
    void primaryKeyUsedAsRowId();
    void structuredFilterAndPreviewLimitDoesNotChangePlan();
    void cancelDoesNotLeaveTable();
    void rejectNullPolicyFails();
    void importPlanValidationRejectsUnknownColumn();
    void viewImportWorks();
    void importPlanJsonRoundTrip();
    void keysetPagingSkipsOffsetAndContinues();
    void keysetRejectsCombinedOffset();
    void compositeKeysetContinuesLexicographically();
    void largeImportCancelDoesNotApplyWorksheet();
    void hundredThousandKeysetPagingStable();
};

namespace {

QString seed_path(QTemporaryDir& dir, const QString& name, const QStringList& sqls)
{
    const QString db_path = dir.filePath(name);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(
            QStringLiteral("QSQLITE"), QStringLiteral("seed_") + name);
        db.setDatabaseName(db_path);
        QVERIFY(db.open());
        QSqlQuery query(db);
        for (const QString& sql : sqls) {
            QVERIFY2(query.exec(sql), query.lastError().text().toUtf8().constData());
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("seed_") + name);
    return db_path;
}

DatabaseImportService connect_service(DatabaseProviderRegistry& registry, const QString& path)
{
    datalab::infrastructure::register_builtin_database_providers(registry);
    DatabaseImportService service(registry);
    datalab::domain::ConnectionOptions options;
    options.provider_id = "sqlite";
    options.file_path = path.toStdString();
    options.read_only = true;
    const auto connected = service.connect(options);
    QVERIFY2(connected.ok, connected.error_message.c_str());
    return service;
}

}  // namespace

void DatabaseImportTest::importSelectedColumnsPreservesOrderAndChineseNames()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("cols.sqlite"), {
        QStringLiteral("CREATE TABLE \"中文表\"("
                       "\"主键\" INTEGER PRIMARY KEY,"
                       "\"中文列\" TEXT,"
                       "\"space col\" REAL,"
                       "\"select\" TEXT,"
                       "c INTEGER)"),
        QStringLiteral("INSERT INTO \"中文表\" VALUES (1,'甲',1.5,'x',9)"),
        QStringLiteral("INSERT INTO \"中文表\" VALUES (2,'乙',2.5,'y',8)")
    });

    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    datalab::domain::ObjectRef object;
    object.name = "中文表";
    object.schema = "main";
    object.kind = datalab::domain::DatabaseObjectKind::table;

    auto plan = service.build_plan(
        object,
        {"select", "中文列", "space col"},
        {},
        "主键",
        std::nullopt,
        datalab::domain::DatabaseNullPolicy::keep_null,
        "导入工作表");
    const auto imported = service.import_table(plan);
    QVERIFY2(imported.ok, imported.error_message.c_str());
    QCOMPARE(imported.value.table.columns,
             (std::vector<std::string>{"select", "中文列", "space col"}));
    QCOMPARE(imported.value.table.rows.size(), std::size_t{2});
    QCOMPARE(imported.value.table.rows[0][0], std::string("x"));
    QCOMPARE(imported.value.table.rows[0][1], std::string("甲"));
    QCOMPARE(imported.value.table.name, std::string("导入工作表"));
    QCOMPARE(imported.value.table.import_metadata.provider_id, std::string("sqlite"));
    QVERIFY(imported.value.table.import_metadata.source_object.find("中文表") != std::string::npos);
    QVERIFY(datalab::domain::validate_data_table_contract(imported.value.table).empty());
}

void DatabaseImportTest::distinguishesNullAndEmptyString()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("nulls.sqlite"), {
        QStringLiteral("CREATE TABLE t(id INTEGER PRIMARY KEY, a TEXT, b TEXT)"),
        QStringLiteral("INSERT INTO t VALUES (1, '', NULL)")
    });
    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    datalab::domain::ObjectRef object{{}, "main", "t", datalab::domain::DatabaseObjectKind::table};
    auto plan = service.build_plan(object, {"a", "b"}, {}, "id", std::nullopt,
                                   datalab::domain::DatabaseNullPolicy::keep_null, "t");
    const auto imported = service.import_table(plan);
    QVERIFY(imported.ok);
    QCOMPARE(imported.value.table.rows[0][0], std::string());
    QCOMPARE(imported.value.table.rows[0][1], std::string());
    QCOMPARE(imported.value.table.cell_states[0][0], datalab::domain::CellState::valid);
    QCOMPARE(imported.value.table.cell_states[0][1], datalab::domain::CellState::missing);
}

void DatabaseImportTest::invalidNumberGetsInvalidState()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("badnum.sqlite"), {
        QStringLiteral("CREATE TABLE t(id INTEGER PRIMARY KEY, v TEXT)"),
        QStringLiteral("INSERT INTO t VALUES (1, 'not-a-number')")
    });
    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    // Force numeric mapping via describe then override after build — use REAL declared
    // column with text affinity insert instead:
    service.disconnect();
    const QString path2 = seed_path(dir, QStringLiteral("badnum2.sqlite"), {
        QStringLiteral("CREATE TABLE t(id INTEGER PRIMARY KEY, v REAL)"),
        QStringLiteral("INSERT INTO t VALUES (1, 'abc')")
    });
    service = connect_service(registry, path2);
    datalab::domain::ObjectRef object{{}, "main", "t", datalab::domain::DatabaseObjectKind::table};
    auto plan = service.build_plan(object, {"v"}, {}, "id", std::nullopt,
                                   datalab::domain::DatabaseNullPolicy::keep_null, "t");
    const auto imported = service.import_table(plan);
    QVERIFY(imported.ok);
    QCOMPARE(imported.value.table.column_types[0], datalab::domain::ColumnType::numeric);
    QCOMPARE(imported.value.table.cell_states[0][0], datalab::domain::CellState::invalid);
}

void DatabaseImportTest::syntheticRowIdWhenNoPrimaryKey()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("nopk.sqlite"), {
        QStringLiteral("CREATE TABLE t(a TEXT, b TEXT)"),
        QStringLiteral("INSERT INTO t VALUES ('1','x')"),
        QStringLiteral("INSERT INTO t VALUES ('2','y')")
    });
    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    datalab::domain::ObjectRef object{{}, "main", "t", datalab::domain::DatabaseObjectKind::table};
    auto plan = service.build_plan(object, {"a", "b"}, {}, "", std::nullopt,
                                   datalab::domain::DatabaseNullPolicy::keep_null, "t");
    const auto imported = service.import_table(plan);
    QVERIFY(imported.ok);
    QVERIFY(imported.value.row_id_is_synthetic);
    QVERIFY(imported.value.table.import_metadata.row_id_is_synthetic);
    QCOMPARE(imported.value.table.row_ids, (std::vector<datalab::domain::RowId>{0, 1}));
}

void DatabaseImportTest::primaryKeyUsedAsRowId()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("pk.sqlite"), {
        QStringLiteral("CREATE TABLE t(id INTEGER PRIMARY KEY, v TEXT)"),
        QStringLiteral("INSERT INTO t VALUES (10,'a')"),
        QStringLiteral("INSERT INTO t VALUES (20,'b')")
    });
    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    datalab::domain::ObjectRef object{{}, "main", "t", datalab::domain::DatabaseObjectKind::table};
    auto plan = service.build_plan(object, {"id", "v"}, {}, "id", std::nullopt,
                                   datalab::domain::DatabaseNullPolicy::keep_null, "t");
    const auto imported = service.import_table(plan);
    QVERIFY(imported.ok);
    QVERIFY(!imported.value.row_id_is_synthetic);
    QCOMPARE(imported.value.table.row_ids,
             (std::vector<datalab::domain::RowId>{10, 20}));
}

void DatabaseImportTest::structuredFilterAndPreviewLimitDoesNotChangePlan()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("filter.sqlite"), {
        QStringLiteral("CREATE TABLE t(id INTEGER PRIMARY KEY, g TEXT)"),
        QStringLiteral("INSERT INTO t VALUES (1,'A')"),
        QStringLiteral("INSERT INTO t VALUES (2,'B')"),
        QStringLiteral("INSERT INTO t VALUES (3,'A')")
    });
    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    datalab::domain::ObjectRef object{{}, "main", "t", datalab::domain::DatabaseObjectKind::table};
    datalab::domain::StructuredFilter filter{"g", "=", "A"};
    auto plan = service.build_plan(object, {"id", "g"}, {filter}, "id", std::nullopt,
                                   datalab::domain::DatabaseNullPolicy::keep_null, "t");
    const auto original_limit = plan.row_limit;
    const auto preview = service.preview(plan, 1);
    QVERIFY(preview.ok);
    QCOMPARE(preview.value.sample.rows.size(), std::size_t{1});
    QVERIFY(preview.value.truncated);
    QCOMPARE(plan.row_limit, original_limit);

    const auto imported = service.import_table(plan);
    QVERIFY(imported.ok);
    QCOMPARE(imported.value.table.rows.size(), std::size_t{2});
    QCOMPARE(imported.value.table.import_metadata.filter_summary.find("g = ?"),
             0);
}

void DatabaseImportTest::cancelDoesNotLeaveTable()
{
    // Cancellation before rows: token already cancelled.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("cancel.sqlite"), {
        QStringLiteral("CREATE TABLE t(id INTEGER PRIMARY KEY, v TEXT)"),
        QStringLiteral("INSERT INTO t VALUES (1,'a')")
    });
    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    datalab::domain::ObjectRef object{{}, "main", "t", datalab::domain::DatabaseObjectKind::table};
    auto plan = service.build_plan(object, {"id", "v"}, {}, "id", std::nullopt,
                                   datalab::domain::DatabaseNullPolicy::keep_null, "t");
    ImportCancellationToken token;
    token.request_cancel();
    // Token checked per row; with 1 row may still complete before check on first iteration
    // after next(). Force cancel mid-flight by cancelling before import of larger set.
    service.disconnect();
    const QString path2 = seed_path(dir, QStringLiteral("cancel2.sqlite"), {
        QStringLiteral("CREATE TABLE t(id INTEGER PRIMARY KEY, v TEXT)")
    });
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("fill"));
        db.setDatabaseName(path2);
        QVERIFY(db.open());
        QSqlQuery q(db);
        QVERIFY(db.transaction());
        for (int i = 0; i < 200; ++i) {
            QVERIFY(q.exec(QStringLiteral("INSERT INTO t VALUES (%1,'x')").arg(i)));
        }
        QVERIFY(db.commit());
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("fill"));
    service = connect_service(registry, path2);
    plan = service.build_plan(object, {"id", "v"}, {}, "id", std::nullopt,
                              datalab::domain::DatabaseNullPolicy::keep_null, "t");
    ImportCancellationToken token2;
    token2.request_cancel();
    const auto imported = service.import_table(plan, &token2);
    // First row check happens after next(), so cancelled token should fail quickly.
    QVERIFY(!imported.ok);
    QCOMPARE(imported.error_code, std::string("cancelled"));
}

void DatabaseImportTest::rejectNullPolicyFails()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("rej.sqlite"), {
        QStringLiteral("CREATE TABLE t(id INTEGER PRIMARY KEY, v TEXT)"),
        QStringLiteral("INSERT INTO t VALUES (1, NULL)")
    });
    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    datalab::domain::ObjectRef object{{}, "main", "t", datalab::domain::DatabaseObjectKind::table};
    auto plan = service.build_plan(object, {"v"}, {}, "id", std::nullopt,
                                   datalab::domain::DatabaseNullPolicy::reject_null, "t");
    const auto imported = service.import_table(plan);
    QVERIFY(!imported.ok);
    QCOMPARE(imported.error_code, std::string("null_rejected"));
}

void DatabaseImportTest::importPlanValidationRejectsUnknownColumn()
{
    datalab::domain::TableMetadata metadata;
    datalab::domain::ColumnMetadata column;
    column.name = "a";
    column.quoted_identifier = "\"a\"";
    metadata.columns.push_back(column);
    datalab::domain::ImportPlan plan;
    plan.provider_id = "sqlite";
    plan.object_ref.name = "t";
    plan.selected_columns = {"missing"};
    const std::string error = datalab::domain::validate_import_plan(plan, metadata);
    QVERIFY(!error.empty());
}

void DatabaseImportTest::viewImportWorks()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("viewimp.sqlite"), {
        QStringLiteral("CREATE TABLE base(id INTEGER PRIMARY KEY, v TEXT)"),
        QStringLiteral("INSERT INTO base VALUES (1,'z')"),
        QStringLiteral("CREATE VIEW v AS SELECT id, v FROM base")
    });
    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    datalab::domain::ObjectRef object{{}, "main", "v", datalab::domain::DatabaseObjectKind::view};
    auto plan = service.build_plan(object, {"id", "v"}, {}, "id", std::nullopt,
                                   datalab::domain::DatabaseNullPolicy::keep_null, "from_view");
    const auto imported = service.import_table(plan);
    QVERIFY2(imported.ok, imported.error_message.c_str());
    QCOMPARE(imported.value.table.import_metadata.object_kind, std::string("view"));
    QCOMPARE(imported.value.table.rows.size(), std::size_t{1});
}

void DatabaseImportTest::importPlanJsonRoundTrip()
{
    datalab::domain::ImportPlan plan;
    plan.source_connection_id = "c1";
    plan.provider_id = "sqlite";
    plan.object_ref = {{}, "main", "t", datalab::domain::DatabaseObjectKind::table};
    plan.selected_columns = {"a", "b"};
    plan.column_order = {"b", "a"};
    plan.aliases = {"B", "A"};
    plan.structured_filter.push_back({"a", "=", "1"});
    plan.order_key = "a";
    plan.row_limit = 10;
    plan.keyset_after = datalab::domain::KeysetCursor::single("a", "5");
    plan.null_policy = datalab::domain::DatabaseNullPolicy::keep_null;
    plan.target_worksheet_name = "ws";
    plan.source_snapshot_time = "2026-08-21T00:00:00Z";

    const QJsonObject json = datalab::infrastructure::import_plan_to_json(plan);
    const datalab::domain::ImportPlan restored =
        datalab::infrastructure::import_plan_from_json(json);
    QCOMPARE(restored.provider_id, plan.provider_id);
    QCOMPARE(restored.object_ref.name, plan.object_ref.name);
    QCOMPARE(restored.selected_columns, plan.selected_columns);
    QCOMPARE(restored.column_order, plan.column_order);
    QCOMPARE(restored.aliases, plan.aliases);
    QCOMPARE(restored.structured_filter.size(), std::size_t{1});
    QCOMPARE(restored.structured_filter[0].op, std::string("="));
    QCOMPARE(*restored.row_limit, std::uint64_t{10});
    QVERIFY(restored.keyset_after.has_value());
    QCOMPARE(restored.keyset_after->columns.size(), std::size_t{1});
    QCOMPARE(restored.keyset_after->columns.front(), std::string("a"));
    QCOMPARE(restored.keyset_after->after_values.front(), std::string("5"));
    QCOMPARE(restored.target_worksheet_name, plan.target_worksheet_name);
    QVERIFY(!json.contains(QStringLiteral("password")));
    QVERIFY(!json.contains(QStringLiteral("dsn")));
}

void DatabaseImportTest::keysetPagingSkipsOffsetAndContinues()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("keyset.sqlite"), {
        QStringLiteral("CREATE TABLE t(id INTEGER PRIMARY KEY, v TEXT)"),
        QStringLiteral("INSERT INTO t VALUES (1,'a')"),
        QStringLiteral("INSERT INTO t VALUES (2,'b')"),
        QStringLiteral("INSERT INTO t VALUES (3,'c')"),
        QStringLiteral("INSERT INTO t VALUES (4,'d')")
    });
    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    datalab::domain::ObjectRef object{{}, "main", "t", datalab::domain::DatabaseObjectKind::table};
    auto plan = service.build_plan(object, {"id", "v"}, {}, "id", 2,
                                   datalab::domain::DatabaseNullPolicy::keep_null, "t");
    const auto first = service.import_table(plan);
    QVERIFY(first.ok);
    QCOMPARE(first.value.table.rows.size(), std::size_t{2});
    QCOMPARE(first.value.table.rows[1][0], std::string("2"));

    plan.keyset_after = datalab::domain::KeysetCursor::single("id", "2");
    plan.row_offset = std::nullopt;
    const auto second = service.import_table(plan);
    QVERIFY2(second.ok, second.error_message.c_str());
    QCOMPARE(second.value.table.rows.size(), std::size_t{2});
    QCOMPARE(second.value.table.rows[0][0], std::string("3"));
    QCOMPARE(second.value.table.rows[1][0], std::string("4"));
    bool saw_keyset = false;
    for (const auto& diagnostic : second.value.diagnostics) {
        if (diagnostic.code == "keyset_paging") {
            saw_keyset = true;
        }
    }
    QVERIFY(saw_keyset);
}

void DatabaseImportTest::keysetRejectsCombinedOffset()
{
    datalab::domain::TableMetadata metadata;
    datalab::domain::ColumnMetadata column;
    column.name = "id";
    column.quoted_identifier = "\"id\"";
    metadata.columns.push_back(column);
    metadata.primary_key_columns = {"id"};
    datalab::domain::ImportPlan plan;
    plan.provider_id = "sqlite";
    plan.object_ref.name = "t";
    plan.selected_columns = {"id"};
    plan.order_key = "id";
    plan.row_limit = 10;
    plan.row_offset = 5;
    plan.keyset_after = datalab::domain::KeysetCursor::single("id", "1");
    const std::string error = datalab::domain::validate_import_plan(plan, metadata);
    QVERIFY(!error.empty());
}

void DatabaseImportTest::compositeKeysetContinuesLexicographically()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("ckey.sqlite"), {
        QStringLiteral("CREATE TABLE t(a INTEGER NOT NULL, b INTEGER NOT NULL, v TEXT, "
                       "PRIMARY KEY(a,b))"),
        QStringLiteral("INSERT INTO t VALUES (1,1,'a')"),
        QStringLiteral("INSERT INTO t VALUES (1,2,'b')"),
        QStringLiteral("INSERT INTO t VALUES (2,1,'c')"),
        QStringLiteral("INSERT INTO t VALUES (2,2,'d')")
    });
    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    datalab::domain::ObjectRef object{{}, "main", "t", datalab::domain::DatabaseObjectKind::table};
    auto plan = service.build_plan(object, {"a", "b", "v"}, {}, "a", 2,
                                   datalab::domain::DatabaseNullPolicy::keep_null, "t");
    const auto first = service.import_table(plan);
    QVERIFY(first.ok);
    QCOMPARE(first.value.table.rows.size(), std::size_t{2});

    datalab::domain::KeysetCursor cursor;
    cursor.columns = {"a", "b"};
    cursor.after_values = {"1", "2"};
    plan.keyset_after = cursor;
    plan.row_offset = std::nullopt;
    const auto second = service.import_table(plan);
    QVERIFY2(second.ok, second.error_message.c_str());
    QCOMPARE(second.value.table.rows.size(), std::size_t{2});
    QCOMPARE(second.value.table.rows[0][2], std::string("c"));
    QCOMPARE(second.value.table.rows[1][2], std::string("d"));
}

void DatabaseImportTest::largeImportCancelDoesNotApplyWorksheet()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("big.sqlite"), {
        QStringLiteral("CREATE TABLE t(id INTEGER PRIMARY KEY, v TEXT)")
    });
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("fillbig"));
        db.setDatabaseName(path);
        QVERIFY(db.open());
        QSqlQuery q(db);
        QVERIFY(db.transaction());
        for (int i = 0; i < 5000; ++i) {
            QVERIFY(q.exec(QStringLiteral("INSERT INTO t VALUES (%1,'x')").arg(i)));
        }
        QVERIFY(db.commit());
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("fillbig"));

    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    datalab::domain::ObjectRef object{{}, "main", "t", datalab::domain::DatabaseObjectKind::table};
    auto plan = service.build_plan(object, {"id", "v"}, {}, "id", std::nullopt,
                                   datalab::domain::DatabaseNullPolicy::keep_null, "t");
    ImportCancellationToken token;
    token.request_cancel();
    const auto imported = service.import_table(plan, &token);
    QVERIFY(!imported.ok);
    QCOMPARE(imported.error_code, std::string("cancelled"));
}

void DatabaseImportTest::hundredThousandKeysetPagingStable()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = seed_path(dir, QStringLiteral("100k.sqlite"), {
        QStringLiteral("CREATE TABLE t(id INTEGER PRIMARY KEY, v TEXT)")
    });
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("fill100k"));
        db.setDatabaseName(path);
        QVERIFY(db.open());
        QSqlQuery q(db);
        QVERIFY(db.transaction());
        for (int i = 1; i <= 100000; ++i) {
            QVERIFY(q.exec(QStringLiteral("INSERT INTO t VALUES (%1,'x')").arg(i)));
        }
        QVERIFY(db.commit());
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("fill100k"));

    DatabaseProviderRegistry registry;
    DatabaseImportService service = connect_service(registry, path);
    datalab::domain::ObjectRef object{{}, "main", "t", datalab::domain::DatabaseObjectKind::table};
    auto plan = service.build_plan(object, {"id", "v"}, {}, "id", 1000,
                                   datalab::domain::DatabaseNullPolicy::keep_null, "t");
    std::uint64_t loaded = 0;
    std::string cursor_value;
    for (int page = 0; page < 5; ++page) {
        if (!cursor_value.empty()) {
            plan.keyset_after = datalab::domain::KeysetCursor::single("id", cursor_value);
            plan.row_offset = std::nullopt;
        }
        const auto chunk = service.import_table(plan);
        QVERIFY2(chunk.ok, chunk.error_message.c_str());
        QCOMPARE(chunk.value.table.rows.size(), std::size_t{1000});
        loaded += chunk.value.table.rows.size();
        cursor_value = chunk.value.table.rows.back()[0];
    }
    QCOMPARE(loaded, std::uint64_t{5000});
    // Full materialize of 100k is intentionally not required; keyset pages stay bounded.
    QVERIFY(loaded < 100000);
}

QTEST_MAIN(DatabaseImportTest)

#include "database_import_test.moc"
