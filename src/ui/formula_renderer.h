#pragma once

#include "ui/algorithm_help_types.h"

#include <QString>

class FormulaRenderer {
public:
    static QString to_html(const QVector<FormulaNode>& nodes);
    static QString to_plain_text(const QVector<FormulaNode>& nodes);
    static QString plain_text_to_html(const QString& text);
    static QString escape_html(const QString& text);
};
