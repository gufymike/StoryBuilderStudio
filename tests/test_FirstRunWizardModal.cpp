/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_FirstRunWizardModal.cpp
** Unit test suite for FirstRunWizardModal QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlContext>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"
#include "settingsmanager.h"
#include "engineregistry.h"

class Test_FirstRunWizardModal : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testStep0InitializationAndDefaults();
    void testScanResultsDynamicSummary();
    void testStepperNavigationTransitions();
    void testStep2SummaryLabelsAndButtonText();
    void testWizardPersistsSettingsOnNextAndFinish();
    void testRemoteModeToggleAndSummary();
    void testDefaultSavePathPersistence();
    void testDefaultsToRemoteWhenNoLocalPlugins();
private:
    QString m_testSessionDir;

    // Helper to create the dialog inside a live ApplicationWindow test harness
    struct TestHarness {
        QQmlApplicationEngine engine;
        std::unique_ptr<SettingsManager> settings;
        QString configPath;
        QQuickWindow *window = nullptr;
        QObject *dialog = nullptr;

        bool init(const QString &sessionDir, EngineRegistry *registry = nullptr) {
            configPath = QDir::toNativeSeparators(
                sessionDir + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".ini"
                );
            settings = std::make_unique<SettingsManager>(configPath);
            engine.rootContext()->setContextProperty(QStringLiteral("settingsManager"), settings.get());

            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 800\n"
                "    height: 600\n"
                "    visible: true\n"
                "    FirstRunWizardModal {\n"
                "        id: wizardModal\n"
                "    }\n"
                "}\n"
                );

            if (registry) {
                engine.rootContext()->setContextProperty(QStringLiteral("engineRegistry"), registry);
            }

            engine.loadData(hostQml);
            if (engine.rootObjects().isEmpty()) {
                return false;
            }

            window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            if (!window) {
                return false;
            }

            dialog = window->findChild<QObject*>(UI::FirstRunWizardModal::Root);
            return (dialog != nullptr);
        }
    };
};

void Test_FirstRunWizardModal::initTestCase()
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

void Test_FirstRunWizardModal::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_FirstRunWizardModal::testStep0InitializationAndDefaults()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *dialog = harness.dialog;
    QCOMPARE(dialog->property("currentStep").toInt(), 0);

    QObject *header = dialog->findChild<QObject*>(UI::FirstRunWizardModal::Step0Header);
    QVERIFY2(header != nullptr, "UI Component Missing: 'step0Header' not found.");
    QCOMPARE(header->property("text").toString(), QStringLiteral("AI Model Repositories"));
    QCOMPARE(header->property("color").toString().toUpper(), QStringLiteral("#00F0FF"));

    QObject *backBtn = dialog->findChild<QObject*>(UI::FirstRunWizardModal::BackStepBtn);
    QVERIFY2(backBtn != nullptr, "UI Component Missing: 'backStepBtn' not found.");
    QVERIFY(backBtn != nullptr);
    QCOMPARE(backBtn->property("visible").toBool(), false);

    QObject *nextBtn = dialog->findChild<QObject*>(UI::FirstRunWizardModal::NextStepBtn);
    QVERIFY2(nextBtn != nullptr, "UI Component Missing: 'nextStepBtn' not found.");
    QCOMPARE(nextBtn->property("text").toString(), QStringLiteral("Next →"));
    QVERIFY(nextBtn->property("hovered").isValid());
}

void Test_FirstRunWizardModal::testScanResultsDynamicSummary()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *dialog = harness.dialog;

    QVariantMap mockScan;
    mockScan[QStringLiteral("textCount")] = 3;
    mockScan[QStringLiteral("imageCount")] = 2;
    mockScan[QStringLiteral("audioCount")] = 1;
    mockScan[QStringLiteral("videoCount")] = 0;
    mockScan[QStringLiteral("hasT5")] = true;
    dialog->setProperty("scanResults", mockScan);

    QObject *scriptSummary = dialog->findChild<QObject*>(UI::FirstRunWizardModal::ScriptSummary);
    QObject *t5Status = dialog->findChild<QObject*>(UI::FirstRunWizardModal::T5StatusText);
    QVERIFY2(scriptSummary != nullptr, "UI Component Missing: 'scriptSummaryText' not found.");
    QVERIFY2(t5Status != nullptr, "UI Component Missing: 't5StatusText' not found.");

    QCOMPARE(scriptSummary->property("text").toString(), QStringLiteral("• Script (GGUF): 3"));
    QCOMPARE(scriptSummary->property("color").toString().toUpper(), QStringLiteral("#00F0FF"));
    QCOMPARE(t5Status->property("text").toString(), QStringLiteral("✔ T5XXL Encoder found"));
    QCOMPARE(t5Status->property("color").toString().toUpper(), QStringLiteral("#00F0FF"));
}

void Test_FirstRunWizardModal::testStepperNavigationTransitions()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *dialog = harness.dialog;

    // Open the dialog inside the active ApplicationWindow overlay
    QMetaObject::invokeMethod(dialog, "open");
    QTRY_VERIFY_WITH_TIMEOUT(dialog->property("visible").toBool(), 1000);

    QObject *backBtn = dialog->findChild<QObject*>(UI::FirstRunWizardModal::BackStepBtn);
    QVERIFY2(backBtn != nullptr, "UI Component Missing: 'backStepBtn' not found.");
    QCOMPARE(backBtn->property("visible").toBool(), false);

    // Transition to Step 1
    QMetaObject::invokeMethod(dialog, "goToStep", Q_ARG(QVariant, 1));
    QTRY_COMPARE_WITH_TIMEOUT(backBtn->property("visible").toBool(), true, 1000);

    QObject *step1Header = dialog->findChild<QObject*>(UI::FirstRunWizardModal::Step1Header);
    QVERIFY(step1Header != nullptr);
    QCOMPARE(step1Header->property("text").toString(), QStringLiteral("Series & Production Workspace"));

    QObject *nextBtn = dialog->findChild<QObject*>(UI::FirstRunWizardModal::NextStepBtn);
    QVERIFY2(nextBtn != nullptr, "UI Component Missing: 'nextStepBtn' not found.");
    QCOMPARE(nextBtn->property("text").toString(), QStringLiteral("Next →"));
}

void Test_FirstRunWizardModal::testStep2SummaryLabelsAndButtonText()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *dialog = harness.dialog;

    dialog->setProperty("currentStep", 2);
    dialog->setProperty("activeSeriesName", QStringLiteral("Macross Delta Production"));
    dialog->setProperty("activeSeriesRoot", m_testSessionDir);
    QObject *step2Header = dialog->findChild<QObject*>(UI::FirstRunWizardModal::Step2Header);
    QObject *seriesSummary = dialog->findChild<QObject*>(UI::FirstRunWizardModal::SummarySeriesText);
    QVERIFY2(step2Header != nullptr, "UI Component Missing: 'step2Header' not found.");
    QVERIFY2(seriesSummary != nullptr, "UI Component Missing: 'summarySeriesText' not found.");
    QObject *nextBtn = dialog->findChild<QObject*>(UI::FirstRunWizardModal::NextStepBtn);
    QVERIFY2(nextBtn != nullptr, "UI Component Missing: 'nextStepBtn' not found.");

    QCOMPARE(step2Header->property("text").toString(), QStringLiteral("Setup Complete"));
    QCOMPARE(nextBtn->property("text").toString(), QStringLiteral("Launch Studio"));
    QCOMPARE(seriesSummary->property("text").toString(), QStringLiteral("• Active Series: Macross Delta Production"));
}

void Test_FirstRunWizardModal::testWizardPersistsSettingsOnNextAndFinish()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *dialog = harness.dialog;
    QVERIFY2(harness.settings->isFirstRun(), "Fresh SettingsManager should start with isFirstRun == true.");

    QMetaObject::invokeMethod(dialog, "open");
    QTRY_VERIFY_WITH_TIMEOUT(dialog->property("visible").toBool(), 1000);

    QObject *modelsDirField = dialog->findChild<QObject*>(UI::FirstRunWizardModal::ModelsDirField);
    QObject *t5Field        = dialog->findChild<QObject*>(UI::FirstRunWizardModal::T5Field);
    QObject *nextBtn        = dialog->findChild<QObject*>(UI::FirstRunWizardModal::NextStepBtn);
    QVERIFY2(modelsDirField != nullptr, "UI Component Missing: 'modelsDirField' not found.");
    QVERIFY2(t5Field != nullptr,        "UI Component Missing: 't5Field' not found.");
    QVERIFY2(nextBtn != nullptr,        "UI Component Missing: 'nextStepBtn' not found.");

    const QString testModelsDir = QDir::toNativeSeparators(m_testSessionDir + "/models");
    const QString testT5Path    = QDir::toNativeSeparators(testModelsDir + "/t5xxl.gguf");
    modelsDirField->setProperty("text", testModelsDir);
    t5Field->setProperty("text", testT5Path);

           // Step 0 -> Step 1: should persist modelsDir and globalT5Path
    QMetaObject::invokeMethod(nextBtn, "click");
    QTRY_COMPARE_WITH_TIMEOUT(dialog->property("currentStep").toInt(), 1, 1000);
    QCOMPARE(harness.settings->baseModelsDir(), testModelsDir);
    QCOMPARE(harness.settings->globalT5Path(), testT5Path);

           // Step 1 -> Step 2
    QMetaObject::invokeMethod(nextBtn, "click");
    QTRY_COMPARE_WITH_TIMEOUT(dialog->property("currentStep").toInt(), 2, 1000);

           // Step 2 -> Finish ("Launch Studio"): should set isFirstRun = false and close dialog
    QMetaObject::invokeMethod(nextBtn, "click");
    QTRY_COMPARE_WITH_TIMEOUT(dialog->property("visible").toBool(), false, 1000);
    QCOMPARE(harness.settings->isFirstRun(), false);

           // Reload from disk to confirm INI persistence
    SettingsManager reloaded(harness.configPath);
    QCOMPARE(reloaded.isFirstRun(), false);
    QCOMPARE(reloaded.baseModelsDir(), testModelsDir);
    QCOMPARE(reloaded.globalT5Path(), testT5Path);
}

void Test_FirstRunWizardModal::testRemoteModeToggleAndSummary()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *dialog = harness.dialog;
    QMetaObject::invokeMethod(dialog, "open");
    QTRY_VERIFY_WITH_TIMEOUT(dialog->property("visible").toBool(), 1000);

    QCOMPARE(dialog->property("useRemoteSetup").toBool(), false);

    QObject *remoteTabBtn = dialog->findChild<QObject*>(QStringLiteral("remoteModeTabBtn"));
    QObject *remoteCard   = dialog->findChild<QObject*>(QStringLiteral("wizardRemoteConfigCard"));
    QVERIFY2(remoteTabBtn != nullptr, "UI Component Missing: 'remoteModeTabBtn' not found.");
    QVERIFY2(remoteCard != nullptr,   "UI Component Missing: 'wizardRemoteConfigCard' not found.");

           // Switch Step 0 to Remote / REST API mode
    QMetaObject::invokeMethod(remoteTabBtn, "click");
    QTRY_COMPARE_WITH_TIMEOUT(dialog->property("useRemoteSetup").toBool(), true, 1000);
    QCOMPARE(remoteCard->property("driverId").toString(), QStringLiteral("gemini_bridge"));

           // Verify Step 2 summary reflects Remote / REST mode
    dialog->setProperty("currentStep", 2);
    QObject *summaryModels = dialog->findChild<QObject*>(QStringLiteral("summaryModelsPathText"));
    QVERIFY2(summaryModels != nullptr, "UI Component Missing: 'summaryModelsPathText' not found.");
    QCOMPARE(summaryModels->property("text").toString(), QStringLiteral("• AI Mode: Remote / REST (gemini_bridge)"));
}

void Test_FirstRunWizardModal::testDefaultSavePathPersistence()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load test host window");

    QObject *dialog = harness.dialog;
    QMetaObject::invokeMethod(dialog, "open");
    QTRY_VERIFY_WITH_TIMEOUT(dialog->property("visible").toBool(), 1000);

    QObject *savePathField = dialog->findChild<QObject*>(QStringLiteral("defaultSavePathField"));
    QObject *nextBtn       = dialog->findChild<QObject*>(UI::FirstRunWizardModal::NextStepBtn);
    QVERIFY2(savePathField != nullptr, "UI Component Missing: 'defaultSavePathField' not found.");
    QVERIFY2(nextBtn != nullptr,       "UI Component Missing: 'nextStepBtn' not found.");

           // Advance to Step 1 and set a custom projects directory
    QMetaObject::invokeMethod(dialog, "goToStep", Q_ARG(QVariant, 1));
    QTRY_COMPARE_WITH_TIMEOUT(dialog->property("currentStep").toInt(), 1, 1000);

    const QString customProjectsDir = QDir::toNativeSeparators(m_testSessionDir + "/MyProjects");
    savePathField->setProperty("text", customProjectsDir);

           // Step 1 -> Step 2: should persist defaultSavePath to SettingsManager
    QMetaObject::invokeMethod(nextBtn, "click");
    QTRY_COMPARE_WITH_TIMEOUT(dialog->property("currentStep").toInt(), 2, 1000);
    QCOMPARE(harness.settings->defaultSavePath(), customProjectsDir);

    QObject *summarySavePath = dialog->findChild<QObject*>(QStringLiteral("summarySavePathText"));
    QVERIFY2(summarySavePath != nullptr, "UI Component Missing: 'summarySavePathText' not found.");
    QCOMPARE(summarySavePath->property("text").toString(),
             QStringLiteral("• Projects Directory: %1").arg(customProjectsDir));

           // Reload from disk to confirm INI persistence
    SettingsManager reloaded(harness.configPath);
    QCOMPARE(reloaded.defaultSavePath(), customProjectsDir);
}

void Test_FirstRunWizardModal::testDefaultsToRemoteWhenNoLocalPlugins()
{
    EngineRegistry registry;
    if (registry.hasLocalPlugins()) {
        QSKIP("Local plugins are present in the test environment; skipping REST-only wizard assertion.");
    }

    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir, &registry), "Failed to load test host window");

    QObject *dialog = harness.dialog;
    QMetaObject::invokeMethod(dialog, "open");
    QTRY_VERIFY_WITH_TIMEOUT(dialog->property("visible").toBool(), 1000);

    QCOMPARE(dialog->property("hasLocalPlugins").toBool(), false);
    QCOMPARE(dialog->property("useRemoteSetup").toBool(), true);

    QObject *step0Header = dialog->findChild<QObject*>(UI::FirstRunWizardModal::Step0Header);
    QVERIFY2(step0Header != nullptr, "UI Component Missing: 'step0Header' not found.");
    QCOMPARE(step0Header->property("text").toString(), QStringLiteral("Remote / REST API Setup"));

    QObject *localTabBtn = dialog->findChild<QObject*>(QStringLiteral("localModeTabBtn"));
    QVERIFY2(localTabBtn != nullptr, "UI Component Missing: 'localModeTabBtn' not found.");
    QCOMPARE(localTabBtn->property("visible").toBool(), false);

    QObject *remoteCard = dialog->findChild<QObject*>(QStringLiteral("wizardRemoteConfigCard"));
    QVERIFY2(remoteCard != nullptr, "UI Component Missing: 'wizardRemoteConfigCard' not found.");
    QCOMPARE(remoteCard->property("visible").toBool(), true);
}


QTEST_MAIN(Test_FirstRunWizardModal)
#include "test_FirstRunWizardModal.moc"

