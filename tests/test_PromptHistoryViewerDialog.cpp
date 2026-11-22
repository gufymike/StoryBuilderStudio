/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_PromptHistoryViewerDialog.cpp
** Unit test suite for PromptHistoryViewerDialog QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"

class Test_PromptHistoryViewerDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testWindowInstantiationAndDefaults();
    void testHistoryLoadingAndSelection();
    void testViewSwitcherToolbarTransitions();

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_PromptHistoryViewerDialog::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    m_engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
    m_engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
    m_engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/promptbuilder"));

    QString qmlFilePath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/promptbuilder/PromptHistoryViewerDialog.qml");
    m_componentUrl = QUrl::fromLocalFile(qmlFilePath);
    QVERIFY2(QFile::exists(qmlFilePath), qPrintable("File not found: " + qmlFilePath));
}

void Test_PromptHistoryViewerDialog::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_PromptHistoryViewerDialog::testWindowInstantiationAndDefaults()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> windowObj(component.create());
    QVERIFY(!windowObj.isNull());

    QQuickWindow *window = qobject_cast<QQuickWindow*>(windowObj.data());
    QVERIFY(window != nullptr);

    QCOMPARE(window->title(), QStringLiteral("Prompt & Thought History Inspector"));

    QObject *promptHeader = window->findChild<QObject*>(UI::PromptHistoryViewerDialog::PromptHeader);
    QVERIFY2(promptHeader != nullptr, "UI Component Missing: 'promptHeaderDisplay' not found.");
    QObject *responseArea = window->findChild<QObject*>(UI::PromptHistoryViewerDialog::ResponseArea);
    QVERIFY2(responseArea != nullptr, "UI Component Missing: 'responseArea' not found.");

    QCOMPARE(promptHeader->property("text").toString(), QStringLiteral("<No prompt selected>"));
    QCOMPARE(responseArea->property("text").toString(), QStringLiteral("<No Response Payload Recorded>"));
}

void Test_PromptHistoryViewerDialog::testHistoryLoadingAndSelection()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> windowObj(component.create());
    QVERIFY(!windowObj.isNull());

    QVariantList mockList;
    QVariantMap entry;
    entry[QStringLiteral("timestamp")] = QStringLiteral("2026-11-07T12:00:00.000Z");
    entry[QStringLiteral("category")] = QStringLiteral("script");
    entry[QStringLiteral("promptText")] = QStringLiteral("Write a dramatic intro scene.");
    entry[QStringLiteral("responseText")] = QStringLiteral("FADE IN: A lone spacecraft drifting in orbit.");
    entry[QStringLiteral("thinkText")] = QStringLiteral("Establish tension first before dialogue.");
    mockList.append(entry);

    windowObj->setProperty("mockEntries", mockList);

    // Call loadMergedHistory
    QMetaObject::invokeMethod(windowObj.data(), "loadMergedHistory");
    QCoreApplication::processEvents();

    QCOMPARE(windowObj->property("selectedTimestamp").toString(), QStringLiteral("2026-11-07T12:00:00.000Z"));
    QCOMPARE(windowObj->property("selectedPromptText").toString(), QStringLiteral("Write a dramatic intro scene."));
    QCOMPARE(windowObj->property("selectedResponseText").toString(), QStringLiteral("FADE IN: A lone spacecraft drifting in orbit."));
    QCOMPARE(windowObj->property("selectedThinkText").toString(), QStringLiteral("Establish tension first before dialogue."));

    QObject *promptHeader = windowObj->findChild<QObject*>(UI::PromptHistoryViewerDialog::PromptHeader);
    QVERIFY2(promptHeader != nullptr, "UI Component Missing: 'promptHeaderDisplay' not found.");

    QObject *responseArea = windowObj->findChild<QObject*>(UI::PromptHistoryViewerDialog::ResponseArea);
    QVERIFY2(responseArea != nullptr, "UI Component Missing: 'responseArea' not found.");

    QCOMPARE(promptHeader->property("text").toString(), QStringLiteral("Write a dramatic intro scene."));
    QCOMPARE(responseArea->property("text").toString(), QStringLiteral("FADE IN: A lone spacecraft drifting in orbit."));
}

void Test_PromptHistoryViewerDialog::testViewSwitcherToolbarTransitions()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> windowObj(component.create());
    QVERIFY(!windowObj.isNull());

    QObject *stack = windowObj->findChild<QObject*>(UI::PromptHistoryViewerDialog::ViewerStack);
    QObject *btnThink = windowObj->findChild<QObject*>(UI::PromptHistoryViewerDialog::BtnViewThink);
    QObject *btnCombined = windowObj->findChild<QObject*>(UI::PromptHistoryViewerDialog::BtnViewCombined);
    QVERIFY2(stack && btnThink && btnCombined, "UI Components Missing: viewer switcher buttons or stack layout not found.");

    // Switch to View 1: Reasoning
    QMetaObject::invokeMethod(btnThink, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(windowObj->property("currentStackIndex").toInt(), 1);
    QCOMPARE(stack->property("currentIndex").toInt(), 1);

    // Switch to View 2: Combined
    QMetaObject::invokeMethod(btnCombined, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(windowObj->property("currentStackIndex").toInt(), 2);
    QCOMPARE(stack->property("currentIndex").toInt(), 2);
}

QTEST_MAIN(Test_PromptHistoryViewerDialog)
#include "test_PromptHistoryViewerDialog.moc"
