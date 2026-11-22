/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_AuditTrailStore.cpp
** Test suite for AuditTrailStore subsystem under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include "app/core/audittrailstore.h"

class Test_AuditTrailStore : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Security & Path Tests
    void testInitialStateEmpty();
    void testSetStorageDirectoryValid();
    void testSecurityRejectsPathTraversal();
    void testStorageDirectoryReturnsValidPath();

    // Functional Transaction Tests
    void testRecordSingleTransactionWritesJson();
    void testRecordMultipleTransactionsAppendsInOrder();
    void testSelectedOptionsPreserved();
    void testClearRemovesAuditFile();
    void testSignalsEmittedOnRecord();
    void testSwitchingDirectoryResetsState();

private:
    QString m_testSessionDir;
    QString createScopedTempDir();
};

void Test_AuditTrailStore::initTestCase()
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

void Test_AuditTrailStore::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_AuditTrailStore::init()
{
}

void Test_AuditTrailStore::cleanup()
{
}

QString Test_AuditTrailStore::createScopedTempDir()
{
    QString subUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString path = QDir::toNativeSeparators(m_testSessionDir + "/" + subUuid);
    QDir().mkpath(path);
    return path;
}

void Test_AuditTrailStore::testInitialStateEmpty()
{
    AuditTrailStore store;

    // 1. Verify default contruction targets the golobal AppData location.
    QString expectedDefault = QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QCOMPARE(store.storageDirectory(), expectedDefault);
    QVERIFY2(!store.getAuditFilePath().isEmpty(), "Default audit file path is unexpectedly empty.");


    // 2. Redirect to test sandbox
    store.setStorageDirectory(m_testSessionDir);

    // 3. verify initial state tests.
    QCOMPARE(store.transactionCount(), 0);
    QCOMPARE(store.getTransactions().size(), 0);
}

void Test_AuditTrailStore::testSetStorageDirectoryValid()
{
    AuditTrailStore store;

    QString projDir = createScopedTempDir();

    QSignalSpy spy(&store, &AuditTrailStore::storageDirectoryChanged);
    store.setStorageDirectory(projDir);

    QCOMPARE(store.storageDirectory(), projDir);
    QCOMPARE(spy.count(), 1);
    QVERIFY(store.getAuditFilePath().endsWith(QStringLiteral("audit_trail.json")));
}

void Test_AuditTrailStore::testSecurityRejectsPathTraversal()
{
    AuditTrailStore store;
    QString initialPath = store.storageDirectory(); // Should be AppDataLocation

    QString malformed = QStringLiteral("C:/Somewhere/../EvilFolder");
    store.setStorageDirectory(malformed);

    // Verify it ignored the traversal and kept the initial path
    QVERIFY2(store.storageDirectory() == initialPath, "Path traversal vulnerability detected! Store accepted a malformed or malicious path.");
}

void Test_AuditTrailStore::testStorageDirectoryReturnsValidPath()
{
    AuditTrailStore store;

    // Explicitly wipe the path to test the guardrail
    QString storageDirectory = store.storageDirectory();
    store.setStorageDirectory(QString());
    QCOMPARE(store.storageDirectory(), storageDirectory);

}

void Test_AuditTrailStore::testRecordSingleTransactionWritesJson()
{
    AuditTrailStore store;
    QString projDir = createScopedTempDir();
    store.setStorageDirectory(projDir);

    store.recordTransaction(
        QStringLiteral("ProjectAlpha"),
        QStringLiteral("script"),
        QStringLiteral("Write an opening shot of Neo Tokyo"),
        QStringLiteral("FADE IN: Neon lights shimmer across rain puddles."),
        QStringLiteral("2026-11-06T08:01:00"),
        QStringLiteral("2026-11-06T08:01:05"),
        QStringList{QStringLiteral("temp=0.7"), QStringLiteral("top_p=0.9")}
        );

    QCOMPARE(store.transactionCount(), 1);
    QVERIFY2(QFile::exists(store.getAuditFilePath()), "JSON audit file was not created on disk after recording a transaction.");

    QVariantList list = store.getTransactions();
    QVERIFY2(list.size() == 1, "getTransactions() did not return the expected single record.");


    QVariantMap item = list.first().toMap();
    QVERIFY2(item.value(QStringLiteral("project_name")).toString() == QStringLiteral("ProjectAlpha"), "JSON Serialization Error: project_name mismatched.");
    QCOMPARE(item.value(QStringLiteral("category")).toString(), QStringLiteral("script"));
    QCOMPARE(item.value(QStringLiteral("prompt_text")).toString(), QStringLiteral("Write an opening shot of Neo Tokyo"));
    QCOMPARE(item.value(QStringLiteral("response_text")).toString(), QStringLiteral("FADE IN: Neon lights shimmer across rain puddles."));
}

void Test_AuditTrailStore::testRecordMultipleTransactionsAppendsInOrder()
{
    AuditTrailStore store;
    QString projDir = createScopedTempDir();
    store.setStorageDirectory(projDir);

    for (int i = 1; i <= 5; ++i) {
        store.recordTransaction(
            QStringLiteral("ProjectAlpha"),
            QStringLiteral("image"),
            QStringLiteral("Prompt %1").arg(i),
            QStringLiteral("Output %1").arg(i),
            QStringLiteral("Timestamp_Sent_%1").arg(i),
            QStringLiteral("Timestamp_Recv_%1").arg(i)
            );
    }

    QCOMPARE(store.transactionCount(), 5);

    QVariantList list = store.getTransactions();
    QCOMPARE(list.size(), 5);
    for (int i = 0; i < 5; ++i) {
        QVariantMap item = list.at(i).toMap();
        QCOMPARE(item.value(QStringLiteral("prompt_text")).toString(), QStringLiteral("Prompt %1").arg(i + 1));
    }
}

void Test_AuditTrailStore::testSelectedOptionsPreserved()
{
    AuditTrailStore store;
    QString projDir = createScopedTempDir();
    store.setStorageDirectory(projDir);

    QStringList expectedOptions = {
        QStringLiteral("--steps 20"),
        QStringLiteral("--guidance 3.5"),
        QStringLiteral("--sampler euler")
    };

    store.recordTransaction(
        QStringLiteral("ProjectAlpha"),
        QStringLiteral("image"),
        QStringLiteral("Cyberpunk motorcycle"),
        QStringLiteral("img_001.png"),
        QStringLiteral("2026-11-06T08:05:00"),
        QStringLiteral("2026-11-06T08:05:04"),
        expectedOptions
        );

    QVariantList list = store.getTransactions();
    QCOMPARE(list.size(), 1);
    QVariantMap item = list.first().toMap();
    QStringList recoveredOptions = item.value(QStringLiteral("selected_options")).toStringList();
    QCOMPARE(recoveredOptions, expectedOptions);
}

void Test_AuditTrailStore::testClearRemovesAuditFile()
{
    AuditTrailStore store;
    QString projDir = createScopedTempDir();
    store.setStorageDirectory(projDir);

    store.recordTransaction(
        QStringLiteral("TempProject"),
        QStringLiteral("audio"),
        QStringLiteral("Synthesize voice"),
        QStringLiteral("audio.wav"),
        QStringLiteral("T1"),
        QStringLiteral("T2")
        );

    QVERIFY(QFile::exists(store.getAuditFilePath()));
    QCOMPARE(store.transactionCount(), 1);

    store.clear();
    QCOMPARE(store.transactionCount(), 0);
    QVERIFY2(!QFile::exists(store.getAuditFilePath()), "clear() failed to physically delete the audit JSON file from the disk.");
}

void Test_AuditTrailStore::testSignalsEmittedOnRecord()
{
    AuditTrailStore store;
    QString projDir = createScopedTempDir();
    store.setStorageDirectory(projDir);

    QSignalSpy loggedSpy(&store, &AuditTrailStore::transactionLogged);
    QSignalSpy countSpy(&store, &AuditTrailStore::transactionCountChanged);

    store.recordTransaction(
        QStringLiteral("ProjectSignals"),
        QStringLiteral("video"),
        QStringLiteral("Generate pan shot"),
        QStringLiteral("video.mp4"),
        QStringLiteral("2026-11-06T08:10:00"),
        QStringLiteral("2026-11-06T08:10:15")
        );

    QVERIFY2(loggedSpy.count() == 1, "transactionLogged signal was not emitted upon recording a transaction.");
    QList<QVariant> loggedArgs = loggedSpy.takeFirst();
    QCOMPARE(loggedArgs.at(0).toString(), QStringLiteral("video"));
    QCOMPARE(loggedArgs.at(1).toString(), QStringLiteral("2026-11-06T08:10:00"));

    QCOMPARE(countSpy.count(), 1);
    QCOMPARE(countSpy.takeFirst().at(0).toInt(), 1);
}

void Test_AuditTrailStore::testSwitchingDirectoryResetsState()
{
    AuditTrailStore store;

    // 1. Setup Sandbox A and dirty the state
    QString dirA = createScopedTempDir();
    store.setStorageDirectory(dirA);
    store.recordTransaction(
        QStringLiteral("ProjectA"), QStringLiteral("script"),
        QStringLiteral("P1"), QStringLiteral("R1"),
        QStringLiteral("T1"), QStringLiteral("T2")
        );
    QCOMPARE(store.transactionCount(), 1);
    QVERIFY(QFile::exists(store.getAuditFilePath()));

    // 2. Switch to fresh Sandbox B
    QString dirB = createScopedTempDir();
    store.setStorageDirectory(dirB);

    // 3. Verify the store wiped its internal memory and points to a fresh slate
    QCOMPARE(store.transactionCount(), 0);
    QVERIFY2(store.getTransactions().isEmpty(), "Memory leak/Bleed: Internal transaction list was not cleared after switching directories.");
    QVERIFY2(!QFile::exists(store.getAuditFilePath()), "Fresh directory should not contain an audit file until explicitly written to.");
}

QTEST_MAIN(Test_AuditTrailStore)
#include "test_AuditTrailStore.moc"

