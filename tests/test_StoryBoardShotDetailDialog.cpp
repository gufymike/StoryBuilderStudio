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
#include "ui_constants.h"

class MockStoryboardManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList shots READ shots NOTIFY shotsChanged)
    Q_PROPERTY(QString episodePath READ episodePath CONSTANT)

public:
    explicit MockStoryboardManager(QObject *parent = nullptr) : QObject(parent) {
        m_episodePath = QStringLiteral("C:/test_project/episodes/01");

        QVariantMap shot;
        shot[QStringLiteral("name")] = QStringLiteral("Shot 01 - Intro Orbit");
        shot[QStringLiteral("key_image")] = QStringLiteral("C:/test_project/images/key_01.png");
        shot[QStringLiteral("final_shot")] = QStringLiteral("C:/test_project/videos/final_01.mp4");
        shot[QStringLiteral("script_file")] = QStringLiteral("intro_scene.md");
        shot[QStringLiteral("modified_at")] = QStringLiteral("2026-11-08 10:00");
        shot[QStringLiteral("images")] = QVariantList{ QStringLiteral("C:/test_project/images/key_01.png") };
        shot[QStringLiteral("audio")] = QVariantList{ QStringLiteral("C:/test_project/audio/dialogue_01.wav") };
        shot[QStringLiteral("videos")] = QVariantList{ QStringLiteral("C:/test_project/videos/final_01.mp4") };

        m_shots.append(shot);
    }

    QVariantList shots() const { return m_shots; }
    QString episodePath() const { return m_episodePath; }

    Q_INVOKABLE void renameShot(int index, const QString &newName) {
        if (index >= 0 && index < m_shots.size()) {
            QVariantMap shot = m_shots[index].toMap();
            shot[QStringLiteral("name")] = newName;
            m_shots[index] = shot;
            emit shotsChanged();
        }
    }

    Q_INVOKABLE void setShotKeyImage(int index, const QString &path) { Q_UNUSED(index); Q_UNUSED(path); }
    Q_INVOKABLE void setShotFinalCut(int index, const QString &path) { Q_UNUSED(index); Q_UNUSED(path); }
    Q_INVOKABLE void removeAssetFromShot(int index, const QString &category, const QString &path) {
        Q_UNUSED(index); Q_UNUSED(category); Q_UNUSED(path);
    }
    Q_INVOKABLE void addAssetToShot(int index, const QString &category, const QString &path) {
        Q_UNUSED(index); Q_UNUSED(category); Q_UNUSED(path);
    }

signals:
    void shotsChanged();

private:
    QVariantList m_shots;
    QString m_episodePath;
};

class Test_StoryBoardShotDetailDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testOpenForShotAndDisplaysMetadata();
    void testRenameShot();
    void testTabSwitching();
    void testToolTipsConfigured();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *dialogWindow = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/storyboard"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/image"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/audio"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/video"));

            const QString qmlPath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/storyboard/StoryBoardShotDetailDialog.qml");
            engine.load(QUrl::fromLocalFile(qmlPath));

            if (engine.rootObjects().isEmpty()) {
                return false;
            }

            dialogWindow = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            return (dialogWindow != nullptr);
        }
    };
};

void Test_StoryBoardShotDetailDialog::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    qmlRegisterType<ImageDirectoryModel>("StoryBuilderStudio", 1, 0, "ImageDirectoryModel");
    qmlRegisterType<AudioDirectoryModel>("StoryBuilderStudio", 1, 0, "AudioDirectoryModel");
    qmlRegisterType<VideoDirectoryModel>("StoryBuilderStudio", 1, 0, "VideoDirectoryModel");
    qmlRegisterType<ScriptsDirectoryModel>("StoryBuilderStudio", 1, 0, "ScriptsDirectoryModel");

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::cleanPath(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_StoryBoardShotDetailDialog::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_StoryBoardShotDetailDialog::testOpenForShotAndDisplaysMetadata()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardShotDetailDialog harness");

    QQuickWindow *win = harness.dialogWindow;
    QVERIFY(win != nullptr);

    MockStoryboardManager mockMgr;
    QMetaObject::invokeMethod(win, "openForShot",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant::fromValue(&mockMgr)),
                              Q_ARG(QVariant, QVariant(0)),
                              Q_ARG(QVariant, QVariant()));
    QCoreApplication::processEvents();

    QObject *nameField = win->findChild<QObject*>(UI::StoryBoardShotDetailDialog::ShotNameField);
    QVERIFY2(nameField != nullptr, "UI Component Missing: 'shotNameField' not found.");

    QObject *scriptLink = win->findChild<QObject*>(UI::StoryBoardShotDetailDialog::ScriptLinkDisplay);
    QVERIFY2(scriptLink != nullptr, "UI Component Missing: 'scriptLinkDisplay' not found.");

    QCOMPARE(nameField->property("text").toString(), QStringLiteral("Shot 01 - Intro Orbit"));
    QCOMPARE(scriptLink->property("text").toString(), QStringLiteral("Script: intro_scene.md"));

    win->close();
}

void Test_StoryBoardShotDetailDialog::testRenameShot()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardShotDetailDialog harness");

    QQuickWindow *win = harness.dialogWindow;
    MockStoryboardManager mockMgr;

    QMetaObject::invokeMethod(win, "openForShot",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant::fromValue(&mockMgr)),
                              Q_ARG(QVariant, QVariant(0)),
                              Q_ARG(QVariant, QVariant()));
    QCoreApplication::processEvents();

    QObject *nameField = win->findChild<QObject*>(UI::StoryBoardShotDetailDialog::ShotNameField);
    QVERIFY2(nameField != nullptr, "UI Component Missing: 'shotNameField' not found.");
    QObject *renameBtn = win->findChild<QObject*>(UI::StoryBoardShotDetailDialog::RenameBtn);
    QVERIFY2(renameBtn != nullptr, "UI Component Missing: 'renameBtn' not found.");

    nameField->setProperty("text", QStringLiteral("Shot 01 - Orbit Descent"));
    QMetaObject::invokeMethod(renameBtn, "clicked");
    QCoreApplication::processEvents();

    QCOMPARE(mockMgr.shots().at(0).toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("Shot 01 - Orbit Descent"));

    win->close();
}

void Test_StoryBoardShotDetailDialog::testTabSwitching()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardShotDetailDialog harness");

    QQuickWindow *win = harness.dialogWindow;
    QObject *assetStack = win->findChild<QObject*>(UI::StoryBoardShotDetailDialog::AssetStack);
    QObject *assetTabBar = win->findChild<QObject*>(UI::StoryBoardShotDetailDialog::AssetTabBar);
    QVERIFY2(assetStack && assetTabBar, "UI Components Missing: Asset Stack or Asset Tab Bar not found.");


    // Switch to Scripts Tab (index 0)
    assetTabBar->setProperty("currentIndex", 0);
    QCoreApplication::processEvents();
    QCOMPARE(assetStack->property("currentIndex").toInt(), 0);
    // Initial Image Tab (index 1)
    assetTabBar->setProperty("currentIndex", 1);
    QCoreApplication::processEvents();
    QCOMPARE(assetStack->property("currentIndex").toInt(), 1);

    // Switch to Audio Tab (index 2)
    assetTabBar->setProperty("currentIndex", 2);
    QCoreApplication::processEvents();
    QCOMPARE(assetStack->property("currentIndex").toInt(), 2);

    // Switch to Video Tab (index 3)
    assetTabBar->setProperty("currentIndex", 3);
    QCoreApplication::processEvents();
    QCOMPARE(assetStack->property("currentIndex").toInt(), 3);

    win->close();
}

void Test_StoryBoardShotDetailDialog::testToolTipsConfigured()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardShotDetailDialog harness");

    QQuickWindow *win = harness.dialogWindow;
    QObject *renameBtn = win->findChild<QObject*>(UI::StoryBoardShotDetailDialog::RenameBtn);
    QVERIFY2(renameBtn != nullptr, "UI Component Missing: 'renameBtn' not found.");

    QQmlContext *ctx = QQmlEngine::contextForObject(renameBtn);
    QVERIFY(ctx != nullptr);
    QQmlExpression expr(ctx, qobject_cast<QQuickItem*>(renameBtn), QStringLiteral("ToolTip.text"));
    QCOMPARE(expr.evaluate().toString(), QStringLiteral("Update the name of this shot"));

    QObject *closeBtn = win->findChild<QObject*>(UI::StoryBoardShotDetailDialog::CloseDialogBtn);
    QVERIFY2(closeBtn != nullptr, "UI Component Missing: 'closeBtn' not found.");
    QQmlExpression closeExpr(ctx, qobject_cast<QQuickItem*>(closeBtn), QStringLiteral("ToolTip.text"));
    QCOMPARE(closeExpr.evaluate().toString(), QStringLiteral("Dismiss shot inspector"));

    win->close();
}

QTEST_MAIN(Test_StoryBoardShotDetailDialog)
#include "test_StoryBoardShotDetailDialog.moc"
