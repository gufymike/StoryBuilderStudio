// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QQuickItem>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>
#include <QtPlugin>

#include "ui_constants.h"

class Test_SeriesHeaderCard : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testUnloadedSeriesState();
    void testLoadedSeriesStateAndLabels();
    void testEditableFieldPairsConfiguration();
    void testToolTipsAndButtonEnabled();
    void testCollapseAndExpandToggle();

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_SeriesHeaderCard::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    // 1. Clear default import paths that might look into stale build-dir qmldir tables
    // 2. Add the project source root so "import StoryBuilderStudio" or direct module paths resolve
    m_engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
    m_engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));

    // Crucial for modular multi-folder QML: Register the app root as a type search root
    m_engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app"));

    QString qmlFilePath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core/SeriesHeaderCard.qml");
    m_componentUrl = QUrl::fromLocalFile(qmlFilePath);
    QVERIFY2(QFile::exists(qmlFilePath), qPrintable("File not found: " + qmlFilePath));
}

void Test_SeriesHeaderCard::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_SeriesHeaderCard::testUnloadedSeriesState()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> card(component.create());
    QVERIFY(!card.isNull());

    card->setProperty("isSeriesLoaded", false);

    // Verify Title shows fallback warning text
    QObject *header = card->findChild<QObject*>(UI::SeriesHeaderCard::HeaderTitle);
    QVERIFY2(header != nullptr, "UI Component Missing: 'headerTitleText' not found.");
    QCOMPARE(header->property("text").toString(), QStringLiteral("No Series Currently Loaded"));
    QCOMPARE(header->property("color").toString().toUpper(), QStringLiteral("#FF5555"));

    // Verify episode badge is hidden
    QObject *badge = card->findChild<QObject*>(UI::SeriesHeaderCard::BadgeText);
    QVERIFY2(badge != nullptr, "UI Component Missing: 'badgeText' not found.");
    QCOMPARE(badge->property("visible").toBool(), false);

    // Verify Form is disabled
    QObject *form = card->findChild<QObject*>(UI::SeriesHeaderCard::FormLayout);
    QVERIFY2(form != nullptr, "UI Component Missing: 'formLayout' not found.");
    QCOMPARE(form->property("enabled").toBool(), false);

    // Verify "＋ New Episode" button is disabled
    QObject *newEpBtn = card->findChild<QObject*>(UI::SeriesHeaderCard::NewEpisodeBtn);
    QVERIFY2(newEpBtn != nullptr, "UI Component Missing: 'newEpisodeBtn' not found.");
    QCOMPARE(newEpBtn->property("enabled").toBool(), false);
}

void Test_SeriesHeaderCard::testLoadedSeriesStateAndLabels()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> card(component.create());
    QVERIFY(!card.isNull());

    card->setProperty("isSeriesLoaded", true);
    card->setProperty("seriesName", QStringLiteral("Chronicles of Orion"));
    card->setProperty("subProjectCount", 8);
    card->setProperty("creationDateStr", QStringLiteral("11/01/2026"));
    card->setProperty("lastModifiedStr", QStringLiteral("11/07/2026"));
    card->setProperty("rootDirectory", m_testSessionDir);

    // Verify Header text and cyan color
    QObject *header = card->findChild<QObject*>(UI::SeriesHeaderCard::HeaderTitle);
    QVERIFY2(header != nullptr, "UI Component Missing: 'headerTitleText' not found.");
    QCOMPARE(header->property("text").toString(), QStringLiteral("Chronicles of Orion"));
    QCOMPARE(header->property("color").toString().toUpper(), QStringLiteral("#00F0FF"));

    // Verify Episode badge text
    QObject *badgeText = card->findChild<QObject*>(UI::SeriesHeaderCard::BadgeText);
    QVERIFY2(badgeText != nullptr, "UI Component Missing: 'badgeText' not found.");
    QCOMPARE(badgeText->property("text").toString(), QStringLiteral("8 Episodes"));

    // Verify Timestamps
    QObject *createdDate = card->findChild<QObject*>(UI::SeriesHeaderCard::CreatedDateText);
    QVERIFY2(createdDate != nullptr, "UI Component Missing: 'createdDateText' not found.");
    QCOMPARE(createdDate->property("text").toString(), QStringLiteral("Created: 11/01/2026"));

    // Verify Path Text
    QObject *manifestPath = card->findChild<QObject*>(UI::SeriesHeaderCard::ManifestPathText);
    QVERIFY2(manifestPath != nullptr, "UI Component Missing: 'manifestPathText' not found.");
    QCOMPARE(manifestPath->property("text").toString(), m_testSessionDir);
}

void Test_SeriesHeaderCard::testEditableFieldPairsConfiguration()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> card(component.create());
    QVERIFY(!card.isNull());

    card->setProperty("isSeriesLoaded", true);
    card->setProperty("seriesName", QStringLiteral("Project A"));
    card->setProperty("shortDescription", QStringLiteral("Epic space adventure"));
    card->setProperty("longDescription", QStringLiteral("A thorough story bible for deep space travel."));
    card->setProperty("creator", QStringLiteral("Mike Ramirez"));

    QVariantList coCreators;
    coCreators.append(QStringLiteral("Assistant Director"));
    coCreators.append(QStringLiteral("Lead Writer"));
    card->setProperty("coCreators", coCreators);
    card->setProperty("email", QStringLiteral("me@home.com"));

    // Verify Title Pair
    QObject *titlePair = card->findChild<QObject*>(UI::SeriesHeaderCard::SeriesTitlePair);
    QVERIFY2(titlePair != nullptr, "UI Component Missing: 'seriesTitlePair' not found.");
    QCOMPARE(titlePair->property("fieldValue").toString(), QStringLiteral("Project A"));
    QCOMPARE(titlePair->property("toolTipText").toString(), QStringLiteral("The series title."));

    // Verify Short Description Pair
    QObject *shortDescPair = card->findChild<QObject*>(UI::SeriesHeaderCard::ShortDescPair);
    QVERIFY2(shortDescPair != nullptr, "UI Component Missing: 'shortDescPair' not found.");
    QCOMPARE(shortDescPair->property("fieldValue").toString(), QStringLiteral("Epic space adventure"));

    // Verify Creator Pair
    QObject *creatorPair = card->findChild<QObject*>(UI::SeriesHeaderCard::CreatorPair);
    QVERIFY2(creatorPair != nullptr, "UI Component Missing: 'creatorPair' not found.");
    QCOMPARE(creatorPair->property("fieldValue").toString(), QStringLiteral("Mike Ramirez"));

    // Verify CoCreators Pair (Comma-separated)
    QObject *coCreatorsPair = card->findChild<QObject*>(UI::SeriesHeaderCard::CoCreatorsPair);
    QVERIFY2(coCreatorsPair != nullptr, "UI Component Missing: 'coCreatorPair' not found.");
    QCOMPARE(coCreatorsPair->property("fieldValue").toString(), QStringLiteral("Assistant Director, Lead Writer"));

    // Verify Email Pair
    QObject *emailPair = card->findChild<QObject*>(UI::SeriesHeaderCard::EmailPair);
    QVERIFY2(emailPair != nullptr, "UI Component Missing: 'emailPair' not found.");
    QCOMPARE(emailPair->property("fieldValue").toString(), QStringLiteral("me@home.com"));
}

void Test_SeriesHeaderCard::testToolTipsAndButtonEnabled()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> card(component.create());
    QVERIFY(!card.isNull());

    card->setProperty("isSeriesLoaded", true);

    QObject *newEpBtn = card->findChild<QObject*>(UI::SeriesHeaderCard::NewEpisodeBtn);
    QVERIFY2(newEpBtn != nullptr, "UI Component Missing: 'newEpisodeBtn' not found.");
    QCOMPARE(newEpBtn->property("text").toString(), QStringLiteral("＋ New Episode"));
    QCOMPARE(newEpBtn->property("enabled").toBool(), true);
    QVERIFY(newEpBtn->property("hovered").isValid());
}

void Test_SeriesHeaderCard::testCollapseAndExpandToggle()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QQuickWindow window;
    window.resize(800, 600);
    window.show();

    QScopedPointer<QObject> cardObj(component.create());
    QVERIFY(!cardObj.isNull());

    auto *card = qobject_cast<QQuickItem*>(cardObj.data());
    QVERIFY(card != nullptr);
    card->setParentItem(window.contentItem());
    card->setWidth(760);
    card->setProperty("isSeriesLoaded", true);
    QCoreApplication::processEvents();

    QObject *form = card->findChild<QObject*>(UI::SeriesHeaderCard::FormLayout);
    QVERIFY2(form != nullptr, "UI Component Missing: 'formLayout' not found.");

    QObject *toggleBtn = card->findChild<QObject*>(UI::SeriesHeaderCard::ToggleCollapseBtn);
    QVERIFY2(toggleBtn != nullptr, "UI Component Missing: 'toggleCollapseBtn' not found.");

    // Starts collapsed by default
    QCOMPARE(card->property("collapsed").toBool(), true);
    QCOMPARE(form->property("visible").toBool(), false);
    QCOMPARE(toggleBtn->property("text").toString(), QStringLiteral("▾ Details"));
    const qreal collapsedHeight = card->property("implicitHeight").toReal();

    // Click to expand
    QMetaObject::invokeMethod(toggleBtn, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(card->property("collapsed").toBool(), false);
    QCOMPARE(form->property("visible").toBool(), true);
    QCOMPARE(toggleBtn->property("text").toString(), QStringLiteral("▴ Hide"));
    QTRY_VERIFY(card->implicitHeight() > collapsedHeight);

    // Click to collapse again
    QMetaObject::invokeMethod(toggleBtn, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(card->property("collapsed").toBool(), true);
    QCOMPARE(form->property("visible").toBool(), false);
    QCOMPARE(toggleBtn->property("text").toString(), QStringLiteral("▾ Details"));
    QTRY_COMPARE(card->implicitHeight(), collapsedHeight);
}

QTEST_MAIN(Test_SeriesHeaderCard)
#include "test_SeriesHeaderCard.moc"

