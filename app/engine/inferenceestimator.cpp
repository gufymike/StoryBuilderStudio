// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include "inferenceestimator.h"
#include <QStandardPaths>
#include <QSettings>
#include <QSet>
#include <QVariantMap>
#include <cmath>

namespace {

bool containsAnyToken(const QString &haystack, const QStringList &tokens)
{
    for (const QString &token : tokens) {
        if (!token.isEmpty() && haystack.contains(token, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

qint64 configuredWeightsSize(const QString &iniPath,
                             const QStringList &keyHints,
                             const QStringList &fileTokens,
                             const QStringList &excludeTokens)
{
    if (!QFileInfo::exists(iniPath)) {
        return 0;
    }

    QSettings settings(iniPath, QSettings::IniFormat);
    const QStringList allKeys = settings.allKeys();

    QSet<QString> seenPaths;
    qint64 totalBytes = 0;

    for (const QString &fullKey : allKeys) {
        const QString rawPath = settings.value(fullKey).toString().trimmed();
        if (rawPath.isEmpty()) {
            continue;
        }

        const QFileInfo fi(rawPath);
        if (!fi.exists() || !fi.isFile()) {
            continue;
        }

        const QString fileName = fi.fileName();
        if (containsAnyToken(fileName, excludeTokens) || containsAnyToken(fullKey, excludeTokens)) {
            continue;
        }

        if (!containsAnyToken(fullKey, keyHints) && !containsAnyToken(fileName, fileTokens)) {
            continue;
        }

        const QString canonical = fi.canonicalFilePath();
        if (seenPaths.contains(canonical)) {
            continue;
        }
        seenPaths.insert(canonical);
        totalBytes += fi.size();
    }

    return totalBytes;
}

} // namespace

InferenceEstimator::InferenceEstimator(QObject *parent)
    : QObject(parent)
{
    recalculate();
}

void InferenceEstimator::setEngineType(const QString &type)
{
    if (m_engineType != type) {
        m_engineType = type;
        emit parametersChanged();
        recalculate();
    }
}

void InferenceEstimator::setPromptText(const QString &text)
{
    if (m_promptText != text) {
        m_promptText = text;
        emit parametersChanged();
        recalculate();
    }
}

void InferenceEstimator::setSteps(int steps)
{
    int clamped = qMax(1, steps);
    if (m_steps != clamped) {
        m_steps = clamped;
        emit parametersChanged();
        recalculate();
    }
}

void InferenceEstimator::setWidth(int width)
{
    if (m_width != width) {
        m_width = width;
        emit parametersChanged();
        recalculate();
    }
}

void InferenceEstimator::setHeight(int height)
{
    if (m_height != height) {
        m_height = height;
        emit parametersChanged();
        recalculate();
    }
}

void InferenceEstimator::setReferenceFilePaths(const QStringList &paths)
{
    m_referenceFilePaths = paths;
    emit parametersChanged();
    recalculate();
}

void InferenceEstimator::setReferenceDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) return;

    QStringList filters{QStringLiteral("*.png"), QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.webp")};
    QFileInfoList list = dir.entryInfoList(filters, QDir::Files);
    QStringList paths;
    paths.reserve(list.size());
    for (const auto &info : list) {
        paths.append(info.absoluteFilePath());
    }
    setReferenceFilePaths(paths);
}

qint64 InferenceEstimator::getFileSize(const QString &filePath) const
{
    QFileInfo fi(filePath);
    return fi.exists() ? fi.size() : 0;
}

qint64 InferenceEstimator::getDirFileSize(const QString &dirPath, const QStringList &filters) const
{
    QDir dir(dirPath);
    if (!dir.exists()) return 0;
    qint64 total = 0;
    const QFileInfoList list = dir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo &fi : list) {
        total += fi.size();
    }
    return total;
}

void InferenceEstimator::recalculate()
{
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString iniPath = appData + QStringLiteral("/workspace_config.ini");
    const QString normalizedType = m_engineType.trimmed().toLower();

    qint64 diskBytes = 0;
    double peakRam = 4.0;
    int totalSecs = 0;
    m_breakdownItems.clear();

    auto addRow = [this](const QString &label, const QString &timeStr) {
        QVariantMap row;
        row[QStringLiteral("label")] = label;
        row[QStringLiteral("time")] = timeStr;
        m_breakdownItems.append(row);
    };

    // ==========================================
    // 0. REMOTE / REST API (Cloud or External Server)
    // ==========================================
    if (normalizedType == QStringLiteral("remote") ||
        normalizedType.startsWith(QStringLiteral("gemini_")) ||
        normalizedType.startsWith(QStringLiteral("openai_"))) {
        diskBytes = 0;
        peakRam = 0.0;
        totalSecs = 5;

        addRow(QStringLiteral("• Remote REST API Request & Generation:"), QStringLiteral("~2–10s (network)"));
        addRow(QStringLiteral("• Local Hardware Footprint:"), QStringLiteral("0.00 GB RAM / Disk"));
    }

    // ==========================================
    // 1. SCRIPT (llama.cpp / Ollama)
    // ==========================================
    else if (normalizedType == QStringLiteral("script") ||
             normalizedType == QStringLiteral("llama_cpp") ||
             normalizedType == QStringLiteral("ollama_local")) {
        diskBytes = configuredWeightsSize(
            iniPath,
            {QStringLiteral("script"), QStringLiteral("llm"), QStringLiteral("llama")},
            {QStringLiteral(".gguf")},
            {QStringLiteral("flux"), QStringLiteral("t5"), QStringLiteral("umt5"), QStringLiteral("wan"), QStringLiteral("ltx"), QStringLiteral("audio")});
        if (diskBytes == 0) {
            const QString scriptDir = appData + QStringLiteral("/models/script");
            diskBytes = getDirFileSize(scriptDir, QStringList() << QStringLiteral("*.gguf"));
        }

        const int promptTokens = qMax(16, static_cast<int>(m_promptText.length() / 3.5));
        const int maxTokens = 512;

        peakRam = 5.8;
        const int promptEvalSecs = qMax(1, promptTokens / 45);
        const int evalSecs = static_cast<int>(maxTokens / 8.5);
        totalSecs = promptEvalSecs + evalSecs;

        addRow(QStringLiteral("• Prompt Tokenization & Batch Eval (%1 tokens):").arg(promptTokens), QStringLiteral("~%1s").arg(promptEvalSecs));
        addRow(QStringLiteral("• Autoregressive Generation (~%1 tokens budget):").arg(maxTokens), QStringLiteral("~%1s").arg(evalSecs));
    }

    // ==========================================
    // 2. SPEECH (Kokoro TTS via Sherpa-ONNX)
    // ==========================================
    else if (normalizedType == QStringLiteral("audio_kokoro") ||
             normalizedType == QStringLiteral("kokoro_tts")) {
        diskBytes = configuredWeightsSize(
            iniPath,
            {QStringLiteral("kokoro"), QStringLiteral("tts")},
            {QStringLiteral("kokoro"), QStringLiteral("voices.bin")},
            {});
        if (diskBytes == 0) {
            const QString kokoroDir = appData + QStringLiteral("/audio/speech/Kokoro");
            diskBytes = getDirFileSize(kokoroDir, QStringList() << QStringLiteral("*.onnx") << QStringLiteral("voices.bin"));
        }

        const int charCount = qMax(30, m_promptText.trimmed().length());
        const int estimatedWords = qMax(5, charCount / 5);

        peakRam = 1.1;
        totalSecs = qMax(1, static_cast<int>(estimatedWords * 0.12));

        addRow(QStringLiteral("• Text Phonemization (espeak-ng):"), QStringLiteral("< 1s"));
        addRow(QStringLiteral("• ONNX Model Inference (~%1 words):").arg(estimatedWords), QStringLiteral("~%1s").arg(totalSecs));
        addRow(QStringLiteral("• WAV Packaging (24 kHz PCM):"), QStringLiteral("< 1s"));
    }

    // ==========================================
    // 3. AUDIO (audio.cpp DiT / SFX Generation)
    // ==========================================
    else if (normalizedType == QStringLiteral("audio") ||
             normalizedType == QStringLiteral("audio_cpp")) {
        diskBytes = configuredWeightsSize(
            iniPath,
            {QStringLiteral("audio")},
            {QStringLiteral("stable-audio"), QStringLiteral("audio")},
            {QStringLiteral("kokoro"), QStringLiteral("voices")});
        if (diskBytes == 0) {
            const QString audioDir = appData + QStringLiteral("/audio");
            diskBytes = getDirFileSize(audioDir, QStringList() << QStringLiteral("*.gguf"));
        }

        peakRam = 4.2;
        const int sfxSteps = 25;
        totalSecs = static_cast<int>(sfxSteps * 1.8);

        addRow(QStringLiteral("• Text Conditioning Encoders:"), QStringLiteral("~3s"));
        addRow(QStringLiteral("• Diffusion Denoising (%1 steps):").arg(sfxSteps), QStringLiteral("~%1s").arg(qMax(1, totalSecs - 4)));
        addRow(QStringLiteral("• VAE Audio Latent Decode (44.1 kHz):"), QStringLiteral("~1s"));
    }

    // ==========================================
    // 4. VIDEO (Wan 2.1 / LTX-Video)
    // ==========================================
    else if (normalizedType == QStringLiteral("video") ||
             normalizedType == QStringLiteral("sd_video") ||
             normalizedType.contains(QStringLiteral("wan")) ||
             normalizedType.contains(QStringLiteral("ltx"))) {
        diskBytes = configuredWeightsSize(
            iniPath,
            {QStringLiteral("video"), QStringLiteral("wan"), QStringLiteral("ltx")},
            {QStringLiteral("wan"), QStringLiteral("ltx"), QStringLiteral("umt5")},
            {});
        if (diskBytes == 0) {
            const QString videoDir = appData + QStringLiteral("/video");
            diskBytes = getDirFileSize(videoDir + QStringLiteral("/wan"), QStringList() << QStringLiteral("*.gguf") << QStringLiteral("*.safetensors"));
            if (diskBytes == 0) {
                diskBytes = getDirFileSize(videoDir + QStringLiteral("/ltx"), QStringList() << QStringLiteral("*.gguf") << QStringLiteral("*.safetensors"));
            }
        }

        peakRam = 14.5;
        const int vidFrames = 25;
        const double frameFactor = vidFrames / 25.0;
        const int stepSecs = static_cast<int>(m_steps * 48 * frameFactor);
        const int vaeSecs = 180;
        totalSecs = 120 + stepSecs + vaeSecs;

        addRow(QStringLiteral("• Text Prompt Encoding (UMT5 / T5-XXL):"), QStringLiteral("~2m 00s"));
        addRow(QStringLiteral("• DiT Latent Sampling (%1 frames, %2 steps):").arg(vidFrames).arg(m_steps),
               QStringLiteral("~%1 mins").arg(stepSecs / 60));
        addRow(QStringLiteral("• 3D Video VAE Temporal Latent Decode:"), QStringLiteral("~%1s").arg(vaeSecs));
    }

    // ==========================================
    // 5. IMAGE (FLUX / SDXL Default)
    // ==========================================
    else {
        diskBytes = configuredWeightsSize(
            iniPath,
            {QStringLiteral("diffusion"), QStringLiteral("clip"), QStringLiteral("t5"), QStringLiteral("vae"), QStringLiteral("flux")},
            {QStringLiteral("flux"), QStringLiteral("clip_l"), QStringLiteral("t5-v1_1"), QStringLiteral("ae.safetensors"), QStringLiteral("sdxl"), QStringLiteral("sd3")},
            {QStringLiteral("wan"), QStringLiteral("ltx"), QStringLiteral("umt5"), QStringLiteral("video")});
        if (diskBytes == 0) {
            const QString fluxDir = appData + QStringLiteral("/flux/");
            diskBytes += getFileSize(fluxDir + QStringLiteral("flux1-schnell-Q2_K.gguf"));
            diskBytes += getFileSize(fluxDir + QStringLiteral("clip_l.safetensors"));
            diskBytes += getFileSize(fluxDir + QStringLiteral("t5-v1_1-xxl-encoder-Q5_K_M.gguf"));
            diskBytes += getFileSize(fluxDir + QStringLiteral("ae.safetensors"));
        }

        const double pixelScale = (m_width * m_height) / (512.0 * 512.0);
        peakRam = 7.45;
        if (!m_referenceFilePaths.isEmpty()) {
            peakRam += 1.25;
        }

        const int textSecs = 111;
        const int visionSecs = m_referenceFilePaths.size() * 45;
        const int samplingSecs = static_cast<int>(m_steps * (850 * pixelScale));
        const int vaeSecs = static_cast<int>(270 * pixelScale);
        totalSecs = textSecs + visionSecs + samplingSecs + vaeSecs;

        addRow(QStringLiteral("• Prompt Conditioning (T5 + CLIP-L):"), QStringLiteral("~1m 51s"));
        if (!m_referenceFilePaths.isEmpty()) {
            addRow(QStringLiteral("• Reference Images Pass (%1 files):").arg(m_referenceFilePaths.length()),
                   QStringLiteral("~%1m").arg(std::round((m_referenceFilePaths.length() * 45.0) / 60.0)));
        }
        addRow(QStringLiteral("• Flow Matching (%1 Euler steps):").arg(m_steps),
               QStringLiteral("~%1 mins").arg(std::round(m_steps * 14.1)));
        addRow(QStringLiteral("• VAE Latent Decode Buffer:"), QStringLiteral("~4m 30s"));
    }

    m_totalDiskSizeGb = diskBytes / (1024.0 * 1024.0 * 1024.0);
    m_estimatedPeakRamGb = peakRam;
    m_estimatedTotalSeconds = totalSecs;

    const int hours = m_estimatedTotalSeconds / 3600;
    const int minutes = (m_estimatedTotalSeconds % 3600) / 60;
    const int seconds = m_estimatedTotalSeconds % 60;

    if (hours > 0) {
        m_estimatedTimeFormatted = QStringLiteral("%1h %2m %3s").arg(hours).arg(minutes).arg(seconds);
    } else {
        m_estimatedTimeFormatted = QStringLiteral("%1m %2s").arg(minutes).arg(seconds);
    }

    emit estimatesUpdated();
}
