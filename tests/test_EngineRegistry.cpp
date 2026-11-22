/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_EngineRegistry.cpp
** Test suite for EngineRegistry under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QPointer>
#include "app/engine/engineregistry.h"

// Concrete mock implementation satisfying the full AbstractInferenceDriver contract
class MockDriver : public AbstractInferenceDriver
{
    Q_OBJECT
public:
    MockDriver(const QString &id, const QString &modality, const QString &displayName, QObject *parent = nullptr)
        : AbstractInferenceDriver(parent)
        , m_id(id)
        , m_modality(modality)
        , m_name(displayName)
        , m_loaded(false)
        , m_busy(false)
    {}

    QString driverId() const override { return m_id; }
    QString displayName() const override { return m_name; }
    QString targetAssetType() const override { return m_modality; }
    bool isModelLoaded() const override { return m_loaded; }
    bool isBusy() const override { return m_busy; }
    QString loadedModelName() const override { return m_loadedModelName; }

    bool loadModel(const QVariantMap &config) override {
        m_loaded = true;
        m_loadedModelName = config.value(QStringLiteral("modelName"), m_id).toString();
        emit modelLoadedChanged(m_loaded);
        emit loadedModelNameChanged(m_loadedModelName);
        return true;
    }

    void unloadModel() override {
        if (m_loaded) {
            m_loaded = false;
            m_loadedModelName.clear();
            emit modelLoadedChanged(m_loaded);
            emit loadedModelNameChanged(m_loadedModelName);
        }
    }

    void cancelGeneration() override {
        m_busy = false;
        emit busyChanged(m_busy);
    }

    QVariantList availableOptions() const override { return m_optionsList; }
    QVariantMap currentOptions() const override { return m_optionsMap; }

    bool setOption(const QString &key, const QVariant &value) override {
        m_optionsMap[key] = value;
        emit optionsChanged();
        return true;
    }

    void generate(const QString &prompt,
                  const QString &systemPrompt = QString(),
                  const QStringList &attachedFiles = QStringList(),
                  const QString &outputDirectory = QString(),
                  const QVariantMap &runtimeOverrides = QVariantMap()) override
    {
        Q_UNUSED(systemPrompt);
        Q_UNUSED(attachedFiles);
        Q_UNUSED(outputDirectory);
        Q_UNUSED(runtimeOverrides);

        m_busy = true;
        emit busyChanged(m_busy);

        QVariantMap payload;
        payload[QStringLiteral("echo")] = prompt;
        emit generationFinished(payload);

        m_busy = false;
        emit busyChanged(m_busy);
    }

    Q_INVOKABLE void verifyConnection() override {
            emit connectionVerified(m_loaded, m_loaded ? tr("Model is loaded.") : tr("Model is not loaded."));
    }

private:
    QString m_id;
    QString m_modality;
    QString m_name;
    bool m_loaded;
    bool m_busy;
    QString m_loadedModelName;
    QVariantList m_optionsList;
    QVariantMap m_optionsMap;
};

class Test_EngineRegistry : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Registry lifecycle & routing tests
    void testRegistrationAndModalityQuery();
    void testActiveDriverSwitching();
    void testRejectsModalityMismatch();
    void testUnregisterDriverSwitchesActiveFallback();
    void testDriverOwnershipAndLifecycleCleanup();
    void testPluginStatusesReportAllExpectedPlugins();
    void testActiveDriversFallbackToRestWhenNoLocalPlugins();
    void testHasLocalPluginForCategoryWhenNoneLoaded();
};

void Test_EngineRegistry::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));
}

void Test_EngineRegistry::cleanupTestCase()
{
}

void Test_EngineRegistry::testRegistrationAndModalityQuery()
{
    EngineRegistry registry;
    registry.clearAllDrivers();

    QSignalSpy spy(&registry, &EngineRegistry::driverRegistered);
    QSignalSpy regListSpy(&registry, &EngineRegistry::driversRegistered);

    MockDriver *llama = new MockDriver(QStringLiteral("llama_cpp"), QStringLiteral("script"), QStringLiteral("LlamaCPP"));
    MockDriver *flux  = new MockDriver(QStringLiteral("sd_image"),  QStringLiteral("image"), QStringLiteral("Standard-Diffusion"));

    registry.registerDriver(llama);
    registry.registerDriver(flux);

    QCOMPARE(spy.count(), 2);
    QCOMPARE(regListSpy.count(), 2);

    QCOMPARE(registry.availableDriversForAsset(QStringLiteral("script")).size(), 1);
    QCOMPARE(registry.availableDriversForAsset(QStringLiteral("image")).size(), 1);
    QCOMPARE(registry.availableDriversForAsset(QStringLiteral("video")).size(), 0);

    QCOMPARE(registry.getDriver(QStringLiteral("llama_cpp")), llama);
    QCOMPARE(registry.activeScriptDriver(), llama);
}

void Test_EngineRegistry::testActiveDriverSwitching()
{
    EngineRegistry registry;
    registry.clearAllDrivers();

    MockDriver *kokoro = new MockDriver(QStringLiteral("kokoro_tts"), QStringLiteral("audio"), QStringLiteral("Kokoro"));
    MockDriver *audioCpp = new MockDriver(QStringLiteral("audio_cpp"), QStringLiteral("audio"), QStringLiteral("AudioCPP"));

    registry.registerDriver(kokoro);
    registry.registerDriver(audioCpp);

    QCOMPARE(registry.getActiveDriverId(QStringLiteral("audio")), QStringLiteral("kokoro_tts"));

    QSignalSpy spy(&registry, &EngineRegistry::activeDriversChanged);
    bool switched = registry.setActiveDriver(QStringLiteral("audio"), QStringLiteral("audio_cpp"));

    QVERIFY(switched);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(registry.getActiveDriverId(QStringLiteral("audio")), QStringLiteral("audio_cpp"));
    QCOMPARE(registry.activeAudioDriver(), audioCpp);
}

void Test_EngineRegistry::testRejectsModalityMismatch()
{
    EngineRegistry registry;
    registry.clearAllDrivers();

    MockDriver *flux = new MockDriver(QStringLiteral("sd_image"), QStringLiteral("image"), QStringLiteral("Standard-Diffusion"));
    registry.registerDriver(flux);

    // Attempting to register an image driver as a script driver must fail
    bool switched = registry.setActiveDriver(QStringLiteral("script"), QStringLiteral("sd_image"));
    QVERIFY2(!switched, "Registry Vulnerability: Incorrectly allowed an image driver to be set as the active script driver.");
    QVERIFY2(registry.activeScriptDriver() == nullptr, "Registry State Corrupted: Script driver pointer was set despite modality mismatch.");
}


void Test_EngineRegistry::testPluginStatusesReportAllExpectedPlugins()
{
    EngineRegistry registry;
    const QVariantList statuses = registry.pluginStatuses();

    QCOMPARE(statuses.size(), 5);

    const QStringList expectedIds = {
        QStringLiteral("LocalLlmEngine"),
        QStringLiteral("KokoroTtsEngine"),
        QStringLiteral("LocalAudioEngine"),
        QStringLiteral("LocalSdImagePlugin"),
        QStringLiteral("LocalSdVideoPlugin")
    };

    QStringList actualIds;
    for (const QVariant &entry : statuses) {
        const QVariantMap map = entry.toMap();
        QVERIFY2(map.contains(QStringLiteral("id")), "Plugin status entry missing 'id' key.");
        QVERIFY2(map.contains(QStringLiteral("label")), "Plugin status entry missing 'label' key.");
        QVERIFY2(map.contains(QStringLiteral("available")), "Plugin status entry missing 'available' key.");
        actualIds.append(map.value(QStringLiteral("id")).toString());
    }

    QCOMPARE(actualIds, expectedIds);
}

void Test_EngineRegistry::testActiveDriversFallbackToRestWhenNoLocalPlugins()
{
    EngineRegistry registry;

    QVERIFY2(registry.activeScriptDriver() != nullptr, "activeScriptDriver() fell back to nullptr.");
    QVERIFY2(registry.activeImageDriver() != nullptr,  "activeImageDriver() fell back to nullptr.");
    QVERIFY2(registry.activeAudioDriver() != nullptr,  "activeAudioDriver() fell back to nullptr.");
    QVERIFY2(registry.activeVideoDriver() != nullptr,  "activeVideoDriver() fell back to nullptr.");
}

void Test_EngineRegistry::testHasLocalPluginForCategoryWhenNoneLoaded()
{
    EngineRegistry registry;
    if (registry.hasLocalPlugins()) {
        QSKIP("Local plugins are present in the test environment; skipping REST-only assertion.");
    }

    QCOMPARE(registry.hasLocalPluginForCategory(QStringLiteral("script")), false);
    QCOMPARE(registry.hasLocalPluginForCategory(QStringLiteral("text")),   false);
    QCOMPARE(registry.hasLocalPluginForCategory(QStringLiteral("image")),  false);
    QCOMPARE(registry.hasLocalPluginForCategory(QStringLiteral("audio")),  false);
    QCOMPARE(registry.hasLocalPluginForCategory(QStringLiteral("video")),  false);
}


void Test_EngineRegistry::testUnregisterDriverSwitchesActiveFallback()
{
    EngineRegistry registry;
    registry.clearAllDrivers();

    MockDriver *d1 = new MockDriver(QStringLiteral("wan_video"), QStringLiteral("video"), QStringLiteral("WAN"));
    MockDriver *d2 = new MockDriver(QStringLiteral("ltx_video"), QStringLiteral("video"), QStringLiteral("LTX"));

    registry.registerDriver(d1);
    registry.registerDriver(d2);

    registry.setActiveDriver(QStringLiteral("video"), QStringLiteral("wan_video"));
    QCOMPARE(registry.getActiveDriverId(QStringLiteral("video")), QStringLiteral("wan_video"));

    registry.unregisterDriver(QStringLiteral("wan_video"));
    QVERIFY2(registry.getDriver(QStringLiteral("wan_video")) == nullptr, "Unregister Failure: Driver remained in memory after unregistering.");

    // Active driver should fall back to remaining registered video driver
    QCOMPARE(registry.getActiveDriverId(QStringLiteral("video")), QStringLiteral("ltx_video"));
}

void Test_EngineRegistry::testDriverOwnershipAndLifecycleCleanup()
{
    QPointer<MockDriver> weakPtr;
    {
        EngineRegistry localRegistry;
        MockDriver *driver = new MockDriver(QStringLiteral("temp_driver"), QStringLiteral("script"), QStringLiteral("Temp"));
        driver->loadModel(QVariantMap());
        weakPtr = driver;

        localRegistry.registerDriver(driver);
        QVERIFY2(!weakPtr.isNull(), "Test setup failed: Weak pointer to mock driver is invalid.");
        QVERIFY2(weakPtr->isModelLoaded(), "Test setup failed: Mock driver did not load.");
    } // localRegistry destroyed here

    // Registry destructor must unload and schedule cleanup of its drivers
    QVERIFY2(weakPtr.isNull() || !weakPtr->isModelLoaded(),
             "Memory Leak / Lifecycle Failure: Registry did not unload or delete owned drivers upon destruction.");
}


QTEST_MAIN(Test_EngineRegistry)
#include "test_EngineRegistry.moc"

