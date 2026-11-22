/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_SettingsControlPanel.cpp
** Unit test suite for SettingsControlPanel QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"

class MockSettingsDriver : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString driverId READ driverId CONSTANT)

public:
    explicit MockSettingsDriver(const QString &id, QObject *parent = nullptr)
        : QObject(parent), m_id(id) {}

    QString driverId() const { return m_id; }

private:
    QString m_id;
};

class MockSettingsEngineRegistry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* activeScriptDriver READ activeScriptDriver NOTIFY activeDriversChanged)
    Q_PROPERTY(QObject* activeImageDriver  READ activeImageDriver  NOTIFY activeDriversChanged)
    Q_PROPERTY(QObject* activeVideoDriver  READ activeVideoDriver  NOTIFY activeDriversChanged)
    Q_PROPERTY(QObject* activeAudioDriver  READ activeAudioDriver  NOTIFY activeDriversChanged)

public:
    explicit MockSettingsEngineRegistry(bool includeLocal = true, QObject *parent = nullptr) : QObject(parent) {
        if (includeLocal) {
            m_drivers[QStringLiteral("llama_cpp")]  = new MockSettingsDriver(QStringLiteral("llama_cpp"), this);
            m_drivers[QStringLiteral("sd_image")]   = new MockSettingsDriver(QStringLiteral("sd_image"), this);
            m_drivers[QStringLiteral("sd_video")]   = new MockSettingsDriver(QStringLiteral("sd_video"), this);
            m_drivers[QStringLiteral("kokoro_tts")] = new MockSettingsDriver(QStringLiteral("kokoro_tts"), this);
        }
        m_drivers[QStringLiteral("gemini_bridge")] = new MockSettingsDriver(QStringLiteral("gemini_bridge"), this);
        m_drivers[QStringLiteral("gemini_image")]  = new MockSettingsDriver(QStringLiteral("gemini_image"), this);
        m_drivers[QStringLiteral("gemini_video")]  = new MockSettingsDriver(QStringLiteral("gemini_video"), this);
        m_drivers[QStringLiteral("gemini_audio")]  = new MockSettingsDriver(QStringLiteral("gemini_audio"), this);

        m_active[QStringLiteral("script")] = includeLocal ? QStringLiteral("llama_cpp")  : QStringLiteral("gemini_bridge");
        m_active[QStringLiteral("image")]  = includeLocal ? QStringLiteral("sd_image")   : QStringLiteral("gemini_image");
        m_active[QStringLiteral("video")]  = includeLocal ? QStringLiteral("sd_video")   : QStringLiteral("gemini_video");
        m_active[QStringLiteral("audio")]  = includeLocal ? QStringLiteral("kokoro_tts") : QStringLiteral("gemini_audio");
    }

    QObject* activeScriptDriver() const { return m_drivers.value(m_active.value(QStringLiteral("script")), nullptr); }
    QObject* activeImageDriver()  const { return m_drivers.value(m_active.value(QStringLiteral("image")), nullptr); }
    QObject* activeVideoDriver()  const { return m_drivers.value(m_active.value(QStringLiteral("video")), nullptr); }
    QObject* activeAudioDriver()  const { return m_drivers.value(m_active.value(QStringLiteral("audio")), nullptr); }

    Q_INVOKABLE QStringList availableDriversForAsset(const QString &asset) const {
        if (asset == QStringLiteral("script")) return { QStringLiteral("llama_cpp"), QStringLiteral("gemini_bridge") };
        if (asset == QStringLiteral("image"))  return { QStringLiteral("sd_image"),  QStringLiteral("gemini_image") };
        if (asset == QStringLiteral("video"))  return { QStringLiteral("sd_video"),  QStringLiteral("gemini_video") };
        if (asset == QStringLiteral("audio"))  return { QStringLiteral("kokoro_tts"), QStringLiteral("gemini_audio") };
        return {};
    }

    Q_INVOKABLE QString getActiveDriverId(const QString &asset) const {
        return m_active.value(asset);
    }

    Q_INVOKABLE QObject* getDriver(const QString &driverId) const {
        return m_drivers.value(driverId, nullptr);
    }

    Q_INVOKABLE void setActiveDriver(const QString &asset, const QString &driverId) {
        if (m_active.value(asset) != driverId) {
            m_active[asset] = driverId;
            emit activeDriversChanged();
        }
    }

signals:
    void activeDriversChanged();

private:
    QMap<QString, MockSettingsDriver*> m_drivers;
    QMap<QString, QString> m_active;
};

class Test_SettingsControlPanel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testPanelHeaderAndDefaults();
    void testModalityTabNavigation();
    void testActionButtonsAndToolTips();
    void testLocalControlsHideWhenRemoteDriverActive();
    void testRestOnlyModeHidesAllLocalControls();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *panel = nullptr;

        bool init(const QString &sessionDir, MockSettingsEngineRegistry *mockRegistry = nullptr) {
            Q_UNUSED(sessionDir);
            if (mockRegistry) {
                engine.rootContext()->setContextProperty(QStringLiteral("engineRegistry"), mockRegistry);
            }
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/settings"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 900\n"
                "    height: 700\n"
                "    visible: true\n"
                "    SettingsControlPanel {\n"
                "        id: settingsPanel\n"
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

            panel = window->findChild<QObject*>(UI::Settings::Root);
            return (panel != nullptr);
        }
    };
};

void Test_SettingsControlPanel::initTestCase()
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

void Test_SettingsControlPanel::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_SettingsControlPanel::testPanelHeaderAndDefaults()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *panel = harness.panel;
    QVERIFY(panel != nullptr);

    QObject *title = panel->findChild<QObject*>(UI::Settings::PanelTitle);
    QVERIFY2(title != nullptr, "UI Component Missing: 'panelTitleText' not found.");
    QObject *tabBar = panel->findChild<QObject*>(UI::Settings::TabBar);
    QVERIFY2(tabBar != nullptr, "UI Component Missing: 'settingsTabBar' not found.");

    QCOMPARE(title->property("text").toString(), QStringLiteral("⚙ Studio Configuration & Driver Engine Settings"));
    QCOMPARE(title->property("color").toString().toUpper(), QStringLiteral("#00F0FF"));
    QCOMPARE(tabBar->property("currentIndex").toInt(), 0);
}

void Test_SettingsControlPanel::testModalityTabNavigation()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *panel = harness.panel;
    QObject *tabBar = panel->findChild<QObject*>(UI::Settings::TabBar);
    QObject *stack = panel->findChild<QObject*>(UI::Settings::Stack);
    QVERIFY2(tabBar != nullptr, "UI Component Missing: 'settingsTabBar' not found.");
    QVERIFY2(stack != nullptr, "UI Component Missing: 'settingsStack' not found.");

    // Switch to Image Tab
    tabBar->setProperty("currentIndex", 1);
    QCoreApplication::processEvents();
    QCOMPARE(stack->property("currentIndex").toInt(), 1);

    // Switch to Video Tab
    tabBar->setProperty("currentIndex", 2);
    QCoreApplication::processEvents();
    QCOMPARE(stack->property("currentIndex").toInt(), 2);

    // Switch to Audio Tab
    tabBar->setProperty("currentIndex", 3);
    QCoreApplication::processEvents();
    QCOMPARE(stack->property("currentIndex").toInt(), 3);

    // Switch to General Tab
    tabBar->setProperty("currentIndex", 4);
    QCoreApplication::processEvents();
    QCOMPARE(stack->property("currentIndex").toInt(), 4);
}

void Test_SettingsControlPanel::testActionButtonsAndToolTips()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *panel = harness.panel;

    QObject *closeToolBtn = panel->findChild<QObject*>(UI::Settings::ClosePanelToolBtn);
    QVERIFY2(closeToolBtn != nullptr, "UI Component Missing: 'closePanelToolBtn' not found.");
    QVERIFY2(closeToolBtn->property("hovered").isValid(), "Close tool button missing hover state required for ToolTips.");

    QObject *resetBtn = panel->findChild<QObject*>(UI::Settings::ResetDefaultsBtn);
    QVERIFY2(resetBtn != nullptr, "UI Component Missing: 'resetDefaultsBtn' not found.");
    QCOMPARE(resetBtn->property("text").toString(), QStringLiteral("Reset Defaults"));

    QObject *closeBtn = panel->findChild<QObject*>(UI::Settings::CloseBtn);
    QVERIFY2(closeBtn != nullptr, "UI Component Missing: 'closeBtn' not found.");
    QCOMPARE(closeBtn->property("text").toString(), QStringLiteral("Close"));
}

void Test_SettingsControlPanel::testLocalControlsHideWhenRemoteDriverActive()
{
    MockSettingsEngineRegistry mockRegistry;
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &mockRegistry), "Failed to load SettingsControlPanel with mock registry.");

    QObject *panel = harness.panel;
    QVERIFY2(panel != nullptr, "UI Component Missing: 'settingsControlPanelRoot' not found.");

           // Open the Popup so child visibility properties reflect active layout state
    QMetaObject::invokeMethod(panel, "open");
    QCoreApplication::processEvents();

           // 1. Verify initial local state across all four modalities
    QVERIFY2(panel->property("isLocalScript").toBool(), "Expected isLocalScript=true for llama_cpp.");
    QVERIFY2(panel->property("isLocalImage").toBool(),  "Expected isLocalImage=true for sd_image.");
    QVERIFY2(panel->property("isLocalVideo").toBool(),  "Expected isLocalVideo=true for sd_video.");
    QVERIFY2(panel->property("isKokoroAudio").toBool(), "Expected isKokoroAudio=true for kokoro_tts.");
    QVERIFY2(!panel->property("isRemoteAudio").toBool(), "Expected isRemoteAudio=false for kokoro_tts.");

    QObject *rescanBtn       = panel->findChild<QObject*>(UI::Settings::RescanWeightsBtn);
    QObject *remoteScript    = panel->findChild<QObject*>(UI::Settings::RemoteScriptCard);
    QObject *remoteImage     = panel->findChild<QObject*>(UI::Settings::RemoteImageCard);
    QObject *remoteVideo     = panel->findChild<QObject*>(UI::Settings::RemoteVideoCard);
    QObject *remoteAudio     = panel->findChild<QObject*>(UI::Settings::RemoteAudioCard);
    QVERIFY2(rescanBtn && remoteScript && remoteImage && remoteVideo && remoteAudio,
             "UI Components Missing: Expected local/remote controls not found in SettingsControlPanel.");

    QVERIFY2(rescanBtn->property("visible").toBool(),     "Local rescanWeightsBtn should be visible for llama_cpp.");
    QVERIFY2(!remoteScript->property("visible").toBool(), "remoteScriptConfigCard should be hidden for llama_cpp.");

           // 2. Switch all four modalities to remote drivers
    mockRegistry.setActiveDriver(QStringLiteral("script"), QStringLiteral("gemini_bridge"));
    mockRegistry.setActiveDriver(QStringLiteral("image"),  QStringLiteral("gemini_image"));
    mockRegistry.setActiveDriver(QStringLiteral("video"),  QStringLiteral("gemini_video"));
    mockRegistry.setActiveDriver(QStringLiteral("audio"),  QStringLiteral("gemini_audio"));
    QCoreApplication::processEvents();

           // 3. Verify local flags flip off and remote cards take over
    QVERIFY2(!panel->property("isLocalScript").toBool(), "Local script controls failed to hide for gemini_bridge.");
    QVERIFY2(!panel->property("isLocalImage").toBool(),  "Local image controls failed to hide for gemini_image.");
    QVERIFY2(!panel->property("isLocalVideo").toBool(),  "Local video controls failed to hide for gemini_video.");
    QVERIFY2(panel->property("isRemoteAudio").toBool(),  "isRemoteAudio did not activate for gemini_audio.");

    QVERIFY2(!rescanBtn->property("visible").toBool(),   "Local rescanWeightsBtn remained visible for remote script driver.");
    QVERIFY2(remoteScript->property("visible").toBool(), "remoteScriptConfigCard did not become visible for gemini_bridge.");

    QMetaObject::invokeMethod(panel, "close");
    QCoreApplication::processEvents();
}

void Test_SettingsControlPanel::testRestOnlyModeHidesAllLocalControls()
{
    MockSettingsEngineRegistry restOnlyRegistry(/*includeLocal=*/false);
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &restOnlyRegistry), "Failed to load SettingsControlPanel in REST-only mode.");

    QObject *panel = harness.panel;
    QMetaObject::invokeMethod(panel, "open");
    QCoreApplication::processEvents();

    QVERIFY2(!panel->property("isLocalScript").toBool(), "isLocalScript should be false in REST-only mode.");
    QVERIFY2(!panel->property("isLocalImage").toBool(),  "isLocalImage should be false in REST-only mode.");
    QVERIFY2(!panel->property("isLocalVideo").toBool(),  "isLocalVideo should be false in REST-only mode.");
    QVERIFY2(panel->property("isRemoteAudio").toBool(),  "isRemoteAudio should be true in REST-only mode.");

    QObject *rescanBtn    = panel->findChild<QObject*>(UI::Settings::RescanWeightsBtn);
    QObject *remoteScript = panel->findChild<QObject*>(UI::Settings::RemoteScriptCard);
    QVERIFY2(rescanBtn && remoteScript, "Expected script controls missing in REST-only mode.");

    QVERIFY2(!rescanBtn->property("visible").toBool(),   "Local rescanWeightsBtn should be hidden in REST-only mode.");
    QVERIFY2(remoteScript->property("visible").toBool(), "remoteScriptConfigCard should be visible in REST-only mode.");
    QCOMPARE(remoteScript->property("driverId").toString(), QStringLiteral("gemini_bridge"));

    QMetaObject::invokeMethod(panel, "close");
    QCoreApplication::processEvents();
}

QTEST_MAIN(Test_SettingsControlPanel)
#include "test_SettingsControlPanel.moc"

