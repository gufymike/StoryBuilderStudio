// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"

class Test_ImageCandidateModal : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDefaultEmptyState();
    void testOpenWithCandidatesAndSelection();
    void testDiscardAllSignal();
    void testSinglePickDefaultAndCtrlMultiSelect();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *modal = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/image"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 900\n"
                "    height: 700\n"
                "    visible: true\n"
                "    ImageCandidateModal {\n"
                "        id: candidateModal\n"
                "    }\n"
                "}\n"
                );

            engine.loadData(hostQml);
            if (engine.rootObjects().isEmpty()) {
                return false;
            }

            window = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
            if (!window) {
                return false;
            }

            modal = window->findChild<QObject*>(UI::ImageCandidateModal::Root);
            return (modal != nullptr);
        }
    };
};

void Test_ImageCandidateModal::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::cleanPath(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_ImageCandidateModal::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_ImageCandidateModal::testDefaultEmptyState()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load ImageCandidateModal harness");

    QObject *modal = harness.modal;
    QVERIFY(modal != nullptr);

    // Open the popup so its child hierarchy becomes visible
    QMetaObject::invokeMethod(modal, "open");
    QCoreApplication::processEvents();

    QObject *emptyLabel = modal->findChild<QObject*>(UI::ImageCandidateModal::EmptyLabel);
    QVERIFY2(emptyLabel != nullptr, "UI Component Missing: 'emptyCandidatesLabel' not found.");
    QVERIFY2(emptyLabel->property("visible").toBool(), "State Failure: Empty label is not visible when candidates list is empty.");

    QCOMPARE(modal->property("candidateList").toList().size(), 0);

    // Clean up popup state
    QMetaObject::invokeMethod(modal, "close");
    QCoreApplication::processEvents();
}

void Test_ImageCandidateModal::testOpenWithCandidatesAndSelection()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load ImageCandidateModal harness");

    QObject *modal = harness.modal;
    QVERIFY(modal != nullptr);

    QSignalSpy selectSpy(modal, SIGNAL(imageSelected(QString)));

    QVariantList candidates = {
        QStringLiteral("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg=="),
        QStringLiteral("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==")
    };

    QMetaObject::invokeMethod(modal, "openWithCandidates",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant::fromValue(candidates)),
                              Q_ARG(QVariant, QVariant(1)));
    QCoreApplication::processEvents();

    QCOMPARE(modal->property("candidateList").toList().size(), 2);
    QCOMPARE(modal->property("targetShotIndex").toInt(), 1);

    // Emulate clicking save on candidate 0
    modal->metaObject()->invokeMethod(modal, "imageSelected", Qt::DirectConnection, Q_ARG(QString, candidates.first().toString()));
    QCOMPARE(selectSpy.count(), 1);
}

void Test_ImageCandidateModal::testDiscardAllSignal()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load ImageCandidateModal harness");

    QObject *modal = harness.modal;
    QVERIFY(modal != nullptr);

    QSignalSpy discardSpy(modal, SIGNAL(allDiscarded()));

    QObject *discardBtn = modal->findChild<QObject*>(UI::ImageCandidateModal::DiscardAllBtn);
    QVERIFY2(discardBtn != nullptr, "UI Component Missing: 'discardAllBtn' not found.");

    QMetaObject::invokeMethod(discardBtn, "clicked");
    QCoreApplication::processEvents();

    QCOMPARE(discardSpy.count(), 1);
}

void Test_ImageCandidateModal::testSinglePickDefaultAndCtrlMultiSelect()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load ImageCandidateModal harness");

    QObject *modal = harness.modal;
    QVERIFY(modal != nullptr);

    QSignalSpy singleSpy(modal, SIGNAL(imageSelected(QString)));
    QSignalSpy multiSpy(modal, SIGNAL(imagesSelected(QVariant)));

    const QString pngA = QStringLiteral("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==");
    const QString pngB = QStringLiteral("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==");
    const QString pngC = QStringLiteral("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII=");
    QVariantList candidates = { pngA, pngB, pngC };

    QMetaObject::invokeMethod(modal, "openWithCandidates",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant::fromValue(candidates)),
                              Q_ARG(QVariant, QVariant(-1)));
    QCoreApplication::processEvents();

    // 1. Defaults to single best pick (index 0)
    QCOMPARE(modal->property("selectedIndices").toList(), QVariantList{0});

    // 2. Standard click switches selection to a single candidate
    QMetaObject::invokeMethod(modal, "selectCandidate",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(2)),
                              Q_ARG(QVariant, QVariant(int(Qt::NoModifier))));
    QCOMPARE(modal->property("selectedIndices").toList(), QVariantList{2});

    // 3. Ctrl+Click adds a second candidate in sorted index order
    QMetaObject::invokeMethod(modal, "selectCandidate",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(1)),
                              Q_ARG(QVariant, QVariant(int(Qt::ControlModifier))));
    QCOMPARE(modal->property("selectedIndices").toList(), (QVariantList{1, 2}));

    // 4. Ctrl+Click removes an already selected candidate
    QMetaObject::invokeMethod(modal, "selectCandidate",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(2)),
                              Q_ARG(QVariant, QVariant(int(Qt::ControlModifier))));
    QCOMPARE(modal->property("selectedIndices").toList(), QVariantList{1});

    // 5. Ctrl+Click keeps at least one candidate selected
    QMetaObject::invokeMethod(modal, "selectCandidate",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(1)),
                              Q_ARG(QVariant, QVariant(int(Qt::ControlModifier))));
    QCOMPARE(modal->property("selectedIndices").toList(), QVariantList{1});

    // 6. Committing emits the chosen candidate and closes the popup
    QMetaObject::invokeMethod(modal, "commitSelectedCandidates", Qt::DirectConnection);
    QCoreApplication::processEvents();

    QCOMPARE(singleSpy.count(), 1);
    QCOMPARE(singleSpy.takeFirst().at(0).toString(), pngB);
    QCOMPARE(multiSpy.count(), 1);
    QCOMPARE(modal->property("opened").toBool(), false);
}

QTEST_MAIN(Test_ImageCandidateModal)
#include "test_ImageCandidateModal.moc"
