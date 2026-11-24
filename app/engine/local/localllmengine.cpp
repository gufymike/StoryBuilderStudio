// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include "localllmengine.h"
#include <QtConcurrent/QtConcurrent>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QFuture>
#include <QHash>
#include <vector>
#include "app/settings/settingsmanager.h"
#include "llama_compat.h"

namespace {
QMutex s_llmWorkerMutex;
QHash<const LocalLlmEngine *, QFuture<void>> s_llmWorkers;

void trackLlmWorker(const LocalLlmEngine *engine, const QFuture<void> &future)
{
    QMutexLocker locker(&s_llmWorkerMutex);
    s_llmWorkers.insert(engine, future);
}

void waitForLlmWorker(const LocalLlmEngine *engine)
{
    QFuture<void> future;
    {
        QMutexLocker locker(&s_llmWorkerMutex);
        future = s_llmWorkers.take(engine);
    }
    if (future.isValid() && future.isRunning()) {
        future.waitForFinished();
    }
}
} // namespace

LocalLlmEngine::LocalLlmEngine(QObject *parent)
    : AbstractInferenceDriver(parent)
{
    m_currentOptions = QVariantMap{
        {QStringLiteral("temperature"), 0.7},
        {QStringLiteral("top_p"), 0.9},
        {QStringLiteral("max_tokens"), 2048},
        {QStringLiteral("stream"), true}
    };
    llama_backend_init();
}

LocalLlmEngine::~LocalLlmEngine()
{
    unloadModel();
    llama_backend_free();
}

void LocalLlmEngine::setContextLimit(int limit)
{
    if (m_nCtx != limit && limit >= 512) {
        m_nCtx = limit;
        emit contextLimitChanged();
    }
}

void LocalLlmEngine::resetConversation()
{
    m_history.clear();
    qInfo() << "[LocalLlmEngine] Conversation memory reset.";
}

void LocalLlmEngine::unloadModel()
{
    if (m_isBusy) {
        cancelGeneration();
    }
    waitForLlmWorker(this);

    if (m_smpl) {
        llama_sampler_free(m_smpl);
        m_smpl = nullptr;
    }
    if (m_ctx) {
        llama_free(m_ctx);
        m_ctx = nullptr;
    }
    if (m_model) {
        llama_model_free(m_model);
        m_model = nullptr;
    }

    m_history.clear();
    m_loadedModelName.clear();
    emit modelLoadedChanged(false);
    emit loadedModelNameChanged(m_loadedModelName);
    qInfo() << "[LocalLlmEngine] Model and context unloaded cleanly.";
}

bool LocalLlmEngine::loadModel(const QVariantMap &config)
{
    QString path = config.value(QStringLiteral("modelPath")).toString();

    if (path.isEmpty()) {
        SettingsManager settings;
        QString activeProvider = settings.activeScriptProvider();
        QString modelDirName = activeProvider.remove(QStringLiteral("Local:")).trimmed();

        QDir scriptDir(settings.scriptModelsDir() + QStringLiteral("/") + modelDirName);
        if (!scriptDir.exists()) {
            scriptDir.setPath(settings.scriptModelsDir());
        }

        QStringList candidates = scriptDir.entryList(QStringList() << QStringLiteral("*.gguf"), QDir::Files | QDir::Readable, QDir::Name);
        if (!candidates.isEmpty()) {
            path = scriptDir.filePath(candidates.first());
        }
    }

    if (path.isEmpty()) {
        emit engineError(tr("No model path specified and no .gguf found in script models directory."));
        return false;
    }

    int nCtx = config.value(QStringLiteral("nCtx"), m_nCtx).toInt();
    int nGpuLayers = config.value(QStringLiteral("nGpuLayers"), 0).toInt();
    return this->loadModelInternal(path, nCtx, nGpuLayers);
}

bool LocalLlmEngine::loadModelInternal(const QString &modelPath, int nCtx, int nGpuLayers)
{
    unloadModel();

    QString cleanPath = QDir::toNativeSeparators(modelPath);
    if (!QFileInfo::exists(cleanPath)) {
        emit engineError(tr("LLM weights not found at: %1").arg(cleanPath));
        return false;
    }

    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = nGpuLayers;

    QByteArray pathUtf8 = cleanPath.toUtf8();
    m_model = llama_model_load_from_file(pathUtf8.constData(), model_params);
    if (!m_model) {
        emit engineError(tr("Failed to load GGUF model via llama.cpp."));
        return false;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = nCtx;
    ctx_params.n_batch = 512;
    ctx_params.n_ubatch = 512;
    ctx_params.n_threads = qMax(1, QThread::idealThreadCount() - 2);
    ctx_params.n_threads_batch = ctx_params.n_threads;

    m_ctx = llama_init_from_model(m_model, ctx_params);
    if (!m_ctx) {
        llama_model_free(m_model);
        m_model = nullptr;
        emit engineError(tr("Failed to allocate llama context."));
        return false;
    }

    m_lastModelPath = cleanPath;
    m_loadedModelName = QFileInfo(cleanPath).baseName();

    emit modelLoadedChanged(true);
    emit loadedModelNameChanged(m_loadedModelName);
    qInfo() << "[LocalLlmEngine] Loaded LLM:" << m_loadedModelName << "(Context size:" << nCtx << ")";
    return true;
}

void LocalLlmEngine::cancelGeneration()
{
    m_abortRequested = true;
}

QVariantList LocalLlmEngine::availableOptions() const
{
    return QVariantList{
        QVariantMap{
            {QStringLiteral("key"), QStringLiteral("temperature")},
            {QStringLiteral("label"), QStringLiteral("Temperature (Creativity)")},
            {QStringLiteral("type"), QStringLiteral("float")},
            {QStringLiteral("defaultValue"), 0.7},
            {QStringLiteral("minValue"), 0.0},
            {QStringLiteral("maxValue"), 2.0},
            {QStringLiteral("step"), 0.05}
        },
        QVariantMap{
            {QStringLiteral("key"), QStringLiteral("top_p")},
            {QStringLiteral("label"), QStringLiteral("Top-P (Nucleus Sampling)")},
            {QStringLiteral("type"), QStringLiteral("float")},
            {QStringLiteral("defaultValue"), 0.9},
            {QStringLiteral("minValue"), 0.1},
            {QStringLiteral("maxValue"), 1.0},
            {QStringLiteral("step"), 0.05}
        },
        QVariantMap{
            {QStringLiteral("key"), QStringLiteral("max_tokens")},
            {QStringLiteral("label"), QStringLiteral("Max Generation Tokens")},
            {QStringLiteral("type"), QStringLiteral("int")},
            {QStringLiteral("defaultValue"), 2048},
            {QStringLiteral("minValue"), 256},
            {QStringLiteral("maxValue"), 8192},
            {QStringLiteral("step"), 256}
        },
        QVariantMap{
            {QStringLiteral("key"), QStringLiteral("stream")},
            {QStringLiteral("label"), QStringLiteral("Stream Response Tokens")},
            {QStringLiteral("type"), QStringLiteral("boolean")},
            {QStringLiteral("defaultValue"), true}
        }
    };
}

bool LocalLlmEngine::setOption(const QString &key, const QVariant &value)
{
    if (key.isEmpty()) return false;
    m_currentOptions[key] = value;
    emit optionsChanged();
    return true;
}

void LocalLlmEngine::generate(const QString &prompt,
                              const QString &systemPrompt,
                              const QStringList &attachedFiles,
                              const QString &outputDirectory,
                              const QVariantMap &runtimeOverrides)
{
    Q_UNUSED(outputDirectory);
    float temp = runtimeOverrides.value(QStringLiteral("temperature"), m_currentOptions.value(QStringLiteral("temperature"), 0.7f)).toFloat();
    float topP = runtimeOverrides.value(QStringLiteral("top_p"), m_currentOptions.value(QStringLiteral("top_p"), 0.9f)).toFloat();
    int maxTok = runtimeOverrides.value(QStringLiteral("max_tokens"), m_currentOptions.value(QStringLiteral("max_tokens"), 2048)).toInt();

    this->generateText(prompt, systemPrompt, attachedFiles, maxTok, temp, topP);
}

void LocalLlmEngine::generateText(const QString &prompt,
                                  const QString &systemPrompt,
                                  const QStringList &attachedFiles,
                                  int maxTokens,
                                  float temperature,
                                  float topP)
{
    if (!isModelLoaded() || !m_model || !m_ctx) {
        emit engineError(tr("Cannot generate: No LLM model loaded."));
        return;
    }

    if (m_isBusy) {
        emit engineError(tr("LLM engine is already busy generating."));
        return;
    }

    m_isBusy = true;
    m_abortRequested = false;
    emit busyChanged(true);

    QFuture<void> future = QtConcurrent::run([this, prompt, systemPrompt, attachedFiles, maxTokens, temperature, topP]() {
        QString fileContext;
        for (const QString &filePath : attachedFiles) {
            QString cleanPath = QDir::toNativeSeparators(filePath);
            QFile file(cleanPath);
            QFileInfo fi(cleanPath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString content = in.readAll();
                fileContext += QStringLiteral("\n--- ATTACHED FILE [%1] ---\n%2\n--- END OF FILE [%1] ---\n")
                                   .arg(fi.fileName(), content);
                file.close();
            } else {
                qWarning() << "[LocalLlmEngine] Could not open attached file:" << cleanPath;
            }
        }

        QString formattedPrompt;
        if (!systemPrompt.isEmpty()) {
            formattedPrompt += QStringLiteral("<|im_start|>system\n%1<|im_end|>\n").arg(systemPrompt.trimmed());
        }

        for (const auto &turn : m_history) {
            formattedPrompt += QStringLiteral("<|im_start|>%1\n%2<|im_end|>\n").arg(turn.role, turn.content);
        }

        QString userPayload = prompt.trimmed();
        if (!fileContext.isEmpty()) {
            userPayload = QStringLiteral("Context Attachments:\n%1\n\nPrompt/Task:\n%2").arg(fileContext, userPayload);
        }

        formattedPrompt += QStringLiteral("<|im_start|>user\n%1<|im_end|>\n<|im_start|>assistant\n").arg(userPayload);

        QByteArray promptBytes = formattedPrompt.toUtf8();
        const llama_vocab *vocab = llama_model_get_vocab(m_model);
        int32_t n_vocab = llama_vocab_n_tokens(vocab);

        int n_tokens = -llama_tokenize(vocab, promptBytes.constData(), promptBytes.size(), nullptr, 0, true, true);
        std::vector<llama_token> tokens(n_tokens);
        llama_tokenize(vocab, promptBytes.constData(), promptBytes.size(), tokens.data(), tokens.size(), true, true);

        int32_t total_tokens = static_cast<int32_t>(tokens.size());
        int32_t n_ctx = llama_n_ctx(m_ctx);
        int32_t max_allowed_prompt = n_ctx - 1024;

        if (total_tokens > max_allowed_prompt) {
            qWarning() << "[LocalLlmEngine] Input exceeds safe context space. Truncating from" << total_tokens << "to" << max_allowed_prompt;
            while (!m_history.isEmpty() && total_tokens > max_allowed_prompt) {
                m_history.removeFirst();
                if (!m_history.isEmpty() && m_history.first().role == QStringLiteral("assistant")) {
                    m_history.removeFirst();
                }
            }
            tokens.resize(max_allowed_prompt);
            total_tokens = max_allowed_prompt;
        }

        llama_sampler_chain_params sparams = llama_sampler_chain_default_params();
        llama_sampler *smpl = llama_sampler_chain_init(sparams);
        llama_sampler_chain_add(smpl, llama_compat::init_penalties(n_vocab, 64, 1.15f, 0.10f, 0.10f));
        llama_sampler_chain_add(smpl, llama_sampler_init_top_p(topP, 1));
        llama_sampler_chain_add(smpl, llama_sampler_init_temp(temperature));
        llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

        llama_memory_t mem = llama_get_memory(m_ctx);
        llama_memory_seq_rm(mem, 0, 0, -1);

        int32_t n_batch = llama_n_batch(m_ctx);
        int32_t evaluated_tokens = 0;

        while (evaluated_tokens < total_tokens && !m_abortRequested) {
            int32_t chunk_size = std::min(n_batch, total_tokens - evaluated_tokens);
            llama_batch chunk_batch = llama_batch_init(chunk_size, 0, 1);
            chunk_batch.n_tokens = chunk_size;

            for (int32_t i = 0; i < chunk_size; ++i) {
                chunk_batch.token[i]  = tokens[evaluated_tokens + i];
                chunk_batch.pos[i]    = evaluated_tokens + i;
                chunk_batch.n_seq_id[i] = 1;
                chunk_batch.seq_id[i][0] = 0;
                chunk_batch.logits[i] = (evaluated_tokens + i == total_tokens - 1);
            }

            if (llama_decode(m_ctx, chunk_batch) != 0) {
                llama_batch_free(chunk_batch);
                llama_sampler_free(smpl);
                QMetaObject::invokeMethod(this, [this]() {
                    m_isBusy = false;
                    emit busyChanged(false);
                    emit engineError(tr("Prompt chunk evaluation failed. Input size exceeds physical context limit."));
                }, Qt::QueuedConnection);
                return;
            }

            llama_batch_free(chunk_batch);
            evaluated_tokens += chunk_size;
        }

        QString accumulatedResponse;
        QString accumulatedThinking;
        bool isInsideThink = false;
        int n_cur = static_cast<int>(tokens.size());
        int n_generated = 0;

        while (n_generated < maxTokens && !m_abortRequested) {
            llama_token new_token = llama_sampler_sample(smpl, m_ctx, -1);
            llama_sampler_accept(smpl, new_token);

            if (llama_vocab_is_eog(vocab, new_token)) {
                break;
            }

            char piece[512] = {0};
            int n_piece = llama_token_to_piece(vocab, new_token, piece, sizeof(piece), 0, false);
            QString pieceStr;
            if (n_piece < 0) {
                std::vector<char> dyn_piece(-n_piece);
                n_piece = llama_token_to_piece(vocab, new_token, dyn_piece.data(), dyn_piece.size(), 0, false);
                if (n_piece > 0) {
                    pieceStr = QString::fromUtf8(dyn_piece.data(), n_piece);
                }
            } else if (n_piece > 0) {
                pieceStr = QString::fromUtf8(piece, n_piece);
            }

            if (!pieceStr.isEmpty()) {
                if (pieceStr.contains(QStringLiteral("<think>"))) {
                    isInsideThink = true;
                    pieceStr.remove(QStringLiteral("<think>"));
                }

                if (isInsideThink) {
                    if (pieceStr.contains(QStringLiteral("</think>"))) {
                        isInsideThink = false;
                        QStringList parts = pieceStr.split(QStringLiteral("</think>"));
                        accumulatedThinking += parts.first();
                        n_generated = 0;

                        if (parts.size() > 1 && !parts.at(1).isEmpty()) {
                            QString rest = parts.at(1);
                            if (accumulatedResponse.isEmpty()) {
                                rest = rest.trimmed();
                            }
                            accumulatedResponse += rest;
                            QMetaObject::invokeMethod(this, [this, rest]() {
                                emit tokenGenerated(rest);
                            }, Qt::QueuedConnection);
                        }
                    } else {
                        accumulatedThinking += pieceStr;
                    }
                } else {
                    accumulatedResponse += pieceStr;
                    n_generated++;

                    QMetaObject::invokeMethod(this, [this, pieceStr]() {
                        emit tokenGenerated(pieceStr);
                    }, Qt::QueuedConnection);
                }
            }

            llama_batch batch = llama_batch_get_one(&new_token, 1);
            if (llama_decode(m_ctx, batch) != 0) {
                qWarning() << "[LocalLlmEngine] Context window limit reached. Triggering llama_memory shift...";
                if (llama_memory_can_shift(mem)) {
                    llama_seq_id seq_id = 0;
                    llama_pos llpos_n_keep = 128;
                    llama_pos max_pos = llama_memory_seq_pos_max(mem, seq_id);
                    llama_pos llpos_n_left = (max_pos - llpos_n_keep) / 2;

                    if (llpos_n_left > 0) {
                        llama_memory_seq_rm(mem, seq_id, llpos_n_keep, llpos_n_keep + llpos_n_left);
                        llama_memory_seq_add(mem, seq_id, llpos_n_keep + llpos_n_left, -1, -llpos_n_left);
                        n_cur -= llpos_n_left;

                        if (llama_decode(m_ctx, batch) != 0) {
                            qCritical() << "[LocalLlmEngine] Decode failed after memory shift. Aborting.";
                            break;
                        }
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }

            n_cur++;
            n_generated++;
        }

        llama_sampler_free(smpl);

        QString sharedTimestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        QString finalResponse = accumulatedResponse.trimmed();
        QString finalThinking = accumulatedThinking.trimmed();

        QMetaObject::invokeMethod(this, [this, userPayload, finalResponse, finalThinking, sharedTimestamp]() {
            m_history.append(ChatTurn{ QStringLiteral("user"), userPayload });
            m_history.append(ChatTurn{ QStringLiteral("assistant"), finalResponse });

            m_isBusy = false;
            emit busyChanged(false);

            QVariantMap payload;
            payload[QStringLiteral("text")] = finalResponse;
            payload[QStringLiteral("think")] = finalThinking;
            payload[QStringLiteral("timestamp")] = sharedTimestamp;
            payload[QStringLiteral("category")] = QStringLiteral("script");

            emit generationFinished(payload);
        }, Qt::QueuedConnection);
    });

    trackLlmWorker(this, future);
}

int LocalLlmEngine::estimateTokenCount(const QString &text) const
{
    return static_cast<int>(text.length() / 3.5) + 4;
}

QString LocalLlmEngine::buildChatMlPrompt(const QString &incomingUserPrompt, const QString &systemPrompt)
{
    QString sys = systemPrompt.trimmed();
    if (sys.isEmpty()) {
        sys = QStringLiteral("You are an expert production assistant in Story Builder Studio helping structure stories, scenes, and asset directions.");
    }

    QString systemBlock = QStringLiteral("<|im_start|>system\n%1<|im_end|>\n").arg(sys);
    int systemTokens = estimateTokenCount(systemBlock);

    int budget = m_nCtx - systemTokens - m_maxGenerationTokens - 32;
    m_history.append(ChatTurn{ QStringLiteral("user"), incomingUserPrompt.trimmed() });

    while (!m_history.isEmpty()) {
        int currentTurnsCost = 0;
        for (const auto &turn : m_history) {
            currentTurnsCost += estimateTokenCount(turn.content) + 8;
        }

        if (currentTurnsCost <= budget || m_history.size() <= 1) {
            break;
        }

        qWarning() << "[LocalLlmEngine] Approaching nCtx limit (" << m_nCtx << "). Pruning oldest turns from context.";
        m_history.removeFirst();
        if (!m_history.isEmpty() && m_history.first().role == QStringLiteral("assistant")) {
            m_history.removeFirst();
        }
    }

    QString finalPrompt = systemBlock;
    for (const auto &turn : m_history) {
        finalPrompt += QStringLiteral("<|im_start|>%1\n%2<|im_end|>\n").arg(turn.role, turn.content);
    }
    finalPrompt += QStringLiteral("<|im_start|>assistant\n");

    return finalPrompt;
}

void LocalLlmEngine::verifyConnection() {
    // Check if the configured model file actually exists on disk
    if (!isModelLoaded() || m_loadedModelName.isEmpty()) {
        emit connectionVerified(false, tr("Local model file not found."));
    } else {
        emit connectionVerified(true, tr("Local weights found and ready for inference."));
    }
}
