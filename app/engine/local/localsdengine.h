/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: localsdengine.h
** Inference driver integrating stable-diffusion.cpp for image and video synthesis.
**
****************************************************************************/
#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/QQmlEngine>
#include "stable-diffusion.h"
#include "app/engine/abstractinferencedriver.h"

/**
 * @class   LocalSdEngine
 * @brief   Inference driver integrating stable-diffusion.cpp for image and video synthesis.
 *
 * File: localsdengine.h
 */
class LocalSdEngine : public AbstractInferenceDriver
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AbstractInferenceDriver_iid)
    Q_INTERFACES(AbstractInferenceDriver)
    QML_ELEMENT

    Q_PROPERTY(bool isVideoMode READ isVideoMode NOTIFY videoSupportedChanged)
    Q_PROPERTY(bool isVideoSupported READ isVideoSupported NOTIFY videoSupportedChanged)
    Q_PROPERTY(VideoArchitecture activeVideoArch READ activeVideoArch NOTIFY activeVideoArchChanged)

public:
    enum VideoArchitecture {
        VIDEO_ARCH_NONE,
        VIDEO_ARCH_LTX,
        VIDEO_ARCH_WAN
    };
    Q_ENUM(VideoArchitecture)


    explicit LocalSdEngine(QObject *parent = nullptr);
    ~LocalSdEngine() override;

    // --- AbstractInferenceDriver Implementation ---
    QString driverId() const override { return m_driverId; }
    QString displayName() const override { return m_displayName; }
    QString targetAssetType() const override { return m_targetAssetType; }

    bool isModelLoaded() const override { return m_isLoaded && m_sdCtx != nullptr; }
    bool isBusy() const override { return m_isBusy; }
    QString loadedModelName() const override { return m_loadedModelName; }

    bool loadModel(const QVariantMap &config) override;
    void unloadModel() override;
    void cancelGeneration() override;

    QVariantList availableOptions() const override;
    QVariantMap currentOptions() const override { return m_currentOptions; }
    bool setOption(const QString &key, const QVariant &value) override;

    void generate(const QString &prompt,
                  const QString &systemPrompt = QString(),
                  const QStringList &attachedFiles = QStringList(),
                  const QString &outputDirectory = QString(),
                  const QVariantMap &runtimeOverrides = QVariantMap()) override;

    // Video Engine Inspection
    bool isVideoSupported() const;
    bool isVideoMode() const { return m_isVideoMode; }
    VideoArchitecture activeVideoArch() const { return m_activeVideoArch; }

    Q_INVOKABLE void verifyConnection() override;

signals:
    void activeVideoArchChanged();
    void videoSupportedChanged(bool supported);

private:
    QString m_driverId;
    QString m_displayName;
    QString m_targetAssetType;
    sd_ctx_t *m_sdCtx = nullptr;
    bool m_isLoaded = false;
    bool m_isBusy = false;
    bool m_isVideoMode = false;
    VideoArchitecture m_activeVideoArch = VIDEO_ARCH_NONE;

    QString m_loadedModelName;
    QString m_lastDiffusionPath;
    QVariantMap m_currentOptions;

    void initDefaultOptions();
    bool loadImageModel(const QString &diffPath, const QString &clipL, const QString &t5, const QString &vae);
    bool loadVideoModelInternal(const QString &diffPath, const QString &t5, const QString &vae, const QString &engineType);

    void runImageGeneration(const QString &prompt, int samples, int w, int h, int steps, float cfg, int baseSeed);
    void runVideoGeneration(const QString &prompt, const QString &initImage, const QString &outDir,
                            int w, int h, int frames, int fps, int steps, float cfg, float flowShift, int baseSeed);
};

