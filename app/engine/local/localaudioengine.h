// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QtQml/QQmlEngine>
#include <QVariantMap>
#include <QVariantList>
#include "app/engine/abstractinferencedriver.h"

extern "C" {
#include <audiocpp.h>
}
/**
 * @class   LocalAudioEngine
 * @brief   Inference driver wrapping audio.cpp for local SFX and speech synthesis.
 */
class LocalAudioEngine : public AbstractInferenceDriver
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AbstractInferenceDriver_iid)
    Q_INTERFACES(AbstractInferenceDriver)
    QML_ELEMENT

public:
    explicit LocalAudioEngine(QObject *parent = nullptr);
    ~LocalAudioEngine() override;

    // --- AbstractInferenceDriver Implementation ---
    QString driverId() const override { return QStringLiteral("audio_cpp"); }
    QString displayName() const override { return QStringLiteral("AudioCPP"); }
    QString targetAssetType() const override { return QStringLiteral("audio"); }

    bool isModelLoaded() const override { return m_model != nullptr && m_session != nullptr; }
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

    QStringList availableAudioModels() const;
    QString getFamilyForModel(const QString &modelBaseName) const;

    Q_INVOKABLE void verifyConnection() override;
private:
    audiocpp_model *m_model = nullptr;
    audiocpp_registry *m_registry = nullptr;
    audiocpp_session *m_session = nullptr;

    bool m_isBusy = false;
    bool m_abortRequested = false;
    QString m_loadedModelName;
    QString m_lastModelPath;
    QVariantMap m_currentOptions;

    void handleAudioCppStatus(const audiocpp_status &status);
    QString saveAudioFile(const QByteArray &wavData, const QString &targetDirectory, const QString &prefix);
};

