// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include "engineregistry.h"

#include <QDebug>
#include <QPluginLoader>
#include <QDir>
#include <QCoreApplication>
#include "remote/genericrestdriver.h"

EngineRegistry::EngineRegistry(QObject *parent)
    : QObject(parent)
{
    m_activeDriverIds[QStringLiteral("script")] = QStringLiteral("llama_cpp");
    m_activeDriverIds[QStringLiteral("image")]  = QStringLiteral("sd_image");
    m_activeDriverIds[QStringLiteral("audio")]  = QStringLiteral("kokoro_tts");
    m_activeDriverIds[QStringLiteral("video")]  = QStringLiteral("sd_video");
    // 1. Manually instantiate lightweight network drivers
    registerDriver(new GenericRestDriver(QStringLiteral("gemini_bridge"), GenericRestDriver::Protocol_Gemini, QStringLiteral("script"), this));
    registerDriver(new GenericRestDriver(QStringLiteral("gemini_image"), GenericRestDriver::Protocol_Gemini, QStringLiteral("image"), this));
    registerDriver(new GenericRestDriver(QStringLiteral("gemini_audio"), GenericRestDriver::Protocol_Gemini, QStringLiteral("audio"), this));
    registerDriver(new GenericRestDriver(QStringLiteral("gemini_video"), GenericRestDriver::Protocol_Gemini, QStringLiteral("video"), this));
    registerDriver(new GenericRestDriver(QStringLiteral("openai_bridge"), GenericRestDriver::Protocol_OpenAI, QStringLiteral("script"), this));
    registerDriver(new GenericRestDriver(QStringLiteral("openai_image"), GenericRestDriver::Protocol_OpenAI, QStringLiteral("image"), this));
    registerDriver(new GenericRestDriver(QStringLiteral("openai_audio"), GenericRestDriver::Protocol_OpenAI, QStringLiteral("audio"), this));
    registerDriver(new GenericRestDriver(QStringLiteral("openai_video"), GenericRestDriver::Protocol_OpenAI, QStringLiteral("video"), this));
    registerDriver(new GenericRestDriver(QStringLiteral("ollama_local"), GenericRestDriver::Protocol_OpenAI, QStringLiteral("script"), this));

    loadPlugins();
    for (auto it = m_activeDriverIds.begin(); it != m_activeDriverIds.end(); ++it) {
        if (!m_drivers.contains(it.value())) {
            const QStringList alternates = availableDriversForAsset(it.key());
            it.value() = alternates.isEmpty() ? QString() : alternates.first();
        }
    }
    emit activeDriversChanged();
}

EngineRegistry::~EngineRegistry()
{
    clearAllDrivers();
}

void EngineRegistry::loadPlugins()
{
    // Locate the plugins directory relative to the application executable
    QDir pluginsDir(QCoreApplication::applicationDirPath());

    // In our CMake file, we output plugins to bin/plugins/drivers
#if defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif
    pluginsDir.cd(QStringLiteral("plugins"));
    pluginsDir.cd(QStringLiteral("drivers"));

    if (!pluginsDir.exists()) {
        qWarning() << "[EngineRegistry] Plugin directory not found at:" << pluginsDir.absolutePath();
        return;
    }

    qInfo() << "[EngineRegistry] Scanning for driver plugins in:" << pluginsDir.absolutePath();

    // Iterate over all files in the directory
    const QStringList entryList = pluginsDir.entryList(QDir::Files);
    bool changed = false;
    for (const QString &fileName : entryList) {
        // Skip obvious non-library files (like pdb on Windows)
        if (fileName.endsWith(QStringLiteral(".pdb")) || fileName.endsWith(QStringLiteral(".lib"))) {
            continue;
        }

        QString pluginPath = pluginsDir.absoluteFilePath(fileName);
        QPluginLoader loader(pluginPath);

        QObject *pluginInstance = loader.instance();

        if (pluginInstance) {
            // Attempt to cast the loaded QObject to our interface
            auto *driver = qobject_cast<AbstractInferenceDriver*>(pluginInstance);
            if (driver) {
                qInfo() << "[EngineRegistry] Successfully loaded plugin:" << fileName;
                changed = true;
                m_loadedPluginIds.insert(QFileInfo(fileName).baseName());
                // Hand the pointer over to the existing registration logic
                registerDriver(driver);
            } else {
                qWarning() << "[EngineRegistry] Plugin loaded but failed to cast to AbstractInferenceDriver:" << fileName;
                loader.unload();
            }
        } else {
            // Some DLLs might not be Qt plugins (e.g., third-party dependencies copied there)
            qDebug() << "[EngineRegistry] Ignored non-plugin file or failed to load:" << fileName
                     << "-" << loader.errorString();
        }
    }
    if(changed) { emit pluginsChanged(); }
}


void EngineRegistry::clearAllDrivers()
{
    for (auto it = m_drivers.begin(); it != m_drivers.end(); ++it) {
        AbstractInferenceDriver *driver = it.value();
        if (driver) {
            driver->unloadModel();
            driver->deleteLater();
        }
    }
    m_drivers.clear();
    m_activeDriverIds.clear();
}

void EngineRegistry::registerDriver(AbstractInferenceDriver *driver)
{
    if (!driver) {
        qWarning() << "[EngineRegistry] Attempted to register null driver pointer.";
        return;
    }

    const QString id = driver->driverId();
    if (id.isEmpty()) {
        qWarning() << "[EngineRegistry] Attempted to register driver with empty driverId.";
        return;
    }

    if (m_drivers.contains(id)) {
        qWarning() << "[EngineRegistry] Overwriting existing driver registration for:" << id;
        AbstractInferenceDriver *old = m_drivers.take(id);
        if (old && old != driver) {
            old->unloadModel();
            old->deleteLater();
        }
    }

    m_drivers.insert(id, driver);
    driver->setParent(this);

    qInfo() << "[EngineRegistry] Successfully registered driver:" << id
            << "for modality:" << driver->targetAssetType();

    emit driverRegistered(id);
    emit driversRegistered();

    const QString modality = driver->targetAssetType();
    if (!m_activeDriverIds.contains(modality) || m_activeDriverIds[modality].isEmpty()) {
        m_activeDriverIds[modality] = id;
        emit activeDriversChanged();
    }
}

void EngineRegistry::unregisterDriver(const QString &driverId)
{
    if (!m_drivers.contains(driverId)) return;

    AbstractInferenceDriver *driver = m_drivers.take(driverId);
    if (driver) {
        driver->unloadModel();
        driver->deleteLater();
    }

    for (auto it = m_activeDriverIds.begin(); it != m_activeDriverIds.end(); ++it) {
        if (it.value() == driverId) {
            QStringList alternates = availableDriversForAsset(it.key());
            it.value() = alternates.isEmpty() ? QString() : alternates.first();
            emit activeDriversChanged();
        }
    }

    emit driversRegistered();
}

AbstractInferenceDriver* EngineRegistry::getDriver(const QString &driverId) const
{
    return m_drivers.value(driverId, nullptr);
}

AbstractInferenceDriver* EngineRegistry::activeDriver(const QString &targetAssetType) const
{
    const QString activeId = m_activeDriverIds.value(targetAssetType);
    return m_drivers.value(activeId, nullptr);
}

QString EngineRegistry::getActiveDriverId(const QString &targetAssetType) const
{
    return m_activeDriverIds.value(targetAssetType);
}

QStringList EngineRegistry::availableDriversForAsset(const QString &targetAssetType) const
{
    QStringList matches;
    for (auto it = m_drivers.constBegin(); it != m_drivers.constEnd(); ++it) {
        if (it.value() && it.value()->targetAssetType() == targetAssetType) {
            matches.append(it.key());
        }
    }
    return matches;
}

bool EngineRegistry::setActiveDriver(const QString &targetAssetType, const QString &driverId)
{
    if (!m_drivers.contains(driverId)) {
        qWarning() << "[EngineRegistry] Cannot switch to unregistered driver:" << driverId;
        return false;
    }

    AbstractInferenceDriver *targetDriver = m_drivers.value(driverId);
    if (!targetDriver || targetDriver->targetAssetType() != targetAssetType) {
        qWarning() << "[EngineRegistry] Mismatched modality: Driver" << driverId
                   << "belongs to" << (targetDriver ? targetDriver->targetAssetType() : QStringLiteral("null"))
                   << "not" << targetAssetType;
        return false;
    }

    if (m_activeDriverIds.value(targetAssetType) != driverId) {
        m_activeDriverIds[targetAssetType] = driverId;
        qInfo() << "[EngineRegistry] Active driver for" << targetAssetType << "switched to:" << driverId;
        emit activeDriversChanged();
    }
    return true;
}

AbstractInferenceDriver* EngineRegistry::activeScriptDriver() const { return activeDriver(QStringLiteral("script")); }
AbstractInferenceDriver* EngineRegistry::activeImageDriver()  const { return activeDriver(QStringLiteral("image")); }
AbstractInferenceDriver* EngineRegistry::activeAudioDriver()  const { return activeDriver(QStringLiteral("audio")); }
AbstractInferenceDriver* EngineRegistry::activeVideoDriver()  const { return activeDriver(QStringLiteral("video")); }

// In engineregistry.cpp
QVariantList EngineRegistry::pluginStatuses() const
{
    static const QList<QPair<QString, QString>> expectedPlugins = {
        { QStringLiteral("LocalLlmEngine"),     QStringLiteral("Local LLM (llama.cpp)") },
        { QStringLiteral("KokoroTtsEngine"),    QStringLiteral("Kokoro TTS (sherpa-onnx)") },
        { QStringLiteral("LocalAudioEngine"),   QStringLiteral("Local Audio (audio.cpp)") },
        { QStringLiteral("LocalSdImagePlugin"), QStringLiteral("Local Image (stable-diffusion.cpp)") },
        { QStringLiteral("LocalSdVideoPlugin"), QStringLiteral("Local Video (stable-diffusion.cpp)") }
    };

    QVariantList report;
    for (const auto &entry : expectedPlugins) {
        QVariantMap item;
        item[QStringLiteral("id")] = entry.first;
        item[QStringLiteral("label")] = entry.second;
        item[QStringLiteral("available")] = m_loadedPluginIds.contains(entry.first);
        report.append(item);
    }
    return report;
}

bool EngineRegistry::hasLocalPlugins() const
{
    return !m_loadedPluginIds.isEmpty();
}

bool EngineRegistry::hasLocalPluginForCategory(const QString &category) const
{
    if (category == QStringLiteral("script") || category == QStringLiteral("text")) {
        return m_loadedPluginIds.contains(QStringLiteral("LocalLlmEngine"));
    }
    if (category == QStringLiteral("image")) {
        return m_loadedPluginIds.contains(QStringLiteral("LocalSdImagePlugin"));
    }
    if (category == QStringLiteral("audio")) {
        return m_loadedPluginIds.contains(QStringLiteral("KokoroTtsEngine")) ||
               m_loadedPluginIds.contains(QStringLiteral("LocalAudioEngine"));
    }
    if (category == QStringLiteral("video")) {
        return m_loadedPluginIds.contains(QStringLiteral("LocalSdVideoPlugin"));
    }
    return false;
}

