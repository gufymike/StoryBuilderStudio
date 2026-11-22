/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_InferenceEstimateDialog.cpp
** Unit test suite for InferenceEstimateDialog QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"

class Test_InferenceEstimateDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDialogHeaderAndDefaults();
    void testMetricsDisplayNormalRam();
    void testMetricsDisplayHighRamWarning();
    void testBreakdownItemsRepeater();
    void testRemoteEngineSubtitle();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *dialog = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/engine"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 600\n"
                "    height: 500\n"
                "    visible: true\n"
                "    InferenceEstimateDialog {\n"
                "        id: estimateDialog\n"
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

            dialog = window->findChild<QObject*>(UI::InferenceEstimateDialog::Root);
            return (dialog != nullptr);
        }
    };
};

void Test_InferenceEstimateDialog::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_InferenceEstimateDialog::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_InferenceEstimateDialog::testDialogHeaderAndDefaults()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *dialog = harness.dialog;
    QVERIFY(dialog != nullptr);

    QObject *header = dialog->findChild<QObject*>(UI::InferenceEstimateDialog::HeaderTitle);
    QVERIFY2(header != nullptr, "UI Component Missing: 'headerTitleText' not found.");
    QObject *subtitle = dialog->findChild<QObject*>(UI::InferenceEstimateDialog::HeaderSubtitle);
    QVERIFY2(subtitle != nullptr, "UI Component Missing: 'headerSubtitleText' not found.");

    QVERIFY(header->property("text").toString().contains(QStringLiteral("Resource & Latency Estimate")));
    QCOMPARE(subtitle->property("text").toString(), QStringLiteral("Offline CPU Execution Profile (AVX2 Active)"));
}

void Test_InferenceEstimateDialog::testMetricsDisplayNormalRam()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *dialog = harness.dialog;

    dialog->setProperty("mockEstimatedTime", QStringLiteral("1m 45s"));
    dialog->setProperty("mockPeakRamGb", 8.50);
    dialog->setProperty("mockDiskSizeGb", 14.20);
    QCoreApplication::processEvents();

    QObject *runtimeText = dialog->findChild<QObject*>(UI::InferenceEstimateDialog::TotalRuntimeText);
    QVERIFY(runtimeText != nullptr);
    QCOMPARE(runtimeText->property("text").toString(), QStringLiteral("1m 45s"));

    QObject *ramText = dialog->findChild<QObject*>(UI::InferenceEstimateDialog::PeakRamText);
    QVERIFY(ramText != nullptr);
    QCOMPARE(ramText->property("text").toString(), QStringLiteral("8.50 GB"));
    QCOMPARE(ramText->property("color").toString().toUpper(), QStringLiteral("#FFFFFF"));

    QObject *diskText = dialog->findChild<QObject*>(UI::InferenceEstimateDialog::TotalDiskText);
    QVERIFY(diskText != nullptr);
    QCOMPARE(diskText->property("text").toString(), QStringLiteral("14.20 GB"));
}

void Test_InferenceEstimateDialog::testMetricsDisplayHighRamWarning()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

        QObject *dialog = harness.dialog;

        // Above 12 GB threshold
        dialog->setProperty("mockPeakRamGb", 16.75);
        QCoreApplication::processEvents();
        QObject *runtimeText = dialog->findChild<QObject*>(UI::InferenceEstimateDialog::TotalRuntimeText);
        QVERIFY2(runtimeText != nullptr, "UI Component Missing: 'totalRuntimeText' not found.");

        QObject *ramText = dialog->findChild<QObject*>(UI::InferenceEstimateDialog::PeakRamText);
        QVERIFY2(ramText != nullptr, "UI Component Missing: 'peakRamText' not found.");

        QTRY_COMPARE_WITH_TIMEOUT(ramText->property("text").toString(), QStringLiteral("16.75 GB"), 1000);
        QCOMPARE(ramText->property("color").toString().toUpper(), QStringLiteral("#FF6666"));

        QObject *ramCard = dialog->findChild<QObject*>(UI::InferenceEstimateDialog::RamCardRect);
        QVERIFY2(ramCard != nullptr, "UI Component Missing: 'ramCardRect' not found.");
        // Read the border color properly from the QQuickPen sub-object or helper property
        QColor borderColor;
    if (ramCard->property("currentBorderColor").isValid()) {
        borderColor = ramCard->property("currentBorderColor").value<QColor>();
    } else {
        QObject *pen = ramCard->property("border").value<QObject*>();
        if (pen) {
            borderColor = pen->property("color").value<QColor>();
        }
    }

    QCOMPARE(borderColor.name().toUpper(), QStringLiteral("#FF4444"));
}

void Test_InferenceEstimateDialog::testBreakdownItemsRepeater()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *dialog = harness.dialog;

    QVariantList stages;
    QVariantMap stage1;
    stage1[QStringLiteral("label")] = QStringLiteral("Prompt Tokenization & Clip Encoding");
    stage1[QStringLiteral("time")] = QStringLiteral("4.2s");
    stages.append(stage1);

    QVariantMap stage2;
    stage2[QStringLiteral("label")] = QStringLiteral("DiT Diffusion Latent Steps (4 steps)");
    stage2[QStringLiteral("time")] = QStringLiteral("38.5s");
    stages.append(stage2);

    dialog->setProperty("mockBreakdownItems", stages);
    QCoreApplication::processEvents();

    QObject *emptyLabel = dialog->findChild<QObject*>(UI::InferenceEstimateDialog::EmptyBreakdownLbl);
    QVERIFY2(emptyLabel != nullptr, "UI Component Missing: 'emptyBreakdownLabel' not found.");
    QCOMPARE(emptyLabel->property("visible").toBool(), false);

    QObject *repeater = dialog->findChild<QObject*>(UI::InferenceEstimateDialog::BreakdownRepeater);
    QVERIFY2(repeater != nullptr, "UI Component Missing: 'breakdownRepeater' not found.");
    QCOMPARE(repeater->property("count").toInt(), 2);
}

void Test_InferenceEstimateDialog::testRemoteEngineSubtitle()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *dialog = harness.dialog;
    dialog->setProperty("engineType", QStringLiteral("remote"));
    QCoreApplication::processEvents();

    QObject *subtitle = dialog->findChild<QObject*>(UI::InferenceEstimateDialog::HeaderSubtitle);
    QVERIFY2(subtitle != nullptr, "UI Component Missing: 'headerSubtitleText' not found.");
    QCOMPARE(subtitle->property("text").toString(),
             QStringLiteral("Remote REST API Execution Profile (Cloud / Server)"));
}

QTEST_MAIN(Test_InferenceEstimateDialog)
#include "test_InferenceEstimateDialog.moc"
