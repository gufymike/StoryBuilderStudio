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
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

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

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_SeriesLauncherModal::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
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

QTEST_MAIN(Test_SeriesLauncherModal)
#include "test_SeriesLauncherModal.moc"

