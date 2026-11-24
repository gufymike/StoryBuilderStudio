// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>
#include <QObject>
#include <QVariantMap>

#include "ui_constants.h"

class MockDriver : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString driverId READ driverId CONSTANT)
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded CONSTANT)

public:
    explicit MockDriver(const QString &id = QStringLiteral("mock_engine"), QObject *parent = nullptr)
        : QObject(parent), m_driverId(id) {}

    QString driverId() const { return m_driverId; }
    bool isBusy() const { return m_isBusy; }
    bool isModelLoaded() const { return true; }

    Q_INVOKABLE void generate(const QString &prompt, const QString &style,
                              const QVariantList &files, const QString &dir,
                              const QVariantMap &options) {
        Q_UNUSED(prompt); Q_UNUSED(style); Q_UNUSED(files);
        Q_UNUSED(dir); Q_UNUSED(options);
        m_isBusy = true;
        emit isBusyChanged();
    }

    void simulateFinished(const QVariantMap &payload) {
        m_isBusy = false;
        emit isBusyChanged();
        emit generationFinished(payload);
    }

    void simulateToken(const QString &token) {
        emit tokenGenerated(token);
    }

signals:
    void isBusyChanged();
    void tokenGenerated(const QString &token);
    void generationFinished(const QVariantMap &payload);

private:
    QString m_driverId;
    bool m_isBusy = false;
};

class Test_PromptBuilder : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testWorkspaceInitializationAndDefaults();
    void testStackLayoutViewSwitching();
    void testPromptAndDirectivesCombination();
    void testAttachedFilesManagement();
    void testGenerationSignalBubbling();
    void testDriverAndModelSelectorIntegration();
    void testSplitterLayoutBoundaries();
    void testPromptHistoryButtonExists();
    void testPromptHistoryPopupOpens();
    void testRemoteDriverResolvesEstimatorToRemote();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *builder = nullptr;
        MockDriver mockDriver;

        bool init(const QString &sessionDir) {
            Q_UNUSED(sessionDir);
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/engine"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/promptbuilder"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 800\n"
                "    height: 600\n"
                "    visible: true\n"
                "    PromptBuilder {\n"
                "        id: builderItem\n"
                "        objectName: \"promptBuilderRoot\"\n"
                "        anchors.fill: parent\n"
                "    }\n"
                "}\n"
                );

            engine.loadData(hostQml);
            if (engine.rootObjects().isEmpty()) return false;

            window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            if (!window) return false;

            builder = window->findChild<QObject*>(UI::PromptBuilder::Root);
            if (!builder) return false;

            // Wire mock driver to PromptBuilder root
            QVariantMap mockRegistry;
            mockRegistry[QStringLiteral("activeDriver")] = QVariant::fromValue<QObject*>(&mockDriver);
            builder->setProperty("mockEngineRegistry", mockRegistry);

            return true;
        }
    };
};

void Test_PromptBuilder::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
    QVERIFY(QDir(m_testSessionDir).exists());
}

void Test_PromptBuilder::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_PromptBuilder::testWorkspaceInitializationAndDefaults()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load PromptBuilder harness");

    QObject *builder = harness.builder;
    QVERIFY2(builder != nullptr, "UI Component Missing: 'promptBuilderRoot' not found.");

    QObject *workspaceStack = builder->findChild<QObject*>(UI::PromptBuilder::WorkspaceStack);
    QVERIFY2(workspaceStack != nullptr, "UI Component Missing: 'workspaceStack' not found.");
    QCOMPARE(workspaceStack->property("currentIndex").toInt(), 0);

    QObject *promptTextArea = builder->findChild<QObject*>(UI::PromptBuilder::PromptTextArea);
    QVERIFY2(promptTextArea != nullptr, "UI Component Missing: 'promptTextArea' not found.");
    QCOMPARE(promptTextArea->property("text").toString(), QStringLiteral(""));

    // compare default properties/
    QCOMPARE(builder->property("targetAssetType").toString(), "script");
    QCOMPARE(builder->property("promptText").toString(), "");
    QCOMPARE(builder->property("isGenerating").toBool(), false);
    QCOMPARE(builder->property("lastDispatchedPrompt").toString(), "");
    QCOMPARE(builder->property("lastSentTimestamp").toString(), "");
    QCOMPARE(builder->property("lastRawPrompt").toString(), "");
    QCOMPARE(builder->property("selectedOptions").toList(), {});
    QCOMPARE(builder->property("attachedFileList").toList(), {});
    // MockEngine() will be loaded here for activeDriver.
    QVERIFY(builder->property("activeDriver").value<QObject*>() != nullptr);
    QCOMPARE(builder->property("engineDisplayName").toString(), "mock_engine");
    QCOMPARE(builder->property("avalableModelsForActiveDriver").toList(), {});
    QCOMPARE(builder->property("resolvedEstimatorEngine").toString(), "script");  // should match targetAssetType with no activeDriver

}

void Test_PromptBuilder::testStackLayoutViewSwitching()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load PromptBuilder harness");

    QObject *builder = harness.builder;
    QObject *workspaceStack = builder->findChild<QObject*>(UI::PromptBuilder::WorkspaceStack);
    QObject *settingsTabBtn = builder->findChild<QObject*>(UI::PromptBuilder::SettingsTabBtn);
    QObject *promptTabBtn = builder->findChild<QObject*>(UI::PromptBuilder::PromptTabBtn);
    QVERIFY2(workspaceStack && settingsTabBtn && promptTabBtn,
             "UI Components Missing: Stack layout or workspace tab buttons not found.");
    // Initial state: Prompt Page (0)
    QCOMPARE(workspaceStack->property("currentIndex").toInt(), 0);

    // Switch to Settings Page (1)
    QMetaObject::invokeMethod(settingsTabBtn, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(workspaceStack->property("currentIndex").toInt(), 1);

    // Switch back to Prompt Page (0)
    QMetaObject::invokeMethod(promptTabBtn, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(workspaceStack->property("currentIndex").toInt(), 0);
}

void Test_PromptBuilder::testPromptAndDirectivesCombination()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load PromptBuilder harness");

    QObject *builder = harness.builder;
    QObject *promptTextArea = builder->findChild<QObject*>(UI::PromptBuilder::PromptTextArea);
    QVERIFY2(promptTextArea != nullptr, "UI Component Missing: 'promptTextArea' not found.");

    promptTextArea->setProperty("text", QStringLiteral("Cinematic shot of pilot in cockpit"));
    QCoreApplication::processEvents();

    // Pass QVariant arguments to match toggleOption(QVariant, QVariant)
    QMetaObject::invokeMethod(builder, "toggleOption",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(QStringLiteral("8k resolution"))),
                              Q_ARG(QVariant, QVariant(true)));
    QMetaObject::invokeMethod(builder, "toggleOption",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(QStringLiteral("neon lighting"))),
                              Q_ARG(QVariant, QVariant(true)));
    QCoreApplication::processEvents();

    // Receive return as QVariant first, then convert to QString
    QVariant combinedVar;
    bool invoked = QMetaObject::invokeMethod(builder, "getCombinedPromptText",
                                             Qt::DirectConnection,
                                             Q_RETURN_ARG(QVariant, combinedVar));
    QVERIFY2(invoked, "Failed to invoke getCombinedPromptText()");

    QString combined = combinedVar.toString();
    QCOMPARE(combined, QStringLiteral("Cinematic shot of pilot in cockpit, 8k resolution, neon lighting"));
}

void Test_PromptBuilder::testAttachedFilesManagement()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load PromptBuilder harness");

    QObject *builder = harness.builder;

    QList<QUrl> urls = {
        QUrl::fromLocalFile(QStringLiteral("C:/Workspace/character_sheet.png")),
        QUrl::fromLocalFile(QStringLiteral("C:/Workspace/lore.txt"))
    };

    QVariantList varList;
    for (const auto &url : urls) {
        varList.append(url.toString());
    }

    QMetaObject::invokeMethod(builder, "addAttachedFiles",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant::fromValue(varList)));
    QCoreApplication::processEvents();

    QVariantList attached = builder->property("attachedFileList").toList();
    QCOMPARE(attached.size(), 2);

    // Receive first image as QVariant first, then convert to QString
    QVariant firstImageVar;
    bool invoked = QMetaObject::invokeMethod(builder, "getFirstAttachedImage",
                                             Qt::DirectConnection,
                                             Q_RETURN_ARG(QVariant, firstImageVar));
    QVERIFY2(invoked, "Failed to invoke getFirstAttachedImage()");

    QString firstImage = firstImageVar.toString();
    QVERIFY2(firstImage.endsWith(QStringLiteral("character_sheet.png")),
             "File Attachment Failure: The getFirstAttachedImage() method returned the wrong file or dropped the extension.");

    // Remove first attachment
    QMetaObject::invokeMethod(builder, "removeAttachedFile",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(0)));
    QCoreApplication::processEvents();

    attached = builder->property("attachedFileList").toList();
    QCOMPARE(attached.size(), 1);
    QVERIFY2(attached.first().toString().endsWith(QStringLiteral("lore.txt")),
             "File Attachment Failure: Expected lore.txt to remain after removing the first file.");
}

void Test_PromptBuilder::testGenerationSignalBubbling()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load PromptBuilder harness");

    QObject *builder = harness.builder;
    QVERIFY2(builder != nullptr, "UI Component Missing: 'promptBuilderRoot' not found.");

    // Set initial generating state
    builder->setProperty("isGenerating", true);
    QCOMPARE(builder->property("isGenerating").toBool(), true);

    // 1. Test imageCandidatesReady signal spy
    QSignalSpy imageSpy(builder, SIGNAL(imageCandidatesReady(QVariant)));
    QVERIFY2(imageSpy.isValid(), "Signal Spy Failure: imageCandidatesReady(QVariant) signal not found on PromptBuilder.");

    QVariantList candidateFiles = {
        QStringLiteral("C:/test/out_01.png"),
        QStringLiteral("C:/test/out_02.png")
    };
    QVariantMap imagePayload;
    imagePayload[QStringLiteral("candidates")] = candidateFiles;
    imagePayload[QStringLiteral("category")] = QStringLiteral("image");

    harness.mockDriver.simulateFinished(imagePayload);
    QCoreApplication::processEvents();

    QCOMPARE(imageSpy.count(), 1);
    QList<QVariant> imageArgs = imageSpy.takeFirst();
    QCOMPARE(imageArgs.at(0).toList().size(), 2);
    QCOMPARE(builder->property("isGenerating").toBool(), false);

    // 2. Test audioGenerated signal spy
    builder->setProperty("isGenerating", true);
    QSignalSpy audioSpy(builder, SIGNAL(audioGenerated(QString)));
    QVERIFY2(audioSpy.isValid(), "Signal Spy Failure: audioGenerated(QString) signal not found on PromptBuilder.");

    QVariantMap audioPayload;
    audioPayload[QStringLiteral("filePath")] = QStringLiteral("C:/test/voiceover.wav");
    audioPayload[QStringLiteral("category")] = QStringLiteral("audio");

    harness.mockDriver.simulateFinished(audioPayload);
    QCoreApplication::processEvents();

    QCOMPARE(audioSpy.count(), 1);
    QList<QVariant> audioArgs = audioSpy.takeFirst();
    QCOMPARE(audioArgs.at(0).toString(), QStringLiteral("C:/test/voiceover.wav"));
    QCOMPARE(builder->property("isGenerating").toBool(), false);

    // 3. Test scriptGenerated signal spy
    builder->setProperty("isGenerating", true);
    QSignalSpy scriptSpy(builder, SIGNAL(scriptGenerated(QString,QString)));
    QVERIFY2(scriptSpy.isValid(), "Signal Spy Failure: scriptGenerated(QString,QString) signal not found on PromptBuilder.");

    QVariantMap scriptPayload;
    scriptPayload[QStringLiteral("text")] = QStringLiteral("INT. COCKPIT - NIGHT");
    scriptPayload[QStringLiteral("category")] = QStringLiteral("script");

    harness.mockDriver.simulateFinished(scriptPayload);
    QCoreApplication::processEvents();

    QCOMPARE(scriptSpy.count(), 1);
    QList<QVariant> scriptArgs = scriptSpy.takeFirst();
    QCOMPARE(scriptArgs.at(0).toString(), QStringLiteral("INT. COCKPIT - NIGHT"));
    QCOMPARE(scriptArgs.at(1).toString(), QStringLiteral("script"));
    QCOMPARE(builder->property("isGenerating").toBool(), false);
}

void Test_PromptBuilder::testDriverAndModelSelectorIntegration()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load PromptBuilder harness.");

    QObject *builder = harness.builder;

    // Checklist Item 2: Upgraded critical child assertions
    QObject *driverCombo = builder->findChild<QObject*>(UI::PromptBuilder::DriverSelectorBox);
    QVERIFY2(driverCombo, "Critical UI Component Missing: 'driverSelectorBox' not found in QML tree. Verify objectName matches ui_constants.");

    QObject *modelCombo = builder->findChild<QObject*>(UI::PromptBuilder::ModelSelectorBox);
    QVERIFY2(modelCombo, "Critical UI Component Missing: 'modelSelectorBox' not found in QML tree. Verify objectName matches ui_constants.");

    // Checklist Item 3: Verify black-box public property aliases exist on root
    int driverAliasIdx = builder->metaObject()->indexOfProperty("currentDriverText");
    QVERIFY2(driverAliasIdx >= 0, "Public QML API Contract broken: 'currentDriverText' alias is missing from PromptBuilder root.");

    int modelAliasIdx = builder->metaObject()->indexOfProperty("currentModelText");
    QVERIFY2(modelAliasIdx >= 0, "Public QML API Contract broken: 'currentModelText' alias is missing from PromptBuilder root.");

    // Checklist Item 4: Verify integration when modality shifts
    builder->setProperty("targetAssetType", "video");
    QCoreApplication::processEvents();

    QVERIFY2(driverCombo->property("enabled").toBool(), "Driver selector disabled unexpectedly after modality shift.");
    QVERIFY2(modelCombo->property("enabled").toBool(), "Model selector disabled unexpectedly after modality shift.");
}

void Test_PromptBuilder::testSplitterLayoutBoundaries()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load PromptBuilder harness.");

    QObject *builder = harness.builder;

    // Checklist Item 2: Upgraded critical child assertions
    QObject *splitter = builder->findChild<QObject*>(UI::PromptBuilder::Splitter);
    QVERIFY2(splitter, "Critical UI Component Missing: 'promptBuilderSplitter' not found.");

    QObject *upperPane = builder->findChild<QObject*>(UI::PromptBuilder::UpperResponsePane);
    QVERIFY2(upperPane, "Critical UI Component Missing: 'upperResponsePane' not found.");

    QObject *lowerPane = builder->findChild<QObject*>(UI::PromptBuilder::LowerInputPane);
    QVERIFY2(lowerPane, "Critical UI Component Missing: 'lowerInputPane' not found.");

    // Checklist Item 5: Verify layout boundary minimums under aggressive crush
    harness.window->setWidth(800);
    harness.window->setHeight(300); // Aggressively small height to force constraint collisions
    QCoreApplication::processEvents();

    double upperHeight = upperPane->property("height").toDouble();
    double lowerHeight = lowerPane->property("height").toDouble();

    // Qt Splitters can be off by fractional pixels depending on dpi scaling, using integer cast or fuzzy compare
    QVERIFY2(upperHeight >= 89.0, qPrintable(QStringLiteral("upperResponsePane violated minimum height constraint. Actual: %1").arg(upperHeight)));
    QVERIFY2(lowerHeight >= 179.0, qPrintable(QStringLiteral("lowerInputPane violated minimum height constraint. Actual: %1").arg(lowerHeight)));
}

void Test_PromptBuilder::testPromptHistoryButtonExists()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load PromptBuilder harness.");

    QObject *historyBtn = harness.builder->findChild<QObject*>(QStringLiteral("promptHistoryBtn"));
    QVERIFY2(historyBtn != nullptr, "UI Component Missing: 'promptHistoryBtn' not found in PromptBuilder.");
    QCOMPARE(historyBtn->property("text").toString(), QStringLiteral("📜 History"));
}

void Test_PromptBuilder::testPromptHistoryPopupOpens()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load PromptBuilder harness.");

    QObject *historyBtn   = harness.builder->findChild<QObject*>(QStringLiteral("promptHistoryBtn"));
    QObject *historyPopup = harness.builder->findChild<QObject*>(QStringLiteral("historyPopup"));
    QVERIFY2(historyBtn != nullptr,   "UI Component Missing: 'promptHistoryBtn' not found.");
    QVERIFY2(historyPopup != nullptr, "UI Component Missing: 'historyPopup' not found.");

    QCOMPARE(historyPopup->property("opened").toBool(), false);

    QMetaObject::invokeMethod(historyBtn, "clicked");
    QTRY_COMPARE_WITH_TIMEOUT(historyPopup->property("opened").toBool(), true, 1000);
    QCOMPARE(historyPopup->property("visible").toBool(), true);

    QMetaObject::invokeMethod(historyPopup, "close");
    QCoreApplication::processEvents();
    QCOMPARE(historyPopup->property("opened").toBool(), false);
}

void Test_PromptBuilder::testRemoteDriverResolvesEstimatorToRemote()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load PromptBuilder harness.");

    QObject *builder = harness.builder;

           // Default mock_engine is treated as local
    QCOMPARE(builder->property("isLocalDriver").toBool(), true);
    QCOMPARE(builder->property("resolvedEstimatorEngine").toString(), QStringLiteral("script"));

           // Swap active driver to a remote REST bridge
    MockDriver remoteDriver(QStringLiteral("gemini_bridge"));
    QVariantMap remoteRegistry;
    remoteRegistry[QStringLiteral("activeDriver")] = QVariant::fromValue<QObject*>(&remoteDriver);
    builder->setProperty("mockEngineRegistry", remoteRegistry);
    QCoreApplication::processEvents();

    QCOMPARE(builder->property("isLocalDriver").toBool(), false);
    QCOMPARE(builder->property("resolvedEstimatorEngine").toString(), QStringLiteral("remote"));
}

QTEST_MAIN(Test_PromptBuilder)
#include "test_PromptBuilder.moc"
