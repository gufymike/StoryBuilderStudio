/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_LocalAudioEngine.cpp
** Unit test suite for LocalAudioEngine under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include "app/engine/local/localaudioengine.h"

class Test_LocalAudioEngine : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Verification of Driver Identity
    void testDriverMetadata();

    // Configuration & Options
    void testDefaultOptions();
    void testOptionMutation();

    // Lifecycle & Guard Conditions
    void testLoadMissingModelFails();
    void testUnloadClearsState();

private:
    QString m_testSessionDir;
};

void Test_LocalAudioEngine::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_LocalAudioEngine::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_LocalAudioEngine::testDriverMetadata()
{
    LocalAudioEngine engine;
    QCOMPARE(engine.driverId(), QStringLiteral("audio_cpp"));
    QCOMPARE(engine.targetAssetType(), QStringLiteral("audio"));
    QVERIFY(!engine.isModelLoaded());
    QVERIFY(!engine.isBusy());
}

void Test_LocalAudioEngine::testDefaultOptions()
{
    LocalAudioEngine engine;
    QVariantMap opts = engine.currentOptions();
    QCOMPARE(opts.value(QStringLiteral("seconds")).toString(), QStringLiteral("5.0"));
    QCOMPARE(opts.value(QStringLiteral("steps")).toString(), QStringLiteral("25"));
}

void Test_LocalAudioEngine::testOptionMutation()
{
    LocalAudioEngine engine;
    QSignalSpy spy(&engine, &LocalAudioEngine::optionsChanged);

    bool ok = engine.setOption(QStringLiteral("steps"), QStringLiteral("40"));
    QVERIFY(ok);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(engine.currentOptions().value(QStringLiteral("steps")).toString(), QStringLiteral("40"));
}

void Test_LocalAudioEngine::testLoadMissingModelFails()
{
    LocalAudioEngine engine;
    QSignalSpy errorSpy(&engine, &LocalAudioEngine::engineError);

    QVariantMap config;
    config[QStringLiteral("modelPath")] = m_testSessionDir + QStringLiteral("/non_existent_audio.gguf");

    bool loaded = engine.loadModel(config);
    QVERIFY2(!loaded, "Engine Vulnerability: AudioCPP returned true when loading a non-existent .gguf file.");
    QVERIFY2(!engine.isModelLoaded(), "Engine State Corrupted: isModelLoaded() is true despite failing to load files.");
    QCOMPARE(errorSpy.count(), 1);
}

void Test_LocalAudioEngine::testUnloadClearsState()
{
    LocalAudioEngine engine;
    engine.unloadModel();

    QVERIFY(!engine.isModelLoaded());
    QVERIFY(!engine.isBusy());
    QVERIFY(engine.loadedModelName().isEmpty());
}

QTEST_MAIN(Test_LocalAudioEngine)
#include "test_LocalAudioEngine.moc"

