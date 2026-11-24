// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include "audittrailstore.h"

#include <QDir>
#include <QStandardPaths>
#include <QUrl>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

AuditTrailStore::AuditTrailStore(QObject *parent)
    : QObject(parent)
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!appData.isEmpty()) {
        m_storageDirectory = QDir::toNativeSeparators(appData);
    }
}

QString AuditTrailStore::storageDirectory() const
{
    return m_storageDirectory;
}

bool AuditTrailStore::isValidDirectoryPath(const QString &path) const
{
    if (path.trimmed().isEmpty()) return false;

    if (path.contains(QStringLiteral(".."))) {
        qWarning() << "[AuditTrailStore] Path traversal sequence detected and blocked:" << path;
        return false;
    }

    return true;
}

void AuditTrailStore::setStorageDirectory(const QString &directoryPath)
{
    QString cleanPath = QDir::toNativeSeparators(directoryPath.trimmed());
    if (cleanPath.startsWith(QStringLiteral("file:///"))) {
        cleanPath = QDir::toNativeSeparators(QUrl(directoryPath).toLocalFile());
    }

    if (!isValidDirectoryPath(cleanPath)) {
        return;
    }

    if (m_storageDirectory != cleanPath) {
        m_storageDirectory = cleanPath;
        emit storageDirectoryChanged(m_storageDirectory);
        emit transactionCountChanged(transactionCount());
    }
}

QString AuditTrailStore::getAuditFilePath() const
{
    if (m_storageDirectory.isEmpty()) return QString();
    QDir dir(m_storageDirectory);
    return dir.absoluteFilePath(QStringLiteral("audit_trail.json"));
}


int AuditTrailStore::transactionCount() const
{
    QString filePath = getAuditFilePath();
    if (filePath.isEmpty() || !QFile::exists(filePath)) return 0;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return 0;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    return doc.isArray() ? doc.array().size() : 0;
}

QVariantList AuditTrailStore::getTransactions() const
{
    QVariantList list;
    QString filePath = getAuditFilePath();
    if (filePath.isEmpty() || !QFile::exists(filePath)) return list;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return list;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isArray()) {
        const QJsonArray arr = doc.array();
        for (const QJsonValue &v : arr) {
            list.append(v.toObject().toVariantMap());
        }
    }
    return list;
}

void AuditTrailStore::clear()
{
    QString filePath = getAuditFilePath();
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (file.exists()) {
        file.remove();
        emit transactionCountChanged(0);
    }
}

void AuditTrailStore::recordTransaction(const QString &projectName,
                                        const QString &category,
                                        const QString &promptText,
                                        const QString &responseText,
                                        const QString &sentTimestamp,
                                        const QString &receivedTimestamp,
                                        const QStringList &selectedOptions,
                                        const bool &isThinkText)
{
    QString filePath = getAuditFilePath();
    if (filePath.isEmpty()) {
        qWarning() << "[AuditTrailStore] Error: Cannot write audit entry. No active project path set.";
        return;
    }

    // Security check: ensure target directory exists before file creation
    QFileInfo fileInfo(filePath);
    QDir parentDir = fileInfo.dir();
    if (!parentDir.exists()) {
        parentDir.mkpath(QStringLiteral("."));
    }

    QJsonArray auditArray;
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray()) {
            auditArray = doc.array();
        }
        file.close();
    }

    QJsonObject transaction;
    transaction[QStringLiteral("project_name")] = projectName;
    transaction[QStringLiteral("category")] = category;
    transaction[QStringLiteral("sent_timestamp")] = sentTimestamp;
    transaction[QStringLiteral("received_timestamp")] = receivedTimestamp;
    transaction[QStringLiteral("prompt_text")] = promptText;
    transaction[QStringLiteral("selected_options")] = QJsonArray::fromStringList(selectedOptions);
    transaction[QStringLiteral("response_text")] = responseText;
    transaction[QStringLiteral("is_think_text")] = isThinkText;

    auditArray.append(transaction);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QJsonDocument doc(auditArray);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.flush();
        file.close();

        emit transactionLogged(category, sentTimestamp);
        emit transactionCountChanged(auditArray.size());
    } else {
        qWarning() << "[AuditTrailStore] Failed to write to audit file:" << file.errorString();
    }
}

