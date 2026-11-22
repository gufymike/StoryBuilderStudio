/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: imagedirectorymodel.cpp
** Provides directory inspection, dimensions, and metadata for project image assets.
**
****************************************************************************/
#include "imagedirectorymodel.h"
#include <QImageReader>
#include <QSize>

ImageDirectoryModel::ImageDirectoryModel(QObject *parent)
    : AbstractAssetDirectoryModel(parent)
{
}

QString ImageDirectoryModel::assetSubfolderName() const
{
    return QStringLiteral("images");
}

QStringList ImageDirectoryModel::fileExtensionFilters() const
{
    return {
        QStringLiteral("*.png"),
        QStringLiteral("*.jpg"),
        QStringLiteral("*.jpeg"),
        QStringLiteral("*.webp"),
        QStringLiteral("*.bmp")
    };
}

QVariant ImageDirectoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_imageExtras.size()) {
        return QVariant();
    }

    const auto &extra = m_imageExtras.at(index.row());
    switch (role) {
    case ResolutionRole: return extra.resolution;
    case FormatRole:     return extra.format;
    case ColorSpaceRole: return extra.colorSpace;
    default:             return AbstractAssetDirectoryModel::data(index, role);
    }
}

QHash<int, QByteArray> ImageDirectoryModel::roleNames() const
{
    auto roles = AbstractAssetDirectoryModel::roleNames();
    roles[ResolutionRole] = "resolution";
    roles[FormatRole]     = "format";
    roles[ColorSpaceRole] = "colorSpace";
    return roles;
}

void ImageDirectoryModel::populateCustomMetadata(int index, const QFileInfo &info)
{
    Q_UNUSED(index);
    ImageExtraMetadata meta;
    QImageReader reader(info.absoluteFilePath());

    if (reader.canRead()) {
        QSize sz = reader.size();
        meta.resolution = QStringLiteral("%1 x %2").arg(sz.width()).arg(sz.height());
        meta.format = QString::fromLatin1(reader.format()).toUpper();
        meta.colorSpace = QStringLiteral("sRGB");
    } else {
        meta.resolution = QStringLiteral("Unknown");
        meta.format = info.suffix().toUpper();
        meta.colorSpace = QStringLiteral("N/A");
    }

    m_imageExtras.append(meta);
}

void ImageDirectoryModel::setProjectPath(const QString &projectPath)
{
    m_imageExtras.clear();
    AbstractAssetDirectoryModel::setProjectPath(projectPath);
}

void ImageDirectoryModel::refresh()
{
    m_imageExtras.clear();
    AbstractAssetDirectoryModel::refresh();
}

