DataLab 便携版（Windows 64 位）
================================

【使用方法】
1. 将整个 dist 文件夹复制到目标电脑（U 盘、网盘、压缩包均可）。
2. 解压后保持文件夹结构不变，双击 DataLab.exe 运行。
3. 不要把 DataLab.exe 单独拷出来，必须连同所有 DLL 和子文件夹一起使用。

【系统要求】
- Windows 10/11 64 位
- 无需安装 Qt、MinGW 或 Visual Studio
- MES 工具需能访问内网 MES 接口（网络连通）

【文件夹内必需内容】
- DataLab.exe
- Q_MES.ico
- Qt6*.dll、libgcc_s_seh-1.dll、libstdc++-6.dll、libwinpthread-1.dll
- platforms\qwindows.dll
- sqldrivers\qsqlite.dll
- tls\qschannelbackend.dll（MES HTTPS）
- help\learning_center.sqlite（学习中心演示数据库；程序内也已嵌入）
- imageformats\、iconengines\、styles\ 等插件目录

【学习中心】
- 菜单：帮助 → 学习中心
- 演示数据已嵌入 DataLab.exe（:/help/learning_center.sqlite）
- 同目录旁路副本：help\learning_center.sqlite（可用「导出 SQLite」或直接拷贝）
- **catalog 版本**：`learning-center-v2`（与程序内 `kExpectedCatalogVersion` 一致）
- **数据集策略**：专用主集 + 极小同构共享白名单（不再使用旧 10 张共享宽表）
- **导入工作表名**：`demo_{dataset_id}`（例如金标 `imr_spi_shift` → `demo_imr_spi_shift`；禁止把 id 再写成 `demo_…` 前缀）
- **升级后必做**：教学升级内容变更后，请在本机重新运行 `tools/package_dist.ps1` 生成新的 dist，确保旁路 `help\learning_center.sqlite` 与嵌入资源同步为 v2 catalog

【常见问题】
- 缺少 DLL：说明未完整复制 dist 文件夹，请重新复制整个目录。
- 无法启动：确认目标电脑为 64 位 Windows，且未被杀毒软件隔离 exe。
- 学习中心打不开/导入失败：确认 sqldrivers\qsqlite.dll 与 help\learning_center.sqlite 都在。
- 学习中心提示版本不匹配 / 禁用导入：当前 dist 仍是旧 catalog；请用含 `learning-center-v2` 的构建重新 `package_dist`。
