// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUuid>
#include "app/workspace/videodirectorymodel.h"

class Test_VideoDirectoryModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testEmptyState();
    void testScanningVideoDirectory();
    void testFileFilteringAndData();
    void testVideoAssetImport();

private:
    QString m_testSessionDir;
    QString m_videoDir;
    QString m_externalDir;
};

void Test_VideoDirectoryModel::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);

    m_videoDir = m_testSessionDir + QStringLiteral("/assets/video");
    m_externalDir = m_testSessionDir + QStringLiteral("/external");

    QDir().mkpath(m_videoDir);
    QDir().mkpath(m_externalDir);

    // Mock video files
    QFile f1(m_videoDir + QStringLiteral("/scene_01.mp4"));
    if (f1.open(QIODevice::WriteOnly)) {
        f1.write(QByteArray(1024 * 100, 'V')); // 100 KB
        f1.close();
    }

    QFile f2(m_videoDir + QStringLiteral("/scene_02.mkv"));
    if (f2.open(QIODevice::WriteOnly)) {
        f2.write(QByteArray(1024 * 1024 * 3, 'K')); // 3 MB
        f2.close();
    }

    // Ignored format
    QFile fIgnored(m_videoDir + QStringLiteral("/script.pdf"));
    if (fIgnored.open(QIODevice::WriteOnly)) {
        fIgnored.write("PDF Content");
        fIgnored.close();
    }

    // External video for import
    QFile fExt(m_externalDir + QStringLiteral("/broll.webm"));
    if (fExt.open(QIODevice::WriteOnly)) {
        fExt.write("WebM payload");
        fExt.close();
    }
}

void Test_VideoDirectoryModel::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_VideoDirectoryModel::testEmptyState()
{
    VideoDirectoryModel model;
    QVERIFY2(model.rowCount() == 0, "State Corruption: VideoDirectoryModel did not initialize empty.");
    QCOMPARE(model.projectPath(), QString());
}

void Test_VideoDirectoryModel::testScanningVideoDirectory()
{
    VideoDirectoryModel model;
    QSignalSpy pathSpy(&model, &VideoDirectoryModel::projectPathChanged);
    QSignalSpy countSpy(&model, &VideoDirectoryModel::countChanged);

    model.setProjectPath(m_testSessionDir);

    QCOMPARE(pathSpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);
    // Discovers scene_01.mp4 and scene_02.mkv, ignores script.pdf
    QCOMPARE(model.rowCount(), 2);
}

void Test_VideoDirectoryModel::testFileFilteringAndData()
{
    VideoDirectoryModel model;
    model.setProjectPath(m_testSessionDir);

    QModelIndex idx = model.index(0, 0);
    QVERIFY(idx.isValid());

    QString fileName = model.data(idx, AbstractAssetDirectoryModel::FileNameRole).toString();
    QVERIFY2(fileName == QStringLiteral("scene_01.mp4") || fileName == QStringLiteral("scene_02.mkv"),
             "Scanning Failure: Model retrieved unexpected file name or failed filter match.");

    QString filePath = model.data(idx, AbstractAssetDirectoryModel::FilePathRole).toString();
    QVERIFY(filePath.startsWith(QStringLiteral("file:")));
}

void Test_VideoDirectoryModel::testVideoAssetImport()
{
    VideoDirectoryModel model;
    model.setProjectPath(m_testSessionDir);
    int initialCount = model.rowCount();

    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(m_externalDir + QStringLiteral("/broll.webm")));

    int imported = model.importAssets(urls);
    QVERIFY2(imported == 1, "File I/O Failure: Model failed to import external video asset.");

    QCOMPARE(model.rowCount(), initialCount + 1);

    QDir targetDir(m_videoDir);
    QVERIFY2(targetDir.exists(QStringLiteral("broll.webm")), "File I/O Failure: Imported video not found in target directory.");
}

QTEST_MAIN(Test_VideoDirectoryModel)
#include "test_VideoDirectoryModel.moc"

