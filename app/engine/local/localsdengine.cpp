// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include "localsdengine.h"
#include <QtConcurrent/QtConcurrent>
#include <QStandardPaths>
#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QThread>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QFuture>
#include <QHash>
#include <cstring>

namespace {
QMutex s_sdLogMutex;
QString s_lastSdError;

QMutex s_workerMutex;
QHash<const LocalSdEngine *, QFuture<void>> s_activeWorkers;

void clearLastSdError()
{
    QMutexLocker locker(&s_sdLogMutex);
    s_lastSdError.clear();
}

void recordSdError(const QString &msg)
{
    QMutexLocker locker(&s_sdLogMutex);
    s_lastSdError = msg;
}

QString takeLastSdError()
{
    QMutexLocker locker(&s_sdLogMutex);
    return s_lastSdError;
}

QString formatActionableSdError(const QString &prefix, const QString &rawError)
{
    if (rawError.isEmpty()) {
        return prefix;
    }

    if (rawError.contains(QStringLiteral("read tensor data failed"), Qt::CaseInsensitive) ||
        rawError.contains(QStringLiteral("weight preparation"), Qt::CaseInsensitive) ||
        rawError.contains(QStringLiteral("failed to encode prompt"), Qt::CaseInsensitive)) {
        return QStringLiteral("%1: %2 (Check available RAM/pagefile or verify the GGUF model file is complete.)")
        .arg(prefix, rawError);
    }

    return QStringLiteral("%1: %2").arg(prefix, rawError);
}

void trackWorkerFuture(const LocalSdEngine *engine, const QFuture<void> &future)
{
    QMutexLocker locker(&s_workerMutex);
    s_activeWorkers.insert(engine, future);
}

void waitForWorkerFuture(const LocalSdEngine *engine)
{
    QFuture<void> future;
    {
        QMutexLocker locker(&s_workerMutex);
        future = s_activeWorkers.take(engine);
    }
    if (future.isValid() && future.isRunning()) {
        future.waitForFinished();
    }
}
} // namespace

static void sd_log_forwarder(enum sd_log_level_t level, const char *text, void *data) {
    Q_UNUSED(data);
    if (!text) return;
    QString msg = QString::fromUtf8(text).trimmed();
    if (msg.isEmpty()) return;
    switch (level) {
    case SD_LOG_DEBUG: qDebug() << "[sd.cpp]" << msg; break;
    case SD_LOG_INFO:
    case SD_LOG_VERBOSE: qInfo() << "[sd.cpp]" << msg; break;
    case SD_LOG_WARN: qWarning() << "[sd.cpp]" << msg; break;
    case SD_LOG_ERROR:
        recordSdError(msg);
        qCritical() << "[sd.cpp-ERROR]" << msg;
        break;
    }
}

LocalSdEngine::LocalSdEngine(QObject *parent)
    : AbstractInferenceDriver(parent)
{
#ifdef SD_MODE_VIDEO
    m_isVideoMode = true;
    m_driverId = QStringLiteral("sd_video");
    m_displayName = QStringLiteral("Stable-Diffusion (Video)");
    m_targetAssetType = QStringLiteral("video");
#else
    m_isVideoMode = false;
    m_driverId = QStringLiteral("sd_image");
    m_displayName = QStringLiteral("Stable-Diffusion (Image)");
    m_targetAssetType = QStringLiteral("image");
#endif
    initDefaultOptions();
}

LocalSdEngine::~LocalSdEngine()
{
    unloadModel();
}

void LocalSdEngine::initDefaultOptions()
{
    m_currentOptions[QStringLiteral("steps")] = m_isVideoMode ? 25 : 4;
    m_currentOptions[QStringLiteral("cfg_scale")] = m_isVideoMode ? 5.0f : 1.0f;
    m_currentOptions[QStringLiteral("width")] = m_isVideoMode ? 768 : 512;
    m_currentOptions[QStringLiteral("height")] = 512;
    m_currentOptions[QStringLiteral("sample_count")] = 4;
    m_currentOptions[QStringLiteral("video_frames")] = 25;
    m_currentOptions[QStringLiteral("fps")] = 16;
    m_currentOptions[QStringLiteral("flow_shift")] = 3.0f;
    m_currentOptions[QStringLiteral("baseSeed")] = -1;
}

bool LocalSdEngine::isVideoSupported() const
{
    return isModelLoaded() && sd_ctx_supports_video_generation(m_sdCtx);
}

void LocalSdEngine::unloadModel()
{
    if (m_isBusy) {
        cancelGeneration();
    }
    waitForWorkerFuture(this);

    if (m_sdCtx) {
        qInfo() << "[LocalSdEngine] Freeing SD context...";
        free_sd_ctx(m_sdCtx);
        m_sdCtx = nullptr;
        qInfo() << "[LocalSdEngine] SD context freed.";
    }
    m_isLoaded = false;
#ifdef SD_MODE_VIDEO
    m_isVideoMode = true;
#else
    m_isVideoMode = false;
#endif
    m_activeVideoArch = VIDEO_ARCH_NONE;
    m_loadedModelName.clear();

    emit modelLoadedChanged(false);
    emit loadedModelNameChanged(m_loadedModelName);
    emit videoSupportedChanged(false);
    emit activeVideoArchChanged();
}

void LocalSdEngine::cancelGeneration()
{
    // stable-diffusion.cpp executes synchronously per step in the worker thread.
}

bool LocalSdEngine::loadModel(const QVariantMap &config)
{
    unloadModel();
    clearLastSdError();
    sd_set_log_callback(sd_log_forwarder, nullptr);

#ifdef SD_MODE_VIDEO
    QString defaultMode = QStringLiteral("video");
#else
    QString defaultMode = QStringLiteral("image");
#endif
    QString mode = config.value(QStringLiteral("mode"), defaultMode).toString();
    m_isVideoMode = (mode == QStringLiteral("video"));

    QString diffPath = config.value(QStringLiteral("diffusionModelPath")).toString();
    QString t5Path   = config.value(QStringLiteral("t5xxlPath")).toString();
    QString vaePath  = config.value(QStringLiteral("vaePath")).toString();
    QString clipL    = config.value(QStringLiteral("clipLPath")).toString();
    QString archType = config.value(QStringLiteral("engineType"), QStringLiteral("wan")).toString();

    if (m_isVideoMode) {
        return loadVideoModelInternal(diffPath, t5Path, vaePath, archType);
    } else {
        return loadImageModel(diffPath, clipL, t5Path, vaePath);
    }
}

bool LocalSdEngine::loadImageModel(const QString &diffPath, const QString &clipL, const QString &t5, const QString &vae)
{
    if (diffPath.isEmpty() || !QFileInfo::exists(diffPath)) {
        emit engineError(tr("Diffusion model path does not exist: %1").arg(diffPath));
        return false;
    }
    if (!clipL.isEmpty() && !QFileInfo::exists(clipL)) {
        emit engineError(tr("CLIP-L encoder path does not exist: %1").arg(clipL));
        return false;
    }
    if (!t5.isEmpty() && !QFileInfo::exists(t5)) {
        emit engineError(tr("T5-XXL encoder path does not exist: %1").arg(t5));
        return false;
    }
    if (!vae.isEmpty() && !QFileInfo::exists(vae)) {
        emit engineError(tr("VAE model path does not exist: %1").arg(vae));
        return false;
    }

    sd_ctx_params_t params;
    sd_ctx_params_init(&params);

    QByteArray diffBytes = QDir::toNativeSeparators(diffPath).toUtf8();
    QByteArray clipBytes = QDir::toNativeSeparators(clipL).toUtf8();
    QByteArray t5Bytes   = QDir::toNativeSeparators(t5).toUtf8();
    QByteArray vaeBytes  = QDir::toNativeSeparators(vae).toUtf8();

    params.diffusion_model_path = diffBytes.constData();
    params.clip_l_path          = clipL.isEmpty() ? nullptr : clipBytes.constData();
    params.t5xxl_path           = t5.isEmpty() ? nullptr : t5Bytes.constData();
    params.vae_path             = vae.isEmpty() ? nullptr : vaeBytes.constData();
    params.n_threads            = qMax(1, QThread::idealThreadCount() - 2);

    m_sdCtx = new_sd_ctx(&params);
    if (!m_sdCtx) {
        emit engineError(formatActionableSdError(tr("Failed to initialize SD image context"), takeLastSdError()));
        return false;
    }

    m_isLoaded = true;
    m_loadedModelName = QFileInfo(diffPath).baseName();
    qInfo() << "[LocalSdEngine] Loaded SD image model:" << m_loadedModelName;
    emit modelLoadedChanged(true);
    emit loadedModelNameChanged(m_loadedModelName);
    emit optionsDiscovered();
    return true;
}

bool LocalSdEngine::loadVideoModelInternal(const QString &diffPath, const QString &t5, const QString &vae, const QString &engineType)
{
    if (diffPath.isEmpty() || !QFileInfo::exists(diffPath)) {
        emit engineError(tr("Video diffusion model path does not exist: %1").arg(diffPath));
        return false;
    }
    if (!t5.isEmpty() && !QFileInfo::exists(t5)) {
        emit engineError(tr("Video text encoder (T5/UMT5) path does not exist: %1").arg(t5));
        return false;
    }
    if (!vae.isEmpty() && !QFileInfo::exists(vae)) {
        emit engineError(tr("Video VAE path does not exist: %1").arg(vae));
        return false;
    }

    bool isWan = engineType.contains(QStringLiteral("wan"), Qt::CaseInsensitive) || diffPath.contains(QStringLiteral("wan"), Qt::CaseInsensitive);

    sd_ctx_params_t params;
    sd_ctx_params_init(&params);

    QByteArray diffBytes = QDir::toNativeSeparators(diffPath).toUtf8();
    QByteArray t5Bytes   = QDir::toNativeSeparators(t5).toUtf8();
    QByteArray vaeBytes  = QDir::toNativeSeparators(vae).toUtf8();

    params.diffusion_model_path = diffBytes.constData();
    params.t5xxl_path           = t5.isEmpty() ? nullptr : t5Bytes.constData();
    params.vae_path             = vae.isEmpty() ? nullptr : vaeBytes.constData();
    params.n_threads            = qMax(1, QThread::idealThreadCount() - 2);

    m_sdCtx = new_sd_ctx(&params);
    if (!m_sdCtx) {
        emit engineError(formatActionableSdError(tr("Failed to initialize SD video context"), takeLastSdError()));
        return false;
    }

    m_isLoaded = true;
    m_activeVideoArch = isWan ? VIDEO_ARCH_WAN : VIDEO_ARCH_LTX;
    m_loadedModelName = QFileInfo(diffPath).baseName();
    qInfo() << "[LocalSdEngine] Loaded SD video model:" << m_loadedModelName;

    emit modelLoadedChanged(true);
    emit loadedModelNameChanged(m_loadedModelName);
    emit videoSupportedChanged(sd_ctx_supports_video_generation(m_sdCtx));
    emit activeVideoArchChanged();
    emit optionsDiscovered();
    return true;
}

QVariantList LocalSdEngine::availableOptions() const
{
    QVariantList opts;

    opts.append(QVariantMap{
        {QStringLiteral("key"), QStringLiteral("steps")},
        {QStringLiteral("label"), QStringLiteral("Inference Steps")},
        {QStringLiteral("type"), QStringLiteral("int")},
        {QStringLiteral("defaultValue"), m_isVideoMode ? 25 : 4},
        {QStringLiteral("minValue"), 1},
        {QStringLiteral("maxValue"), 50},
        {QStringLiteral("description"), QStringLiteral("Sampling denoising iterations.")}
    });

    opts.append(QVariantMap{
        {QStringLiteral("key"), QStringLiteral("cfg_scale")},
        {QStringLiteral("label"), QStringLiteral("Guidance Scale (CFG)")},
        {QStringLiteral("type"), QStringLiteral("float")},
        {QStringLiteral("defaultValue"), m_isVideoMode ? 5.0f : 1.0f},
        {QStringLiteral("minValue"), 1.0f},
        {QStringLiteral("maxValue"), 10.0f},
        {QStringLiteral("description"), QStringLiteral("Prompt adherence factor.")}
    });

    opts.append(QVariantMap{
        {QStringLiteral("key"), QStringLiteral("width")},
        {QStringLiteral("label"), QStringLiteral("Width")},
        {QStringLiteral("type"), QStringLiteral("int")},
        {QStringLiteral("defaultValue"), m_isVideoMode ? 768 : 512},
        {QStringLiteral("minValue"), 256},
        {QStringLiteral("maxValue"), 1536},
        {QStringLiteral("description"), QStringLiteral("Pixel width of generated frames.")}
    });

    opts.append(QVariantMap{
        {QStringLiteral("key"), QStringLiteral("height")},
        {QStringLiteral("label"), QStringLiteral("Height")},
        {QStringLiteral("type"), QStringLiteral("int")},
        {QStringLiteral("defaultValue"), 512},
        {QStringLiteral("minValue"), 256},
        {QStringLiteral("maxValue"), 1536},
        {QStringLiteral("description"), QStringLiteral("Pixel height of generated frames.")}
    });

    if (m_isVideoMode) {
        opts.append(QVariantMap{
            {QStringLiteral("key"), QStringLiteral("video_frames")},
            {QStringLiteral("label"), QStringLiteral("Frame Count")},
            {QStringLiteral("type"), QStringLiteral("int")},
            {QStringLiteral("defaultValue"), 25},
            {QStringLiteral("minValue"), 9},
            {QStringLiteral("maxValue"), 81},
            {QStringLiteral("description"), QStringLiteral("Total output video frames.")}
        });
        opts.append(QVariantMap{
            {QStringLiteral("key"), QStringLiteral("fps")},
            {QStringLiteral("label"), QStringLiteral("Frames Per Second")},
            {QStringLiteral("type"), QStringLiteral("int")},
            {QStringLiteral("defaultValue"), 16},
            {QStringLiteral("minValue"), 8},
            {QStringLiteral("maxValue"), 30},
            {QStringLiteral("description"), QStringLiteral("Playback frame rate.")}
        });
        opts.append(QVariantMap{
            {QStringLiteral("key"), QStringLiteral("flow_shift")},
            {QStringLiteral("label"), QStringLiteral("Flow Shift Scale")},
            {QStringLiteral("type"), QStringLiteral("float")},
            {QStringLiteral("defaultValue"), 3.0f},
            {QStringLiteral("minValue"), 1.0f},
            {QStringLiteral("maxValue"), 7.0f},
            {QStringLiteral("description"), QStringLiteral("Motion magnitude scale for Wan models.")}
        });
    } else {
        opts.append(QVariantMap{
            {QStringLiteral("key"), QStringLiteral("sample_count")},
            {QStringLiteral("label"), QStringLiteral("Candidates")},
            {QStringLiteral("type"), QStringLiteral("int")},
            {QStringLiteral("defaultValue"), 4},
            {QStringLiteral("minValue"), 1},
            {QStringLiteral("maxValue"), 4},
            {QStringLiteral("description"), QStringLiteral("Number of image candidate variations.")}
        });
    }

    return opts;
}

bool LocalSdEngine::setOption(const QString &key, const QVariant &value)
{
    m_currentOptions[key] = value;
    emit optionsChanged();
    return true;
}

void LocalSdEngine::generate(const QString &prompt,
                             const QString &systemPrompt,
                             const QStringList &attachedFiles,
                             const QString &outputDirectory,
                             const QVariantMap &runtimeOverrides)
{
    Q_UNUSED(systemPrompt);

    if (!isModelLoaded() || !m_sdCtx) {
        emit engineError(tr("No diffusion model loaded."));
        return;
    }

    if (m_isBusy) {
        emit engineError(tr("Diffusion engine is already busy generating."));
        return;
    }

    int steps = runtimeOverrides.value(QStringLiteral("steps"), m_currentOptions.value(QStringLiteral("steps"), 4)).toInt();
    float cfg = runtimeOverrides.value(QStringLiteral("cfg_scale"), m_currentOptions.value(QStringLiteral("cfg_scale"), 1.0f)).toFloat();
    int w = runtimeOverrides.value(QStringLiteral("width"), m_currentOptions.value(QStringLiteral("width"), 512)).toInt();
    int h = runtimeOverrides.value(QStringLiteral("height"), m_currentOptions.value(QStringLiteral("height"), 512)).toInt();
    int baseSeed = runtimeOverrides.value(QStringLiteral("seed"), -1).toInt();

    if (m_isVideoMode) {
        if (!sd_ctx_supports_video_generation(m_sdCtx)) {
            emit engineError(tr("Loaded diffusion model does not support video generation."));
            return;
        }

        int frames = runtimeOverrides.value(QStringLiteral("video_frames"), m_currentOptions.value(QStringLiteral("video_frames"), 25)).toInt();
        int fps = runtimeOverrides.value(QStringLiteral("fps"), m_currentOptions.value(QStringLiteral("fps"), 16)).toInt();
        float shift = runtimeOverrides.value(QStringLiteral("flow_shift"), m_currentOptions.value(QStringLiteral("flow_shift"), 3.0f)).toFloat();

        QString initImg;
        for (const QString &f : attachedFiles) {
            QString lower = f.toLower();
            if (lower.endsWith(QStringLiteral(".png")) || lower.endsWith(QStringLiteral(".jpg")) || lower.endsWith(QStringLiteral(".jpeg")) || lower.endsWith(QStringLiteral(".webp"))) {
                initImg = f;
                break;
            }
        }

        runVideoGeneration(prompt, initImg, outputDirectory, w, h, frames, fps, steps, cfg, shift, baseSeed);
    } else {
        int count = runtimeOverrides.value(QStringLiteral("sample_count"), m_currentOptions.value(QStringLiteral("sample_count"), 4)).toInt();
        runImageGeneration(prompt, count, w, h, steps, cfg, baseSeed);
    }
}

void LocalSdEngine::runImageGeneration(const QString &prompt, int samples, int width, int height, int steps, float cfgScale, int baseSeed)
{
    m_isBusy = true;
    emit busyChanged(true);
    clearLastSdError();

    QFuture<void> future = QtConcurrent::run([this, prompt, samples, width, height, steps, cfgScale, baseSeed]() {
        QVariantList candidateBase64List;
        QByteArray promptBytes = prompt.toUtf8();
        QString generationError;

        for (int i = 0; i < samples; ++i) {
            if (!m_sdCtx) {
                generationError = tr("Diffusion context was unloaded during generation.");
                break;
            }

            sd_img_gen_params_t genParams;
            sd_img_gen_params_init(&genParams);

            genParams.prompt = promptBytes.constData();
            genParams.negative_prompt = "";
            genParams.clip_skip = -1;
            genParams.width = width;
            genParams.height = height;
            genParams.seed = (baseSeed == -1) ? -1 : (baseSeed + i);
            genParams.batch_count = 1;
            genParams.sample_params.sample_method = EULER_SAMPLE_METHOD;
            genParams.sample_params.sample_steps = steps;
            genParams.sample_params.guidance.txt_cfg = cfgScale;
            genParams.sample_params.guidance.distilled_guidance = 1.0f;

            sd_image_generation_result_t *outImages = nullptr;
            bool ok = generate_image(m_sdCtx, &genParams, &outImages);

            if (ok && outImages && outImages->count > 0 && outImages->items) {
                for (int j = 0; j < outImages->count; ++j) {
                    const sd_image_t &sdImg = outImages->items[j].image;
                    if (!sdImg.data || sdImg.width == 0 || sdImg.height == 0) continue;

                    QImage img(sdImg.data, sdImg.width, sdImg.height, sdImg.width * sdImg.channel, QImage::Format_RGB888);
                    QByteArray rawBytes;
                    QBuffer buffer(&rawBytes);
                    buffer.open(QIODevice::WriteOnly);
                    img.save(&buffer, "PNG");
                    candidateBase64List.append(QStringLiteral("data:image/png;base64,") + rawBytes.toBase64());
                }
                free_sd_image_generation_result(outImages);
            } else {
                if (outImages) {
                    free_sd_image_generation_result(outImages);
                }
                generationError = formatActionableSdError(tr("Image generation failed"), takeLastSdError());
                break;
            }
        }

        QMetaObject::invokeMethod(this, [this, candidateBase64List, generationError]() {
            m_isBusy = false;
            emit busyChanged(false);

            if (candidateBase64List.isEmpty()) {
                emit engineError(generationError.isEmpty()
                                 ? tr("Image generation produced 0 images.")
                                 : generationError);
                return;
            }

            if (!generationError.isEmpty()) {
                qWarning() << "[LocalSdEngine] Partial batch completed before error:" << generationError;
            }

            QVariantMap result;
            result[QStringLiteral("candidates")] = candidateBase64List;
            result[QStringLiteral("count")] = candidateBase64List.size();
            result[QStringLiteral("category")] = QStringLiteral("image");

            emit generationFinished(result);
        }, Qt::QueuedConnection);
    });

    trackWorkerFuture(this, future);
}

void LocalSdEngine::runVideoGeneration(const QString &prompt, const QString &inputImagePath, const QString &outputDirectory,
                                       int width, int height, int videoFrames, int fps, int steps, float cfgScale, float flowShift, int baseSeed)
{
    m_isBusy = true;
    emit busyChanged(true);
    clearLastSdError();
    bool isWan = (m_activeVideoArch == VIDEO_ARCH_WAN);

    int effectiveFrames = videoFrames;
    if (isWan) {
        effectiveFrames = ((videoFrames - 1) / 4) * 4 + 1;
        if (effectiveFrames < 5) effectiveFrames = 17;
    }

    QFuture<void> future = QtConcurrent::run([this, prompt, inputImagePath, outputDirectory, width, height, effectiveFrames, fps, steps, cfgScale, flowShift, isWan, baseSeed]() {
        if (!m_sdCtx) {
            QMetaObject::invokeMethod(this, [this]() {
                m_isBusy = false;
                emit busyChanged(false);
                emit engineError(tr("Diffusion context is not initialized."));
            }, Qt::QueuedConnection);
            return;
        }

        sd_vid_gen_params_t vidParams;
        sd_vid_gen_params_init(&vidParams);

        QByteArray promptBytes = prompt.toUtf8();
        vidParams.prompt = promptBytes.constData();
        vidParams.negative_prompt = "low quality, worst quality, deformed, blurry";
        vidParams.width = width;
        vidParams.height = height;
        vidParams.video_frames = effectiveFrames;
        vidParams.fps = fps;
        vidParams.seed = baseSeed;

        if (isWan) {
            vidParams.sample_params.sample_method = EULER_SAMPLE_METHOD;
            vidParams.sample_params.scheduler     = DISCRETE_SCHEDULER;
            vidParams.sample_params.sample_steps  = steps;
            vidParams.sample_params.guidance.txt_cfg = cfgScale;
            vidParams.sample_params.flow_shift    = flowShift;
        } else {
            vidParams.sample_params.sample_method = EULER_SAMPLE_METHOD;
            vidParams.sample_params.scheduler     = LTX2_SCHEDULER;
            vidParams.sample_params.sample_steps  = steps;
            vidParams.sample_params.guidance.txt_cfg = cfgScale;
            vidParams.sample_params.flow_shift    = 1.0f;
        }

        // Pack init image into contiguous RGB888 bytes without QImage 4-byte scanline padding
        QByteArray packedInitRgb;
        if (!inputImagePath.isEmpty()) {
            QImage initImg;
            if (!initImg.load(inputImagePath)) {
                QMetaObject::invokeMethod(this, [this, inputImagePath]() {
                    m_isBusy = false;
                    emit busyChanged(false);
                    emit engineError(tr("Failed to load reference image for video generation: %1").arg(inputImagePath));
                }, Qt::QueuedConnection);
                return;
            }

            initImg = initImg.convertToFormat(QImage::Format_RGB888)
                          .scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

            const int rowBytes = initImg.width() * 3;
            packedInitRgb.resize(rowBytes * initImg.height());
            for (int y = 0; y < initImg.height(); ++y) {
                std::memcpy(packedInitRgb.data() + (y * rowBytes), initImg.constScanLine(y), static_cast<size_t>(rowBytes));
            }

            vidParams.init_image.width   = static_cast<uint32_t>(initImg.width());
            vidParams.init_image.height  = static_cast<uint32_t>(initImg.height());
            vidParams.init_image.channel = 3;
            vidParams.init_image.data    = reinterpret_cast<uint8_t *>(packedInitRgb.data());
            vidParams.strength           = 0.85f;
        }

        sd_video_generation_result_t *videoResult = nullptr;
        bool ok = generate_video(m_sdCtx, &vidParams, &videoResult);

        QString targetDir = outputDirectory;
        if (targetDir.isEmpty()) {
            targetDir = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
            .filePath(QStringLiteral("vid_%1").arg(QDateTime::currentMSecsSinceEpoch()));
        }

        int numFramesOut = 0;
        int effectiveFps = fps;
        QString generationError;

        if (ok && videoResult && videoResult->frame_count > 0 && videoResult->frames) {
            QDir().mkpath(targetDir);
            effectiveFps = (videoResult->fps > 0) ? videoResult->fps : fps;

            for (int i = 0; i < videoResult->frame_count; ++i) {
                const sd_image_t &frame = videoResult->frames[i];
                if (!frame.data || frame.width == 0 || frame.height == 0) continue;
                QImage f(frame.data, frame.width, frame.height, frame.width * frame.channel, QImage::Format_RGB888);
                if (f.save(QStringLiteral("%1/frame_%2.png").arg(targetDir).arg(numFramesOut, 4, 10, QChar('0')))) {
                    ++numFramesOut;
                }
            }
        }

        if (videoResult) {
            free_sd_video_generation_result(videoResult);
        }

        if (numFramesOut == 0) {
            generationError = formatActionableSdError(tr("Video generation failed"), takeLastSdError());
        }

        QMetaObject::invokeMethod(this, [this, targetDir, numFramesOut, effectiveFps, generationError]() {
            m_isBusy = false;
            emit busyChanged(false);

            if (numFramesOut == 0) {
                emit engineError(generationError.isEmpty()
                                 ? tr("Video generation produced 0 frames.")
                                 : generationError);
                return;
            }

            QVariantMap result;
            result[QStringLiteral("outputDir")] = targetDir;
            result[QStringLiteral("frameCount")] = numFramesOut;
            result[QStringLiteral("fps")] = effectiveFps;
            result[QStringLiteral("category")] = QStringLiteral("video");

            emit generationFinished(result);
        }, Qt::QueuedConnection);
    });

    trackWorkerFuture(this, future);
}

void LocalSdEngine::verifyConnection() {
    // Check if the configured model file actually exists on disk
    if (!isModelLoaded() || m_loadedModelName.isEmpty()) {
        emit connectionVerified(false, tr("Local model file not found."));
    } else {
        emit connectionVerified(true, tr("Local weights found and ready for inference."));
    }
}
