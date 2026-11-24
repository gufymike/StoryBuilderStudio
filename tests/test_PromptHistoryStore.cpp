// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>

#include "app/promptbuilder/prompthistorystore.h"


class Test_PromptHistoryStore : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testEmptyProjectState();
    void testAddAndRetrieveHistoryEntries();
    void testAddAndRetrieveThinkEntries();
    void testClearHistory();
    void testClearThinkHistory();

private:
    QString m_testSessionDir;
};

void Test_PromptHistoryStore::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_PromptHistoryStore::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_PromptHistoryStore::testEmptyProjectState()
{
    PromptHistoryStore store;
    QCOMPARE(store.projectPath(), QString());
    QVERIFY2(store.getHistoryEntries().isEmpty(), "Data Leak: PromptHistoryStore did not initialize with an empty history list.");
    QVERIFY2(store.getThinkEntries().isEmpty(), "Data Leak: PromptHistoryStore did not initialize with an empty think list.");
}

void Test_PromptHistoryStore::testAddAndRetrieveHistoryEntries()
{
    PromptHistoryStore store;
    store.setProjectPath(m_testSessionDir);

    // Spy attached after setProjectPath so count starts at 0
    QSignalSpy spy(&store, &PromptHistoryStore::historyUpdated);

    const QString timestamp = QStringLiteral("2026-11-06T15:30:00.000Z");
    store.addHistoryEntry(QStringLiteral("script"),
                          QStringLiteral("Write intro"),
                          QStringLiteral("Once upon a time..."),
                          timestamp);

    QCOMPARE(spy.count(), 1);
    QVariantList list = store.getHistoryEntries();
    QCOMPARE(list.size(), 1);

    QVariantMap entry = list.first().toMap();
    QCOMPARE(entry.value(QStringLiteral("category")).toString(), QStringLiteral("script"));
    QCOMPARE(entry.value(QStringLiteral("promptText")).toString(), QStringLiteral("Write intro"));
    QCOMPARE(entry.value(QStringLiteral("responseText")).toString(), QStringLiteral("Once upon a time..."));
    QCOMPARE(entry.value(QStringLiteral("timestamp")).toString(), timestamp);
}

void Test_PromptHistoryStore::testAddAndRetrieveThinkEntries()
{
    PromptHistoryStore store;
    store.setProjectPath(m_testSessionDir);

    // Spy attached after setProjectPath so count starts at 0
    QSignalSpy spy(&store, &PromptHistoryStore::thinkHistoryUpdated);

    const QString timestamp = QStringLiteral("2026-11-06T15:30:00.000Z");
    store.addThinkEntry(QStringLiteral("script"),
                        QStringLiteral("Plan plot"),
                        QStringLiteral("Analyzing beat sheet"),
                        timestamp);

    QCOMPARE(spy.count(), 1);
    QVariantList list = store.getThinkEntries();
    QCOMPARE(list.size(), 1);

    QVariantMap entry = list.first().toMap();
    QCOMPARE(entry.value(QStringLiteral("timestamp")).toString(), timestamp);
    QCOMPARE(entry.value(QStringLiteral("thinkText")).toString(), QStringLiteral("Analyzing beat sheet"));
}

void Test_PromptHistoryStore::testClearHistory()
{
    PromptHistoryStore store;
    store.setProjectPath(m_testSessionDir);
    store.addHistoryEntry(QStringLiteral("script"), QStringLiteral("Test clear"), QStringLiteral("Response"), QStringLiteral("2026-11-06T15:30:00.000Z"));
    QVERIFY(store.getHistoryEntries().size() > 0);

    QSignalSpy spy(&store, &PromptHistoryStore::historyUpdated);
    store.clearHistory();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(store.getHistoryEntries().size(), 0);
}

void Test_PromptHistoryStore::testClearThinkHistory()
{
    PromptHistoryStore store;
    store.setProjectPath(m_testSessionDir);
    store.addThinkEntry(QStringLiteral("script"), QStringLiteral("Test prompt"), QStringLiteral("Think"), QStringLiteral("2026-11-06T15:30:00.000Z"));
    QVERIFY(store.getThinkEntries().size() > 0);

    QSignalSpy spy(&store, &PromptHistoryStore::thinkHistoryUpdated);
    store.clearThinkHistory();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(store.getThinkEntries().size(), 0);
}

QTEST_MAIN(Test_PromptHistoryStore)
#include "test_PromptHistoryStore.moc"


