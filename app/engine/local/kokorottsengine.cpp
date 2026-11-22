/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: kokorottsengine.cpp
** Inference driver wrapping Sherpa-ONNX Kokoro offline TTS.
**
****************************************************************************/
#include "kokorottsengine.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDateTime>
#include <QThread>
#include <cstring>

#if defined(_WIN32)
#undef SHERPA_ONNX_BUILD_SHARED_LIBS
#ifndef SHERPA_ONNX_IMPORT
#define SHERPA_ONNX_IMPORT __declspec(dllimport)
#endif
#endif

extern "C" {
#include <sherpa-onnx/c-api/c-api.h>
}

KokoroTtsEngine::KokoroTtsEngine(QObject *parent)
    : AbstractInferenceDriver(parent)
{
    initVoices();
}

KokoroTtsEngine::~KokoroTtsEngine()
{
    unloadModel();
}

void KokoroTtsEngine::setActiveVoice(const QString &voiceName)
{
    if (m_activeVoice != voiceName && m_voiceSpeakerIds.contains(voiceName)) {
        m_activeVoice = voiceName;
        emit activeVoiceChanged();
    }
}

void KokoroTtsEngine::initVoices()
{
    m_voiceSpeakerIds = {
        {QStringLiteral("af_bella"), 1}, {QStringLiteral("af_nicole"), 2}, {QStringLiteral("af_sarah"), 3}, {QStringLiteral("af_sky"), 4}, {QStringLiteral("af"), 0},
        {QStringLiteral("am_adam"), 5}, {QStringLiteral("am_michael"), 6}, {QStringLiteral("bf_emma"), 7}, {QStringLiteral("bf_isabella"), 8},
        {QStringLiteral("bm_george"), 9}, {QStringLiteral("bm_lewis"), 10}
    };
    m_availableVoices = m_voiceSpeakerIds.keys();
    m_currentOptions[QStringLiteral("voice")] = QStringLiteral("af_bella");
    m_currentOptions[QStringLiteral("speed")] = 1.0f;
}

void KokoroTtsEngine::unloadModel()
{
    if (m_tts) {
        SherpaOnnxDestroyOfflineTts(m_tts);
        m_tts = nullptr;
        m_loadedModelName.clear();
        emit modelLoadedChanged(false);
        emit loadedModelNameChanged(m_loadedModelName);
    }
}

void KokoroTtsEngine::cancelGeneration()
{
    m_isBusy = false;
}

bool KokoroTtsEngine::loadModel(const QVariantMap &config)
{
    unloadModel();

    QString baseDir = config.value(QStringLiteral("modelDir")).toString();
    if (baseDir.isEmpty()) {
        QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        baseDir = appData + QStringLiteral("/audio/speech/Kokoro");
    }

    QDir dir(baseDir);
    QStringList onnxFiles = dir.entryList(QStringList() << QStringLiteral("*.onnx"), QDir::Files, QDir::Name);
    if (onnxFiles.isEmpty()) {
        emit engineError(tr("Kokoro ONNX model missing in: %1").arg(baseDir));
        return false;
    }

    QString modelPath   = dir.filePath(onnxFiles.first());
    QString voicesPath  = dir.filePath(QStringLiteral("voices.bin"));
    QString tokensPath  = dir.filePath(QStringLiteral("tokens.txt"));
    QString dataDirPath = dir.filePath(QStringLiteral("espeak-ng-data"));

    SherpaOnnxOfflineTtsConfig scfg;
    std::memset(&scfg, 0, sizeof(scfg));

    QByteArray mBytes = QDir::toNativeSeparators(modelPath).toUtf8();
    QByteArray vBytes = QDir::toNativeSeparators(voicesPath).toUtf8();
    QByteArray tBytes = QDir::toNativeSeparators(tokensPath).toUtf8();
    QByteArray dBytes = QDir::toNativeSeparators(dataDirPath).toUtf8();

    // scfg.model.kokoro.model        = mBytes.constData();
    // scfg.model.kokoro.voices       = vBytes.constData();
    // scfg.model.kokoro.tokens       = tBytes.constData();
    // scfg.model.kokoro.data_dir     = dBytes.constData();
    // scfg.model.kokoro.lexicon      = nullptr;
    // scfg.model.kokoro.length_scale = 1.0f;
    // scfg.model.num_threads         = qMax(1, QThread::idealThreadCount() - 2);

    scfg.model.kokoro.model        = mBytes.constData();
    scfg.model.kokoro.voices       = vBytes.constData();
    scfg.model.kokoro.tokens       = tBytes.constData();
    scfg.model.kokoro.data_dir     = dBytes.constData();
    scfg.model.kokoro.lexicon      = "";
    scfg.model.kokoro.length_scale = 1.0f;

    scfg.model.num_threads = qMax(1, QThread::idealThreadCount() - 2);
    scfg.model.provider    = "cpu";
    scfg.model.debug       = 1;

    scfg.max_num_sentences = 1;
    scfg.silence_scale     = 0.2f;
    scfg.model.kokoro.lang = "en";

    if (!QFileInfo::exists(voicesPath) || !QFileInfo::exists(tokensPath) || !QDir(dataDirPath).exists()) {
        emit engineError(tr("Missing Kokoro files (voices.bin, tokens.txt, or espeak-ng-data) in: %1").arg(baseDir));
        return false;
    }

    m_tts = SherpaOnnxCreateOfflineTts(&scfg);
    if (!m_tts) {
        emit engineError(tr("Failed to initialize Sherpa-ONNX Kokoro engine."));
        return false;
    }

    m_loadedModelName = QFileInfo(modelPath).baseName();
    emit modelLoadedChanged(true);
    emit loadedModelNameChanged(m_loadedModelName);
    emit optionsDiscovered();
    return true;
}

QVariantList KokoroTtsEngine::availableOptions() const
{
    QVariantList opts;

    opts.append(QVariantMap{
        {QStringLiteral("key"), QStringLiteral("voice")},
        {QStringLiteral("label"), QStringLiteral("Speaker Voice")},
        {QStringLiteral("type"), QStringLiteral("enum")},
        {QStringLiteral("defaultValue"), QStringLiteral("af_bella")},
        {QStringLiteral("choices"), m_availableVoices},
        {QStringLiteral("description"), QStringLiteral("Acoustic speaker identity profile.")}
    });

    opts.append(QVariantMap{
        {QStringLiteral("key"), QStringLiteral("speed")},
        {QStringLiteral("label"), QStringLiteral("Speaking Rate")},
        {QStringLiteral("type"), QStringLiteral("float")},
        {QStringLiteral("defaultValue"), 1.0f},
        {QStringLiteral("minValue"), 0.5f},
        {QStringLiteral("maxValue"), 2.0f},
        {QStringLiteral("description"), QStringLiteral("Speech tempo multiplier.")}
    });

    return opts;
}

bool KokoroTtsEngine::setOption(const QString &key, const QVariant &value)
{
    m_currentOptions[key] = value;
    emit optionsChanged();
    return true;
}

void KokoroTtsEngine::generate(const QString &prompt,
                               const QString &systemPrompt,
                               const QStringList &attachedFiles,
                               const QString &outputDirectory,
                               const QVariantMap &runtimeOverrides)
{
    Q_UNUSED(systemPrompt);

    if (!isModelLoaded()) {
        emit engineError(tr("Kokoro TTS engine is not loaded."));
        return;
    }

    QString voice = runtimeOverrides.value(QStringLiteral("voice"), m_currentOptions.value(QStringLiteral("voice"), QStringLiteral("af_bella"))).toString();
    float speed = runtimeOverrides.value(QStringLiteral("speed"), m_currentOptions.value(QStringLiteral("speed"), 1.0f)).toFloat();
    int speakerId = m_voiceSpeakerIds.value(voice, 1);

    m_isBusy = true;
    emit busyChanged(true);

    (void)QtConcurrent::run([this, prompt, attachedFiles, outputDirectory, voice, speed, speakerId]() {
        QString combinedScript = prompt.trimmed();

        // Ingest attached text files
        for (const QString &rawPath : attachedFiles) {
            QString cleanPath = rawPath;
            QUrl url(cleanPath);
            if(url.isLocalFile()) {
                cleanPath = url.toLocalFile();
            }
            QFile file(cleanPath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString text = in.readAll();
                text.replace(QRegularExpression(R"(^#{1,6}\s+)", QRegularExpression::MultilineOption), QString());
                text.replace(QRegularExpression(R"(\*\*|__)"), QString());
                if (!text.trimmed().isEmpty()) {
                    if (!combinedScript.isEmpty()) combinedScript += QStringLiteral("\n\n");
                    combinedScript += text.trimmed();
                }
            }
        }

        if (combinedScript.isEmpty()) {
            QMetaObject::invokeMethod(this, [this]() {
                m_isBusy = false;
                emit busyChanged(false);
                emit engineError(tr("No text payload available to synthesize."));
            });
            return;
        }

        QStringList chunks;
        for (const QString &para : combinedScript.split(QRegularExpression(R"(\r?\n+)"), Qt::SkipEmptyParts)) {
            if (para.trimmed().length() > 250) {
                chunks.append(para.trimmed().split(QRegularExpression(R"((?<=[.?!])\s+)"), Qt::SkipEmptyParts));
            } else if (!para.trimmed().isEmpty()) {
                chunks.append(para.trimmed());
            }
        }

        QByteArray totalPcmBytes;
        int resolvedSampleRate = 24000;

        SherpaOnnxGenerationConfig gcfg;
        std::memset(&gcfg, 0, sizeof(gcfg));
        gcfg.sid = speakerId;
        gcfg.speed = speed;

        for (const QString &chunk : chunks) {
            if (!m_isBusy) break; // Canceled
            QByteArray utf8 = chunk.trimmed().toUtf8();
            const SherpaOnnxGeneratedAudio *audio = SherpaOnnxOfflineTtsGenerateWithConfig(m_tts, utf8.constData(), &gcfg, nullptr, nullptr);
            if (audio && audio->n > 0) {
                resolvedSampleRate = audio->sample_rate;
                for (int i = 0; i < audio->n; ++i) {
                    float s = qBound(-1.0f, audio->samples[i], 1.0f);
                    qint16 val = static_cast<qint16>(s * 32767.0f);
                    totalPcmBytes.append(reinterpret_cast<const char *>(&val), sizeof(qint16));
                }
                // Trailing pause (150ms)
                int pauseSamples = static_cast<int>(resolvedSampleRate * 0.15f);
                qint16 zeroVal = 0;
                for (int p = 0; p < pauseSamples; ++p) {
                    totalPcmBytes.append(reinterpret_cast<const char *>(&zeroVal), sizeof(qint16));
                }
                SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
            }
        }

        QByteArray header(44, 0);
        char *d = header.data();
        quint32 chunkSize     = 36 + totalPcmBytes.size();
        quint32 subChunk1Size = 16;
        quint16 audioFormat   = 1;
        quint16 numChannels   = 1;
        quint32 sRate         = static_cast<quint32>(resolvedSampleRate);
        quint32 bRate         = static_cast<quint32>(resolvedSampleRate * sizeof(qint16));
        quint16 bAlign        = sizeof(qint16);
        quint16 bps           = 16;
        quint32 pcmSize       = static_cast<quint32>(totalPcmBytes.size());

        std::memcpy(d + 0,  "RIFF", 4);
        std::memcpy(d + 4,  &chunkSize, 4);
        std::memcpy(d + 8,  "WAVEfmt ", 8);
        std::memcpy(d + 16, &subChunk1Size, 4);
        std::memcpy(d + 20, &audioFormat, 2);
        std::memcpy(d + 22, &numChannels, 2);
        std::memcpy(d + 24, &sRate, 4);
        std::memcpy(d + 28, &bRate, 4);
        std::memcpy(d + 32, &bAlign, 2);
        std::memcpy(d + 34, &bps, 2);
        std::memcpy(d + 36, "data", 4);
        std::memcpy(d + 40, &pcmSize, 4);

        QByteArray finalWav = header + totalPcmBytes;

        QString targetDir = outputDirectory.trimmed();
        if (targetDir.isEmpty()) {
            targetDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/audio");
        }
        QDir outDir(targetDir);
        outDir.mkpath(QStringLiteral("."));
        QString filename = QStringLiteral("kokoro_%1_%2.wav").arg(voice, QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss")));
        QString savedPath = QDir::toNativeSeparators(outDir.filePath(filename));
        QFile wavFile(savedPath);
        if (wavFile.open(QIODevice::WriteOnly)) {
            wavFile.write(finalWav);
            wavFile.close();
        }

        QMetaObject::invokeMethod(this, [this, finalWav, savedPath]() {
            m_isBusy = false;
            emit busyChanged(false);

            QVariantMap result;
            result[QStringLiteral("filePath")] = savedPath;
            result[QStringLiteral("wavBytes")] = finalWav;
            result[QStringLiteral("category")] = QStringLiteral("audio");

            emit generationFinished(result);
        });
    });
}

void KokoroTtsEngine::synthesizeSpeech(const QString &text,
                                       const QString &voiceName,
                                       float speed,
                                       const QStringList &attachedFiles,
                                       const QString &outputDirectory)
{
    QVariantMap overrides;
    if (!voiceName.isEmpty()) overrides[QStringLiteral("voice")] = voiceName;
    if (speed > 0.01f) overrides[QStringLiteral("speed")] = speed;

    generate(text, QString(), attachedFiles, outputDirectory, overrides);
}

void KokoroTtsEngine::verifyConnection() {
    // Check if the configured model file actually exists on disk
    if (!isModelLoaded() || m_loadedModelName.isEmpty()) {
        emit connectionVerified(false, tr("Local model file not found."));
    } else {
        emit connectionVerified(true, tr("Local weights found and ready for inference."));
    }
}