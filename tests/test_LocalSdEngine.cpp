// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include "app/engine/local/localsdengine.h"

class Test_LocalSdEngine : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Mode & Metadata Inspection
    void testInitialStateMode();

    // Options Management
    void testImageOptionsSchema();
    void testVideoOptionsSchema();
    void testOptionMutation();
    void testGenerateWhenUnloadedEmitsError();

    // Load & Resource Lifecycle
    void testLoadMissingWeightsEmitsError();
    void testUnloadClearsContextState();


private:
    QString m_testSessionDir;
};

void Test_LocalSdEngine::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_LocalSdEngine::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_LocalSdEngine::testInitialStateMode()
{
    LocalSdEngine engine;
    if(!engine.isVideoMode()) {
        QVERIFY2(!engine.isModelLoaded(), "SD Engine (Image) initialized with an unexpected loaded state.");
        QCOMPARE(engine.driverId(), QStringLiteral("sd_image"));
        QCOMPARE(engine.displayName(), QStringLiteral("Stable-Diffusion (Image)"));
        QCOMPARE(engine.targetAssetType(), QStringLiteral("image"));
        QVERIFY(!engine.isVideoMode());
        QVERIFY(!engine.isModelLoaded());
        QVERIFY(!engine.isBusy());
        QCOMPARE(engine.activeVideoArch(), LocalSdEngine::VIDEO_ARCH_NONE);

    } else {
        QVERIFY2(!engine.isModelLoaded(), "SD Engine (Video) initialized with an unexpected loaded state.");
        QCOMPARE(engine.driverId(), QStringLiteral("sd_video"));
        QCOMPARE(engine.displayName(), QStringLiteral("Stable-Diffusion (Video)"));
        QCOMPARE(engine.targetAssetType(), QStringLiteral("video"));
        QVERIFY(engine.isVideoMode());
        QVERIFY(!engine.isModelLoaded());
        QVERIFY(!engine.isBusy());
        QCOMPARE(engine.activeVideoArch(), LocalSdEngine::VIDEO_ARCH_NONE);
    }
}

void Test_LocalSdEngine::testImageOptionsSchema()
{
    LocalSdEngine engine;
    // if video, no need to run.
    if(engine.isVideoMode()) { return; }
    QVariantList options = engine.availableOptions();
    QVERIFY(!options.isEmpty());

    bool hasSampleCount = false;
    bool hasVideoFrames = false;

    for (const QVariant &item : options) {
        QString key = item.toMap().value(QStringLiteral("key")).toString();
        if (key == QStringLiteral("sample_count")) hasSampleCount = true;
        if (key == QStringLiteral("video_frames")) hasVideoFrames = true;
    }

    QVERIFY(hasSampleCount);
    QVERIFY(!hasVideoFrames);
}

void Test_LocalSdEngine::testVideoOptionsSchema()
{
    LocalSdEngine engine;
    // if it's image, no need to run
    if(!engine.isVideoMode()) { return; }
    QVariantList options = engine.availableOptions();
    QVERIFY(!options.isEmpty());

    bool hasVideoFrames = false;
    bool hasFlowShift = false;

    for (const QVariant &item : options) {
        QString key = item.toMap().value(QStringLiteral("key")).toString();
        if (key == QStringLiteral("video_frames")) hasVideoFrames = true;
        if (key == QStringLiteral("flow_shift"))   hasFlowShift = true;
    }

    QVERIFY(hasVideoFrames);
    QVERIFY(hasFlowShift);
}

void Test_LocalSdEngine::testOptionMutation()
{
    LocalSdEngine engine;
    QSignalSpy spy(&engine, &LocalSdEngine::optionsChanged);

    bool set = engine.setOption(QStringLiteral("steps"), 12);
    QVERIFY(set);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(engine.currentOptions().value(QStringLiteral("steps")).toInt(), 12);
}

void Test_LocalSdEngine::testLoadMissingWeightsEmitsError()
{
    LocalSdEngine engine;
    QSignalSpy errorSpy(&engine, &LocalSdEngine::engineError);

    QVariantMap config;
    config[QStringLiteral("diffusionModelPath")] = m_testSessionDir + QStringLiteral("/missing_model.safetensors");

    bool ok = engine.loadModel(config);
    QVERIFY(!ok);
    QVERIFY(!engine.isModelLoaded());
    QCOMPARE(errorSpy.count(), 1);
}

void Test_LocalSdEngine::testUnloadClearsContextState()
{
    LocalSdEngine engine;
    engine.unloadModel();

    QVERIFY(!engine.isModelLoaded());
#ifdef SD_MODE_VIDEO
    QVERIFY(engine.isVideoMode());
#else
    QVERIFY(!engine.isVideoMode());
#endif
    QCOMPARE(engine.activeVideoArch(), LocalSdEngine::VIDEO_ARCH_NONE);
    QVERIFY(engine.loadedModelName().isEmpty());
}

void Test_LocalSdEngine::testGenerateWhenUnloadedEmitsError()
{
    LocalSdEngine engine;
    QSignalSpy errorSpy(&engine, &LocalSdEngine::engineError);
    QSignalSpy finishedSpy(&engine, &LocalSdEngine::generationFinished);

    engine.generate(QStringLiteral("a futuristic skyline"), QString(), {}, QString(), {});

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);
}

QTEST_MAIN(Test_LocalSdEngine)
#include "test_LocalSdEngine.moc"

