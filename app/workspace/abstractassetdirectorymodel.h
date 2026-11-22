/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: abstractassetdirectorymodel.h
** Abstract base QAbstractListModel managing project workspace asset directories.
**
****************************************************************************/
#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QList>
#include <QFileInfo>
#include <QtQml/QQmlEngine>

/**
 * @class AbstractAssetDirectoryModel
 * @brief Abstract base QAbstractListModel scanning and managing project workspace asset directories.
 *
 * Provides the unified foundation for populating QML ListViews with local files.
 * Handles directory scanning, file deduplication during imports, byte-size formatting,
 * and standard QAbstractListModel role bindings.
 */
class AbstractAssetDirectoryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("AbstractAssetDirectoryModel is an abstract base class.")

    Q_PROPERTY(QString projectPath READ projectPath WRITE setProjectPath NOTIFY projectPathChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum AssetRoles {
        FileNameRole = Qt::UserRole + 1,
        FilePathRole,
        FileSizeRole,
        CreatedDateRole,
        ModifiedDateRole,
        UserCustomRoleBase = Qt::UserRole + 100
    };
    Q_ENUM(AssetRoles)

    struct BaseAssetItem {
        QString fileName;
        QString filePath;
        QString fileSizeFormatted;
        QString createdDateFormatted;
        QString modifiedDateFormatted;
    };

    explicit AbstractAssetDirectoryModel(QObject *parent = nullptr);
    ~AbstractAssetDirectoryModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QString projectPath() const { return m_projectPath; }
    Q_INVOKABLE void setProjectPath(const QString &projectPath);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE int importAssets(const QList<QUrl> &fileUrls);

signals:
    void projectPathChanged();
    void countChanged();
    void assetsImported(int importedCount, int skippedCount);

protected:
    virtual QString assetSubfolderName() const = 0;
    virtual QStringList fileExtensionFilters() const = 0;
    virtual void populateCustomMetadata(int index, const QFileInfo &info) { Q_UNUSED(index); Q_UNUSED(info); }

    QList<BaseAssetItem> m_baseAssets;

private:
    QString m_projectPath;
    void scanDirectoryInternal();
};
