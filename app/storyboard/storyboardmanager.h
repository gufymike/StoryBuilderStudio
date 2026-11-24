// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QJsonObject>
#include <QJsonArray>
#include <QtQml/QQmlEngine>

/**
 * @class StoryBoardManager
 * @brief Orchestrates shot metadata, ordering, and asset bindings in storyboard.json.
 *
 * Exposes a QML-friendly interface for dynamically managing a storyboard timeline.
 * Handles the creation and reordering of shots, assigning primary multimedia assets
 * (key image, final video, primary script), and persisting the entire board structure
 * cleanly to a JSON manifest.
 */
class StoryBoardManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString episodePath READ episodePath WRITE setEpisodePath NOTIFY episodePathChanged)
    Q_PROPERTY(QVariantList shots READ shots NOTIFY shotsChanged)
    Q_PROPERTY(int shotCount READ shotCount NOTIFY shotsChanged)
    Q_PROPERTY(bool hasValidPath READ hasValidPath NOTIFY hasValidPathChanged)

public:
    explicit StoryBoardManager(QObject *parent = nullptr);
    ~StoryBoardManager() override = default;

    QString episodePath() const { return m_episodePath; }
    void setEpisodePath(const QString &path);

    QVariantList shots() const { return m_shots; }
    int shotCount() const { return m_shots.size(); }
    bool hasValidPath() const { return !m_episodePath.isEmpty(); }

    Q_INVOKABLE void load();
    Q_INVOKABLE void save();

    // Reorder & Shot Operations
    Q_INVOKABLE bool addShot(const QString &name);
    Q_INVOKABLE bool removeShot(int index);
    Q_INVOKABLE bool moveShot(int fromIndex, int toIndex);
    Q_INVOKABLE bool moveShotUp(int index);
    Q_INVOKABLE bool moveShotDown(int index);

    // Targeted Single-Field Setters
    Q_INVOKABLE bool renameShot(int index, const QString &newName);
    Q_INVOKABLE bool setShotKeyImage(int index, const QString &filePath);
    Q_INVOKABLE bool setShotFinalCut(int index, const QString &filePath);
    Q_INVOKABLE bool setShotScript(int index, const QString &filePath);

    // Asset Attachment Management
    Q_INVOKABLE bool addAssetToShot(int index, const QString &category, const QString &filePath);
    Q_INVOKABLE bool removeAssetFromShot(int index, const QString &category, const QString &filePath);

    // Auto-prune dead assets when deleted from project assets
    Q_INVOKABLE void cleanupDeletedAsset(const QString &filePath);

signals:
    void episodePathChanged();
    void shotsChanged();
    void hasValidPathChanged();
    void boardError(const QString &errorMessage);
    void boardSaved();

private:
    QString m_episodePath;
    QVariantList m_shots;

    QString getJsonFilePath() const;
    void normalizeOrders();
    bool updateShotField(int index, const QString &key, const QVariant &value);
};

