// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QVariantMap>
#include <QVariantList>
#include <QPointer>
#include <QtQml/QQmlEngine>
#include "app/engine/abstractinferencedriver.h"

/**
 * @class   GenericRestDriver
 * @brief   Multimodal remote inference driver supporting OpenAI-compatible and Google Gemini REST endpoints.
 *
 * Routes requests, dynamic UI options, and response parsing based on both ProtocolFlavor
 * and targetAssetType ("script", "image", "audio", "video").
 */
class GenericRestDriver : public AbstractInferenceDriver
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum ProtocolFlavor {
        Protocol_OpenAI, // OpenAI, Ollama, LM Studio, vLLM, compatible REST servers
        Protocol_Gemini  // Google AI Studio / Gemini / Imagen / Veo
    };
    Q_ENUM(ProtocolFlavor)

    explicit GenericRestDriver(const QString &driverId,
                               ProtocolFlavor flavor,
                               const QString &assetType = QStringLiteral("script"),
                               QObject *parent = nullptr);
    ~GenericRestDriver() override;

    Q_PROPERTY(bool connectionVerifiedState READ isConnectionVerified NOTIFY verificationInfoChanged)
    Q_PROPERTY(QString verificationSummary READ verificationSummary NOTIFY verificationInfoChanged)
    Q_PROPERTY(QStringList verifiedModels READ verifiedModels NOTIFY verificationInfoChanged)

           // --- AbstractInferenceDriver Contract ---
    QString driverId() const override { return m_driverId; }
    QString displayName() const override;
    QString targetAssetType() const override { return m_targetAssetType; }

    bool isModelLoaded() const override;
    bool isBusy() const override { return m_isBusy; }
    QString loadedModelName() const override { return m_modelName; }

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

    Q_INVOKABLE void verifyConnection() override;
    bool isConnectionVerified() const { return m_connectionVerified; }
    QString verificationSummary() const { return m_verificationSummary; }
    QStringList verifiedModels() const { return m_verifiedModels; }

           // Exposed for Unit Testing
    QString parseVerificationMetadata(const QByteArray &rawData, const QString &orgHeader = QString());
           // --- Inspection Helpers ---
    ProtocolFlavor flavor() const { return m_flavor; }
    QString baseUrl() const { return m_baseUrl; }

           // --- Request & Payload Builders (Exposed for Unit Testing) ---
    QNetworkRequest buildRequest(bool streaming) const;
    QByteArray buildOpenAiPayload(const QString &prompt, const QString &systemPrompt,
                                  const QStringList &files, const QVariantMap &opts, bool streaming) const;
    QByteArray buildGeminiPayload(const QString &prompt, const QString &systemPrompt,
                                  const QStringList &files, const QVariantMap &opts) const;
    QByteArray buildImagePayload(const QString &prompt, const QVariantMap &opts) const;
    QByteArray buildAudioPayload(const QString &prompt, const QVariantMap &opts) const;
    QByteArray buildVideoPayload(const QString &prompt, const QVariantMap &opts) const;

           // --- Response Parsers (Exposed for Unit Testing) ---
    void processSseLine(const QString &line);
    void parseOpenAiChunk(const QJsonObject &root);
    void parseGeminiChunk(const QJsonObject &root);
    QVariantMap parseImageResponse(const QByteArray &rawData) const;
    QVariantMap parseAudioResponse(const QByteArray &rawData, const QString &outputDir) const;
    QString extractVideoOperationId(const QByteArray &rawData) const;
    QString extractVideoDownloadUrl(const QJsonObject &root) const;

signals:
    void verificationInfoChanged();

private slots:
    void onReadyRead();
    void onReplyFinished();
    void onNetworkError(QNetworkReply::NetworkError code);
    void pollVideoOperation();

private:
    void initModalityDefaults();
    void downloadVideoAsset(const QUrl &videoUrl);

    QString m_driverId;
    QString m_displayName;
    ProtocolFlavor m_flavor;
    QString m_targetAssetType;

    QString m_baseUrl;
    QString m_apiKey;
    QString m_modelName;
    QString m_currentOutputDir;

    bool m_isBusy = false;
    QVariantMap m_currentOptions;

    QNetworkAccessManager *m_netManager = nullptr;
    QPointer<QNetworkReply> m_currentReply;

    // Async video polling
    QTimer *m_pollTimer = nullptr;
    QString m_pendingOperationId;

    // Streaming accumulator (script modality)
    QByteArray m_streamBuffer;
    QString m_accumulatedText;
    QString m_accumulatedThink;
    bool m_isInsideThinkTag = false;

    // connection verification
    bool m_connectionVerified = false;
    QString m_verificationSummary;
    QStringList m_verifiedModels;
};
