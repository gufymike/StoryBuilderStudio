// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

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

class Test_AssetContextMenu : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testMenuInitializationAndEmptyDefaults();
    void testStoryboardCategoryMapping();
    void testDynamicShotsAndToolsInstantiation();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *menu = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 600\n"
                "    height: 400\n"
                "    visible: true\n"
                "    AssetContextMenu {\n"
                "        id: contextMenu\n"
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

            menu = window->findChild<QObject*>(UI::AssetContextMenu::Root);
            return (menu != nullptr);
        }
    };
};

void Test_AssetContextMenu::initTestCase()
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

void Test_AssetContextMenu::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_AssetContextMenu::testMenuInitializationAndEmptyDefaults()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load AssetContextMenu harness");

    QObject *menu = harness.menu;
    QVERIFY(menu != nullptr);

    QObject *storyboardSubMenu = menu->findChild<QObject*>(UI::AssetContextMenu::StoryboardSubMenu);
    QVERIFY2(storyboardSubMenu != nullptr, "UI Component Missing: 'storyboardSubMenu' not found.");
    QVERIFY2(!storyboardSubMenu->property("enabled").toBool(), "State Failure: Storyboard submenu enabled despite empty active project.");

    QObject *toolsSubMenu = menu->findChild<QObject*>(UI::AssetContextMenu::ExternalToolsSubMenu);
    QVERIFY2(toolsSubMenu != nullptr, "UI Component Missing: 'externalToolsSubMenu' not found.");
    QVERIFY2(!toolsSubMenu->property("enabled").toBool(), "State Failure: Tools submenu enabled despite empty tools registry.");
}

void Test_AssetContextMenu::testStoryboardCategoryMapping()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load AssetContextMenu harness");

    QObject *menu = harness.menu;

    menu->setProperty("assetType", QStringLiteral("image"));
    QCOMPARE(menu->property("storyboardCategory").toString(), QStringLiteral("images"));

    menu->setProperty("assetType", QStringLiteral("video"));
    QCOMPARE(menu->property("storyboardCategory").toString(), QStringLiteral("videos"));

    menu->setProperty("assetType", QStringLiteral("audio"));
    QCOMPARE(menu->property("storyboardCategory").toString(), QStringLiteral("audio"));
}

void Test_AssetContextMenu::testDynamicShotsAndToolsInstantiation()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load AssetContextMenu harness");

    QObject *menu = harness.menu;

    // Inject mock shots
    QVariantList shotsList;
    QVariantMap shot1;
    shot1[QStringLiteral("name")] = QStringLiteral("Intro Sequence");
    shotsList.append(shot1);

    menu->setProperty("mockShots", shotsList);

    // Inject mock external tools
    QVariantList toolsList;
    QVariantMap tool1;
    tool1[QStringLiteral("name")] = QStringLiteral("Audacity");
    toolsList.append(tool1);

    menu->setProperty("mockTools", toolsList);
    QCoreApplication::processEvents();

    QObject *storyboardSubMenu = menu->findChild<QObject*>(UI::AssetContextMenu::StoryboardSubMenu);
    QVERIFY2(storyboardSubMenu != nullptr, "UI Component Missing: 'storyboardSubMenu' not found.");
    QVERIFY2(storyboardSubMenu->property("enabled").toBool(), "State Failure: Storyboard submenu enabled despite empty active project.");

    QObject *toolsSubMenu = menu->findChild<QObject*>(UI::AssetContextMenu::ExternalToolsSubMenu);
    QVERIFY2(toolsSubMenu != nullptr, "UI Component Missing: 'externalToolsSubMenu' not found.");
    QVERIFY2(toolsSubMenu->property("enabled").toBool(), "State Failure: Tools submenu enabled despite empty tools registry.");

}

QTEST_MAIN(Test_AssetContextMenu)
#include "test_AssetContextMenu.moc"
