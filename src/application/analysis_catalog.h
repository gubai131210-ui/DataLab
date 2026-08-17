#pragma once

#include <QString>

#include <vector>

namespace datalab::application {

struct AnalysisDescriptor {
    QString id;
    QString label;
    QString menu_path;
};

class AnalysisCatalog final {
public:
    static const std::vector<AnalysisDescriptor>& all();
};

}  // namespace datalab::application
