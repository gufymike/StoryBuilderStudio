// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QVariantMap>
#include <QtQml/QQmlEngine>
#include <QSet>

#include "abstractinferencedriver.h"
/**
 * @class EngineRegistry
 * @brief Central hub managing the instantiation, routing, and lifecycle of AI drivers.
 *
 * Operates as a QML Singleton. It scans the `plugins/drivers` directory at startup
 * to load heavy C++ AI engines (Stable Diffusion, Llama) as dynamic libraries, while
 * directly instantiating lightweight network drivers (REST APIs).
 */
class EngineRegistry : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    /**
     * @property activeScriptDriver
     * @brief Pointer to the currently selected driver for text/script generation.
     * @notifier activeDriversChanged
     */
    Q_PROPERTY(AbstractInferenceDriver* activeScriptDriver READ activeScriptDriver NOTIFY activeDriversChanged)
    /**
     * @property activeImageDriver
     * @brief Pointer to the currently selected driver for Image generation.
     * @notifier activeDriversChanged
     */
    Q_PROPERTY(AbstractInferenceDriver* activeImageDriver READ activeImageDriver NOTIFY activeDriversChanged)
    /**
     * @property activeAudioDriver
     * @brief Pointer to the currently selected driver for Audio/Speech generation.
     * @notifier activeDriversChanged
     */
    Q_PROPERTY(AbstractInferenceDriver* activeAudioDriver READ activeAudioDriver NOTIFY activeDriversChanged)
    /**
     * @property activeVideoDriver
     * @brief Pointer to the currently selected driver for Video generation.
     * @notifier activeDriversChanged
     */
    Q_PROPERTY(AbstractInferenceDriver* activeVideoDriver READ activeVideoDriver NOTIFY activeDriversChanged)

    Q_PROPERTY(QStringList scriptDrivers READ scriptDrivers NOTIFY driversRegistered)
    Q_PROPERTY(QStringList imageDrivers READ imageDrivers NOTIFY driversRegistered)
    Q_PROPERTY(QStringList audioDrivers READ audioDrivers NOTIFY driversRegistered)
    Q_PROPERTY(QStringList videoDrivers READ videoDrivers NOTIFY driversRegistered)

    Q_PROPERTY(bool hasLocalPlugins READ hasLocalPlugins NOTIFY pluginsChanged)
    Q_PROPERTY(QVariantList pluginStatuses READ pluginStatuses NOTIFY pluginsChanged)
public:
    explicit EngineRegistry(QObject *parent = nullptr);
    ~EngineRegistry() override;

    void registerDriver(AbstractInferenceDriver *driver);
    void unregisterDriver(const QString &driverId);
    void clearAllDrivers();

    Q_INVOKABLE AbstractInferenceDriver* getDriver(const QString &driverId) const;
    Q_INVOKABLE AbstractInferenceDriver* activeDriver(const QString &targetAssetType) const;
    Q_INVOKABLE QStringList availableDriversForAsset(const QString &targetAssetType) const;

    Q_INVOKABLE bool setActiveDriver(const QString &targetAssetType, const QString &driverId);
    Q_INVOKABLE QString getActiveDriverId(const QString &targetAssetType) const;

    AbstractInferenceDriver* activeScriptDriver() const;
    AbstractInferenceDriver* activeImageDriver() const;
    AbstractInferenceDriver* activeAudioDriver() const;
    AbstractInferenceDriver* activeVideoDriver() const;

    QStringList scriptDrivers() const { return availableDriversForAsset(QStringLiteral("script")); }
    QStringList imageDrivers() const  { return availableDriversForAsset(QStringLiteral("image")); }
    QStringList audioDrivers() const  { return availableDriversForAsset(QStringLiteral("audio")); }
    QStringList videoDrivers() const  { return availableDriversForAsset(QStringLiteral("video")); }

    bool hasLocalPlugins() const;
    QVariantList pluginStatuses() const;
    Q_INVOKABLE bool hasLocalPluginForCategory(const QString &category) const;

signals:
    void activeDriversChanged();
    void driversRegistered();
    void driverRegistered(const QString &driverId);
    void pluginsChanged();

private:
    void loadPlugins();

    QSet<QString> m_loadedPluginIds;
    QMap<QString, AbstractInferenceDriver*> m_drivers;
    QMap<QString, QString> m_activeDriverIds;
};

