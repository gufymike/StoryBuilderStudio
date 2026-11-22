/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_AssetWorkspaceSplitView.cpp
** Unit test suite for AssetWorkspaceSplitView QML component under QTest.
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

class Test_AssetWorkspaceSplitView : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testSplitViewInitializationAndPanes();
    void testTogglePromptPaneVisibility();
    void testTargetAssetTypePropagation();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *splitViewRoot = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/engine"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/promptbuilder"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 900\n"
                "    height: 600\n"
                "    visible: true\n"
                "    AssetWorkspaceSplitView {\n"
                "        id: workspaceSplit\n"
                "        anchors.fill: parent\n"
                "        targetAssetType: \"image\"\n"
                "        Rectangle {\n"
                "            objectName: \"injectedContentItem\"\n"
                "            anchors.fill: parent\n"
                "            color: \"#FF0000\"\n"
                "        }\n"
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

            splitViewRoot = window->findChild<QObject*>(UI::AssetWorkspaceSplitView::Root);
            return (splitViewRoot != nullptr);
        }
    };
};

void Test_AssetWorkspaceSplitView::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_AssetWorkspaceSplitView::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_AssetWorkspaceSplitView::testSplitViewInitializationAndPanes()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load AssetWorkspaceSplitView harness");

    QObject *root = harness.splitViewRoot;
    QVERIFY(root != nullptr);

    // Verify injected default content loaded into the left container
    QObject *injectedItem = root->findChild<QObject*>(QStringLiteral("injectedContentItem"));
    QVERIFY2(injectedItem != nullptr, "Test Only UI Component Missing: 'injectedContentItem' not found.");

    QObject *rightPane = root->findChild<QObject*>(UI::AssetWorkspaceSplitView::RightPaneContainer);
    QVERIFY2(rightPane != nullptr, "UI Component Missing: 'rightPaneContainer' not found.");
    QCOMPARE(rightPane->property("visible").toBool(), true);
}

void Test_AssetWorkspaceSplitView::testTogglePromptPaneVisibility()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load AssetWorkspaceSplitView harness");

    QObject *root = harness.splitViewRoot;
    QObject *rightPane = root->findChild<QObject*>(UI::AssetWorkspaceSplitView::RightPaneContainer);
    QVERIFY2(rightPane != nullptr, "UI Component Missing: 'rightPaneContainer' not found.");
    QObject *toggleBtn = root->findChild<QObject*>(UI::AssetWorkspaceSplitView::TogglePromptPaneBtn);
    QVERIFY2(toggleBtn != nullptr, "UI Component Missing: 'togglePromptPaneBtn' not found.");

    // Initial state: visible
    QCOMPARE(root->property("promptPaneVisible").toBool(), true);
    QCOMPARE(rightPane->property("visible").toBool(), true);

    // Toggle off
    QMetaObject::invokeMethod(toggleBtn, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(root->property("promptPaneVisible").toBool(), false);
    QCOMPARE(rightPane->property("visible").toBool(), false);

    // Toggle back on
    QMetaObject::invokeMethod(toggleBtn, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(root->property("promptPaneVisible").toBool(), true);
    QCOMPARE(rightPane->property("visible").toBool(), true);
}

void Test_AssetWorkspaceSplitView::testTargetAssetTypePropagation()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load AssetWorkspaceSplitView harness");

    QObject *root = harness.splitViewRoot;
    QObject *builder = root->findChild<QObject*>(UI::AssetWorkspaceSplitView::PromptBuilderInstance);
    QVERIFY2(builder != nullptr, "UI Component Missing: 'promptBuilderInstance' not found.");

    // Verified propagation of targetAssetType: "image"
    QCOMPARE(builder->property("targetAssetType").toString(), QStringLiteral("image"));

    // Dynamically change targetAssetType and verify propagation
    root->setProperty("targetAssetType", QStringLiteral("video"));
    QCoreApplication::processEvents();
    QCOMPARE(builder->property("targetAssetType").toString(), QStringLiteral("video"));
}

QTEST_MAIN(Test_AssetWorkspaceSplitView)
#include "test_AssetWorkspaceSplitView.moc"
