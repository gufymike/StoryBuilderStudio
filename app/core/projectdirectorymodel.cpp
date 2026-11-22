/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: projectdirectorymodel.cpp
** QML Model for inspecting and loading categorized prompt templates.
**
****************************************************************************/
#include "projectdirectorymodel.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

ProjectDirectoryModel::ProjectDirectoryModel(QObject *parent)
    : QObject(parent)
{
}

void ProjectDirectoryModel::setProjectPath(const QString &projectPath)
{
    if (m_projectPath == projectPath) {
        return;
    }

    m_projectPath = projectPath;
    emit projectPathChanged();
    scanPromptsDirectory();
}

void ProjectDirectoryModel::scanPromptsDirectory()
{
    m_categories.clear();
    m_categoryFilesMap.clear();

    if (m_projectPath.isEmpty()) {
        emit categoriesChanged();
        return;
    }

    QDir promptsDir(m_projectPath + QStringLiteral("/prompts"));
    if (!promptsDir.exists()) {
        emit categoriesChanged();
        return;
    }

    const QStringList subDirs = promptsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for (const QString &subDirName : subDirs) {
        QDir subDir(promptsDir.absoluteFilePath(subDirName));
        const QStringList files = subDir.entryList(QStringList() << QStringLiteral("*.prompt") << QStringLiteral("*.txt"), QDir::Files, QDir::Name);

        QStringList cleanedFileNames;
        cleanedFileNames.reserve(files.size());
        for (const QString &fileName : files) {
            cleanedFileNames.append(QFileInfo(fileName).completeBaseName());
        }

        m_categories.append(subDirName);
        m_categoryFilesMap.insert(subDirName, cleanedFileNames);
    }

    emit categoriesChanged();
}

QStringList ProjectDirectoryModel::getFilesForCategory(const QString &categoryName) const
{
    return m_categoryFilesMap.value(categoryName, QStringList());
}

QString ProjectDirectoryModel::loadPromptFileContent(const QString &categoryName, const QString &fileName) const
{
    if (m_projectPath.isEmpty() || categoryName.isEmpty() || fileName.isEmpty()) {
        return QString();
    }

    QDir subDir(m_projectPath + QStringLiteral("/prompts/") + categoryName);
    QString filePath = subDir.absoluteFilePath(fileName + QStringLiteral(".prompt"));

    if (!QFile::exists(filePath)) {
        filePath = subDir.absoluteFilePath(fileName + QStringLiteral(".txt"));
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&file);
    return in.readAll();
}

