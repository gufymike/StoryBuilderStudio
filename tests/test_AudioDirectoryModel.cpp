// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUuid>
#include "app/workspace/audiodirectorymodel.h"

class Test_AudioDirectoryModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testInitialEmptyState();
    void testScanningAudioDirectory();
    void testFileRolesAndData();
    void testAssetImportingAndDeduplication();

private:
    QString m_testSessionDir;
    QString m_audioDir;
    QString m_externalDir;
};

void Test_AudioDirectoryModel::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);

    m_audioDir = m_testSessionDir + QStringLiteral("/assets/audio");
    m_externalDir = m_testSessionDir + QStringLiteral("/external");

    QDir().mkpath(m_audioDir);
    QDir().mkpath(m_externalDir);

    // Create mock audio files in project assets
    QFile f1(m_audioDir + QStringLiteral("/theme.wav"));
    if (f1.open(QIODevice::WriteOnly)) {
        f1.write(QByteArray(1024 * 50, 'A')); // 50 KB
        f1.close();
    }

    QFile f2(m_audioDir + QStringLiteral("/sfx_laser.mp3"));
    if (f2.open(QIODevice::WriteOnly)) {
        f2.write(QByteArray(1024 * 1024 * 2, 'B')); // 2 MB
        f2.close();
    }

    // Create an un-matched file format (should be ignored by filter)
    QFile f3(m_audioDir + QStringLiteral("/notes.txt"));
    if (f3.open(QIODevice::WriteOnly)) {
        f3.write("Some notes");
        f3.close();
    }

    // External file to import
    QFile fExternal(m_externalDir + QStringLiteral("/dialogue.flac"));
    if (fExternal.open(QIODevice::WriteOnly)) {
        fExternal.write("Sample dialogue");
        fExternal.close();
    }
}

void Test_AudioDirectoryModel::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_AudioDirectoryModel::testInitialEmptyState()
{
    AudioDirectoryModel model;
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.projectPath(), QString());
}

void Test_AudioDirectoryModel::testScanningAudioDirectory()
{
    AudioDirectoryModel model;
    QSignalSpy pathSpy(&model, &AudioDirectoryModel::projectPathChanged);
    QSignalSpy countSpy(&model, &AudioDirectoryModel::countChanged);

    model.setProjectPath(m_testSessionDir);

    QCOMPARE(pathSpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);
    // Should discover theme.wav and sfx_laser.mp3, ignoring notes.txt
    QCOMPARE(model.rowCount(), 2);
}

void Test_AudioDirectoryModel::testFileRolesAndData()
{
    AudioDirectoryModel model;
    model.setProjectPath(m_testSessionDir);

    QModelIndex idx = model.index(0, 0);
    QVERIFY(idx.isValid());

    QString fileName = model.data(idx, AudioDirectoryModel::FileNameRole).toString();
    QString filePath = model.data(idx, AudioDirectoryModel::FilePathRole).toString();


    QVERIFY2(fileName == QStringLiteral("sfx_laser.mp3") || fileName == QStringLiteral("theme.wav"),
             "Scanning Failure: Model retrieved unexpected file name or failed filter match.");
    QVERIFY2(filePath.startsWith(QStringLiteral("file:")), "Formatting Failure: File path role did not return a valid QUrl schema string.");

    QString sizeFormatted = model.data(idx, AudioDirectoryModel::FileSizeRole).toString();
    QVERIFY2(sizeFormatted.endsWith(QStringLiteral("KB")) || sizeFormatted.endsWith(QStringLiteral("MB")),
             "Formatting Failure: size was not formatted correctly.");
}

void Test_AudioDirectoryModel::testAssetImportingAndDeduplication()
{
    AudioDirectoryModel model;
    model.setProjectPath(m_testSessionDir);

    int initialCount = model.rowCount();

    QList<QUrl> importUrls;
    importUrls.append(QUrl::fromLocalFile(m_externalDir + QStringLiteral("/dialogue.flac")));

    QSignalSpy importSpy(&model, &AudioDirectoryModel::assetsImported);
    int imported = model.importAssets(importUrls);

    QVERIFY2(imported == 1, "File I/O Failure: Model failed to import external audio asset.");
    QCOMPARE(importSpy.count(), 1);
    QCOMPARE(model.rowCount(), initialCount + 1);

    // Test Deduplication: Re-importing same file must produce _copy
    int importedAgain = model.importAssets(importUrls);
    QCOMPARE(importedAgain, 1);
    QCOMPARE(model.rowCount(), initialCount + 2);

    QDir targetDir(m_audioDir);

    QVERIFY2(targetDir.exists(QStringLiteral("dialogue_copy.flac")),
             "Deduplication Failure: Re-imported file did not receive the '_copy' suffix.");
}

QTEST_MAIN(Test_AudioDirectoryModel)
#include "test_AudioDirectoryModel.moc"

