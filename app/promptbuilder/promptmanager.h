/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: promptmanager.h
** Handles writing, displaying, using custom prompts.
**
****************************************************************************/
#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/QQmlEngine>

struct PromptItem {
    QString fileName;
    QString scope;       // "System", "Series", "Episode"
    QString fullPath;
    QString category;    // "text", "audio", "image", "video"
};

/**
 * @class  PromptManager
 * @brief  Handles writing, displaying, using custom prompts.
 */
class PromptManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString activeCategory READ activeCategory WRITE setActiveCategory NOTIFY activeCategoryChanged)
    Q_PROPERTY(QVariantList availablePrompts READ availablePrompts NOTIFY availablePromptsChanged)

public:
    explicit PromptManager(QObject *parent = nullptr);
    ~PromptManager() override = default;

    QString activeCategory() const { return m_activeCategory; }
    void setActiveCategory(const QString &category);

    QVariantList availablePrompts() const { return m_availablePrompts; }

    Q_INVOKABLE void setPaths(const QString &seriesPath, const QString &episodePath);
    Q_INVOKABLE void refreshPrompts();
    Q_INVOKABLE QString readPromptFile(const QString &fullPath);
    Q_INVOKABLE bool savePrompt(const QString &fileName,
                                const QString &content,
                                const QString &category,
                                bool isSeriesScope);
    Q_INVOKABLE QStringList getFilesForCategory(const QString &category);

signals:
    void activeCategoryChanged();
    void availablePromptsChanged();
    void promptSaved(const QString &fullPath);
    void promptError(const QString &error);

private:
    QString m_activeCategory{QStringLiteral("text")};
    QString m_systemBaseDir;
    QString m_seriesBaseDir;
    QString m_episodeBaseDir;
    QVariantList m_availablePrompts;

    void ensureDirectoryStructure(const QString &root);
    void scanCategoryDirectory(const QString &dirPath, const QString &scope, const QString &category, QList<PromptItem> &outList);
};

