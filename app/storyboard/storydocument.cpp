// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include "storydocument.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>

StoryDocument::StoryDocument(QObject *parent)
    : QObject(parent)
{
}

StoryDocument::~StoryDocument() = default;

bool StoryDocument::loadLocalFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit fileOperationFailure(tr("Failed to open file: %1").arg(filePath));
        return false;
    }

    QTextStream in(&file);
    QString incomingData = in.readAll();
    file.close();

    QFileInfo info(filePath);
    QString extension = info.suffix().toLower();

    if (extension == QStringLiteral("html") || extension == QStringLiteral("htm")) {
        m_wysiwygDocument.setHtml(incomingData);
        m_rawMarkdownBuffer = m_wysiwygDocument.toMarkdown();
    } else {
        m_rawMarkdownBuffer = incomingData;
        m_wysiwygDocument.setMarkdown(m_rawMarkdownBuffer);
    }

    emit rawTextChanged();
    emit htmlTextChanged();
    emit fileOperationSuccess(tr("File loaded successfully into the document matrix."));
    return true;
}

bool StoryDocument::saveLocalFile(const QString &filePath, const QString &format)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit fileOperationFailure(tr("Write initialization failure: %1").arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    QString outputData;

    QString fmt = format.toLower();
    if (fmt == QStringLiteral("html")) {
        outputData = m_wysiwygDocument.toHtml();
    } else if (fmt == QStringLiteral("text") || fmt == QStringLiteral("txt")) {
        outputData = m_wysiwygDocument.toPlainText();
    } else {
        outputData = m_rawMarkdownBuffer;
    }

    out << outputData;
    file.close();
    emit fileOperationSuccess(tr("Story buffer safely written to local disk tracks."));
    return true;
}

void StoryDocument::setRawText(const QString &text)
{
    if (m_rawMarkdownBuffer == text) return;

    m_rawMarkdownBuffer = text;
    m_wysiwygDocument.setMarkdown(m_rawMarkdownBuffer);

    emit rawTextChanged();
    emit htmlTextChanged();
}

QString StoryDocument::htmlText() const
{
    return m_wysiwygDocument.toHtml();
}

