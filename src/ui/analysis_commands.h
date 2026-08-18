#pragma once

// 阶段 3.1 命令化：数据驱动的分析命令表。
// 每个分析一行数据（菜单文字/对话框标题/角色与输入规格/图标/菜单路径），
// 配置构建（apply）与执行（run）为两个可独立维护的 lambda。
// MainWindow 只保留通用 run_from_spec(id)，新增分析 = 向 all() 追加一行。

#include "application/analysis_intent.h"
#include "domain/quality_types.h"

#include <QString>

#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace analysis_commands {

struct RoleSpec {
    QString id;
    QString label;
    bool multi = false;
    bool optional = false;
    std::vector<datalab::domain::ColumnType> allowed_types;
    int minimum_count = 0;
    int maximum_count = 0;
    QString help;
};

enum class InputKind {
    text,
    integer,
    number,
    percentage,
    choice,
    boolean,
    special_cause_tests
};

struct InputSpec {
    QString id;
    QString label;
    QString placeholder;
    InputKind kind = InputKind::text;
    std::optional<double> minimum;
    std::optional<double> maximum;
    QString unit;
    QString group = QStringLiteral("主要选项");
    QString help;
    bool advanced = false;
    std::vector<std::pair<QString, QString>> choices;

    InputSpec() = default;
    InputSpec(QString input_id, QString input_label, QString input_placeholder);
};

// apply 的校验结果：valid=false 表示中止；error_title 非空时由
// run_from_spec 弹出提示（对应原 run_* 里的 QMessageBox），为空表示静默中止。
struct AnalysisApplyResult {
    bool valid = true;
    QString error_title;
    QString error_message;
    QString field_id;
};

struct AnalysisCommand {
    QString id;               // 命令标识（菜单动作 id）
    QString menu_label;       // 菜单项文字
    QString dialog_title;     // 设置对话框标题（可能与菜单文字不同）
    QString menu_path;        // 顶层菜单名（"统计"/"图形"/"控制图"/"质量工具"）
    QString icon_file;        // 资源图标文件名（不含 .svg，路径 :/icons/<file>.svg）
    bool separator_before = false;  // 在该菜单项前插入分隔线
    bool requires_data = true;      // 运行前是否需要 ensure_data()（t_power 为 false）
    std::vector<RoleSpec> roles;    // 对话框 add_role 参数
    std::vector<InputSpec> inputs;  // 对话框 add_line_edit 参数
    std::function<AnalysisApplyResult(
        datalab::domain::AnalysisConfiguration&,
        const datalab::application::AnalysisIntent&)> apply;
    std::function<datalab::domain::OutputPage(
        const datalab::domain::DataTable&,
        const datalab::domain::AnalysisConfiguration&)> run;
    QString menu_group;
};

// 全量命令表（表顺序即菜单项顺序，按 menu_path 分组）。
// 返回指向静态存储的 const 引用，调用方不得修改。
const std::vector<AnalysisCommand>& all();

// 按 id 查找命令（未找到返回 nullptr；返回的指针指向静态存储，长期有效）。
const AnalysisCommand* find(const QString& id);

}  // namespace analysis_commands
