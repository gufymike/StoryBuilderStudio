// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

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
#include <QMutex>
#include <QMutexLocker>
#include <QFuture>
#include <QHash>
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

namespace {
QMutex s_kokoroWorkerMutex;
QHash<const KokoroTtsEngine *, QFuture<void>> s_kokoroWorkers;

void trackKokoroWorker(const KokoroTtsEngine *engine, const QFuture<void> &future)
{
    QMutexLocker locker(&s_kokoroWorkerMutex);
    s_kokoroWorkers.insert(engine, future);
}

void waitForKokoroWorker(const KokoroTtsEngine *engine)
{
    QFuture<void> future;
    {
        QMutexLocker locker(&s_kokoroWorkerMutex);
        future = s_kokoroWorkers.take(engine);
    }
    if (future.isValid() && future.isRunning()) {
        future.waitForFinished();
    }
}

QStringList kokoroV019VoiceList()
{
    return {
        QStringLiteral("af"),
        QStringLiteral("af_bella"),
        QStringLiteral("af_nicole"),
        QStringLiteral("af_sarah"),
        QStringLiteral("af_sky"),
        QStringLiteral("am_adam"),
        QStringLiteral("am_michael"),
        QStringLiteral("bf_emma"),
        QStringLiteral("bf_isabella"),
        QStringLiteral("bm_george"),
        QStringLiteral("bm_lewis")
    };
}

QStringList kokoroV10MultiLangVoiceList()
{
    // Ordered identically to speaker IDs 0..53 in Kokoro v1.0 voices.bin
    // (0..27 are the 28 English voices in kokoro-en-v1_0; 28..53 are multi-language voices)
    return {
        // American Female (0..10)
        QStringLiteral("af_alloy"),
        QStringLiteral("af_aoede"),
        QStringLiteral("af_bella"),
        QStringLiteral("af_heart"),
        QStringLiteral("af_jessica"),
        QStringLiteral("af_kore"),
        QStringLiteral("af_nicole"),
        QStringLiteral("af_nova"),
        QStringLiteral("af_river"),
        QStringLiteral("af_sarah"),
        QStringLiteral("af_sky"),
        // American Male (11..19)
        QStringLiteral("am_adam"),
        QStringLiteral("am_echo"),
        QStringLiteral("am_eric"),
        QStringLiteral("am_fenrir"),
        QStringLiteral("am_liam"),
        QStringLiteral("am_michael"),
        QStringLiteral("am_onyx"),
        QStringLiteral("am_puck"),
        QStringLiteral("am_santa"),
        // British Female (20..23)
        QStringLiteral("bf_alice"),
        QStringLiteral("bf_emma"),
        QStringLiteral("bf_isabella"),
        QStringLiteral("bf_lily"),
        // British Male (24..27)
        QStringLiteral("bm_daniel"),
        QStringLiteral("bm_fable"),
        QStringLiteral("bm_george"),
        QStringLiteral("bm_lewis"),
        // Spanish (28..30)
        QStringLiteral("ef_dora"),
        QStringLiteral("em_alex"),
        QStringLiteral("em_santa"),
        // French (31)
        QStringLiteral("ff_siwis"),
        // Hindi (32..35)
        QStringLiteral("hf_alpha"),
        QStringLiteral("hf_beta"),
        QStringLiteral("hm_omega"),
        QStringLiteral("hm_psi"),
        // Italian (36..37)
        QStringLiteral("if_sara"),
        QStringLiteral("im_nicola"),
        // Japanese (38..42)
        QStringLiteral("jf_alpha"),
        QStringLiteral("jf_gongitsune"),
        QStringLiteral("jf_nezumi"),
        QStringLiteral("jf_tebukuro"),
        QStringLiteral("jm_kumo"),
        // Portuguese (43..45)
        QStringLiteral("pf_dora"),
        QStringLiteral("pm_alex"),
        QStringLiteral("pm_santa"),
        // Mandarin Chinese (46..53)
        QStringLiteral("zf_xiaobei"),
        QStringLiteral("zf_xiaoni"),
        QStringLiteral("zf_xiaoxiao"),
        QStringLiteral("zf_xiaoyi"),
        QStringLiteral("zm_yunjian"),
        QStringLiteral("zm_yunxi"),
        QStringLiteral("zm_yunxia"),
        QStringLiteral("zm_yunyang")
    };
}
} // namespace

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
        m_currentOptions[QStringLiteral("voice")] = voiceName;
        emit activeVoiceChanged();
        emit optionsChanged();
    }
}

void KokoroTtsEngine::initVoices()
{
    m_voiceSpeakerIds.clear();
    m_availableVoices.clear();

    const QStringList v019Voices = kokoroV019VoiceList();
    for (int i = 0; i < v019Voices.size(); ++i) {
        m_voiceSpeakerIds.insert(v019Voices.at(i), i);
        m_availableVoices.append(v019Voices.at(i));
    }

    m_activeVoice = QStringLiteral("af_bella");
    m_currentOptions[QStringLiteral("voice")] = m_activeVoice;
    m_currentOptions[QStringLiteral("speed")] = 1.0f;
}

void KokoroTtsEngine::unloadModel()
{
    if (m_isBusy) {
        cancelGeneration();
    }
    waitForKokoroWorker(this);

    if (m_tts) {
        qInfo() << "[KokoroTtsEngine] Destroying Sherpa-ONNX offline TTS context...";
        SherpaOnnxDestroyOfflineTts(m_tts);
        m_tts = nullptr;
        m_loadedModelName.clear();
        initVoices();
        emit modelLoadedChanged(false);
        emit loadedModelNameChanged(m_loadedModelName);
        emit optionsDiscovered();
        emit optionsChanged();
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

    // Detect Kokoro model version via speaker count (v0.19 = 11, v1.0-en = 28, v1.0-multi = 54)
    const int numSpeakers = SherpaOnnxOfflineTtsNumSpeakers(m_tts);
    m_voiceSpeakerIds.clear();
    m_availableVoices.clear();

    if (numSpeakers == 11) {
        const QStringList v019Voices = kokoroV019VoiceList();
        for (int i = 0; i < v019Voices.size(); ++i) {
            m_voiceSpeakerIds.insert(v019Voices.at(i), i);
            m_availableVoices.append(v019Voices.at(i));
        }
        qInfo() << "[KokoroTtsEngine] Detected Kokoro v0.19 model with 11 voices.";
    } else if (numSpeakers > 0) {
        const QStringList v10Voices = kokoroV10MultiLangVoiceList();
        const int mappedCount = qMin(numSpeakers, v10Voices.size());
        for (int i = 0; i < mappedCount; ++i) {
            m_voiceSpeakerIds.insert(v10Voices.at(i), i);
            m_availableVoices.append(v10Voices.at(i));
        }
        for (int i = mappedCount; i < numSpeakers; ++i) {
            const QString fallbackName = QStringLiteral("speaker_%1").arg(i);
            m_voiceSpeakerIds.insert(fallbackName, i);
            m_availableVoices.append(fallbackName);
        }
        qInfo() << "[KokoroTtsEngine] Detected Kokoro v1.0+ model with" << numSpeakers << "speakers.";
    } else {
        initVoices();
    }

    if (!m_voiceSpeakerIds.contains(m_activeVoice) && !m_availableVoices.isEmpty()) {
        m_activeVoice = m_voiceSpeakerIds.contains(QStringLiteral("af_bella"))
        ? QStringLiteral("af_bella")
        : m_availableVoices.first();
    }
    m_currentOptions[QStringLiteral("voice")] = m_activeVoice;

    m_loadedModelName = QFileInfo(modelPath).baseName();
    emit activeVoiceChanged();
    emit modelLoadedChanged(true);
    emit loadedModelNameChanged(m_loadedModelName);
    emit optionsDiscovered();
    emit optionsChanged();
    return true;
}

QVariantList KokoroTtsEngine::availableOptions() const
{
    QVariantList opts;

    const QString defaultVoice = m_voiceSpeakerIds.contains(m_activeVoice)
                                     ? m_activeVoice
                                     : (m_availableVoices.isEmpty() ? QStringLiteral("af_bella") : m_availableVoices.first());

    opts.append(QVariantMap{
        {QStringLiteral("key"), QStringLiteral("voice")},
        {QStringLiteral("label"), QStringLiteral("Speaker Voice")},
        {QStringLiteral("type"), QStringLiteral("enum")},
        {QStringLiteral("defaultValue"), defaultVoice},
        {QStringLiteral("choices"), m_availableVoices},
        {QStringLiteral("description"), QStringLiteral("Acoustic speaker identity profile (%1 voices available).").arg(m_availableVoices.size())}
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
    if (key == QStringLiteral("voice")) {
        const QString voiceName = value.toString();
        if (m_voiceSpeakerIds.contains(voiceName) && m_activeVoice != voiceName) {
            m_activeVoice = voiceName;
            emit activeVoiceChanged();
        }
    }
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

    if (!isModelLoaded() || !m_tts) {
        emit engineError(tr("Kokoro TTS engine is not loaded."));
        return;
    }

    if (m_isBusy) {
        emit engineError(tr("Kokoro TTS engine is already synthesizing audio."));
        return;
    }

    QString voice = runtimeOverrides.value(QStringLiteral("voice"), m_currentOptions.value(QStringLiteral("voice"), m_activeVoice)).toString();
    if (!m_voiceSpeakerIds.contains(voice) && !m_availableVoices.isEmpty()) {
        voice = m_availableVoices.first();
    }
    float speed = runtimeOverrides.value(QStringLiteral("speed"), m_currentOptions.value(QStringLiteral("speed"), 1.0f)).toFloat();
    int speakerId = m_voiceSpeakerIds.value(voice, 0);

    m_isBusy = true;
    emit busyChanged(true);

    QFuture<void> future = QtConcurrent::run([this, prompt, attachedFiles, outputDirectory, voice, speed, speakerId]() {
        QString combinedScript = prompt.trimmed();

        // Ingest attached text files
        for (const QString &rawPath : attachedFiles) {
            QString cleanPath = rawPath;
            QUrl url(cleanPath);
            if (url.isLocalFile()) {
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
            }, Qt::QueuedConnection);
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
            if (!m_isBusy || !m_tts) break; // Canceled or unloaded
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
            } else if (audio) {
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
        }, Qt::QueuedConnection);
    });

    trackKokoroWorker(this, future);
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
