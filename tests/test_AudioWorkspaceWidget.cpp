/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_AudioWorkspaceWidget.cpp
** Unit test suite for AudioWorkspaceWidget QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>
#include <QFile>
#include <QDataStream>
#include <QtMath>
#include "audiodirectorymodel.h"
#include "audiowaveformmodel.h"
#include "ui_constants.h"

class Test_AudioWorkspaceWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testWidgetInitialization();
    void testPreviewAudioFileTrigger();
    void testTimeFormattingFunction();

private:
    QString m_testSessionDir;
    QString m_sampleAudioPath;

    void generateSyntheticWav(const QString &filePath, int sampleRate = 44100, double durationSec = 0.2);

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *widget = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/audio"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 600\n"
                "    height: 500\n"
                "    visible: true\n"
                "    AudioWorkspaceWidget {\n"
                "        id: audioWidget\n"
                "        anchors.fill: parent\n"
                "    }\n"
                "}\n"
                );

            engine.loadData(hostQml);
            if (engine.rootObjects().isEmpty()) {
                return false;
            }

            window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            if (!window) {
                return false;
            }

            widget = window->findChild<QObject*>(UI::AudioWorkspaceWidget::Root);
            return (widget != nullptr);
        }
    };
};

void Test_AudioWorkspaceWidget::generateSyntheticWav(const QString &filePath, int sampleRate, double durationSec)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return;

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);

    int totalSamples = static_cast<int>(sampleRate * durationSec);
    int dataChunkSize = totalSamples * sizeof(qint16);

    file.write("RIFF", 4);
    out << static_cast<quint32>(36 + dataChunkSize);
    file.write("WAVE", 4);

    file.write("fmt ", 4);
    out << static_cast<quint32>(16);
    out << static_cast<quint16>(1);
    out << static_cast<quint16>(1);
    out << static_cast<quint32>(sampleRate);
    out << static_cast<quint32>(sampleRate * sizeof(qint16));
    out << static_cast<quint16>(sizeof(qint16));
    out << static_cast<quint16>(16);

    file.write("data", 4);
    out << static_cast<quint32>(dataChunkSize);

    for (int i = 0; i < totalSamples; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double s = qSin(2.0 * M_PI * 440.0 * t) * 0.7;
        qint16 pcm = static_cast<qint16>(s * 32767.0);
        out << pcm;
    }

    file.close();
}

void Test_AudioWorkspaceWidget::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    // Explicitly register C++ types to guarantee availability in headless QQmlApplicationEngine
    qmlRegisterType<AudioDirectoryModel>("StoryBuilderStudio", 1, 0, "AudioDirectoryModel");
    qmlRegisterType<AudioWaveformModel>("StoryBuilderStudio", 1, 0, "AudioWaveformModel");

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::cleanPath(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    m_sampleAudioPath = QDir::cleanPath(m_testSessionDir + "/preview_sample.wav");
    generateSyntheticWav(m_sampleAudioPath);
}

void Test_AudioWorkspaceWidget::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_AudioWorkspaceWidget::testWidgetInitialization()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load AudioWorkspaceWidget harness");

    QObject *widget = harness.widget;
    QVERIFY(widget != nullptr);

    QObject *trackStatusText = widget->findChild<QObject*>(UI::AudioWorkspaceWidget::TrackStatusText);
    QVERIFY2(trackStatusText != nullptr, "UI Component Missing: 'trackStatusText' not found.");
    QObject *sharedMenu = widget->findChild<QObject*>(UI::AudioWorkspaceWidget::SharedContextMenu);
    QVERIFY2(sharedMenu != nullptr, "UI Component Missing: 'sharedContextMenu' not found.");

    QCOMPARE(trackStatusText->property("text").toString(), QStringLiteral("No track selected"));
}

void Test_AudioWorkspaceWidget::testPreviewAudioFileTrigger()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load AudioWorkspaceWidget harness");

    QObject *widget = harness.widget;
    QVERIFY(widget != nullptr);

    // Call previewAudioFile with local path
    bool invoked = QMetaObject::invokeMethod(widget, "previewAudioFile",
                                             Qt::DirectConnection,
                                             Q_ARG(QVariant, QVariant(m_sampleAudioPath)));
    QVERIFY2(invoked, "Failed to invoke previewAudioFile()");
    QCoreApplication::processEvents();

    QObject *player = widget->findChild<QObject*>(UI::AudioWorkspaceWidget::MediaPlayer);
    QVERIFY2(player != nullptr, "UI Component Missing: 'mediaPlayer' not found.");

    // Verify source was populated with the file
    QString sourceStr = player->property("source").toString();
    QVERIFY2(sourceStr.contains(QStringLiteral("preview_sample.wav")), "Audio Preview Failure: MediaPlayer source was not set correctly.");

    // Verify waveform model triggered
    QObject *waveModel = widget->findChild<QObject*>(UI::AudioWorkspaceWidget::AudioWaveformModel);
    QVERIFY2(waveModel != nullptr, "UI Component Missing: 'audioWaveformModel' not found.");
    QTRY_VERIFY_WITH_TIMEOUT(!waveModel->property("isLoading").toBool(), 4000);
}

void Test_AudioWorkspaceWidget::testTimeFormattingFunction()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load AudioWorkspaceWidget harness");

    QObject *widget = harness.widget;

    QVariant result;
    QMetaObject::invokeMethod(widget, "formatTime",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, result),
                              Q_ARG(QVariant, QVariant(65000))); // 65 seconds -> 01:05
    QCOMPARE(result.toString(), QStringLiteral("01:05"));

    QMetaObject::invokeMethod(widget, "formatTime",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, result),
                              Q_ARG(QVariant, QVariant(0)));
    QCOMPARE(result.toString(), QStringLiteral("00:00"));
}

QTEST_MAIN(Test_AudioWorkspaceWidget)
#include "test_AudioWorkspaceWidget.moc"
