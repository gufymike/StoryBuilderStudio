/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_AppLogger.cpp
** Test suite for AppLogger core subsystem under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <QLoggingCategory>
#include "app/core/applogger.h"

class Test_AppLogger : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void testInitializationCreatesLogFile();
    void testMessageFormattingAndLevels();
    void testImmediateFlushOnWrite();
    void testThreadSafetyConcurrentWrites();
    void testMemoryAndResourceCleanupOnClose();

private:
    QString m_testSessionDir;
    QString createScopedTempDir();
    QString readLogContents(const QString &filePath);
};

void Test_AppLogger::initTestCase()
{
    QLoggingCategory::setFilterRules(QStringLiteral("*.debug=true"));
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    // StandardLocations::TempLocation/<uuid>/ conforming to project guidelines
    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);

    QDir().mkpath(m_testSessionDir);
}

void Test_AppLogger::cleanupTestCase()
{
    AppLogger::closeLogging();
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_AppLogger::init()
{
    if (AppLogger::isLoggingInitialized()) {
        AppLogger::closeLogging();
    }
}

void Test_AppLogger::cleanup()
{
    if (AppLogger::isLoggingInitialized()) {
        AppLogger::closeLogging();
    }
}

QString Test_AppLogger::createScopedTempDir()
{
    QString subUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString path = QDir::toNativeSeparators(m_testSessionDir + "/" + subUuid);
    QDir().mkpath(path);
    return path;
}

QString Test_AppLogger::readLogContents(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

void Test_AppLogger::testInitializationCreatesLogFile()
{
    QString testLogDir = createScopedTempDir();
    AppLogger::initLogging(testLogDir);

    QVERIFY2(AppLogger::isLoggingInitialized(), "AppLogger failed to initialize after initLogging() was called.");
    QString logPath = AppLogger::getLogFilePath();
    QVERIFY2(!logPath.isEmpty(), "Log file path returned empty after successful initialization.");

    QFileInfo fileInfo(logPath);
    QVERIFY2(fileInfo.exists(), "Log file was not created on disk.");
    QVERIFY2(fileInfo.isFile(), "Log path exists but is not a file.");

    QString content = readLogContents(logPath);
    QVERIFY2(content.contains(QStringLiteral("Story Builder Studio Log Session Started")),
             "Initial session header was not written to the log file.");
}

void Test_AppLogger::testMessageFormattingAndLevels()
{
    QString testLogDir = createScopedTempDir();
    AppLogger::initLogging(testLogDir);
    QString logPath = AppLogger::getLogFilePath();

    qDebug() << "Test Debug Message 101";
    qInfo() << "Test Info Message 202";
    qWarning() << "Test Warning Message 303";
    qCritical() << "Test Critical Message 404";

    QString content = readLogContents(logPath);

    QVERIFY2(content.contains(QStringLiteral("[DEBUG] Test Debug Message 101")), "Debug message failed formatting or routing.");
    QVERIFY2(content.contains(QStringLiteral("[INFO]  Test Info Message 202")), "Info message failed formatting or routing.");
    QVERIFY2(content.contains(QStringLiteral("[WARN]  Test Warning Message 303")), "Warning message failed formatting or routing.");
    QVERIFY2(content.contains(QStringLiteral("[CRIT]  Test Critical Message 404")), "Critical message failed formatting or routing.");
}

void Test_AppLogger::testImmediateFlushOnWrite()
{
    QString testLogDir = createScopedTempDir();
    AppLogger::initLogging(testLogDir);
    QString logPath = AppLogger::getLogFilePath();

    const QString sentinelMsg = QStringLiteral("CrashSentinelImmediateFlushCheck_777");
    qInfo() << sentinelMsg;

    QFile reader(logPath);
    QVERIFY(reader.open(QIODevice::ReadOnly | QIODevice::Text));
    QString onDiskData = QString::fromUtf8(reader.readAll());
    reader.close();

    QVERIFY2(onDiskData.contains(sentinelMsg), "Log message must be flushed to disk immediately");
}

void Test_AppLogger::testThreadSafetyConcurrentWrites()
{
    QString testLogDir = createScopedTempDir();
    AppLogger::initLogging(testLogDir);
    QString logPath = AppLogger::getLogFilePath();

    const int threadCount = 4;
    const int messagesPerThread = 25;

    QList<QFuture<void>> futures;
    for (int t = 0; t < threadCount; ++t) {
        futures.append(QtConcurrent::run([t, messagesPerThread]() {
            for (int i = 0; i < messagesPerThread; ++i) {
                qInfo() << QStringLiteral("WorkerThread_%1_Iter_%2").arg(t).arg(i);
            }
        }));
    }

    for (auto &f : futures) {
        f.waitForFinished();
    }

    QString content = readLogContents(logPath);
    for (int t = 0; t < threadCount; ++t) {
        QVERIFY2(content.contains(QStringLiteral("WorkerThread_%1_Iter_0").arg(t)),
                 qPrintable(QStringLiteral("Thread %1 failed to log its first message. Race condition detected.").arg(t)));
        QVERIFY2(content.contains(QStringLiteral("WorkerThread_%1_Iter_24").arg(t)),
                 qPrintable(QStringLiteral("Thread %1 failed to log its last message. Race condition detected.").arg(t)));
    }
}

void Test_AppLogger::testMemoryAndResourceCleanupOnClose()
{
    QString testLogDir = createScopedTempDir();
    AppLogger::initLogging(testLogDir);
    QString logPath = AppLogger::getLogFilePath();

    QVERIFY2(AppLogger::isLoggingInitialized(), "Pre-condition failed: Logger did not initialize.");
    AppLogger::closeLogging();
    QVERIFY2(!AppLogger::isLoggingInitialized(), "AppLogger reported active after closeLogging() was called.");

    // File lock must be released so deletion succeeds
    QVERIFY2(QFile::remove(logPath), "Failed to delete log file. closeLogging() did not release the underlying file lock.");
}

QTEST_MAIN(Test_AppLogger)
#include "test_AppLogger.moc"

