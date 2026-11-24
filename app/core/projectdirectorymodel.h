// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QtQml/QQmlEngine>

class ProjectDirectoryModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    /**
    * @property projectPath
    * @brief The absolute path to the active project root.
    * @notifier projectPathChanged
    */
    Q_PROPERTY(QString projectPath READ projectPath WRITE setProjectPath NOTIFY projectPathChanged)

    /**
     * @property categories
     * @brief A list of all sub-directories discovered within the project's prompts folder.
     * @notifier categoriesChanged
     */
    Q_PROPERTY(QStringList categories READ categories NOTIFY categoriesChanged)

public:
    explicit ProjectDirectoryModel(QObject *parent = nullptr);
    ~ProjectDirectoryModel() override = default;

    QString projectPath() const { return m_projectPath; }
    Q_INVOKABLE void setProjectPath(const QString &projectPath);

    Q_INVOKABLE QStringList getFilesForCategory(const QString &categoryName) const;
    Q_INVOKABLE QString loadPromptFileContent(const QString &categoryName, const QString &fileName) const;

    QStringList categories() const { return m_categories; }

signals:
    void projectPathChanged();
    void categoriesChanged();

private:
    QString m_projectPath;
    QStringList m_categories;
    QMap<QString, QStringList> m_categoryFilesMap;

    void scanPromptsDirectory();
};

