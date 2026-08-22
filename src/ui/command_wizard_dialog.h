#pragma once

// G6 命令 Wizard：选列 → 意图 → 推荐列表 → 发出打开既有分析的信号。
// 不调用 AnalysisService；不自己跑 AnalysisSetupDialog。

#include "domain/quality_types.h"

#include <QDialog>
#include <QString>
#include <QStringList>

#include <vector>

class QComboBox;
class QLabel;
class QListWidget;
class QPushButton;
class QStackedWidget;

namespace datalab::ui {

class CommandWizardDialog final : public QDialog {
    Q_OBJECT

public:
    CommandWizardDialog(
        const QStringList& column_names,
        const std::vector<datalab::domain::ColumnType>& column_types,
        QWidget* parent = nullptr);
    ~CommandWizardDialog() override = default;

signals:
    // openAnalysisRequested：确认后发出；MainWindow 应先关本对话框再 run_from_spec。
    void openAnalysisRequested(const QString& command_id);

private slots:
    void go_next();
    void go_back();
    void refresh_recommendations();
    void open_selected_command();

private:
    void build_pages();
    void update_navigation();
    std::vector<datalab::domain::ColumnType> selected_column_types() const;
    QString selected_command_id() const;

    QStringList column_names_;
    std::vector<datalab::domain::ColumnType> column_types_;

    QStackedWidget* stack_ = nullptr;
    QLabel* page_title_ = nullptr;
    QLabel* status_label_ = nullptr;
    QListWidget* column_list_ = nullptr;
    QComboBox* intent_combo_ = nullptr;
    QListWidget* recommendation_list_ = nullptr;
    QPushButton* back_button_ = nullptr;
    QPushButton* next_button_ = nullptr;
    QPushButton* open_button_ = nullptr;
    QPushButton* cancel_button_ = nullptr;
};

}  // namespace datalab::ui
