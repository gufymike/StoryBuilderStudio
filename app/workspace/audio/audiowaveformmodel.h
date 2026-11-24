// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QUrl>
#include <QtMultimedia/QAudioDecoder>
#include <QtMultimedia/QAudioBuffer>
#include <QtQml/QQmlEngine>

/**
 * @class AudioWaveformModel
 * @brief Decodes audio files and generates normalized PCM amplitude arrays.
 *
 * Exposes an asynchronous API to QML that loads an audio file via QAudioDecoder,
 * downsamples the PCM data into a designated number of buckets, and normalizes
 * the amplitudes (0.05 to 1.0) for visual rendering on a QML Canvas element.
 */
class AudioWaveformModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QList<qreal> wavePoints READ wavePoints NOTIFY wavePointsChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    explicit AudioWaveformModel(QObject *parent = nullptr);
    ~AudioWaveformModel() override;

    QList<qreal> wavePoints() const { return m_wavePoints; }
    bool isLoading() const { return m_isLoading; }

    Q_INVOKABLE void loadAudioFile(const QString &filePathOrUrl, int sampleBucketCount = 140);
    Q_INVOKABLE void clear();

signals:
    void wavePointsChanged();
    void isLoadingChanged();

private slots:
    void handleBufferReady();
    void handleFinished();
    void handleError(QAudioDecoder::Error error);

private:
    QAudioDecoder *m_decoder = nullptr;
    QList<qreal> m_wavePoints;
    QList<float> m_rawPcmSamples;
    int m_targetBucketCount = 140;
    bool m_isLoading = false;

    void processExtractedSamples();
};
