/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_ImageDirectoryModel.cpp
** Unit test suite for ImageDirectoryModel under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QImage>
#include <QStandardPaths>
#include <QUuid>
#include "app/workspace/imagedirectorymodel.h"

class Test_ImageDirectoryModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testEmptyState();
    void testScanningImagesAndResolution();
    void testRoleValues();
    void testImportImage();

private:
    QString m_testSessionDir;
    QString m_imagesDir;
    QString m_externalDir;
};

void Test_ImageDirectoryModel::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);

    m_imagesDir = m_testSessionDir + QStringLiteral("/assets/images");
    m_externalDir = m_testSessionDir + QStringLiteral("/external");

    QDir().mkpath(m_imagesDir);
    QDir().mkpath(m_externalDir);

    // Create a 512x256 test PNG image
    QImage testImg(512, 256, QImage::Format_RGB32);
    testImg.fill(Qt::blue);
    testImg.save(m_imagesDir + QStringLiteral("/shot_001.png"), "PNG");

    // Create an external image for import testing
    QImage extImg(640, 480, QImage::Format_RGB32);
    extImg.fill(Qt::darkGreen);
    extImg.save(m_externalDir + QStringLiteral("/reference.jpg"), "JPG");
}

void Test_ImageDirectoryModel::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_ImageDirectoryModel::testEmptyState()
{
    ImageDirectoryModel model;
    QVERIFY2(model.rowCount() == 0, "State Corruption: ImageDirectoryModel did not initialize empty.");
    QCOMPARE(model.projectPath(), QString());
}

void Test_ImageDirectoryModel::testScanningImagesAndResolution()
{
    ImageDirectoryModel model;
    model.setProjectPath(m_testSessionDir);

    QCOMPARE(model.rowCount(), 1);

    QModelIndex idx = model.index(0, 0);
    QVERIFY(idx.isValid());

    QString fileName = model.data(idx, AbstractAssetDirectoryModel::FileNameRole).toString();
    QVERIFY2(fileName == QStringLiteral("shot_001.png"), "Scanning Failure: Model retrieved incorrect image file name.");

    QString resolution = model.data(idx, ImageDirectoryModel::ResolutionRole).toString();
    QCOMPARE(resolution, QStringLiteral("512 x 256"));

    QString format = model.data(idx, ImageDirectoryModel::FormatRole).toString();
    QCOMPARE(format, QStringLiteral("PNG"));
}

void Test_ImageDirectoryModel::testRoleValues()
{
    ImageDirectoryModel model;
    model.setProjectPath(m_testSessionDir);

    QHash<int, QByteArray> roles = model.roleNames();
    QVERIFY(roles.contains(ImageDirectoryModel::ResolutionRole));
    QVERIFY(roles.contains(ImageDirectoryModel::FormatRole));
    QVERIFY(roles.contains(ImageDirectoryModel::ColorSpaceRole));
}

void Test_ImageDirectoryModel::testImportImage()
{
    ImageDirectoryModel model;
    model.setProjectPath(m_testSessionDir);
    int initialCount = model.rowCount();

    QList<QUrl> urls;
    urls.append(QUrl::fromLocalFile(m_externalDir + QStringLiteral("/reference.jpg")));

    int imported = model.importAssets(urls);
    QVERIFY2(imported == 1, "File I/O Failure: Model failed to import external image asset.");
    QCOMPARE(model.rowCount(), initialCount + 1);

    QDir targetDir(m_imagesDir);
    QVERIFY2(targetDir.exists(QStringLiteral("reference.jpg")), "File I/O Failure: Imported image not found in target directory.");
}

QTEST_MAIN(Test_ImageDirectoryModel)
#include "test_ImageDirectoryModel.moc"

