#include "infrastructure/report_export_writer.h"

#include "infrastructure/pdf_report_writer.h"
#include "infrastructure/report_serialization.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QStringList>

#include <cstdlib>

namespace datalab::infrastructure {
namespace {

void remove_if_exists(const QString& path)
{
    if (QFile::exists(path)) {
        QFile::remove(path);
    }
}

bool atomic_replace(const QString& temp_path, const QString& final_path, QString* error)
{
    remove_if_exists(final_path);
    if (!QFile::rename(temp_path, final_path)) {
        if (!QFile::copy(temp_path, final_path)) {
            if (error != nullptr) {
                *error = QStringLiteral("无法将临时文件替换为最终文件：%1").arg(final_path);
            }
            return false;
        }
    }
    QFile::remove(temp_path);
    return true;
}

QString truncate_output(const QByteArray& bytes, int max_chars = 400)
{
    QString text = QString::fromUtf8(bytes).trimmed();
    if (text.size() > max_chars) {
        text = text.left(max_chars) + QStringLiteral("…");
    }
    return text;
}

}  // namespace

domain::ExternalPdfaValidatorResult run_optional_verapdf(const QString& pdf_path)
{
    domain::ExternalPdfaValidatorResult result;
    result.validator_name = "veraPDF";
    const char* configured = std::getenv("DATALAB_VERAPDF");
    if (configured == nullptr || configured[0] == '\0') {
        result.tool_configured = false;
        result.notes = "DATALAB_VERAPDF unset; PDF/A remains not_validated.";
        return result;
    }
    result.tool_configured = true;
    const QString tool = QString::fromLocal8Bit(configured);
    if (!QFileInfo::exists(tool)) {
        result.tool_available = false;
        result.notes = "DATALAB_VERAPDF path does not exist.";
        return result;
    }
    result.tool_available = true;
    if (pdf_path.isEmpty() || !QFileInfo::exists(pdf_path)) {
        result.tool_invoked = false;
        result.notes = "PDF path missing; validator not invoked.";
        return result;
    }

    QProcess version_process;
    version_process.start(tool, {QStringLiteral("--version")});
    if (version_process.waitForFinished(15000)
        && version_process.exitStatus() == QProcess::NormalExit) {
        result.validator_version =
            truncate_output(version_process.readAllStandardOutput()
                            + version_process.readAllStandardError(),
                            120)
                .toStdString();
    }

    QProcess process;
    // Prefer non-interactive batch validation; callers must not treat missing
    // flags as a silent pass — non-zero exit becomes validated_fail after merge.
    process.start(tool, {pdf_path});
    if (!process.waitForStarted(10000)) {
        result.tool_available = false;
        result.tool_invoked = false;
        result.notes = "Failed to start configured veraPDF process.";
        return result;
    }
    if (!process.waitForFinished(120000)) {
        process.kill();
        process.waitForFinished(5000);
        result.tool_invoked = false;
        result.exit_code = -1;
        result.notes = "veraPDF timed out; PDF/A remains not_validated (not a pass).";
        return result;
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        result.tool_invoked = false;
        result.exit_code = -1;
        result.notes =
            "veraPDF crashed or was killed; PDF/A remains not_validated (not a pass).";
        return result;
    }
    result.tool_invoked = true;
    result.exit_code = process.exitCode();
    result.stdout_excerpt =
        truncate_output(process.readAllStandardOutput() + process.readAllStandardError())
            .toStdString();
    result.notes = result.stdout_excerpt.empty()
        ? ("veraPDF exit_code=" + std::to_string(result.exit_code))
        : result.stdout_excerpt;
    return result;
}

domain::ExternalPdfaValidatorResult run_optional_pac(const QString& pdf_path)
{
    domain::ExternalPdfaValidatorResult result;
    result.validator_name = "PAC";
    const char* configured = std::getenv("DATALAB_PAC");
    if (configured == nullptr || configured[0] == '\0') {
        result.tool_configured = false;
        result.notes =
            "DATALAB_PAC unset; PDF/UA remains unsupported on QPainter "
            "(no tagged-PDF). Do not claim PDF/UA compliance.";
        return result;
    }
    result.tool_configured = true;
    const QString tool = QString::fromLocal8Bit(configured);
    if (!QFileInfo::exists(tool)) {
        result.tool_available = false;
        result.notes = "DATALAB_PAC path does not exist; PDF/UA remains unsupported.";
        return result;
    }
    result.tool_available = true;
    if (pdf_path.isEmpty() || !QFileInfo::exists(pdf_path)) {
        result.tool_invoked = false;
        result.notes = "PDF path missing; PAC not invoked; PDF/UA remains unsupported.";
        return result;
    }

    QProcess version_process;
    version_process.start(tool, {QStringLiteral("--version")});
    if (version_process.waitForFinished(15000)
        && version_process.exitStatus() == QProcess::NormalExit) {
        result.validator_version =
            truncate_output(version_process.readAllStandardOutput()
                            + version_process.readAllStandardError(),
                            120)
                .toStdString();
    }

    QProcess process;
    process.start(tool, {pdf_path});
    if (!process.waitForStarted(10000)) {
        result.tool_available = false;
        result.tool_invoked = false;
        result.notes = "Failed to start configured PAC process; PDF/UA remains unsupported.";
        return result;
    }
    if (!process.waitForFinished(120000)) {
        process.kill();
        process.waitForFinished(5000);
        result.tool_invoked = false;
        result.exit_code = -1;
        result.notes =
            "PAC timed out; PDF/UA remains unsupported (timeout is not a UA pass).";
        return result;
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        result.tool_invoked = false;
        result.exit_code = -1;
        result.notes =
            "PAC crashed or was killed; PDF/UA remains unsupported (not a pass).";
        return result;
    }
    result.tool_invoked = true;
    result.exit_code = process.exitCode();
    result.stdout_excerpt =
        truncate_output(process.readAllStandardOutput() + process.readAllStandardError())
            .toStdString();
    // Honesty: QPainter exports are not tagged-PDF; exit 0 must never become UA pass.
    result.notes =
        "PAC exit_code=" + std::to_string(result.exit_code)
        + "; QPainter pipeline has no tagged-PDF — PDF/UA stays unsupported "
          "regardless of PAC exit code. Do not claim PDF/UA compliance."
        + (result.stdout_excerpt.empty() ? "" : (" " + result.stdout_excerpt));
    return result;
}

ReportExportResult export_report_package(
    const QString& pdf_path,
    const domain::ReportDocument& document,
    domain::ReportExportManifest manifest,
    const std::function<void(domain::ReportExportManifest&, const QString& temp_pdf)>&
        after_pdf)
{
    ReportExportResult result;
    result.pdf_path = pdf_path;
    result.manifest = std::move(manifest);

    const QFileInfo pdf_info(pdf_path);
    const QString directory = pdf_info.absolutePath();
    result.audit_json_path = directory + QLatin1Char('/')
        + QString::fromStdString(result.manifest.audit_json_relative_path);
    result.manifest_path = directory + QLatin1Char('/')
        + QFileInfo(QString::fromStdString(result.manifest.pdf_relative_path)).completeBaseName()
        + QStringLiteral(".manifest.json");
    if (!result.manifest.pdf_relative_path.empty()) {
        const QString audit_name =
            QString::fromStdString(result.manifest.audit_json_relative_path);
        if (!audit_name.isEmpty()) {
            result.audit_json_path =
                directory + QLatin1Char('/') + QFileInfo(audit_name).fileName();
        }
    }
    result.manifest_path = directory + QLatin1Char('/') + pdf_info.completeBaseName()
        + QStringLiteral(".manifest.json");

    const QString temp_pdf = result.pdf_path + QStringLiteral(".tmp");
    const QString temp_audit = result.audit_json_path + QStringLiteral(".tmp");
    const QString temp_manifest = result.manifest_path + QStringLiteral(".tmp");

    const auto cleanup_temps = [&] {
        remove_if_exists(temp_pdf);
        remove_if_exists(temp_audit);
        remove_if_exists(temp_manifest);
    };
    cleanup_temps();

    QString pdf_error;
    if (!PdfReportWriter::write_document(temp_pdf, document, &pdf_error)) {
        cleanup_temps();
        result.error_message =
            pdf_error.isEmpty() ? QStringLiteral("PDF 导出失败。") : pdf_error;
        return result;
    }
    if (!QFileInfo::exists(temp_pdf) || QFileInfo(temp_pdf).size() <= 0) {
        cleanup_temps();
        result.error_message = QStringLiteral("PDF 临时文件无效，已清理。");
        return result;
    }

    if (after_pdf) {
        after_pdf(result.manifest, temp_pdf);
    }

    {
        QFile audit_file(temp_audit);
        if (!audit_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            cleanup_temps();
            result.error_message = QStringLiteral("无法写入审计 JSON 临时文件。");
            return result;
        }
        const QJsonDocument json(report_document_to_json(document));
        if (audit_file.write(json.toJson(QJsonDocument::Indented)) <= 0) {
            audit_file.close();
            cleanup_temps();
            result.error_message = QStringLiteral("审计 JSON 写入失败，已清理临时文件。");
            return result;
        }
        audit_file.close();
    }

    {
        QFile manifest_file(temp_manifest);
        if (!manifest_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            cleanup_temps();
            result.error_message = QStringLiteral("无法写入 manifest 临时文件。");
            return result;
        }
        const QJsonDocument json(report_export_manifest_to_json(result.manifest));
        if (manifest_file.write(json.toJson(QJsonDocument::Indented)) <= 0) {
            manifest_file.close();
            cleanup_temps();
            result.error_message = QStringLiteral("manifest 写入失败，已清理临时文件。");
            return result;
        }
        manifest_file.close();
    }

    QString replace_error;
    if (!atomic_replace(temp_pdf, result.pdf_path, &replace_error)
        || !atomic_replace(temp_audit, result.audit_json_path, &replace_error)
        || !atomic_replace(temp_manifest, result.manifest_path, &replace_error)) {
        cleanup_temps();
        remove_if_exists(result.pdf_path);
        remove_if_exists(result.audit_json_path);
        remove_if_exists(result.manifest_path);
        result.error_message = replace_error.isEmpty()
            ? QStringLiteral("原子替换失败，已回滚导出文件。")
            : replace_error;
        return result;
    }

    cleanup_temps();
    result.ok = true;
    return result;
}

}  // namespace datalab::infrastructure
