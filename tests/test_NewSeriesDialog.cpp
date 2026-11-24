// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"

class Test_NewSeriesDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testComponentInstantiation();
    void testUrlToPathSanitization();
    void testFormFieldsAndDefaults();

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_NewSeriesDialog::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    // StandardLocations/TempLocations/uuid sandbox setup
    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    QString qmlFilePath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core/NewSeriesDialog.qml");
    m_componentUrl = QUrl::fromLocalFile(qmlFilePath);
    QVERIFY2(QFile::exists(qmlFilePath), qPrintable("File not found: " + qmlFilePath));
}

void Test_NewSeriesDialog::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_NewSeriesDialog::testComponentInstantiation()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    QCOMPARE(dialog->property("title").toString(), QStringLiteral("Create New Series Container"));

    QObject *header = dialog->findChild<QObject*>(UI::NewSeriesDialog::HeaderTitle);
    QVERIFY2(header != nullptr, "UI Component Missing: 'headerTitle' not found.");
    QCOMPARE(header->property("color").toString().toUpper(), QStringLiteral("#00F0FF"));
}

void Test_NewSeriesDialog::testUrlToPathSanitization()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    QVariant result;
#ifdef Q_OS_WIN
    QMetaObject::invokeMethod(dialog.data(), "urlToPath",
                              Q_RETURN_ARG(QVariant, result),
                              Q_ARG(QVariant, QStringLiteral("file:///C:/Projects/Universe")));
    QCOMPARE(result.toString(), QStringLiteral("C:/Projects/Universe"));
#else
    QMetaObject::invokeMethod(dialog.data(), "urlToPath",
                              Q_RETURN_ARG(QVariant, result),
                              Q_ARG(QVariant, QStringLiteral("file:///home/user/Projects/Universe")));
    QCOMPARE(result.toString(), QStringLiteral("/home/user/Projects/Universe"));
#endif
}

void Test_NewSeriesDialog::testFormFieldsAndDefaults()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    QObject *seriesTitleField = dialog->findChild<QObject*>(UI::NewSeriesDialog::SeriesTitleField);
    QObject *createEpisodeCheckbox = dialog->findChild<QObject*>(UI::NewSeriesDialog::CreateEpisodeBox);
    QObject *locationField = dialog->findChild<QObject*>(UI::NewSeriesDialog::LocationField);
    QVERIFY2(seriesTitleField != nullptr, "UI Component Missing: 'seriesTitleField' not found.");
    QVERIFY2(createEpisodeCheckbox != nullptr, "UI Component Missing: 'createEpisodeCheckbox' not found.");
    QVERIFY2(locationField != nullptr, "UI Component Missing: 'locationField' not found.");

    // Default state: checkbox should be checked
    QCOMPARE(createEpisodeCheckbox->property("checked").toBool(), true);

    // Target folder fallback verification
    QString targetFolder = dialog->property("targetRootFolder").toString();
    QVERIFY(targetFolder.contains(QStringLiteral("StoryBuilderStudioProjects")));
}

QTEST_MAIN(Test_NewSeriesDialog)
#include "test_NewSeriesDialog.moc"

