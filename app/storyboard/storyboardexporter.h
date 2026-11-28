// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/QQmlEngine>

/**
 * @class StoryBoardExporter
 * @brief Exports ordered storyboard sequences into 16:9 single-shot PDF and PowerPoint (.pptx) decks.
 *
 * Reads shot metadata from StoryBoardManager (or any compatible QObject exposing `shots`
 * and `episodePath`) without mutating storyboard state.
 */
class StoryBoardExporter : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit StoryBoardExporter(QObject *parent = nullptr);
    ~StoryBoardExporter() override = default;

    Q_INVOKABLE bool exportToPdf(const QString &filePath,
                                 QObject *board,
                                 const QString &episodeTitle = QString(),
                                 const QString &author = QString());

    Q_INVOKABLE bool exportToPptx(const QString &filePath,
                                  QObject *board,
                                  const QString &episodeTitle = QString(),
                                  const QString &author = QString());

signals:
    void exportSucceeded(const QString &filePath);
    void exportFailed(const QString &errorMessage);

private:
    struct ShotExportData {
        int orderNumber = 1;
        QString name;
        QString keyImagePath;
        QString finalShotPath;
        QString scriptPath;
        QString scriptExcerpt;
        QStringList imageFiles;
        QStringList audioFiles;
        QStringList videoFiles;
        QStringList scriptFiles;
    };

    QString normalizeOutputPath(const QString &rawPath, const QString &requiredExt) const;
    QString resolveAssetPath(const QString &rawAssetPath, const QString &episodePath) const;
    QString readScriptExcerpt(const QString &scriptFilePath, int maxChars = 900) const;
    QList<ShotExportData> extractShots(QObject *board, QString &outEpisodeTitle) const;
};
