// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

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

class Test_PromptEditorDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testWindowPropertiesAndDefaults();
    void testFileSelectionUpdatesEditor();
    void testDirtyStateTracking();

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_PromptEditorDialog::initTestCase()
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

    QString qmlFilePath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/promptbuilder/PromptEditorDialog.qml");
    m_componentUrl = QUrl::fromLocalFile(qmlFilePath);
    QVERIFY2(QFile::exists(qmlFilePath), qPrintable("File not found: " + qmlFilePath));
}

void Test_PromptEditorDialog::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_PromptEditorDialog::testWindowPropertiesAndDefaults()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> windowObj(component.create());
    QVERIFY(!windowObj.isNull());

    QQuickWindow *window = qobject_cast<QQuickWindow*>(windowObj.data());
    QVERIFY(window != nullptr);

    QCOMPARE(window->title(), QStringLiteral("Edit Default Prompts - Story Builder Studio"));

    QObject *categoryCombo = window->findChild<QObject*>(UI::PromptEditorDialog::CategoryCombo);
    QVERIFY2(categoryCombo != nullptr, "UI Component Missing: 'categoryCombo' not found.");
    QCOMPARE(categoryCombo->property("currentText").toString(), QStringLiteral("text"));

    QObject *statusLabel = window->findChild<QObject*>(UI::PromptEditorDialog::StatusLabel);
    QVERIFY2(statusLabel != nullptr, "UI Component Missing: 'statusLabel' not found.");
    QCOMPARE(statusLabel->property("text").toString(), QStringLiteral(""));
}

void Test_PromptEditorDialog::testFileSelectionUpdatesEditor()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> windowObj(component.create());
    QVERIFY(!windowObj.isNull());

    QVariantList prompts;
    QVariantMap newPromptItem;
    newPromptItem[QStringLiteral("label")] = QStringLiteral("+ New Prompt...");
    prompts.append(newPromptItem);

    QVariantMap existingItem;
    existingItem[QStringLiteral("label")] = QStringLiteral("Intro Scene");
    existingItem[QStringLiteral("fileName")] = QStringLiteral("Intro_Scene.md");
    existingItem[QStringLiteral("content")] = QStringLiteral("FADE IN: Deep space nebula.");
    prompts.append(existingItem);

    windowObj->setProperty("mockAvailablePrompts", prompts);
    QCoreApplication::processEvents();

    QObject *fileCombo = windowObj->findChild<QObject*>(UI::PromptEditorDialog::FileCombo);
    QObject *nameRow = windowObj->findChild<QObject*>(UI::PromptEditorDialog::NameRow);
    QObject *promptEditor = windowObj->findChild<QObject*>(UI::PromptEditorDialog::PromptEditor);
    QVERIFY2(fileCombo && nameRow && promptEditor, "UI Components Missing: fileCombo, nameRow, or promptEditor not found.");

    // Initial state: index 0 -> nameRow visible
    QCOMPARE(nameRow->property("visible").toBool(), true);

    // Switch to existing prompt (index 1)
    fileCombo->setProperty("currentIndex", 1);
    QMetaObject::invokeMethod(fileCombo, "activated", Qt::DirectConnection, Q_ARG(int, 1));
    QCoreApplication::processEvents();

    QCOMPARE(nameRow->property("visible").toBool(), false);
    QCOMPARE(promptEditor->property("text").toString(), QStringLiteral("FADE IN: Deep space nebula."));
}

void Test_PromptEditorDialog::testDirtyStateTracking()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> windowObj(component.create());
    QVERIFY(!windowObj.isNull());

    QObject *promptEditor = windowObj->findChild<QObject*>(UI::PromptEditorDialog::PromptEditor);
    QObject *statusLabel = windowObj->findChild<QObject*>(UI::PromptEditorDialog::StatusLabel);
    QVERIFY2(promptEditor && statusLabel, "UI Components Missing: statusLabel or promptEditor not found.");

    QCOMPARE(windowObj->property("isDirty").toBool(), false);

    // Modify text
    promptEditor->setProperty("text", QStringLiteral("Updated prompt text"));
    QCoreApplication::processEvents();

    QVERIFY2(windowObj->property("isDirty").toBool(), "State Tracking Failure: Window did not flag as dirty after text modification.");
    QCOMPARE(statusLabel->property("text").toString(), QStringLiteral("● Unsaved changes"));
}

QTEST_MAIN(Test_PromptEditorDialog)
#include "test_PromptEditorDialog.moc"
