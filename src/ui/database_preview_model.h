#pragma once

#include "domain/database_types.h"
#include "domain/quality_types.h"

#include <QAbstractTableModel>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace datalab::ui {

class DatabasePreviewModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    using PageLoader = std::function<
        domain::DatabaseResult<domain::DataTable>(
            const domain::ImportPlan& plan,
            std::uint64_t offset,
            std::uint64_t page_size)>;

    explicit DatabasePreviewModel(QObject* parent = nullptr);

    void set_sample(const domain::DataTable& sample, bool truncated);
    void configure_paging(
        domain::ImportPlan plan,
        std::uint64_t page_size,
        PageLoader loader,
        std::vector<std::string> keyset_columns = {});
    void clear();

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    bool truncated() const;
    std::uint64_t loaded_rows() const;
    QString last_error() const;
    bool using_keyset_paging() const;

private:
    std::vector<int> keyset_column_indices() const;

    domain::DataTable table_;
    domain::ImportPlan plan_;
    PageLoader loader_;
    std::vector<std::string> keyset_columns_;
    std::uint64_t page_size_ = 50;
    bool truncated_ = false;
    bool paging_enabled_ = false;
    bool fetch_in_progress_ = false;
    bool using_keyset_ = false;
    QString last_error_;
};

}  // namespace datalab::ui
