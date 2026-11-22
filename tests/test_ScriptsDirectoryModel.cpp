/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_ScriptsDirectoryModel.cpp
** Unit test suite for ScriptsDirectoryModel under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUuid>
#include "app/workspace/scriptsdirectorymodel.h"

class Test_ScriptsDirectoryModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testInitialEmptyState();
    void testScanningScriptsDirectory();
    void testFileRolesAndData();
    void testAssetImportingAndDeduplication();

private:
    QString m_testSessionDir;
    QString m_scriptsDir;
    QString m_externalDir;
};

void Test_ScriptsDirectoryModel::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);

    m_scriptsDir = m_testSessionDir + QStringLiteral("/assets/scripts");
    m_externalDir = m_testSessionDir + QStringLiteral("/external");

    QDir().mkpath(m_scriptsDir);
    QDir().mkpath(m_externalDir);

    // Create mock audio files in project assets
    QFile f1(m_scriptsDir + QStringLiteral("/ep1.md"));
    if (f1.open(QIODevice::Text|QIODevice::WriteOnly)) {
        f1.write("#### Test MD File"); // 50 KB
        f1.close();
    }

    QFile f2(m_scriptsDir + QStringLiteral("/story.txt"));
    if (f2.open(QIODevice::Text|QIODevice::WriteOnly)) {
        f2.write("The year was 1999..."); // 2 MB
        f2.close();
    }

    // Create an un-matched file format (should be ignored by filter)
    QFile f3(m_scriptsDir + QStringLiteral("/theme.wav"));
    if (f3.open(QIODevice::WriteOnly)) {
        f3.write(QByteArray(1024 * 50, 'A')); // 50 KB
        f3.close();
    }

    // External file to import
    QFile fExternal(m_externalDir + QStringLiteral("/scene.prompt"));
    if (fExternal.open(QIODevice::Text|QIODevice::WriteOnly)) {
        fExternal.write("An island with a city on it in the middle of the ocean.");
        fExternal.close();
    }
}

void Test_ScriptsDirectoryModel::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_ScriptsDirectoryModel::testInitialEmptyState()
{
    ScriptsDirectoryModel model;
    QVERIFY2(model.rowCount() == 0, "State Corruption: ScriptsDirectoryModel did not initialize empty.");
    QCOMPARE(model.projectPath(), QString());
}

void Test_ScriptsDirectoryModel::testScanningScriptsDirectory()
{
    ScriptsDirectoryModel model;
    QSignalSpy pathSpy(&model, &ScriptsDirectoryModel::projectPathChanged);
    QSignalSpy countSpy(&model, &ScriptsDirectoryModel::countChanged);

    model.setProjectPath(m_testSessionDir);

    QCOMPARE(pathSpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);
    // Should discover ep1.md and story.txt, ignoring theme.wav
    QCOMPARE(model.rowCount(), 2);
}

void Test_ScriptsDirectoryModel::testFileRolesAndData()
{
    ScriptsDirectoryModel model;
    model.setProjectPath(m_testSessionDir);

    QModelIndex idx = model.index(0, 0);
    QVERIFY(idx.isValid());

    QString fileName = model.data(idx, ScriptsDirectoryModel::FileNameRole).toString();
    QVERIFY2(fileName == QStringLiteral("ep1.md") || fileName == QStringLiteral("story.txt"),
             "Scanning Failure: Model retrieved unexpected file name or failed filter match.");

    QString filePath = model.data(idx, ScriptsDirectoryModel::FilePathRole).toString();
    QVERIFY(filePath.startsWith(QStringLiteral("file:")));

    QString sizeFormatted = model.data(idx, ScriptsDirectoryModel::FileSizeRole).toString();
    QVERIFY(sizeFormatted.endsWith(QStringLiteral("KB")) || sizeFormatted.endsWith(QStringLiteral("MB")));
}

void Test_ScriptsDirectoryModel::testAssetImportingAndDeduplication()
{
    ScriptsDirectoryModel model;
    model.setProjectPath(m_testSessionDir);

    int initialCount = model.rowCount();

    QList<QUrl> importUrls;
    importUrls.append(QUrl::fromLocalFile(m_externalDir + QStringLiteral("/scene.prompt")));

    QSignalSpy importSpy(&model, &ScriptsDirectoryModel::assetsImported);
    int imported = model.importAssets(importUrls);

    QVERIFY2(imported == 1, "File I/O Failure: Model failed to import external script asset.");
    QCOMPARE(importSpy.count(), 1);
    QCOMPARE(model.rowCount(), initialCount + 1);

    // Test Deduplication: Re-importing same file must produce _copy
    int importedAgain = model.importAssets(importUrls);
    QCOMPARE(importedAgain, 1);
    QCOMPARE(model.rowCount(), initialCount + 2);

    QDir targetDir(m_scriptsDir);
    QVERIFY2(targetDir.exists(QStringLiteral("scene_copy.prompt")),
             "Deduplication Failure: Re-imported file did not receive the '_copy' suffix.");
}

QTEST_MAIN(Test_ScriptsDirectoryModel)
#include "test_ScriptsDirectoryModel.moc"
