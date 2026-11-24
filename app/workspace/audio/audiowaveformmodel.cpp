// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include "audiowaveformmodel.h"
#include <QUrl>
#include <QtMath>
#include <QDebug>
#include <QDir>

AudioWaveformModel::AudioWaveformModel(QObject *parent)
    : QObject(parent)
    , m_decoder(new QAudioDecoder(this))
{
    connect(m_decoder, &QAudioDecoder::bufferReady, this, &AudioWaveformModel::handleBufferReady);
    connect(m_decoder, &QAudioDecoder::finished, this, &AudioWaveformModel::handleFinished);
    connect(m_decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error), this, &AudioWaveformModel::handleError);
}

AudioWaveformModel::~AudioWaveformModel() = default;

void AudioWaveformModel::clear()
{
    m_decoder->stop();
    m_rawPcmSamples.clear();
    m_wavePoints.clear();
    m_isLoading = false;
    emit wavePointsChanged();
    emit isLoadingChanged();
}

void AudioWaveformModel::loadAudioFile(const QString &filePathOrUrl, int sampleBucketCount)
{
    m_decoder->stop();
    m_rawPcmSamples.clear();
    m_targetBucketCount = sampleBucketCount > 0 ? sampleBucketCount : 140;
    if (filePathOrUrl.trimmed().isEmpty()) {
        clear();
        return;
    }

    QUrl mediaUrl;
    if (filePathOrUrl.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive) ||
        filePathOrUrl.startsWith(QStringLiteral("qrc://"), Qt::CaseInsensitive) ||
        filePathOrUrl.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive) ||
        filePathOrUrl.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)) {
        mediaUrl = QUrl(filePathOrUrl);
    } else {
        mediaUrl = QUrl::fromLocalFile(QDir::cleanPath(filePathOrUrl));
    }

    if (!mediaUrl.isValid() || mediaUrl.scheme().isEmpty()) {
        mediaUrl = QUrl::fromLocalFile(filePathOrUrl);
    }

    if (mediaUrl.isEmpty() || !mediaUrl.isValid()) {
        clear();
        return;
    }

    m_isLoading = true;
    emit isLoadingChanged();

    m_decoder->setSource(mediaUrl);
    m_decoder->start();
}

void AudioWaveformModel::handleBufferReady()
{
    QAudioBuffer buffer = m_decoder->read();
    if (!buffer.isValid()) return;

    int frameCount = buffer.frameCount();
    int channelCount = buffer.format().channelCount();
    if (frameCount <= 0 || channelCount <= 0) return;

    if (buffer.format().sampleFormat() == QAudioFormat::Int16) {
        const qint16 *data = buffer.constData<qint16>();
        for (int i = 0; i < frameCount; ++i) {
            float sample = static_cast<float>(data[i * channelCount]) / 32768.0f;
            m_rawPcmSamples.append(sample);
        }
    } else if (buffer.format().sampleFormat() == QAudioFormat::Float) {
        const float *data = buffer.constData<float>();
        for (int i = 0; i < frameCount; ++i) {
            m_rawPcmSamples.append(data[i * channelCount]);
        }
    } else if (buffer.format().sampleFormat() == QAudioFormat::Int32) {
        const qint32 *data = buffer.constData<qint32>();
        for (int i = 0; i < frameCount; ++i) {
            float sample = static_cast<float>(data[i * channelCount]) / 2147483648.0f;
            m_rawPcmSamples.append(sample);
        }
    }
}

void AudioWaveformModel::handleFinished()
{
    processExtractedSamples();
    m_isLoading = false;
    emit isLoadingChanged();
}

void AudioWaveformModel::handleError(QAudioDecoder::Error error)
{
    qWarning() << "[AudioWaveformModel] Decoder Error (" << error << "):" << m_decoder->errorString();
    m_wavePoints.clear();
    m_isLoading = false;
    emit wavePointsChanged();
    emit isLoadingChanged();
}

void AudioWaveformModel::processExtractedSamples()
{
    int totalSamples = m_rawPcmSamples.size();
    if (totalSamples == 0) {
        m_wavePoints.clear();
        emit wavePointsChanged();
        return;
    }

    QList<qreal> points;
    int samplesPerBucket = qMax(1, totalSamples / m_targetBucketCount);

    for (int bucket = 0; bucket < m_targetBucketCount; ++bucket) {
        int startSample = bucket * samplesPerBucket;
        int endSample = qMin(startSample + samplesPerBucket, totalSamples);

        float peak = 0.0f;
        for (int s = startSample; s < endSample; ++s) {
            float absVal = qAbs(m_rawPcmSamples[s]);
            if (absVal > peak) {
                peak = absVal;
            }
        }

        qreal normalized = static_cast<qreal>(qBound(0.05f, peak, 1.0f));
        points.append(normalized);
    }

    m_wavePoints = points;
    m_rawPcmSamples.clear();
    emit wavePointsChanged();
}
