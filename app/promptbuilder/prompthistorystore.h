// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/QQmlEngine>
/**
 * @class   PromptHistoryStore
 * @brief   Manages JSON persistence for prompt execution logs and reasoning/think chains.
 */
class PromptHistoryStore : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString projectPath READ projectPath WRITE setProjectPath NOTIFY projectPathChanged)

public:
    explicit PromptHistoryStore(QObject *parent = nullptr);
    ~PromptHistoryStore() override = default;

    QString projectPath() const { return m_projectPath; }
    Q_INVOKABLE void setProjectPath(const QString &projectPath);

    // Regular prompt history (supports explicit timestamp / UUID)
    Q_INVOKABLE void addHistoryEntry(const QString &category,
                                     const QString &promptText,
                                     const QString &responseText,
                                     const QString &timestamp);
    Q_INVOKABLE QVariantList getHistoryEntries() const;
    Q_INVOKABLE void clearHistory();

    // Think history (uses think_history.json with matching timestamp)
    Q_INVOKABLE void addThinkEntry(const QString &category,
                                   const QString &promptText,
                                   const QString &thinkText,
                                   const QString &timestamp);
    Q_INVOKABLE QVariantList getThinkEntries() const;
    Q_INVOKABLE void clearThinkHistory();

signals:
    void projectPathChanged();
    void historyUpdated();
    void thinkHistoryUpdated();

private:
    QString m_projectPath;
    QString getHistoryFilePath() const;
    QString getThinkFilePath() const;
};

