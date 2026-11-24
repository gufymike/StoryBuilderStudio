/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: seriescontainer.h
** Manages top-level television series and anthology manifests.
**
****************************************************************************/
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QtQml/QQmlEngine>
#include <QDir>

struct SubProjectDescriptor {
    QString id;
    QString relativePath;
    QString title;
    int displayOrder = 0;
};

/**
 * @class SeriesContainer
 * @brief Manages a top-level television series container, tracking sub-projects (episodes).
 *
 * Exposes a QML interface to create, load, and manage a series-level JSON manifest.
 * A SeriesContainer acts as the parent umbrella for multiple ProjectManager instances,
 * maintaining the display order and metadata for a complete season or anthology.
 */
class SeriesContainer : public QObject {
    Q_OBJECT
    QML_ELEMENT

    /**
     * @property name
     * @brief The display title of the series/season.
     * @notifier nameChanged
     */
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

    /**
     * @property subProjectCount
     * @brief The total number of tracked episodes/sub-projects within this series.
     * @notifier subProjectsChanged
     */
    Q_PROPERTY(int subProjectCount READ subProjectCount NOTIFY subProjectsChanged)
    /**
     * @property shortDescription
     * @brief A tagline or small description for list displays.
     * @notifier shortDescriptionChanged
     */
    Q_PROPERTY(QString shortDescription READ shortDescription WRITE setShortDescription NOTIFY shortDescriptionChanged)
    /**
     * @property longDescription
     * @brief A long description or bible for the project
     * @notifier longDescriptionChanged
     */
    Q_PROPERTY(QString longDescription READ longDescription WRITE setLongDescription NOTIFY longDescriptionChanged)
    Q_PROPERTY(QDateTime creationDate READ creationDate CONSTANT)
    Q_PROPERTY(QDateTime lastModified READ lastModified WRITE setLastModified NOTIFY lastModifiedChanged)



    Q_PROPERTY(QString rootDirectory READ rootDirectory NOTIFY rootDirectoryChanged)
    Q_PROPERTY(QString episodesDirectory READ episodesDirectory NOTIFY episodeDirectoryChanged)
    Q_PROPERTY(QString creator READ creator WRITE setCreator NOTIFY creatorChanged)
    Q_PROPERTY(QStringList coCreators READ coCreators WRITE setCoCreators NOTIFY coCreatorsChanged)
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)

    // Cached library of existing series on disk
    Q_PROPERTY(QStringList availableSeries READ availableSeries NOTIFY availableSeriesChanged)
    Q_PROPERTY(int availableSeriesCount READ availableSeriesCount NOTIFY availableSeriesChanged)

public:
    explicit SeriesContainer(QObject *parent = nullptr);
    ~SeriesContainer() override = default;

    // Getters
    QString name() const { return m_name; }
    QString shortDescription() const { return m_shortDescription; }
    QString longDescription() const { return m_longDescription; }
    QDateTime creationDate() const { return m_creationDate; }
    QDateTime lastModified() const { return m_lastModified; }
    int subProjectCount() const { return m_subProjects.size(); }
    QString rootDirectory() const { return m_rootDir; }
    QString episodesDirectory() const {
        if (m_rootDir.isEmpty()) return QString();
        return QDir(m_rootDir).filePath(QStringLiteral("episodes"));
    }
    QString creator() const { return m_creator; }
    QStringList coCreators() const { return m_coCreators; }
    QString email() const { return m_email; }

    QStringList availableSeries() const { return m_cachedAvailableSeries; }
    int availableSeriesCount() const { return m_cachedAvailableSeries.size(); }

    // Setters
    void setName(const QString &val);
    void setShortDescription(const QString &val);
    void setLongDescription(const QString &val);
    void setLastModified(const QDateTime &dt);
    void setCreator(const QString &val);
    void setCoCreators(const QStringList &val);
    void setEmail(const QString &val);

    Q_INVOKABLE bool createNewContainer(const QString &folderPath, const QString &seriesName,
                                        const QString &shortDesc, const QString &longDesc);
    Q_INVOKABLE bool loadFromDirectory(const QString &folderPath);
    Q_INVOKABLE bool saveManifest();

    // Sub-Project Management
    Q_INVOKABLE bool addExistingSubProject(const QString &childProjectPath);
    Q_INVOKABLE QString createNewSubProject(const QString &childSlugName, const QString &title);
    Q_INVOKABLE void removeSubProject(int index);
    Q_INVOKABLE bool registerSubProject(const QString &folderName, const QString &title);
    Q_INVOKABLE QString getSubProjectAbsolutePath(int index) const;
    Q_INVOKABLE QString getSubProjectTitle(int index) const;
    Q_INVOKABLE void refreshAvailableSeries();
    Q_INVOKABLE bool deleteSeriesDirectory(const QString &seriesPath);
    Q_INVOKABLE void setCustomProjectsRoot(const QString &path);
    QString projectsRootPath() const;
signals:
    void nameChanged();
    void shortDescriptionChanged();
    void longDescriptionChanged();
    void lastModifiedChanged();
    void subProjectsChanged();
    void rootDirectoryChanged();
    void episodeDirectoryChanged();
    void manifestError(const QString &errorMessage);
    void creatorChanged();
    void coCreatorsChanged();
    void emailChanged();
    void availableSeriesChanged();
    void manifestChanged();

private:
    void scanAvailableSeriesInternal();

    QString m_rootDir;
    QString m_manifestFilePath;
    QString m_name;
    QString m_shortDescription;
    QString m_longDescription;
    QDateTime m_creationDate;
    QDateTime m_lastModified;
    QList<SubProjectDescriptor> m_subProjects;
    QString m_creator;
    QStringList m_coCreators;
    QString m_email;
    QStringList m_cachedAvailableSeries;
    QString m_customProjectsRoot;
};
