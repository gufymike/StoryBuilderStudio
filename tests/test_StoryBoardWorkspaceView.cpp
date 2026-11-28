// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

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
#include "videodirectorymodel.h"
#include "storyboardexporter.h"
#include "ui_constants.h"

class MockWorkspaceStoryboardManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList shots READ shots NOTIFY shotsChanged)
    Q_PROPERTY(bool hasValidPath READ hasValidPath NOTIFY hasValidPathChanged)
    Q_PROPERTY(int shotCount READ shotCount NOTIFY shotCountChanged)
    Q_PROPERTY(QString episodePath READ episodePath WRITE setEpisodePath NOTIFY episodePathChanged)

public:
    explicit MockWorkspaceStoryboardManager(QObject *parent = nullptr)
        : QObject(parent), m_hasValidPath(true)
    {
        QVariantMap s1;
        s1[QStringLiteral("name")] = QStringLiteral("Shot 01 Ext Space");
        s1[QStringLiteral("key_image")] = QStringLiteral("");
        s1[QStringLiteral("final_shot")] = QStringLiteral("");
        s1[QStringLiteral("images")] = QVariantList{};
        s1[QStringLiteral("audio")] = QVariantList{};
        s1[QStringLiteral("videos")] = QVariantList{};
        m_shots.append(s1);
    }

    QVariantList shots() const { return m_shots; }
    bool hasValidPath() const { return m_hasValidPath; }
    int shotCount() const { return m_shots.size(); }
    QString episodePath() const { return m_episodePath; }

    void setEpisodePath(const QString &p) {
        if (m_episodePath != p) {
            m_episodePath = p;
            emit episodePathChanged();
        }
    }

    Q_INVOKABLE bool addShot(const QString &title) {
        QVariantMap s;
        s[QStringLiteral("name")] = title;
        s[QStringLiteral("key_image")] = QStringLiteral("");
        s[QStringLiteral("final_shot")] = QStringLiteral("");
        s[QStringLiteral("images")] = QVariantList{};
        s[QStringLiteral("audio")] = QVariantList{};
        s[QStringLiteral("videos")] = QVariantList{};
        m_shots.append(s);
        emit shotsChanged();
        emit shotCountChanged();
        return true;
    }

    Q_INVOKABLE void moveShotUp(int index) { Q_UNUSED(index); }
    Q_INVOKABLE void moveShotDown(int index) { Q_UNUSED(index); }
    Q_INVOKABLE void removeShot(int index) {
        if (index >= 0 && index < m_shots.size()) {
            m_shots.removeAt(index);
            emit shotsChanged();
            emit shotCountChanged();
        }
    }
    Q_INVOKABLE void cleanupDeletedAsset(const QString &) {}

signals:
    void shotsChanged();
    void hasValidPathChanged();
    void shotCountChanged();
    void episodePathChanged();
    void boardError(const QString &err);
    void boardSaved();

private:
    QVariantList m_shots;
    bool m_hasValidPath;
    QString m_episodePath;
};

class Test_StoryBoardWorkspaceView : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testViewInitializationAndShotStats();
    void testAddShotCardInteraction();
    void testToolTipsConfigured();

private:
    QString m_testSessionDir;
    MockWorkspaceStoryboardManager m_mockMgr;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *workspace = nullptr;

        bool init(const QString &sessionDir, MockWorkspaceStoryboardManager *mgr) {
            engine.rootContext()->setContextProperty(QStringLiteral("storyBoardManager"), mgr);

            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/storyboard"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/image"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/audio"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/video"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 900\n"
                "    height: 700\n"
                "    visible: true\n"
                "    StoryBoardWorkspaceView {\n"
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

            workspace = window->findChild<QObject*>(UI::StoryBoardWorkspaceView::Root);
            return (workspace != nullptr);
        }
    };
};

void Test_StoryBoardWorkspaceView::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    qmlRegisterType<ImageDirectoryModel>("StoryBuilderStudio", 1, 0, "ImageDirectoryModel");
    qmlRegisterType<AudioDirectoryModel>("StoryBuilderStudio", 1, 0, "AudioDirectoryModel");
    qmlRegisterType<VideoDirectoryModel>("StoryBuilderStudio", 1, 0, "VideoDirectoryModel");
    qmlRegisterType<ScriptsDirectoryModel>("StoryBuilderStudio", 1, 0, "ScriptsDirectoryModel");
    qmlRegisterType<StoryBoardExporter>("StoryBuilderStudio", 1, 0, "StoryBoardExporter");

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::cleanPath(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_StoryBoardWorkspaceView::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_StoryBoardWorkspaceView::testViewInitializationAndShotStats()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &m_mockMgr), "Failed to load StoryBoardWorkspaceView harness");

    QObject *workspace = harness.workspace;
    QVERIFY(workspace != nullptr);

    QObject *headerTitle = workspace->findChild<QObject*>(UI::StoryBoardWorkspaceView::ViewHeaderTitle);
    QVERIFY2(headerTitle != nullptr, "UI Component Missing: 'viewHeaderTitle' not found.");
    QObject *status = workspace->findChild<QObject*>(UI::StoryBoardWorkspaceView::StatusText);
    QVERIFY2(status != nullptr, "UI Component Missing: 'statusText' not found.");
    QObject *statsText = workspace->findChild<QObject*>(UI::StoryBoardWorkspaceView::ShotStatsText);
    QVERIFY2(statsText != nullptr, "UI Component Missing: 'shotStatsText' not found.");

    QCOMPARE(headerTitle->property("text").toString(), QStringLiteral("Production Story Board"));
    QCOMPARE(statsText->property("text").toString(), QStringLiteral("(1 Shots)"));
    QCOMPARE(status->property("text").toString(), QStringLiteral("Source of truth: storyboard.json"));
}

void Test_StoryBoardWorkspaceView::testAddShotCardInteraction()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &m_mockMgr), "Failed to load StoryBoardWorkspaceView harness");

    QObject *workspace = harness.workspace;
    QObject *addShotBtn = workspace->findChild<QObject*>(UI::StoryBoardWorkspaceView::AddShotBtn);
    QObject *addShotDialog = workspace->findChild<QObject*>(UI::StoryBoardWorkspaceView::AddShotDialog);
    QVERIFY2(addShotBtn && addShotDialog, "UI Components Missing: Add shot button or its dialog not found.");

    // Grab the new TextField inside the dialog
    QObject *dialogNameField = workspace->findChild<QObject*>(UI::StoryBoardWorkspaceView::DialogShotNameField);
    QVERIFY2(dialogNameField != nullptr, "UI Component Missing: 'dialogShotNameField' not found.");

    int initialCount = m_mockMgr.shotCount();

    // 1. Click the button to open the dialog
    QMetaObject::invokeMethod(addShotBtn, "clicked");
    QCoreApplication::processEvents();

    // 2. Set the text in the new dialog field
    dialogNameField->setProperty("text", QStringLiteral("Shot 02 Int Cockpit"));
    QCoreApplication::processEvents();

    // 3. Trigger the accept() slot to simulate clicking "Save"
    QMetaObject::invokeMethod(addShotDialog, "accept");
    QCoreApplication::processEvents();

    // 4. Verify the backend updated and the UI cleared
    QCOMPARE(m_mockMgr.shotCount(), initialCount + 1);
    QCOMPARE(dialogNameField->property("text").toString(), QStringLiteral(""));
}

void Test_StoryBoardWorkspaceView::testToolTipsConfigured()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &m_mockMgr), "Failed to load StoryBoardWorkspaceView harness");

    QObject *workspace = harness.workspace;
    QVERIFY(workspace != nullptr);

    auto getAttachedToolTip = [](QObject *item) -> QString {
        if (!item) return QString();
        QQmlContext *ctx = QQmlEngine::contextForObject(item);
        if (!ctx) return QString();
        QQmlExpression expr(ctx, qobject_cast<QQuickItem*>(item), QStringLiteral("ToolTip.text"));
        return expr.evaluate().toString();
    };


    QObject *addShotBtn = workspace->findChild<QObject*>(UI::StoryBoardWorkspaceView::AddShotBtn);
    QVERIFY2(addShotBtn != nullptr, "UI Components Missing: Add shot button was not found.");

    QCOMPARE(getAttachedToolTip(addShotBtn), QStringLiteral("Create a new shot card and append to sequence"));
}

QTEST_MAIN(Test_StoryBoardWorkspaceView)
#include "test_StoryBoardWorkspaceView.moc"
