/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_WorkspaceView.cpp
** Unit test suite for WorkspaceView aggregator component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQuickItem>

#include "scriptsdirectorymodel.h"
#include "imagedirectorymodel.h"
#include "audiodirectorymodel.h"
#include "audiowaveformmodel.h"
#include "videodirectorymodel.h"
#include "ui_constants.h"

class MockProjectManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isProjectOpen READ isProjectOpen NOTIFY isProjectOpenChanged)
    Q_PROPERTY(QString projectName READ projectName NOTIFY projectNameChanged)
    Q_PROPERTY(QString currentProjectPath READ currentProjectPath NOTIFY currentProjectPathChanged)

public:
    explicit MockProjectManager(QObject *parent = nullptr)
        : QObject(parent), m_isOpen(true), m_name(QStringLiteral("CyberOdyssey_Ep01")),
        m_path(QStringLiteral("C:/test_project")) {}

    bool isProjectOpen() const { return m_isOpen; }
    QString projectName() const { return m_name; }
    QString currentProjectPath() const { return m_path; }

    Q_INVOKABLE bool saveCandidateImage(const QString &) { return true; }

signals:
    void isProjectOpenChanged();
    void projectNameChanged();
    void currentProjectPathChanged();
    void assetDeleted(const QString &path);

private:
    bool m_isOpen;
    QString m_name;
    QString m_path;
};

class MockEngineRegistry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* activeScriptDriver READ activeScriptDriver CONSTANT)
    Q_PROPERTY(QObject* activeImageDriver READ activeImageDriver CONSTANT)
    Q_PROPERTY(QObject* activeAudioDriver READ activeAudioDriver CONSTANT)
    Q_PROPERTY(QObject* activeVideoDriver READ activeVideoDriver CONSTANT)

public:
    explicit MockEngineRegistry(QObject *parent = nullptr) : QObject(parent) {}
    QObject* activeScriptDriver() const { return nullptr; }
    QObject* activeImageDriver() const { return nullptr; }
    QObject* activeAudioDriver() const { return nullptr; }
    QObject* activeVideoDriver() const { return nullptr; }

    Q_INVOKABLE QStringList availableDriversForAsset(const QString &) const {
        return QStringList();
    }

    Q_INVOKABLE QString getActiveDriverId(const QString &) const {
        return QString();
    }

    Q_INVOKABLE void setActiveDriver(const QString &, const QString &) {}
};
class Test_WorkspaceView : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testViewInitializationAndHeader();
    void testTabNavigation();
    void testToolTipsConfigured();
    void testTabSwitchingPropagatesPromptCategory();

private:
    QString m_testSessionDir;
    MockProjectManager m_mockProj;
    MockEngineRegistry m_mockEngine;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *view = nullptr;

        bool init(const QString &sessionDir, MockProjectManager *proj, MockEngineRegistry *eng) {
            engine.rootContext()->setContextProperty(QStringLiteral("projectManager"), proj);
            engine.rootContext()->setContextProperty(QStringLiteral("engineRegistry"), eng);

            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/text"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/image"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/audio"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/video"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/storyboard"));
            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 1100\n"
                "    height: 800\n"
                "    visible: true\n"
                "    WorkspaceView {\n"
                "        id: workspaceItem\n"
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

            view = window->findChild<QObject*>(UI::WorkspaceView::Root);
            return (view != nullptr);
        }
    };
};

void Test_WorkspaceView::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    qmlRegisterType<ImageDirectoryModel>("StoryBuilderStudio", 1, 0, "ImageDirectoryModel");
    qmlRegisterType<AudioDirectoryModel>("StoryBuilderStudio", 1, 0, "AudioDirectoryModel");
    qmlRegisterType<AudioWaveformModel>("StoryBuilderStudio", 1, 0, "AudioWaveformModel");
    qmlRegisterType<VideoDirectoryModel>("StoryBuilderStudio", 1, 0, "VideoDirectoryModel");
    qmlRegisterType<ScriptsDirectoryModel>("StoryBuilderStudio", 1, 0, "ScriptsDirectoryModel");

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::cleanPath(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_WorkspaceView::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_WorkspaceView::testViewInitializationAndHeader()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &m_mockProj, &m_mockEngine), "Failed to load WorkspaceView harness");

    QObject *view = harness.view;
    QVERIFY(view != nullptr);

    QObject *appTitle = view->findChild<QObject*>(UI::WorkspaceView::AppTitleText);
    QVERIFY2(appTitle != nullptr, "UI Component Missing: 'appTitleText' not found.");
    QObject *episodeStatus = view->findChild<QObject*>(UI::WorkspaceView::EpisodeStatusText);
    QVERIFY2(episodeStatus != nullptr, "UI Component Missing: 'episodeStatusText' not found.");

    QCOMPARE(appTitle->property("text").toString(), QStringLiteral("Story Builder Studio"));
    QCOMPARE(episodeStatus->property("text").toString(), QStringLiteral("Episode: CyberOdyssey_Ep01"));
}

void Test_WorkspaceView::testTabNavigation()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &m_mockProj, &m_mockEngine), "Failed to load WorkspaceView harness");

    QObject *view = harness.view;
    QObject *tabBar = view->findChild<QObject*>(UI::WorkspaceView::WorkspaceTabBar);
    QObject *stack = view->findChild<QObject*>(UI::WorkspaceView::WorkspaceStack);
    QVERIFY2(tabBar && stack, "UI Components Missing: 'workspaceTabBar' or 'workspaceStack' not found.");

    // Initial Tab: 0 (Project)
    QCOMPARE(stack->property("currentIndex").toInt(), 0);

    // Switch to Storyboard (1)
    tabBar->setProperty("currentIndex", 1);
    QCoreApplication::processEvents();
    QCOMPARE(stack->property("currentIndex").toInt(), 1);

    // Switch to Script (2)
    tabBar->setProperty("currentIndex", 2);
    QCoreApplication::processEvents();
    QCOMPARE(stack->property("currentIndex").toInt(), 2);

    // Switch to Images (3)
    tabBar->setProperty("currentIndex", 3);
    QCoreApplication::processEvents();
    QCOMPARE(stack->property("currentIndex").toInt(), 3);
}

void Test_WorkspaceView::testToolTipsConfigured()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &m_mockProj, &m_mockEngine), "Failed to load WorkspaceView harness");

    QObject *view = harness.view;
    QVERIFY(view != nullptr);

    auto getAttachedToolTip = [](QObject *item) -> QString {
        if (!item) return QString();
        QQmlContext *ctx = QQmlEngine::contextForObject(item);
        if (!ctx) return QString();
        QQmlExpression expr(ctx, qobject_cast<QQuickItem*>(item), QStringLiteral("ToolTip.text"));
        return expr.evaluate().toString();
    };

    QObject *projectTab = view->findChild<QObject*>(UI::WorkspaceView::ProjectTabBtn);
    QObject *storyboardTab = view->findChild<QObject*>(UI::WorkspaceView::StoryboardTabBtn);
    QObject *scriptTab = view->findChild<QObject*>(UI::WorkspaceView::ScriptTabBtn);
    QVERIFY2(projectTab && storyboardTab && scriptTab, "UI Components Missing: Workspace tab buttons not found for tooltip verification.");

    QCOMPARE(getAttachedToolTip(projectTab),
             QStringLiteral("Manage project configuration, episode metadata, and directory structure"));
    QCOMPARE(getAttachedToolTip(storyboardTab),
             QStringLiteral("Sequence shots, assign keyframes, and structure timeline beats"));
    QCOMPARE(getAttachedToolTip(scriptTab),
             QStringLiteral("Screenplay, beats editor, markdown preview, and prompt dispatcher"));
}

void Test_WorkspaceView::testTabSwitchingPropagatesPromptCategory()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &m_mockProj, &m_mockEngine), "Failed to load WorkspaceView harness");

    QObject *view = harness.view;
    QObject *tabBar = view->findChild<QObject*>(UI::WorkspaceView::WorkspaceTabBar);
    QVERIFY2(tabBar != nullptr, "UI Components Missing: 'workspaceTabBar' not found.");

    // Switch to Script tab (index 2)
    tabBar->setProperty("currentIndex", 2);
    QCoreApplication::processEvents();

    QObject *scriptSplit = view->findChild<QObject*>(UI::WorkspaceView::ScriptWorkspace);
    QVERIFY2(scriptSplit != nullptr, "UI Component Missing: 'scriptWorkspace' not found.");
    QCOMPARE(scriptSplit->property("targetAssetType").toString(), QStringLiteral("script"));

    // Switch to Images tab (index 3)
    tabBar->setProperty("currentIndex", 3);
    QCoreApplication::processEvents();

    QObject *imageSplit = view->findChild<QObject*>(UI::WorkspaceView::ImageWorkspace);
    QVERIFY2(imageSplit != nullptr, "UI Component Missing: 'imageWorkspace' not found.");
    QCOMPARE(imageSplit->property("targetAssetType").toString(), QStringLiteral("image"));

    // Switch to Audio tab (index 4)
    tabBar->setProperty("currentIndex", 4);
    QCoreApplication::processEvents();

    QObject *audioSplit = view->findChild<QObject*>(UI::WorkspaceView::AudioWorkspace);
    QVERIFY2(audioSplit != nullptr, "UI Component Missing: 'audioWorkspace' not found.");
    QCOMPARE(audioSplit->property("targetAssetType").toString(), QStringLiteral("audio"));

    // Switch to Video tab (index 5)
    tabBar->setProperty("currentIndex", 5);
    QCoreApplication::processEvents();

    QObject *videoSplit = view->findChild<QObject*>(UI::WorkspaceView::VideoWorkspace);
    QVERIFY2(videoSplit != nullptr, "UI Component Missing: 'videoWorkspace' not found.");
    QCOMPARE(videoSplit->property("targetAssetType").toString(), QStringLiteral("video"));
}

QTEST_MAIN(Test_WorkspaceView)
#include "test_WorkspaceView.moc"
