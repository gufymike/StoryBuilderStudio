/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_RemoteApiConfigCard.cpp
** Unit test suite for RemoteApiConfigCard QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"

class MockVerifyDriver : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connectionVerifiedState READ isVerified NOTIFY verificationInfoChanged)
    Q_PROPERTY(QString verificationSummary READ summary NOTIFY verificationInfoChanged)
public:
    bool isVerified() const { return m_verified; }
    QString summary() const { return m_summary; }
    void setStatus(bool ok, const QString &msg) {
        m_verified = ok;
        m_summary = msg;
        emit verificationInfoChanged();
    }
signals:
    void verificationInfoChanged();
private:
    bool m_verified = false;
    QString m_summary;
};

class MockVerifyRegistry : public QObject
{
    Q_OBJECT
public:
    explicit MockVerifyRegistry(QObject *parent = nullptr) : QObject(parent) {
        driver.setParent(this);
    }
    MockVerifyDriver driver;
    Q_INVOKABLE QObject* getDriver(const QString &) { return &driver; }
};

class Test_RemoteApiConfigCard : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testGeminiBridgeDefaults();
    void testOpenAiBridgeDefaults();
    void testOllamaLocalDefaults();
    void testCustomDriverValidation();
    void testVerificationSummaryPropagatesFromDriver();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *card = nullptr;

        bool init(const QString &sessionDir, QObject *registry = nullptr) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/settings"));
            if (registry) engine.rootContext()->setContextProperty(QStringLiteral("engineRegistry"), registry);
            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 600\n"
                "    height: 400\n"
                "    visible: true\n"
                "    RemoteApiConfigCard {\n"
                "        id: remoteCard\n"
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

            card = window->findChild<QObject*>(UI::RemoteConfig::Root);
            return (card != nullptr);
        }
    };
};

void Test_RemoteApiConfigCard::initTestCase()
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

void Test_RemoteApiConfigCard::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_RemoteApiConfigCard::testGeminiBridgeDefaults()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *card = harness.card;
    QVERIFY(card != nullptr);

    card->setProperty("driverId", QStringLiteral("gemini_bridge"));
    QCoreApplication::processEvents();

    QCOMPARE(card->property("targetEnvName").toString(), QStringLiteral("GEMINI_API_KEY"));
    QCOMPARE(card->property("baseUrl").toString(), QStringLiteral("https://generativelanguage.googleapis.com"));
    QCOMPARE(card->property("modelIdentifier").toString(), QStringLiteral("gemini-3.5-flash-lite"));

    QObject *customSection = card->findChild<QObject*>(UI::RemoteConfig::CustomEnvSection);
    QVERIFY2(customSection != nullptr, "UI Component Missing: 'customEnvSection' not found.");
    QVERIFY2(!customSection->property("visible").toBool(), "Custom environment section appears for Gemini Bridge.");
}

void Test_RemoteApiConfigCard::testOpenAiBridgeDefaults()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *card = harness.card;

    card->setProperty("driverId", QStringLiteral("openai_bridge"));
    QCoreApplication::processEvents();

    QCOMPARE(card->property("targetEnvName").toString(), QStringLiteral("OPENAI_API_KEY"));
    QCOMPARE(card->property("baseUrl").toString(), QStringLiteral("https://api.openai.com/v1"));
    QCOMPARE(card->property("modelIdentifier").toString(), QStringLiteral("gpt-4o"));
}

void Test_RemoteApiConfigCard::testOllamaLocalDefaults()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *card = harness.card;

    card->setProperty("driverId", QStringLiteral("ollama_local"));
    QCoreApplication::processEvents();

    QCOMPARE(card->property("isKeyConfigured").toBool(), true);
    QCOMPARE(card->property("baseUrl").toString(), QStringLiteral("http://localhost:11434/v1"));
    QCOMPARE(card->property("modelIdentifier").toString(), QStringLiteral("qwen2.5:7b"));

    QObject *setupBtn = card->findChild<QObject*>(UI::RemoteConfig::SetupGuideBtn);
    QVERIFY2(setupBtn != nullptr, "UI Component Missing: 'setupGuideBtn' not found.");
    QCOMPARE(setupBtn->property("visible").toBool(), false);
}

void Test_RemoteApiConfigCard::testCustomDriverValidation()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *card = harness.card;

    card->setProperty("driverId", QStringLiteral("custom_rest"));
    QCoreApplication::processEvents();

    QObject *customSection = card->findChild<QObject*>(UI::RemoteConfig::CustomEnvSection);
    QVERIFY2(customSection != nullptr, "UI Component Missing: 'customEnvSection' not found.");
    QVERIFY2(customSection->property("visible").toBool(), "Custom environment section failed to appear for custom driver.");

    QObject *customEnvField = card->findChild<QObject*>(UI::RemoteConfig::CustomEnvField);
    QVERIFY2(customEnvField != nullptr, "UI Component Missing: 'customEnvField' not found.");

    // Invalid env name (missing _API_KEY or _TOKEN)
    customEnvField->setProperty("text", QStringLiteral("MY_ENDPOINT"));
    QCOMPARE(customEnvField->property("acceptableInput").toBool(), false);

    // Valid env name
    customEnvField->setProperty("text", QStringLiteral("MISTRAL_API_KEY"));
    QCOMPARE(customEnvField->property("acceptableInput").toBool(), true);
    QCOMPARE(card->property("targetEnvName").toString(), QStringLiteral("MISTRAL_API_KEY"));
}

void Test_RemoteApiConfigCard::testVerificationSummaryPropagatesFromDriver()
{
    MockVerifyRegistry mockReg;
    mockReg.driver.setStatus(true, QStringLiteral("Verified — 12 Gemini models available"));

    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &mockReg), "Failed to load RemoteApiConfigCard with mock registry.");

    QObject *title    = harness.card->findChild<QObject*>(UI::RemoteConfig::BannerTitle);
    QObject *subtitle = harness.card->findChild<QObject*>(UI::RemoteConfig::BannerSubtitle);
    QVERIFY2(title && subtitle, "Banner title or subtitle missing.");

    QCOMPARE(title->property("text").toString(), QStringLiteral("API Key Verified"));
    QCOMPARE(subtitle->property("text").toString(), QStringLiteral("Verified — 12 Gemini models available"));
}

QTEST_MAIN(Test_RemoteApiConfigCard)
#include "test_RemoteApiConfigCard.moc"

