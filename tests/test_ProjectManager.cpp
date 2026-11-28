// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSignalSpy>
#include "app/core/projectmanager.h"

class Test_ProjectManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Lifecycle & Scaffolding
    void testInitialStateClosed();
    void testCreateProjectScaffoldingAndPrompts();
    void testOpenProjectReadsMetadata();
    void testCloseProjectClearsProperties();
    void testCreateAndOpenProjectWithEmptyEmail();

    // Security & Path Traversal Guards
    void testSecurityRejectsPathTraversalOnCreate();
    void testSecurityRejectsPathTraversalOnOpen();
    void testSecurityRejectsAssetDeletionOutsideAssets();

    // Data Persistence & Assets
    void testSaveCurrentProjectDataUpdatesTimestamp();
    void testSaveCandidateImageDecodesBase64();
    void testSaveAudioAndVideoFiles();
    void testDeleteAssetRemovesFromDisk();
    void testSaveCandidateImageSupportsNonPngDataUris();

private:
    QString m_testSessionDir;
    QString createScopedDir();
};

void Test_ProjectManager::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    // StandardLocations::TempLocation/<uuid>/ conforming to project guidelines
    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);

    QDir().mkpath(m_testSessionDir);
}

void Test_ProjectManager::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_ProjectManager::init()
{
}

void Test_ProjectManager::cleanup()
{
}

QString Test_ProjectManager::createScopedDir()
{
    QString subUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString dir = QDir::toNativeSeparators(m_testSessionDir + "/" + subUuid);
    QDir().mkpath(dir);
    return dir;
}

void Test_ProjectManager::testInitialStateClosed()
{
    ProjectManager pm;
    QVERIFY2(!pm.isProjectOpen(), "ProjectManager reported a project was open immediately after construction.");
    QVERIFY2(pm.currentProjectPath().isEmpty(), "ProjectManager initialized with a non-empty project path.");
    QVERIFY2(pm.projectName().isEmpty(), "ProjectManager initialized with a non-empty project name.");
}

void Test_ProjectManager::testCreateProjectScaffoldingAndPrompts()
{
    QString root = createScopedDir();
    ProjectManager pm(root);

    QString projPath = QDir::toNativeSeparators(root + "/Episode_01");

    bool created = pm.createProject(
        projPath,
        QStringLiteral("Episode One"),
        QStringLiteral("Full screenplay narrative"),
        QStringLiteral("Pilot teaser"),
        QStringLiteral("Mike Ramirez"),
        QStringList{QStringLiteral("CoWriter A")},
        QStringLiteral("me@home.com"),
        QStringLiteral("1.0")
        );

    QVERIFY(created);
    QVERIFY(pm.isProjectOpen());


    QVERIFY2(created, "createProject() returned false, failing to scaffold the project.");
    QVERIFY2(pm.isProjectOpen(), "createProject() succeeded but internal state did not register as open.");
    QCOMPARE(pm.projectName(), QStringLiteral("Episode One"));
    QCOMPARE(pm.author(), QStringLiteral("Mike Ramirez"));

    QVERIFY2(QDir(projPath + "/assets/scripts").exists(), "Scaffolding failure: scripts directory not created.");
    QVERIFY2(QDir(projPath + "/assets/images").exists(), "Scaffolding failure: images directory not created.");
    QVERIFY2(QDir(projPath + "/assets/audio").exists(), "Scaffolding failure: audio directory not created.");
    QVERIFY2(QDir(projPath + "/assets/video").exists(), "Scaffolding failure: video directory not created.");



    // Verify configuration file
    QVERIFY2(QFile::exists(projPath + "/Episode_01.conf"), "Configuration file was not written to disk.");
}

void Test_ProjectManager::testOpenProjectReadsMetadata()
{
    QString root = createScopedDir();
    QString projPath = QDir::toNativeSeparators(root + "/OpenTestProject");

    {
        ProjectManager creator(root);
        creator.createProject(
            projPath,
            QStringLiteral("Saved Story"),
            QStringLiteral("Long Desc"),
            QStringLiteral("Short Desc"),
            QStringLiteral("Lead Author"),
            QStringList{QStringLiteral("Assistant 1"), QStringLiteral("Assistant 2")},
            QStringLiteral("author@test.com"),
            QStringLiteral("2.1")
            );
        creator.closeProject();
    }

    ProjectManager opener(root);
    bool opened = opener.openProject(projPath);
    QVERIFY(opened);
    QVERIFY(opener.isProjectOpen());
    QCOMPARE(opener.projectName(), QStringLiteral("Saved Story"));
    QCOMPARE(opener.projectDescription(), QStringLiteral("Long Desc"));
    QCOMPARE(opener.projectShortDescription(), QStringLiteral("Short Desc"));
    QCOMPARE(opener.author(), QStringLiteral("Lead Author"));
    QCOMPARE(opener.authors().size(), 2);
    QCOMPARE(opener.email(), QStringLiteral("author@test.com"));
    QCOMPARE(opener.projectVersion(), QStringLiteral("2.1"));
}

void Test_ProjectManager::testCloseProjectClearsProperties()
{
    QString root = createScopedDir();
    ProjectManager pm(root);
    pm.createProject(root + "/CloseTest", QStringLiteral("Closing Show"), QStringLiteral("D"), QStringLiteral("S"), QStringLiteral("A"), QStringList(), QStringLiteral("e@m.com"));
    QVERIFY(pm.isProjectOpen());

    pm.closeProject();
    QVERIFY(!pm.isProjectOpen());
    QVERIFY(pm.currentProjectPath().isEmpty());
    QVERIFY(pm.projectName().isEmpty());
    QVERIFY(pm.author().isEmpty());
}

void Test_ProjectManager::testSecurityRejectsPathTraversalOnCreate()
{
    ProjectManager pm;
    QSignalSpy errorSpy(&pm, &ProjectManager::projectError);

    bool result = pm.createProject(
        QStringLiteral("C:/Path/../OutsideDir"),
        QStringLiteral("Infiltration"),
        QStringLiteral("D"),
        QStringLiteral("S"),
        QStringLiteral("A"),
        QStringList(),
        QStringLiteral("e@m.com")
        );

    QVERIFY2(!result, "Security Vulnerability: createProject allowed a path traversal attempt outside the root.");
    QCOMPARE(errorSpy.count(), 1);
}

void Test_ProjectManager::testSecurityRejectsPathTraversalOnOpen()
{
    ProjectManager pm;
    QSignalSpy errorSpy(&pm, &ProjectManager::projectError);

    bool result = pm.openProject(QStringLiteral("../../../etc/shadow"));
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void Test_ProjectManager::testSecurityRejectsAssetDeletionOutsideAssets()
{
    QString root = createScopedDir();
    ProjectManager pm(root);
    QString projPath = root + "/SecureProject";
    pm.createProject(projPath, QStringLiteral("Sec"), QStringLiteral("D"), QStringLiteral("S"), QStringLiteral("A"), QStringList(), QStringLiteral("e@m.com"));

    // File located outside the assets/ folder
    QString forbiddenFile = projPath + "/SecureProject.conf";
    QVERIFY(QFile::exists(forbiddenFile));

    QSignalSpy errorSpy(&pm, &ProjectManager::projectError);
    bool deleted = pm.deleteAsset(forbiddenFile);

    QVERIFY2(!deleted, "Security Vulnerability: deleteAsset successfully deleted a critical project file outside the assets boundary.");
    QVERIFY2(QFile::exists(forbiddenFile), "Security Vulnerability: The forbidden file was physically removed from the disk.");
    QCOMPARE(errorSpy.count(), 1);
}

void Test_ProjectManager::testSaveCurrentProjectDataUpdatesTimestamp()
{
    QString root = createScopedDir();
    ProjectManager pm(root);
    pm.createProject(root + "/DataTest", QStringLiteral("Data Story"), QStringLiteral("D"), QStringLiteral("S"), QStringLiteral("A"), QStringList(), QStringLiteral("e@m.com"));

    QString oldModified = pm.dateLastEdited();

    // Minor delay to guarantee millisecond delta
    QTest::qWait(20);

    bool saved = pm.saveCurrentProjectData(QStringLiteral("assets/script"), QStringLiteral("act1.txt"), QStringLiteral("SCENE 1: START"));
    QVERIFY(saved);

    QFile file(root + "/DataTest/assets/script/act1.txt");
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(file.readAll(), QByteArray("SCENE 1: START"));
    file.close();

    QVERIFY(pm.dateLastEdited() != oldModified);
}

void Test_ProjectManager::testSaveCandidateImageDecodesBase64()
{
    QString root = createScopedDir();
    ProjectManager pm(root);
    pm.createProject(root + "/ImageTest", QStringLiteral("Img Story"), QStringLiteral("D"), QStringLiteral("S"), QStringLiteral("A"), QStringList(), QStringLiteral("e@m.com"));

    // 1x1 pixel PNG transparent byte sequence encoded in base64
    QString base64Png = QStringLiteral("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=");

    bool saved = pm.saveCandidateImage(base64Png, QStringLiteral("test_frame"));
    QVERIFY2(saved, "saveCandidateImage() failed to process or save the base64 payload.");


    QString savedPath = root + "/ImageTest/assets/images/test_frame.png";
    QVERIFY2(QFile::exists(savedPath), "Base64 image was processed but not written to the correct assets/images path.");
    QVERIFY(QFileInfo(savedPath).size() > 0);
}

void Test_ProjectManager::testSaveAudioAndVideoFiles()
{
    QString root = createScopedDir();
    ProjectManager pm(root);
    pm.createProject(root + "/MediaTest", QStringLiteral("Media Story"), QStringLiteral("D"), QStringLiteral("S"), QStringLiteral("A"), QStringList(), QStringLiteral("e@m.com"));

    QByteArray fakeAudio = "RIFF....WAVEfmt ";
    QByteArray fakeVideo = "....ftypmp42";

    QVERIFY(pm.saveAudioFile(fakeAudio, QStringLiteral("voiceover")));
    QVERIFY(pm.saveVideoFile(fakeVideo, QStringLiteral("scene_render")));

    QVERIFY(QFile::exists(root + "/MediaTest/assets/audio/voiceover.wav"));
    QVERIFY(QFile::exists(root + "/MediaTest/assets/video/scene_render.mp4"));
}

void Test_ProjectManager::testDeleteAssetRemovesFromDisk()
{
    QString root = createScopedDir();
    ProjectManager pm(root);
    pm.createProject(root + "/DelTest", QStringLiteral("Del Story"), QStringLiteral("D"), QStringLiteral("S"), QStringLiteral("A"), QStringList(), QStringLiteral("e@m.com"));

    pm.saveAudioFile(QByteArray("fake audio"), QStringLiteral("temp_sfx"));
    QString audioPath = root + "/DelTest/assets/audio/temp_sfx.wav";
    QVERIFY(QFile::exists(audioPath));

    QSignalSpy spy(&pm, &ProjectManager::assetDeleted);
    bool deleted = pm.deleteAsset(audioPath);

    QVERIFY(deleted);
    QCOMPARE(spy.count(), 1);
    QVERIFY(!QFile::exists(audioPath));
}

void Test_ProjectManager::testCreateAndOpenProjectWithEmptyEmail()
{
    QString root = createScopedDir();
    QString projPath = QDir::toNativeSeparators(root + "/NoEmailEpisode");

    {
        ProjectManager creator(root);
        bool created = creator.createProject(
            projPath,
            QStringLiteral("No Email Episode"),
            QStringLiteral("Desc"),
            QStringLiteral("Short"),
            QStringLiteral("Mike Ramirez"),
            QStringList(),
            QString(), // Empty email
            QStringLiteral("1.0")
            );
        QVERIFY2(created, "createProject() failed when email was left empty.");
        QCOMPARE(creator.email(), QString());
        creator.closeProject();
    }

    ProjectManager opener(root);
    QVERIFY2(opener.openProject(projPath), "openProject() failed for project created without email.");
    QCOMPARE(opener.email(), QString());
}

void Test_ProjectManager::testSaveCandidateImageSupportsNonPngDataUris()
{
    QString root = createScopedDir();
    ProjectManager pm(root);
    pm.createProject(root + "/NonPngImageTest", QStringLiteral("NonPng Story"), QStringLiteral("D"), QStringLiteral("S"), QStringLiteral("A"), QStringList(), QStringLiteral("e@m.com"));

    // Generate a valid 4x4 JPEG in memory and wrap it in a data URI
    QImage sample(4, 4, QImage::Format_RGB32);
    sample.fill(Qt::cyan);

    QByteArray jpegBytes;
    QBuffer buffer(&jpegBytes);
    QVERIFY(buffer.open(QIODevice::WriteOnly));
    QVERIFY(sample.save(&buffer, "JPEG", 90));

    const QString jpegDataUri = QStringLiteral("data:image/jpeg;base64,")
                                + QString::fromLatin1(jpegBytes.toBase64());

    QVERIFY2(pm.saveCandidateImage(jpegDataUri, QStringLiteral("remote_candidate")),
             "saveCandidateImage() failed to decode and save a JPEG data URI.");

    const QString outPath = root + QStringLiteral("/NonPngImageTest/assets/images/remote_candidate.png");
    QVERIFY2(QFile::exists(outPath), "Transcoded PNG was not written to assets/images/remote_candidate.png.");

    QImage reloaded(outPath);
    QVERIFY(!reloaded.isNull());
    QCOMPARE(reloaded.size(), QSize(4, 4));
}

QTEST_MAIN(Test_ProjectManager)
#include "test_ProjectManager.moc"

