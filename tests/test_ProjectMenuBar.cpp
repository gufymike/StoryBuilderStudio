// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>
#include <QtGui>
#include <QQuickItem>

#include "ui_constants.h"

class Test_ProjectMenuBar : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testMenuBarInitialization();
    void testStateGatingGatedActions();
    void testFileSignalsEmission();
    void testAboutDialogContents();
    void testPromptHistoryActionTrigger();
    void testMnemonicFormatting();
    void testMenuOpenActivation();
    void testMouseClickOpensSubMenu();
    void testMenuBarItemHoverStates();
    void testProjectMenuEnabledStates();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *menuBar = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/promptbuilder"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 800\n"
                "    height: 600\n"
                "    visible: true\n"
                "    menuBar: ProjectMenuBar {\n"
                "        id: pMenuBar\n"
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

            menuBar = window->findChild<QObject*>(UI::ProjectMenuBar::Root);
            return (menuBar != nullptr);
        }
    };
};

void Test_ProjectMenuBar::initTestCase()
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

void Test_ProjectMenuBar::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_ProjectMenuBar::testMenuBarInitialization()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *menuBar = harness.menuBar;
    QVERIFY(menuBar != nullptr);

    QCOMPARE(menuBar->property("isSeriesOpen").toBool(), false);
    QCOMPARE(menuBar->property("isEpisodeOpen").toBool(), false);
    QCOMPARE(menuBar->property("isDocumentLoaded").toBool(), false);

    QObject *fileMenu = menuBar->findChild<QObject*>(UI::ProjectMenuBar::FileMenu);
    QVERIFY(fileMenu != nullptr);

    QObject *projectMenu = menuBar->findChild<QObject*>(UI::ProjectMenuBar::ProjectMenu);
    QVERIFY(projectMenu != nullptr);
    QCOMPARE(projectMenu->property("enabled").toBool(), false);
}

void Test_ProjectMenuBar::testStateGatingGatedActions()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *menuBar = harness.menuBar;

    QObject *projectMenu = menuBar->findChild<QObject*>(UI::ProjectMenuBar::ProjectMenu);
    QVERIFY2(projectMenu != nullptr, "UI Component Missing: 'projectMenu' not found.");
    QObject *closeSeriesAction = menuBar->findChild<QObject*>(UI::ProjectMenuBar::CloseSeriesAct);
    QVERIFY2(closeSeriesAction != nullptr, "UI Component Missing: 'closeSeriesAction' not found.");
    QObject *closeEpisodeAction = menuBar->findChild<QObject*>(UI::ProjectMenuBar::CloseEpisodeAct);
    QVERIFY2(closeEpisodeAction != nullptr, "UI Component Missing: 'closeEpisodeAction' not found.");

    QVERIFY(projectMenu && closeSeriesAction && closeEpisodeAction);

    // Initial state: series closed
    QCOMPARE(closeSeriesAction->property("enabled").toBool(), false);
    QCOMPARE(projectMenu->property("enabled").toBool(), false);
    QCOMPARE(closeEpisodeAction->property("enabled").toBool(), false);

    // Open Series
    menuBar->setProperty("isSeriesOpen", true);
    QCoreApplication::processEvents();

    QCOMPARE(closeSeriesAction->property("enabled").toBool(), true);
    QCOMPARE(projectMenu->property("enabled").toBool(), true);
    QCOMPARE(closeEpisodeAction->property("enabled").toBool(), false);

    // Open Episode
    menuBar->setProperty("isEpisodeOpen", true);
    QCoreApplication::processEvents();

    QCOMPARE(closeEpisodeAction->property("enabled").toBool(), true);
}

void Test_ProjectMenuBar::testFileSignalsEmission()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *menuBar = harness.menuBar;
    QSignalSpy newSeriesSpy(menuBar, SIGNAL(newSeriesTriggered()));
    QSignalSpy openSeriesSpy(menuBar, SIGNAL(openSeriesTriggered()));
    QSignalSpy exitSpy(menuBar, SIGNAL(exitTriggered()));

    QObject *newSeriesAction = menuBar->findChild<QObject*>(UI::ProjectMenuBar::NewSeriesAct);
    QVERIFY2(newSeriesAction != nullptr, "UI Component Missing: 'newSeriesAction' not found.");
    QMetaObject::invokeMethod(newSeriesAction, "trigger");
    QCOMPARE(newSeriesSpy.count(), 1);

    QObject *openSeriesAction = menuBar->findChild<QObject*>(UI::ProjectMenuBar::OpenSeriesAct);
    QVERIFY2(openSeriesAction != nullptr, "UI Component Missing: 'openSeriesAction' not found.");
    QMetaObject::invokeMethod(openSeriesAction, "trigger");
    QCOMPARE(openSeriesSpy.count(), 1);

    QObject *exitAction = menuBar->findChild<QObject*>(UI::ProjectMenuBar::ExitAct);
    QVERIFY2(exitAction != nullptr, "UI Component Missing: 'exitAction' not found.");
    QMetaObject::invokeMethod(exitAction, "trigger");
    QCOMPARE(exitSpy.count(), 1);
}

void Test_ProjectMenuBar::testAboutDialogContents()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *menuBar = harness.menuBar;
    QObject *aboutAction = menuBar->findChild<QObject*>(UI::ProjectMenuBar::AboutAct);
    QVERIFY2(aboutAction != nullptr, "Could not find aboutAction object");

    QSignalSpy spy(menuBar, SIGNAL(aboutTriggered()));
    QMetaObject::invokeMethod(aboutAction, "trigger");

    QCOMPARE(spy.count(), 1);
}

void Test_ProjectMenuBar::testPromptHistoryActionTrigger()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *menuBar = harness.menuBar;
    QObject *historyAction = menuBar->findChild<QObject*>(UI::ProjectMenuBar::PromptHistoryAct);
    QVERIFY2(historyAction != nullptr, "Could not find promptHistoryAction object");

    // Initial state: Episode closed -> action disabled
    QCOMPARE(historyAction->property("enabled").toBool(), false);

    // Open episode -> action enabled
    menuBar->setProperty("isEpisodeOpen", true);
    QCoreApplication::processEvents();
    QCOMPARE(historyAction->property("enabled").toBool(), true);

    QSignalSpy spy(menuBar, SIGNAL(promptHistoryTriggered()));
    QMetaObject::invokeMethod(historyAction, "trigger");

    QCOMPARE(spy.count(), 1);
}

void Test_ProjectMenuBar::testMnemonicFormatting()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to initialize MenuBar harness");

    QObject *menuBar = harness.menuBar;
    QVERIFY(menuBar != nullptr);

    QVariant resultFile;
    QMetaObject::invokeMethod(menuBar, "formatMnemonic",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, resultFile),
                              Q_ARG(QVariant, QVariant(QStringLiteral("&File"))));
    QCOMPARE(resultFile.toString(), QStringLiteral("<u>F</u>ile"));

    QVariant resultProject;
    QMetaObject::invokeMethod(menuBar, "formatMnemonic",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, resultProject),
                              Q_ARG(QVariant, QVariant(QStringLiteral("&Project"))));
    QCOMPARE(resultProject.toString(), QStringLiteral("<u>P</u>roject"));

    QVariant resultNone;
    QMetaObject::invokeMethod(menuBar, "formatMnemonic",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, resultNone),
                              Q_ARG(QVariant, QVariant(QStringLiteral("Edit"))));
    QCOMPARE(resultNone.toString(), QStringLiteral("Edit"));
}


void Test_ProjectMenuBar::testMenuOpenActivation()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to initialize MenuBar harness");

    QObject *menuBar = harness.menuBar;
    QVERIFY(menuBar != nullptr);

    QObject *fileMenu = menuBar->findChild<QObject*>(UI::ProjectMenuBar::FileMenu);
    if (!fileMenu) {
        QVariant menusVar = menuBar->property("menus");
        if (menusVar.isValid()) {
            QList<QObject*> menus = menusVar.value<QList<QObject*>>();
            if (!menus.isEmpty()) {
                fileMenu = menus.first();
            }
        }
    }

    QVERIFY2(fileMenu != nullptr, "Could not locate fileMenu instance");
    QCOMPARE(fileMenu->property("visible").toBool(), false);

    QMetaObject::invokeMethod(fileMenu, "open");
    QTest::qWait(50);
    QCOMPARE(fileMenu->property("visible").toBool(), true);

    QMetaObject::invokeMethod(fileMenu, "close");
    QTest::qWait(50);
    QCOMPARE(fileMenu->property("visible").toBool(), false);
}

void Test_ProjectMenuBar::testMouseClickOpensSubMenu()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to initialize MenuBar harness");

    QQuickWindow *win = harness.window;
    QObject *menuBar = harness.menuBar;
    QVERIFY(win != nullptr && menuBar != nullptr);

    QObject *fileMenu = menuBar->findChild<QObject*>(UI::ProjectMenuBar::FileMenu);
    QVERIFY2(fileMenu != nullptr, "Could not locate fileMenu");
    QCOMPARE(fileMenu->property("visible").toBool(), false);

    // Locate the first visual MenuBarItem inside the MenuBar
    QQuickItem *menuBarItem = nullptr;
    const auto children = qobject_cast<QQuickItem*>(menuBar)->findChildren<QQuickItem*>();
    for (QQuickItem *child : children) {
        if (child->inherits("QQuickMenuBarItem")) {
            menuBarItem = child;
            break;
        }
    }
    QVERIFY2(menuBarItem != nullptr, "Could not find QQuickMenuBarItem delegate in visual tree");

    // Click the center of the MenuBarItem
    const QPoint clickPos = menuBarItem->mapToScene(QPointF(menuBarItem->width() / 2.0, menuBarItem->height() / 2.0)).toPoint();
    QTest::mouseClick(win, Qt::LeftButton, Qt::NoModifier, clickPos);
    QTest::qWait(100);

    // Verify clicking the delegate opened the attached Menu
    QCOMPARE(fileMenu->property("visible").toBool(), true);

    // Close menu to reset state
    QMetaObject::invokeMethod(fileMenu, "close");
    QTest::qWait(50);
    QCOMPARE(fileMenu->property("visible").toBool(), false);
}

void Test_ProjectMenuBar::testMenuBarItemHoverStates()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to initialize MenuBar harness");

    QQuickWindow *win = harness.window;
    QObject *menuBar = harness.menuBar;
    QVERIFY(win != nullptr && menuBar != nullptr);

    win->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(win));

    QQuickItem *menuBarItem = nullptr;
    const auto children = qobject_cast<QQuickItem*>(menuBar)->findChildren<QQuickItem*>();
    for (QQuickItem *child : children) {
        if (child->inherits("QQuickMenuBarItem")) {
            menuBarItem = child;
            break;
        }
    }
    QVERIFY2(menuBarItem != nullptr, "Could not find QQuickMenuBarItem delegate");

    // Move mouse well off-screen outside the window bounds to ensure no hover
    QTest::mouseMove(win, QPoint(-50, -50));
    QCoreApplication::processEvents();
    QTRY_COMPARE(menuBarItem->property("hovered").toBool(), false);

    // Map the center of the menu bar item to window scene coordinates
    QPointF itemCenterLocal(menuBarItem->width() / 2.0, menuBarItem->height() / 2.0);
    QPoint hoverPos = menuBarItem->mapToScene(itemCenterLocal).toPoint();

    // Move mouse onto the item delegate
    QTest::mouseMove(win, hoverPos);
    QCoreApplication::processEvents();
    QTRY_COMPARE(menuBarItem->property("hovered").toBool(), true);

    // Move back out to verify unhover
    QTest::mouseMove(win, QPoint(-50, -50));
    QCoreApplication::processEvents();
    QTRY_COMPARE(menuBarItem->property("hovered").toBool(), false);
}

void Test_ProjectMenuBar::testProjectMenuEnabledStates()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to initialize MenuBar harness");

    QObject *menuBar = harness.menuBar;
    QVERIFY(menuBar != nullptr);

    menuBar->setProperty("isSeriesOpen", false);
    menuBar->setProperty("isEpisodeOpen", false);
    menuBar->setProperty("isDocumentLoaded", false);

    QCOMPARE(menuBar->property("isSeriesOpen").toBool(), false);
    QCOMPARE(menuBar->property("isEpisodeOpen").toBool(), false);
    QCOMPARE(menuBar->property("isDocumentLoaded").toBool(), false);

    menuBar->setProperty("isSeriesOpen", true);
    menuBar->setProperty("isEpisodeOpen", true);
    menuBar->setProperty("isDocumentLoaded", true);

    QCOMPARE(menuBar->property("isSeriesOpen").toBool(), true);
    QCOMPARE(menuBar->property("isEpisodeOpen").toBool(), true);
    QCOMPARE(menuBar->property("isDocumentLoaded").toBool(), true);
}


QTEST_MAIN(Test_ProjectMenuBar)
#include "test_ProjectMenuBar.moc"
