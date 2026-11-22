/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: storydocument.h
** Manages dual markdown and rich QTextDocument layout synchronization.
**
****************************************************************************/
#pragma once

#include <QObject>
#include <QString>
#include <QTextDocument>
#include <QtQml/QQmlEngine>

/**
 * @class StoryDocument
 * @brief Synchronizes raw Markdown and rich HTML representations for WYSIWYG editing.
 *
 * Wraps a QTextDocument to provide real-time dual synchronization between raw Markdown
 * and Rich HTML. Allows the QML text area to seamlessly toggle views and saves the
 * document to disk in either format without losing structural integrity.
 */
class StoryDocument : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString rawText READ rawText WRITE setRawText NOTIFY rawTextChanged)
    Q_PROPERTY(QString htmlText READ htmlText NOTIFY htmlTextChanged)

public:
    explicit StoryDocument(QObject *parent = nullptr);
    ~StoryDocument() override;

    Q_INVOKABLE bool loadLocalFile(const QString &filePath);
    Q_INVOKABLE bool saveLocalFile(const QString &filePath, const QString &format = QStringLiteral("markdown"));

    QString rawText() const { return m_rawMarkdownBuffer; }
    QString htmlText() const;

    QTextDocument* qtextDocument() { return &m_wysiwygDocument; }

public slots:
    void setRawText(const QString &text);

signals:
    void rawTextChanged();
    void htmlTextChanged();
    void fileOperationSuccess(const QString &message);
    void fileOperationFailure(const QString &errorMessage);

private:
    QString m_rawMarkdownBuffer;
    QTextDocument m_wysiwygDocument;
};

