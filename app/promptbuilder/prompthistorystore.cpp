// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include "prompthistorystore.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

PromptHistoryStore::PromptHistoryStore(QObject *parent)
    : QObject(parent)
{
}

void PromptHistoryStore::setProjectPath(const QString &projectPath)
{
    if (m_projectPath == projectPath) return;

    m_projectPath = projectPath;
    emit projectPathChanged();
    emit historyUpdated();
    emit thinkHistoryUpdated();
}

QString PromptHistoryStore::getHistoryFilePath() const
{
    if (m_projectPath.isEmpty()) return QString();
    QDir dir(m_projectPath);
    return dir.absoluteFilePath(QStringLiteral("prompt_history.json"));
}

QString PromptHistoryStore::getThinkFilePath() const
{
    if (m_projectPath.isEmpty()) return QString();
    QDir dir(m_projectPath);
    return dir.absoluteFilePath(QStringLiteral("think_history.json"));
}

void PromptHistoryStore::addHistoryEntry(const QString &category,
                                         const QString &promptText,
                                         const QString &responseText,
                                         const QString &timestamp)
{
    QString filePath = getHistoryFilePath();
    if (filePath.isEmpty() || promptText.trimmed().isEmpty()) return;
    // Add these two debug lines:
    qDebug() << "[PromptHistoryStore] addHistoryEntry called. Target path:" << filePath;
    if (promptText.trimmed().isEmpty()) qDebug() << "[PromptHistoryStore] Bailing out because promptText is empty!";

    QJsonArray historyArray;
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray()) {
            historyArray = doc.array();
        }
        file.close();
    }

    QJsonObject entry;
    entry[QStringLiteral("timestamp")] = timestamp.isEmpty()
                                             ? QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
                                             : timestamp;
    entry[QStringLiteral("category")] = category;
    entry[QStringLiteral("promptText")] = promptText;
    entry[QStringLiteral("responseText")] = responseText.trimmed();
    historyArray.prepend(entry);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QJsonDocument doc(historyArray);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        emit historyUpdated();
    } else {
        qWarning() << "[PromptHistoryStore] Failed to write prompt history:" << file.errorString();
    }
}

QVariantList PromptHistoryStore::getHistoryEntries() const
{
    QString filePath = getHistoryFilePath();
    if (filePath.isEmpty()) return QVariantList();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QVariantList();

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) return QVariantList();

    QVariantList list;
    QJsonArray array = doc.array();
    list.reserve(array.size());
    for (int i = 0; i < array.size(); ++i) {
        list.append(array.at(i).toVariant());
    }
    return list;
}

void PromptHistoryStore::clearHistory()
{
    QString filePath = getHistoryFilePath();
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        QFile::remove(filePath);
        emit historyUpdated();
    }
}

void PromptHistoryStore::addThinkEntry(const QString &category,
                                       const QString &promptText,
                                       const QString &thinkText,
                                       const QString &timestamp)
{
    QString filePath = getThinkFilePath();
    if (filePath.isEmpty() || thinkText.trimmed().isEmpty()) return;
    // Add these two debug lines:
    qDebug() << "[PromptHistoryStore] addThinkEntry called. Target path:" << filePath;
    if (thinkText.trimmed().isEmpty()) qDebug() << "[PromptHistoryStore] Bailing out because thinkText is empty!";
    QJsonArray thinkArray;
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray()) {
            thinkArray = doc.array();
        }
        file.close();
    }

    QJsonObject entry;
    entry[QStringLiteral("timestamp")] = timestamp;
    entry[QStringLiteral("category")] = category;
    entry[QStringLiteral("promptText")] = promptText;
    entry[QStringLiteral("thinkText")] = thinkText.trimmed();

    thinkArray.prepend(entry);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QJsonDocument doc(thinkArray);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        emit thinkHistoryUpdated();
    } else {
        qWarning() << "[PromptHistoryStore] Failed to write think history:" << file.errorString();
    }
}

QVariantList PromptHistoryStore::getThinkEntries() const
{
    QString filePath = getThinkFilePath();
    if (filePath.isEmpty()) return QVariantList();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QVariantList();

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) return QVariantList();

    QVariantList list;
    QJsonArray array = doc.array();
    list.reserve(array.size());
    for (int i = 0; i < array.size(); ++i) {
        list.append(array.at(i).toVariant());
    }
    return list;
}

void PromptHistoryStore::clearThinkHistory()
{
    QString filePath = getThinkFilePath();
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        QFile::remove(filePath);
        emit thinkHistoryUpdated();
    }
}

