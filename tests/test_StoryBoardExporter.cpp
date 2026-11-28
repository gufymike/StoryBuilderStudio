// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QUuid>

#include "app/storyboard/storyboardmanager.h"
#include "app/storyboard/storyboardexporter.h"

class Test_StoryBoardExporter : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testRejectsNullOrEmptyBoard();
    void testExportToPdfSingleShotPerPage();
    void testExportToPptxPackageAndEmbeddedMedia();

private:
    QString m_testSessionDir;
    QString m_episodeDir;
};

void Test_StoryBoardExporter::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));

    const QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + QStringLiteral("/") + sessionUuid);
    m_episodeDir = m_testSessionDir + QStringLiteral("/Episode_01");

    QDir().mkpath(m_episodeDir + QStringLiteral("/assets/images"));
    QDir().mkpath(m_episodeDir + QStringLiteral("/assets/scripts"));
    QDir().mkpath(m_episodeDir + QStringLiteral("/assets/audio"));
    QDir().mkpath(m_episodeDir + QStringLiteral("/assets/video"));

    // Create sample keyframe PNG and JPEG images
    QImage frame1(320, 180, QImage::Format_RGB32);
    frame1.fill(QColor(QStringLiteral("#00B4F0")));
    QVERIFY(frame1.save(m_episodeDir + QStringLiteral("/assets/images/shot01.png"), "PNG"));

    QImage frame2(320, 180, QImage::Format_RGB32);
    frame2.fill(QColor(QStringLiteral("#132B3A")));
    QVERIFY(frame2.save(m_episodeDir + QStringLiteral("/assets/images/shot02.jpg"), "JPEG", 90));

    // Create sample script file
    QFile scriptFile(m_episodeDir + QStringLiteral("/assets/scripts/scene01.md"));
    QVERIFY(scriptFile.open(QIODevice::WriteOnly | QIODevice::Text));
    scriptFile.write("EXT. ORBITAL DOCK - NIGHT\nThe Valkyrie launches into the starfield.\n");
    scriptFile.close();
}

void Test_StoryBoardExporter::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_StoryBoardExporter::testRejectsNullOrEmptyBoard()
{
    StoryBoardExporter exporter;
    QSignalSpy failSpy(&exporter, &StoryBoardExporter::exportFailed);

    const QString outPdf = m_testSessionDir + QStringLiteral("/empty.pdf");
    QVERIFY(!exporter.exportToPdf(outPdf, nullptr));
    QCOMPARE(failSpy.count(), 1);

    StoryBoardManager emptyMgr;
    emptyMgr.setEpisodePath(m_testSessionDir + QStringLiteral("/EmptyEp"));
    QVERIFY(!exporter.exportToPptx(m_testSessionDir + QStringLiteral("/empty.pptx"), &emptyMgr));
    QCOMPARE(failSpy.count(), 2);
}

void Test_StoryBoardExporter::testExportToPdfSingleShotPerPage()
{
    StoryBoardManager manager;
    manager.setEpisodePath(m_episodeDir);

    QVERIFY(manager.addShot(QStringLiteral("Shot 01 - Launch Bay")));
    QVERIFY(manager.addShot(QStringLiteral("Shot 02 - Nebula Jump")));

    const QString img1 = m_episodeDir + QStringLiteral("/assets/images/shot01.png");
    const QString script1 = m_episodeDir + QStringLiteral("/assets/scripts/scene01.md");
    const QString finalVid = m_episodeDir + QStringLiteral("/assets/video/shot01_final.mp4");

    QVERIFY(manager.addAssetToShot(0, QStringLiteral("images"), img1));
    QVERIFY(manager.setShotScript(0, script1));
    QVERIFY(manager.setShotFinalCut(0, finalVid));

    StoryBoardExporter exporter;
    QSignalSpy successSpy(&exporter, &StoryBoardExporter::exportSucceeded);

    // Intentionally omit .pdf extension to verify automatic normalization
    const QString rawTarget = m_testSessionDir + QStringLiteral("/storyboard_export");
    QVERIFY(exporter.exportToPdf(rawTarget, &manager, QStringLiteral("Episode 01"), QStringLiteral("Mike Ramirez")));

    QCOMPARE(successSpy.count(), 1);
    const QString savedPdf = successSpy.takeFirst().at(0).toString();
    QVERIFY(savedPdf.endsWith(QStringLiteral(".pdf")));
    QVERIFY(QFile::exists(savedPdf));

    QFile pdfFile(savedPdf);
    QVERIFY(pdfFile.open(QIODevice::ReadOnly));
    const QByteArray bytes = pdfFile.readAll();
    pdfFile.close();

    QVERIFY(bytes.startsWith("%PDF-"));
    QVERIFY(bytes.size() > 2048);
}

void Test_StoryBoardExporter::testExportToPptxPackageAndEmbeddedMedia()
{
    StoryBoardManager manager;
    manager.setEpisodePath(m_episodeDir);
    QCOMPARE(manager.shotCount(), 2);

    const QString img2 = m_episodeDir + QStringLiteral("/assets/images/shot02.jpg");
    QVERIFY(manager.addAssetToShot(1, QStringLiteral("images"), img2));

    StoryBoardExporter exporter;
    QSignalSpy successSpy(&exporter, &StoryBoardExporter::exportSucceeded);

    const QString pptxTarget = m_testSessionDir + QStringLiteral("/storyboard_deck.pptx");
    QVERIFY(exporter.exportToPptx(pptxTarget, &manager, QStringLiteral("Episode 01"), QStringLiteral("Mike Ramirez")));

    QCOMPARE(successSpy.count(), 1);
    QVERIFY(QFile::exists(pptxTarget));

    QFile pptxFile(pptxTarget);
    QVERIFY(pptxFile.open(QIODevice::ReadOnly));
    const QByteArray zipBytes = pptxFile.readAll();
    pptxFile.close();

    // Verify PKZIP local file header signature and OPC internal parts
    QVERIFY(zipBytes.startsWith("PK\x03\x04"));
    QVERIFY(zipBytes.contains("[Content_Types].xml"));
    QVERIFY(zipBytes.contains("ppt/presentation.xml"));
    QVERIFY(zipBytes.contains("ppt/slides/slide1.xml"));
    QVERIFY(zipBytes.contains("ppt/slides/slide2.xml"));
    QVERIFY(zipBytes.contains("ppt/slides/slide3.xml"));
    QVERIFY(zipBytes.contains("ppt/media/image2.png"));
    QVERIFY(zipBytes.contains("ppt/media/image3.png"));
}

QTEST_MAIN(Test_StoryBoardExporter)
#include "test_StoryBoardExporter.moc"
