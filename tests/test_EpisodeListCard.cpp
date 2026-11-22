/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_EpisodeListCard.cpp
** Unit test suite for EpisodeListCard QML component under QTest.
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

class Test_EpisodeListCard : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testComponentInstantiationAndHeader();
    void testEmptyStatePlaceholderText();
    void testCountBadgeFormatting();
    void testNewEpisodeButtonEnableState();
    void testToolTipsConfigured();

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_EpisodeListCard::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    // StandardLocations/TempLocations/uuid sandbox setup
    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    QString qmlFilePath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core/EpisodeListCard.qml");
    m_componentUrl = QUrl::fromLocalFile(qmlFilePath);
    QVERIFY2(QFile::exists(qmlFilePath), qPrintable("File not found: " + qmlFilePath));
}

void Test_EpisodeListCard::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_EpisodeListCard::testComponentInstantiationAndHeader()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> card(component.create());
    QVERIFY(!card.isNull());

    // Verify Title Text
    QObject *title = card->findChild<QObject*>(UI::EpisodeListCard::HeaderTitle);
    QVERIFY2(title != nullptr, "UI Component Missing: 'cardHeaderTitle' not found.");


    QCOMPARE(title->property("text").toString(), QStringLiteral("Series Episodes / Chapters"));

    // Verify Button Text
    QObject *newEpBtn = card->findChild<QObject*>(UI::EpisodeListCard::NewEpisodeBtn);
    QVERIFY2(newEpBtn != nullptr, "UI Component Missing: 'epNewEpisodeBtn' not found.");
    QCOMPARE(newEpBtn->property("text").toString(), QStringLiteral("＋ New Episode"));
}

void Test_EpisodeListCard::testEmptyStatePlaceholderText()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> card(component.create());
    QVERIFY(!card.isNull());

    card->setProperty("subProjectCount", 0);

    QObject *placeholder = card->findChild<QObject*>(UI::EpisodeListCard::EmptyPlaceholder);
    QVERIFY2(placeholder != nullptr, "UI Component Missing: 'emptyPlaceHolder' not found.");
    QCOMPARE(placeholder->property("visible").toBool(), true);
    QCOMPARE(placeholder->property("text").toString(),
             QStringLiteral("No episodes created for this series yet. Click '＋ New Episode' above to begin."));
}

void Test_EpisodeListCard::testCountBadgeFormatting()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> card(component.create());
    QVERIFY(!card.isNull());

    card->setProperty("subProjectCount", 5);

    QObject *countText = card->findChild<QObject*>(UI::EpisodeListCard::CountText);
    QVERIFY2(countText != nullptr, "UI Component Missing: 'countText' not found.");
    QCOMPARE(countText->property("text").toString(), QStringLiteral("5 Total"));
}

void Test_EpisodeListCard::testNewEpisodeButtonEnableState()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> card(component.create());
    QVERIFY(!card.isNull());

    QObject *newEpBtn = card->findChild<QObject*>(UI::EpisodeListCard::NewEpisodeBtn);
    QVERIFY2(newEpBtn != nullptr, "UI Component Missing: 'epNewEpisodeBtn' not found.");

    // Disabled when rootDirectory is empty
    card->setProperty("rootDirectory", QString());
    QCOMPARE(newEpBtn->property("enabled").toBool(), false);

    // Enabled when valid directory path is provided
    card->setProperty("rootDirectory", m_testSessionDir);
    QCOMPARE(newEpBtn->property("enabled").toBool(), true);
}

void Test_EpisodeListCard::testToolTipsConfigured()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> card(component.create());
    QVERIFY(!card.isNull());

    QObject *newEpBtn = card->findChild<QObject*>(UI::EpisodeListCard::NewEpisodeBtn);
    QVERIFY2(newEpBtn != nullptr, "UI Component Missing: 'epNewEpisodeBtn' not found.");
    QVERIFY(newEpBtn->property("hovered").isValid());
}

QTEST_MAIN(Test_EpisodeListCard)
#include "test_EpisodeListCard.moc"

