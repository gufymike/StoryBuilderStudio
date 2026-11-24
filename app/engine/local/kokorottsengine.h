// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QMap>
#include <QtQml/QQmlEngine>

#include "app/engine/abstractinferencedriver.h"

// Forward-declare the raw C handle pointer
struct SherpaOnnxOfflineTts;
/**
 * @class   KokoroTtsEngine
 * @brief   Inference driver wrapping Sherpa-ONNX Kokoro offline TTS.
 */
class KokoroTtsEngine : public AbstractInferenceDriver
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AbstractInferenceDriver_iid)
    Q_INTERFACES(AbstractInferenceDriver)
    QML_ELEMENT

    Q_PROPERTY(QStringList availableVoices READ availableVoices NOTIFY availableVoicesChanged)
    Q_PROPERTY(QString activeVoice READ activeVoice WRITE setActiveVoice NOTIFY activeVoiceChanged)

public:
    explicit KokoroTtsEngine(QObject *parent = nullptr);
    ~KokoroTtsEngine() override;

    // --- AbstractInferenceDriver Implementation ---
    QString driverId() const override { return QStringLiteral("kokoro_tts"); }
    QString displayName() const override { return QStringLiteral("Kokoro"); }
    QString targetAssetType() const override { return QStringLiteral("audio"); }

    bool isModelLoaded() const override { return m_tts != nullptr; }
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

    QStringList availableVoices() const { return m_availableVoices; }
    QString activeVoice() const { return m_activeVoice; }
    void setActiveVoice(const QString &voiceName);

    Q_INVOKABLE void synthesizeSpeech(
        const QString &text,
        const QString &voiceName = QString(),
        float speed = 1.0f,
        const QStringList &attachedFiles = QStringList(),
        const QString &outputDirectory = QString()
        );

    Q_INVOKABLE void verifyConnection() override;
signals:
    void availableVoicesChanged();
    void activeVoiceChanged();
    void speechSynthesized(const QByteArray &wavData, const QString &savedFilePath = QString());

private:
    const SherpaOnnxOfflineTts *m_tts{nullptr};
    bool m_isBusy{false};
    QString m_activeVoice{QStringLiteral("af_bella")};
    QString m_loadedModelName;
    QStringList m_availableVoices;
    QMap<QString, int> m_voiceSpeakerIds;
    QVariantMap m_currentOptions;

    void initVoices();
};

