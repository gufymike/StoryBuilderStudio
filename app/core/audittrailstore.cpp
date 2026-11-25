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

QString AuditTrailStore::normalizeSavedFileNames(const QString &rawFileOrList)
{
    if (rawFileOrList.trimmed().isEmpty()) {
        return QString();
    }

    const QStringList parts = rawFileOrList.split(QLatin1Char(','), Qt::SkipEmptyParts);
    QStringList cleanNames;
    cleanNames.reserve(parts.size());

    for (const QString &part : parts) {
        QString trimmed = part.trimmed();
        if (trimmed.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
            trimmed = QUrl(trimmed).toLocalFile();
        }
        const QString baseName = QFileInfo(trimmed).fileName();
        if (!baseName.isEmpty() && !cleanNames.contains(baseName)) {
            cleanNames.append(baseName);
        }
    }

    return cleanNames.join(QStringLiteral(", "));
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
                                        const bool &isThinkText,
                                        const QString &savedFile,
                                        const QString &status,
                                        const QString &provider)
{
    QString filePath = getAuditFilePath();
    if (filePath.isEmpty()) {
        qWarning() << "[AuditTrailStore] Error: Cannot write audit entry. No active project path set.";
        return;
    }

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

    const QString cleanSavedFile = normalizeSavedFileNames(savedFile);
    QString resolvedStatus = status.trimmed();
    if (resolvedStatus.isEmpty()) {
        resolvedStatus = cleanSavedFile.isEmpty() ? QStringLiteral("completed") : QStringLiteral("saved");
    } else if (!cleanSavedFile.isEmpty() && resolvedStatus == QStringLiteral("completed")) {
        resolvedStatus = QStringLiteral("saved");
    }

    QJsonObject transaction;
    transaction[QStringLiteral("project_name")] = projectName;
    transaction[QStringLiteral("category")] = category;
    transaction[QStringLiteral("provider")] = provider;
    transaction[QStringLiteral("sent_timestamp")] = sentTimestamp;
    transaction[QStringLiteral("received_timestamp")] = receivedTimestamp;
    transaction[QStringLiteral("prompt_text")] = promptText;
    transaction[QStringLiteral("selected_options")] = QJsonArray::fromStringList(selectedOptions);
    transaction[QStringLiteral("response_text")] = responseText;
    transaction[QStringLiteral("saved_file")] = cleanSavedFile;
    transaction[QStringLiteral("status")] = resolvedStatus;
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

bool AuditTrailStore::updateTransactionFile(const QString &sentTimestamp,
                                            const QString &savedFile,
                                            const QString &status,
                                            const QString &category)
{
    QString filePath = getAuditFilePath();
    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) {
        return false;
    }

    QJsonArray auditArray = doc.array();
    int targetIndex = -1;

    // 1. Exact match by sent_timestamp UUID (non-think entry)
    if (!sentTimestamp.trimmed().isEmpty()) {
        for (int i = auditArray.size() - 1; i >= 0; --i) {
            const QJsonObject obj = auditArray.at(i).toObject();
            if (!obj.value(QStringLiteral("is_think_text")).toBool() &&
                obj.value(QStringLiteral("sent_timestamp")).toString() == sentTimestamp) {
                targetIndex = i;
                break;
            }
        }
    }

    // 2. Fallback: latest non-think entry in the specified modality category
    if (targetIndex < 0 && !category.trimmed().isEmpty()) {
        for (int i = auditArray.size() - 1; i >= 0; --i) {
            const QJsonObject obj = auditArray.at(i).toObject();
            if (!obj.value(QStringLiteral("is_think_text")).toBool() &&
                obj.value(QStringLiteral("category")).toString().compare(category, Qt::CaseInsensitive) == 0) {
                targetIndex = i;
                break;
            }
        }
    }

    if (targetIndex < 0) {
        return false;
    }

    QJsonObject targetObj = auditArray.at(targetIndex).toObject();
    const QString cleanSavedFile = normalizeSavedFileNames(savedFile);
    const QString resolvedStatus = status.trimmed().isEmpty()
                                       ? (cleanSavedFile.isEmpty() ? QStringLiteral("completed") : QStringLiteral("saved"))
                                       : status.trimmed();

    targetObj[QStringLiteral("saved_file")] = cleanSavedFile;
    targetObj[QStringLiteral("status")] = resolvedStatus;
    auditArray[targetIndex] = targetObj;

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "[AuditTrailStore] Failed to update audit file:" << file.errorString();
        return false;
    }

    QJsonDocument updatedDoc(auditArray);
    file.write(updatedDoc.toJson(QJsonDocument::Indented));
    file.flush();
    file.close();

    emit transactionUpdated(targetObj.value(QStringLiteral("sent_timestamp")).toString(),
                            cleanSavedFile,
                            resolvedStatus);
    return true;
}
