/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_SettingsManager.cpp
** Test suite for SettingsManager subsystem under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include "app/settings/settingsmanager.h"

class Test_SettingsManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Directory & Model Architecture
    void testInitialFirstRunFlag();
    void testSetBaseModelsDirPropagatesSubpaths();
    void testScanModelDirectoryCounts();

    // Providers Management
    void testAddAndRemoveCustomProvider();
    void testPreventDuplicateProviderNames();
    void testCustomProviderMetadataPersistence();
    void testCustomProviderRemovedSignal();
    void testSaveAndGetBuiltInRemoteDriverConfig();
    void testSaveAndGetCustomRemoteDriverConfig();

    // Security & Environment Guardrails
    void testResolveApiKeySecurityWhitelist();

    // File History & Persistence
    void testRecentFilesQueueLimits();
    void testClearRecentFiles();

private:
    QString m_testSessionDir;
    QString createScopedConfigPath();
    void createDummyFile(const QString &path);
};

void Test_SettingsManager::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    // Comply with guideline: StandardLocations::TempLocation/<uuid>/
    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);

    QDir().mkpath(m_testSessionDir);
}

void Test_SettingsManager::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_SettingsManager::init()
{
}

void Test_SettingsManager::cleanup()
{
}

QString Test_SettingsManager::createScopedConfigPath()
{
    QString subUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString dir = QDir::toNativeSeparators(m_testSessionDir + "/" + subUuid);
    QDir().mkpath(dir);
    return QDir::toNativeSeparators(dir + "/workspace_config.ini");
}

void Test_SettingsManager::createDummyFile(const QString &path)
{
    QFileInfo fi(path);
    QDir().mkpath(fi.dir().path());
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write("dummy");
        f.close();
    }
}

void Test_SettingsManager::testInitialFirstRunFlag()
{
    QString configPath = createScopedConfigPath();
    SettingsManager mgr(configPath);

    // Initial state must be true
    QVERIFY2(mgr.isFirstRun(), "SettingsManager did not default to FirstRun=true on a clean initialization.");

    QSignalSpy spy(&mgr, &SettingsManager::isFirstRunChanged);
    mgr.setIsFirstRun(false);

    QVERIFY(!mgr.isFirstRun());
    QCOMPARE(spy.count(), 1);

    // Verify written to disk config
    SettingsManager reloaded(configPath);
    QVERIFY(!reloaded.isFirstRun());
}

void Test_SettingsManager::testSetBaseModelsDirPropagatesSubpaths()
{
    QString configPath = createScopedConfigPath();
    SettingsManager mgr(configPath);

    QString customRoot = QDir::toNativeSeparators(m_testSessionDir + "/MockModelsRoot");
    QDir().mkpath(customRoot);

    QSignalSpy baseSpy(&mgr, &SettingsManager::baseModelsDirChanged);
    mgr.setBaseModelsDir(customRoot);

    QCOMPARE(baseSpy.count(), 1);
    QCOMPARE(mgr.baseModelsDir(), customRoot);
    QCOMPARE(mgr.scriptModelsDir(), QDir::toNativeSeparators(customRoot + "/text"));
    QCOMPARE(mgr.imagesRootDir(), QDir::toNativeSeparators(customRoot + "/images"));
    QCOMPARE(mgr.audioRootDir(), QDir::toNativeSeparators(customRoot + "/audio"));
    QCOMPARE(mgr.videoRootDir(), QDir::toNativeSeparators(customRoot + "/video"));
}

void Test_SettingsManager::testScanModelDirectoryCounts()
{
    QString configPath = createScopedConfigPath();
    SettingsManager mgr(configPath);

    QString customRoot = QDir::toNativeSeparators(m_testSessionDir + "/ScanTestModels");
    createDummyFile(customRoot + "/text/qwen.gguf");
    createDummyFile(customRoot + "/text/llama.bin");
    createDummyFile(customRoot + "/images/flux.safetensors");
    createDummyFile(customRoot + "/audio/speech/Kokoro/model.onnx");
    createDummyFile(customRoot + "/video/wan.safetensors");

    QString t5Path = customRoot + "/images/t5xxl.safetensors";
    createDummyFile(t5Path);

    QVariantMap results = mgr.scanModelDirectory(customRoot, t5Path);
    QCOMPARE(results.value("textCount").toInt(), 2);
    QCOMPARE(results.value("imageCount").toInt(), 2);
    QCOMPARE(results.value("videoCount").toInt(), 1);
    QVERIFY(results.value("hasT5").toBool());
}

void Test_SettingsManager::testAddAndRemoveCustomProvider()
{
    QString configPath = createScopedConfigPath();
    SettingsManager mgr(configPath);

    bool added = mgr.addCustomProvider(QStringLiteral("script"), QStringLiteral("Local Qwen REST"), QStringLiteral("http://localhost:8080/v1"));
    QVERIFY(added);

    QVariantList scriptProviders = mgr.scriptProviders();
    bool found = false;
    for (const QVariant &item : scriptProviders) {
        if (item.toMap().value("name").toString() == QStringLiteral("Local Qwen REST")) {
            found = true;
            break;
        }
    }
    QVERIFY(found);

    bool removed = mgr.removeCustomProvider(QStringLiteral("script"), QStringLiteral("Local Qwen REST"));
    QVERIFY(removed);

    scriptProviders = mgr.scriptProviders();
    found = false;
    for (const QVariant &item : scriptProviders) {
        if (item.toMap().value("name").toString() == QStringLiteral("Local Qwen REST")) {
            found = true;
            break;
        }
    }
    QVERIFY(!found);
}

void Test_SettingsManager::testPreventDuplicateProviderNames()
{
    QString configPath = createScopedConfigPath();
    SettingsManager mgr(configPath);

    QVERIFY(mgr.addCustomProvider(QStringLiteral("images"), QStringLiteral("Custom StableDiff"), QStringLiteral("http://localhost:7860")));
    // Adding with same name must fail
    QVERIFY(!mgr.addCustomProvider(QStringLiteral("images"), QStringLiteral("Custom StableDiff"), QStringLiteral("http://localhost:9000")));
}

void Test_SettingsManager::testResolveApiKeySecurityWhitelist()
{
    QString configPath = createScopedConfigPath();
    SettingsManager mgr(configPath);

    // Rule: Variable name must end with _API_KEY or _TOKEN
    QString invalidName = QStringLiteral("MY_SECRET_PASSWORD");
    QVERIFY2(mgr.resolveApiKey(QStringLiteral("custom"), invalidName).isEmpty(),
             "SECURITY FAILURE: Manager accepted an environment variable that didn't end in _API_KEY or _TOKEN.");

    // Rule: Blacklisted sensitive system vars must be rejected
    QVERIFY2(mgr.resolveApiKey(QStringLiteral("custom"), QStringLiteral("AWS_SECRET_ACCESS_KEY")).isEmpty(),
             "SECURITY FAILURE: Manager leaked a blacklisted system credential!");
    QVERIFY2(mgr.resolveApiKey(QStringLiteral("custom"), QStringLiteral("PATH")).isEmpty(),
             "SECURITY FAILURE: Manager leaked a blacklisted system credential!");
    QVERIFY2(mgr.resolveApiKey(QStringLiteral("custom"), QStringLiteral("GITHUB_TOKEN")).isEmpty(),
             "SECURITY FAILURE: Manager leaked a blacklisted system credential!");
}

void Test_SettingsManager::testRecentFilesQueueLimits()
{
    QString configPath = createScopedConfigPath();
    SettingsManager mgr(configPath);

    // Create 7 dummy files to test the max limit (5)
    QStringList createdPaths;
    for (int i = 1; i <= 7; ++i) {
        QString p = QDir::toNativeSeparators(m_testSessionDir + QString("/script_%1.md").arg(i));
        createDummyFile(p);
        createdPaths.append(p);
        mgr.addRecentFile(p);
    }

    QStringList recents = mgr.recentFiles();
    QCOMPARE(recents.size(), 5);

    // Most recent added should be first
    QCOMPARE(recents.first(), createdPaths.last());
}

void Test_SettingsManager::testClearRecentFiles()
{
    QString configPath = createScopedConfigPath();
    SettingsManager mgr(configPath);

    QString p = QDir::toNativeSeparators(m_testSessionDir + "/sample.md");
    createDummyFile(p);
    mgr.addRecentFile(p);
    QCOMPARE(mgr.recentFiles().size(), 1);

    mgr.clearRecentFiles();
    QCOMPARE(mgr.recentFiles().size(), 0);
}

void Test_SettingsManager::testCustomProviderMetadataPersistence()
{
    QString configPath = createScopedConfigPath();
    SettingsManager mgr(configPath);
    QSignalSpy netSpy(&mgr, &SettingsManager::apiNetworkSettingsChanged);

    bool added = mgr.addCustomProvider(
        QStringLiteral("video"),
        QStringLiteral("Local Video Server"),
        QStringLiteral("http://127.0.0.1:8080/v1"),
        QStringLiteral("wan-2.1"),
        QStringLiteral("my_video_api_key")); // Lowercase to verify automatic uppercasing

    QVERIFY2(added, "Failed to add custom video provider with modelName and envVar.");
    QCOMPARE(netSpy.count(), 1);

           // Reload from disk to confirm INI JSON persistence
    SettingsManager reloaded(configPath);
    QVariantList videoProviders = reloaded.videoProviders();

    bool found = false;
    for (const QVariant &item : videoProviders) {
        QVariantMap map = item.toMap();
        if (map.value(QStringLiteral("name")).toString() == QStringLiteral("Local Video Server")) {
            found = true;
            QCOMPARE(map.value(QStringLiteral("url")).toString(), QStringLiteral("http://127.0.0.1:8080/v1"));
            QCOMPARE(map.value(QStringLiteral("modelName")).toString(), QStringLiteral("wan-2.1"));
            QCOMPARE(map.value(QStringLiteral("envVar")).toString(), QStringLiteral("MY_VIDEO_API_KEY"));
            break;
        }
    }
    QVERIFY2(found, "Reloaded SettingsManager did not persist custom provider metadata.");
}

void Test_SettingsManager::testCustomProviderRemovedSignal()
{
    QString configPath = createScopedConfigPath();
    SettingsManager mgr(configPath);

    QVERIFY2(mgr.addCustomProvider(
                 QStringLiteral("images"),
                 QStringLiteral("Local SD Server"),
                 QStringLiteral("http://127.0.0.1:7860/v1"),
                 QStringLiteral("flux1-schnell")),
             "Setup failed: could not add custom image provider.");

    QSignalSpy removedSpy(&mgr, &SettingsManager::customProviderRemoved);
    QSignalSpy netSpy(&mgr, &SettingsManager::apiNetworkSettingsChanged);

    bool removed = mgr.removeCustomProvider(QStringLiteral("images"), QStringLiteral("Local SD Server"));
    QVERIFY2(removed, "removeCustomProvider() returned false for an existing provider.");
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(netSpy.count(), 1);

           // Verify category "images" normalized to "image" and name normalized to snake_case
    QCOMPARE(removedSpy.takeFirst().at(0).toString(), QStringLiteral("custom_image_local_sd_server"));
}

void Test_SettingsManager::testSaveAndGetBuiltInRemoteDriverConfig()
{
    QString configPath = createScopedConfigPath();
    SettingsManager mgr(configPath);
    QSignalSpy netSpy(&mgr, &SettingsManager::apiNetworkSettingsChanged);

           // Reject empty URL or model name
    QVERIFY2(!mgr.saveRemoteDriverConfig(QStringLiteral("script"), QStringLiteral("gemini_bridge"),
                                         QString(), QStringLiteral("gemini-3.5-flash-lite")),
             "saveRemoteDriverConfig() should reject an empty baseUrl.");
    QVERIFY2(!mgr.saveRemoteDriverConfig(QStringLiteral("script"), QStringLiteral("gemini_bridge"),
                                         QStringLiteral("https://generativelanguage.googleapis.com"), QString()),
             "saveRemoteDriverConfig() should reject an empty modelName.");
    QCOMPARE(netSpy.count(), 0);

           // Save valid built-in override
    bool saved = mgr.saveRemoteDriverConfig(
        QStringLiteral("script"),
        QStringLiteral("gemini_bridge"),
        QStringLiteral("https://custom.gemini.proxy/v1beta"),
        QStringLiteral("gemini-3.8-flash"));

    QVERIFY2(saved, "saveRemoteDriverConfig() failed for built-in driver gemini_bridge.");
    QCOMPARE(netSpy.count(), 1);

           // Reload from disk to confirm INI persistence
    SettingsManager reloaded(configPath);
    QVariantMap cfg = reloaded.getRemoteDriverConfig(QStringLiteral("script"), QStringLiteral("gemini_bridge"));

    QCOMPARE(cfg.value(QStringLiteral("baseUrl")).toString(), QStringLiteral("https://custom.gemini.proxy/v1beta"));
    QCOMPARE(cfg.value(QStringLiteral("modelName")).toString(), QStringLiteral("gemini-3.8-flash"));
}

void Test_SettingsManager::testSaveAndGetCustomRemoteDriverConfig()
{
    QString configPath = createScopedConfigPath();
    SettingsManager mgr(configPath);

    QVERIFY2(mgr.addCustomProvider(
                 QStringLiteral("images"),
                 QStringLiteral("Local SD Server"),
                 QStringLiteral("http://127.0.0.1:7860/v1"),
                 QStringLiteral("flux1-schnell"),
                 QString()),
             "Setup failed: could not add custom image provider.");

    const QString driverId = QStringLiteral("custom_image_local_sd_server");

           // Non-existent custom driver should return false
    QVERIFY2(!mgr.saveRemoteDriverConfig(
                 QStringLiteral("images"),
                 QStringLiteral("custom_image_does_not_exist"),
                 QStringLiteral("http://127.0.0.1:9999/v1"),
                 QStringLiteral("sdxl")),
             "saveRemoteDriverConfig() should return false for an unknown custom driverId.");

    QSignalSpy imgSpy(&mgr, &SettingsManager::imageProvidersChanged);
    QSignalSpy netSpy(&mgr, &SettingsManager::apiNetworkSettingsChanged);

           // Update existing custom provider (including lowercase envVar to verify uppercasing)
    bool updated = mgr.saveRemoteDriverConfig(
        QStringLiteral("images"),
        driverId,
        QStringLiteral("http://127.0.0.1:8188/v1"),
        QStringLiteral("flux1-dev"),
        QStringLiteral("local_sd_api_key"));

    QVERIFY2(updated, "saveRemoteDriverConfig() failed to update existing custom image provider.");
    QCOMPARE(imgSpy.count(), 1);
    QCOMPARE(netSpy.count(), 1);

           // Reload from disk and verify via getRemoteDriverConfig()
    SettingsManager reloaded(configPath);
    QVariantMap cfg = reloaded.getRemoteDriverConfig(QStringLiteral("images"), driverId);

    QCOMPARE(cfg.value(QStringLiteral("name")).toString(),      QStringLiteral("Local SD Server"));
    QCOMPARE(cfg.value(QStringLiteral("baseUrl")).toString(),   QStringLiteral("http://127.0.0.1:8188/v1"));
    QCOMPARE(cfg.value(QStringLiteral("modelName")).toString(), QStringLiteral("flux1-dev"));
    QCOMPARE(cfg.value(QStringLiteral("envVar")).toString(),    QStringLiteral("LOCAL_SD_API_KEY"));
}

QTEST_MAIN(Test_SettingsManager)
#include "test_SettingsManager.moc"

