/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_EditableFieldPair.cpp
** Unit test suite verifying EditableFieldPair QML behavior and ToolTips.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>
#include "ui_constants.h"

class Test_EditableFieldPair : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testComponentInstantiation();
    void testToggleEditModeAndSignals();
    void testToolTipConfiguration();

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_EditableFieldPair::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    // StandardLocations/TempLocations/uuid sandbox setup
    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    QString qmlFilePath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core/EditableFieldPair.qml");
    m_componentUrl = QUrl::fromLocalFile(qmlFilePath);
    QVERIFY2(QFile::exists(qmlFilePath), qPrintable("File not found: " + qmlFilePath));
}

void Test_EditableFieldPair::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_EditableFieldPair::testComponentInstantiation()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> instance(component.create());
    QVERIFY(!instance.isNull());

    instance->setProperty("labelText", QStringLiteral("Episode Title:"));
    instance->setProperty("fieldValue", QStringLiteral("Pilot Episode"));
    instance->setProperty("toolTipText", QStringLiteral("Enter title here"));

    QCOMPARE(instance->property("labelText").toString(), QStringLiteral("Episode Title:"));
    QCOMPARE(instance->property("fieldValue").toString(), QStringLiteral("Pilot Episode"));
    QCOMPARE(instance->property("toolTipText").toString(), QStringLiteral("Enter title here"));
    QCOMPARE(instance->property("isEditing").toBool(), false);
}

void Test_EditableFieldPair::testToggleEditModeAndSignals()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> instance(component.create());
    QVERIFY(!instance.isNull());

    instance->setProperty("fieldValue", QStringLiteral("Original Value"));

    QSignalSpy submitSpy(instance.data(), SIGNAL(valueSubmitted(QString)));

    // Switch to editing
    instance->setProperty("isEditing", true);
    QCOMPARE(instance->property("isEditing").toBool(), true);

    // Modify input
    QObject *editField = instance->findChild<QObject*>(UI::EditableFieldPair::EditField);
    QVERIFY2(editField != nullptr, "Critical UI Component Missing: 'editField' not found in QML tree.");
    editField->setProperty("text", QStringLiteral("New Modified Value"));

    // Trigger save button
    QObject *editBtn = instance->findChild<QObject*>(UI::EditableFieldPair::EditBtn);
    QVERIFY2(editBtn != nullptr, "Critical UI Component Missing: 'editBtn' not found in QML tree.");
    QMetaObject::invokeMethod(editBtn, "clicked");

    // Verify signal emission and value
    QCOMPARE(submitSpy.count(), 1);
    QCOMPARE(submitSpy.first().at(0).toString(), QStringLiteral("New Modified Value"));
    QCOMPARE(instance->property("isEditing").toBool(), false);
}

void Test_EditableFieldPair::testToolTipConfiguration()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> instance(component.create());
    QVERIFY(!instance.isNull());

    instance->setProperty("labelText", QStringLiteral("Camera Model:"));
    instance->setProperty("toolTipText", QStringLiteral("Select anamorphic rig"));

    QCOMPARE(instance->property("toolTipText").toString(), QStringLiteral("Select anamorphic rig"));

    QObject *editBtn = instance->findChild<QObject*>(UI::EditableFieldPair::EditBtn);
    QVERIFY2(editBtn != nullptr, "Critical UI Component Missing: 'editBtn' not found in QML tree for tool tip.");

    // Verify the button has the hover handling and can toggle states
    QVERIFY(editBtn->property("hovered").isValid());

    // In edit mode vs display mode
    instance->setProperty("isEditing", false);
    QCOMPARE(instance->property("isEditing").toBool(), false);

    instance->setProperty("isEditing", true);
    QCOMPARE(instance->property("isEditing").toBool(), true);
}

QTEST_MAIN(Test_EditableFieldPair)
#include "test_EditableFieldPair.moc"

