/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_SeriesLauncherModal.cpp
** Unit test suite for SeriesLauncherModal QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlContext>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "seriescontainer.h"
#include "settingsmanager.h"
#include "ui_constants.h"

class Test_SeriesLauncherModal : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testComponentInstantiationAndHeader();
    void testEmptySeriesListState();
    void testPopulatedSeriesListState();
    void testActionButtonsAndToolTips();
    void testDeleteSeriesConfirmationAndDiskRemoval();

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_SeriesLauncherModal::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    // StandardLocations/TempLocations/uuid sandbox directory setup
    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    m_engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
    m_engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
    m_engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core"));

    QString qmlFilePath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core/SeriesLauncherModal.qml");
    m_componentUrl = QUrl::fromLocalFile(qmlFilePath);
    QVERIFY2(QFile::exists(qmlFilePath), qPrintable("File not found: " + qmlFilePath));
}

void Test_SeriesLauncherModal::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
    QStandardPaths::setTestModeEnabled(false);
}

void Test_SeriesLauncherModal::testComponentInstantiationAndHeader()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    QObject *title = dialog->findChild<QObject*>(UI::SeriesLauncherModal::ModalTitle);
    QVERIFY2(title != nullptr, "UI Component Missing: 'modalTitleText' not found.");
    QCOMPARE(title->property("text").toString(), QStringLiteral("Story Builder Studio — Series Workspace"));
    QCOMPARE(title->property("color").toString().toUpper(), QStringLiteral("#00F0FF"));

    QObject *subtitle = dialog->findChild<QObject*>(UI::SeriesLauncherModal::ModalSubtitle);
    QVERIFY2(subtitle != nullptr, "UI Component Missing: 'modalSubtitleText' not found.");
    QCOMPARE(subtitle->property("text").toString(), QStringLiteral("Select or create a series to continue"));
}

void Test_SeriesLauncherModal::testEmptySeriesListState()
{
    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
    engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
    engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core"));

    const QByteArray hostQml = QByteArrayLiteral(
        "import QtQuick\n"
        "import QtQuick.Controls\n"
        "import StoryBuilderStudio\n"
        "ApplicationWindow {\n"
        "    id: testRootWindow\n"
        "    width: 800\n"
        "    height: 600\n"
        "    visible: true\n"
        "    SeriesLauncherModal {\n"
        "        id: modal\n"
        "        visible: true\n"
        "        availableSeriesList: []\n"
        "    }\n"
        "}\n"
        );

    engine.loadData(hostQml);
    QVERIFY(!engine.rootObjects().isEmpty());

    QQuickWindow *win = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    QVERIFY(win != nullptr);
    win->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(win));

    QObject *placeholder = win->findChild<QObject*>(UI::SeriesLauncherModal::EmptyStateLabel);
    QVERIFY2(placeholder != nullptr, "UI Component Missing: 'emptyStateLabel' not found.");

    QTRY_COMPARE(placeholder->property("visible").toBool(), true);
    QCOMPARE(placeholder->property("text").toString(),
             QStringLiteral("No known series registered.\nCreate a new series or browse an existing folder below."));
}

void Test_SeriesLauncherModal::testPopulatedSeriesListState()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    QVariantList mockSeries;
    mockSeries.append(QStringLiteral("C:/Workspace/MacrossDelta"));
    mockSeries.append(QStringLiteral("C:/Workspace/GalacticHeroes"));
    dialog->setProperty("availableSeriesList", mockSeries);

    QObject *listView = dialog->findChild<QObject*>(UI::SeriesLauncherModal::ListView);
    QVERIFY2(listView != nullptr, "UI Component Missing: 'seriesListView' not found.");
    QCOMPARE(listView->property("count").toInt(), 2);

    QObject *placeholder = dialog->findChild<QObject*>(UI::SeriesLauncherModal::EmptyStateLabel);
    QVERIFY2(placeholder != nullptr, "UI Component Missing: 'emptyStateLabel' not found.");
    QCOMPARE(placeholder->property("visible").toBool(), false);
}

void Test_SeriesLauncherModal::testActionButtonsAndToolTips()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    QObject *newSeriesBtn = dialog->findChild<QObject*>(UI::SeriesLauncherModal::NewSeriesBtn);
    QVERIFY2(newSeriesBtn != nullptr, "UI Component Missing: 'newSeriesBtn' not found.");
    QVERIFY2(newSeriesBtn->property("hovered").isValid(), "newSeriesBtn missing hover state required for ToolTips.");

    QObject *browseSeriesBtn = dialog->findChild<QObject*>(UI::SeriesLauncherModal::BrowseSeriesBtn);
    QVERIFY2(browseSeriesBtn != nullptr, "UI Component Missing: 'browseSeriesBtn' not found.");
    QCOMPARE(browseSeriesBtn->property("text").toString(), QStringLiteral("📁 Browse Other Series Folder..."));
    QVERIFY(browseSeriesBtn->property("hovered").isValid());
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
void Test_SeriesLauncherModal::testDeleteSeriesConfirmationAndDiskRemoval()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString projectsRoot       = tempDir.path();
    const QString targetSeriesPath   = QDir(projectsRoot).filePath(QStringLiteral("000_qtest_doomed_series"));
    const QString survivorSeriesPath = QDir(projectsRoot).filePath(QStringLiteral("001_qtest_keeper_series"));
    const QString tempIniPath        = QDir(projectsRoot).filePath(QStringLiteral("000_qtest_config.ini"));

    SeriesContainer seriesContainer;
    seriesContainer.setCustomProjectsRoot(projectsRoot);
    QVERIFY(seriesContainer.createNewContainer(targetSeriesPath, QStringLiteral("Doomed Series"), QStringLiteral("Hook"), QStringLiteral("Desc")));
    QVERIFY(seriesContainer.createNewContainer(survivorSeriesPath, QStringLiteral("Keeper Series"), QStringLiteral("Hook"), QStringLiteral("Desc")));
    seriesContainer.refreshAvailableSeries();
    QCOMPARE(seriesContainer.availableSeries().size(), 2);

    SettingsManager settingsManager(tempIniPath);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("seriesContainer"), &seriesContainer);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsManager"), &settingsManager);

    QQuickWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQmlComponent component(&engine, m_componentUrl);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QObject *dialogObj = component.create();
    QVERIFY(dialogObj != nullptr);
    dialogObj->setProperty("parent", QVariant::fromValue(window.contentItem()));
    QMetaObject::invokeMethod(dialogObj, "open");
    QTest::qWait(50);

    // Search the visual item tree (QQuickOverlay + ListView delegates)
    QQuickItem *removeBtn = findVisualItem(window.contentItem(), QStringLiteral("removeSeriesBtn_0"));
    QVERIFY2(removeBtn != nullptr, "Expected removeSeriesBtn_0 in SeriesLauncherModal");
    QMetaObject::invokeMethod(removeBtn, "clicked");
    QTest::qWait(50);

    QObject *confirmDialog = dialogObj->findChild<QObject *>(QStringLiteral("confirmDeleteSeriesDialog"));
    QVERIFY(confirmDialog != nullptr);
    QCOMPARE(confirmDialog->property("opened").toBool(), true);
    QVERIFY2(QDir(targetSeriesPath).exists(), "Series directory must not be deleted before confirmation");

    QQuickItem *confirmDeleteBtn = findVisualItem(window.contentItem(), QStringLiteral("confirmDeleteSeriesBtn"));
    QVERIFY2(confirmDeleteBtn != nullptr, "Expected confirmDeleteSeriesBtn inside confirmation dialog");
    QMetaObject::invokeMethod(confirmDeleteBtn, "clicked");
    QTest::qWait(50);

    QVERIFY2(!QDir(targetSeriesPath).exists(), "Confirmed series directory should be deleted from disk");
    QVERIFY2(QDir(survivorSeriesPath).exists(), "Unselected series directory must remain intact");
    QCOMPARE(seriesContainer.availableSeries().size(), 1);

    delete dialogObj;
}


QTEST_MAIN(Test_SeriesLauncherModal)
#include "test_SeriesLauncherModal.moc"
