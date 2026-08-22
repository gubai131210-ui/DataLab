# DataLab 报告可读性、表格交互与数据库导入架构研究

研究日期：2026-08-21（UTC+8）  
研究范围：只读检查当前 `src/`、`tests/`、`docs/`、`CMakeLists.txt`；不修改 C++/CMake 代码，不运行 cmake/ctest。  
目标：为报告格式/内容优化、表格交互增强、数据库表与列发现、按选择导入工作表以及分层多页面 UI 提供可执行的架构基线。

## 1. 执行摘要

当前项目已经具备可复用的四层意图（domain / application / infrastructure / ui）、结构化 `OutputPage`、`StatisticTable`、`DiagnosticMessage`、`RuleEvidence` 和 `DataTable`，但三个用户可感知问题仍由同一批隐式契约放大：

1. 报告的表格和解释仍有 `Test 1`、`Test 1–4` 等内部编号，用户必须记忆编号才能理解证据。
2. 工作表是完整复制到 `WorksheetModel` 的内存表；表头、单元格编辑、选择和分页没有形成数据库查询模型的统一交互契约。
3. 导入服务按扩展名静态分派，并且 `.xlsx` 只读取第一个工作表；SQLite 项目存储也硬编码在 `ProjectRepository`，尚无数据库 provider 的发现、注册和元数据接口。

建议采用“端口 + provider 适配器 + 元数据快照 + 显式导入计划”的方向：

- `DatabaseProviderRegistry` 只管理能力声明和工厂，不让 UI 直接构造 `QSqlDatabase`。
- 每个 provider 负责连接、列出表/视图、读取列元数据、生成安全的限定标识符和创建查询模型。
- UI 先展示“连接 → 对象 → 列选择 → 预览/导入”的独立页面，再生成不可变 `ImportPlan`；导入结果保留源、表、列、类型、行数、时间和诊断。
- 小数据仍可 materialize 成领域 `DataTable`；大表使用分页/窗口查询或增量 `QAbstractItemModel`，不要把全部数据库行强行复制到 QWidget。
- 报告由“摘要 → 规则证据 → 详细表 → 图 → 诊断/来源”的页面层级构成；每个规则同时有稳定 ID、具体名称、阈值、证据行和建议动作。

## 2. 当前现状证据

### 2.1 文档与分层约束

- `CMakeLists.txt:8-10` 明确写出依赖方向 `ui → {application, infrastructure, reporting} → domain`。
- `docs/adr/0001-core-architecture.md` 已决定：核心算法使用独立 C++ 模块，UI 通过 application 调用领域模块，原始数据与分析数据分离，项目文件采用 SQLite 容器并预留 MySQL/SQL Server 适配器。
- `docs/adr/0006-native-xlsx-import.md` 已明确运行时不依赖 Python，`.csv/.txt` 和 `.xlsx` 分别由 `CsvImporter` / `ExcelTableImporter` 处理，但当前仍只读取第一个工作表。
- `docs/ui-guidelines.md` 已规定导航 dock、白色内容面板、状态色、表格行高/表头高度、图表命中提示以及“属性面板固定在图表右侧、不覆盖图表”等交互安全线。新页面应复用这些约束，不把更多控件挤进现有主窗口。

### 2.2 导入和存储的实际形态

- `src/infrastructure/data_import_service.cpp` 以扩展名判断 `.xlsx` 或 CSV，并直接调用两个静态 importer；未知扩展名会落入 CSV 分支，不能表达 provider 能力或连接配置。
- `src/infrastructure/excel_table_importer.h` 的注释和接口均是“读取第一个 worksheet 到 `DataTable`”，没有工作表目录、列选择或预览接口。
- `src/domain/quality_types.h` 的 `DataTable` 已有 `ImportMetadata`、`row_ids`、`column_types`、`cell_states` 和 warning 字段，这是数据库导入审计和类型归一化的可复用基础。
- 同一文件的 `StatisticTable` 只有 `title`、`headers`、字符串二维 `rows`；没有列类型、排序键、原始行 ID、规则 ID、可访问描述或分页游标。因此报告展示层只能把信息压成字符串。
- `ProjectRepository` 当前使用 `QSQLITE`、`QSqlDatabase::addDatabase`、事务和 `PRAGMA user_version`。这证明 Qt SQL 已经在 infrastructure 中使用，但连接、迁移和原始数据存储尚未抽成 provider/store 端口。项目文件的 `raw_rows.values_json` 实际写入转义后的管道字符串，名称与内容格式不一致，后续报告 schema 不应继续复制这种隐式编码。

### 2.3 报告与 Test 编号证据

- `src/domain/statistics/control_charts.cpp` 已实现 8 类特殊原因规则，并在诊断消息中提供中文解释；规则结果还保留 `triggered_tests` 和 `primary_test_by_point`。
- `src/application/output_builder.cpp` 的 `triggered_tests_text()` 将触发结果格式化为 `"Test 1,Test 2"`；多个逐点表的表头也直接使用 `Test 1` 至 `Test 8`，最小测试列仍输出编号。
- `src/application/chart_pages.cpp` 和 `src/application/analysis_service.cpp` 仍输出 `"Test 1 超限点数"`、`"MR 图适用规则 Test 1–4"` 等编号文案。
- 这是“算法已经有规则证据、输出层丢失规则语义”的边界，而不是要求重写统计算法。首选改造点是规则注册表/标签服务与 `StatisticTable`/报告 renderer 的结构化字段。

## 3. 需求分类与非目标

### 3.1 功能需求

| 类别 | 必须支持 | 验收含义 |
|---|---|---|
| 报告可读性 | Test 1–8 具体规则名、解释、窗口、阈值、状态、建议 | 用户不看帮助页也能解释每一行 |
| 表格交互 | 排序、筛选、列显隐、复制、原始行联动、固定摘要列、分页/增量加载 | 操作不会破坏原始行映射 |
| 数据库发现 | SQLite/PostgreSQL/MySQL/ODBC 的连接和表/视图/列元数据 | provider 可单测，UI 不依赖具体驱动 |
| 选择导入 | 选表/视图、选列、预览、限制行数、导入到新工作表 | 只导入用户确认的内容 |
| 分层 UI | 连接页、对象页、列选择页、预览页、结果页分开 | 不把连接参数、元数据表和导入按钮堆在一页 |
| 审计与安全 | 参数绑定、标识符安全、最小权限、事务、取消、诊断和来源 | 失败可解释，成功可复现 |

### 3.2 明确非目标

- 本研究不选择或捆绑 PostgreSQL/MySQL 客户端驱动许可证，也不承诺所有 ODBC 驱动的同等能力。
- 不把数据库表直接变成可编辑数据源；第一阶段导入应是只读源到 DataLab 工作表的快照。
- 不把“可连接”误写成“已支持所有数据库特性”；provider 必须声明能力和限制。
- 不用分页掩盖统计算法需要全量数据的事实；分析前应明确 materialize、聚合查询或抽样策略。

## 4. Test 1–8 规则语义基线

产品文案可本地化，但稳定 ID 不变。以下名称与当前实现窗口一致；`>`、`<`、等于控制限、缺失断点和分阶段边界必须在规则元数据中冻结。

| 稳定 ID | 用户可读规则名 | 具体判定与解释 | 建议输出 |
|---|---|---|---|
| `beyond_control_limit` | 单点超出 3σ 控制限 | 任一点低于 LCL 或高于 UCL；当前代码对有限控制限使用严格越界比较。表示该点与当前控制模型不一致，不等于根因已确认。 | 关联观测行、图点、CL/LCL/UCL、方向、建议复核测量/批次/设备 |
| `nine_same_side` | 连续 9 点位于中心线同侧 | 连续 9 个可用点全部在中心线同一侧；跨缺失或阶段断点不应形成窗口。提示均值偏移、分层或阶段变化。 | 窗口首尾、上/下侧、相关行 |
| `six_point_trend` | 连续 6 点持续单调趋势 | 连续 6 点严格递增或严格递减；相等值不构成趋势。提示磨损、漂移或时间相关结构。 | 窗口首尾、上升/下降、趋势点 |
| `fourteen_alternating` | 连续 14 点上下交替 | 连续 14 点的相邻差值符号严格交替；零差值打断规则。提示周期、人机/设备交替或过度调整。 | 窗口首尾、交替方向、相关行 |
| `two_of_three_beyond_2sigma` | 3 点中至少 2 点同侧超过 2σ | 3 点窗口内至少 2 点位于中心线同侧且距离严格大于 2σ；提示较小但系统性的偏移。 | 窗口、侧别、命中点及其标准化距离 |
| `four_of_five_beyond_1sigma` | 5 点中至少 4 点同侧超过 1σ | 5 点窗口内至少 4 点位于同侧且距离严格大于 1σ；提示过程均值发生小幅移动。 | 窗口、侧别、命中点及其标准化距离 |
| `fifteen_within_1sigma` | 连续 15 点全部落在 1σ 内 | 连续 15 个点的绝对中心线距离严格小于 σ；提示控制限可能过宽或数据存在分层。 | 窗口、sigma 来源、分层调查提示 |
| `eight_beyond_1sigma` | 连续 8 点全部位于 1σ 外且同侧 | 连续 8 点均在中心线同一侧且绝对距离严格大于 σ；提示混合总体或双群模式。 | 窗口、侧别、分组/批次调查提示 |

规则显示契约：

```text
rule_id | rule_name | short_explanation | threshold | window | status
        | evidence_rows | plotted_points | suggested_action
```

“Zone”是距离中心线的分带/累积分数概念，不应与上述 Tests 混称。控制图类型不适用某规则时显示 `not_applicable` 和原因，不显示空白或伪造的“通过”。

## 5. 目标架构

### 5.1 依赖方向

```text
ui pages/widgets
    ↓
application: ImportService / ReportService / TableQueryService
    ↓
ports: DatabaseProvider / MetadataReader / RowReader / ProjectStore
    ↓
infrastructure: QSql provider adapters, SQLite project store, CSV/XLSX adapters
    ↓
Qt SQL drivers / native or ODBC drivers
```

`domain` 只接收标准化后的 `DataTable`、`ImportMetadata`、规则证据和报告 schema，不 include Qt SQL。`ui` 不调用 `QSqlDatabase::addDatabase()`，也不拼接用户输入的表名或列名。

### 5.2 provider 注册接口与工厂

建议先定义稳定的能力接口，再为 Qt SQL 写适配器。接口表达能力，不暴露 Qt 连接对象：

```cpp
struct DatabaseProviderDescriptor {
    std::string id;                 // sqlite, postgresql, mysql, odbc
    std::string display_name;
    std::vector<std::string> driver_names;
    ProviderCapabilities capabilities;
};

class IDatabaseProvider {
public:
    virtual ~IDatabaseProvider() = default;
    virtual DatabaseProviderDescriptor descriptor() const = 0;
    virtual Result<ConnectionInfo> connect(const ConnectionOptions&) = 0;
    virtual Result<std::vector<DatabaseObject>> list_objects(
        const ObjectFilter&) = 0;
    virtual Result<TableMetadata> describe(const ObjectRef&) = 0;
    virtual Result<QueryPreview> preview(const ImportPlan&, PreviewLimits) = 0;
    virtual Result<ImportedTable> import(const ImportPlan&, ImportLimits,
                                         CancellationToken) = 0;
};

class DatabaseProviderRegistry {
public:
    void register_factory(DatabaseProviderDescriptor,
                          std::function<std::unique_ptr<IDatabaseProvider>()>);
    std::vector<DatabaseProviderDescriptor> descriptors() const;
    std::unique_ptr<IDatabaseProvider> create(std::string_view provider_id) const;
};
```

注册表规则：

1. `id`、能力位和元数据字段是稳定契约；显示名称可翻译。
2. 注册发生在 infrastructure composition root；页面只消费 descriptor。
3. 重复 ID、空 factory、驱动不可用要在启动/连接页明确诊断。
4. provider 可返回 `unsupported`，不能用 SQLite SQL 方言静默降级。
5. 工厂创建的连接属于当前线程；Qt 官方要求 `QSqlDatabase` 只能被创建它的线程访问，后台读取使用专属连接和专属模型。

### 5.3 元数据模型

```text
ConnectionProfile
  provider_id, display_name, host, port, database, schema,
  credential_ref, ssl_mode, read_only, timeout_ms

DatabaseObject
  catalog, schema, name, kind(table/view/materialized_view/system),
  selectable, estimated_rows

ColumnMetadata
  ordinal, name, display_name, native_type, normalized_type,
  nullable, default_expression, primary_key, generated, hidden,
  selectable, read_only, source_expression

TableMetadata
  object, columns, primary_key_columns, foreign_keys,
  supports_keyset_paging, estimated_rows

ImportPlan
  source_connection_id, object_ref, selected_columns,
  filter_expression (structured, not raw UI SQL), order_key,
  row_limit, page_size, null_policy, target_worksheet_name
```

“反射数据库表与列”在这里指 provider 驱动的 schema introspection，不把 C++ RTTI 当数据库反射。Qt 侧可优先使用 `QSqlDatabase::tables()`、`record(tableName)`、`primaryIndex()` 与 `QSqlDriver::tables()/record()`，但 provider 必须保存原始能力差异和失败原因。

### 5.4 各数据库元数据策略

- SQLite：对象目录来自 SQLite schema/Qt driver；列优先使用官方 `PRAGMA table_xinfo` 以覆盖生成列和隐藏列，需要普通列时可用 `table_info`。表名/列名必须通过 provider 的 identifier quoting 处理。
- PostgreSQL：优先使用标准 `information_schema.tables` 和 `information_schema.columns`，按当前用户权限可见；需要 PostgreSQL 特有字段时再走受控 system catalog 查询。
- MySQL：使用 `INFORMATION_SCHEMA.TABLES` 与 `INFORMATION_SCHEMA.COLUMNS`，保留 `DATA_TYPE`、`IS_NULLABLE`、`COLUMN_DEFAULT`、`COLUMN_KEY`、`EXTRA`、生成表达式等信息；不要把 `TABLE_ROWS` 当精确行数。
- ODBC：使用 `SQLTables`、`SQLColumns`、必要时 `SQLPrimaryKeys`/`SQLForeignKeys`；将 catalog/schema/table type 原样保存，因为不同驱动对 catalog、schema 和视图类型的支持不同。
- Qt SQL 共性：`QSqlQueryModel` 适合任意查询的只读结果；`QSqlRelationalTableModel` 适合单表、可编辑且有外键关系的模型。本需求第一阶段是只读导入，因此默认 `QSqlQueryModel` 或自定义只读分页模型，不使用可编辑关系模型冒充导入工作表。

## 6. 导入流程与多页面 UI

### 6.1 页面层级/导航

主窗口保留工作表、分析和报告的上下文导航；数据库导入作为独立的向导/页面栈，不向主窗口继续添加几十个连接控件。

```text
项目
├─ 工作表
├─ 分析
├─ 报告
└─ 数据导入
   ├─ 1. 来源与连接
   ├─ 2. 数据对象
   ├─ 3. 列与过滤
   ├─ 4. 预览与质量检查
   └─ 5. 导入结果
```

页面职责：

1. 来源与连接：provider 卡片、连接配置、只读开关、连接测试、驱动/权限诊断。凭据使用 `credential_ref`，不在 `ImportPlan` 和报告中保存明文密码。
2. 数据对象：表/视图树、schema/catalog 过滤、搜索、对象类型、估计行数、刷新元数据。
3. 列与过滤：列多选、顺序、别名、类型映射、主键提示、结构化过滤器、排序键、行数上限。高级 SQL 另设可确认的高级页，不与普通选择控件混排。
4. 预览与质量检查：只读表格、有限行预览、缺失/类型转换/重复键统计、原始列名与目标列名对照。预览必须显示“样本/限制”标记。
5. 导入结果：进度、取消、已导入行数、警告、目标工作表名、来源摘要和“打开工作表/返回报告”动作。

禁止控件堆叠的具体规则：

- 每页一个主要任务和一个主要 CTA；连接、对象、列、预览不共用一个长滚动表单。
- 高级选项默认折叠或独立页；不把所有 provider 专属参数塞入通用页面。
- 表格工具栏只放筛选、列、刷新、复制、导入等与表格直接相关的动作；连接管理放来源页。
- 错误、警告和进度在页内状态区展示，不能以多个弹窗串联掩盖流程状态。
- 小屏/125% DPI 下页面仍可完成；长列设置用滚动区域，底部导航固定。

### 6.2 导入步骤

```text
connect
  → discover objects
  → describe selected object
  → create ImportPlan
  → preview with bound values
  → validate type/null/key policy
  → snapshot in transaction/chunk
  → populate DataTable or paged source
  → record provenance and diagnostics
```

导入到领域 `DataTable` 时：

- 只复制选中的列，按 `ImportPlan.selected_columns` 顺序生成工作表。
- 优先使用稳定主键作为 `row_ids`；没有主键时生成导入快照内的稳定行序号，并标记 `row_id_is_synthetic`。
- 逐列记录 native type 到 `ColumnType` 的映射；未知或混合类型必须进入 `cell_states`/warning，而不是静默转成数字 0。
- 明确 null、空字符串、数据库 NULL、无效转换和截断的区别。
- 保存 source provider、catalog/schema/table/view、列列表、过滤摘要、排序、导入时间、驱动版本和应用 schema version。

## 7. 表格交互增强与大数据策略

### 7.1 Qt Model/View 选择

Qt 官方 Model/View 文档把 `QAbstractItemModel` 定义为模型与视图/委托之间的标准接口；数据可以来自独立 repository、文件或数据库。建议分三种模型：

1. `WorksheetModel`：继续服务可编辑的本地 `DataTable`，补充 typed roles、原始 RowId、tooltip 和规则状态角色。
2. `DatabasePreviewModel`：只读 `QSqlQueryModel` 包装器或 `QAbstractTableModel` 分页实现，支持列元数据和诊断。
3. `ImportResultModel`：显示导入行数、跳过行、警告、目标工作表与来源，不把结果状态塞进数据表单元格。

必备交互：

- 点击表头排序时生成结构化 order key，只允许来自 `ColumnMetadata` 的白名单列。
- 快速筛选应使用 provider 参数绑定；过滤表达式显示为用户可读摘要。
- 列显隐不改变导入列顺序；拖拽排序必须同步到 `ImportPlan`。
- 复制提供可读文本和带列名/原始行 ID 的审计格式；空值、错误、被排除行使用不同表示。
- 单击表格行可联动工作表/报告/图表原始行；必须用 RowId，不使用分页后的视觉行号。
- `Qt::ToolTipRole`/`Qt::WhatsThisRole` 提供列类型、来源列、缺失和转换信息；不可只依赖颜色。

### 7.2 分页、增量加载和统计语义

- 预览默认 `LIMIT`/绑定参数；大数据使用 keyset pagination（优先主键/唯一排序键），无稳定键时才退回 offset 并显示性能限制。
- 对 `QAbstractItemModel` 的增量模型实现 `canFetchMore()`/`fetchMore()`，插入行时严格调用 `beginInsertRows()`/`endInsertRows()`。
- 查询在后台线程使用独立连接；UI 线程只接收模型信号和取消结果。取消要中断 query/读取任务，不能仅隐藏进度条。
- 分页视图只适合浏览和选择；需要全量统计的分析必须明确“已导入快照”“数据库聚合”“抽样”三种模式，报告中显示实际 N、过滤条件和未加载行数。
- 数据库聚合可以生成 `StatisticTable`，但不能把聚合值冒充原始观测行；图表命中提示应区分汇总桶、数据库行和工作表 RowId。

## 8. 错误、安全、事务和生命周期

### 8.1 错误分类

统一 `DiagnosticMessage`/`ImportDiagnostic` 代码：

```text
driver_missing | connection_failed | authentication_failed
permission_denied | metadata_failed | object_not_found
unsupported_feature | invalid_identifier | invalid_plan
conversion_failed | null_policy_violation | query_timeout
cancelled | transaction_failed | partial_import
```

每条错误包含用户动作、provider、对象、列（如适用）、底层错误的安全摘要、是否可重试和建议动作。驱动返回的密码、连接串密钥和完整 SQL 参数不得进入日志/报告。

### 8.2 安全边界

- 值使用 `QSqlQuery::prepare()`/bind；标识符只能从已发现元数据白名单选择，并由 provider 生成 quoted identifier。
- 默认只读连接；导入页面不执行 DDL/DML，不接受任意多语句脚本。
- 凭据存系统凭据库或应用已有 secret reference，不存项目 JSON、SQLite 项目表和报告。
- ODBC DSN 和 PostgreSQL/MySQL 连接信息分离于 `ImportPlan`；报告只存脱敏 source summary。
- 服务器端 row limit、query timeout、最大列数/最大单元格大小和取消都要有上限。
- 最小权限：只申请连接、读取 metadata、读取选定对象所需权限；权限不足时显示可见对象范围，不反复尝试提升权限。

### 8.3 事务与一致性

- 元数据快照和导入查询应带 source snapshot time；数据库发生变化时，报告写明“非单一快照”风险。
- materialize 到工作表时，先写临时目标/内存 staging，完成转换和 contract validation 后再一次性提交到项目状态。
- SQLite 项目保存使用事务；任一列/行写入失败都回滚，成功提交后才更新 UI 的工作表导航。
- 分块导入需要 checkpoint 和可恢复策略；第一阶段可选择“失败即全回滚”，但必须展示已读取和未提交行数。
- 连接/模型销毁前关闭 query、清理模型，再关闭数据库并移除连接名，避免 Qt 全局连接注册表残留。

## 9. 报告 schema 与呈现顺序

当前 `OutputPage` 是良好起点，但应把字符串表升级为带语义的版本化 schema：

```json
{
  "schema_version": 2,
  "report_id": "spc_...",
  "title": "I-MR 控制图",
  "method": {"id": "imr", "name": "Individuals-Moving Range", "version": "1"},
  "source": {
    "dataset_id": "...", "worksheet": "...", "row_ids": [1, 2],
    "provider": "sqlite", "object": "schema.table",
    "columns": ["measurement"], "filter_summary": "...",
    "imported_at": "2026-08-21T..."
  },
  "parameters": {"sigma_method": "...", "rule_policy": "..."},
  "summary": [{"label": "有效 N", "value": 42, "unit": "rows"}],
  "rules": [{
    "id": "beyond_control_limit",
    "name": "单点超出 3σ 控制限",
    "status": "triggered",
    "explanation": "...",
    "threshold": {"kind": "control_limit", "comparison": "outside"},
    "evidence": {"row_ids": [7], "plot_points": [6]},
    "suggested_action": "..."
  }],
  "tables": [{
    "id": "point_statistics",
    "title": "逐点统计",
    "columns": [{
      "id": "source_row", "label": "原始行", "type": "row_id",
      "sortable": true, "visible_by_default": true
    }],
    "rows": []
  }],
  "plots": [],
  "diagnostics": [],
  "interpretation": []
}
```

页面顺序：

1. 结论摘要：方法、数据来源、有效 N、主要状态；不写“过程已合格”这类超出证据的结论。
2. 规则证据：规则名、状态、解释、阈值、触发行和建议动作。
3. 关键表格：默认显示最有决策价值的列，详细列可展开/列选择。
4. 图表：命中点与表格/工作表联动。
5. 诊断和假设：缺失、未验证、不适用、计算失败和参数来源。
6. 来源与复现：导入计划摘要、schema version、方法版本、驱动/provider 和时间。

表格列类型应至少包括 `text`、`number`、`percent`、`p_value`、`row_id`、`status`、`rule_id`、`timestamp`；renderer 负责格式，不用字符串反解析解释层。JSON round-trip 必须保留未知字段的兼容策略和版本迁移。

## 10. 可访问性验收

Qt 官方可访问性模型通过 `QAccessibleInterface` 向辅助技术暴露控件信息；表格还需提供列/行描述和模型变化事件。验收要求：

- 每个页面有唯一标题、焦点顺序和键盘可完成的下一步/返回/取消。
- 连接字段、对象树、列选择和过滤器都有 label、错误关联和可读状态。
- 表头、列类型、排序方向、筛选状态、当前行/列和规则状态可被辅助技术读取；不以颜色作为唯一状态表达。
- 进度、取消、错误和导入完成通过可访问名称/状态更新通知。
- 分页或增量插入时维护辅助技术对行数和模型变化的认知；模型 reset 必须说明上下文已改变。
- 125%/150% DPI、1080×680 和仅键盘操作下不截断主要动作。

## 11. 测试与验收矩阵

### 11.1 provider/元数据

- 注册表：重复 ID、空 factory、驱动缺失、能力声明和 create 失败。
- SQLite：普通列、主键、生成列、隐藏列、视图、空 schema、中文/空格标识符。
- PostgreSQL/MySQL：catalog/schema、权限可见性、NULL/default/type、视图和精确/估计行数区分。
- ODBC：catalog/schema/table type、SQLTables/SQLColumns 顺序、驱动不支持 catalog 或超长名称。
- Qt 模型：正确 row/column count、header/tooltips、排序白名单、分页、取消、线程边界和 query 错误。

### 11.2 导入契约

- 选列顺序、别名、NULL/空字符串/无效值、类型映射、主键 RowId 和无主键 synthetic RowId。
- 预览行数限制不影响最终 ImportPlan；过滤与排序可复现。
- 中途取消、网络断开、权限变化、超时、转换失败和事务回滚不留下半份工作表。
- 中文路径、中文表名、中文列名、空格/保留字标识符。
- 导入后 `DataTable` contract、`ImportMetadata`、warnings、source provenance 和 JSON round-trip。

### 11.3 报告/规则

- Test 1–8 每个规则用最小触发样本、边界等于值、缺失断点、阶段断点和不适用图种测试。
- 报告只出现稳定规则 ID + 具体规则名称；禁止用户可见的裸 `Test 1`。
- 规则表行可联动原始 RowId，图表和表格状态一致，复制内容包含规则名称和解释。
- schema version 迁移、未知字段保留/忽略策略、旧报告兼容。
- “未触发”“未验证”“不适用”“计算失败”四种状态视觉和机器字段不同。

### 11.4 UI/可访问性

- 每页独立测试：连接、对象、列、预览、结果；主窗口只验证导航入口和返回行为。
- 键盘 Tab 顺序、屏幕阅读器可读 label/status、DPI、最小窗口、长错误消息和取消。
- 1 千、10 万、100 万行的预览响应；确认没有一次性创建百万 QWidget。
- 表头排序、筛选、列隐藏、复制、原始行联动和分页后选择保持一致。

## 12. 分期 goal 竖切建议

每轮都应从 UI 入口到模型/应用服务/测试形成闭环，避免只交付空接口。

### Goal 1：规则可读性竖切（3–4 项）

1. 建立 `SpecialCauseRuleCatalog`，冻结 8 个稳定 ID、名称、解释、窗口和边界。
2. 将 `triggered_tests_text`、逐点表头、最小测试和摘要统一改为规则名称/状态；保留 ID 供 JSON。
3. 扩展报告表列元数据和规则证据，补 8 个最小样本 + 边界测试。
4. 在现有报告页面加入规则证据块、原始行联动和复制文本验收。

### Goal 2：只读数据库发现竖切（3–4 项）

1. 引入 provider descriptor/registry/factory 及 SQLite provider，封装连接生命周期。
2. 完成 SQLite 表、视图、`table_xinfo` 列元数据和 quoted identifier。
3. 新建“来源与连接 → 数据对象”两页，支持测试连接、搜索和权限/驱动错误。
4. 用 provider fake 做 registry/metadata/UI model 测试，不连接真实服务器。

### Goal 3：选择列导入竖切（3–4 项）

1. 定义 `ImportPlan`、`ColumnMetadata`、source provenance 和转换诊断。
2. 完成列选择 → 绑定过滤 → 有限预览 → 导入新工作表的闭环。
3. 加入事务 staging、取消、类型/null/RowId 契约和中文标识符测试。
4. 更新项目 schema/version，使导入来源可复现且不保存凭据。

### Goal 4：大表交互竖切（3–4 项）

1. 实现只读 `DatabasePreviewModel` 的分页/增量加载和排序白名单。
2. 加入列显隐、筛选、复制、固定 RowId/关键列和原始行联动。
3. 对 10 万/100 万行测试内存、响应、取消和分页选择稳定性。
4. 加入可访问表头、状态、模型变化和键盘操作验收。

### Goal 5：跨数据库 provider 竖切（3–4 项）

1. PostgreSQL provider：information_schema 表/列发现和只读预览。
2. MySQL provider：INFORMATION_SCHEMA 表/列发现、生成/隐藏字段提示。
3. ODBC provider：SQLTables/SQLColumns/主键能力和驱动差异诊断。
4. 每个 provider 明确安装前提、能力矩阵、脱敏日志和集成测试隔离策略。

## 13. 明确禁止偷懒清单

- 禁止只把 `Test 1` 替换成“规则 1”；必须给出具体规则名、判定窗口、阈值、解释、状态和建议。
- 禁止只改表头不改逐点 tooltip、复制文本、图表命中提示、帮助页、JSON 和解释层。
- 禁止在 UI 中直接调用 `QSqlDatabase::addDatabase()` 或按用户输入拼接 SQL/标识符。
- 禁止只注册一个空 provider 接口而没有可运行的 SQLite 端到端竖切。
- 禁止把“反射”实现成字符串猜测；必须有表/列元数据模型、权限可见性和 provider 能力声明。
- 禁止数据库导入仍固定第一个工作表/第一列；必须有对象选择、列选择和导入计划。
- 禁止把连接页、元数据树、列选择、预览和导入结果堆到一个巨型 dialog。
- 禁止用 `QTableWidget` 为每个数据库单元格创建 QWidget；大表必须用 Model/View。
- 禁止把分页后的视觉行号当稳定 RowId，禁止破坏图表/报告/工作表联动。
- 禁止用 offset 分页却不声明排序稳定性；禁止将估计行数当精确统计 N。
- 禁止把数据库 NULL、空字符串、无效数值和缺失标记静默混成同一个值。
- 禁止把密码、完整 DSN、绑定参数或底层敏感错误写入日志、项目文件或报告。
- 禁止导入失败后保留半份工作表；必须 staging + contract validation + 事务/回滚。
- 禁止把大表分页当成统计结果；报告必须写明快照、过滤、N、抽样或聚合语义。
- 禁止以颜色作为唯一可访问状态；必须提供文本、角色、tooltip/accessible name 和模型变化通知。
- 禁止只写 happy-path 测试；必须覆盖驱动缺失、权限、取消、超时、中文标识符、边界窗口和失败回滚。
- 禁止本研究目标变成“顺便重写所有 MainWindow/AnalysisService”；按竖切目标逐步收敛。

## 14. 高可信官方来源

以下来源为本研究的技术依据，访问日期均为 2026-08-21：

### Qt 6 官方文档

- [QSqlDatabase Class](https://doc.qt.io/qt-6/qsqldatabase.html)：连接、driver、`record()`、`primaryIndex()`、事务和线程访问约束。
- [QSqlDriver Class](https://doc.qt.io/qt-6/qsqldriver.html)：driver 能力、表列表、记录和标识符处理。
- [QSqlRecord Class](https://doc.qt.io/qt-6/qsqlrecord.html)：字段名称、类型/值、generated 字段和记录描述。
- [QSqlQueryModel Class](https://doc.qt.io/qt-6/qsqlquerymodel.html)：任意 SQL 查询的只读结果模型和 `QAbstractItemModel` 接口。
- [Using the SQL Model Classes](https://doc.qt.io/qt-6/sql-model.html)：`QSqlQueryModel`、`QSqlTableModel`、`QSqlRelationalTableModel` 的职责差异。
- [Model/View Programming](https://doc.qt.io/qt-6/model-view-programming.html)：模型、视图、委托分离；模型可连接独立 repository、文件或数据库。
- [QAbstractItemModel Class](https://doc.qt.io/qt-6/qabstractitemmodel.html)：模型最小接口、editable flags、`canFetchMore/fetchMore` 和 begin/end 插入约束。
- [Fetch More Example](https://doc.qt.io/qt-6/qtwidgets-itemviews-fetchmore-example.html)：大/无限数据按视图需要分批加载。
- [QTableView Class](https://doc.qt.io/qt-6/qtableview.html)：表格视图、header、delegate 和列尺寸行为。
- [Accessibility for QWidget Applications](https://doc.qt.io/qt-6/accessible-qwidget.html)：`QAccessibleInterface`、自定义控件可访问性和 interface factory。
- [QAccessibleTableInterface Class](https://doc.qt.io/qt-6/qaccessibletableinterface.html)：表格行列描述、选择和 model change。
- [QAccessibleTableModelChangeEvent Class](https://doc.qt.io/qt-6/qaccessibletablemodelchangeevent.html)：reset/data changed/rows inserted 等模型变化事件。

### 数据库官方文档

- [SQLite PRAGMA](https://sqlite.org/pragma.html)：`table_info`、`table_xinfo`、`table_list` 的列、生成列和隐藏列元数据。
- [PostgreSQL Information Schema](https://www.postgresql.org/docs/current/information-schema.html)：标准化、权限受限的信息 schema。
- [PostgreSQL `columns`](https://www.postgresql.org/docs/current/infoschema-columns.html)：表/视图列名、顺序、默认值、类型和当前用户可见范围。
- [MySQL INFORMATION_SCHEMA COLUMNS](https://dev.mysql.com/doc/refman/8.0/en/information-schema-columns-table.html)：列类型、可空、默认、主键、生成表达式等。
- [MySQL INFORMATION_SCHEMA TABLES](https://dev.mysql.com/doc/refman/en/information-schema-tables-table.html)：表对象、类型和行数等元数据，并说明部分统计是估计值。
- [Microsoft ODBC Catalog Functions](https://learn.microsoft.com/en-us/sql/odbc/reference/develop-app/catalog-functions-in-odbc?view=sql-server-ver17)：`SQLTables`、`SQLColumns`、主键/外键等 catalog 函数。
- [ODBC SQLTables](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqltables-function?view=sql-server-ver17)：catalog/schema/table type 结果集和驱动差异。
- [ODBC SQLColumns](https://learn.microsoft.com/en-us/sql/odbc/reference/syntax/sqlcolumns-function?view=sql-server-ver17)：列定义、顺序和查询前元数据发现。

## 15. 实现进度（工作树，2026-08-21）

| 竖切 | 状态 | 证据 |
|---|---|---|
| SpecialCauseRuleCatalog + 8 稳定 ID | ✅ | `src/domain/statistics/special_cause_rule_catalog.*`、`special_cause_rule_catalog_test` |
| 报告/表格/tooltip/解释改用规则名 | ✅ | `output_builder`、`chart_renderer`、`analysis_chart_widget`、`interpretation_service` |
| SpcFacts.rules JSON round-trip | ✅ | `output_serialization` + catalog test |
| Provider Registry + SQLite 元数据 | ✅ | `domain/database/*`、`sqlite_database_provider`、`database_provider_registry_test` |
| SQLite 列选择导入工作表 | ✅ | `ImportPlan` + `import_table`/`preview` + `database_import_test` + 向导五页 |
| ImportPlan JSON（无密码/DSN） | ✅ | `import_plan_serialization.*` |
| 数据库导入分层 UI | ✅ | `DatabaseImportWizard` 连接/对象/列/预览/结果 |
| PostgreSQL Provider | ⏸ 驱动门控 | QPSQL 可用则 information_schema + 导入；否则诚实 ❌ |
| MySQL Provider | ⏸ 驱动门控 | QMYSQL 可用则 INFORMATION_SCHEMA + 导入；否则诚实 ❌ |
| ODBC Provider | ⏸ 驱动门控 | QODBC 可用则 tables/record/primaryIndex；DSN；否则诚实 ❌ |
| 复合主键 keyset | ✅ | `KeysetCursor` 多列 + `keyset_sql` OR 展开；`database_import_test` |
| 表格交互/大表策略 | ✅/⏸ | roles/筛选排序/RowId/ReportTableModel/keyset fetchMore；百万行压测仍 ⏸ |

手工验收：见本轮结束说明（Qt Creator）。

## 15. 结论

最短安全路径不是立即把 `WorksheetModel` 替换成某一个数据库模型，而是先把“规则语义、来源追溯、表格列元数据、导入计划”做成稳定契约，再以 SQLite provider 贯通一条真实竖切。这样既保留现有 C++ 领域层和报告证据，也为 PostgreSQL/MySQL/ODBC、分页大表和多页面 UI 留出可测试的扩展点。
