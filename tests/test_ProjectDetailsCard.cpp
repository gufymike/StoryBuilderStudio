// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>
#include <QtPlugin>

#include "ui_constants.h"

class Test_ProjectDetailsCard : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testClosedProjectState();
    void testOpenProjectStateAndText();
    void testEditableFieldPairsConfiguration();

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_ProjectDetailsCard::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    // StandardLocations/TempLocations/uuid sandbox directory setup
    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    // Add source directory to engine import paths so EditableFieldPair resolves
    m_engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
    m_engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core"));
    m_engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));

    QString qmlFilePath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core/ProjectDetailsCard.qml");
    m_componentUrl = QUrl::fromLocalFile(qmlFilePath);
    QVERIFY2(QFile::exists(qmlFilePath), qPrintable("File not found: " + qmlFilePath));
}

void Test_ProjectDetailsCard::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_ProjectDetailsCard::testClosedProjectState()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> card(component.create());
    QVERIFY(!card.isNull());

    card->setProperty("isProjectOpen", false);

    QObject *headerText = card->findChild<QObject*>(UI::ProjectDetailsCard::HeaderTitle);
    QVERIFY2(headerText != nullptr, "UI Component Missing: 'headerTitleText' not found.");
    QCOMPARE(headerText->property("text").toString(), QStringLiteral("No Episode Currently Open"));
    QCOMPARE(headerText->property("color").toString().toUpper(), QStringLiteral("#FF5555"));

    QObject *form = card->findChild<QObject*>(UI::ProjectDetailsCard::FormContainer);
    QVERIFY2(form != nullptr, "UI Component Missing: 'formContainer' not found.");
    QCOMPARE(form->property("enabled").toBool(), false);
}

void Test_ProjectDetailsCard::testOpenProjectStateAndText()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> card(component.create());
    QVERIFY(!card.isNull());

    card->setProperty("isProjectOpen", true);
    card->setProperty("projectName", QStringLiteral("Episode 01 - Arrival"));
    card->setProperty("projectVersion", QStringLiteral("1.2"));
    card->setProperty("dateCreated", QStringLiteral("2026-11-01"));
    card->setProperty("dateLastEdited", QStringLiteral("2026-11-07"));

    QObject *headerText = card->findChild<QObject*>(UI::ProjectDetailsCard::HeaderTitle);
    QVERIFY2(headerText != nullptr, "UI Component Missing: 'headerTitleText' not found.");
    QCOMPARE(headerText->property("text").toString(), QStringLiteral("Active Episode Configuration: Episode 01 - Arrival"));
    QCOMPARE(headerText->property("color").toString().toUpper(), QStringLiteral("#00F0FF"));

    QObject *versionText = card->findChild<QObject*>(UI::ProjectDetailsCard::VersionText);
    QVERIFY2(versionText != nullptr, "UI Component Missing: 'versionText' not found.");
    QCOMPARE(versionText->property("text").toString(), QStringLiteral("Version: 1.2"));

    QObject *createdDateText = card->findChild<QObject*>(UI::ProjectDetailsCard::CreatedDateText);
    QVERIFY2(createdDateText != nullptr, "UI Component Missing: 'createdDateText' not found.");
    QCOMPARE(createdDateText->property("text").toString(), QStringLiteral("2026-11-01"));

    QObject *form = card->findChild<QObject*>(UI::ProjectDetailsCard::FormContainer);
    QVERIFY2(form != nullptr, "UI Component Missing: 'formContainer' not found.");
    QCOMPARE(form->property("enabled").toBool(), true);
}

void Test_ProjectDetailsCard::testEditableFieldPairsConfiguration()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> card(component.create());
    QVERIFY(!card.isNull());

    card->setProperty("isProjectOpen", true);
    card->setProperty("projectName", QStringLiteral("Pilot"));
    card->setProperty("projectDescription", QStringLiteral("Opening chapter."));
    card->setProperty("author", QStringLiteral("Mike Ramirez"));
    card->setProperty("email", QStringLiteral("me@home.com"));

    QObject *epNameField = card->findChild<QObject*>(UI::ProjectDetailsCard::EpNameField);
    QVERIFY2(epNameField != nullptr, "UI Component Missing: 'epNameField' not found.");
    QCOMPARE(epNameField->property("fieldValue").toString(), QStringLiteral("Pilot"));
    QCOMPARE(epNameField->property("toolTipText").toString(), QStringLiteral("The episode name."));

    QObject *descField = card->findChild<QObject*>(UI::ProjectDetailsCard::DescField);
    QVERIFY2(descField != nullptr, "UI Component Missing: 'descrField' not found.");
    QCOMPARE(descField->property("fieldValue").toString(), QStringLiteral("Opening chapter."));

    QObject *writerField = card->findChild<QObject*>(UI::ProjectDetailsCard::WriterField);
    QVERIFY2(writerField != nullptr, "UI Component Missing: 'writerField' not found.");
    QCOMPARE(writerField->property("fieldValue").toString(), QStringLiteral("Mike Ramirez"));

    QObject *contactField = card->findChild<QObject*>(UI::ProjectDetailsCard::ContactField);
    QVERIFY2(contactField != nullptr, "UI Component Missing: 'contactField' not found.");
    QCOMPARE(contactField->property("fieldValue").toString(), QStringLiteral("me@home.com"));
}

QTEST_MAIN(Test_ProjectDetailsCard)
#include "test_ProjectDetailsCard.moc"

