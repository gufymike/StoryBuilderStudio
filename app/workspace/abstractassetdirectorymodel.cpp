/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: abstractassetdirectorymodel.cpp
** Abstract base QAbstractListModel managing project workspace asset directories.
**
****************************************************************************/
#include "abstractassetdirectorymodel.h"
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QFile>
#include <QDateTime>
#include <QRegularExpression>

AbstractAssetDirectoryModel::AbstractAssetDirectoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int AbstractAssetDirectoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_baseAssets.size();
}

QVariant AbstractAssetDirectoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_baseAssets.size()) return QVariant();

    const auto &item = m_baseAssets.at(index.row());
    switch (role) {
    case FileNameRole:     return item.fileName;
    case FilePathRole:     return item.filePath;
    case FileSizeRole:     return item.fileSizeFormatted;
    case CreatedDateRole:  return item.createdDateFormatted;
    case ModifiedDateRole: return item.modifiedDateFormatted;
    default:               return QVariant();
    }
}

QHash<int, QByteArray> AbstractAssetDirectoryModel::roleNames() const
{
    return {
        {FileNameRole,     "fileName"},
        {FilePathRole,     "filePath"},
        {FileSizeRole,     "fileSize"},
        {CreatedDateRole,  "createdDate"},
        {ModifiedDateRole, "modifiedDate"}
    };
}

void AbstractAssetDirectoryModel::setProjectPath(const QString &projectPath)
{
    if (m_projectPath == projectPath) return;
    beginResetModel();
    m_baseAssets.clear();
    m_projectPath = projectPath;
    scanDirectoryInternal();
    endResetModel();
    emit projectPathChanged();
    emit countChanged();
}

void AbstractAssetDirectoryModel::refresh()
{
    beginResetModel();
    m_baseAssets.clear();
    scanDirectoryInternal();
    endResetModel();
    emit countChanged();
}

void AbstractAssetDirectoryModel::scanDirectoryInternal()
{
    if (m_projectPath.isEmpty()) return;

    QDir targetDir(m_projectPath + QStringLiteral("/assets/") + assetSubfolderName());
    qDebug() << "[AbstractAssetDirectoryModel] scanDirectoryInternal targetDir: " << targetDir.absolutePath();
    if (!targetDir.exists()) {
        targetDir.mkpath(QStringLiteral("."));
        return;
    }

    QFileInfoList fileList = targetDir.entryInfoList(fileExtensionFilters(), QDir::Files | QDir::NoDotAndDotDot);
    qDebug() << "[AbstractAssetDirectoryModel] Found " << fileList.size() << " files in " << targetDir.absolutePath();
    for (int i = 0; i < fileList.size(); ++i) {
        const QFileInfo &info = fileList.at(i);
        BaseAssetItem item;
        item.fileName = info.fileName();
        item.filePath = QUrl::fromLocalFile(info.filePath()).toString();

        qint64 bytes = info.size();
        if (bytes < 1024 * 1024)
            item.fileSizeFormatted = QString::number(bytes / 1024.0, 'f', 1) + QStringLiteral(" KB");
        else
            item.fileSizeFormatted = QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + QStringLiteral(" MB");

        item.createdDateFormatted = info.birthTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        item.modifiedDateFormatted = info.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

        m_baseAssets.append(item);
        populateCustomMetadata(i, info);
    }
}

int AbstractAssetDirectoryModel::importAssets(const QList<QUrl> &fileUrls)
{
    if (m_projectPath.isEmpty() || fileUrls.isEmpty()) return 0;

    QString targetDirPath = m_projectPath + QStringLiteral("/assets/") + assetSubfolderName();
    QDir targetDir(targetDirPath);
    if (!targetDir.exists()) {
        targetDir.mkpath(QStringLiteral("."));
    }

    int copied = 0;
    int skipped = 0;

    for (const QUrl &url : fileUrls) {
        QString srcPath = url.isLocalFile() ? url.toLocalFile() : url.toString();
        srcPath = QDir::toNativeSeparators(srcPath.replace(QRegularExpression(R"(/^file:\/\//i)"), QString()));
#ifdef Q_OS_WIN
        if (srcPath.startsWith(QLatin1Char('\\')) && srcPath.length() > 2 && srcPath[2] == QLatin1Char(':')) {
            srcPath.remove(0, 1);
        }
#endif

        QFileInfo srcInfo(srcPath);
        if (!srcInfo.exists() || !srcInfo.isFile()) {
            skipped++;
            continue;
        }

        QString destFileName = srcInfo.fileName();
        QString destFilePath = targetDir.filePath(destFileName);

        if (QFile::exists(destFilePath)) {
            QString base = srcInfo.completeBaseName();
            QString ext = srcInfo.suffix().isEmpty() ? QString() : (QStringLiteral(".") + srcInfo.suffix());
            int counter = 1;
            do {
                destFileName = QStringLiteral("%1_copy%2%3").arg(base).arg(counter == 1 ? QString() : QString::number(counter)).arg(ext);
                destFilePath = targetDir.filePath(destFileName);
                counter++;
            } while (QFile::exists(destFilePath));
        }

        if (QFile::copy(srcPath, destFilePath)) {
            copied++;
        } else {
            skipped++;
        }
    }

    if (copied > 0) {
        refresh();
    }

    emit assetsImported(copied, skipped);
    return copied;
}

