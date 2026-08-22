# G1+G2 人工验收清单（公式注册表 + 图表/表格复制）

> Track：G1 公式注册表 UI + G2 图表/表格复制  
> 研究：`docs/research/g1-g2-formula-registry-chart-copy.md`  
> 日期：2026-08-22  
> **策略**：G1+G2 已单独签收。后续 Track 采用 **连续交付 · 末尾统一测** → [`unified_track_acceptance_plan.md`](unified_track_acceptance_plan.md)

在 **Qt Creator Release/Debug** 构建通过后，按序勾选。

## 构建签收（Qt Creator）

| 项 | 结果 | 备注 |
|----|------|------|
| Run CMake（含 `Track G1+G2: 5 test targets registered`） | ✅ Debug | `build/Desktop_Qt_6_11_1_MinGW_64_bit_Debug` |
| **DataLab.exe** 编译 | ✅ | 2026-08-22 22:03，约 9m20s，无错误 |
| G1+G2 测试 | ✅ | 用户签收 2026-08-22 全 PASS |

## 预检（脚本侧）

```powershell
python tools/print_acceptance_status.py
python tools/verify_g1_g2_track.py
```

期望：**13/13** + **G1+G2 track preflight OK**。

## 自动化测试

```powershell
powershell -File tools/run_g1g2_tests.ps1
```

期望：**5/5 PASS**（用户签收 2026-08-22）。

## G1 公式注册表

- [x] **帮助 → 公式注册表** 打开独立窗口（非主窗口堆控件）
- [x] 搜索 `capability`：树中出现 id，右侧有公式块
- [x] 详情含 **Primary URL**（可点击）与 **research md 路径**
- [x] **复制 id** / **复制公式纯文本** 可用
- [x] **帮助 → 算法、公式与参考资料** → 选 `capability` → **在公式注册表中打开** → 跳到同一 id

## G2 图表复制

- [x] 运行 **I-MR** 或 **过程能力**（含图）
- [x] 点击图形 → **Ctrl+C** 或右键 **复制图形** → 粘贴 Word/画图有图
- [x] **编辑 → 复制图形**（Ctrl+Shift+C）同样有效
- [x] 有 **hidden/excluded** 时：粘贴图下方可见行可见性脚注
- [x] 输出页 **解读区空白处** 焦点 → **Ctrl+C** → 复制当前页第一张/焦点图
- [x] **Sixpack** 多图：点选某图再复制，应为该图（非总是第一张）

## G2 表格复制

- [x] 输出页统计表：右键 **复制表格（TSV）** → 粘贴 Excel
- [x] 选中若干行 → **Ctrl+C** → 仅选中行 + 表头
- [x] 输出统计表焦点 → **编辑 → 复制** → TSV（勿误复制图形）
- [x] 有 hidden/excluded 时：末尾 `# 行可见性契约…` 注释行
- [x] **导出 CSV** 同样含注释脚注

## 回归（勿破坏）

- [x] hidden / excluded 语义与 Phase 7 一致（数据菜单隐藏 vs 排除）
- [x] 工作表 **Delete** 清除单元格 + **Ctrl+Z** 撤销仍正常
- [x] 导入 A→B 后旧输出页/排除行失效

## 签署

| 项 | 结果 | 备注 |
|----|------|------|
| 脚本 13/13 | PASS | |
| DataLab Debug 编译 | PASS | 2026-08-22 22:03 |
| G1 | PASS | 用户签收 |
| G2 | PASS | 用户签收 |
| 测试 5/5 | PASS | 用户签收 2026-08-22 |
| 验收人 | 用户 | |
| 日期 | 2026-08-22 | **Track G1+G2 COMPLETE** |
