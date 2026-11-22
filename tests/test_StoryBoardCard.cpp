/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_StoryBoardCard.cpp
** Unit test suite for StoryBoardCard QML component under QTest.
**
****************************************************************************/
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

class Test_StoryBoardCard : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testCardInitializationAndOrderBadge();
    void testActionSignals();
    void testToolTipsConfigured();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *card = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/storyboard"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 300\n"
                "    height: 350\n"
                "    visible: true\n"
                "    StoryBoardCard {\n"
                "        id: cardItem\n"
                "        shotIndex: 2\n"
                "        shotData: ({\n"
                "            \"name\": \"Shot 03 - Space Station Approaching\",\n"
                "            \"key_image\": \"\",\n"
                "            \"final_shot\": \"\",\n"
                "            \"images\": [\"img1.png\", \"img2.png\"],\n"
                "            \"audio\": [\"snd1.wav\"],\n"
                "            \"videos\": []\n"
                "        })\n"
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

            card = window->findChild<QObject*>(UI::StoryBoardCard::Root);
            return (card != nullptr);
        }
    };
};

void Test_StoryBoardCard::initTestCase()
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

void Test_StoryBoardCard::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_StoryBoardCard::testCardInitializationAndOrderBadge()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardCard harness");

    QObject *card = harness.card;
    QVERIFY(card != nullptr);


    QObject *orderBadgeText = card->findChild<QObject*>(UI::StoryBoardCard::OrderBadgeText);
    QVERIFY2(orderBadgeText != nullptr, "UI Component Missing: 'orderBadgeText' not found.");

    QObject *shotTitleText = card->findChild<QObject*>(UI::StoryBoardCard::ShotTitleText);
    QVERIFY2(shotTitleText != nullptr, "UI Component Missing: 'shotTitleText' not found.");

    QObject *scriptCountText = card->findChild<QObject*>(UI::StoryBoardCard::ScriptCountText);
    QVERIFY2(scriptCountText != nullptr, "UI Component Missing: 'scriptCountText' not found.");
    QObject *imgCountText = card->findChild<QObject*>(UI::StoryBoardCard::ImgCountText);
    QVERIFY2(imgCountText != nullptr, "UI Component Missing: 'imgCountText' not found.");
    QObject *audioCountText = card->findChild<QObject*>(UI::StoryBoardCard::AudioCountText);
    QVERIFY2(audioCountText != nullptr, "UI Component Missing: 'audioCountText' not found.");
    QObject *videoCountText = card->findChild<QObject*>(UI::StoryBoardCard::VideoCountText);
    QVERIFY2(videoCountText != nullptr, "UI Component Missing: 'videoCountText' not found.");

    // Verify shotIndex and order badge text (shotIndex 2 -> "03")
    QCOMPARE(orderBadgeText->property("text").toString(), QStringLiteral("03"));
    // Verify title text
    QCOMPARE(shotTitleText->property("text").toString(), QStringLiteral("Shot 03 - Space Station Approaching"));
    // Verify asset counts
    QCOMPARE(scriptCountText->property("text").toString(), QStringLiteral("📝 0"));
    QCOMPARE(imgCountText->property("text").toString(), QStringLiteral("🖼 2"));
    QCOMPARE(audioCountText->property("text").toString(), QStringLiteral("🔊 1"));
    QCOMPARE(videoCountText->property("text").toString(), QStringLiteral("🎥 0"));
}

void Test_StoryBoardCard::testActionSignals()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardCard harness");

    QObject *card = harness.card;
    QVERIFY(card != nullptr);

    QSignalSpy moveUpSpy(card, SIGNAL(moveUpRequested()));
    QSignalSpy moveDownSpy(card, SIGNAL(moveDownRequested()));
    QSignalSpy deleteSpy(card, SIGNAL(deleteRequested()));

    QObject *moveUpBtn = card->findChild<QObject*>(UI::StoryBoardCard::MoveUpBtn);
    QVERIFY2(moveUpBtn != nullptr, "UI Component Missing: 'moveUpBtn' not found.");
    QObject *moveDownBtn = card->findChild<QObject*>(UI::StoryBoardCard::MoveDownBtn);
    QVERIFY2(moveDownBtn != nullptr, "UI Component Missing: 'moveDownBtn' not found.");
    QObject *deleteBtn = card->findChild<QObject*>(UI::StoryBoardCard::DeleteBtn);
    QVERIFY2(deleteBtn != nullptr, "UI Component Missing: 'deleteBtn' not found.");


    QMetaObject::invokeMethod(moveUpBtn, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(moveUpSpy.count(), 1);

    QMetaObject::invokeMethod(moveDownBtn, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(moveDownSpy.count(), 1);

    QMetaObject::invokeMethod(deleteBtn, "clicked");
    QCoreApplication::processEvents();
    QCOMPARE(deleteSpy.count(), 1);
}

void Test_StoryBoardCard::testToolTipsConfigured()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load StoryBoardCard harness");

    QObject *card = harness.card;
    QVERIFY(card != nullptr);

    auto getAttachedToolTip = [](QObject *item) -> QString {
        if (!item) return QString();
        QQmlContext *ctx = QQmlEngine::contextForObject(item);
        if (!ctx) return QString();
        QQmlExpression expr(ctx, qobject_cast<QQuickItem*>(item), QStringLiteral("ToolTip.text"));
        return expr.evaluate().toString();
    };

    QObject *moveUpBtn = card->findChild<QObject*>(UI::StoryBoardCard::MoveUpBtn);
    QVERIFY2(moveUpBtn != nullptr, "UI Component Missing: 'moveUpBtn' not found.");
    QObject *moveDownBtn = card->findChild<QObject*>(UI::StoryBoardCard::MoveDownBtn);
    QVERIFY2(moveDownBtn != nullptr, "UI Component Missing: 'moveDownBtn' not found.");
    QObject *deleteBtn = card->findChild<QObject*>(UI::StoryBoardCard::DeleteBtn);
    QVERIFY2(deleteBtn != nullptr, "UI Component Missing: 'deleteBtn' not found.");

    QCOMPARE(getAttachedToolTip(moveUpBtn), QStringLiteral("Move shot left / earlier in sequence"));
    QCOMPARE(getAttachedToolTip(moveDownBtn), QStringLiteral("Move shot right / later in sequence"));
    QCOMPARE(getAttachedToolTip(deleteBtn), QStringLiteral("Remove this shot from storyboard"));
}

QTEST_MAIN(Test_StoryBoardCard)
#include "test_StoryBoardCard.moc"
