/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_NewProjectDialog.cpp
** Unit test suite for NewProjectDialog QML component under QTest.
**
****************************************************************************/
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

class Test_NewProjectDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testComponentInstantiation();
    void testSmartEmailExtraction();
    void testResetAndOpenDefaults();

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_NewProjectDialog::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    // Set up standard sandboxed test directory
    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    QString qmlFilePath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core/NewProjectDialog.qml");
    m_componentUrl = QUrl::fromLocalFile(qmlFilePath);
    QVERIFY2(QFile::exists(qmlFilePath), qPrintable("File not found: " + qmlFilePath));
}

void Test_NewProjectDialog::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_NewProjectDialog::testComponentInstantiation()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    QCOMPARE(dialog->property("title").toString(), QStringLiteral("Create Story Builder Studio Project"));

    QObject *header = dialog->findChild<QObject*>(UI::NewProjectDialog::HeaderTitle);
    QVERIFY2(header != nullptr, "UI Component Missing: 'headerTitle' not found.");
    QCOMPARE(header->property("color").toString().toUpper(), QStringLiteral("#00F0FF"));
}

void Test_NewProjectDialog::testSmartEmailExtraction()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    QObject *authorField = dialog->findChild<QObject*>(UI::NewProjectDialog::AuthorField);
    QObject *emailField = dialog->findChild<QObject*>(UI::NewProjectDialog::EmailField);
    QVERIFY2(authorField != nullptr, "UI Component Missing: 'authorField' not found.");
    QVERIFY2(emailField != nullptr, "UI Component Missing: 'emailField' not found.");

    // Simulate entering "Mike Ramirez <me@home.com>"
    authorField->setProperty("text", QStringLiteral("Mike Ramirez <me@home.com>"));

    // Verify regex auto-filled emailField
    QCOMPARE(emailField->property("text").toString(), QStringLiteral("me@home.com"));
}

void Test_NewProjectDialog::testResetAndOpenDefaults()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    // Invoke resetAndOpen with custom series dir
    QString mockSeriesDir = m_testSessionDir + QStringLiteral("/SeriesA");
    QMetaObject::invokeMethod(dialog.data(), "resetAndOpen", Q_ARG(QVariant, mockSeriesDir));

    QCOMPARE(dialog->property("targetSeriesDirectory").toString(), mockSeriesDir);

    QObject *projNameField = dialog->findChild<QObject*>(UI::NewProjectDialog::ProjNameField);
    QVERIFY2(projNameField != nullptr, "UI Component Missing: 'projNameField' not found.");
    QCOMPARE(projNameField->property("text").toString(), QStringLiteral("ep01"));

    QObject *versionField = dialog->findChild<QObject*>(UI::NewProjectDialog::VersionField);
    QVERIFY2(versionField != nullptr, "UI Component Missing: 'versionField' not found.");
    QCOMPARE(versionField->property("text").toString(), QStringLiteral("1.0"));
}

QTEST_MAIN(Test_NewProjectDialog)
#include "test_NewProjectDialog.moc"

