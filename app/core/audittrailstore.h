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
 * @brief Records narrative generation prompts, outputs, saved asset filenames, and model parameters.
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
     * @param category The modality of the request (e.g., "script", "image", "audio", "speech", "video").
     * @param promptText The user's input prompt.
     * @param responseText The model's text output or status summary.
     * @param sentTimestamp ISO-8601 timestamp of request dispatch.
     * @param receivedTimestamp ISO-8601 timestamp of final response.
     * @param selectedOptions A list of formatted configuration options used.
     * @param isThinkText Indicates if the response represents model reasoning content.
     * @param savedFile Bare filename(s) written to project assets, if applicable.
     * @param status Outcome status ("completed", "saved", "discarded", or "failed").
     * @param provider Active driver or model identifier (e.g., "gemini_bridge", "LocalLlmEngine").
     */
    Q_INVOKABLE void recordTransaction(const QString &projectName,
                                       const QString &category,
                                       const QString &promptText,
                                       const QString &responseText,
                                       const QString &sentTimestamp,
                                       const QString &receivedTimestamp,
                                       const QStringList &selectedOptions = QStringList(),
                                       const bool &isThinkText = false,
                                       const QString &savedFile = QString(),
                                       const QString &status = QStringLiteral("completed"),
                                       const QString &provider = QString());

    /**
     * @brief Updates saved_file and status on an existing transaction after user selection or file export.
     * @param sentTimestamp ISO-8601 timestamp UUID of the prompt entry (if empty, matches latest non-think entry in category).
     * @param savedFile Bare filename or full path (normalized to bare filename(s)).
     * @param status Updated outcome status (defaults to "saved").
     * @param category Optional modality fallback when sentTimestamp is not supplied.
     * @return True if a matching transaction was found and updated.
     */
    Q_INVOKABLE bool updateTransactionFile(const QString &sentTimestamp,
                                           const QString &savedFile,
                                           const QString &status = QStringLiteral("saved"),
                                           const QString &category = QString());

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
    void transactionUpdated(const QString &sentTimestamp, const QString &savedFile, const QString &status);

private:
    QString m_storageDirectory;
    bool isValidDirectoryPath(const QString &path) const;
    static QString normalizeSavedFileNames(const QString &rawFileOrList);
};
