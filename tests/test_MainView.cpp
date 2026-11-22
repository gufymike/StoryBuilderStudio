/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_MainView.cpp
** Unit test suite verifying top-level Main.qml layout, geometry, and status banners.
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

#include "storydocument.h"
#include "scriptsdirectorymodel.h"
#include "imagedirectorymodel.h"
#include "audiodirectorymodel.h"
#include "audiowaveformmodel.h"
#include "videodirectorymodel.h"
#include "ui_constants.h"

class MockMainEngineRegistry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool hasLocalPlugins READ hasLocalPlugins NOTIFY pluginsChanged)
    Q_PROPERTY(QVariantList pluginStatuses READ pluginStatuses NOTIFY pluginsChanged)

public:
    explicit MockMainEngineRegistry(bool hasLocal, QObject *parent = nullptr)
        : QObject(parent), m_hasLocal(hasLocal) {}

    bool hasLocalPlugins() const { return m_hasLocal; }

    QVariantList pluginStatuses() const {
        QVariantList list;
        const QStringList ids = {
            QStringLiteral("LocalLlmEngine"),
            QStringLiteral("KokoroTtsEngine"),
            QStringLiteral("LocalAudioEngine"),
            QStringLiteral("LocalSdImagePlugin"),
            QStringLiteral("LocalSdVideoPlugin")
        };
        for (const QString &id : ids) {
            QVariantMap m;
            m[QStringLiteral("id")] = id;
            m[QStringLiteral("label")] = id;
            m[QStringLiteral("available")] = m_hasLocal;
            list.append(m);
        }
        return list;
    }

signals:
    void pluginsChanged();

private:
    bool m_hasLocal;
};

class MockMainProjectManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isProjectOpen READ isProjectOpen NOTIFY isProjectOpenChanged)
    Q_PROPERTY(QString projectName READ projectName NOTIFY projectNameChanged)
    Q_PROPERTY(QString currentProjectPath READ currentProjectPath NOTIFY currentProjectPathChanged)

public:
    explicit MockMainProjectManager(QObject *parent = nullptr)
        : QObject(parent), m_isOpen(true), m_name(QStringLiteral("CyberOdyssey_Ep01")),
        m_path(QStringLiteral("C:/test_project")) {}

    bool isProjectOpen() const { return m_isOpen; }
    QString projectName() const { return m_name; }
    QString currentProjectPath() const { return m_path; }

    Q_INVOKABLE void closeProject() {
        m_isOpen = false;
        emit isProjectOpenChanged();
    }

signals:
    void isProjectOpenChanged();
    void projectNameChanged();
    void currentProjectPathChanged();

private:
    bool m_isOpen;
    QString m_name;
    QString m_path;
};

class MockMainSeriesContainer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString rootDirectory READ rootDirectory NOTIFY rootDirectoryChanged)

public:
    explicit MockMainSeriesContainer(QObject *parent = nullptr)
        : QObject(parent), m_dir(QStringLiteral("C:/test_series")) {}

    QString rootDirectory() const { return m_dir; }
    void setRootDirectory(const QString &p) { m_dir = p; emit rootDirectoryChanged(); }
    Q_INVOKABLE bool loadFromDirectory(const QString &p) { m_dir = p; emit rootDirectoryChanged(); return true; }

signals:
    void rootDirectoryChanged();
    void manifestError(const QString &msg);

private:
    QString m_dir;
};

class MockMainSettingsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isFirstRun READ isFirstRun WRITE setIsFirstRun NOTIFY isFirstRunChanged)

public:
    explicit MockMainSettingsManager(QObject *parent = nullptr)
        : QObject(parent), m_isFirstRun(false) {}

    bool isFirstRun() const { return m_isFirstRun; }
    void setIsFirstRun(bool val) {
        if (m_isFirstRun != val) {
            m_isFirstRun = val;
            emit isFirstRunChanged();
        }
    }

    Q_INVOKABLE void addToRecentFiles(const QString &) {}

signals:
    void isFirstRunChanged();

private:
    bool m_isFirstRun;
};

class Test_MainView : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testWindowDimensionsAndMinimums();
    void testTitleReflectsActiveProject();
    void testUrlToLocalPathFormatting();
    void testFirstRunWizardTriggeredOnStartup(); // <-- New Startup Modal Test
    void testAboutDialogDisplaysPluginStatuses();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;

        bool init(const QString &sessionDir, MockMainProjectManager *proj,
                  MockMainSeriesContainer *series, MockMainSettingsManager *settings,
                  QObject *registry = nullptr) {
            Q_UNUSED(sessionDir);
            engine.rootContext()->setContextProperty(QStringLiteral("projectManager"), proj);
            engine.rootContext()->setContextProperty(QStringLiteral("seriesProjectContainer"), series);
            engine.rootContext()->setContextProperty(QStringLiteral("settingsManager"), settings);
            if (registry) {
                engine.rootContext()->setContextProperty(QStringLiteral("engineRegistry"), registry);
            }
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/text"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/image"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/audio"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/video"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/storyboard"));

            const QString mainQmlPath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/Main.qml");
            engine.load(QUrl::fromLocalFile(mainQmlPath));

            if (engine.rootObjects().isEmpty()) {
                return false;
            }

            window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            return (window != nullptr);
        }
    };
};

void Test_MainView::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    qmlRegisterType<StoryDocument>("StoryBuilderStudio", 1, 0, "StoryDocument");
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

void Test_MainView::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_MainView::testWindowDimensionsAndMinimums()
{
    MockMainProjectManager mockProj;
    MockMainSeriesContainer mockSeries;
    MockMainSettingsManager mockSettings;

    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &mockProj, &mockSeries, &mockSettings), "Failed to load Main.qml");

    QQuickWindow *win = harness.window;
    QVERIFY(win != nullptr);
    QVERIFY(win->width() == 1150);
    QVERIFY(win->minimumWidth() >= 1084);
    QVERIFY(win->minimumHeight() >= 800);
}

void Test_MainView::testTitleReflectsActiveProject()
{
    MockMainProjectManager mockProj;
    MockMainSeriesContainer mockSeries;
    MockMainSettingsManager mockSettings;

    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &mockProj, &mockSeries, &mockSettings), "Failed to load Main.qml");

    QQuickWindow *win = harness.window;
    QVERIFY(win != nullptr);

    QCOMPARE(win->title(), QStringLiteral("Story Builder Studio - CyberOdyssey_Ep01"));

    mockProj.closeProject();
    QCoreApplication::processEvents();

    QCOMPARE(win->title(), QStringLiteral("Story Builder Studio - No Project Open"));
}

void Test_MainView::testUrlToLocalPathFormatting()
{
    MockMainProjectManager mockProj;
    MockMainSeriesContainer mockSeries;
    MockMainSettingsManager mockSettings;

    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &mockProj, &mockSeries, &mockSettings), "Failed to load Main.qml");

    QQuickWindow *win = harness.window;
    QVERIFY(win != nullptr);

    QVariant converted;
#ifdef Q_OS_WIN
    QMetaObject::invokeMethod(win, "urlToLocalPath",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, converted),
                              Q_ARG(QVariant, QVariant(QStringLiteral("file:///C:/Workspace/script.md"))));
    QCOMPARE(converted.toString(), QStringLiteral("C:/Workspace/script.md"));
#else
    QMetaObject::invokeMethod(win, "urlToLocalPath",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, converted),
                              Q_ARG(QVariant, QVariant(QStringLiteral("file:///home/user/Workspace/script.md"))));
    QCOMPARE(converted.toString(), QStringLiteral("/home/user/Workspace/script.md"));
#endif
}

void Test_MainView::testFirstRunWizardTriggeredOnStartup()
{
    MockMainProjectManager mockProj;
    MockMainSeriesContainer mockSeries;
    MockMainSettingsManager mockSettings;

    // Simulate clean first run state
    mockSettings.setIsFirstRun(true);

    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &mockProj, &mockSeries, &mockSettings), "Failed to load Main.qml");

    QQuickWindow *win = harness.window;
    QVERIFY(win != nullptr);

    // Locate the firstRunWizard modal instance
    QObject *firstRunWizard = win->findChild<QObject*>(UI::MainView::FirstRunWizard);
    QVERIFY2(firstRunWizard != nullptr, "UI Component Missing: 'firstRunWizard' not found in Main.qml object tree.");

    // Flush the event queue to allow Qt.callLater() to execute
    QTest::qWait(50);

    // Verify modal state
    QVERIFY2(firstRunWizard->property("opened").toBool() == true, "State Routing Failure: FirstRunWizard did not logically open on startup.");
    QVERIFY2(firstRunWizard->property("visible").toBool() == true, "UI Rendering Failure: FirstRunWizard is not visually rendered despite being triggered.");;
}

void Test_MainView::testAboutDialogDisplaysPluginStatuses()
{
    MockMainProjectManager mockProj;
    MockMainSeriesContainer mockSeries;
    MockMainSettingsManager mockSettings;

           // 1. REST-only mode (no local plugins installed)
    {
        MockMainEngineRegistry restOnlyRegistry(/*hasLocal=*/false);
        TestHarness harness;
        QVERIFY2(harness.init(m_testSessionDir, &mockProj, &mockSeries, &mockSettings, &restOnlyRegistry),
                 "Failed to load Main.qml in REST-only mode");

        QObject *aboutDialog = harness.window->findChild<QObject*>(QStringLiteral("aboutDialog"));
        QVERIFY2(aboutDialog != nullptr, "UI Component Missing: 'aboutDialog' not found.");

        QObject *modeText = aboutDialog->findChild<QObject*>(QStringLiteral("aboutPluginModeText"));
        QObject *repeater = aboutDialog->findChild<QObject*>(QStringLiteral("aboutPluginRepeater"));
        QVERIFY2(modeText != nullptr, "UI Component Missing: 'aboutPluginModeText' not found.");
        QVERIFY2(repeater != nullptr, "UI Component Missing: 'aboutPluginRepeater' not found.");

        QCOMPARE(modeText->property("text").toString(), QStringLiteral("Remote / REST Only"));
        QCOMPARE(repeater->property("count").toInt(), 5);
    }

           // 2. Local + Remote mode
    {
        MockMainEngineRegistry fullRegistry(/*hasLocal=*/true);
        TestHarness harness;
        QVERIFY2(harness.init(m_testSessionDir, &mockProj, &mockSeries, &mockSettings, &fullRegistry),
                 "Failed to load Main.qml in Local + Remote mode");

        QObject *aboutDialog = harness.window->findChild<QObject*>(QStringLiteral("aboutDialog"));
        QObject *modeText    = aboutDialog->findChild<QObject*>(QStringLiteral("aboutPluginModeText"));
        QVERIFY2(modeText != nullptr, "UI Component Missing: 'aboutPluginModeText' not found.");

        QCOMPARE(modeText->property("text").toString(), QStringLiteral("Local + Remote Mode"));
    }
}

QTEST_MAIN(Test_MainView)
#include "test_MainView.moc"
