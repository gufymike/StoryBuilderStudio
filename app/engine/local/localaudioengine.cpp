/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: localaudioengine.cpp
** Inference driver wrapping audio.cpp for local SFX and speech synthesis.
**
****************************************************************************/
#include "localaudioengine.h"
#include <QDir>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include <QStandardPaths>
#include <QDateTime>
#include <cstring>
#include "app/settings/settingsmanager.h"

LocalAudioEngine::LocalAudioEngine(QObject *parent)
    : AbstractInferenceDriver(parent)
{
    m_currentOptions[QStringLiteral("seconds")] = QStringLiteral("5.0");
    m_currentOptions[QStringLiteral("steps")]   = QStringLiteral("25");
}

LocalAudioEngine::~LocalAudioEngine()
{
    unloadModel();
}

void LocalAudioEngine::cancelGeneration()
{
    m_abortRequested = true;
}

void LocalAudioEngine::unloadModel()
{
    if (m_isBusy) {
        cancelGeneration();
    }

    if (m_session) {
        audiocpp_session_free(m_session);
        m_session = nullptr;
    }
    if (m_model) {
        audiocpp_model_free(m_model);
        m_model = nullptr;
    }
    if (m_registry) {
        audiocpp_registry_free(m_registry);
        m_registry = nullptr;
    }

    m_loadedModelName.clear();
    emit modelLoadedChanged(false);
    emit loadedModelNameChanged(m_loadedModelName);
}

void LocalAudioEngine::handleAudioCppStatus(const audiocpp_status &status)
{
    const char *msg = audiocpp_status_string(status);
    QString qmsg = QString::fromUtf8(msg ? msg : "Unknown audio.cpp error");
    qWarning() << "[LocalAudioEngine] audio.cpp error:" << qmsg;
    emit engineError(qmsg);
}

bool LocalAudioEngine::loadModel(const QVariantMap &config)
{
    unloadModel();

    QString modelPath    = config.value(QStringLiteral("modelPath")).toString();
    QString modelFamily  = config.value(QStringLiteral("modelFamily"), QStringLiteral("stable_audio")).toString();
    QString task         = config.value(QStringLiteral("task"), QStringLiteral("gen")).toString();
    QString backend      = config.value(QStringLiteral("backendEngine"), QStringLiteral("cpu")).toString();
    int nThreads         = config.value(QStringLiteral("n_threads"), 4).toInt();
    int cpuId            = config.value(QStringLiteral("cpu_id"), 0).toInt();

    QString cleanPath = QDir::toNativeSeparators(modelPath);
    QFileInfo fileInfo(cleanPath);
    if (!fileInfo.exists()) {
        emit engineError(tr("Audio model path not found: %1").arg(cleanPath));
        return false;
    }

    QString resolvedPath = cleanPath;
    if (fileInfo.isDir()) {
        QDir dir(cleanPath);
        QStringList ggufs = dir.entryList(QStringList() << QStringLiteral("*.gguf"), QDir::Files, QDir::Name);
        if (ggufs.isEmpty()) {
            emit engineError(tr("No .gguf model found in directory: %1").arg(cleanPath));
            return false;
        }
        resolvedPath = dir.filePath(ggufs.first());
    }

    QByteArray familyUtf8 = modelFamily.toUtf8();
    audiocpp_model_config mcfg;
    std::memset(&mcfg, 0, sizeof(mcfg));
    mcfg.family_hint = familyUtf8.constData();

    audiocpp_status st = audiocpp_registry_create(nullptr, &m_registry);
    if (st != AUDIOCPP_OK || !m_registry) {
        handleAudioCppStatus(st);
        return false;
    }

    QByteArray targetUtf8 = QDir::toNativeSeparators(resolvedPath).toUtf8();
    st = audiocpp_model_load(m_registry, targetUtf8.constData(), &mcfg, nullptr, &m_model);
    if (st != AUDIOCPP_OK || !m_model) {
        handleAudioCppStatus(st);
        unloadModel();
        return false;
    }

    QByteArray backendUtf8 = backend.toUtf8();
    QByteArray taskUtf8    = task.toUtf8();

    audiocpp_backend_config bcfg = {
        backendUtf8.constData(),
        cpuId,
        nThreads
    };

    st = audiocpp_session_create(m_model, taskUtf8.constData(), "offline", &bcfg, nullptr, &m_session);
    if (st != AUDIOCPP_OK || !m_session) {
        handleAudioCppStatus(st);
        unloadModel();
        return false;
    }

    m_lastModelPath = resolvedPath;
    m_loadedModelName = QFileInfo(resolvedPath).baseName();

    emit modelLoadedChanged(true);
    emit loadedModelNameChanged(m_loadedModelName);
    emit optionsDiscovered();
    return true;
}

QVariantList LocalAudioEngine::availableOptions() const
{
    QVariantList options;
    if (!m_model) {
        return options;
    }

    size_t count = audiocpp_model_option_count(m_model, AUDIOCPP_OPTION_SCOPE_REQUEST);
    for (size_t i = 0; i < count; ++i) {
        const char *name = nullptr, *valName = nullptr, *desc = nullptr;
        const char *fallback = nullptr, *minVal = nullptr, *maxVal = nullptr;
        int required = 0;
        audiocpp_model_option(m_model, AUDIOCPP_OPTION_SCOPE_REQUEST, i,
                              &name, &valName, &desc, &fallback,
                              &minVal, &maxVal, &required);

        QString keyStr = QString::fromUtf8(name ? name : "");
        QVariantMap opt;
        opt[QStringLiteral("key")]         = keyStr;
        opt[QStringLiteral("label")]       = keyStr.toUpper();
        opt[QStringLiteral("value_name")]  = QString::fromUtf8(valName ? valName : "");
        opt[QStringLiteral("description")] = QString::fromUtf8(desc ? desc : "");

        QString defStr = QString::fromUtf8(fallback ? fallback : "");
        QString minStr = QString::fromUtf8(minVal ? minVal : "");
        QString maxStr = QString::fromUtf8(maxVal ? maxVal : "");

        if (keyStr == QStringLiteral("steps") || keyStr == QStringLiteral("sample_count")) {
            opt[QStringLiteral("type")]         = QStringLiteral("int");
            opt[QStringLiteral("defaultValue")] = defStr.isEmpty() ? 25 : defStr.toInt();
            opt[QStringLiteral("minValue")]     = minStr.isEmpty() ? 1 : minStr.toInt();
            opt[QStringLiteral("maxValue")]     = maxStr.isEmpty() ? 100 : maxStr.toInt();
            opt[QStringLiteral("step")]         = 1;
        } else if (keyStr == QStringLiteral("seconds") || keyStr == QStringLiteral("cfg_scale") || !minStr.isEmpty()) {
            opt[QStringLiteral("type")]         = QStringLiteral("float");
            opt[QStringLiteral("defaultValue")] = defStr.isEmpty() ? 5.0 : defStr.toDouble();
            opt[QStringLiteral("minValue")]     = minStr.isEmpty() ? 1.0 : minStr.toDouble();
            opt[QStringLiteral("maxValue")]     = maxStr.isEmpty() ? 30.0 : maxStr.toDouble();
            opt[QStringLiteral("step")]         = 0.5;
        } else {
            opt[QStringLiteral("type")]         = QStringLiteral("string");
            opt[QStringLiteral("defaultValue")] = defStr;
        }

        options.append(opt);
    }
    return options;
}

bool LocalAudioEngine::setOption(const QString &key, const QVariant &value)
{
    m_currentOptions[key] = value.toString();
    emit optionsChanged();
    return true;
}

void LocalAudioEngine::generate(const QString &prompt,
                                const QString &systemPrompt,
                                const QStringList &attachedFiles,
                                const QString &outputDirectory,
                                const QVariantMap &runtimeOverrides)
{
    Q_UNUSED(attachedFiles);

    if (!isModelLoaded()) {
        emit engineError(tr("Audio session is not loaded."));
        return;
    }

    m_isBusy = true;
    m_abortRequested = false;
    emit busyChanged(true);

    QString finalPrompt = prompt.trimmed();
    if (!systemPrompt.isEmpty()) {
        finalPrompt = systemPrompt.trimmed() + QStringLiteral(", ") + finalPrompt;
    }

    QVariantMap activeOpts = m_currentOptions;
    for (auto it = runtimeOverrides.constBegin(); it != runtimeOverrides.constEnd(); ++it) {
        activeOpts[it.key()] = it.value();
    }

    (void)QtConcurrent::run([this, finalPrompt, activeOpts, outputDirectory]() {
        QByteArray promptUtf8 = finalPrompt.toUtf8();
        audiocpp_request *request = audiocpp_request_create();
        audiocpp_result *result = nullptr;

        audiocpp_request_set_text(request, promptUtf8.constData(), nullptr);

        for (auto it = activeOpts.constBegin(); it != activeOpts.constEnd(); ++it) {
            QByteArray k = it.key().toUtf8();
            QByteArray v = it.value().toString().toUtf8();
            audiocpp_request_set_option(request, k.constData(), v.constData());
        }

        audiocpp_status runStatus = audiocpp_session_run(m_session, request, &result);
        if (runStatus != AUDIOCPP_OK || !result) {
            handleAudioCppStatus(runStatus);
            audiocpp_request_free(request);
            QMetaObject::invokeMethod(this, [this]() {
                m_isBusy = false;
                emit busyChanged(false);
            });
            return;
        }

        const float *samples = nullptr;
        size_t frames = 0;
        int rate = 44100;
        int channels = 1;
        audiocpp_status audioStatus = audiocpp_result_audio(result, &samples, &frames, &rate, &channels);

        if (audioStatus != AUDIOCPP_OK || !samples || frames == 0) {
            handleAudioCppStatus(audioStatus);
            audiocpp_result_free(result);
            audiocpp_request_free(request);
            QMetaObject::invokeMethod(this, [this]() {
                m_isBusy = false;
                emit busyChanged(false);
            });
            return;
        }

        size_t totalSamples = frames * static_cast<size_t>(channels);
        QByteArray pcmBytes;
        pcmBytes.reserve(static_cast<qsizetype>(totalSamples * sizeof(qint16)));

        for (size_t i = 0; i < totalSamples; ++i) {
            float s = qBound(-1.0f, samples[i], 1.0f);
            qint16 val = static_cast<qint16>(s * 32767.0f);
            pcmBytes.append(reinterpret_cast<const char *>(&val), sizeof(qint16));
        }

        QByteArray header(44, 0);
        char *d = header.data();
        quint32 chunkSize     = 36 + pcmBytes.size();
        quint32 subChunk1Size = 16;
        quint16 audioFormat   = 1;
        quint16 numChannels   = static_cast<quint16>(channels);
        quint32 sRate         = static_cast<quint32>(rate);
        quint32 bRate         = static_cast<quint32>(rate * numChannels * sizeof(qint16));
        quint16 bAlign        = static_cast<quint16>(numChannels * sizeof(qint16));
        quint16 bps           = 16;
        quint32 pcmSize       = static_cast<quint32>(pcmBytes.size());

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

        QByteArray finalWav = header + pcmBytes;
        audiocpp_result_free(result);
        audiocpp_request_free(request);

        QString savedPath = saveAudioFile(finalWav, outputDirectory, QStringLiteral("sfx"));

        QMetaObject::invokeMethod(this, [this, savedPath, finalWav]() {
            m_isBusy = false;
            emit busyChanged(false);

            QVariantMap res;
            res[QStringLiteral("filePath")] = savedPath;
            res[QStringLiteral("wavBytes")] = finalWav;
            res[QStringLiteral("category")] = QStringLiteral("audio");

            emit generationFinished(res);
        });
    });
}

QString LocalAudioEngine::saveAudioFile(const QByteArray &wavData, const QString &targetDirectory, const QString &prefix)
{
    QString destDir = targetDirectory.trimmed();
    if (destDir.isEmpty()) {
        destDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/audio");
    }
    QDir dir(destDir);
    dir.mkpath(QStringLiteral("."));

    QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss_zzz"));
    QString fileName = QStringLiteral("%1_%2.wav").arg(prefix, timestamp);
    QString fullPath = QDir::toNativeSeparators(dir.filePath(fileName));

    QFile file(fullPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(wavData);
        file.close();
        return fullPath;
    }
    return QString();
}

QStringList LocalAudioEngine::availableAudioModels() const
{
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir audioDir(appData + QStringLiteral("/audio"));
    QStringList files = audioDir.entryList(QStringList() << QStringLiteral("*.gguf"), QDir::Files, QDir::Name);
    QStringList result;
    for (const QString &f : files) {
        result << QFileInfo(f).completeBaseName();
    }
    return result;
}

QString LocalAudioEngine::getFamilyForModel(const QString &modelBaseName) const
{
    SettingsManager settings;
    return settings.getAudioCppFamily(modelBaseName);
}

void LocalAudioEngine::verifyConnection() {
    // Check if the configured model file actually exists on disk
    if (!isModelLoaded() || m_loadedModelName.isEmpty()) {
        emit connectionVerified(false, tr("Local model file not found."));
    } else {
        emit connectionVerified(true, tr("Local weights found and ready for inference."));
    }
}