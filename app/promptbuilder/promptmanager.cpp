// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include "promptmanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDebug>

PromptManager::PromptManager(QObject *parent)
    : QObject(parent)
{
    // move to test_promptManager.cpp Q_INIT_RESOURCE(default_prompts);

    QString appData = QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    m_systemBaseDir = QStringLiteral(":/prompts");
    qInfo() << "[PromptManager] System Prompts Base Directory:" << m_systemBaseDir;

    ensureDirectoryStructure(m_systemBaseDir);
    refreshPrompts();
}

void PromptManager::scanCategoryDirectory(const QString &dirPath, const QString &scope, const QString &category, QList<PromptItem> &outList)
{
    if (dirPath.isEmpty()) return;

    QDir dir(dirPath + QStringLiteral("/") + category);
    if (!dir.exists()) {
        // let it continue if it starts with :/prompts
        if(!dir.absolutePath().startsWith(":/prompts")) {
            return;
        }
    }

    QStringList nameFilters;
    nameFilters << QStringLiteral("*.prompt") << QStringLiteral("*.txt");

    QFileInfoList files = dir.entryInfoList(nameFilters, QDir::Files | QDir::Readable, QDir::Name);
    qInfo() << "[PromptManager] Found " << files.length() << " files in directory " << dir.absolutePath();

    for (const QFileInfo &fi : files) {
        PromptItem item;
        item.fileName = fi.fileName();
        item.scope = scope;
        // Preserve forward slashes for QRC (":/...") paths
        if (dirPath.startsWith(QStringLiteral(":"))) {
            item.fullPath = fi.absoluteFilePath();
        } else {
            item.fullPath = QDir::toNativeSeparators(fi.absoluteFilePath());
        }
        item.category = category;
        outList.append(item);
    }
}

void PromptManager::ensureDirectoryStructure(const QString &root)
{
    if (root.isEmpty() || root.startsWith(QStringLiteral(":"))) return;

    QDir rootDir(root);
    if (!rootDir.isAbsolute()) {
        qWarning() << "[PromptManager] Refusing to create relative prompt directory:" << root;
        return;
    }

    const QStringList categories{QStringLiteral("text"), QStringLiteral("audio"), QStringLiteral("image"), QStringLiteral("video")};
    for (const QString &cat : categories) {
        rootDir.mkpath(cat);
    }
}

void PromptManager::setPaths(const QString &seriesPath, const QString &episodePath)
{
    m_seriesBaseDir = seriesPath.isEmpty() ? QString() : QDir::toNativeSeparators(seriesPath + QStringLiteral("/prompts"));
    m_episodeBaseDir = episodePath.isEmpty() ? QString() : QDir::toNativeSeparators(episodePath + QStringLiteral("/prompts"));

    ensureDirectoryStructure(m_seriesBaseDir);
    ensureDirectoryStructure(m_episodeBaseDir);

    refreshPrompts();
}

void PromptManager::setActiveCategory(const QString &category)
{
    QString normalized = (category == QStringLiteral("script")) ? QStringLiteral("text") : category;
    if (m_activeCategory != normalized) {
        m_activeCategory = normalized;
        emit activeCategoryChanged();
        refreshPrompts();
    }
}

void PromptManager::refreshPrompts()
{
    QList<PromptItem> items;

    // 1. System Defaults: Read directly from embedded QRC (100% read-only by design)
    scanCategoryDirectory(QStringLiteral(":/prompts"), QStringLiteral("System"), m_activeCategory, items);

    // 2. Series & Episode: Read from writable project directories
    scanCategoryDirectory(m_seriesBaseDir, QStringLiteral("Series"), m_activeCategory, items);
    scanCategoryDirectory(m_episodeBaseDir, QStringLiteral("Episode"), m_activeCategory, items);

    QVariantList result;
    // Creation option at index 0
    QVariantMap newOpt;
    newOpt[QStringLiteral("label")] = tr("+ Create New Prompt...");
    newOpt[QStringLiteral("fileName")] = QString();
    newOpt[QStringLiteral("scope")] = QString();
    newOpt[QStringLiteral("fullPath")] = QString();
    newOpt[QStringLiteral("category")] = m_activeCategory;
    result.append(newOpt);

    for (const PromptItem &item : items) {
        QVariantMap map;
        map[QStringLiteral("label")] = QStringLiteral("%1 (%2)").arg(item.fileName, item.scope);
        map[QStringLiteral("fileName")] = item.fileName;
        map[QStringLiteral("scope")] = item.scope;
        map[QStringLiteral("fullPath")] = item.fullPath;
        map[QStringLiteral("category")] = item.category;
        result.append(map);
    }

    m_availablePrompts = result;
    emit availablePromptsChanged();
}

QString PromptManager::readPromptFile(const QString &fullPath)
{
    if (fullPath.isEmpty() || !QFile::exists(fullPath)) return QString();

    QFile file(fullPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit promptError(tr("Unable to open prompt file: %1").arg(fullPath));
        return QString();
    }
    QTextStream in(&file);
    return in.readAll();
}

bool PromptManager::savePrompt(const QString &fileName,
                               const QString &content,
                               const QString &category,
                               bool isSeriesScope)
{
    QString safeName = fileName.trimmed();
    if (safeName.isEmpty()) {
        emit promptError(tr("Filename cannot be empty."));
        return false;
    }
    if (!safeName.endsWith(QStringLiteral(".prompt"), Qt::CaseInsensitive)) {
        safeName += QStringLiteral(".prompt");
    }

    // Explicit path filtering: Never write to :/prompts or m_systemBaseDir
    QString targetBase = isSeriesScope ? m_seriesBaseDir : m_episodeBaseDir;
    if (targetBase.isEmpty()) {
        emit promptError(tr("Cannot overwrite System presets. Please open a Series or Episode to save custom prompts."));
        return false;
    }

    QString dirPath = targetBase + QStringLiteral("/") + category;
    QDir().mkpath(dirPath);

    QString fullPath = QDir::toNativeSeparators(dirPath + QStringLiteral("/") + safeName);
    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        emit promptError(tr("Failed to save prompt to: %1").arg(fullPath));
        return false;
    }

    QTextStream out(&file);
    out << content;
    file.close();

    emit promptSaved(fullPath);
    refreshPrompts();
    return true;
}

QStringList PromptManager::getFilesForCategory(const QString &category)
{
    QString cat = (category == QStringLiteral("script")) ? QStringLiteral("text") : category;
    QStringList result;

    // Scan the requested category directly
    auto scanDir = [&](const QString &baseDir) {
        if (baseDir.isEmpty()) return;
        QDir dir(baseDir + QStringLiteral("/") + cat);
        if (!dir.exists()) return;

        QStringList filters{QStringLiteral("*.prompt"), QStringLiteral("*.txt")};
        const QFileInfoList entries = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);
        for (const auto &fi : entries) {
            result.append(fi.fileName());
        }
    };

    scanDir(m_systemBaseDir);
    scanDir(m_seriesBaseDir);
    scanDir(m_episodeBaseDir);

    result.removeDuplicates();
    return result;
}