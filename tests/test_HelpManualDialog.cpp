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

#include "ui_constants.h"

class Test_HelpManualDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDialogHeaderAndDefaults();
    void testTabNavigationAndPageSync();
    void testActionButtonsAndToolTips();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *dialog = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 900\n"
                "    height: 700\n"
                "    visible: true\n"
                "    HelpManualDialog {\n"
                "        id: helpModal\n"
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

            dialog = window->findChild<QObject*>(UI::HelpManualDialog::Root);
            return (dialog != nullptr);
        }
    };
};

void Test_HelpManualDialog::initTestCase()
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

void Test_HelpManualDialog::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_HelpManualDialog::testDialogHeaderAndDefaults()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *dialog = harness.dialog;
    QVERIFY(dialog != nullptr);

    QObject *headerTitle = dialog->findChild<QObject*>(UI::HelpManualDialog::HeaderTitle);
    QVERIFY2(headerTitle != nullptr, "UI Component Missing: 'headerTitleText' not found.");
    QObject *tabBar = dialog->findChild<QObject*>(UI::HelpManualDialog::HelpTabBar);
    QObject *page0 = dialog->findChild<QObject*>(UI::HelpManualDialog::Page0TextArea);
    QVERIFY2(tabBar != nullptr, "UI Component Missing: 'helpTabBar' not found.");
    QVERIFY2(page0 != nullptr, "UI Component Missing: 'page0TextArea' not found.");


    QVERIFY(headerTitle->property("text").toString().contains(QStringLiteral("Story Builder Studio Documentation")));
    QCOMPARE(headerTitle->property("color").toString().toUpper(), QStringLiteral("#00F0FF"));
    QCOMPARE(tabBar->property("currentIndex").toInt(), 0);
    QVERIFY(page0->property("text").toString().contains(QStringLiteral("Story Builder Studio Architecture")));
}

void Test_HelpManualDialog::testTabNavigationAndPageSync()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *dialog = harness.dialog;
    QObject *tabBar = dialog->findChild<QObject*>(UI::HelpManualDialog::HelpTabBar);
    QObject *stackLayout = dialog->findChild<QObject*>(UI::HelpManualDialog::HelpStackLayout);
    QObject *page1 = dialog->findChild<QObject*>(UI::HelpManualDialog::Page1TextArea);
    QVERIFY2(stackLayout != nullptr, "UI Component Missing: 'helpStackLayout' not found.");
    QVERIFY2(page1 != nullptr, "UI Component Missing: 'page1TextArea' not found.");

    // Switch to Tab 1 (Directories)
    tabBar->setProperty("currentIndex", 1);
    QCoreApplication::processEvents();
    QCOMPARE(stackLayout->property("currentIndex").toInt(), 1);


    QVERIFY(page1->property("text").toString().contains(QStringLiteral("storyBuilderStudio")));

    // Switch to Tab 2 (Prompts & Ledger)
    tabBar->setProperty("currentIndex", 2);
    QCoreApplication::processEvents();
    QCOMPARE(stackLayout->property("currentIndex").toInt(), 2);

    // Switch to Tab 3 (Shortcuts & Diagnostics)
    tabBar->setProperty("currentIndex", 3);
    QCoreApplication::processEvents();
    QCOMPARE(stackLayout->property("currentIndex").toInt(), 3);
}

void Test_HelpManualDialog::testActionButtonsAndToolTips()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *dialog = harness.dialog;

    QObject *browserBtn = dialog->findChild<QObject*>(UI::HelpManualDialog::OpenBrowserBtn);
    QObject *closeBtn = dialog->findChild<QObject*>(UI::HelpManualDialog::CloseHelpBtn);
    QVERIFY2(browserBtn != nullptr, "UI Component Missing: 'openBrowserBtn' not found.");
    QVERIFY2(closeBtn != nullptr, "UI Component Missing: 'closeHelpBtn' not found.");

    QCOMPARE(browserBtn->property("text").toString(), QStringLiteral("🌐 Open Full Manual in Browser"));
    QVERIFY(browserBtn->property("hovered").isValid());
    QCOMPARE(closeBtn->property("text").toString(), QStringLiteral("Close"));
    QVERIFY(closeBtn->property("hovered").isValid());
}

QTEST_MAIN(Test_HelpManualDialog)
#include "test_HelpManualDialog.moc"
