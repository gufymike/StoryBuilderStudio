// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QtQml/QQmlEngine>

/**
 * @class AuditTrailStore
 * @brief Records narrative generation prompts, outputs, and model parameters.
 *
 * Provides a QML-accessible interface for logging and retrieving the history
 * of AI interactions. Backed by a JSON file on disk to ensure persistence
 * across application restarts.
 */
class AuditTrailStore : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    /**
     * @property storageDirectory
     * @brief The absolute path to the directory containing the audit_trail.json file.
     * @notifier storageDirectoryChanged
     */
    Q_PROPERTY(QString storageDirectory READ storageDirectory WRITE setStorageDirectory NOTIFY storageDirectoryChanged)

    /**
     * @property transactionCount
     * @brief The total number of transactions currently loaded in the active trail.
     * @notifier transactionCountChanged
     */
    Q_PROPERTY(int transactionCount READ transactionCount NOTIFY transactionCountChanged)

public:
    explicit AuditTrailStore(QObject *parent = nullptr);
    ~AuditTrailStore() override = default;

    QString storageDirectory() const;
    int transactionCount() const;

    /**
     * @brief Sets the active directory for the audit log, creating it if it doesn't exist.
     * @param directoryPath Absolute path to the desired storage folder.
     */
    Q_INVOKABLE void setStorageDirectory(const QString &directoryPath);

    /**
     * @brief Retrieves the absolute file path to the underlying JSON file.
     */
    Q_INVOKABLE QString getAuditFilePath() const;

    /**
     * @brief Appends a new inference request and response to the audit log.
     * @param projectName The name of the active project.
     * @param category The modality of the request (e.g., "script", "image").
     * @param promptText The user's input prompt.
     * @param responseText The model's output or the generated file path.
     * @param sentTimestamp ISO-8601 timestamp of request dispatch.
     * @param receivedTimestamp ISO-8601 timestamp of final response.
     * @param selectedOptions A list of formatted configuration options used.
     * @param isThinkText Indicates if the response represents model reasoning content.
     */
    Q_INVOKABLE void recordTransaction(const QString &projectName,
                                       const QString &category,
                                       const QString &promptText,
                                       const QString &responseText,
                                       const QString &sentTimestamp,
                                       const QString &receivedTimestamp,
                                       const QStringList &selectedOptions = QStringList(),
                                       const bool &isThinkText = false);

    /**
     * @brief Retrieves all logged transactions as a QML-compatible list of dictionaries.
     */
    Q_INVOKABLE QVariantList getTransactions() const;

    /**
     * @brief Deletes the underlying JSON file and resets the transaction count to zero.
     */
    Q_INVOKABLE void clear();

signals:
    void storageDirectoryChanged(const QString &newPath);
    void transactionCountChanged(int count);
    void transactionLogged(const QString &category, const QString &timestamp);

private:
    QString m_storageDirectory;
    bool isValidDirectoryPath(const QString &path) const;
};
