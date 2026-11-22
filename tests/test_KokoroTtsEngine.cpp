/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_KokoroTtsEngine.cpp
** Unit test suite for KokoroTtsEngine under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include "app/engine/local/kokorottsengine.h"

class Test_KokoroTtsEngine : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Verification of Driver Identity
    void testDriverMetadata();

    // Voice Profiles & Options
    void testVoiceListAndDefaultOptions();
    void testActiveVoiceChange();
    void testOptionMutation();

    // Lifecycle & Guard Conditions
    void testLoadMissingModelFails();
    void testUnloadClearsState();

private:
    QString m_testSessionDir;
};

void Test_KokoroTtsEngine::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_KokoroTtsEngine::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_KokoroTtsEngine::testDriverMetadata()
{
    KokoroTtsEngine engine;
    QCOMPARE(engine.driverId(), QStringLiteral("kokoro_tts"));
    QCOMPARE(engine.targetAssetType(), QStringLiteral("audio"));
    QVERIFY2(!engine.isModelLoaded(), "Kokoro engine initialized with an unexpected loaded state.");
    QVERIFY2(!engine.isBusy(), "Kokoro engine initialized in a busy state.");
}

void Test_KokoroTtsEngine::testVoiceListAndDefaultOptions()
{
    KokoroTtsEngine engine;
    QStringList voices = engine.availableVoices();
    QVERIFY(!voices.isEmpty());
    QVERIFY(voices.contains(QStringLiteral("af_bella")));
    QCOMPARE(engine.activeVoice(), QStringLiteral("af_bella"));

    QVariantMap opts = engine.currentOptions();
    QCOMPARE(opts.value(QStringLiteral("voice")).toString(), QStringLiteral("af_bella"));
    QCOMPARE(opts.value(QStringLiteral("speed")).toFloat(), 1.0f);
}

void Test_KokoroTtsEngine::testActiveVoiceChange()
{
    KokoroTtsEngine engine;
    QSignalSpy spy(&engine, &KokoroTtsEngine::activeVoiceChanged);

    engine.setActiveVoice(QStringLiteral("bm_george"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(engine.activeVoice(), QStringLiteral("bm_george"));

    // Setting an unknown voice should be a no-op
    engine.setActiveVoice(QStringLiteral("unknown_voice_id"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(engine.activeVoice(), QStringLiteral("bm_george"));
}

void Test_KokoroTtsEngine::testOptionMutation()
{
    KokoroTtsEngine engine;
    QSignalSpy spy(&engine, &KokoroTtsEngine::optionsChanged);

    bool ok = engine.setOption(QStringLiteral("speed"), 1.25f);
    QVERIFY(ok);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(engine.currentOptions().value(QStringLiteral("speed")).toFloat(), 1.25f);
}

void Test_KokoroTtsEngine::testLoadMissingModelFails()
{
    KokoroTtsEngine engine;
    QSignalSpy errorSpy(&engine, &KokoroTtsEngine::engineError);

    QVariantMap config;
    config[QStringLiteral("modelDir")] = m_testSessionDir + QStringLiteral("/missing_kokoro_dir");

    bool loaded = engine.loadModel(config);
    QVERIFY2(!loaded, "Engine Vulnerability: Kokoro returned true when loading a non-existent directory.");
    QVERIFY2(!engine.isModelLoaded(), "Engine State Corrupted: isModelLoaded() is true despite failing to load files.");
    QCOMPARE(errorSpy.count(), 1);
}

void Test_KokoroTtsEngine::testUnloadClearsState()
{
    KokoroTtsEngine engine;
    engine.unloadModel();

    QVERIFY(!engine.isModelLoaded());
    QVERIFY(!engine.isBusy());
    QVERIFY(engine.loadedModelName().isEmpty());
}

QTEST_MAIN(Test_KokoroTtsEngine)
#include "test_KokoroTtsEngine.moc"

