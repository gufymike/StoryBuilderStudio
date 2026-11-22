/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: test_VideoWorkspaceWidget.cpp
** Unit test suite for VideoWorkspaceWidget QML component under QTest.
**
****************************************************************************/
#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQuickWindow>
#include <QQuickItem>
#include <QAbstractListModel>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

#include "ui_constants.h"

class MockVideoDirectoryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        FilePathRole = Qt::UserRole + 1,
        FileNameRole = Qt::UserRole + 2,
        FileSizeRole = Qt::UserRole + 3,
        ModifiedDateRole = Qt::UserRole + 4
    };

    explicit MockVideoDirectoryModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return m_items.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.row() >= m_items.size()) return QVariant();
        const auto &map = m_items.at(index.row());
        switch (role) {
        case FilePathRole: return map.value(QStringLiteral("filePath"));
        case FileNameRole: return map.value(QStringLiteral("fileName"));
        case FileSizeRole: return map.value(QStringLiteral("fileSize"));
        case ModifiedDateRole: return map.value(QStringLiteral("modifiedDate"));
        default: return QVariant();
        }
    }

    QHash<int, QByteArray> roleNames() const override {
        return {
            { FilePathRole, "filePath" },
            { FileNameRole, "fileName" },
            { FileSizeRole, "fileSize" },
            { ModifiedDateRole, "modifiedDate" }
        };
    }

    Q_INVOKABLE void setProjectPath(const QString &) {}
    Q_INVOKABLE void refresh() {}

    void addMockVideo(const QString &name, const QString &path) {
        beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
        QVariantMap m;
        m[QStringLiteral("fileName")] = name;
        m[QStringLiteral("filePath")] = path;
        m[QStringLiteral("fileSize")] = QStringLiteral("24.5 MB");
        m[QStringLiteral("modifiedDate")] = QStringLiteral("2026-11-08");
        m_items.append(m);
        endInsertRows();
    }

private:
    QList<QVariantMap> m_items;
};

class Test_VideoWorkspaceWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testWidgetInitialization();
    void testTimeFormatting();
    void testToolTipsConfigured();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *widget = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/video"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 800\n"
                "    height: 650\n"
                "    visible: true\n"
                "    VideoWorkspaceWidget {\n"
                "        id: videoWidget\n"
                "        anchors.fill: parent\n"
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

            widget = window->findChild<QObject*>(UI::VideoWorkspaceWidget::Root);
            return (widget != nullptr);
        }
    };
};

void Test_VideoWorkspaceWidget::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    qmlRegisterType<MockVideoDirectoryModel>("StoryBuilderStudio", 1, 0, "VideoDirectoryModel");

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::cleanPath(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_VideoWorkspaceWidget::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_VideoWorkspaceWidget::testWidgetInitialization()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load VideoWorkspaceWidget harness");

    QObject *widget = harness.widget;
    QVERIFY(widget != nullptr);
    QObject *statusText = widget->findChild<QObject*>(UI::VideoWorkspaceWidget::PlayStatusText);
    QVERIFY2(statusText != nullptr, "UI Component Missing: 'playStatusText' not found.");
    QObject *emptyLabel = widget->findChild<QObject*>(UI::VideoWorkspaceWidget::EmptyViewportText);
    QVERIFY2(emptyLabel != nullptr, "UI Component Missing: 'emptyViewportText' not found.");
    QObject *timeDisplay = widget->findChild<QObject*>(UI::VideoWorkspaceWidget::TimeDisplay);
    QVERIFY2(timeDisplay != nullptr, "UI Component Missing: 'timeDisplay' not found.");

    // Initial playback and status checks
    QCOMPARE(statusText->property("text").toString(), QStringLiteral("No video selected"));
    QCOMPARE(emptyLabel->property("visible").toBool(), true);
    QCOMPARE(emptyLabel->property("text").toString(), QStringLiteral("Select a video asset below to preview"));

    // Verify time display initial value
    QCOMPARE(timeDisplay->property("text").toString(), QStringLiteral("00:00 / 00:00"));
}

void Test_VideoWorkspaceWidget::testTimeFormatting()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load VideoWorkspaceWidget harness");

    QObject *widget = harness.widget;

    QVariant formatted;
    QMetaObject::invokeMethod(widget, "formatTime",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, formatted),
                              Q_ARG(QVariant, QVariant(125000))); // 2m 5s
    QCOMPARE(formatted.toString(), QStringLiteral("02:05"));

    QMetaObject::invokeMethod(widget, "formatTime",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, formatted),
                              Q_ARG(QVariant, QVariant(0)));
    QCOMPARE(formatted.toString(), QStringLiteral("00:00"));
}

void Test_VideoWorkspaceWidget::testToolTipsConfigured()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load VideoWorkspaceWidget harness");

    QObject *widget = harness.widget;
    QVERIFY(widget != nullptr);

    auto getAttachedToolTipText = [&](QObject *item) -> QString {
        if (!item) return QString();
        QQmlContext *context = QQmlEngine::contextForObject(item);
        if (!context) return QString();

        // Evaluate ToolTip.text in the item's QML scope
        QQmlExpression expr(context, qobject_cast<QQuickItem*>(item), QStringLiteral("ToolTip.text"));
        QVariant val = expr.evaluate();
        return val.toString();
    };

    QObject *importBtn = widget->findChild<QObject*>(UI::VideoWorkspaceWidget::ImportVideoBtn);
    QVERIFY2(importBtn != nullptr, "UI Component Missing: 'importVideoBtn' not found.");
    QObject *refreshBtn = widget->findChild<QObject*>(UI::VideoWorkspaceWidget::RefreshVideoBtn);
    QVERIFY2(refreshBtn != nullptr, "UI Component Missing: 'refreshVideoBtn' not found.");
    QObject *scrubSlider = widget->findChild<QObject*>(UI::VideoWorkspaceWidget::ScrubSlider);
    QVERIFY2(scrubSlider != nullptr, "UI Component Missing: 'scrubSlider' not found.");

    // 1. Check ToolTip text for Import Button
    QCOMPARE(getAttachedToolTipText(importBtn), QStringLiteral("Select video clips to import into this project."));
    // 2. Check ToolTip text for Refresh Button
    QCOMPARE(getAttachedToolTipText(refreshBtn), QStringLiteral("Looks in the video asset folder for new video assets and loads them."));
    // 3. Check ToolTip text for Scrub Slider
    QCOMPARE(getAttachedToolTipText(scrubSlider), QStringLiteral("Seek Video Position"));
}

QTEST_MAIN(Test_VideoWorkspaceWidget)
#include "test_VideoWorkspaceWidget.moc"
