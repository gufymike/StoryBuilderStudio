// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQuickItem>
#include "ui_constants.h"


class Test_StoryBoardAssetList : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testEmptyState();
    void testPopulatedListAndSignals();
    void testToolTipsConfigured();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *assetList = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/storyboard"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 400\n"
                "    height: 500\n"
                "    visible: true\n"
                "    StoryBoardAssetList {\n"
                "        id: listComponent\n"
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

            assetList = window->findChild<QObject*>(UI::StoryBoardAssetList::Root);
            return (assetList != nullptr);
        }
    };
};

void Test_StoryBoardAssetList::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::cleanPath(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_StoryBoardAssetList::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_StoryBoardAssetList::testEmptyState()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardAssetList harness");

    QObject *listRoot = harness.assetList;
    QVERIFY(listRoot != nullptr);

    QObject *emptyLabel = listRoot->findChild<QObject*>(UI::StoryBoardAssetList::EmptyLabel);
    QVERIFY2(emptyLabel != nullptr, "UI Component Missing: 'emptyLabel' not found.");
    QCOMPARE(emptyLabel->property("visible").toBool(), true);
    QCOMPARE(emptyLabel->property("text").toString(), QStringLiteral("No images attached to this shot"));
}

void Test_StoryBoardAssetList::testPopulatedListAndSignals()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardAssetList harness");

    QObject *listRoot = harness.assetList;
    QVERIFY(listRoot != nullptr);

    QSignalSpy keySpy(listRoot, SIGNAL(setKeyRequested(QString)));
    QSignalSpy finalSpy(listRoot, SIGNAL(setFinalRequested(QString)));
    QSignalSpy removeSpy(listRoot, SIGNAL(removeRequested(QString)));

    QString testPath = QStringLiteral("C:/test_project/images/frame_01.png");
    QVariantList items = { testPath };
    listRoot->setProperty("modelDataList", items);
    QCoreApplication::processEvents();

    // Verify key signal trigger
    QMetaObject::invokeMethod(listRoot, "setKeyRequested",
                              Qt::DirectConnection,
                              Q_ARG(QString, testPath));
    QCOMPARE(keySpy.count(), 1);
    QCOMPARE(keySpy.takeFirst().at(0).toString(), testPath);

    // Verify final signal trigger
    QMetaObject::invokeMethod(listRoot, "setFinalRequested",
                              Qt::DirectConnection,
                              Q_ARG(QString, testPath));
    QCOMPARE(finalSpy.count(), 1);
    QCOMPARE(finalSpy.takeFirst().at(0).toString(), testPath);

    // Verify remove signal trigger
    QMetaObject::invokeMethod(listRoot, "removeRequested",
                              Qt::DirectConnection,
                              Q_ARG(QString, testPath));
    QCOMPARE(removeSpy.count(), 1);
    QCOMPARE(removeSpy.takeFirst().at(0).toString(), testPath);
}

void Test_StoryBoardAssetList::testToolTipsConfigured()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardAssetList harness");

    QObject *listRoot = harness.assetList;
    QVERIFY(listRoot != nullptr);

    QObject *attachBtn = listRoot->findChild<QObject*>(UI::StoryBoardAssetList::AttachAssetBtn);
    QVERIFY2(attachBtn != nullptr, "UI Component Missing: 'attachAssetBtn' not found.");

    QQmlContext *ctx = QQmlEngine::contextForObject(attachBtn);
    QVERIFY(ctx != nullptr);
    QQmlExpression expr(ctx, qobject_cast<QQuickItem*>(attachBtn), QStringLiteral("ToolTip.text"));
    QCOMPARE(expr.evaluate().toString(), QStringLiteral("Browse library or project to attach images"));
}

QTEST_MAIN(Test_StoryBoardAssetList)
#include "test_StoryBoardAssetList.moc"
