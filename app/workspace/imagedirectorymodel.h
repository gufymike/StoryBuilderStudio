// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include "abstractassetdirectorymodel.h"
#include <QtQml/QQmlEngine>

struct ImageExtraMetadata {
    QString resolution;
    QString format;
    QString colorSpace;
};
/**
 * @class   ImageDirectoryModel
 * @brief   Provides directory inspection, dimensions, and metadata for project image assets.
 */
class ImageDirectoryModel : public AbstractAssetDirectoryModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum ImageRoles {
        ResolutionRole = UserCustomRoleBase,
        FormatRole,
        ColorSpaceRole
    };
    Q_ENUM(ImageRoles)

    explicit ImageDirectoryModel(QObject *parent = nullptr);
    ~ImageDirectoryModel() override = default;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setProjectPath(const QString &projectPath);
    void refresh();

protected:
    QString assetSubfolderName() const override;
    QStringList fileExtensionFilters() const override;
    void populateCustomMetadata(int index, const QFileInfo &info) override;

private:
    QList<ImageExtraMetadata> m_imageExtras;
};
