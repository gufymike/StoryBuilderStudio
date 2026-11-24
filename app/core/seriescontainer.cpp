/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: seriescontainer.cpp
** Manages top-level television series and anthology manifests.
**
****************************************************************************/
#include "seriescontainer.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QDebug>
#include <QUrl>

SeriesContainer::SeriesContainer(QObject *parent)
    : QObject(parent)
    , m_creationDate(QDateTime::currentDateTimeUtc())
    , m_lastModified(QDateTime::currentDateTimeUtc())
{
    scanAvailableSeriesInternal();
}

static QString normalizeLocalPath(const QString &rawPath)
{
    const QString trimmed = rawPath.trimmed();
    if (trimmed.isEmpty()) return QString();

    QUrl url(trimmed);
    const QString local = url.isLocalFile() ? url.toLocalFile() : trimmed;
    return QDir::toNativeSeparators(QDir::cleanPath(local));
}

void SeriesContainer::setCustomProjectsRoot(const QString &path)
{
    m_customProjectsRoot = normalizeLocalPath(path);
    scanAvailableSeriesInternal();
}

QString SeriesContainer::projectsRootPath() const
{
    if (!m_customProjectsRoot.isEmpty()) {
        return m_customProjectsRoot;
    }
    if (QStandardPaths::isTestModeEnabled()) {
        const QString tempRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        return QDir(tempRoot).filePath(QStringLiteral("StoryBuilderStudioProjects_Test"));
    }
    const QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return QDir(homePath).filePath(QStringLiteral("StoryBuilderStudioProjects"));
}

void SeriesContainer::scanAvailableSeriesInternal()
{
    QDir projectsRoot(projectsRootPath());

    QStringList seriesDirs;
    if (projectsRoot.exists()) {
        const QStringList subDirs = projectsRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString &dirName : subDirs) {
            QDir candidate(projectsRoot.filePath(dirName));
            if (candidate.exists(QStringLiteral("season_manifest.json"))) {
                seriesDirs.append(candidate.absolutePath());
            }
        }
    }

    if (m_cachedAvailableSeries != seriesDirs) {
        m_cachedAvailableSeries = seriesDirs;
        emit availableSeriesChanged();
    }
}

void SeriesContainer::setName(const QString &val) {
    if (m_name != val) {
        m_name = val;
        setLastModified(QDateTime::currentDateTimeUtc());
        emit nameChanged();
    }
}

void SeriesContainer::setShortDescription(const QString &val) {
    if (m_shortDescription != val) {
        m_shortDescription = val;
        setLastModified(QDateTime::currentDateTimeUtc());
        emit shortDescriptionChanged();
    }
}

void SeriesContainer::setLongDescription(const QString &val) {
    if (m_longDescription != val) {
        m_longDescription = val;
        setLastModified(QDateTime::currentDateTimeUtc());
        emit longDescriptionChanged();
    }
}

void SeriesContainer::setLastModified(const QDateTime &dt) {
    if (m_lastModified != dt) {
        m_lastModified = dt;
        emit lastModifiedChanged();
    }
}

bool SeriesContainer::createNewContainer(const QString &folderPath, const QString &seriesName,
                                         const QString &shortDesc, const QString &longDesc)
{
    QString cleanPath = normalizeLocalPath(folderPath);
    if (cleanPath.isEmpty()) {
        emit manifestError(tr("Directory path cannot be empty."));
        return false;
    }

    QDir targetDir(cleanPath);
    if (!targetDir.exists()) {
        if (!targetDir.mkpath(QStringLiteral("."))) {
            emit manifestError(tr("Unable to create series directory: %1").arg(cleanPath));
            return false;
        }
    }

    targetDir.mkpath(QStringLiteral("episodes"));

    m_rootDir = cleanPath;
    m_manifestFilePath = targetDir.filePath(QStringLiteral("season_manifest.json"));
    m_name = seriesName;
    m_shortDescription = shortDesc;
    m_longDescription = longDesc;
    m_creationDate = QDateTime::currentDateTimeUtc();
    m_lastModified = m_creationDate;
    m_subProjects.clear();

    emit rootDirectoryChanged();
    emit episodeDirectoryChanged();
    emit nameChanged();
    emit shortDescriptionChanged();
    emit longDescriptionChanged();
    emit lastModifiedChanged();
    emit subProjectsChanged();

    return saveManifest();
}

bool SeriesContainer::loadFromDirectory(const QString &folderPath) {
    QString native = normalizeLocalPath(folderPath);
    if (native.isEmpty()) {
        m_rootDir.clear();
        m_manifestFilePath.clear();
        m_name.clear();
        m_shortDescription.clear();
        m_longDescription.clear();
        m_creator.clear();
        m_coCreators.clear();
        m_email.clear();
        m_subProjects.clear();

        emit rootDirectoryChanged();
        emit episodeDirectoryChanged();
        emit nameChanged();
        emit shortDescriptionChanged();
        emit longDescriptionChanged();
        emit subProjectsChanged();
        emit creatorChanged();
        emit coCreatorsChanged();
        emit emailChanged();
        return true;
    }

    QDir dir(native);
    if (!dir.exists()) {
        emit manifestError(tr("Directory does not exist: %1").arg(native));
        return false;
    }

    QString manifestPath = dir.filePath(QStringLiteral("season_manifest.json"));
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit manifestError(tr("Cannot open container manifest: %1").arg(manifestPath));
        return false;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
    file.close();

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit manifestError(tr("Invalid JSON structure in series manifest."));
        return false;
    }

    QJsonObject root = doc.object();
    m_rootDir = native;
    m_manifestFilePath = manifestPath;
    m_name = root.value(QStringLiteral("name")).toString();
    m_shortDescription = root.value(QStringLiteral("short_description")).toString();
    m_longDescription = root.value(QStringLiteral("long_description")).toString();
    m_creationDate = QDateTime::fromString(root.value(QStringLiteral("created_timestamp")).toString(), Qt::ISODateWithMs);
    m_lastModified = QDateTime::fromString(root.value(QStringLiteral("last_modified_timestamp")).toString(), Qt::ISODateWithMs);
    m_creator = root.value(QStringLiteral("creator")).toString();
    m_email = root.value(QStringLiteral("email")).toString();

    m_coCreators.clear();
    QJsonArray coArray = root.value(QStringLiteral("co_creators")).toArray();
    for (const auto &val : coArray) {
        m_coCreators.append(val.toString());
    }

    m_subProjects.clear();
    QJsonArray subs = root.value(QStringLiteral("sub_projects")).toArray();
    for (const auto &val : subs) {
        QJsonObject obj = val.toObject();
        SubProjectDescriptor desc;
        desc.id = obj.value(QStringLiteral("id")).toString();
        desc.relativePath = obj.value(QStringLiteral("relative_path")).toString();
        desc.title = obj.value(QStringLiteral("title")).toString();
        desc.displayOrder = obj.value(QStringLiteral("display_order")).toInt();
        m_subProjects.append(desc);
    }

    emit rootDirectoryChanged();
    emit episodeDirectoryChanged();
    emit nameChanged();
    emit shortDescriptionChanged();
    emit longDescriptionChanged();
    emit lastModifiedChanged();
    emit subProjectsChanged();
    emit creatorChanged();
    emit coCreatorsChanged();
    emit emailChanged();
    return true;
}

bool SeriesContainer::saveManifest() {
    if (m_manifestFilePath.isEmpty()) return false;

    QJsonObject root;
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("container_type")] = QStringLiteral("series_container");
    root[QStringLiteral("name")] = m_name;
    root[QStringLiteral("short_description")] = m_shortDescription;
    root[QStringLiteral("long_description")] = m_longDescription;
    root[QStringLiteral("created_timestamp")] = m_creationDate.toString(Qt::ISODateWithMs);
    root[QStringLiteral("last_modified_timestamp")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    root[QStringLiteral("sub_project_count")] = m_subProjects.size();
    root[QStringLiteral("creator")] = m_creator;
    root[QStringLiteral("email")] = m_email;

    QJsonArray coArray;
    for (const QString &c : m_coCreators) {
        coArray.append(c);
    }
    root[QStringLiteral("co_creators")] = coArray;

    QJsonArray subsArray;
    for (const auto &item : m_subProjects) {
        QJsonObject subObj;
        subObj[QStringLiteral("id")] = item.id;
        subObj[QStringLiteral("relative_path")] = item.relativePath;
        subObj[QStringLiteral("title")] = item.title;
        subObj[QStringLiteral("display_order")] = item.displayOrder;
        subsArray.append(subObj);
    }
    root[QStringLiteral("sub_projects")] = subsArray;

    QFile file(m_manifestFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        emit manifestError(tr("Unable to write manifest to disk: %1").arg(m_manifestFilePath));
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.flush();
    file.close();
    emit manifestChanged();
    return true;
}

bool SeriesContainer::addExistingSubProject(const QString &childProjectPath)
{
    if (m_rootDir.isEmpty() || childProjectPath.isEmpty()) {
        emit manifestError(tr("Container not initialized or invalid sub-project path."));
        return false;
    }

    QDir rootDir(m_rootDir);
    QDir childDir(childProjectPath);

    if (!childDir.exists()) {
        emit manifestError(tr("Target sub-project directory does not exist: %1").arg(childProjectPath));
        return false;
    }

    QString relative = rootDir.relativeFilePath(childDir.absolutePath());

    for (const auto &item : m_subProjects) {
        if (item.relativePath == relative) {
            emit manifestError(tr("Sub-project is already linked: %1").arg(relative));
            return false;
        }
    }

    SubProjectDescriptor desc;
    desc.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    desc.relativePath = relative;
    desc.title = childDir.dirName();
    desc.displayOrder = m_subProjects.size() + 1;

    m_subProjects.append(desc);
    setLastModified(QDateTime::currentDateTimeUtc());
    emit subProjectsChanged();

    return saveManifest();
}

QString SeriesContainer::createNewSubProject(const QString &childSlugName, const QString &title)
{
    if (m_rootDir.isEmpty() || childSlugName.isEmpty()) {
        emit manifestError(tr("Container not initialized or empty sub-project name."));
        return QString();
    }

    QDir rootDir(m_rootDir);
    QString subDirPath = rootDir.filePath(QStringLiteral("episodes/") + childSlugName);
    QDir subDir(subDirPath);

    if (subDir.exists()) {
        emit manifestError(tr("A sub-project directory with that name already exists."));
        return QString();
    }

    if (!subDir.mkpath(QStringLiteral(".")) ||
        !subDir.mkpath(QStringLiteral("assets/images")) ||
        !subDir.mkpath(QStringLiteral("assets/audio")) ||
        !subDir.mkpath(QStringLiteral("assets/video"))) {
        emit manifestError(tr("Failed to create sub-project directories at: %1").arg(subDirPath));
        return QString();
    }

    QJsonObject meta;
    meta[QStringLiteral("project_name")] = title.isEmpty() ? childSlugName : title;
    meta[QStringLiteral("created_timestamp")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    QFile metaFile(subDir.filePath(QStringLiteral("project.meta")));
    if (metaFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        metaFile.write(QJsonDocument(meta).toJson(QJsonDocument::Indented));
        metaFile.flush();
        metaFile.close();
    }

    SubProjectDescriptor desc;
    desc.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    desc.relativePath = rootDir.relativeFilePath(subDirPath);
    desc.title = title.isEmpty() ? childSlugName : title;
    desc.displayOrder = m_subProjects.size() + 1;

    m_subProjects.append(desc);
    setLastModified(QDateTime::currentDateTimeUtc());
    emit subProjectsChanged();

    saveManifest();
    return subDir.absolutePath();
}

void SeriesContainer::removeSubProject(int index)
{
    if (index < 0 || index >= m_subProjects.size()) {
        return;
    }

    m_subProjects.removeAt(index);

    for (int i = 0; i < m_subProjects.size(); ++i) {
        m_subProjects[i].displayOrder = i + 1;
    }

    setLastModified(QDateTime::currentDateTimeUtc());
    emit subProjectsChanged();
    saveManifest();
}

bool SeriesContainer::registerSubProject(const QString &folderName, const QString &title) {
    if (m_rootDir.isEmpty()) return false;

    SubProjectDescriptor desc;
    desc.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    desc.relativePath = QStringLiteral("episodes/") + folderName;
    desc.title = title.isEmpty() ? folderName : title;
    desc.displayOrder = m_subProjects.size() + 1;

    m_subProjects.append(desc);
    emit subProjectsChanged();
    return saveManifest();
}

QString SeriesContainer::getSubProjectAbsolutePath(int index) const
{
    if (index < 0 || index >= m_subProjects.size() || m_rootDir.isEmpty()) {
        return QString();
    }
    QDir root(m_rootDir);
    return root.absoluteFilePath(m_subProjects.at(index).relativePath);
}

void SeriesContainer::setCreator(const QString &val) {
    if (m_creator != val) {
        m_creator = val;
        setLastModified(QDateTime::currentDateTimeUtc());
        emit creatorChanged();
    }
}

void SeriesContainer::setCoCreators(const QStringList &val) {
    if (m_coCreators != val) {
        m_coCreators = val;
        setLastModified(QDateTime::currentDateTimeUtc());
        emit coCreatorsChanged();
    }
}

void SeriesContainer::setEmail(const QString &val) {
    if (m_email != val) {
        m_email = val;
        setLastModified(QDateTime::currentDateTimeUtc());
        emit emailChanged();
    }
}


void SeriesContainer::refreshAvailableSeries()
{
    scanAvailableSeriesInternal();
}

QString SeriesContainer::getSubProjectTitle(int index) const
{
    if (index < 0 || index >= m_subProjects.size()) {
        return QString();
    }
    return m_subProjects.at(index).title;
}

bool SeriesContainer::deleteSeriesDirectory(const QString &seriesPath)
{
    QString trimmed = seriesPath.trimmed();
    if (trimmed.isEmpty()) return false;

    if (trimmed.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)) {
        trimmed = QUrl(trimmed).toLocalFile();
    }

    const QString cleanPath = QDir::cleanPath(QDir::fromNativeSeparators(trimmed));
    QFileInfo fi(cleanPath);
    if (!fi.exists() || !fi.isDir()) {
        emit manifestError(tr("Series directory does not exist: %1").arg(cleanPath));
        return false;
    }

    // Safety guard: never allow deleting filesystem roots, user home/documents, or the parent projects container
    QDir targetDir(cleanPath);
    const QString homePath = QDir::cleanPath(QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)));
    const QString docsPath = QDir::cleanPath(QDir::fromNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    if (targetDir.isRoot() ||
        cleanPath.compare(homePath, Qt::CaseInsensitive) == 0 ||
        cleanPath.compare(docsPath, Qt::CaseInsensitive) == 0 ||
        cleanPath.endsWith(QStringLiteral("/StoryBuilderStudioProjects"), Qt::CaseInsensitive)) {
        emit manifestError(tr("Refusing to delete protected directory: %1").arg(cleanPath));
        return false;
    }

    // Verify this directory is a known series or contains a series manifest before recursive removal
    bool isKnownSeries = false;
    for (const QString &known : std::as_const(m_cachedAvailableSeries)) {
        QString knownClean = known.startsWith(QStringLiteral("file://"), Qt::CaseInsensitive)
        ? QUrl(known).toLocalFile()
        : known;
        if (QDir::cleanPath(QDir::fromNativeSeparators(knownClean)).compare(cleanPath, Qt::CaseInsensitive) == 0) {
            isKnownSeries = true;
            break;
        }
    }

    const bool hasManifest = QFile::exists(targetDir.filePath(QStringLiteral("season_manifest.json"))) ||
                             QFile::exists(targetDir.filePath(QStringLiteral("series.json"))) ||
                             QFile::exists(targetDir.filePath(QStringLiteral("manifest.json"))) ||
                             QFile::exists(targetDir.filePath(QStringLiteral("series_manifest.json")));
    if (!isKnownSeries && !hasManifest) {
        emit manifestError(tr("Directory is not a recognized series folder: %1").arg(cleanPath));
        return false;
    }

    // If this series is currently active, clear active root first so file handles are released
    if (!m_rootDir.isEmpty() &&
        QDir::cleanPath(QDir::fromNativeSeparators(m_rootDir)).compare(cleanPath, Qt::CaseInsensitive) == 0) {
        m_rootDir.clear();
        emit rootDirectoryChanged();
    }

    if (!targetDir.removeRecursively()) {
        emit manifestError(tr("Failed to completely delete series directory (check open files): %1").arg(cleanPath));
        return false;
    }

    qInfo() << "[SeriesContainer] Deleted series directory from disk:" << cleanPath;
    refreshAvailableSeries();
    return true;
}
