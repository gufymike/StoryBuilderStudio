/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: localllmengine.h
** llama.cpp local execution backend supporting ChatML tokenization and dynamic options.
**
****************************************************************************/
#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QVariantMap>
#include <QtQml/QQmlEngine>
#include "llama.h"
#include "app/engine/abstractinferencedriver.h"

struct ChatTurn {
    QString role;    // "system", "user", "assistant"
    QString content;
};

/**
 * @class   LocalLlmEngine
 * @brief   llama.cpp local execution backend supporting ChatML tokenization and dynamic options.
 */
class LocalLlmEngine : public AbstractInferenceDriver
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID AbstractInferenceDriver_iid)
    Q_INTERFACES(AbstractInferenceDriver)
    QML_ELEMENT

    Q_PROPERTY(int contextLimit READ contextLimit WRITE setContextLimit NOTIFY contextLimitChanged)

public:
    explicit LocalLlmEngine(QObject *parent = nullptr);
    ~LocalLlmEngine() override;

    // Metadata
    QString driverId() const override { return QStringLiteral("llama_cpp"); }
    QString displayName() const override { return QStringLiteral("LlamaCPP"); }
    QString targetAssetType() const override { return QStringLiteral("script"); }

    // Lifecycle
    bool isModelLoaded() const override { return m_model != nullptr && m_ctx != nullptr; }
    bool isBusy() const override { return m_isBusy; }
    QString loadedModelName() const override { return m_loadedModelName; }
    int contextLimit() const { return m_nCtx; }
    void setContextLimit(int limit);

    Q_INVOKABLE void resetConversation();
    const QList<ChatTurn>& conversationHistory() const { return m_history; }

    bool loadModel(const QVariantMap &config) override;
    void unloadModel() override;
    Q_INVOKABLE void cancelGeneration() override;

    // Dynamic Options Discovery
    QVariantList availableOptions() const override;
    QVariantMap currentOptions() const override { return m_currentOptions; }
    bool setOption(const QString &key, const QVariant &value) override;

    // Execution
    Q_INVOKABLE void generateText(const QString &prompt,
                                  const QString &systemPrompt = QString(),
                                  const QStringList &attachedFiles = QStringList(),
                                  int maxTokens = 2048,
                                  float temperature = 0.7f,
                                  float topP = 0.9f);

    void generate(const QString &prompt,
                  const QString &systemPrompt = QString(),
                  const QStringList &attachedFiles = QStringList(),
                  const QString &outputDirectory = QString(),
                  const QVariantMap &runtimeOverrides = QVariantMap()) override;

    // Helper estimation exposed for test coverage & budgeting
    int estimateTokenCount(const QString &text) const;
    QString buildChatMlPrompt(const QString &incomingUserPrompt, const QString &systemPrompt);

    Q_INVOKABLE void verifyConnection() override;

signals:
    void contextLimitChanged();
    void responseChunkReceived(const QString &chunk);

private:
    bool loadModelInternal(const QString &path, int nCtx, int nGpuLayers);

    llama_model *m_model = nullptr;
    llama_context *m_ctx = nullptr;
    llama_sampler *m_smpl = nullptr;

    bool m_isBusy = false;
    bool m_abortRequested = false;
    QString m_loadedModelName;
    QString m_lastModelPath;

    QList<ChatTurn> m_history;
    int m_nCtx = 4096;
    int m_maxGenerationTokens = 512;

    QVariantMap m_currentOptions;
};

