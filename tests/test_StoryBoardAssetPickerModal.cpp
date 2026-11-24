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

#include "imagedirectorymodel.h"
#include "audiodirectorymodel.h"
#include "videodirectorymodel.h"
#include "ui_constants.h"

class Test_StoryBoardAssetPickerModal : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testOpenForCategoryModal();
    void testAssetSelectionSignal();
    void testModalCloseToolTip();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *modal = nullptr;

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
                "    width: 800\n"
                "    height: 600\n"
                "    visible: true\n"
                "    StoryBoardAssetPickerModal {\n"
                "        id: pickerModalItem\n"
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

            modal = window->findChild<QObject*>(UI::StoryBoardAssetPickerModal::Root);
            return (modal != nullptr);
        }
    };
};

void Test_StoryBoardAssetPickerModal::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    qmlRegisterType<VideoDirectoryModel>("StoryBuilderStudio", 1, 0, "ScriptsDirectoryModel");
    qmlRegisterType<ImageDirectoryModel>("StoryBuilderStudio", 1, 0, "ImageDirectoryModel");
    qmlRegisterType<AudioDirectoryModel>("StoryBuilderStudio", 1, 0, "AudioDirectoryModel");
    qmlRegisterType<VideoDirectoryModel>("StoryBuilderStudio", 1, 0, "VideoDirectoryModel");

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::cleanPath(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_StoryBoardAssetPickerModal::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_StoryBoardAssetPickerModal::testOpenForCategoryModal()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardAssetPickerModal harness");

    QObject *modal = harness.modal;
    QVERIFY(modal != nullptr);

    QMetaObject::invokeMethod(modal, "openForCategory",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(QStringLiteral("audio"))),
                              Q_ARG(QVariant, QVariant(m_testSessionDir)));
    QCoreApplication::processEvents();

    QCOMPARE(modal->property("category").toString(), QStringLiteral("audio"));
    QCOMPARE(modal->property("projectPath").toString(), m_testSessionDir);

    QObject *titleText = modal->findChild<QObject*>(UI::StoryBoardAssetPickerModal::HeaderTitleText);
    QVERIFY2(titleText != nullptr, "UI Component Missing: 'headerTitleText' not found.");
    QCOMPARE(titleText->property("text").toString(), QStringLiteral("Select AUDIO from Project Library"));

    QMetaObject::invokeMethod(modal, "close");
    QCoreApplication::processEvents();
}

void Test_StoryBoardAssetPickerModal::testAssetSelectionSignal()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardAssetPickerModal harness");

    QObject *modal = harness.modal;
    QVERIFY(modal != nullptr);

    QSignalSpy selectSpy(modal, SIGNAL(assetSelected(QString)));

    QString selectedAsset = QStringLiteral("C:/Workspace/audio/narration.wav");
    modal->metaObject()->invokeMethod(modal, "assetSelected",
                                      Qt::DirectConnection,
                                      Q_ARG(QString, selectedAsset));

    QCOMPARE(selectSpy.count(), 1);
    QCOMPARE(selectSpy.takeFirst().at(0).toString(), selectedAsset);
}

void Test_StoryBoardAssetPickerModal::testModalCloseToolTip()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardAssetPickerModal harness");

    QObject *modal = harness.modal;
    QVERIFY(modal != nullptr);

    QObject *closeBtn = modal->findChild<QObject*>(UI::StoryBoardAssetPickerModal::ClosePickerBtn);
    QVERIFY2(closeBtn != nullptr, "UI Component Missing: 'closePickerBtn' not found.");

    QQmlContext *ctx = QQmlEngine::contextForObject(closeBtn);
    QVERIFY(ctx != nullptr);
    QQmlExpression expr(ctx, qobject_cast<QQuickItem*>(closeBtn), QStringLiteral("ToolTip.text"));
    QCOMPARE(expr.evaluate().toString(), QStringLiteral("Close Asset Picker"));
}

QTEST_MAIN(Test_StoryBoardAssetPickerModal)
#include "test_StoryBoardAssetPickerModal.moc"
