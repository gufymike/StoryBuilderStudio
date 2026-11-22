/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_EditorPane.cpp
** Unit test suite for EditorPane QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"

class Test_EditorPane : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testEditorInitialization();
    void testViewModeSwitching();
    void testFindAndReplaceAll();
    void testStatusBarMetrics();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *editor = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/text"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 800\n"
                "    height: 600\n"
                "    visible: true\n"
                "    EditorPane {\n"
                "        id: editorItem\n"
                "        anchors.fill: parent\n"
                "    }\n"
                "}\n"
                );

            engine.loadData(hostQml);
            if (engine.rootObjects().isEmpty()) {
                return false;
            }

            window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            if (!window) {
                return false;
            }

            editor = window->findChild<QObject*>(UI::EditorPane::Root);
            return (editor != nullptr);
        }
    };
};

void Test_EditorPane::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::cleanPath(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_EditorPane::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_EditorPane::testEditorInitialization()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load EditorPane harness");

    QObject *editor = harness.editor;
    QVERIFY(editor != nullptr);

    QObject *rawTextArea = editor->findChild<QObject*>(UI::EditorPane::RawTextArea);
    QVERIFY2(rawTextArea != nullptr, "UI Component Missing: 'rawTextArea' not found.");
    QObject *titleDisplay = editor->findChild<QObject*>(UI::EditorPane::TitleDisplay);
    QVERIFY2(titleDisplay != nullptr, "UI Component Missing: 'titleDisplay' not found.");

    QCOMPARE(rawTextArea->property("text").toString(), QStringLiteral(""));
    QCOMPARE(titleDisplay->property("text").toString(), QStringLiteral("Editor Pane"));
}

void Test_EditorPane::testViewModeSwitching()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load EditorPane harness");

    QObject *editor = harness.editor;
    QObject *editorStack = editor->findChild<QObject*>(UI::EditorPane::EditorStack);
    QObject *mdModeBtn = editor->findChild<QObject*>(UI::EditorPane::MdModeBtn);
    QObject *htmlModeBtn = editor->findChild<QObject*>(UI::EditorPane::HtmlModeBtn);
    QObject *rawModeBtn = editor->findChild<QObject*>(UI::EditorPane::RawModeBtn);
    QVERIFY2(editorStack && mdModeBtn && htmlModeBtn && rawModeBtn, "UI Components Missing: Stack layout or mode toggle buttons not found.");

    // Initial state: Raw Edit (0)
    QCOMPARE(editorStack->property("currentIndex").toInt(), 0);

    // Switch to Markdown Preview (1)
    QMetaObject::invokeMethod(mdModeBtn, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(editorStack->property("currentIndex").toInt(), 1);

    // Switch to HTML Preview (2)
    QMetaObject::invokeMethod(htmlModeBtn, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(editorStack->property("currentIndex").toInt(), 2);

    // Switch back to Raw Edit (0)
    QMetaObject::invokeMethod(rawModeBtn, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(editorStack->property("currentIndex").toInt(), 0);
}

void Test_EditorPane::testFindAndReplaceAll()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load EditorPane harness");

    QObject *editor = harness.editor;
    QObject *rawTextArea = editor->findChild<QObject*>(UI::EditorPane::RawTextArea);
    QVERIFY2(rawTextArea != nullptr, "UI Component Missing: 'rawTextArea' not found.");

    rawTextArea->setProperty("text", QStringLiteral("INT. HANGAR - DAY\nThe robot looks at the pilot. The robot beeps."));
    QCoreApplication::processEvents();

    // Call replaceAllOccurrences with QVariant to match the QML signature
    QMetaObject::invokeMethod(editor, "replaceAllOccurrences",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(QStringLiteral("robot"))),
                              Q_ARG(QVariant, QVariant(QStringLiteral("drone"))));
    QCoreApplication::processEvents();

    QString expected = QStringLiteral("INT. HANGAR - DAY\nThe drone looks at the pilot. The drone beeps.");
    QCOMPARE(rawTextArea->property("text").toString(), expected);
}

void Test_EditorPane::testStatusBarMetrics()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load EditorPane harness");

    QObject *editor = harness.editor;
    QObject *rawTextArea = editor->findChild<QObject*>(UI::EditorPane::RawTextArea);
    QVERIFY2(rawTextArea != nullptr, "UI Component Missing: 'rawTextArea' not found.");
    QObject *lineCountText = editor->findChild<QObject*>(UI::EditorPane::LineCountText);
    QObject *wordCountText = editor->findChild<QObject*>(UI::EditorPane::WordCountText);
    QObject *charCountText = editor->findChild<QObject*>(UI::EditorPane::CharCountText);
    QVERIFY2(lineCountText && wordCountText && charCountText, "UI Components Missing: Status bar text metric labels not found.");

    QVERIFY(rawTextArea && lineCountText && wordCountText && charCountText);

    rawTextArea->setProperty("text", QStringLiteral("First line.\nSecond line with words."));
    QCoreApplication::processEvents();

    QCOMPARE(lineCountText->property("text").toString(), QStringLiteral("Lines: 2"));
    QCOMPARE(wordCountText->property("text").toString(), QStringLiteral("Words: 6"));
    QCOMPARE(charCountText->property("text").toString(), QStringLiteral("Chars: 35"));
}

QTEST_MAIN(Test_EditorPane)
#include "test_EditorPane.moc"
