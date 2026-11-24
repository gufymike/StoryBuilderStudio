// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include "abstractassetdirectorymodel.h"
#include <QtQml/QQmlEngine>
/**
 * @class   VideoDirectoryModel
 * @brief   Provides directory inspection and asset ingestion for project video files.
 */
class VideoDirectoryModel : public AbstractAssetDirectoryModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit VideoDirectoryModel(QObject *parent = nullptr)
        : AbstractAssetDirectoryModel(parent) {}

protected:
    QString assetSubfolderName() const override { return QStringLiteral("video"); }
    QStringList fileExtensionFilters() const override {
        return {
            QStringLiteral("*.mp4"),
            QStringLiteral("*.mkv"),
            QStringLiteral("*.webm"),
            QStringLiteral("*.mov"),
            QStringLiteral("*.avi")
        };
    }
};

