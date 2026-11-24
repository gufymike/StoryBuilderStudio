// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "app/core/seriescontainer.h"

class Test_SeriesContainer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Container Creation and Lifecycle
    void testInitialEmptyState();
    void testCreateNewContainerCreatesEpisodesDirAndManifest();
    void testLoadFromDirectoryValid();
    void testLoadFromEmptyPathClearsState();
    void testCreateAndLoadHandleFileUrls();

    // SubProject Operations
    void testCreateNewSubProjectScaffolding();
    void testRemoveSubProjectReindexesDisplayOrder();
    void testAddExistingSubProjectDetectsDuplicates();
    void testDeleteSeriesDirectoryAndSafetyGuards();

    // Metadata & Signals
    void testCreatorAndCoCreatorsSignals();

private:
    QString m_testSessionDir;
    QString createScopedDir();
};

void Test_SeriesContainer::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));
    QStandardPaths::setTestModeEnabled(true);
    // StandardLocations::TempLocation/<uuid>/
    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);

    QDir().mkpath(m_testSessionDir);
}

void Test_SeriesContainer::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
    QStandardPaths::setTestModeEnabled(false);
}

void Test_SeriesContainer::init()
{
}

void Test_SeriesContainer::cleanup()
{
}

QString Test_SeriesContainer::createScopedDir()
{
    QString subUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString dir = QDir::toNativeSeparators(m_testSessionDir + "/" + subUuid);
    QDir().mkpath(dir);
    return dir;
}

void Test_SeriesContainer::testInitialEmptyState()
{
    SeriesContainer container;
    QVERIFY2(container.name().isEmpty(), "SeriesContainer initialized with a non-empty name state.");
    QVERIFY2(container.rootDirectory().isEmpty(), "SeriesContainer initialized with a non-empty root directory.");
    QVERIFY2(container.episodesDirectory().isEmpty(), "SeriesContainer episodes directory did not return empty when root is unset.");
    QCOMPARE(container.subProjectCount(), 0);
}

void Test_SeriesContainer::testCreateNewContainerCreatesEpisodesDirAndManifest()
{
    SeriesContainer container;
    QString seriesPath = createScopedDir() + "/Season1";

    QSignalSpy rootSpy(&container, &SeriesContainer::rootDirectoryChanged);
    QSignalSpy nameSpy(&container, &SeriesContainer::nameChanged);

    bool created = container.createNewContainer(
        seriesPath,
        QStringLiteral("Galactic Frontier"),
        QStringLiteral("Sci-Fi Animated Anthology"),
        QStringLiteral("A deep exploration of distant star outposts.")
        );

    QVERIFY2(created, "createNewContainer() returned false, failing to scaffold the series.");

    QCOMPARE(rootSpy.count(), 1);
    QCOMPARE(nameSpy.count(), 1);
    QCOMPARE(container.name(), QStringLiteral("Galactic Frontier"));

    // Verify disk files
    QVERIFY2(QDir(container.episodesDirectory()).exists(), "Scaffolding failure: episodes/ subdirectory was not created on disk.");
    QVERIFY2(QFile::exists(seriesPath + "/season_manifest.json"), "Scaffolding failure: season_manifest.json was not written to disk.");
}

void Test_SeriesContainer::testLoadFromDirectoryValid()
{
    QString seriesPath = createScopedDir() + "/SavedSeries";
    {
        SeriesContainer creator;
        creator.createNewContainer(seriesPath, QStringLiteral("Echoes"), QStringLiteral("Short"), QStringLiteral("Long"));
        creator.setCreator(QStringLiteral("Mike Ramirez"));
        creator.setEmail(QStringLiteral("me@home.com"));
        creator.setCoCreators(QStringList{QStringLiteral("Lucie"), QStringLiteral("Alex")});
        creator.saveManifest();
    }

    SeriesContainer loader;
    bool loaded = loader.loadFromDirectory(seriesPath);
    QVERIFY(loaded);
    QCOMPARE(loader.name(), QStringLiteral("Echoes"));
    QCOMPARE(loader.creator(), QStringLiteral("Mike Ramirez"));
    QCOMPARE(loader.email(), QStringLiteral("me@home.com"));
    QCOMPARE(loader.coCreators().size(), 2);
}

void Test_SeriesContainer::testLoadFromEmptyPathClearsState()
{
    SeriesContainer container;
    QString seriesPath = createScopedDir() + "/TempSeries";
    container.createNewContainer(seriesPath, QStringLiteral("Active"), QStringLiteral("S"), QStringLiteral("L"));
    QVERIFY(!container.name().isEmpty());

    container.loadFromDirectory(QString());
    QVERIFY2(container.name().isEmpty(), "loadFromDirectory(empty) failed to clear the internal series name.");
    QVERIFY2(container.rootDirectory().isEmpty(), "loadFromDirectory(empty) failed to clear the root directory.");
    QCOMPARE(container.subProjectCount(), 0);
}

void Test_SeriesContainer::testCreateNewSubProjectScaffolding()
{
    SeriesContainer container;
    QString seriesPath = createScopedDir() + "/SubProjTest";
    container.createNewContainer(seriesPath, QStringLiteral("Show"), QStringLiteral("S"), QStringLiteral("L"));

    QSignalSpy spy(&container, &SeriesContainer::subProjectsChanged);

    QString episodePath = container.createNewSubProject(QStringLiteral("ep01_pilot"), QStringLiteral("Pilot Episode"));
    QVERIFY(!episodePath.isEmpty());
    QCOMPARE(container.subProjectCount(), 1);
    QCOMPARE(spy.count(), 1);

    // Verify subproject scaffolded assets directories on disk
    QVERIFY2(!episodePath.isEmpty(), "createNewSubProject returned an empty path string.");
    QVERIFY2(QDir(episodePath + "/assets/images").exists(), "Sub-project scaffolding failed: assets/images directory missing.");
    QVERIFY2(QDir(episodePath + "/assets/audio").exists(), "Sub-project scaffolding failed: assets/audio directory missing.");
    QVERIFY2(QDir(episodePath + "/assets/video").exists(), "Sub-project scaffolding failed: assets/video directory missing.");
    QVERIFY2(QFile::exists(episodePath + "/project.meta"), "Sub-project scaffolding failed: project.meta not written.");
}

void Test_SeriesContainer::testRemoveSubProjectReindexesDisplayOrder()
{
    SeriesContainer container;
    QString seriesPath = createScopedDir() + "/ReindexTest";
    container.createNewContainer(seriesPath, QStringLiteral("Reindexer"), QStringLiteral("S"), QStringLiteral("L"));

    container.createNewSubProject(QStringLiteral("ep01"), QStringLiteral("Episode 1"));
    container.createNewSubProject(QStringLiteral("ep02"), QStringLiteral("Episode 2"));
    container.createNewSubProject(QStringLiteral("ep03"), QStringLiteral("Episode 3"));

    QCOMPARE(container.subProjectCount(), 3);
    QCOMPARE(container.getSubProjectTitle(0), QStringLiteral("Episode 1"));

    // Remove middle episode
    container.removeSubProject(1);
    QCOMPARE(container.subProjectCount(), 2);
    QCOMPARE(container.getSubProjectTitle(1), QStringLiteral("Episode 3"));
}

void Test_SeriesContainer::testAddExistingSubProjectDetectsDuplicates()
{
    SeriesContainer container;
    QString seriesPath = createScopedDir() + "/DuplicateTest";
    container.createNewContainer(seriesPath, QStringLiteral("Dup"), QStringLiteral("S"), QStringLiteral("L"));

    QString childPath = createScopedDir() + "/ExternalEpisode";
    QDir().mkpath(childPath);

    QVERIFY(container.addExistingSubProject(childPath));
    QCOMPARE(container.subProjectCount(), 1);

    // Adding same sub-project a second time must fail and emit manifestError
    QSignalSpy errorSpy(&container, &SeriesContainer::manifestError);
    bool addedSecond = container.addExistingSubProject(childPath);
    QVERIFY2(!addedSecond, "Duplicate Check Failure: The container allowed the exact same sub-project to be added twice.");
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(container.subProjectCount(), 1);
}

void Test_SeriesContainer::testCreatorAndCoCreatorsSignals()
{
    SeriesContainer container;
    QSignalSpy creatorSpy(&container, &SeriesContainer::creatorChanged);
    QSignalSpy coCreatorsSpy(&container, &SeriesContainer::coCreatorsChanged);
    QSignalSpy emailSpy(&container, &SeriesContainer::emailChanged);

    container.setCreator(QStringLiteral("Mike Ramirez"));
    container.setEmail(QStringLiteral("me@home.com"));
    container.setCoCreators(QStringList{QStringLiteral("Writer A"), QStringLiteral("Writer B")});

    QCOMPARE(creatorSpy.count(), 1);
    QCOMPARE(emailSpy.count(), 1);
    QCOMPARE(coCreatorsSpy.count(), 1);
}

void Test_SeriesContainer::testCreateAndLoadHandleFileUrls()
{
    SeriesContainer container;
    const QString expectedPath = QDir::cleanPath(createScopedDir() + "/UrlSeries");
    const QString fileUrl = QUrl::fromLocalFile(expectedPath).toString();

    QVERIFY2(container.createNewContainer(fileUrl, QStringLiteral("URL Show"), QStringLiteral("S"), QStringLiteral("L")),
             "createNewContainer() failed when given a file:// URL.");
    QCOMPARE(QDir::cleanPath(container.rootDirectory()), expectedPath);
    QVERIFY(QFile::exists(expectedPath + "/season_manifest.json"));

    SeriesContainer loader;
    QVERIFY2(loader.loadFromDirectory(fileUrl), "loadFromDirectory() failed when given a file:// URL.");
    QCOMPARE(QDir::cleanPath(loader.rootDirectory()), expectedPath);
}

void Test_SeriesContainer::testDeleteSeriesDirectoryAndSafetyGuards()
{
    QTemporaryDir tempRoot;
    QVERIFY(tempRoot.isValid());

    const QString validSeries = tempRoot.filePath(QStringLiteral("test_series"));
    const QString nonSeries   = tempRoot.filePath(QStringLiteral("random_folder"));
    QDir().mkpath(nonSeries);

    SeriesContainer container;
    QVERIFY(container.createNewContainer(validSeries, QStringLiteral("Test Series"), QStringLiteral("Hook"), QStringLiteral("Synopsis")));
    QVERIFY(QDir(validSeries).exists());

    // Must refuse directories without a series manifest
    QSignalSpy errSpy(&container, &SeriesContainer::manifestError);
    QVERIFY(!container.deleteSeriesDirectory(nonSeries));
    QVERIFY(QDir(nonSeries).exists());
    QCOMPARE(errSpy.count(), 1);

    // Must refuse user home directory
    const QString homeDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QVERIFY(!container.deleteSeriesDirectory(homeDir));
    QVERIFY(!container.deleteSeriesDirectory(QDir(homeDir).filePath(QStringLiteral("StoryBuilderStudioProjects"))));

    // Must delete valid series and clear active rootDirectory
    QVERIFY(container.deleteSeriesDirectory(validSeries));
    QVERIFY(!QDir(validSeries).exists());
    QVERIFY(container.rootDirectory().isEmpty());
}

QTEST_MAIN(Test_SeriesContainer)
#include "test_SeriesContainer.moc"

