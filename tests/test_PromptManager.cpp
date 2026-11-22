/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_PromptManager.cpp
** Unit test suite for PromptManager under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUuid>
#include "app/promptbuilder/promptmanager.h"

class Test_PromptManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDefaultState();
    void testActiveCategoryChange();
    void testSaveAndLoadPrompt();
    void testScopeDiscovery();

private:
    QString m_testSessionDir;
    QString m_seriesDir;
    QString m_episodeDir;
};

void Test_PromptManager::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);

    m_seriesDir = m_testSessionDir + QStringLiteral("/SeriesA");
    m_episodeDir = m_testSessionDir + QStringLiteral("/SeriesA/Episode1");

    QDir().mkpath(m_seriesDir);
    QDir().mkpath(m_episodeDir);
}

void Test_PromptManager::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_PromptManager::testDefaultState()
{
    PromptManager manager;
    QCOMPARE(manager.activeCategory(), QStringLiteral("text"));
    // First entry is always "+ Create New Prompt..."
    QVERIFY2(!manager.availablePrompts().isEmpty(), "PromptManager failed to initialize with the default '+ Create New Prompt...' entry.");
    QCOMPARE(manager.availablePrompts().first().toMap().value(QStringLiteral("label")).toString(), QStringLiteral("+ Create New Prompt..."));
}

void Test_PromptManager::testActiveCategoryChange()
{
    PromptManager manager;
    QSignalSpy catSpy(&manager, &PromptManager::activeCategoryChanged);

    manager.setActiveCategory(QStringLiteral("image"));
    QCOMPARE(catSpy.count(), 1);
    QCOMPARE(manager.activeCategory(), QStringLiteral("image"));
}

void Test_PromptManager::testSaveAndLoadPrompt()
{
    PromptManager manager;
    manager.setPaths(m_seriesDir, m_episodeDir);

    QSignalSpy saveSpy(&manager, &PromptManager::promptSaved);

    bool saved = manager.savePrompt(QStringLiteral("cinematic_shot"),
                                    QStringLiteral("35mm anamorphic wide angle lens"),
                                    QStringLiteral("image"),
                                    true); // Series scope

    QVERIFY2(saved, "File I/O Failure: PromptManager failed to write the prompt file to disk.");
    QCOMPARE(saveSpy.count(), 1);

    QString fullPath = saveSpy.first().at(0).toString();
    QVERIFY(QFile::exists(fullPath));

    QString content = manager.readPromptFile(fullPath);
    QCOMPARE(content, QStringLiteral("35mm anamorphic wide angle lens"));
}

void Test_PromptManager::testScopeDiscovery()
{
    PromptManager manager;
    manager.setPaths(m_seriesDir, m_episodeDir);
    manager.setActiveCategory(QStringLiteral("image"));

    QVariantList prompts = manager.availablePrompts();
    // Index 0 is "+ Create New Prompt...", Index 1 is the newly saved series prompt
    QVERIFY(prompts.size() >= 2);

    QVariantMap item = prompts.at(1).toMap();
    QCOMPARE(item.value(QStringLiteral("fileName")).toString(), QStringLiteral("cinematic_shot.prompt"));
    QCOMPARE(item.value(QStringLiteral("scope")).toString(), QStringLiteral("Series"));
    QCOMPARE(item.value(QStringLiteral("category")).toString(), QStringLiteral("image"));
}

QTEST_MAIN(Test_PromptManager)
#include "test_PromptManager.moc"

