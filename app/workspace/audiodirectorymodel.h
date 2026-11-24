// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include "abstractassetdirectorymodel.h"
#include <QtQml/QQmlEngine>
/**
 * @class   AudioDirectoryModel
 * @brief   Provides directory inspection and asset ingestion for project audio files.
 */
class AudioDirectoryModel : public AbstractAssetDirectoryModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit AudioDirectoryModel(QObject *parent = nullptr)
        : AbstractAssetDirectoryModel(parent) {}

protected:
    QString assetSubfolderName() const override { return QStringLiteral("audio"); }
    QStringList fileExtensionFilters() const override {
        return {
            QStringLiteral("*.wav"),
            QStringLiteral("*.mp3"),
            QStringLiteral("*.ogg"),
            QStringLiteral("*.flac")
        };
    }
};
