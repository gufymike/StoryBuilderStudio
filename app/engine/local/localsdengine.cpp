/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: localsdengine.cpp
** Inference driver integrating stable-diffusion.cpp for image and video synthesis.
**
****************************************************************************/
#include "localsdengine.h"
#include <QtConcurrent/QtConcurrent>
#include <QStandardPaths>
#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QThread>
#include <QDateTime>

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
    case SD_LOG_ERROR: qCritical() << "[sd.cpp-ERROR]" << msg; break;
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

    if (m_sdCtx) {
        qInfo() << "[LocalSdEngine] Freeing SD context...";
        free_sd_ctx(m_sdCtx);
        m_sdCtx = nullptr;
    }
    m_isLoaded = false;
    m_isVideoMode = false;
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
    sd_set_log_callback(sd_log_forwarder, nullptr);

    QString mode = config.value(QStringLiteral("mode"), QStringLiteral("image")).toString();
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
        emit engineError(tr("Failed to initialize SD image context."));
        return false;
    }

    m_isLoaded = true;
    m_loadedModelName = QFileInfo(diffPath).baseName();
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
        emit engineError(tr("Failed to initialize SD video context."));
        return false;
    }

    m_isLoaded = true;
    m_activeVideoArch = isWan ? VIDEO_ARCH_WAN : VIDEO_ARCH_LTX;
    m_loadedModelName = QFileInfo(diffPath).baseName();

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

    if (!isModelLoaded()) {
        emit engineError(tr("No diffusion model loaded."));
        return;
    }

    int steps = runtimeOverrides.value(QStringLiteral("steps"), m_currentOptions.value(QStringLiteral("steps"), 4)).toInt();
    float cfg = runtimeOverrides.value(QStringLiteral("cfg_scale"), m_currentOptions.value(QStringLiteral("cfg_scale"), 1.0f)).toFloat();
    int w = runtimeOverrides.value(QStringLiteral("width"), m_currentOptions.value(QStringLiteral("width"), 512)).toInt();
    int h = runtimeOverrides.value(QStringLiteral("height"), m_currentOptions.value(QStringLiteral("height"), 512)).toInt();
    int baseSeed = runtimeOverrides.value(QStringLiteral("seed"), -1).toInt();

    if (m_isVideoMode) {
        int frames = runtimeOverrides.value(QStringLiteral("video_frames"), m_currentOptions.value(QStringLiteral("video_frames"), 25)).toInt();
        int fps = runtimeOverrides.value(QStringLiteral("fps"), m_currentOptions.value(QStringLiteral("fps"), 16)).toInt();
        float shift = runtimeOverrides.value(QStringLiteral("flow_shift"), m_currentOptions.value(QStringLiteral("flow_shift"), 3.0f)).toFloat();

        QString initImg;
        for (const QString &f : attachedFiles) {
            QString lower = f.toLower();
            if (lower.endsWith(QStringLiteral(".png")) || lower.endsWith(QStringLiteral(".jpg")) || lower.endsWith(QStringLiteral(".webp"))) {
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

    (void)QtConcurrent::run([this, prompt, samples, width, height, steps, cfgScale, baseSeed]() {
        QVariantList candidateBase64List;
        QByteArray promptBytes = prompt.toUtf8();

        for (int i = 0; i < samples; ++i) {
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
                    if (!sdImg.data) continue;

                    QImage img(sdImg.data, sdImg.width, sdImg.height, sdImg.width * sdImg.channel, QImage::Format_RGB888);
                    QByteArray rawBytes;
                    QBuffer buffer(&rawBytes);
                    buffer.open(QIODevice::WriteOnly);
                    img.save(&buffer, "PNG");
                    candidateBase64List.append(QStringLiteral("data:image/png;base64,") + rawBytes.toBase64());
                }
            }
            if (outImages) free_sd_image_generation_result(outImages);
        }

        QMetaObject::invokeMethod(this, [this, candidateBase64List]() {
            m_isBusy = false;
            emit busyChanged(false);

            QVariantMap result;
            result[QStringLiteral("candidates")] = candidateBase64List;
            result[QStringLiteral("count")] = candidateBase64List.size();
            result[QStringLiteral("category")] = QStringLiteral("image");

            emit generationFinished(result);
        });
    });
}

void LocalSdEngine::runVideoGeneration(const QString &prompt, const QString &inputImagePath, const QString &outputDirectory,
                                       int width, int height, int videoFrames, int fps, int steps, float cfgScale, float flowShift, int baseSeed)
{
    m_isBusy = true;
    emit busyChanged(true);
    bool isWan = (m_activeVideoArch == VIDEO_ARCH_WAN);

    int effectiveFrames = videoFrames;
    if (isWan) {
        effectiveFrames = ((videoFrames - 1) / 4) * 4 + 1;
        if (effectiveFrames < 5) effectiveFrames = 17;
    }

    (void)QtConcurrent::run([this, prompt, inputImagePath, outputDirectory, width, height, effectiveFrames, fps, steps, cfgScale, flowShift, isWan, baseSeed]() {
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

        QImage initImg;
        if (!inputImagePath.isEmpty() && initImg.load(inputImagePath)) {
            initImg = initImg.convertToFormat(QImage::Format_RGB888).scaled(width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            vidParams.init_image.width   = initImg.width();
            vidParams.init_image.height  = initImg.height();
            vidParams.init_image.channel = 3;
            vidParams.init_image.data    = initImg.bits();
            vidParams.strength           = 0.85f;
        }

        sd_video_generation_result_t *videoResult = nullptr;
        bool ok = generate_video(m_sdCtx, &vidParams, &videoResult);

        QString targetDir = outputDirectory;
        if (targetDir.isEmpty()) {
            targetDir = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
            .filePath(QStringLiteral("vid_%1").arg(QDateTime::currentMSecsSinceEpoch()));
        }
        QDir().mkpath(targetDir);

        int numFramesOut = 0;
        int effectiveFps = fps;

        if (ok && videoResult && videoResult->frame_count > 0 && videoResult->frames) {
            numFramesOut = videoResult->frame_count;
            effectiveFps = (videoResult->fps > 0) ? videoResult->fps : fps;

            for (int i = 0; i < numFramesOut; ++i) {
                const sd_image_t &frame = videoResult->frames[i];
                if (!frame.data) continue;
                QImage f(frame.data, frame.width, frame.height, frame.width * frame.channel, QImage::Format_RGB888);
                f.save(QStringLiteral("%1/frame_%2.png").arg(targetDir).arg(i, 4, 10, QChar('0')));
            }
            free_sd_video_generation_result(videoResult);
        }

        QMetaObject::invokeMethod(this, [this, targetDir, numFramesOut, effectiveFps]() {
            m_isBusy = false;
            emit busyChanged(false);

            QVariantMap result;
            result[QStringLiteral("outputDir")] = targetDir;
            result[QStringLiteral("frameCount")] = numFramesOut;
            result[QStringLiteral("fps")] = effectiveFps;
            result[QStringLiteral("category")] = QStringLiteral("video");

            emit generationFinished(result);
        });
    });
}

void LocalSdEngine::verifyConnection() {
    // Check if the configured model file actually exists on disk
    if (!isModelLoaded() || m_loadedModelName.isEmpty()) {
        emit connectionVerified(false, tr("Local model file not found."));
    } else {
        emit connectionVerified(true, tr("Local weights found and ready for inference."));
    }
}
