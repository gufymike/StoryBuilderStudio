// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUuid>
#include "app/storyboard/storyboardmanager.h"

class Test_StoryBoardManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDefaultState();
    void testAddAndReorderShots();
    void testShotDuplicateNameError();
    void testAssetAttachmentAndKeyImageAssignment();
    void testCleanupDeletedAsset();

private:
    QString m_testSessionDir;
    QString m_episodeDir;
};

void Test_StoryBoardManager::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    m_episodeDir = m_testSessionDir + QStringLiteral("/Episode_01");
    QDir().mkpath(m_episodeDir);
}

void Test_StoryBoardManager::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_StoryBoardManager::testDefaultState()
{
    StoryBoardManager manager;
    QCOMPARE(manager.shotCount(), 0);
    QVERIFY2(!manager.hasValidPath(), "State Corruption: StoryBoardManager initialized with a valid path.");
}

void Test_StoryBoardManager::testAddAndReorderShots()
{
    StoryBoardManager manager;
    manager.setEpisodePath(m_episodeDir);
    QVERIFY(manager.hasValidPath());

    QVERIFY(manager.addShot(QStringLiteral("Shot 1 - Establishing")));
    QVERIFY(manager.addShot(QStringLiteral("Shot 2 - Close Up")));
    QCOMPARE(manager.shotCount(), 2);

    // Verify ordering
    QCOMPARE(manager.shots().at(0).toMap().value(QStringLiteral("name")).toString(), QStringLiteral("Shot 1 - Establishing"));
    QCOMPARE(manager.shots().at(1).toMap().value(QStringLiteral("name")).toString(), QStringLiteral("Shot 2 - Close Up"));

    // Move Shot 2 up
    QVERIFY(manager.moveShotUp(1));
    QCOMPARE(manager.shots().at(0).toMap().value(QStringLiteral("name")).toString(), QStringLiteral("Shot 2 - Close Up"));
    QCOMPARE(manager.shots().at(1).toMap().value(QStringLiteral("name")).toString(), QStringLiteral("Shot 1 - Establishing"));

    // Verify normalization
    QCOMPARE(manager.shots().at(0).toMap().value(QStringLiteral("order")).toInt(), 0);
    QCOMPARE(manager.shots().at(1).toMap().value(QStringLiteral("order")).toInt(), 1);
}

void Test_StoryBoardManager::testShotDuplicateNameError()
{
    StoryBoardManager manager;
    manager.setEpisodePath(m_episodeDir);
    QSignalSpy errorSpy(&manager, &StoryBoardManager::boardError);

    bool added = manager.addShot(QStringLiteral("Shot 2 - Close Up"));
    QVERIFY2(!added, "Data Integrity Failure: StoryBoardManager allowed a duplicate shot name to be added.");
    QCOMPARE(errorSpy.count(), 1);
}

void Test_StoryBoardManager::testAssetAttachmentAndKeyImageAssignment()
{
    StoryBoardManager manager;
    manager.setEpisodePath(m_episodeDir);

    const QString imgPath1 = QStringLiteral("C:/assets/images/frame1.png");
    const QString imgPath2 = QStringLiteral("C:/assets/images/frame2.png");

    QVERIFY2(manager.addAssetToShot(0, QStringLiteral("images"), imgPath1),
             "Asset Attachment Failure: Failed to attach initial key image to shot.");

    // First image attached should automatically become key_image
    QVariantMap shot = manager.shots().at(0).toMap();
    QCOMPARE(shot.value(QStringLiteral("key_image")).toString(), imgPath1);

    // Add second image
    QVERIFY2(manager.addAssetToShot(0, QStringLiteral("images"), imgPath2),
             "Asset Attachment Failure: Failed to attach second key image to shot.");
    shot = manager.shots().at(0).toMap();
    QCOMPARE(shot.value(QStringLiteral("images")).toList().size(), 2);

    // Remove first image, key_image should fall back to second image
    QVERIFY2(manager.removeAssetFromShot(0, QStringLiteral("images"), imgPath1),
             "Asset Attachment Failure: Failed to remove initial key image from shot.");
    shot = manager.shots().at(0).toMap();
    QCOMPARE(shot.value(QStringLiteral("key_image")).toString(), imgPath2);
}

void Test_StoryBoardManager::testCleanupDeletedAsset()
{
    StoryBoardManager manager;
    manager.setEpisodePath(m_episodeDir);

    const QString imgPath2 = QStringLiteral("C:/assets/images/frame2.png");

    // Clean up deleted asset
    manager.cleanupDeletedAsset(imgPath2);

    QVariantMap shot = manager.shots().at(0).toMap();
    QCOMPARE(shot.value(QStringLiteral("images")).toList().size(), 0);
    QVERIFY(shot.value(QStringLiteral("key_image")).toString().isEmpty());
}

QTEST_MAIN(Test_StoryBoardManager)
#include "test_StoryBoardManager.moc"

