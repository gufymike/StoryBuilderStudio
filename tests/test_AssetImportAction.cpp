// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QUrl>

class MockAssetModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList importedList READ importedList NOTIFY importedListChanged)

public:
    explicit MockAssetModel(QObject *parent = nullptr) : QObject(parent) {}

    QStringList importedList() const { return m_imported; }

    Q_INVOKABLE void importAssets(const QVariantList &files) {
        for (const auto &file : files) {
            m_imported.append(file.toString());
        }
        emit importedListChanged();
    }

signals:
    void importedListChanged();

private:
    QStringList m_imported;
};

class Test_AssetImportAction : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDefaultProperties();
    void testTargetModelImportInvocation();
    void testPathSanitizationOnWindows();

private:
    QQmlEngine m_engine;
    QString m_testSessionDir;
    QUrl m_componentUrl;
};

void Test_AssetImportAction::initTestCase()
{
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::toNativeSeparators(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);

    m_engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR));
    m_engine.addImportPath(QStringLiteral(CMAKE_BINARY_DIR));
    m_engine.addImportPath(QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace"));

    QString qmlFilePath = QStringLiteral(CMAKE_SOURCE_DIR) + QStringLiteral("/app/workspace/AssetImportAction.qml");
    m_componentUrl = QUrl::fromLocalFile(qmlFilePath);
    QVERIFY2(QFile::exists(qmlFilePath), qPrintable("File not found: " + qmlFilePath));
}

void Test_AssetImportAction::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_AssetImportAction::testDefaultProperties()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> actionObj(component.create());
    QVERIFY(!actionObj.isNull());

    QCOMPARE(actionObj->property("dialogTitle").toString(), QStringLiteral("Import Media Asset(s)"));
    QCOMPARE(actionObj->property("nameFilters").toStringList(), QStringList{ QStringLiteral("All Files (*)") });
    QVERIFY(actionObj->property("targetModel").isNull());
}

void Test_AssetImportAction::testTargetModelImportInvocation()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> actionObj(component.create());
    QVERIFY(!actionObj.isNull());

    MockAssetModel mockModel;
    actionObj->setProperty("targetModel", QVariant::fromValue(&mockModel));

    QSignalSpy spy(actionObj.data(), SIGNAL(assetsImported(QVariant)));

    QVariantList mockFiles = {
        QStringLiteral("C:/test/asset1.png"),
        QStringLiteral("C:/test/asset2.wav")
    };

    bool invoked = QMetaObject::invokeMethod(actionObj.data(), "importFiles",
                                             Qt::DirectConnection,
                                             Q_ARG(QVariant, QVariant::fromValue(mockFiles)));
    QVERIFY2(invoked, "Method Invocation Failure: Failed to invoke importFiles() on AssetImportAction.");
    QCoreApplication::processEvents();
    QVERIFY2(spy.count() == 1, "Signal Failure: assetsImported was not emitted after importFiles().");

    QCOMPARE(mockModel.importedList().size(), 2);
    QCOMPARE(mockModel.importedList().at(0), QStringLiteral("C:/test/asset1.png"));
    QCOMPARE(mockModel.importedList().at(1), QStringLiteral("C:/test/asset2.wav"));
}

void Test_AssetImportAction::testPathSanitizationOnWindows()
{
    QQmlComponent component(&m_engine, m_componentUrl);
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> actionObj(component.create());
    QVERIFY(!actionObj.isNull());

    MockAssetModel mockModel;
    actionObj->setProperty("targetModel", QVariant::fromValue(&mockModel));

    // Simulate URL formatted strings passed by native FileDialog
    QVariantList urlStrings = {
        QStringLiteral("file:///C:/Workspace/test_shot.png")
};

QMetaObject::invokeMethod(actionObj.data(), "importFiles",
                          Qt::DirectConnection,
                          Q_ARG(QVariant, QVariant::fromValue(urlStrings)));
QCoreApplication::processEvents();

QCOMPARE(mockModel.importedList().size(), 1);
#ifdef Q_OS_WIN
    // Windows: file:///C:/Workspace/test_shot.png -> C:/Workspace/test_shot.png
    QCOMPARE(mockModel.importedList().at(0), QStringLiteral("C:/Workspace/test_shot.png"));
#else
    // Linux/macOS: leading '/' is preserved on POSIX paths
    QCOMPARE(mockModel.importedList().at(0), QStringLiteral("/C:/Workspace/test_shot.png"));
#endif
}

QTEST_MAIN(Test_AssetImportAction)
#include "test_AssetImportAction.moc"
