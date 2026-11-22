/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_InferenceEstimator.cpp
** Unit test suite for InferenceEstimator under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include "app/engine/inferenceestimator.h"

class Test_InferenceEstimator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testDefaultValues();
    void testScriptModeEstimates();
    void testSpeechKokoroEstimates();
    void testAudioCppEstimates();
    void testVideoEstimates();
    void testImageModeStepScaling();
    void testRemoteModeEstimates();
};

void Test_InferenceEstimator::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
}

void Test_InferenceEstimator::testDefaultValues()
{
    InferenceEstimator estimator;
    QCOMPARE(estimator.engineType(), QStringLiteral("image"));
    QCOMPARE(estimator.steps(), 4);
    QCOMPARE(estimator.width(), 512);
    QCOMPARE(estimator.height(), 512);
    QVERIFY(estimator.estimatedPeakRamGb() > 7.0);
    QVERIFY(estimator.estimatedTotalSeconds() > 0);
    QVERIFY(!estimator.estimatedTimeFormatted().isEmpty());
    QVERIFY(!estimator.breakdownItems().isEmpty());
}

void Test_InferenceEstimator::testScriptModeEstimates()
{
    InferenceEstimator estimator;
    QSignalSpy spy(&estimator, &InferenceEstimator::estimatesUpdated);

    estimator.setEngineType(QStringLiteral("script"));
    estimator.setPromptText(QStringLiteral("Write a complete script for a sci-fi episode."));

    QVERIFY2(spy.count() >= 1, "Estimator Failed: Did not emit estimatesUpdated signal after parameter change.");
    QVERIFY2(estimator.estimatedTotalSeconds() > 50, "Estimator Failure: Script mode execution time estimate is abnormally low.");
    QCOMPARE(estimator.estimatedPeakRamGb(), 5.8);
}

void Test_InferenceEstimator::testSpeechKokoroEstimates()
{
    InferenceEstimator estimator;
    estimator.setEngineType(QStringLiteral("audio_kokoro"));
    estimator.setPromptText(QStringLiteral("Hello world this is a test speech generation prompt for verification."));

    QCOMPARE(estimator.estimatedPeakRamGb(), 1.1);
    QVERIFY(estimator.estimatedTotalSeconds() >= 1);
}

void Test_InferenceEstimator::testAudioCppEstimates()
{
    InferenceEstimator estimator;
    estimator.setEngineType(QStringLiteral("audio_cpp"));

    QCOMPARE(estimator.estimatedPeakRamGb(), 4.2);
    QVERIFY(estimator.estimatedTotalSeconds() >= 40);
}

void Test_InferenceEstimator::testVideoEstimates()
{
    InferenceEstimator estimator;
    estimator.setEngineType(QStringLiteral("video"));
    estimator.setSteps(20);

    QCOMPARE(estimator.estimatedPeakRamGb(), 14.5);
    QVERIFY(estimator.estimatedTotalSeconds() > 1000);
}

void Test_InferenceEstimator::testImageModeStepScaling()
{
    InferenceEstimator estimator;
    estimator.setEngineType(QStringLiteral("image"));

    estimator.setSteps(4);
    int baselineSeconds = estimator.estimatedTotalSeconds();

    estimator.setSteps(8);
    int scaledSeconds = estimator.estimatedTotalSeconds();

    QVERIFY(scaledSeconds > baselineSeconds);
}

void Test_InferenceEstimator::testRemoteModeEstimates()
{
    InferenceEstimator estimator;
    estimator.setEngineType(QStringLiteral("remote"));

    QCOMPARE(estimator.estimatedPeakRamGb(), 0.0);
    QCOMPARE(estimator.totalDiskSizeGb(), 0.0);
    QVERIFY(estimator.estimatedTotalSeconds() > 0);
    QVERIFY(!estimator.breakdownItems().isEmpty());
}

QTEST_MAIN(Test_InferenceEstimator)
#include "test_InferenceEstimator.moc"

