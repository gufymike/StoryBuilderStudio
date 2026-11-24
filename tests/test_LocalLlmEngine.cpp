// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include "app/engine/local/localllmengine.h"

class Test_LocalLlmEngine : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Interface & Metadata
    void testDriverMetadata();
    void testOptionsManifestAndMutation();
    void testContextLimitBounds();

    // Memory & History Operations
    void testChatMlFormattingAndHistoryPruning();
    void testResetConversation();

    // Failure Modes & Cleanup
    void testLoadMissingFileEmitsError();
    void testUnloadModelFreesResources();

private:
    QString m_testSessionDir;
};

void Test_LocalLlmEngine::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_LocalLlmEngine::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_LocalLlmEngine::testDriverMetadata()
{
    LocalLlmEngine engine;
    QCOMPARE(engine.driverId(), QStringLiteral("llama_cpp"));
    QCOMPARE(engine.targetAssetType(), QStringLiteral("script"));
    QVERIFY2(!engine.isModelLoaded(), "LLM engine initialized with an unexpected loaded state.");
    QVERIFY2(engine.loadedModelName().isEmpty(), "LLM engine initialized with a populated model name.");
    QVERIFY(engine.loadedModelName().isEmpty());
}

void Test_LocalLlmEngine::testOptionsManifestAndMutation()
{
    LocalLlmEngine engine;
    QVariantList options = engine.availableOptions();
    QVERIFY(!options.isEmpty());

    // Validate temperature option schema
    bool foundTemp = false;
    for (const QVariant &item : options) {
        QVariantMap map = item.toMap();
        if (map.value(QStringLiteral("key")).toString() == QStringLiteral("temperature")) {
            foundTemp = true;
            QCOMPARE(map.value(QStringLiteral("type")).toString(), QStringLiteral("float"));
            break;
        }
    }
    QVERIFY(foundTemp);

    QSignalSpy spy(&engine, &LocalLlmEngine::optionsChanged);
    bool set = engine.setOption(QStringLiteral("temperature"), 1.25);
    QVERIFY(set);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(engine.currentOptions().value(QStringLiteral("temperature")).toDouble(), 1.25);
}

void Test_LocalLlmEngine::testContextLimitBounds()
{
    LocalLlmEngine engine;
    QSignalSpy spy(&engine, &LocalLlmEngine::contextLimitChanged);

    engine.setContextLimit(8192);
    QCOMPARE(engine.contextLimit(), 8192);
    QCOMPARE(spy.count(), 1);

    // Below minimum guardrail should be ignored
    engine.setContextLimit(256);
    QCOMPARE(engine.contextLimit(), 8192);
}

void Test_LocalLlmEngine::testChatMlFormattingAndHistoryPruning()
{
    LocalLlmEngine engine;
    engine.setContextLimit(1024); // Low ceiling to test pruning budget

    QString prompt1 = engine.buildChatMlPrompt(QStringLiteral("Hello world"), QStringLiteral("You are helpful."));
    QVERIFY(prompt1.contains(QStringLiteral("<|im_start|>system\nYou are helpful.<|im_end|>")));
    QVERIFY(prompt1.contains(QStringLiteral("<|im_start|>user\nHello world<|im_end|>")));
    QVERIFY(prompt1.endsWith(QStringLiteral("<|im_start|>assistant\n")));

    // Inject turns to force context pruning
    for (int i = 0; i < 20; ++i) {
        engine.buildChatMlPrompt(QStringLiteral("This is an extended repetitive conversation turn to consume context memory."), QStringLiteral("You are helpful."));
    }

    // Must not grow indefinitely; old history pruned
    QVERIFY2(engine.conversationHistory().size() < 25,
             "Memory Leak / Budget Failure: LocalLlmEngine failed to prune context history within the token budget limits.");
}

void Test_LocalLlmEngine::testResetConversation()
{
    LocalLlmEngine engine;
    engine.buildChatMlPrompt(QStringLiteral("Turn 1"), QString());
    QVERIFY(!engine.conversationHistory().isEmpty());

    engine.resetConversation();
    QVERIFY(engine.conversationHistory().isEmpty());
}

void Test_LocalLlmEngine::testLoadMissingFileEmitsError()
{
    LocalLlmEngine engine;
    QSignalSpy errorSpy(&engine, &LocalLlmEngine::engineError);

    QVariantMap config;
    config[QStringLiteral("modelPath")] = m_testSessionDir + QStringLiteral("/non_existent_model.gguf");

    bool result = engine.loadModel(config);
    QVERIFY2(!result, "Engine Vulnerability: llama.cpp driver returned true when loading a non-existent model.");
    QVERIFY2(!engine.isModelLoaded(), "Engine State Corrupted: isModelLoaded() is true despite failing to load files.");
    QCOMPARE(errorSpy.count(), 1);
}

void Test_LocalLlmEngine::testUnloadModelFreesResources()
{
    LocalLlmEngine engine;
    engine.unloadModel();
    QVERIFY(!engine.isModelLoaded());
    QVERIFY(engine.loadedModelName().isEmpty());
}

QTEST_MAIN(Test_LocalLlmEngine)
#include "test_LocalLlmEngine.moc"

