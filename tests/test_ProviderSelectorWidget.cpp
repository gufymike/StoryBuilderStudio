// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"

class Test_ProviderSelectorWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDefaultStateAndAddActionPresent();
    void testLocalModelSelectionIndicator();
    void testCustomEndpointIndicator();
    void testAddCustomProviderFormToggle();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *widget = nullptr;

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
                "    height: 400\n"
                "    visible: true\n"
                "    ProviderSelectorWidget {\n"
                "        id: providerWidget\n"
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

            widget = window->findChild<QObject*>(UI::ProviderSelector::Root);
            return (widget != nullptr);
        }
    };
};

void Test_ProviderSelectorWidget::initTestCase()
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

void Test_ProviderSelectorWidget::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_ProviderSelectorWidget::testDefaultStateAndAddActionPresent()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *widget = harness.widget;
    QVERIFY(widget != nullptr);

    QObject *combo = widget->findChild<QObject*>(UI::ProviderSelector::ProviderCombo);
    QVERIFY2(combo != nullptr, "UI Component Missing: 'providerCombo' not found.");

    QObject *addForm = widget->findChild<QObject*>(UI::ProviderSelector::AddFormBox);
    QVERIFY2(addForm != nullptr, "UI Component Missing: 'addFormBox' not found.");

    // Empty model should still contain '+ Add Custom Provider...'
    QCOMPARE(combo->property("count").toInt(), 1);

    QCOMPARE(addForm->property("visible").toBool(), false);
}

void Test_ProviderSelectorWidget::testLocalModelSelectionIndicator()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *widget = harness.widget;

    QVariantList mockItems;
    QVariantMap localItem;
    localItem[QStringLiteral("name")] = QStringLiteral("Local FLUX.1");
    localItem[QStringLiteral("type")] = QStringLiteral("local");
    localItem[QStringLiteral("localModel")] = QStringLiteral("flux1-schnell");
    mockItems.append(localItem);

    widget->setProperty("mockProviderModel", mockItems);
    QCoreApplication::processEvents();

    QObject *combo = widget->findChild<QObject*>(UI::ProviderSelector::ProviderCombo);
    QVERIFY(combo != nullptr);
    combo->setProperty("currentIndex", 0);
    QCoreApplication::processEvents();

    QCOMPARE(widget->property("selectedProviderType").toString(), QStringLiteral("local"));
    QCOMPARE(widget->property("selectedLocalModel").toString(), QStringLiteral("flux1-schnell"));

    QObject *localIndicator = widget->findChild<QObject*>(UI::ProviderSelector::LocalIndicator);
    QVERIFY2(localIndicator != nullptr, "UI Component Missing: 'localModelIndicator' not found.");
    QCOMPARE(localIndicator->property("visible").toBool(), true);
}

void Test_ProviderSelectorWidget::testCustomEndpointIndicator()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *widget = harness.widget;

    QVariantList mockItems;
    QVariantMap customItem;
    customItem[QStringLiteral("name")] = QStringLiteral("My Endpoint");
    customItem[QStringLiteral("type")] = QStringLiteral("custom");
    customItem[QStringLiteral("url")] = QStringLiteral("http://localhost:8080/v1");
    mockItems.append(customItem);

    widget->setProperty("mockProviderModel", mockItems);
    QCoreApplication::processEvents();

    QObject *combo = widget->findChild<QObject*>(UI::ProviderSelector::ProviderCombo);
    QVERIFY2(combo != nullptr, "UI Component Missing: 'providerCombo' not found.");
    combo->setProperty("currentIndex", 0);
    QCoreApplication::processEvents();

    QCOMPARE(widget->property("selectedProviderType").toString(), QStringLiteral("custom"));

    QObject *customIndicator = widget->findChild<QObject*>(UI::ProviderSelector::CustomIndicator);
    QVERIFY2(customIndicator != nullptr, "UI Component Missing: 'customEndpointIndicator' not found.");
    QCOMPARE(customIndicator->property("visible").toBool(), true);

    QObject *customUrlText = widget->findChild<QObject*>(UI::ProviderSelector::CustomUrlText);
    QVERIFY2(customUrlText != nullptr, "UI Component Missing: 'customUrlText' not found.");
    QVERIFY(customUrlText->property("text").toString().contains(QStringLiteral("http://localhost:8080/v1")));
}

void Test_ProviderSelectorWidget::testAddCustomProviderFormToggle()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load host window");

    QObject *widget = harness.widget;
    QObject *combo = widget->findChild<QObject*>(UI::ProviderSelector::ProviderCombo);
    QVERIFY2(combo != nullptr, "UI Component Missing: 'providerCombo' not found.");
    QObject *addForm = widget->findChild<QObject*>(UI::ProviderSelector::AddFormBox);
    QVERIFY2(addForm != nullptr, "UI Component Missing: 'addFormBox' not found.");


    // Activating the last item ("+ Add Custom Provider...")
    int addIndex = combo->property("count").toInt() - 1;
    combo->setProperty("currentIndex", addIndex);

    // Pass int explicitly to match activated(int)
    bool invoked = QMetaObject::invokeMethod(combo, "activated", Qt::DirectConnection, Q_ARG(int, addIndex));
    QVERIFY(invoked);
    QCoreApplication::processEvents();

    QCOMPARE(addForm->property("visible").toBool(), true);

    // Cancel hides form
    QObject *cancelBtn = widget->findChild<QObject*>(UI::ProviderSelector::CancelCustomBtn);
    QVERIFY2(cancelBtn != nullptr, "UI Component Missing: 'cancelCustomBtn' not found.");
    QMetaObject::invokeMethod(cancelBtn, "clicked");
    QCoreApplication::processEvents();

    QCOMPARE(addForm->property("visible").toBool(), false);
}

QTEST_MAIN(Test_ProviderSelectorWidget)
#include "test_ProviderSelectorWidget.moc"

