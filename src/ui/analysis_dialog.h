#pragma once

#include <QDialog>

#include <optional>

class QComboBox;
class QLineEdit;

class AnalysisDialog final : public QDialog {
public:
    struct Configuration {
        int measurement_column = -1;
        std::optional<double> lower_specification;
        std::optional<double> upper_specification;
    };

    explicit AnalysisDialog(
        const QStringList& columns,
        QWidget* parent = nullptr);

    Configuration configuration() const;

private:
    QComboBox* measurement_column_ = nullptr;
    QLineEdit* lower_specification_ = nullptr;
    QLineEdit* upper_specification_ = nullptr;
};
