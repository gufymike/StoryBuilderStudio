// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include "storyboardexporter.h"

#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QMarginsF>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QTextStream>
#include <QUrl>
#include <QtEndian>
#include <array>

namespace {

// Standard IEEE 802.3 CRC-32 for standard PKZIP archive assembly
quint32 computeCrc32(const QByteArray &data)
{
    static const std::array<quint32, 256> table = []() {
        std::array<quint32, 256> t{};
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1U) ? (0xEDB88320U ^ (c >> 1U)) : (c >> 1U);
            }
            t[i] = c;
        }
        return t;
    }();

    quint32 crc = 0xFFFFFFFFU;
    const auto *bytes = reinterpret_cast<const quint8 *>(data.constData());
    for (qsizetype i = 0; i < data.size(); ++i) {
        crc = table[(crc ^ bytes[i]) & 0xFFU] ^ (crc >> 8U);
    }
    return crc ^ 0xFFFFFFFFU;
}

void appendLe16(QByteArray &buf, quint16 v)
{
    char raw[2];
    qToLittleEndian(v, raw);
    buf.append(raw, 2);
}

void appendLe32(QByteArray &buf, quint32 v)
{
    char raw[4];
    qToLittleEndian(v, raw);
    buf.append(raw, 4);
}

quint16 toDosTime(const QTime &t)
{
    return static_cast<quint16>(((t.hour() & 0x1F) << 11)
                                | ((t.minute() & 0x3F) << 5)
                                | ((t.second() / 2) & 0x1F));
}

quint16 toDosDate(const QDate &d)
{
    const int year = qBound(1980, d.year(), 2107);
    return static_cast<quint16>((((year - 1980) & 0x7F) << 9)
                                | ((d.month() & 0x0F) << 5)
                                | (d.day() & 0x1F));
}

struct ZipEntry {
    QByteArray utf8Name;
    QByteArray payload;
    quint32 crc32 = 0;
    quint32 localHeaderOffset = 0;
};

bool writeZipArchive(const QString &targetPath, QList<ZipEntry> &entries, QString &errorOut)
{
    QFile outFile(targetPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        errorOut = outFile.errorString();
        return false;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const quint16 dosTime = toDosTime(now.time());
    const quint16 dosDate = toDosDate(now.date());

    QByteArray archive;

    // 1. Local file headers + uncompressed (STORE) payloads
    for (ZipEntry &entry : entries) {
        entry.crc32 = computeCrc32(entry.payload);
        entry.localHeaderOffset = static_cast<quint32>(archive.size());

        appendLe32(archive, 0x04034B50U); // Local file header signature
        appendLe16(archive, 20);          // Version needed to extract (2.0)
        appendLe16(archive, 0x0800);      // General purpose bit flag: UTF-8 filename
        appendLe16(archive, 0);           // Compression method: 0 (STORE)
        appendLe16(archive, dosTime);
        appendLe16(archive, dosDate);
        appendLe32(archive, entry.crc32);
        appendLe32(archive, static_cast<quint32>(entry.payload.size()));
        appendLe32(archive, static_cast<quint32>(entry.payload.size()));
        appendLe16(archive, static_cast<quint16>(entry.utf8Name.size()));
        appendLe16(archive, 0);           // Extra field length
        archive.append(entry.utf8Name);
        archive.append(entry.payload);
    }

    // 2. Central directory records
    const quint32 centralDirOffset = static_cast<quint32>(archive.size());
    for (const ZipEntry &entry : entries) {
        appendLe32(archive, 0x02014B50U); // Central directory file header signature
        appendLe16(archive, 20);          // Version made by
        appendLe16(archive, 20);          // Version needed to extract
        appendLe16(archive, 0x0800);      // UTF-8 filename flag
        appendLe16(archive, 0);           // Compression method: STORE
        appendLe16(archive, dosTime);
        appendLe16(archive, dosDate);
        appendLe32(archive, entry.crc32);
        appendLe32(archive, static_cast<quint32>(entry.payload.size()));
        appendLe32(archive, static_cast<quint32>(entry.payload.size()));
        appendLe16(archive, static_cast<quint16>(entry.utf8Name.size()));
        appendLe16(archive, 0);           // Extra field length
        appendLe16(archive, 0);           // File comment length
        appendLe16(archive, 0);           // Disk number start
        appendLe16(archive, 0);           // Internal file attributes
        appendLe32(archive, 0);           // External file attributes
        appendLe32(archive, entry.localHeaderOffset);
        archive.append(entry.utf8Name);
    }
    const quint32 centralDirSize = static_cast<quint32>(archive.size()) - centralDirOffset;

    // 3. End of central directory record (EOCD)
    appendLe32(archive, 0x06054B50U);
    appendLe16(archive, 0); // Number of this disk
    appendLe16(archive, 0); // Disk where central directory starts
    appendLe16(archive, static_cast<quint16>(entries.size()));
    appendLe16(archive, static_cast<quint16>(entries.size()));
    appendLe32(archive, centralDirSize);
    appendLe32(archive, centralDirOffset);
    appendLe16(archive, 0); // ZIP file comment length

    if (outFile.write(archive) != archive.size()) {
        errorOut = outFile.errorString();
        outFile.close();
        return false;
    }

    outFile.flush();
    outFile.close();
    return true;
}

QString escapeXml(const QString &text)
{
    QString out;
    out.reserve(text.size() + 16);
    for (const QChar ch : text) {
        switch (ch.unicode()) {
        case u'&':  out += QLatin1String("&amp;"); break;
        case u'<':  out += QLatin1String("&lt;"); break;
        case u'>':  out += QLatin1String("&gt;"); break;
        case u'"':  out += QLatin1String("&quot;"); break;
        case u'\'': out += QLatin1String("&apos;"); break;
        default:
            // Strip illegal XML 1.0 control chars while keeping \t, \n, \r
            if (ch.unicode() >= 0x20 || ch == u'\t' || ch == u'\n' || ch == u'\r') {
                out += ch;
            }
            break;
        }
    }
    return out;
}

QString buildTextParagraphsXml(const QString &multilineText,
                               int fontSizeHundredthsPt,
                               const QString &hexRgb,
                               bool bold = false)
{
    const QStringList lines = multilineText.split(u'\n');
    QString xml;
    for (const QString &line : lines) {
        xml += QStringLiteral(
                   "<a:p>"
                   "<a:r>"
                   "<a:rPr lang=\"en-US\" sz=\"%1\" b=\"%2\" dirty=\"0\">"
                   "<a:solidFill><a:srgbClr val=\"%3\"/></a:solidFill>"
                   "<a:latin typeface=\"Calibri\"/>"
                   "</a:rPr>"
                   "<a:t>%4</a:t>"
                   "</a:r>"
                   "</a:p>")
                   .arg(fontSizeHundredthsPt)
                   .arg(bold ? QStringLiteral("1") : QStringLiteral("0"),
                        hexRgb,
                        escapeXml(line));
    }
    return xml;
}

QString buildShapeXml(int shapeId,
                      const QString &shapeName,
                      qint64 x,
                      qint64 y,
                      qint64 cx,
                      qint64 cy,
                      const QString &fillHex,
                      const QString &borderHex,
                      const QString &paragraphsXml,
                      const QString &anchor = QStringLiteral("t"))
{
    const QString fillXml = fillHex.isEmpty()
    ? QStringLiteral("<a:noFill/>")
    : QStringLiteral("<a:solidFill><a:srgbClr val=\"%1\"/></a:solidFill>").arg(fillHex);

    const QString lineXml = borderHex.isEmpty()
                                ? QStringLiteral("<a:ln><a:noFill/></a:ln>")
                                : QStringLiteral("<a:ln w=\"12700\"><a:solidFill><a:srgbClr val=\"%1\"/></a:solidFill></a:ln>").arg(borderHex);

    const QString bodyXml = paragraphsXml.isEmpty()
                                ? QStringLiteral("<a:p><a:endParaRPr lang=\"en-US\"/></a:p>")
                                : paragraphsXml;

    return QStringLiteral(
               "<p:sp>"
               "<p:nvSpPr>"
               "<p:cNvPr id=\"%1\" name=\"%2\"/>"
               "<p:cNvSpPr/>"
               "<p:nvPr/>"
               "</p:nvSpPr>"
               "<p:spPr>"
               "<a:xfrm><a:off x=\"%3\" y=\"%4\"/><a:ext cx=\"%5\" cy=\"%6\"/></a:xfrm>"
               "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>"
               "%7"
               "%8"
               "</p:spPr>"
               "<p:txBody>"
               "<a:bodyPr wrap=\"square\" lIns=\"108000\" tIns=\"72000\" rIns=\"108000\" bIns=\"72000\" anchor=\"%9\"/>"
               "<a:lstStyle/>"
               "%10"
               "</p:txBody>"
               "</p:sp>")
        .arg(shapeId)
        .arg(escapeXml(shapeName))
        .arg(x)
        .arg(y)
        .arg(cx)
        .arg(cy)
        .arg(fillXml, lineXml, anchor, bodyXml);
}

QString buildPictureXml(int shapeId,
                        const QString &picName,
                        const QString &relId,
                        qint64 x,
                        qint64 y,
                        qint64 cx,
                        qint64 cy)
{
    return QStringLiteral(
               "<p:pic>"
               "<p:nvPicPr>"
               "<p:cNvPr id=\"%1\" name=\"%2\"/>"
               "<p:cNvPicPr><a:picLocks noChangeAspect=\"1\"/></p:cNvPicPr>"
               "<p:nvPr/>"
               "</p:nvPicPr>"
               "<p:blipFill>"
               "<a:blip r:embed=\"%3\"/>"
               "<a:stretch><a:fillRect/></a:stretch>"
               "</p:blipFill>"
               "<p:spPr>"
               "<a:xfrm><a:off x=\"%4\" y=\"%5\"/><a:ext cx=\"%6\" cy=\"%7\"/></a:xfrm>"
               "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>"
               "</p:spPr>"
               "</p:pic>")
        .arg(shapeId)
        .arg(escapeXml(picName), relId)
        .arg(x)
        .arg(y)
        .arg(cx)
        .arg(cy);
}

QString wrapSlideXml(const QString &shapesXml)
{
    return QStringLiteral(
               "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
               "<p:sld xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
               "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
               "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
               "<p:cSld>"
               "<p:bg><p:bgPr><a:solidFill><a:srgbClr val=\"F4F7FA\"/></a:solidFill><a:effectLst/></p:bgPr></p:bg>"
               "<p:spTree>"
               "<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
               "<p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/>"
               "<a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"
               "%1"
               "</p:spTree>"
               "</p:cSld>"
               "<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>"
               "</p:sld>")
        .arg(shapesXml);
}

QStringList fileNamesFromList(const QVariantList &paths)
{
    QStringList names;
    names.reserve(paths.size());
    for (const QVariant &v : paths) {
        const QString p = v.toString().trimmed();
        if (!p.isEmpty()) {
            names.append(QFileInfo(p).fileName());
        }
    }
    return names;
}

} // namespace

StoryBoardExporter::StoryBoardExporter(QObject *parent)
    : QObject(parent)
{
}

QString StoryBoardExporter::normalizeOutputPath(const QString &rawPath, const QString &requiredExt) const
{
    QString clean = rawPath.trimmed();
    if (clean.isEmpty()) {
        return QString();
    }

    QUrl url(clean);
    if (url.isLocalFile()) {
        clean = url.toLocalFile();
    } else if (clean.startsWith(QStringLiteral("file:///"), Qt::CaseInsensitive)) {
        clean = clean.mid(8);
    } else if (clean.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        clean = clean.mid(7);
    }

    clean = QDir::toNativeSeparators(clean);
    const QString dotExt = QStringLiteral(".") + requiredExt;
    if (!clean.endsWith(dotExt, Qt::CaseInsensitive)) {
        clean += dotExt;
    }
    return clean;
}

QString StoryBoardExporter::resolveAssetPath(const QString &rawAssetPath, const QString &episodePath) const
{
    QString clean = rawAssetPath.trimmed();
    if (clean.isEmpty()) {
        return QString();
    }

    QUrl url(clean);
    if (url.isLocalFile()) {
        clean = url.toLocalFile();
    } else if (clean.startsWith(QStringLiteral("file:///"), Qt::CaseInsensitive)) {
        clean = clean.mid(8);
    } else if (clean.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        clean = clean.mid(7);
    }

    QFileInfo info(clean);
    if (info.isAbsolute()) {
        return QDir::toNativeSeparators(info.filePath());
    }

    if (!episodePath.isEmpty()) {
        return QDir::toNativeSeparators(QDir(episodePath).filePath(clean));
    }
    return QDir::toNativeSeparators(clean);
}

QString StoryBoardExporter::readScriptExcerpt(const QString &scriptFilePath, int maxChars) const
{
    if (scriptFilePath.isEmpty() || !QFile::exists(scriptFilePath)) {
        return QString();
    }

    QFile f(scriptFilePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&f);
    QString text = in.readAll().trimmed();
    f.close();

    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    if (text.size() > maxChars) {
        text = text.left(maxChars).trimmed() + QStringLiteral("...");
    }
    return text;
}

QList<StoryBoardExporter::ShotExportData> StoryBoardExporter::extractShots(QObject *board, QString &outEpisodeTitle) const
{
    QList<ShotExportData> result;
    if (!board) {
        return result;
    }

    const QString episodePath = board->property("episodePath").toString().trimmed();
    if (outEpisodeTitle.trimmed().isEmpty()) {
        outEpisodeTitle = !episodePath.isEmpty()
        ? QDir(episodePath).dirName()
        : QStringLiteral("Untitled Episode");
    }

    const QVariantList rawShots = board->property("shots").toList();
    result.reserve(rawShots.size());

    for (int i = 0; i < rawShots.size(); ++i) {
        const QVariantMap m = rawShots.at(i).toMap();
        ShotExportData shot;
        shot.orderNumber = i + 1;
        shot.name = m.value(QStringLiteral("name")).toString().trimmed();
        if (shot.name.isEmpty()) {
            shot.name = tr("Shot %1").arg(shot.orderNumber, 2, 10, QLatin1Char('0'));
        }

        shot.keyImagePath  = resolveAssetPath(m.value(QStringLiteral("key_image")).toString(), episodePath);
        shot.finalShotPath = resolveAssetPath(m.value(QStringLiteral("final_shot")).toString(), episodePath);

        shot.imageFiles  = fileNamesFromList(m.value(QStringLiteral("images")).toList());
        shot.audioFiles  = fileNamesFromList(m.value(QStringLiteral("audio")).toList());
        shot.videoFiles  = fileNamesFromList(m.value(QStringLiteral("videos")).toList());
        shot.scriptFiles = fileNamesFromList(m.value(QStringLiteral("scripts")).toList());

        QString primaryScript = m.value(QStringLiteral("script_file")).toString().trimmed();
        if (primaryScript.isEmpty() && !m.value(QStringLiteral("scripts")).toList().isEmpty()) {
            primaryScript = m.value(QStringLiteral("scripts")).toList().first().toString().trimmed();
        }
        shot.scriptPath = resolveAssetPath(primaryScript, episodePath);
        shot.scriptExcerpt = readScriptExcerpt(shot.scriptPath);

        result.append(shot);
    }

    return result;
}

bool StoryBoardExporter::exportToPdf(const QString &filePath,
                                     QObject *board,
                                     const QString &episodeTitle,
                                     const QString &author)
{
    if (!board) {
        const QString err = tr("Export failed: No storyboard manager provided.");
        emit exportFailed(err);
        return false;
    }

    const QString targetPath = normalizeOutputPath(filePath, QStringLiteral("pdf"));
    if (targetPath.isEmpty()) {
        const QString err = tr("Export failed: Destination PDF path cannot be empty.");
        emit exportFailed(err);
        return false;
    }

    QString resolvedTitle = episodeTitle.trimmed();
    const QList<ShotExportData> shots = extractShots(board, resolvedTitle);
    if (shots.isEmpty()) {
        const QString err = tr("Export failed: Storyboard contains no shots to export.");
        emit exportFailed(err);
        return false;
    }

    QDir().mkpath(QFileInfo(targetPath).absolutePath());

    QPdfWriter writer(targetPath);
    // 16:9 widescreen landscape page (297mm x 167.0625mm)
    writer.setPageSize(QPageSize(QSizeF(297.0, 167.0625), QPageSize::Millimeter));
    writer.setPageMargins(QMarginsF(0, 0, 0, 0));
    writer.setResolution(150);
    writer.setTitle(resolvedTitle + QStringLiteral(" - Storyboard"));
    writer.setCreator(QStringLiteral("Story Builder Studio"));

    QPainter painter(&writer);
    if (!painter.isActive()) {
        const QString err = tr("Export failed: Unable to initialize PDF writer for %1").arg(targetPath);
        emit exportFailed(err);
        return false;
    }

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const int pageW = writer.width();
    const int pageH = writer.height();
    const int totalSlides = shots.size() + 1;
    const QString timestampStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));

    // ========================================================================
    // SLIDE 1: TITLE PAGE
    // ========================================================================
    painter.fillRect(0, 0, pageW, pageH, QColor(QStringLiteral("#08121A")));
    painter.setPen(QPen(QColor(QStringLiteral("#1A4A68")), 4));
    painter.drawRect(36, 36, pageW - 72, pageH - 72);

    painter.fillRect(90, pageH / 2 - 155, 180, 6, QColor(QStringLiteral("#00F0FF")));

    QFont subtitleFont(QStringLiteral("Segoe UI"), 13, QFont::Bold);
    painter.setFont(subtitleFont);
    painter.setPen(QColor(QStringLiteral("#00F0FF")));
    painter.drawText(QRect(90, pageH / 2 - 135, pageW - 180, 40),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     tr("PRODUCTION STORYBOARD"));

    QFont heroTitleFont(QStringLiteral("Segoe UI"), 30, QFont::Bold);
    painter.setFont(heroTitleFont);
    painter.setPen(Qt::white);
    painter.drawText(QRect(90, pageH / 2 - 85, pageW - 180, 110),
                     Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                     resolvedTitle);

    QFont metaFont(QStringLiteral("Segoe UI"), 12);
    painter.setFont(metaFont);
    painter.setPen(QColor(QStringLiteral("#B0C4D4")));
    QStringList metaParts;
    if (!author.trimmed().isEmpty()) {
        metaParts << tr("Author: %1").arg(author.trimmed());
    }
    metaParts << tr("Total Shots: %1").arg(shots.size());
    metaParts << tr("Exported: %1").arg(timestampStr);
    painter.drawText(QRect(90, pageH / 2 + 45, pageW - 180, 45),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     metaParts.join(QStringLiteral("   |   ")));

    QFont footerFont(QStringLiteral("Segoe UI"), 9);
    painter.setFont(footerFont);
    painter.setPen(QColor(QStringLiteral("#7890A4")));
    painter.drawText(QRect(90, pageH - 85, pageW - 180, 30),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("Story Builder Studio"));
    painter.drawText(QRect(90, pageH - 85, pageW - 180, 30),
                     Qt::AlignRight | Qt::AlignVCenter,
                     tr("Slide 1 of %1").arg(totalSlides));

    // ========================================================================
    // SLIDES 2..N+1: SINGLE-SHOT PAGES
    // ========================================================================
    for (int i = 0; i < shots.size(); ++i) {
        writer.newPage();
        const ShotExportData &shot = shots.at(i);

        // Slide background
        painter.fillRect(0, 0, pageW, pageH, QColor(QStringLiteral("#F4F7FA")));

        // Top navy header bar
        const int headerH = 110;
        painter.fillRect(0, 0, pageW, headerH, QColor(QStringLiteral("#08121A")));
        painter.fillRect(0, headerH, pageW, 4, QColor(QStringLiteral("#00B4F0")));

        // Shot number pill
        const QRect badgeRect(55, 28, 145, 54);
        painter.setBrush(QColor(QStringLiteral("#132B3A")));
        painter.setPen(QPen(QColor(QStringLiteral("#00F0FF")), 2));
        painter.drawRoundedRect(badgeRect, 8, 8);

        QFont badgeFont(QStringLiteral("Segoe UI"), 12, QFont::Bold);
        painter.setFont(badgeFont);
        painter.setPen(QColor(QStringLiteral("#00F0FF")));
        painter.drawText(badgeRect,
                         Qt::AlignCenter,
                         tr("SHOT %1").arg(shot.orderNumber, 2, 10, QLatin1Char('0')));

        // Final cut indicator pill (right side of header)
        int titleRightLimit = pageW - 55;
        if (!shot.finalShotPath.isEmpty()) {
            const QRect finalBadgeRect(pageW - 235, 30, 180, 50);
            painter.setBrush(QColor(QStringLiteral("#00A3E0")));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(finalBadgeRect, 8, 8);

            QFont finalFont(QStringLiteral("Segoe UI"), 10, QFont::Bold);
            painter.setFont(finalFont);
            painter.setPen(Qt::white);
            painter.drawText(finalBadgeRect, Qt::AlignCenter, tr("✓ FINAL CUT"));
            titleRightLimit = finalBadgeRect.left() - 20;
        }

        // Shot title
        QFont shotHeaderFont(QStringLiteral("Segoe UI"), 17, QFont::Bold);
        painter.setFont(shotHeaderFont);
        painter.setPen(Qt::white);
        const QRect titleRect(220, 20, titleRightLimit - 220, 70);
        const QString elidedShotTitle = QFontMetrics(shotHeaderFont).elidedText(shot.name, Qt::ElideRight, titleRect.width());
        painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, elidedShotTitle);

        // Left Keyframe Stage
        const QRect imageBox(55, 145, static_cast<int>(pageW * 0.56), pageH - 215);
        painter.setBrush(QColor(QStringLiteral("#0B1B26")));
        painter.setPen(QPen(QColor(QStringLiteral("#1A4A68")), 2));
        painter.drawRect(imageBox);

        QImage keyImg;
        if (!shot.keyImagePath.isEmpty() && QFile::exists(shot.keyImagePath)) {
            keyImg.load(shot.keyImagePath);
        }

        if (!keyImg.isNull()) {
            const QRect innerBox = imageBox.adjusted(8, 8, -8, -8);
            const QSize scaledSize = keyImg.size().scaled(innerBox.size(), Qt::KeepAspectRatio);
            const QRect targetImgRect(
                innerBox.x() + (innerBox.width() - scaledSize.width()) / 2,
                innerBox.y() + (innerBox.height() - scaledSize.height()) / 2,
                scaledSize.width(),
                scaledSize.height());
            painter.drawImage(targetImgRect, keyImg);
        } else {
            QFont emptyImgFont(QStringLiteral("Segoe UI"), 13, QFont::Bold);
            painter.setFont(emptyImgFont);
            painter.setPen(QColor(QStringLiteral("#7890A4")));
            painter.drawText(imageBox, Qt::AlignCenter, tr("No Key Image Assigned"));
        }

        // Right Metadata & Script Panel
        const int panelX = imageBox.right() + 36;
        const int panelW = pageW - panelX - 55;
        const QRect metaPanel(panelX, 145, panelW, pageH - 215);
        painter.setBrush(Qt::white);
        painter.setPen(QPen(QColor(QStringLiteral("#CBD7E3")), 2));
        painter.drawRect(metaPanel);

        int cursorY = metaPanel.top() + 22;
        const int contentX = metaPanel.left() + 24;
        const int contentW = metaPanel.width() - 48;

        QFont sectionFont(QStringLiteral("Segoe UI"), 11, QFont::Bold);
        QFont bodyFont(QStringLiteral("Segoe UI"), 10);

        // Script / Beat Notes Section
        painter.setFont(sectionFont);
        painter.setPen(QColor(QStringLiteral("#08121A")));
        painter.drawText(QRect(contentX, cursorY, contentW, 30), Qt::AlignLeft | Qt::AlignVCenter, tr("SCRIPT / BEAT NOTES"));
        cursorY += 32;

        painter.setFont(bodyFont);
        painter.setPen(QColor(QStringLiteral("#2D3E50")));
        const QString scriptHeader = !shot.scriptPath.isEmpty()
                                         ? tr("File: %1").arg(QFileInfo(shot.scriptPath).fileName())
                                         : tr("File: <None Linked>");
        painter.drawText(QRect(contentX, cursorY, contentW, 26), Qt::AlignLeft | Qt::AlignVCenter, scriptHeader);
        cursorY += 30;

        const QString scriptBody = !shot.scriptExcerpt.isEmpty()
                                       ? shot.scriptExcerpt
                                       : tr("No script text available for this shot.");
        const QRect scriptTextRect(contentX, cursorY, contentW, metaPanel.height() / 2 - 55);
        painter.drawText(scriptTextRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, scriptBody);
        cursorY = scriptTextRect.bottom() + 18;

        // Divider
        painter.setPen(QPen(QColor(QStringLiteral("#D8E2EC")), 2));
        painter.drawLine(contentX, cursorY, contentX + contentW, cursorY);
        cursorY += 16;

        // Linked Assets Section
        painter.setFont(sectionFont);
        painter.setPen(QColor(QStringLiteral("#08121A")));
        painter.drawText(QRect(contentX, cursorY, contentW, 30), Qt::AlignLeft | Qt::AlignVCenter, tr("LINKED MULTIMEDIA ASSETS"));
        cursorY += 34;

        painter.setFont(bodyFont);
        painter.setPen(QColor(QStringLiteral("#2D3E50")));
        QStringList assetLines;
        assetLines << tr("Final Cut: %1").arg(shot.finalShotPath.isEmpty() ? tr("<None>") : QFileInfo(shot.finalShotPath).fileName());
        assetLines << tr("Images (%1): %2").arg(shot.imageFiles.size()).arg(shot.imageFiles.isEmpty() ? tr("None") : shot.imageFiles.join(QStringLiteral(", ")));
        assetLines << tr("Audio (%1): %2").arg(shot.audioFiles.size()).arg(shot.audioFiles.isEmpty() ? tr("None") : shot.audioFiles.join(QStringLiteral(", ")));
        assetLines << tr("Video (%1): %2").arg(shot.videoFiles.size()).arg(shot.videoFiles.isEmpty() ? tr("None") : shot.videoFiles.join(QStringLiteral(", ")));

        const QRect assetsRect(contentX, cursorY, contentW, metaPanel.bottom() - cursorY - 16);
        painter.drawText(assetsRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, assetLines.join(QStringLiteral("\n\n")));

        // Footer
        painter.setFont(footerFont);
        painter.setPen(QColor(QStringLiteral("#5A6E82")));
        painter.drawText(QRect(55, pageH - 52, pageW - 110, 30),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("%1  |  Story Builder Studio").arg(resolvedTitle));
        painter.drawText(QRect(55, pageH - 52, pageW - 110, 30),
                         Qt::AlignRight | Qt::AlignVCenter,
                         tr("Slide %1 of %2").arg(i + 2).arg(totalSlides));
    }

    painter.end();
    emit exportSucceeded(targetPath);
    return true;
}

bool StoryBoardExporter::exportToPptx(const QString &filePath,
                                      QObject *board,
                                      const QString &episodeTitle,
                                      const QString &author)
{
    if (!board) {
        const QString err = tr("Export failed: No storyboard manager provided.");
        emit exportFailed(err);
        return false;
    }

    const QString targetPath = normalizeOutputPath(filePath, QStringLiteral("pptx"));
    if (targetPath.isEmpty()) {
        const QString err = tr("Export failed: Destination PowerPoint path cannot be empty.");
        emit exportFailed(err);
        return false;
    }

    QString resolvedTitle = episodeTitle.trimmed();
    const QList<ShotExportData> shots = extractShots(board, resolvedTitle);
    if (shots.isEmpty()) {
        const QString err = tr("Export failed: Storyboard contains no shots to export.");
        emit exportFailed(err);
        return false;
    }

    QDir().mkpath(QFileInfo(targetPath).absolutePath());

    // 16:9 widescreen slide dimensions in EMUs (13.333 in x 7.5 in)
    constexpr qint64 kSlideW = 12192000;
    constexpr qint64 kSlideH = 6858000;

    const int totalSlides = shots.size() + 1;
    const QString authorClean = author.trimmed().isEmpty() ? QStringLiteral("Story Builder Studio") : author.trimmed();
    const QString isoTimestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    const QString displayTimestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));

    QList<ZipEntry> entries;

    // 1. _rels/.rels
    entries.append({
        QByteArrayLiteral("_rels/.rels"),
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
            "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"ppt/presentation.xml\"/>"
            "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/>"
            "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/>"
            "</Relationships>")
    });

    // 2. docProps/core.xml & docProps/app.xml
    entries.append({
        QByteArrayLiteral("docProps/core.xml"),
        QStringLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<cp:coreProperties xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\" "
            "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
            "xmlns:dcterms=\"http://purl.org/dc/terms/\" "
            "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
            "<dc:title>%1 - Storyboard</dc:title>"
            "<dc:creator>%2</dc:creator>"
            "<cp:lastModifiedBy>%2</cp:lastModifiedBy>"
            "<dcterms:created xsi:type=\"dcterms:W3CDTF\">%3</dcterms:created>"
            "<dcterms:modified xsi:type=\"dcterms:W3CDTF\">%3</dcterms:modified>"
            "</cp:coreProperties>")
            .arg(escapeXml(resolvedTitle), escapeXml(authorClean), isoTimestamp)
            .toUtf8()
    });

    entries.append({
        QByteArrayLiteral("docProps/app.xml"),
        QStringLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\">"
            "<Application>Story Builder Studio</Application>"
            "<Slides>%1</Slides>"
            "<PresentationFormat>Widescreen</PresentationFormat>"
            "</Properties>")
            .arg(totalSlides)
            .toUtf8()
    });

    // 3. ppt/theme/theme1.xml, slideMaster1.xml, slideLayout1.xml, presProps, viewProps, tableStyles
    entries.append({
        QByteArrayLiteral("ppt/theme/theme1.xml"),
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<a:theme xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" name=\"StudioTheme\">"
            "<a:themeElements>"
            "<a:clrScheme name=\"Studio\">"
            "<a:dk1><a:srgbClr val=\"08121A\"/></a:dk1>"
            "<a:lt1><a:srgbClr val=\"FFFFFF\"/></a:lt1>"
            "<a:dk2><a:srgbClr val=\"132B3A\"/></a:dk2>"
            "<a:lt2><a:srgbClr val=\"F4F7FA\"/></a:lt2>"
            "<a:accent1><a:srgbClr val=\"00B4F0\"/></a:accent1>"
            "<a:accent2><a:srgbClr val=\"00F0FF\"/></a:accent2>"
            "<a:accent3><a:srgbClr val=\"1A4A68\"/></a:accent3>"
            "<a:accent4><a:srgbClr val=\"2D3E50\"/></a:accent4>"
            "<a:accent5><a:srgbClr val=\"5A6E82\"/></a:accent5>"
            "<a:accent6><a:srgbClr val=\"0077A8\"/></a:accent6>"
            "<a:hlink><a:srgbClr val=\"00B4F0\"/></a:hlink>"
            "<a:folHlink><a:srgbClr val=\"0077A8\"/></a:folHlink>"
            "</a:clrScheme>"
            "<a:fontScheme name=\"StudioFonts\">"
            "<a:majorFont><a:latin typeface=\"Calibri\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:majorFont>"
            "<a:minorFont><a:latin typeface=\"Calibri\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:minorFont>"
            "</a:fontScheme>"
            "<a:fmtScheme name=\"StudioFmt\">"
            "<a:fillStyleLst><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:fillStyleLst>"
            "<a:lnStyleLst><a:ln w=\"9525\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:ln><a:ln w=\"12700\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:ln><a:ln w=\"19050\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:ln></a:lnStyleLst>"
            "<a:effectStyleLst><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle><a:effectStyle><a:effectLst/></a:effectStyle></a:effectStyleLst>"
            "<a:bgFillStyleLst><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill></a:bgFillStyleLst>"
            "</a:fmtScheme>"
            "</a:themeElements>"
            "</a:theme>")
    });

    entries.append({
        QByteArrayLiteral("ppt/slideMasters/slideMaster1.xml"),
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<p:sldMaster xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
            "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
            "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
            "<p:cSld><p:bg><p:bgRef idx=\"1001\"><a:schemeClr val=\"bg1\"/></p:bgRef></p:bg>"
            "<p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
            "<p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"
            "</p:spTree></p:cSld>"
            "<p:clrMap bg1=\"lt1\" tx1=\"dk1\" bg2=\"lt2\" tx2=\"dk2\" accent1=\"accent1\" accent2=\"accent2\" accent3=\"accent3\" accent4=\"accent4\" accent5=\"accent5\" accent6=\"accent6\" hlink=\"hlink\" folHlink=\"folHlink\"/>"
            "<p:sldLayoutIdLst><p:sldLayoutId id=\"2147483649\" r:id=\"rId1\"/></p:sldLayoutIdLst>"
            "</p:sldMaster>")
    });

    entries.append({
        QByteArrayLiteral("ppt/slideMasters/_rels/slideMaster1.xml.rels"),
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
            "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/>"
            "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\" Target=\"../theme/theme1.xml\"/>"
            "</Relationships>")
    });

    entries.append({
        QByteArrayLiteral("ppt/slideLayouts/slideLayout1.xml"),
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<p:sldLayout xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
            "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
            "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" type=\"blank\" preserve=\"1\">"
            "<p:cSld name=\"Blank\"><p:spTree>"
            "<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
            "<p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/><a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"
            "</p:spTree></p:cSld>"
            "<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>"
            "</p:sldLayout>")
    });

    entries.append({
        QByteArrayLiteral("ppt/slideLayouts/_rels/slideLayout1.xml.rels"),
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
            "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\" Target=\"../slideMasters/slideMaster1.xml\"/>"
            "</Relationships>")
    });

    entries.append({
        QByteArrayLiteral("ppt/presProps.xml"),
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<p:presProps xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"/>")
    });

    entries.append({
        QByteArrayLiteral("ppt/viewProps.xml"),
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<p:viewProps xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\"/>")
    });

    entries.append({
        QByteArrayLiteral("ppt/tableStyles.xml"),
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<a:tblStyleLst xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" def=\"{5C22544A-7EE6-4342-B048-85BDC9FD1C3A}\"/>")
    });

    // 4. Slide 1 (Title Slide)
    {
        QStringList metaParts;
        if (!author.trimmed().isEmpty()) {
            metaParts << tr("Author: %1").arg(author.trimmed());
        }
        metaParts << tr("Total Shots: %1").arg(shots.size());
        metaParts << tr("Exported: %1").arg(displayTimestamp);

        QString shapes;
        shapes += buildShapeXml(2, QStringLiteral("TitleBg"), 0, 0, kSlideW, kSlideH,
                                QStringLiteral("08121A"), QString(), QString());
        shapes += buildShapeXml(3, QStringLiteral("AccentBar"), 762000, 1950000, 1400000, 55000,
                                QStringLiteral("00F0FF"), QString(), QString());
        shapes += buildShapeXml(4, QStringLiteral("Subtitle"), 762000, 2100000, 10668000, 460000,
                                QString(), QString(),
                                buildTextParagraphsXml(tr("PRODUCTION STORYBOARD"), 1600, QStringLiteral("00F0FF"), true));
        shapes += buildShapeXml(5, QStringLiteral("MainTitle"), 762000, 2650000, 10668000, 1300000,
                                QString(), QString(),
                                buildTextParagraphsXml(resolvedTitle, 3800, QStringLiteral("FFFFFF"), true));
        shapes += buildShapeXml(6, QStringLiteral("MetaLine"), 762000, 4100000, 10668000, 520000,
                                QString(), QString(),
                                buildTextParagraphsXml(metaParts.join(QStringLiteral("   |   ")), 1500, QStringLiteral("B0C4D4"), false));
        shapes += buildShapeXml(7, QStringLiteral("FooterLeft"), 762000, 6150000, 7000000, 360000,
                                QString(), QString(),
                                buildTextParagraphsXml(QStringLiteral("Story Builder Studio"), 1100, QStringLiteral("7890A4"), false));

        entries.append({
            QByteArrayLiteral("ppt/slides/slide1.xml"),
            wrapSlideXml(shapes).toUtf8()
        });

        entries.append({
            QByteArrayLiteral("ppt/slides/_rels/slide1.xml.rels"),
            QByteArrayLiteral(
                "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
                "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
                "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/>"
                "</Relationships>")
        });
    }

    // 5. Slides 2..N+1 (One Shot Per Slide)
    for (int i = 0; i < shots.size(); ++i) {
        const ShotExportData &shot = shots.at(i);
        const int slideNum = i + 2;

        // Encode keyframe image to PNG bytes if present so any Qt image format embeds cleanly
        QByteArray embeddedPngBytes;
        QSize embeddedImgSize;
        if (!shot.keyImagePath.isEmpty() && QFile::exists(shot.keyImagePath)) {
            QImage keyImg(shot.keyImagePath);
            if (!keyImg.isNull()) {
                QBuffer pngBuf(&embeddedPngBytes);
                if (pngBuf.open(QIODevice::WriteOnly) && keyImg.save(&pngBuf, "PNG")) {
                    embeddedImgSize = keyImg.size();
                } else {
                    embeddedPngBytes.clear();
                }
            }
        }

        const bool hasEmbeddedImage = !embeddedPngBytes.isEmpty() && embeddedImgSize.isValid();
        const QString mediaFileName = QStringLiteral("image%1.png").arg(slideNum);

        if (hasEmbeddedImage) {
            entries.append({
                QStringLiteral("ppt/media/%1").arg(mediaFileName).toUtf8(),
                embeddedPngBytes
            });
        }

        QString shapes;
        int nextShapeId = 2;

        // Top navy header + cyan accent strip
        shapes += buildShapeXml(nextShapeId++, QStringLiteral("HeaderBar"), 0, 0, kSlideW, 760000,
                                QStringLiteral("08121A"), QString(), QString());
        shapes += buildShapeXml(nextShapeId++, QStringLiteral("HeaderAccent"), 0, 760000, kSlideW, 30000,
                                QStringLiteral("00B4F0"), QString(), QString());

        // Shot number badge
        shapes += buildShapeXml(nextShapeId++, QStringLiteral("ShotBadge"), 420000, 175000, 1180000, 410000,
                                QStringLiteral("132B3A"), QStringLiteral("00F0FF"),
                                buildTextParagraphsXml(
                                    tr("SHOT %1").arg(shot.orderNumber, 2, 10, QLatin1Char('0')),
                                    1300, QStringLiteral("00F0FF"), true),
                                QStringLiteral("ctr"));

        // Optional Final Cut badge
        qint64 titleWidth = 9950000;
        if (!shot.finalShotPath.isEmpty()) {
            shapes += buildShapeXml(nextShapeId++, QStringLiteral("FinalCutBadge"), 10320000, 185000, 1450000, 390000,
                                    QStringLiteral("00A3E0"), QString(),
                                    buildTextParagraphsXml(tr("FINAL CUT"), 1150, QStringLiteral("FFFFFF"), true),
                                    QStringLiteral("ctr"));
            titleWidth = 8420000;
        }

        // Shot Title
        shapes += buildShapeXml(nextShapeId++, QStringLiteral("ShotTitle"), 1720000, 155000, titleWidth, 450000,
                                QString(), QString(),
                                buildTextParagraphsXml(shot.name, 1900, QStringLiteral("FFFFFF"), true),
                                QStringLiteral("ctr"));

        // Left Keyframe Stage Frame
        constexpr qint64 kBoxX = 420000;
        constexpr qint64 kBoxY = 980000;
        constexpr qint64 kBoxW = 6840000;
        constexpr qint64 kBoxH = 5140000;

        shapes += buildShapeXml(nextShapeId++, QStringLiteral("KeyframeBackdrop"),
                                kBoxX, kBoxY, kBoxW, kBoxH,
                                QStringLiteral("0B1B26"), QStringLiteral("1A4A68"),
                                hasEmbeddedImage
                                    ? QString()
                                    : buildTextParagraphsXml(tr("No Key Image Assigned"), 1400, QStringLiteral("7890A4"), true),
                                QStringLiteral("ctr"));

        if (hasEmbeddedImage) {
            constexpr qint64 kPad = 50000;
            const qint64 innerW = kBoxW - (kPad * 2);
            const qint64 innerH = kBoxH - (kPad * 2);
            const double scale = qMin(static_cast<double>(innerW) / embeddedImgSize.width(),
                                      static_cast<double>(innerH) / embeddedImgSize.height());
            const qint64 drawW = qMax<qint64>(1, qRound64(embeddedImgSize.width() * scale));
            const qint64 drawH = qMax<qint64>(1, qRound64(embeddedImgSize.height() * scale));
            const qint64 drawX = kBoxX + kPad + (innerW - drawW) / 2;
            const qint64 drawY = kBoxY + kPad + (innerH - drawH) / 2;

            shapes += buildPictureXml(nextShapeId++,
                                      QStringLiteral("Keyframe_%1").arg(slideNum),
                                      QStringLiteral("rId2"),
                                      drawX, drawY, drawW, drawH);
        }

        // Right Panel: Script Notes + Linked Assets
        constexpr qint64 kPanelX = 7520000;
        constexpr qint64 kPanelW = 4250000;

        const QString scriptFileName = !shot.scriptPath.isEmpty()
                                           ? QFileInfo(shot.scriptPath).fileName()
                                           : tr("<None Linked>");
        const QString scriptExcerpt = !shot.scriptExcerpt.isEmpty()
                                          ? shot.scriptExcerpt
                                          : tr("No script text available for this shot.");

        QString scriptBoxParagraphs;
        scriptBoxParagraphs += buildTextParagraphsXml(tr("SCRIPT / BEAT NOTES"), 1200, QStringLiteral("08121A"), true);
        scriptBoxParagraphs += buildTextParagraphsXml(tr("File: %1").arg(scriptFileName), 1050, QStringLiteral("0077A8"), true);
        scriptBoxParagraphs += buildTextParagraphsXml(scriptExcerpt, 1050, QStringLiteral("2D3E50"), false);

        shapes += buildShapeXml(nextShapeId++, QStringLiteral("ScriptPanel"),
                                kPanelX, kBoxY, kPanelW, 2820000,
                                QStringLiteral("FFFFFF"), QStringLiteral("CBD7E3"),
                                scriptBoxParagraphs, QStringLiteral("t"));

        QString assetsBoxParagraphs;
        assetsBoxParagraphs += buildTextParagraphsXml(tr("LINKED MULTIMEDIA ASSETS"), 1200, QStringLiteral("08121A"), true);
        assetsBoxParagraphs += buildTextParagraphsXml(
            tr("Final Cut: %1").arg(shot.finalShotPath.isEmpty() ? tr("<None>") : QFileInfo(shot.finalShotPath).fileName()),
            1050, QStringLiteral("2D3E50"), false);
        assetsBoxParagraphs += buildTextParagraphsXml(
            tr("Images (%1): %2").arg(shot.imageFiles.size()).arg(shot.imageFiles.isEmpty() ? tr("None") : shot.imageFiles.join(QStringLiteral(", "))),
            1050, QStringLiteral("2D3E50"), false);
        assetsBoxParagraphs += buildTextParagraphsXml(
            tr("Audio (%1): %2").arg(shot.audioFiles.size()).arg(shot.audioFiles.isEmpty() ? tr("None") : shot.audioFiles.join(QStringLiteral(", "))),
            1050, QStringLiteral("2D3E50"), false);
        assetsBoxParagraphs += buildTextParagraphsXml(
            tr("Video (%1): %2").arg(shot.videoFiles.size()).arg(shot.videoFiles.isEmpty() ? tr("None") : shot.videoFiles.join(QStringLiteral(", "))),
            1050, QStringLiteral("2D3E50"), false);

        shapes += buildShapeXml(nextShapeId++, QStringLiteral("AssetsPanel"),
                                kPanelX, 3960000, kPanelW, 2160000,
                                QStringLiteral("FFFFFF"), QStringLiteral("CBD7E3"),
                                assetsBoxParagraphs, QStringLiteral("t"));

        // Footer
        shapes += buildShapeXml(nextShapeId++, QStringLiteral("SlideFooter"),
                                420000, 6280000, 11350000, 360000,
                                QString(), QString(),
                                buildTextParagraphsXml(
                                    tr("%1  |  Slide %2 of %3  |  Story Builder Studio")
                                        .arg(resolvedTitle)
                                        .arg(slideNum)
                                        .arg(totalSlides),
                                    1000, QStringLiteral("5A6E82"), false));

        entries.append({
            QStringLiteral("ppt/slides/slide%1.xml").arg(slideNum).toUtf8(),
            wrapSlideXml(shapes).toUtf8()
        });

        QString slideRels = QStringLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
            "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/>");
        if (hasEmbeddedImage) {
            slideRels += QStringLiteral(
                             "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" Target=\"../media/%1\"/>")
                             .arg(mediaFileName);
        }
        slideRels += QStringLiteral("</Relationships>");

        entries.append({
            QStringLiteral("ppt/slides/_rels/slide%1.xml.rels").arg(slideNum).toUtf8(),
            slideRels.toUtf8()
        });
    }

    // 6. ppt/presentation.xml & ppt/_rels/presentation.xml.rels
    QString sldIdLstXml;
    QString presRelsXml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\" Target=\"slideMasters/slideMaster1.xml\"/>");

    for (int s = 1; s <= totalSlides; ++s) {
        const int relNum = s + 1;
        const int sldId = 255 + s;
        sldIdLstXml += QStringLiteral("<p:sldId id=\"%1\" r:id=\"rId%2\"/>").arg(sldId).arg(relNum);
        presRelsXml += QStringLiteral(
                           "<Relationship Id=\"rId%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" Target=\"slides/slide%2.xml\"/>")
                           .arg(relNum)
                           .arg(s);
    }

    const int nextPresRel = totalSlides + 2;
    presRelsXml += QStringLiteral(
                       "<Relationship Id=\"rId%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/presProps\" Target=\"presProps.xml\"/>"
                       "<Relationship Id=\"rId%2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/viewProps\" Target=\"viewProps.xml\"/>"
                       "<Relationship Id=\"rId%3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/tableStyles\" Target=\"tableStyles.xml\"/>"
                       "<Relationship Id=\"rId%4\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\" Target=\"theme/theme1.xml\"/>"
                       "</Relationships>")
                       .arg(nextPresRel)
                       .arg(nextPresRel + 1)
                       .arg(nextPresRel + 2)
                       .arg(nextPresRel + 3);

    entries.append({
        QByteArrayLiteral("ppt/_rels/presentation.xml.rels"),
        presRelsXml.toUtf8()
    });

    entries.append({
        QByteArrayLiteral("ppt/presentation.xml"),
        QStringLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            "<p:presentation xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
            "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\" "
            "xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
            "<p:sldMasterIdLst><p:sldMasterId id=\"2147483648\" r:id=\"rId1\"/></p:sldMasterIdLst>"
            "<p:sldIdLst>%1</p:sldIdLst>"
            "<p:sldSz cx=\"%2\" cy=\"%3\"/>"
            "<p:notesSz cx=\"6858000\" cy=\"9144000\"/>"
            "</p:presentation>")
            .arg(sldIdLstXml)
            .arg(kSlideW)
            .arg(kSlideH)
            .toUtf8()
    });

    // 7. [Content_Types].xml
    QString contentTypesXml = QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Default Extension=\"png\" ContentType=\"image/png\"/>"
        "<Default Extension=\"jpeg\" ContentType=\"image/jpeg\"/>"
        "<Default Extension=\"jpg\" ContentType=\"image/jpeg\"/>"
        "<Override PartName=\"/ppt/presentation.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml\"/>"
        "<Override PartName=\"/ppt/slideMasters/slideMaster1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml\"/>"
        "<Override PartName=\"/ppt/slideLayouts/slideLayout1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml\"/>"
        "<Override PartName=\"/ppt/theme/theme1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/>"
        "<Override PartName=\"/ppt/presProps.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presProps+xml\"/>"
        "<Override PartName=\"/ppt/viewProps.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.viewProps+xml\"/>"
        "<Override PartName=\"/ppt/tableStyles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.tableStyles+xml\"/>"
        "<Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>"
        "<Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>");

    for (int s = 1; s <= totalSlides; ++s) {
        contentTypesXml += QStringLiteral(
                               "<Override PartName=\"/ppt/slides/slide%1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>")
                               .arg(s);
    }
    contentTypesXml += QStringLiteral("</Types>");

    entries.append({
        QByteArrayLiteral("[Content_Types].xml"),
        contentTypesXml.toUtf8()
    });

    QString zipError;
    if (!writeZipArchive(targetPath, entries, zipError)) {
        const QString err = tr("Export failed: Unable to write PowerPoint file (%1): %2").arg(targetPath, zipError);
        emit exportFailed(err);
        return false;
    }

    emit exportSucceeded(targetPath);
    return true;
}
