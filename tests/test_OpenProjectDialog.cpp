/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_OpenProjectDialog.cpp
** Unit test suite for OpenProjectDialog QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"

class Test_OpenProjectDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testComponentInstantiationAndText();
    void testEmptyStateStackSwitch();
    void testPopulatedStateModelBinding();
    void testToolTipsConfigured();

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_OpenProjectDialog::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    // Sandbox folder setup according to guidelines
    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    QString qmlFilePath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core/OpenProjectDialog.qml");
    m_componentUrl = QUrl::fromLocalFile(qmlFilePath);
    QVERIFY2(QFile::exists(qmlFilePath), qPrintable("File not found: " + qmlFilePath));
}

void Test_OpenProjectDialog::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_OpenProjectDialog::testComponentInstantiationAndText()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    // Verify Header Label Text
    QObject *header = dialog->findChild<QObject*>(UI::OpenProjectDialog::HeaderTitle);
    QVERIFY2(header != nullptr, "UI Component Missing: 'headerTitle' not found.");
    QCOMPARE(header->property("text").toString(), QStringLiteral("Open Existing Project"));
    QCOMPARE(header->property("color").toString().toUpper(), QStringLiteral("#00F0FF"));

    // Verify Cancel Button Text
    QObject *cancelBtn = dialog->findChild<QObject*>(UI::OpenProjectDialog::CancelBtn);
    QVERIFY2(cancelBtn != nullptr, "UI Component Missing: 'cancelBtn' not found.");
    QCOMPARE(cancelBtn->property("text").toString(), QStringLiteral("Cancel"));
}

void Test_OpenProjectDialog::testEmptyStateStackSwitch()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    // Explicitly set an empty project list
    dialog->setProperty("projectList", QVariantList());

    QObject *stack = dialog->findChild<QObject*>(UI::OpenProjectDialog::ContentStack);
    QVERIFY2(stack != nullptr, "UI Component Missing: 'contentStack' not found.");
    QCOMPARE(stack->property("currentIndex").toInt(), 1);

    // Verify empty state warning message
    QObject *emptyLabel = dialog->findChild<QObject*>(UI::OpenProjectDialog::EmptyStateLabel);
    QVERIFY2(emptyLabel != nullptr, "UI Component Missing: 'emptyStateLabel' not found.");
    QCOMPARE(emptyLabel->property("text").toString(),
             QStringLiteral("No existing projects found in your workspace repository."));

    // Verify creation button in empty state
    QObject *createBtn = dialog->findChild<QObject*>(UI::OpenProjectDialog::CreateNewBtn);
    QVERIFY2(createBtn != nullptr, "UI Component Missing: 'createNewBtn' not found.");
    QCOMPARE(createBtn->property("text").toString(), QStringLiteral("Create a New Project Now"));
}

void Test_OpenProjectDialog::testPopulatedStateModelBinding()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    QVariantList mockProjects;
    mockProjects.append(QStringLiteral("Chronicles_Ep01"));
    mockProjects.append(QStringLiteral("Chronicles_Ep02"));

    dialog->setProperty("projectList", mockProjects);

    QObject *stack = dialog->findChild<QObject*>(UI::OpenProjectDialog::ContentStack);
    QVERIFY2(stack != nullptr, "UI Component Missing: 'contentStack' not found.");
    QCOMPARE(stack->property("currentIndex").toInt(), 0);

    QObject *listView = dialog->findChild<QObject*>(UI::OpenProjectDialog::ProjectListView);
    QVERIFY2(listView != nullptr, "UI Component Missing: 'projectListView' not found.");
    QCOMPARE(listView->property("model").toList().size(), 2);
}

void Test_OpenProjectDialog::testToolTipsConfigured()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());

    QObject *cancelBtn = dialog->findChild<QObject*>(UI::OpenProjectDialog::CancelBtn);
    QVERIFY2(cancelBtn != nullptr, "UI Component Missing: 'cancelBtn' not found.");
    QVERIFY(cancelBtn->property("hovered").isValid());

    QObject *createBtn = dialog->findChild<QObject*>(UI::OpenProjectDialog::CreateNewBtn);
    QVERIFY2(createBtn != nullptr, "UI Component Missing: 'createNewBtn' not found.");
    QVERIFY(createBtn->property("hovered").isValid());
}

QTEST_MAIN(Test_OpenProjectDialog)
#include "test_OpenProjectDialog.moc"

