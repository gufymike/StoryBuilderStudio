// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QAbstractListModel>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>
#include "imagedirectorymodel.h"
#include "ui_constants.h"

// Mock ImageDirectoryModel for headless testing
class MockImageDirectoryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        FilePathRole = Qt::UserRole + 1,
        FileNameRole = Qt::UserRole + 2,
        FileSizeRole = Qt::UserRole + 3,
        CreatedDateRole = Qt::UserRole + 4,
        ModifiedDateRole = Qt::UserRole + 5,
        ResolutionRole = Qt::UserRole + 6,
        FormatRole = Qt::UserRole + 7,
        ColorSpaceRole = Qt::UserRole + 8
    };

    explicit MockImageDirectoryModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

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
        case ResolutionRole: return map.value(QStringLiteral("resolution"));
        default: return QVariant();
        }
    }

    QHash<int, QByteArray> roleNames() const override {
        return {
            { FilePathRole, "filePath" },
            { FileNameRole, "fileName" },
            { FileSizeRole, "fileSize" },
            { ResolutionRole, "resolution" }
        };
    }

    Q_INVOKABLE void setProjectPath(const QString &) {}
    Q_INVOKABLE void refresh() {}

    void addMockItem(const QString &name, const QString &path) {
        beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
        QVariantMap m;
        m[QStringLiteral("fileName")] = name;
        m[QStringLiteral("filePath")] = path;
        m[QStringLiteral("fileSize")] = QStringLiteral("1.2 MB");
        m[QStringLiteral("resolution")] = QStringLiteral("1024x1024");
        m_items.append(m);
        endInsertRows();
    }

private:
    QList<QVariantMap> m_items;
};

class Test_ImageGalleryWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testGallerySelectionLogic();
    void testMultiSelectToggle();

private:
    QString m_testSessionDir;

    struct TestHarness {
        QQmlApplicationEngine engine;
        QQuickWindow *window = nullptr;
        QObject *gallery = nullptr;

        bool init(const QString &sessionDir) {
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace"));
            engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/image"));

            const QByteArray hostQml = QByteArrayLiteral(
                "import QtQuick\n"
                "import QtQuick.Controls\n"
                "import StoryBuilderStudio\n"
                "ApplicationWindow {\n"
                "    id: testRootWindow\n"
                "    width: 800\n"
                "    height: 600\n"
                "    visible: true\n"
                "    ImageGalleryWidget {\n"
                "        id: galleryItem\n"
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

            gallery = window->findChild<QObject*>(UI::ImageGalleryWidget::Root);
            return (gallery != nullptr);
        }
    };
};

void Test_ImageGalleryWidget::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    // Register real ImageDirectoryModel for the QML engine
    qmlRegisterType<ImageDirectoryModel>("StoryBuilderStudio", 1, 0, "ImageDirectoryModel");

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::cleanPath(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_ImageGalleryWidget::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_ImageGalleryWidget::testGallerySelectionLogic()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load ImageGalleryWidget harness");

    QObject *gallery = harness.gallery;
    QVERIFY(gallery != nullptr);

    // Initial state: empty selection
    QCOMPARE(gallery->property("selectedCount").toInt(), 0);

    // Single item click selection
    QString path1 = QStringLiteral("C:/Workspace/shot_01.png");
    QMetaObject::invokeMethod(gallery, "handleItemClick",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(path1)),
                              Q_ARG(QVariant, QVariant(0))); // 0 modifiers
    QCoreApplication::processEvents();

    QVERIFY2(gallery->property("selectedCount").toInt() == 1, "Selection Logic Failure: selectedCount did not update to 1 after click.");

    QVariant isSel;
    QMetaObject::invokeMethod(gallery, "isSelected",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, isSel),
                              Q_ARG(QVariant, QVariant(path1)));
    QVERIFY2(isSel.toBool() == true, "Selection Logic Failure: isSelected() returned false for an actively selected item.");

    // Clear selection
    QMetaObject::invokeMethod(gallery, "clearSelection");
    QCoreApplication::processEvents();
    QCOMPARE(gallery->property("selectedCount").toInt(), 0);
}

void Test_ImageGalleryWidget::testMultiSelectToggle()
{
    TestHarness harness;
    QVERIFY2(harness.init(m_testSessionDir), "Failed to load ImageGalleryWidget harness");

    QObject *gallery = harness.gallery;
    QString path1 = QStringLiteral("C:/Workspace/shot_01.png");
    QString path2 = QStringLiteral("C:/Workspace/shot_02.png");

    // Select first item
    QMetaObject::invokeMethod(gallery, "handleItemClick",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(path1)),
                              Q_ARG(QVariant, QVariant(0)));

    // Ctrl+Click second item (Qt::ControlModifier = 0x04000000)
    QMetaObject::invokeMethod(gallery, "handleItemClick",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(path2)),
                              Q_ARG(QVariant, QVariant(static_cast<int>(Qt::ControlModifier))));
    QCoreApplication::processEvents();

    QVERIFY2(gallery->property("selectedCount").toInt() == 2, "Selection Logic Failure: Multi-select Ctrl+Click did not increment selection count.");

    // Ctrl+Click first item again (toggles off)
    QMetaObject::invokeMethod(gallery, "handleItemClick",
                              Qt::DirectConnection,
                              Q_ARG(QVariant, QVariant(path1)),
                              Q_ARG(QVariant, QVariant(static_cast<int>(Qt::ControlModifier))));
    QCoreApplication::processEvents();

    QCOMPARE(gallery->property("selectedCount").toInt(), 1);
}

QTEST_MAIN(Test_ImageGalleryWidget)
#include "test_ImageGalleryWidget.moc"
