/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_LocalModalManifestCard.cpp
** Unit test suite for LocalModalManifestCard QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"

class Test_LocalModalManifestCard : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testEmptyModelFallback();
    void testPopulatedManifestAllFound();
    void testPopulatedManifestMissingFiles();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *card = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/settings"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 600\n"
                "    height: 400\n"
                "    visible: true\n"
                "    LocalModalManifestCard {\n"
                "        id: manifestCard\n"
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

            card = window->findChild<QObject*>(UI::ManifestCard::Root);
            return (card != nullptr);
        }
    };
};

void Test_LocalModalManifestCard::initTestCase()
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

void Test_LocalModalManifestCard::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_LocalModalManifestCard::testEmptyModelFallback()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *card = harness.card;
    QVERIFY(card != nullptr);

    QObject *emptyLabel = card->findChild<QObject*>(UI::ManifestCard::EmptyLabel);
    QVERIFY2(emptyLabel != nullptr, "UI Component Missing: 'emptyManifestLabel' not found.");
    QVERIFY2(emptyLabel->property("visible").toBool(), "Empty state label failed to appear for unconfigured model.");

    QObject *summaryBadge = card->findChild<QObject*>(UI::ManifestCard::StatusBadge);
    QVERIFY2(summaryBadge != nullptr, "UI Component Missing: 'statusSummaryBadge' not found.");
    QCOMPARE(summaryBadge->property("text").toString(), QStringLiteral("Unconfigured"));
}

void Test_LocalModalManifestCard::testPopulatedManifestAllFound()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *card = harness.card;

    QVariantList items;
    QVariantMap item1;
    item1[QStringLiteral("found")] = true;
    item1[QStringLiteral("label")] = QStringLiteral("Diffusion Model (FLUX.1)");
    item1[QStringLiteral("path")] = QStringLiteral("flux1-schnell.safetensors");
    items.append(item1);

    card->setProperty("mockItems", items);
    QCoreApplication::processEvents();

    QCOMPARE(card->property("allFound").toBool(), true);

    QObject *statusIcon = card->findChild<QObject*>(UI::ManifestCard::StatusIconText);
    QVERIFY2(statusIcon != nullptr, "UI Component Missing: 'statusIconText' not found.");
    QCOMPARE(statusIcon->property("text").toString(), QStringLiteral("✔"));

    QObject *summaryBadge = card->findChild<QObject*>(UI::ManifestCard::StatusBadge);
    QVERIFY2(summaryBadge != nullptr, "UI Component Missing: 'statusSummaryBadge' not found.");
    QCOMPARE(summaryBadge->property("text").toString(), QStringLiteral("Ready for Inference"));
    QCOMPARE(summaryBadge->property("color").toString().toUpper(), QStringLiteral("#00F0FF"));
}

void Test_LocalModalManifestCard::testPopulatedManifestMissingFiles()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *card = harness.card;

    QVariantList items;
    QVariantMap item1;
    item1[QStringLiteral("found")] = false;
    item1[QStringLiteral("label")] = QStringLiteral("Universal T5-XXL");
    item1[QStringLiteral("path")] = QStringLiteral("t5xxl_fp16.safetensors");
    items.append(item1);

    card->setProperty("mockItems", items);
    QCoreApplication::processEvents();

    QCOMPARE(card->property("allFound").toBool(), false);

    QObject *statusIcon = card->findChild<QObject*>(UI::ManifestCard::StatusIconText);
    QVERIFY2(statusIcon != nullptr, "UI Component Missing: 'statusIconText' not found.");
    QCOMPARE(statusIcon->property("text").toString(), QStringLiteral("⚠️"));

    QObject *summaryBadge = card->findChild<QObject*>(UI::ManifestCard::StatusBadge);
    QVERIFY2(summaryBadge != nullptr, "UI Component Missing: 'statusSummaryBadge' not found.");
    QVERIFY2(summaryBadge->property("color").toString().toUpper() == QStringLiteral("#FF6666"),
             "Summary badge did not turn red when a required file was missing.");
    QCOMPARE(summaryBadge->property("text").toString(), QStringLiteral("Missing Required Files"));
}

QTEST_MAIN(Test_LocalModalManifestCard)
#include "test_LocalModalManifestCard.moc"

