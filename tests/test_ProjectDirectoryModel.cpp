// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUuid>
#include "app/core/projectdirectorymodel.h"

class Test_ProjectDirectoryModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testEmptyPathHandling();
    void testScanningCategoriesAndFiles();
    void testFileContentLoading();
    void testFallbackToTxtExtension();

private:
    QString m_testSessionDir;
    QString m_promptsDir;
};

void Test_ProjectDirectoryModel::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);

    m_promptsDir = m_testSessionDir + QStringLiteral("/prompts");
    QDir().mkpath(m_promptsDir + QStringLiteral("/lighting"));
    QDir().mkpath(m_promptsDir + QStringLiteral("/camera"));

    // Write sample files
    QFile file1(m_promptsDir + QStringLiteral("/lighting/daylight.prompt"));
    if (file1.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file1.write("Natural golden hour daylight, soft diffusion");
        file1.close();
    }

    QFile file2(m_promptsDir + QStringLiteral("/camera/wide_angle.txt"));
    if (file2.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file2.write("24mm cinematic wide shot, anamorphic lens");
        file2.close();
    }
}

void Test_ProjectDirectoryModel::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_ProjectDirectoryModel::testEmptyPathHandling()
{
    ProjectDirectoryModel model;
    QSignalSpy spy(&model, &ProjectDirectoryModel::categoriesChanged);

    model.setProjectPath(QString());
    QCOMPARE(model.categories().size(), 0);
    QVERIFY2(model.getFilesForCategory(QStringLiteral("lighting")).isEmpty(), "getFilesForCategory did not return empty for an uninitialized path.");
    QVERIFY2(model.loadPromptFileContent(QStringLiteral("lighting"), QStringLiteral("daylight")).isEmpty(), "loadPromptFileContent did not return empty for an uninitialized path.");
}

void Test_ProjectDirectoryModel::testScanningCategoriesAndFiles()
{
    ProjectDirectoryModel model;
    QSignalSpy spy(&model, &ProjectDirectoryModel::categoriesChanged);

    model.setProjectPath(m_testSessionDir);
    QCOMPARE(spy.count(), 1);

    QStringList categories = model.categories();
    QCOMPARE(categories.size(), 2);
    QVERIFY2(categories.contains(QStringLiteral("camera")), "Model failed to detect 'camera' directory category.");
    QVERIFY2(categories.contains(QStringLiteral("lighting")), "Model failed to detect 'lighting' directory category.");


    QStringList lightingFiles = model.getFilesForCategory(QStringLiteral("lighting"));
    QCOMPARE(lightingFiles.size(), 1);
    QVERIFY2(lightingFiles.contains(QStringLiteral("daylight")), "Model failed to detext 'daylight' file for 'lighting' category.");
}

void Test_ProjectDirectoryModel::testFileContentLoading()
{
    ProjectDirectoryModel model;
    model.setProjectPath(m_testSessionDir);

    QString content = model.loadPromptFileContent(QStringLiteral("lighting"), QStringLiteral("daylight"));
    QCOMPARE(content, QStringLiteral("Natural golden hour daylight, soft diffusion"));
}

void Test_ProjectDirectoryModel::testFallbackToTxtExtension()
{
    ProjectDirectoryModel model;
    model.setProjectPath(m_testSessionDir);

    QString content = model.loadPromptFileContent(QStringLiteral("camera"), QStringLiteral("wide_angle"));
    QCOMPARE(content, QStringLiteral("24mm cinematic wide shot, anamorphic lens"));
}

QTEST_MAIN(Test_ProjectDirectoryModel)
#include "test_ProjectDirectoryModel.moc"

