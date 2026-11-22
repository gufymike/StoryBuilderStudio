/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_InferenceSettingsPanel.cpp
** Unit test suite for InferenceSettingsPanel QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"

class Test_InferenceSettingsPanel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testEmptyDriverFallbackMessage();
    void testDynamicOptionGenerationAndDefaults();
    void testOptionValueModification();
    void testResetToDefaults();

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_InferenceSettingsPanel::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    m_engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
    m_engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
    m_engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/engine"));

    QString qmlFilePath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/engine/InferenceSettingsPanel.qml");
    m_componentUrl = QUrl::fromLocalFile(qmlFilePath);
    QVERIFY2(QFile::exists(qmlFilePath), qPrintable("File not found: " + qmlFilePath));
}

void Test_InferenceSettingsPanel::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_InferenceSettingsPanel::testEmptyDriverFallbackMessage()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> panelObj(component.create());
    QVERIFY(!panelObj.isNull());

    QObject *headerTitle = panelObj->findChild<QObject*>(UI::InferenceSettingsPanel::HeaderTitle);
    QVERIFY2(headerTitle != nullptr, "UI Component Missing: 'headerTitleText' not found.");
    QVERIFY(headerTitle->property("text").toString().contains(QStringLiteral("Default")));

    QVariantMap payload = panelObj->property("currentOptions").toMap();
    QVERIFY(payload.isEmpty());
}

void Test_InferenceSettingsPanel::testDynamicOptionGenerationAndDefaults()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> panelObj(component.create());
    QVERIFY(!panelObj.isNull());

    QVariantList mockOpts;

    QVariantMap stepsOpt;
    stepsOpt[QStringLiteral("key")] = QStringLiteral("steps");
    stepsOpt[QStringLiteral("label")] = QStringLiteral("Steps");
    stepsOpt[QStringLiteral("type")] = QStringLiteral("int");
    stepsOpt[QStringLiteral("defaultValue")] = 25;
    stepsOpt[QStringLiteral("minValue")] = 1;
    stepsOpt[QStringLiteral("maxValue")] = 50;
    mockOpts.append(stepsOpt);

    QVariantMap fastOpt;
    fastOpt[QStringLiteral("key")] = QStringLiteral("use_turbo");
    fastOpt[QStringLiteral("label")] = QStringLiteral("Turbo Mode");
    fastOpt[QStringLiteral("type")] = QStringLiteral("bool");
    fastOpt[QStringLiteral("defaultValue")] = true;
    mockOpts.append(fastOpt);

    panelObj->setProperty("mockDriverId", QStringLiteral("StableDiffusionCpp"));
    panelObj->setProperty("mockDriverOptions", mockOpts);
    QCoreApplication::processEvents();

    QVariantMap currentOpts = panelObj->property("currentOptions").toMap();
    QCOMPARE(currentOpts.value(QStringLiteral("steps")).toInt(), 25);
    QCOMPARE(currentOpts.value(QStringLiteral("use_turbo")).toBool(), true);
}

void Test_InferenceSettingsPanel::testOptionValueModification()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> panelObj(component.create());
    QVERIFY(!panelObj.isNull());

    QVariantList mockOpts;
    QVariantMap cfgOpt;
    cfgOpt[QStringLiteral("key")] = QStringLiteral("cfg_scale");
    cfgOpt[QStringLiteral("label")] = QStringLiteral("CFG Scale");
    cfgOpt[QStringLiteral("type")] = QStringLiteral("float");
    cfgOpt[QStringLiteral("defaultValue")] = 7.0;
    cfgOpt[QStringLiteral("minValue")] = 1.0;
    cfgOpt[QStringLiteral("maxValue")] = 20.0;
    mockOpts.append(cfgOpt);

    panelObj->setProperty("mockDriverOptions", mockOpts);
    QCoreApplication::processEvents();

    // Emulate updating options map
    QVariantMap updated;
    updated[QStringLiteral("cfg_scale")] = 8.5;
    panelObj->setProperty("currentOptions", updated);
    QCoreApplication::processEvents();

    // Receive as QVariant first, then convert to QVariantMap
    QVariant resultVar;
    bool invoked = QMetaObject::invokeMethod(panelObj.data(), "getOptionsPayload",
                                             Qt::DirectConnection,
                                             Q_RETURN_ARG(QVariant, resultVar));
    QVERIFY2(invoked, "Failed to invoke getOptionsPayload()");

    QVariantMap resultPayload = resultVar.toMap();
    QCOMPARE(resultPayload.value(QStringLiteral("cfg_scale")).toDouble(), 8.5);
}

void Test_InferenceSettingsPanel::testResetToDefaults()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> panelObj(component.create());
    QVERIFY(!panelObj.isNull());

    QVariantList mockOpts;
    QVariantMap seedOpt;
    seedOpt[QStringLiteral("key")] = QStringLiteral("seed");
    seedOpt[QStringLiteral("label")] = QStringLiteral("Random Seed");
    seedOpt[QStringLiteral("type")] = QStringLiteral("int");
    seedOpt[QStringLiteral("defaultValue")] = -1;
    mockOpts.append(seedOpt);

    panelObj->setProperty("mockDriverOptions", mockOpts);
    QCoreApplication::processEvents();

    // Modify value
    QVariantMap modified;
    modified[QStringLiteral("seed")] = 42;
    panelObj->setProperty("currentOptions", modified);
    QCoreApplication::processEvents();

    QCOMPARE(panelObj->property("currentOptions").toMap().value(QStringLiteral("seed")).toInt(), 42);

    // Trigger Reset Defaults button
    QObject *resetBtn = panelObj->findChild<QObject*>(UI::InferenceSettingsPanel::ResetDefaultsBtn);
    QVERIFY2(resetBtn != nullptr, "UI Component Missing: 'resetDefaultsBtn' not found.");
    QMetaObject::invokeMethod(resetBtn, "clicked");
    QCoreApplication::processEvents();

    QCOMPARE(panelObj->property("currentOptions").toMap().value(QStringLiteral("seed")).toInt(), -1);
}

QTEST_MAIN(Test_InferenceSettingsPanel)
#include "test_InferenceSettingsPanel.moc"
