/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_PromptOptionsPanel.cpp
** Unit test suite for PromptOptionsPanel QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "promptmanager.h"
#include "ui_constants.h"

class Test_PromptOptionsPanel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testPanelInitializationWithMockCategories();
    void testCheckboxToggleSignalEmission();
    void testLivePromptManagerIntegration();
    void testCheckboxInjectsPromptFileContents();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *panel = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/promptbuilder"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 600\n"
                "    height: 400\n"
                "    visible: true\n"
                "    PromptOptionsPanel {\n"
                "        id: optionsPanel\n"
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

            panel = window->findChild<QObject*>(UI::PromptOptionsPanel::Root);
            return (panel != nullptr);
        }
    };
};

void Test_PromptOptionsPanel::initTestCase()
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

void Test_PromptOptionsPanel::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_PromptOptionsPanel::testPanelInitializationWithMockCategories()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *panel = harness.panel;
    QVERIFY(panel != nullptr);

    QVariantMap mockCategories;
    QStringList textFiles = { QStringLiteral("cinematic_mood.txt"), QStringLiteral("style_noir.txt") };
    mockCategories[QStringLiteral("cinematic")] = textFiles;

    panel->setProperty("mockCategoriesData", mockCategories);
    QCoreApplication::processEvents();
    QObject *categoryRepeater = panel->findChild<QObject*>(UI::PromptOptionsPanel::CategoryRepeater);
    QVERIFY2(categoryRepeater != nullptr, "UI Component Missing: 'categoryRepeater' not found.");
    QCOMPARE(categoryRepeater->property("count").toInt(), 1);
}

void Test_PromptOptionsPanel::testCheckboxToggleSignalEmission()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

        QObject *panel = harness.panel;
        QSignalSpy toggleSpy(panel, SIGNAL(promptOptionToggled(QString,bool)));

        QVariantMap mockCategories;
        QStringList textFiles = { QStringLiteral("wide_angle.txt") };
        mockCategories[QStringLiteral("camera")] = textFiles;

        panel->setProperty("mockCategoriesData", mockCategories);

        // 1. Locate categoryRepeater and wait for it to generate the column
        QObject *categoryRepeater = panel->findChild<QObject*>(UI::PromptOptionsPanel::CategoryRepeater);
        QVERIFY2(categoryRepeater != nullptr, "UI Component Missing: 'categoryRepeater' not found.");
        QTRY_COMPARE_WITH_TIMEOUT(categoryRepeater->property("count").toInt(), 1, 2000);

        // 2. Retrieve the ColumnLayout directly from the Repeater using itemAt(0)
        QQuickItem *columnItem = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((QMetaObject::invokeMethod(categoryRepeater, "itemAt",
                                                        Qt::DirectConnection,
                                                        Q_RETURN_ARG(QQuickItem*, columnItem),
                                                        Q_ARG(int, 0)),
                              columnItem != nullptr), 2000);

    // 3. Find the ListView inside this specific ColumnLayout
    QObject *listView = columnItem->findChild<QObject*>(QStringLiteral("fileListView_camera"));
    QVERIFY(listView != nullptr);

    // 4. Query the ListView directly for its delegate item at index 0
    QQuickItem *checkBoxItem = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((QMetaObject::invokeMethod(listView, "itemAtIndex",
                                                        Qt::DirectConnection,
                                                        Q_RETURN_ARG(QQuickItem*, checkBoxItem),
                                                        Q_ARG(int, 0)),
                              checkBoxItem != nullptr), 2000);

    // 5. Toggle the checkbox property
    checkBoxItem->setProperty("checked", true);
    QCoreApplication::processEvents();

    QCOMPARE(toggleSpy.count(), 1);
    QList<QVariant> args = toggleSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), QStringLiteral("wide_angle.txt"));
    QCOMPARE(args.at(1).toBool(), true);
}

void Test_PromptOptionsPanel::testLivePromptManagerIntegration()
{
    // Create temporary category folder and prompt file
    QString textDir = m_testSessionDir + QStringLiteral("/prompts/text");
    QDir().mkpath(textDir);
    QFile samplePrompt(textDir + QStringLiteral("/sample.prompt"));
    QVERIFY(samplePrompt.open(QIODevice::WriteOnly | QIODevice::Text));
    samplePrompt.write("Cinematic master shot, 8k resolution");
    samplePrompt.close();

    PromptManager manager;
    manager.setPaths(m_testSessionDir, QString());

    TestHarness harness;
    harness.engine.rootContext()->setContextProperty(QStringLiteral("promptManager"), &manager);
    QVERIFY(harness.init(m_testSessionDir));

    QObject *panel = harness.panel;
    QVERIFY(panel != nullptr);

    // Verify 'text' category picks up 'sample.prompt'
    QVariant result;
    QMetaObject::invokeMethod(panel, "getFiles",
                              Q_RETURN_ARG(QVariant, result),
                              Q_ARG(QVariant, QStringLiteral("text")));

    QStringList files = result.toStringList();
    QVERIFY(files.contains(QStringLiteral("sample.prompt")));
}

static QQuickItem *findVisualItem(QQuickItem *root, const QString &objectName)
{
    if (!root) return nullptr;
    if (root->objectName() == objectName) return root;
    const QList<QQuickItem *> children = root->childItems();
    for (QQuickItem *child : children) {
        if (QQuickItem *found = findVisualItem(child, objectName)) {
            return found;
        }
    }
    return nullptr;
}

void Test_PromptOptionsPanel::testCheckboxInjectsPromptFileContents()
{
    QTemporaryDir tempSeriesDir;
    QVERIFY(tempSeriesDir.isValid());

    const QString textPromptDir = tempSeriesDir.filePath(QStringLiteral("prompts/text"));
    QVERIFY(QDir().mkpath(textPromptDir));

    const QString expectedContent = QStringLiteral("35mm anamorphic lens, moody sci-fi lighting");
    QFile promptFile(textPromptDir + QStringLiteral("/cinematic.prompt"));
    QVERIFY(promptFile.open(QIODevice::WriteOnly | QIODevice::Text));
    promptFile.write(expectedContent.toUtf8());
    promptFile.close();

    PromptManager promptManager;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("promptManager"), &promptManager);

    QQuickWindow window;
    window.resize(720, 320);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qt/qml/StoryBuilderStudio/app/promptbuilder/PromptOptionsPanel.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QObject *panelObj = component.create();
    QVERIFY(panelObj != nullptr);
    auto *panelItem = qobject_cast<QQuickItem *>(panelObj);
    QVERIFY(panelItem != nullptr);
    panelItem->setParentItem(window.contentItem());
    panelItem->setSize(QSizeF(700, 260));

    QMetaObject::invokeMethod(panelObj, "updateProjectPath",
                              Q_ARG(QVariant, tempSeriesDir.path()),
                              Q_ARG(QVariant, QString()));
    QCoreApplication::processEvents();

    QQuickItem *textCol = findVisualItem(panelItem, QStringLiteral("categoryColumn_text"));
    QVERIFY2(textCol != nullptr, "Expected categoryColumn_text");

    // 1. Verify built-in system prompt at index 0 (anime.prompt)
    QQuickItem *checkBox = findVisualItem(textCol, QStringLiteral("optionCheckBox_0"));
    QVERIFY2(checkBox != nullptr, "Expected optionCheckBox_0 (anime.prompt)");

    QSignalSpy toggleSpy(panelObj, SIGNAL(promptOptionToggled(QString,bool)));
    checkBox->setProperty("checked", true);
    QCoreApplication::processEvents();

    QCOMPARE(toggleSpy.count(), 1);
    QList<QVariant> args = toggleSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), QStringLiteral("Japanese animation style, vibrant linework"));
    QCOMPARE(args.at(1).toBool(), true);

    // give a bit of time for list view to instantiate.
    QMetaObject::invokeMethod(panelObj, "updateProjectPath",
                              Q_ARG(QVariant, tempSeriesDir.path()),
                              Q_ARG(QVariant, QString()));
    QTest::qWait(20);
    // 2. Scroll text ListView so index 4 (cinematic.prompt) is instantiated in the viewport
    QQuickItem *textListView = findVisualItem(textCol, QStringLiteral("fileListView_text"));
    QVERIFY2(textListView != nullptr, "Expected fileListView_text");
    QMetaObject::invokeMethod(textListView, "positionViewAtEnd");
    QCoreApplication::processEvents();

    QQuickItem *customCheckBox = findVisualItem(textCol, QStringLiteral("optionCheckBox_4"));
    QVERIFY2(customCheckBox != nullptr, "Expected optionCheckBox_4 (cinematic.prompt)");

    customCheckBox->setProperty("checked", true);
    QCoreApplication::processEvents();

    QCOMPARE(toggleSpy.count(), 1);
    args = toggleSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), expectedContent);
    QCOMPARE(args.at(1).toBool(), true);

    delete panelObj;
}

QTEST_MAIN(Test_PromptOptionsPanel)
#include "test_PromptOptionsPanel.moc"
