// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUuid>
#include "app/storyboard/storydocument.h"

class Test_StoryDocument : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDefaultState();
    void testMarkdownToHtmlSync();
    void testSaveAndLoadMarkdown();
    void testSaveAndLoadHtml();
    void testLoadNonExistentFileFails();
    void testSaveTruncatesShorterOverwrite();

private:
    QString m_testSessionDir;
};

void Test_StoryDocument::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_StoryDocument::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_StoryDocument::testDefaultState()
{
    StoryDocument doc;
    QVERIFY(doc.rawText().isEmpty());
    QVERIFY(doc.qtextDocument() != nullptr);
}

void Test_StoryDocument::testMarkdownToHtmlSync()
{
    StoryDocument doc;
    QSignalSpy rawSpy(&doc, &StoryDocument::rawTextChanged);
    QSignalSpy htmlSpy(&doc, &StoryDocument::htmlTextChanged);

    doc.setRawText(QStringLiteral("# Chapter 1\n\nThe spacecraft exited hyperspace."));

    QCOMPARE(rawSpy.count(), 1);
    QCOMPARE(htmlSpy.count(), 1);

    QString html = doc.htmlText();
    QVERIFY(html.contains(QStringLiteral("Chapter 1")));
    QVERIFY(html.contains(QStringLiteral("The spacecraft exited hyperspace.")));
}

void Test_StoryDocument::testSaveAndLoadMarkdown()
{
    StoryDocument doc;
    const QString filePath = m_testSessionDir + QStringLiteral("/scene.md");

    doc.setRawText(QStringLiteral("## Beat 1: Confrontation\n\nDialogue begins here."));
    bool saved = doc.saveLocalFile(filePath, QStringLiteral("markdown"));
    QVERIFY(saved);
    QVERIFY(QFile::exists(filePath));

    StoryDocument readerDoc;
    QSignalSpy successSpy(&readerDoc, &StoryDocument::fileOperationSuccess);

    bool loaded = readerDoc.loadLocalFile(filePath);
    QVERIFY(loaded);
    QCOMPARE(successSpy.count(), 1);
    QCOMPARE(readerDoc.rawText(), QStringLiteral("## Beat 1: Confrontation\n\nDialogue begins here."));
}

void Test_StoryDocument::testSaveAndLoadHtml()
{
    StoryDocument doc;
    const QString filePath = m_testSessionDir + QStringLiteral("/scene.html");

    doc.setRawText(QStringLiteral("Plain text with **bold narrative**."));
    bool saved = doc.saveLocalFile(filePath, QStringLiteral("html"));
    QVERIFY(saved);

    StoryDocument readerDoc;
    bool loaded = readerDoc.loadLocalFile(filePath);
    QVERIFY(loaded);
    QVERIFY(readerDoc.htmlText().contains(QStringLiteral("bold narrative")));
}

void Test_StoryDocument::testLoadNonExistentFileFails()
{
    StoryDocument doc;
    QSignalSpy failureSpy(&doc, &StoryDocument::fileOperationFailure);

    bool loaded = doc.loadLocalFile(m_testSessionDir + QStringLiteral("/non_existent_file.md"));
    QVERIFY(!loaded);
    QCOMPARE(failureSpy.count(), 1);
}

void Test_StoryDocument::testSaveTruncatesShorterOverwrite()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString filePath = tempDir.filePath(QStringLiteral("script.md"));

    StoryDocument doc;
    doc.setRawText(QStringLiteral("First version with a very long paragraph of scene dialogue."));
    QVERIFY(doc.saveLocalFile(filePath, QStringLiteral("markdown")));

    doc.setRawText(QStringLiteral("Short v2."));
    QVERIFY(doc.saveLocalFile(filePath, QStringLiteral("markdown")));

    StoryDocument reloaded;
    QVERIFY(reloaded.loadLocalFile(filePath));
    QCOMPARE(reloaded.rawText(), QStringLiteral("Short v2."));
}

QTEST_MAIN(Test_StoryDocument)
#include "test_StoryDocument.moc"

