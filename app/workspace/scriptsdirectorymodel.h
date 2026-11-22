/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: scriptsdirectorymodel.h
** Provides directory inspection and asset ingestion for project script files.
**
****************************************************************************/
#pragma once

#include "abstractassetdirectorymodel.h"
#include <QtQml/QQmlEngine>

class ScriptsDirectoryModel : public AbstractAssetDirectoryModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    ScriptsDirectoryModel(QObject *parent = nullptr)
        : AbstractAssetDirectoryModel(parent) {}

protected:
    QString assetSubfolderName() const override { return QStringLiteral("scripts"); }
    QStringList fileExtensionFilters() const override {
        return {
            QStringLiteral("*.md"),
            QStringLiteral("*.txt"),
            QStringLiteral("*.prompt")
        };
    }
};

