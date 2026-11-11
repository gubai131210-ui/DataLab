#include "infrastructure/table_fingerprint.h"

#include <array>
#include <cstdint>
#include <string_view>

#include <QByteArrayView>
#include <QCryptographicHash>

namespace datalab::infrastructure {
namespace {

void append_count(QCryptographicHash& hash, std::uint64_t count)
{
    std::array<char, 8> encoded{};
    for (std::size_t byte = 0; byte < encoded.size(); ++byte) {
        encoded[byte] = static_cast<char>((count >> (byte * 8)) & 0xffU);
    }
    hash.addData(QByteArrayView(encoded.data(), encoded.size()));
}

void append_string(QCryptographicHash& hash, std::string_view value)
{
    append_count(hash, value.size());
    hash.addData(QByteArrayView(value.data(), static_cast<qsizetype>(value.size())));
}

} // namespace

std::string table_fingerprint(const domain::DataTable& table)
{
    if (table.columns.empty() || table.rows.empty()) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    append_string(hash, table.name);
    append_string(hash, table.source_path);
    append_count(hash, table.columns.size());
    for (const std::string& column : table.columns) {
        append_string(hash, column);
    }
    append_count(hash, table.rows.size());
    for (const auto& row : table.rows) {
        for (std::size_t column = 0; column < table.columns.size(); ++column) {
            append_string(hash, column < row.size() ? std::string_view(row[column])
                                                   : std::string_view{});
        }
    }
    return "worksheet-sha256-v1:" + hash.result().toHex().toStdString();
}

} // namespace datalab::infrastructure
