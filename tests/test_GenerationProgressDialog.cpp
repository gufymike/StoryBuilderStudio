/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_GenerationProgressDialog.cpp
** Unit test suite for GenerationProgressDialog and PromptBuilder progress/busy UI.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QStandardPaths>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUrl>

#include "app/engine/abstractinferencedriver.h"

class MockProgressDriver : public AbstractInferenceDriver
{
    Q_OBJECT
public:
    explicit MockProgressDriver(const QString &id = QStringLiteral("sd_image"),
                                const QString &assetType = QStringLiteral("image"),
                                QObject *parent = nullptr)
        : AbstractInferenceDriver(parent)
        , m_driverId(id)
        , m_assetType(assetType)
    {}

    QString driverId() const override { return m_driverId; }
    QString displayName() const override { return QStringLiteral("Mock SD Image"); }
    QString targetAssetType() const override { return m_assetType; }
    bool isModelLoaded() const override { return m_loaded; }
    bool isBusy() const override { return m_busy; }
    QString loadedModelName() const override { return QStringLiteral("flux1-dev-Q4_0.gguf"); }

    bool loadModel(const QVariantMap &) override
    {
        m_loaded = true;
        emit modelLoadedChanged(true);
        return true;
    }

    void unloadModel() override
    {
        m_loaded = false;
        emit modelLoadedChanged(false);
    }

    void cancelGeneration() override
    {
        ++cancelCallCount;
    }

    QVariantList availableOptions() const override { return {}; }
    QVariantMap currentOptions() const override { return {}; }
    bool setOption(const QString &, const QVariant &) override { return true; }

    void generate(const QString &prompt,
                  const QString &,
                  const QStringList &,
                  const QString &,
                  const QVariantMap &) override
    {
        ++generateCallCount;
        lastPrompt = prompt;
        m_busy = true;
        emit busyChanged(true);
    }

    void verifyConnection() override
    {
        emit connectionVerified(true, QStringLiteral("OK"));
    }

    void completeRun()
    {
        m_busy = false;
        emit busyChanged(false);
    }

    int generateCallCount = 0;
    int cancelCallCount = 0;
    QString lastPrompt;

private:
    QString m_driverId;
    QString m_assetType;
    bool m_loaded = true;
    bool m_busy = false;
};

class MockEngineRegistry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AbstractInferenceDriver *activeDriver READ activeDriver CONSTANT)
public:
    explicit MockEngineRegistry(AbstractInferenceDriver *driver, QObject *parent = nullptr)
        : QObject(parent), m_driver(driver) {}

    AbstractInferenceDriver *activeDriver() const { return m_driver; }

private:
    AbstractInferenceDriver *m_driver = nullptr;
};

static QQuickItem *findVisualItem(QQuickItem *root, const QString &objectName)
{
    if (!root) return nullptr;
    if (root->objectName() == objectName) return root;
    const QList<QQuickItem *> children = root->childItems();
    for (QQuickItem *child : children) {
        if (QQuickItem *found = findVisualItem(child, objectName)) {
            return found;
        }
    }
    return nullptr;
}

class Test_GenerationProgressDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testStartTrackingInitializesIndeterminateState();
    void testElapsedTimerFormattingAndTick();
    void testDeterminateProgressUpdates();
    void testRunInBackgroundClosesWithoutCancelling();
    void testCancelButtonDisablesAndEmitsSignal();
    void testPromptBuilderIntegrationAndViewportBusyOverlay();

private:
    QUrl m_dialogUrl;
    QUrl m_promptBuilderUrl;
};

void Test_GenerationProgressDialog::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    const QString dialogPath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/engine/GenerationProgressDialog.qml");
    const QString builderPath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/promptbuilder/PromptBuilder.qml");

    QVERIFY2(QFile::exists(dialogPath), qPrintable("Missing file: " + dialogPath));
    QVERIFY2(QFile::exists(builderPath), qPrintable("Missing file: " + builderPath));

    m_dialogUrl        = QUrl(QStringLiteral("qrc:/qt/qml/StoryBuilderStudio/app/engine/GenerationProgressDialog.qml"));
    m_promptBuilderUrl = QUrl(QStringLiteral("qrc:/qt/qml/StoryBuilderStudio/app/promptbuilder/PromptBuilder.qml"));
}

void Test_GenerationProgressDialog::cleanupTestCase()
{
    QStandardPaths::setTestModeEnabled(false);
}

void Test_GenerationProgressDialog::testStartTrackingInitializesIndeterminateState()
{
    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
    engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));

    QQuickWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQmlComponent component(&engine, m_dialogUrl);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());
    dialog->setProperty("parent", QVariant::fromValue(window.contentItem()));

    QMetaObject::invokeMethod(dialog.data(), "startTracking",
                              Q_ARG(QVariant, QStringLiteral("image")),
                              Q_ARG(QVariant, QStringLiteral("sd_image")),
                              Q_ARG(QVariant, QStringLiteral("Running FLUX...")));
    QTest::qWait(30);

    QCOMPARE(dialog->property("opened").toBool(), true);
    QCOMPARE(dialog->property("cancelling").toBool(), false);
    QCOMPARE(dialog->property("elapsedSeconds").toInt(), 0);
    QVERIFY(dialog->property("progressValue").toReal() < 0.0);

    QQuickItem *titleText = findVisualItem(window.contentItem(), QStringLiteral("progressTitleText"));
    QVERIFY(titleText != nullptr);
    QCOMPARE(titleText->property("text").toString(), QStringLiteral("Generating Image"));

    QQuickItem *driverText = findVisualItem(window.contentItem(), QStringLiteral("driverInfoText"));
    QVERIFY(driverText != nullptr);
    QCOMPARE(driverText->property("text").toString(), QStringLiteral("Driver: sd_image"));

    QQuickItem *progressBar = findVisualItem(window.contentItem(), QStringLiteral("genProgressBar"));
    QVERIFY(progressBar != nullptr);
    QCOMPARE(progressBar->property("indeterminate").toBool(), true);

    QMetaObject::invokeMethod(dialog.data(), "finishTracking");
}

void Test_GenerationProgressDialog::testElapsedTimerFormattingAndTick()
{
    QQmlApplicationEngine engine;
    QQuickWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQmlComponent component(&engine, m_dialogUrl);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());
    dialog->setProperty("parent", QVariant::fromValue(window.contentItem()));

    QVariant formattedZero;
    QMetaObject::invokeMethod(dialog.data(), "formatElapsed",
                              Q_RETURN_ARG(QVariant, formattedZero),
                              Q_ARG(QVariant, 0));
    QCOMPARE(formattedZero.toString(), QStringLiteral("00:00"));

    QVariant formattedMinute;
    QMetaObject::invokeMethod(dialog.data(), "formatElapsed",
                              Q_RETURN_ARG(QVariant, formattedMinute),
                              Q_ARG(QVariant, 65));
    QCOMPARE(formattedMinute.toString(), QStringLiteral("01:05"));

    QMetaObject::invokeMethod(dialog.data(), "startTracking",
                              Q_ARG(QVariant, QStringLiteral("script")),
                              Q_ARG(QVariant, QStringLiteral("llama_cpp")),
                              Q_ARG(QVariant, QStringLiteral("Generating...")));

    QTRY_VERIFY_WITH_TIMEOUT(dialog->property("elapsedSeconds").toInt() >= 1, 2000);

    QQuickItem *elapsedText = findVisualItem(window.contentItem(), QStringLiteral("elapsedTimeText"));
    QVERIFY(elapsedText != nullptr);
    QVERIFY(elapsedText->property("text").toString() != QStringLiteral("00:00"));

    QMetaObject::invokeMethod(dialog.data(), "finishTracking");
}

void Test_GenerationProgressDialog::testDeterminateProgressUpdates()
{
    QQmlApplicationEngine engine;
    QQuickWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQmlComponent component(&engine, m_dialogUrl);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());
    dialog->setProperty("parent", QVariant::fromValue(window.contentItem()));

    QMetaObject::invokeMethod(dialog.data(), "startTracking",
                              Q_ARG(QVariant, QStringLiteral("image")),
                              Q_ARG(QVariant, QStringLiteral("sd_image")),
                              Q_ARG(QVariant, QStringLiteral("Initializing...")));

    QMetaObject::invokeMethod(dialog.data(), "updateProgress",
                              Q_ARG(QVariant, QStringLiteral("Step 2 of 4 (74.3s/it)")),
                              Q_ARG(QVariant, 0.5));
    QTest::qWait(20);

    QQuickItem *progressBar = findVisualItem(window.contentItem(), QStringLiteral("genProgressBar"));
    QVERIFY(progressBar != nullptr);
    QCOMPARE(progressBar->property("indeterminate").toBool(), false);
    QCOMPARE(progressBar->property("value").toReal(), 0.5);

    QQuickItem *statusText = findVisualItem(window.contentItem(), QStringLiteral("generationStatusText"));
    QVERIFY(statusText != nullptr);
    QCOMPARE(statusText->property("text").toString(), QStringLiteral("Step 2 of 4 (74.3s/it)"));

    QMetaObject::invokeMethod(dialog.data(), "finishTracking");
}

void Test_GenerationProgressDialog::testRunInBackgroundClosesWithoutCancelling()
{
    QQmlApplicationEngine engine;
    QQuickWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQmlComponent component(&engine, m_dialogUrl);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());
    dialog->setProperty("parent", QVariant::fromValue(window.contentItem()));

    QSignalSpy cancelSpy(dialog.data(), SIGNAL(cancelRequested()));

    QMetaObject::invokeMethod(dialog.data(), "startTracking",
                              Q_ARG(QVariant, QStringLiteral("video")),
                              Q_ARG(QVariant, QStringLiteral("sd_video")),
                              Q_ARG(QVariant, QStringLiteral("Rendering frames...")));
    QTest::qWait(30);
    QCOMPARE(dialog->property("opened").toBool(), true);

    QQuickItem *bgBtn = findVisualItem(window.contentItem(), QStringLiteral("runInBackgroundBtn"));
    QVERIFY2(bgBtn != nullptr, "Expected runInBackgroundBtn inside GenerationProgressDialog");
    QMetaObject::invokeMethod(bgBtn, "clicked");
    QTest::qWait(30);

    QCOMPARE(dialog->property("opened").toBool(), false);
    QCOMPARE(dialog->property("cancelling").toBool(), false);
    QCOMPARE(cancelSpy.count(), 0);
}

void Test_GenerationProgressDialog::testCancelButtonDisablesAndEmitsSignal()
{
    QQmlApplicationEngine engine;
    QQuickWindow window;
    window.resize(800, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQmlComponent component(&engine, m_dialogUrl);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> dialog(component.create());
    QVERIFY(!dialog.isNull());
    dialog->setProperty("parent", QVariant::fromValue(window.contentItem()));

    QSignalSpy cancelSpy(dialog.data(), SIGNAL(cancelRequested()));

    QMetaObject::invokeMethod(dialog.data(), "startTracking",
                              Q_ARG(QVariant, QStringLiteral("image")),
                              Q_ARG(QVariant, QStringLiteral("sd_image")),
                              Q_ARG(QVariant, QStringLiteral("Sampling...")));
    QTest::qWait(30);

    QQuickItem *cancelBtn = findVisualItem(window.contentItem(), QStringLiteral("cancelGenerationBtn"));
    QVERIFY2(cancelBtn != nullptr, "Expected cancelGenerationBtn inside GenerationProgressDialog");
    QCOMPARE(cancelBtn->property("enabled").toBool(), true);

    QMetaObject::invokeMethod(cancelBtn, "clicked");
    QTest::qWait(30);

    QCOMPARE(cancelSpy.count(), 1);
    QCOMPARE(dialog->property("cancelling").toBool(), true);
    QCOMPARE(cancelBtn->property("enabled").toBool(), false);
    QCOMPARE(cancelBtn->property("text").toString(), QStringLiteral("Cancelling..."));

    QQuickItem *statusText = findVisualItem(window.contentItem(), QStringLiteral("generationStatusText"));
    QVERIFY(statusText != nullptr);
    QCOMPARE(statusText->property("text").toString(), QStringLiteral("Cancelling after current step..."));

    QMetaObject::invokeMethod(dialog.data(), "finishTracking");
    QCOMPARE(dialog->property("opened").toBool(), false);
    QCOMPARE(dialog->property("cancelling").toBool(), false);
}

void Test_GenerationProgressDialog::testPromptBuilderIntegrationAndViewportBusyOverlay()
{
    MockProgressDriver mockDriver(QStringLiteral("sd_image"), QStringLiteral("image"));
    MockEngineRegistry mockRegistry(&mockDriver);

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
    engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
    engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/core"));
    engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/engine"));
    engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/promptbuilder"));

    QQuickWindow window;
    window.resize(700, 620);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQmlComponent component(&engine, m_promptBuilderUrl);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> builderObj(component.create());
    QVERIFY(!builderObj.isNull());

    QQuickItem *builderItem = qobject_cast<QQuickItem *>(builderObj.data());
    QVERIFY(builderItem != nullptr);
    builderItem->setParentItem(window.contentItem());
    builderItem->setWidth(window.width());
    builderItem->setHeight(window.height());

    builderObj->setProperty("targetAssetType", QStringLiteral("image"));
    builderObj->setProperty("mockEngineRegistry", QVariant::fromValue(&mockRegistry));
    builderObj->setProperty("promptText", QStringLiteral("A neon-lit orbital shipyard at dusk"));
    QTest::qWait(30);

    // Overflow responseDisplayArea well beyond viewport height to verify busy overlay stays centered
    QString longLog;
    for (int i = 0; i < 120; ++i) {
        longLog += QStringLiteral("Log line %1: verifying viewport-pinned busy overlay alignment\n").arg(i);
    }
    QMetaObject::invokeMethod(builderObj.data(), "appendResponseText", Q_ARG(QVariant, longLog));
    QTest::qWait(30);

    QQuickItem *submitBtn = findVisualItem(window.contentItem(), QStringLiteral("submitPromptBtn"));
    QVERIFY(submitBtn != nullptr);
    QCOMPARE(submitBtn->property("enabled").toBool(), true);
    QMetaObject::invokeMethod(submitBtn, "clicked");
    QTest::qWait(30);

    QQuickItem *confirmBtn = findVisualItem(window.contentItem(), QStringLiteral("confirmDispatchBtn"));
    QVERIFY(confirmBtn != nullptr);
    QMetaObject::invokeMethod(confirmBtn, "clicked");
    QTest::qWait(30);

    QCOMPARE(mockDriver.generateCallCount, 1);
    QCOMPARE(builderObj->property("isGenerating").toBool(), true);

    QObject *progressDialog = builderObj->findChild<QObject *>(QStringLiteral("generationProgressDialog"));
    QVERIFY2(progressDialog != nullptr, "Expected child generationProgressDialog in PromptBuilder");
    QCOMPARE(progressDialog->property("opened").toBool(), true);

    // Verify busy overlay matches the responseWellRect viewport rather than scrolling off-screen
    QQuickItem *responseWell = findVisualItem(window.contentItem(), QStringLiteral("responseWellRect"));
    QQuickItem *busyOverlay = findVisualItem(window.contentItem(), QStringLiteral("responseBusyOverlay"));
    QVERIFY(responseWell != nullptr);
    QVERIFY(busyOverlay != nullptr);
    QCOMPARE(busyOverlay->property("visible").toBool(), true);
    QCOMPARE(busyOverlay->x(), 0.0);
    QCOMPARE(busyOverlay->y(), 0.0);
    QCOMPARE(busyOverlay->width(), responseWell->width());
    QCOMPARE(busyOverlay->height(), responseWell->height());

    // Emit 3-argument progressUpdated signal and verify dialog updates
    emit mockDriver.progressUpdated(2, 4, 12.5f);
    QTest::qWait(20);
    QCOMPARE(progressDialog->property("progressValue").toReal(), 0.5);

    QQuickItem *statusText = findVisualItem(window.contentItem(), QStringLiteral("generationStatusText"));
    QVERIFY(statusText != nullptr);
    QCOMPARE(statusText->property("text").toString(), QStringLiteral("Step 2 of 4 (12.5s/it)"));

    // Click Cancel inside the progress popup and verify driver receives cancelGeneration()
    QQuickItem *cancelBtn = findVisualItem(window.contentItem(), QStringLiteral("cancelGenerationBtn"));
    QVERIFY(cancelBtn != nullptr);
    QMetaObject::invokeMethod(cancelBtn, "clicked");
    QTest::qWait(20);
    QCOMPARE(mockDriver.cancelCallCount, 1);

    // Emit engineError and verify dialog closes and isGenerating resets
    mockDriver.completeRun();
    emit mockDriver.engineError(QStringLiteral("Generation cancelled by user."));
    QTest::qWait(30);

    QCOMPARE(progressDialog->property("opened").toBool(), false);
    QCOMPARE(builderObj->property("isGenerating").toBool(), false);
    QCOMPARE(busyOverlay->property("visible").toBool(), false);
}

QTEST_MAIN(Test_GenerationProgressDialog)
#include "test_GenerationProgressDialog.moc"
