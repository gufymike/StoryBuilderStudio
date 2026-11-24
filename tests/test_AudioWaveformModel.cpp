// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QtMath>
#include "audiowaveformmodel.h"

class Test_AudioWaveformModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDefaultState();
    void testClearState();
    void testWaveformDecodingSyntheticWav();
    void testInvalidFileHandling();

private:
    QString m_testSessionDir;
    QString m_syntheticWavPath;

    void generateSyntheticWav(const QString &filePath, int sampleRate = 44100, double durationSec = 0.5);
};

void Test_AudioWaveformModel::generateSyntheticWav(const QString &filePath, int sampleRate, double durationSec)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return;

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);

    int totalSamples = static_cast<int>(sampleRate * durationSec);
    int dataChunkSize = totalSamples * sizeof(qint16);

    // RIFF header
    file.write("RIFF", 4);
    out << static_cast<quint32>(36 + dataChunkSize);
    file.write("WAVE", 4);

    // fmt subchunk
    file.write("fmt ", 4);
    out << static_cast<quint32>(16);      // Subchunk1Size (16 for PCM)
    out << static_cast<quint16>(1);       // AudioFormat (1 for PCM)
    out << static_cast<quint16>(1);       // NumChannels (1 mono)
    out << static_cast<quint32>(sampleRate);
    out << static_cast<quint32>(sampleRate * sizeof(qint16)); // ByteRate
    out << static_cast<quint16>(sizeof(qint16));              // BlockAlign
    out << static_cast<quint16>(16);                          // BitsPerSample

    // data subchunk
    file.write("data", 4);
    out << static_cast<quint32>(dataChunkSize);

    // Generate 440 Hz Sine Wave
    const double freq = 440.0;
    for (int i = 0; i < totalSamples; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double s = qSin(2.0 * M_PI * freq * t) * 0.8; // Peak 0.8
        qint16 pcm = static_cast<qint16>(s * 32767.0);
        out << pcm;
    }

    file.close();
}

void Test_AudioWaveformModel::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    // Use clean forward slashes for Qt multimedia / FFmpeg URLs
    m_testSessionDir = QDir::cleanPath(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    m_syntheticWavPath = QDir::cleanPath(m_testSessionDir + "/sine440.wav");
    generateSyntheticWav(m_syntheticWavPath);

    QVERIFY2(QFile::exists(m_syntheticWavPath), qPrintable("File was not created: " + m_syntheticWavPath));
}

void Test_AudioWaveformModel::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_AudioWaveformModel::testDefaultState()
{
    AudioWaveformModel model;
    QVERIFY2(!model.isLoading(), "State Corruption: Model reported as loading while idle.");
    QVERIFY2(model.wavePoints().isEmpty(), "State Corruption: Model initialized with populated wavePoints.");
}

void Test_AudioWaveformModel::testClearState()
{
    AudioWaveformModel model;
    QSignalSpy waveSpy(&model, &AudioWaveformModel::wavePointsChanged);
    QSignalSpy loadSpy(&model, &AudioWaveformModel::isLoadingChanged);

    model.clear();
    QVERIFY2(!model.isLoading(), "State Corruption: Model reported as loading while idle.");
    QVERIFY2(model.wavePoints().isEmpty(), "State Corruption: Model initialized with populated wavePoints.");
    QCOMPARE(waveSpy.count(), 1);
    QCOMPARE(loadSpy.count(), 1);
}

void Test_AudioWaveformModel::testWaveformDecodingSyntheticWav()
{
    QVERIFY(QFile::exists(m_syntheticWavPath));

    AudioWaveformModel model;
    QSignalSpy waveSpy(&model, &AudioWaveformModel::wavePointsChanged);
    QSignalSpy loadSpy(&model, &AudioWaveformModel::isLoadingChanged);

    const int requestedBuckets = 80;
    model.loadAudioFile(m_syntheticWavPath, requestedBuckets);

    // Wait for decoding to complete and verify wavePoints were generated
    QTRY_VERIFY_WITH_TIMEOUT(!model.isLoading(), 5000);
    QCOMPARE(model.isLoading(), false);
       QVERIFY2(waveSpy.count() >= 1, "Signal Failure: wavePointsChanged was never emitted after decoding.");

    // Verify generated wavePoints
    QList<qreal> points = model.wavePoints();
    QCOMPARE(points.size(), requestedBuckets);

    // Verify values fall within normalized floor/ceiling [0.05, 1.0]
    for (qreal val : points) {
        QVERIFY2(val >= 0.05, "Math/Normalization Failure: Waveform point dipped below minimum visual threshold (0.05).");
        QVERIFY2(val <= 1.0, "Math/Normalization Failure: Waveform point exceeded maximum amplitude (1.0).");
    }
}

void Test_AudioWaveformModel::testInvalidFileHandling()
{
    AudioWaveformModel model;
    QSignalSpy waveSpy(&model, &AudioWaveformModel::wavePointsChanged);

    model.loadAudioFile(QStringLiteral("C:/non_existent_audio_path_xyz.wav"));

    // Decoder emits errorOccurred or immediate clear
    QTRY_VERIFY_WITH_TIMEOUT(!model.isLoading(), 3000);
    QCOMPARE(model.isLoading(), false);
    QVERIFY2(model.wavePoints().isEmpty(), "Error Handling Failure: Model populated wavePoints despite an invalid file path.");
}

QTEST_MAIN(Test_AudioWaveformModel)
#include "test_AudioWaveformModel.moc"
