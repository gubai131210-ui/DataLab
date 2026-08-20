#include "infrastructure/excel_table_importer.h"

#include "domain/column_extract.h"
#include "miniz.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTimeZone>
#include <QXmlStreamReader>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace datalab::infrastructure {
namespace {

constexpr int kMaxZipEntryCount = 4096;
constexpr int kMaxZipEntryBytes = 64 * 1024 * 1024;
constexpr int kMaxWorksheetRows = 1'000'000;
constexpr int kMaxWorksheetColumns = 16'384;

struct ZipEntry {
    QString path;
    QByteArray data;
};

struct CellReference {
    int row = 0;
    int column = 0;
};

struct ParsedCell {
    int row = 0;
    int column = 0;
    QString text;
};

struct FirstSheetInfo {
    QString path;
    QString name;
};

bool read_zip_entries(const QByteArray& archive, std::vector<ZipEntry>* entries, QString* error_message)
{
    if (entries == nullptr) {
        return false;
    }
    entries->clear();

    mz_zip_archive zip_archive {};
    if (!mz_zip_reader_init_mem(
            &zip_archive,
            archive.constData(),
            static_cast<size_t>(archive.size()),
            0)) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("不是有效的 XLSX/ZIP 文件。");
        }
        return false;
    }

    const int file_count = static_cast<int>(mz_zip_reader_get_num_files(&zip_archive));
    if (file_count > kMaxZipEntryCount) {
        mz_zip_reader_end(&zip_archive);
        if (error_message != nullptr) {
            *error_message = QStringLiteral("ZIP 条目数量超出限制。");
        }
        return false;
    }

    for (int index = 0; index < file_count; ++index) {
        char filename[512] = {};
        if (!mz_zip_reader_get_filename(&zip_archive, static_cast<mz_uint>(index), filename, sizeof(filename))) {
            mz_zip_reader_end(&zip_archive);
            if (error_message != nullptr) {
                *error_message = QStringLiteral("ZIP 文件名读取失败。");
            }
            return false;
        }

        size_t uncompressed_size = 0;
        void* buffer = mz_zip_reader_extract_to_heap(
            &zip_archive, static_cast<mz_uint>(index), &uncompressed_size, 0);
        if (buffer == nullptr) {
            mz_zip_reader_end(&zip_archive);
            if (error_message != nullptr) {
                *error_message = QStringLiteral("ZIP 条目解压失败。");
            }
            return false;
        }
        if (uncompressed_size > static_cast<size_t>(kMaxZipEntryBytes)) {
            mz_free(buffer);
            mz_zip_reader_end(&zip_archive);
            if (error_message != nullptr) {
                *error_message = QStringLiteral("ZIP 条目过大，已中止导入。");
            }
            return false;
        }

        QByteArray data(static_cast<int>(uncompressed_size), Qt::Uninitialized);
        if (uncompressed_size > 0) {
            std::memcpy(data.data(), buffer, uncompressed_size);
        }
        mz_free(buffer);
        entries->push_back(ZipEntry{QString::fromUtf8(filename), data});
    }

    mz_zip_reader_end(&zip_archive);
    return true;
}

const ZipEntry* find_zip_entry(const std::vector<ZipEntry>& entries, const QString& path)
{
    for (const ZipEntry& entry : entries) {
        if (entry.path.compare(path, Qt::CaseInsensitive) == 0) {
            return &entry;
        }
    }
    return nullptr;
}

QString normalize_zip_path(QString path)
{
    path.replace('\\', '/');
    while (path.startsWith("./")) {
        path.remove(0, 2);
    }
    return path;
}

std::optional<CellReference> parse_cell_reference(const QString& reference)
{
    static const QRegularExpression pattern(QStringLiteral("^([A-Za-z]+)([0-9]+)$"));
    const QRegularExpressionMatch match = pattern.match(reference.trimmed());
    if (!match.hasMatch()) {
        return std::nullopt;
    }

    const QString letters = match.captured(1).toUpper();
    int column = 0;
    for (const QChar character : letters) {
        column = column * 26 + (character.unicode() - QChar('A').unicode() + 1);
    }
    --column;

    bool ok = false;
    const int row = match.captured(2).toInt(&ok) - 1;
    if (!ok || row < 0 || column < 0 || column >= kMaxWorksheetColumns) {
        return std::nullopt;
    }
    return CellReference{row, column};
}

QString decode_xml_text(const QString& value)
{
    QString decoded = value;
    decoded.replace("&lt;", "<");
    decoded.replace("&gt;", ">");
    decoded.replace("&amp;", "&");
    decoded.replace("&quot;", "\"");
    decoded.replace("&apos;", "'");
    return decoded;
}

QString format_numeric_text(const QString& raw_value, bool probably_date)
{
    bool ok = false;
    const double numeric_value = raw_value.toDouble(&ok);
    if (!ok) {
        return raw_value;
    }
    if (probably_date) {
        static constexpr double kExcelEpochOffset = 25569.0;
        const double day_fraction = numeric_value - kExcelEpochOffset;
        const qint64 milliseconds = static_cast<qint64>(day_fraction * 86400000.0);
        const QDateTime date_time = QDateTime::fromMSecsSinceEpoch(milliseconds, QTimeZone::UTC);
        if (date_time.isValid()) {
            return date_time.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        }
    }
    if (std::floor(numeric_value) == numeric_value) {
        return QString::number(static_cast<qlonglong>(numeric_value));
    }
    return raw_value;
}

std::vector<QString> parse_shared_strings(const QByteArray& xml, QString* error_message)
{
    std::vector<QString> shared_strings;
    QXmlStreamReader reader(xml);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == QStringLiteral("si")) {
            QString text;
            while (!(reader.isEndElement() && reader.name() == QStringLiteral("si"))) {
                reader.readNext();
                if (reader.isStartElement() && reader.name() == QStringLiteral("t")) {
                    text += decode_xml_text(reader.readElementText());
                }
            }
            shared_strings.push_back(text);
        }
    }
    if (reader.hasError()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("共享字符串 XML 解析失败。");
        }
        return {};
    }
    return shared_strings;
}

std::unordered_map<int, bool> parse_date_format_ids(const QByteArray& styles_xml)
{
    std::unordered_map<int, bool> date_formats;
    QXmlStreamReader reader(styles_xml);
    std::vector<QString> number_formats;
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement()) {
            continue;
        }
        if (reader.name() == QStringLiteral("numFmt")) {
            const int format_id = reader.attributes().value(QStringLiteral("numFmtId")).toInt();
            const QString format_code = reader.attributes().value(QStringLiteral("formatCode")).toString();
            const bool is_date = format_code.contains('y', Qt::CaseInsensitive)
                || format_code.contains('d', Qt::CaseInsensitive)
                || format_code.contains('h', Qt::CaseInsensitive)
                || format_code.contains('m', Qt::CaseInsensitive);
            date_formats[format_id] = is_date;
        } else if (reader.name() == QStringLiteral("cellXfs")) {
            while (!(reader.isEndElement() && reader.name() == QStringLiteral("cellXfs"))) {
                reader.readNext();
                if (reader.isStartElement() && reader.name() == QStringLiteral("xf")) {
                    const int format_id = reader.attributes().value(QStringLiteral("numFmtId")).toInt();
                    number_formats.push_back(date_formats.count(format_id) != 0 && date_formats[format_id]
                        ? QStringLiteral("date")
                        : QString());
                }
            }
        }
    }

    std::unordered_map<int, bool> style_date_flags;
    for (int index = 0; index < static_cast<int>(number_formats.size()); ++index) {
        style_date_flags[index] = number_formats[static_cast<std::size_t>(index)] == QStringLiteral("date");
    }
    return style_date_flags;
}

std::optional<FirstSheetInfo> resolve_first_sheet(
    const ZipEntry& workbook_xml,
    const std::vector<ZipEntry>& entries,
    QString* error_message)
{
    QString first_relationship_id;
    QString first_sheet_name;
    QXmlStreamReader workbook_reader(workbook_xml.data);
    while (!workbook_reader.atEnd()) {
        workbook_reader.readNext();
        if (workbook_reader.isStartElement() && workbook_reader.name() == QStringLiteral("sheet")) {
            if (first_relationship_id.isEmpty()) {
                first_relationship_id = workbook_reader.attributes().value(QStringLiteral("r:id")).toString();
                first_sheet_name = workbook_reader.attributes().value(QStringLiteral("name")).toString();
            }
        }
    }
    if (first_relationship_id.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("Excel 工作簿不包含工作表。");
        }
        return std::nullopt;
    }

    const ZipEntry* workbook_rels = find_zip_entry(entries, QStringLiteral("xl/_rels/workbook.xml.rels"));
    if (workbook_rels == nullptr) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("缺少 workbook 关系文件。");
        }
        return std::nullopt;
    }

    QString target_path;
    QXmlStreamReader rels_reader(workbook_rels->data);
    while (!rels_reader.atEnd()) {
        rels_reader.readNext();
        if (rels_reader.isStartElement() && rels_reader.name() == QStringLiteral("Relationship")) {
            if (rels_reader.attributes().value(QStringLiteral("Id")).toString() == first_relationship_id) {
                target_path = normalize_zip_path(
                    rels_reader.attributes().value(QStringLiteral("Target")).toString());
                break;
            }
        }
    }
    if (target_path.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("无法定位第一个工作表。");
        }
        return std::nullopt;
    }
    if (!target_path.startsWith("xl/")) {
        target_path = QStringLiteral("xl/") + target_path;
    }
    return FirstSheetInfo{target_path, first_sheet_name};
}

std::vector<ParsedCell> parse_worksheet_cells(
    const QByteArray& worksheet_xml,
    const std::vector<QString>& shared_strings,
    const std::unordered_map<int, bool>& style_date_flags,
    QString* error_message)
{
    std::vector<ParsedCell> cells;
    QXmlStreamReader reader(worksheet_xml);
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement() || reader.name() != QStringLiteral("c")) {
            continue;
        }

        const QString reference = reader.attributes().value(QStringLiteral("r")).toString();
        const std::optional<CellReference> parsed_reference = parse_cell_reference(reference);
        if (!parsed_reference.has_value()) {
            continue;
        }
        if (parsed_reference->row >= kMaxWorksheetRows) {
            if (error_message != nullptr) {
                *error_message = QStringLiteral("工作表行数超出限制。");
            }
            return {};
        }

        const QString cell_type = reader.attributes().value(QStringLiteral("t")).toString();
        const int style_index = reader.attributes().value(QStringLiteral("s")).toInt();
        const bool probably_date = style_date_flags.count(style_index) != 0
            && style_date_flags.at(style_index);

        QString raw_value;
        QString inline_text;
        while (!(reader.isEndElement() && reader.name() == QStringLiteral("c"))) {
            reader.readNext();
            if (reader.isStartElement() && reader.name() == QStringLiteral("v")) {
                raw_value = reader.readElementText();
            } else if (reader.isStartElement() && reader.name() == QStringLiteral("is")) {
                while (!(reader.isEndElement() && reader.name() == QStringLiteral("is"))) {
                    reader.readNext();
                    if (reader.isStartElement() && reader.name() == QStringLiteral("t")) {
                        inline_text += decode_xml_text(reader.readElementText());
                    }
                }
            }
        }

        QString text;
        if (cell_type == QStringLiteral("s")) {
            bool ok = false;
            const int shared_index = raw_value.toInt(&ok);
            if (!ok || shared_index < 0
                || shared_index >= static_cast<int>(shared_strings.size())) {
                if (error_message != nullptr) {
                    *error_message = QStringLiteral("共享字符串索引无效。");
                }
                return {};
            }
            text = shared_strings[static_cast<std::size_t>(shared_index)];
        } else if (cell_type == QStringLiteral("inlineStr")) {
            text = inline_text;
        } else if (cell_type == QStringLiteral("b")) {
            text = raw_value == QStringLiteral("1") ? QStringLiteral("True") : QStringLiteral("False");
        } else if (!raw_value.isEmpty()) {
            text = format_numeric_text(raw_value, probably_date);
        } else {
            text = inline_text;
        }

        cells.push_back(
            ParsedCell{parsed_reference->row, parsed_reference->column, text});
    }

    if (reader.hasError()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("工作表 XML 解析失败。");
        }
        return {};
    }
    return cells;
}

domain::DataTable build_table_from_cells(
    const QString& file_path,
    const QString& sheet_name,
    const std::vector<ParsedCell>& cells)
{
    int max_row = -1;
    int max_column = -1;
    for (const ParsedCell& cell : cells) {
        max_row = std::max(max_row, cell.row);
        max_column = std::max(max_column, cell.column);
    }

    domain::DataTable table;
    table.name = QFileInfo(file_path).completeBaseName().toStdString();
    table.source_path = file_path.toStdString();
    table.import_metadata.sheet_name = sheet_name.toStdString();
    table.import_metadata.sheet_index = 0;

    if (max_row < 0 || max_column < 0) {
        return table;
    }

    std::vector<std::vector<QString>> grid(
        static_cast<std::size_t>(max_row + 1),
        std::vector<QString>(static_cast<std::size_t>(max_column + 1)));
    for (const ParsedCell& cell : cells) {
        grid[static_cast<std::size_t>(cell.row)][static_cast<std::size_t>(cell.column)] = cell.text;
    }

    table.columns.reserve(static_cast<std::size_t>(max_column + 1));
    for (int column = 0; column <= max_column; ++column) {
        table.columns.push_back(
            grid[0][static_cast<std::size_t>(column)].toStdString());
    }
    for (std::size_t column = 0; column < table.columns.size(); ++column) {
        if (table.columns[column].empty()) {
            table.import_warnings.push_back(
                "第 " + std::to_string(column + 1) + " 列的列名为空。");
        }
        if (std::find(table.columns.begin(), table.columns.begin() + column, table.columns[column])
            != table.columns.begin() + column) {
            table.import_warnings.push_back("检测到重复列名：" + table.columns[column]);
        }
    }

    for (int row = 1; row <= max_row; ++row) {
        std::vector<std::string> row_values;
        row_values.reserve(static_cast<std::size_t>(max_column + 1));
        for (int column = 0; column <= max_column; ++column) {
            row_values.push_back(
                grid[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)].toStdString());
        }
        table.rows.push_back(std::move(row_values));
    }

    domain::populate_data_table_contract(table);
    table.import_metadata.schema_version = 1;
    table.import_metadata.warnings = table.import_warnings;
    return table;
}

}  // namespace

std::optional<domain::DataTable> ExcelTableImporter::import_file(
    const QString& file_path,
    QString* error_message)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error_message != nullptr) {
            *error_message = file.errorString();
        }
        return std::nullopt;
    }

    const QByteArray archive = file.readAll();
    if (archive.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("Excel 文件为空。");
        }
        return std::nullopt;
    }

    std::vector<ZipEntry> entries;
    if (!read_zip_entries(archive, &entries, error_message)) {
        return std::nullopt;
    }

    const ZipEntry* workbook = find_zip_entry(entries, QStringLiteral("xl/workbook.xml"));
    if (workbook == nullptr) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("缺少 Excel workbook 定义。");
        }
        return std::nullopt;
    }

    const std::optional<FirstSheetInfo> first_sheet = resolve_first_sheet(*workbook, entries, error_message);
    if (!first_sheet.has_value()) {
        return std::nullopt;
    }

    const ZipEntry* worksheet = find_zip_entry(entries, first_sheet->path);
    if (worksheet == nullptr) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("找不到第一个工作表内容。");
        }
        return std::nullopt;
    }

    std::vector<QString> shared_strings;
    if (const ZipEntry* shared_strings_xml = find_zip_entry(entries, QStringLiteral("xl/sharedStrings.xml"));
        shared_strings_xml != nullptr) {
        shared_strings = parse_shared_strings(shared_strings_xml->data, error_message);
        if (error_message != nullptr && !error_message->isEmpty()) {
            return std::nullopt;
        }
    }

    std::unordered_map<int, bool> style_date_flags;
    if (const ZipEntry* styles_xml = find_zip_entry(entries, QStringLiteral("xl/styles.xml"));
        styles_xml != nullptr) {
        style_date_flags = parse_date_format_ids(styles_xml->data);
    }

    const std::vector<ParsedCell> cells = parse_worksheet_cells(
        worksheet->data, shared_strings, style_date_flags, error_message);
    if (error_message != nullptr && !error_message->isEmpty()) {
        return std::nullopt;
    }

    domain::DataTable table = build_table_from_cells(file_path, first_sheet->name, cells);
    if (table.columns.empty()) {
        if (error_message != nullptr) {
            *error_message = QStringLiteral("Excel 工作表没有可导入的列。");
        }
        return std::nullopt;
    }
    return table;
}

}  // namespace datalab::infrastructure
