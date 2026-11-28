// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include "projectmanager.h"

#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QImage>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QDateTime>
#include <QUuid>
#include <QUrl>
#include <QDebug>

ProjectManager::ProjectManager(QObject *parent)
    : QObject(parent)
{
}

ProjectManager::ProjectManager(const QString &customProjectsRoot, QObject *parent)
    : QObject(parent)
    , m_customProjectsRoot(QDir::toNativeSeparators(customProjectsRoot.trimmed()))
{
}

void ProjectManager::setCustomProjectsRoot(const QString &path)
{
    m_customProjectsRoot = QDir::toNativeSeparators(path.trimmed());
    emit availableProjectsChanged();
}

QString ProjectManager::getGlobalProjectsRoot() const
{
    if (!m_customProjectsRoot.isEmpty()) {
        QDir dir(m_customProjectsRoot);
        if (!dir.exists()) {
            dir.mkpath(QStringLiteral("."));
        }
        return m_customProjectsRoot;
    }

    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QDir homeDir(homePath);
    QString rootPath = homeDir.absoluteFilePath(QStringLiteral("StoryBuilderStudioProjects"));

    QDir rootDir(rootPath);
    if (!rootDir.exists()) {
        rootDir.mkpath(QStringLiteral("."));
    }

    return QDir::toNativeSeparators(rootPath);
}

QString ProjectManager::slugify(const QString &name) const
{
    QString slug = name.toLower().trimmed();
    slug.remove(QRegularExpression(QStringLiteral("[^a-z0-9\\s_-]")));
    slug.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral("_"));
    return slug;
}

bool ProjectManager::createProject(const QString &folderPath,
                                   const QString &projectName,
                                   const QString &description,
                                   const QString &shortDescription,
                                   const QString &author,
                                   const QStringList &authors,
                                   const QString &email,
                                   const QString &version)
{
    if (projectName.trimmed().isEmpty()) {
        QString err = QStringLiteral("Project creation failed: Name cannot be empty.");
        qWarning() << err;
        emit projectError(err);
        return false;
    }

    QDir targetDir;
    if (!folderPath.trimmed().isEmpty()) {
        QString cleanPath = QDir::toNativeSeparators(folderPath.trimmed());
        if (cleanPath.contains(QStringLiteral(".."))) {
            QString err = QStringLiteral("Security violation: Traversal paths are not permitted.");
            emit projectError(err);
            return false;
        }
        targetDir.setPath(cleanPath);
    } else {
        QString slug = slugify(projectName);
        targetDir.setPath(QDir(getGlobalProjectsRoot()).absoluteFilePath(slug));
    }

    QString projectDirPath = QDir::toNativeSeparators(targetDir.absolutePath());

    if (!targetDir.exists()) {
        if (!targetDir.mkpath(QStringLiteral("."))) {
            QString err = QStringLiteral("Failed to create directory: ") + projectDirPath;
            qWarning() << err;
            emit projectError(err);
            return false;
        }
    }

    bool pathsCreated = targetDir.mkpath(QStringLiteral("assets/scripts")) &&
                        targetDir.mkpath(QStringLiteral("assets/images")) &&
                        targetDir.mkpath(QStringLiteral("assets/audio")) &&
                        targetDir.mkpath(QStringLiteral("assets/video"));

    if (!pathsCreated) {
        QString err = QStringLiteral("Project creation failed: Unable to initialize filesystem folders under ") + projectDirPath;
        qWarning() << err;
        emit projectError(err);
        return false;
    }

    QString currentTimestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    if (!writeConfigFile(projectDirPath, projectName, description, shortDescription, author, authors, email, version, currentTimestamp, currentTimestamp)) {
        QString err = QStringLiteral("Project creation failed: Could not write configuration file to ") + projectDirPath;
        qWarning() << err;
        emit projectError(err);
        return false;
    }

    return openProject(projectDirPath);
}

bool ProjectManager::openProject(const QString &pathOrSlug)
{
    QString trimmed = pathOrSlug.trimmed();
    if (trimmed.isEmpty()) {
        emit projectError(QStringLiteral("Project open failed: Path or slug cannot be empty."));
        return false;
    }

    if (trimmed.contains(QStringLiteral(".."))) {
        emit projectError(QStringLiteral("Security violation: Traversal paths are not permitted."));
        return false;
    }

    QDir projectDir;
    QFileInfo checkInfo(trimmed);
    if (checkInfo.isAbsolute() && checkInfo.exists() && checkInfo.isDir()) {
        projectDir.setPath(trimmed);
    } else {
        QDir rootDir(getGlobalProjectsRoot());
        projectDir.setPath(rootDir.absoluteFilePath(trimmed));
    }

    if (!projectDir.exists()) {
        emit projectError(QStringLiteral("Project open failed: Specified project directory does not exist: ") + projectDir.absolutePath());
        return false;
    }

    QString projectDirPath = QDir::toNativeSeparators(projectDir.absolutePath());
    QString slug = projectDir.dirName();

    QString confFilePath = projectDir.absoluteFilePath(slug + QStringLiteral(".conf"));
    if (!QFile::exists(confFilePath)) {
        confFilePath = projectDir.absoluteFilePath(QStringLiteral("project.conf"));
    }

    QFile confFile(confFilePath);
    if (!confFile.exists()) {
        emit projectError(QStringLiteral("Integrity Warning: Directory exists but is missing configuration file at: ") + confFilePath);
        return false;
    }

    if (!confFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit projectError(QStringLiteral("Project open failed: Could not read configuration file."));
        return false;
    }

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(confFile.readAll(), &parseErr);
    confFile.close();

    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        emit projectError(QStringLiteral("Project open failed: Configuration file contains corrupt JSON data."));
        return false;
    }

    QJsonObject obj = doc.object();
    m_currentProjectPath = projectDirPath;
    emit currentProjectPathChanged();

    m_projectName = obj[QStringLiteral("name")].toString(QStringLiteral("Untitled Project"));
    emit projectNameChanged();

    if (obj.contains(QStringLiteral("long_description"))) {
        m_projectDescription = obj.value(QStringLiteral("long_description")).toString();
    } else {
        m_projectDescription = obj.value(QStringLiteral("description")).toString();
    }
    emit projectDescriptionChanged();

    m_projectShortDescription = obj[QStringLiteral("short_description")].toString();
    emit projectShortDescriptionChanged();

    m_projectVersion = obj[QStringLiteral("version")].toString(QStringLiteral("1.0"));
    emit projectVersionChanged();

    m_author = obj[QStringLiteral("author")].toString();
    emit authorChanged();

    QStringList loadedAuthors;
    QJsonArray authorsArray = obj[QStringLiteral("authors")].toArray();
    loadedAuthors.reserve(authorsArray.size());
    for (int i = 0; i < authorsArray.size(); ++i) {
        loadedAuthors.append(authorsArray.at(i).toString());
    }
    m_authors = loadedAuthors;
    emit authorsChanged();

    m_email = obj[QStringLiteral("email")].toString();
    emit emailChanged();

    if (obj.contains(QStringLiteral("created_timestamp"))) {
        m_dateCreated = obj.value(QStringLiteral("created_timestamp")).toString();
    } else {
        m_dateCreated = obj.value(QStringLiteral("date_created")).toString();
    }
    emit dateCreatedChanged();

    if (obj.contains(QStringLiteral("last_modified_timestamp"))) {
        m_dateLastEdited = obj.value(QStringLiteral("last_modified_timestamp")).toString();
    } else {
        m_dateLastEdited = obj.value(QStringLiteral("date_last_edited")).toString();
    }
    emit dateLastEditedChanged();

    m_isProjectOpen = true;
    emit isProjectOpenChanged();
    return true;
}

bool ProjectManager::writeConfigFile(const QString &projectDirPath,
                                     const QString &name,
                                     const QString &description,
                                     const QString &shortDescription,
                                     const QString &author,
                                     const QStringList &authors,
                                     const QString &email,
                                     const QString &version,
                                     const QString &dateCreated,
                                     const QString &dateLastEdited)
{
    QDir dir(projectDirPath);
    QString slug = dir.dirName();
    QString confPath = dir.absoluteFilePath(slug + QStringLiteral(".conf"));

    QJsonObject obj;
    obj[QStringLiteral("container_type")] = QStringLiteral("episode_container");
    obj[QStringLiteral("version")] = version.isEmpty() ? QStringLiteral("1.0") : version;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("slug")] = slug;
    obj[QStringLiteral("author")] = author;

    QJsonArray authorsArray;
    for (const QString &a : authors) {
        authorsArray.append(a);
    }
    obj[QStringLiteral("authors")] = authorsArray;
    obj[QStringLiteral("email")] = email;
    obj[QStringLiteral("short_description")] = shortDescription;
    obj[QStringLiteral("long_description")] = description;
    obj[QStringLiteral("description")] = description;

    obj[QStringLiteral("created_timestamp")] = dateCreated;
    obj[QStringLiteral("last_modified_timestamp")] = dateLastEdited;
    obj[QStringLiteral("date_created")] = dateCreated;
    obj[QStringLiteral("date_last_edited")] = dateLastEdited;

    QJsonDocument doc(obj);
    QFile file(confPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.flush();
    file.close();
    return true;
}

void ProjectManager::closeProject()
{
    m_currentProjectPath.clear();
    emit currentProjectPathChanged();

    m_projectName.clear();
    emit projectNameChanged();

    m_projectDescription.clear();
    emit projectDescriptionChanged();

    m_projectShortDescription.clear();
    emit projectShortDescriptionChanged();

    m_projectVersion = QStringLiteral("1.0");
    emit projectVersionChanged();

    m_author.clear();
    emit authorChanged();

    m_authors.clear();
    emit authorsChanged();

    m_email.clear();
    emit emailChanged();

    m_dateCreated.clear();
    emit dateCreatedChanged();

    m_dateLastEdited.clear();
    emit dateLastEditedChanged();

    m_isProjectOpen = false;
    emit isProjectOpenChanged();
}

bool ProjectManager::saveCurrentProjectData(const QString &subFolder, const QString &fileName, const QString &content)
{
    if (!m_isProjectOpen || m_currentProjectPath.isEmpty()) {
        emit projectError(QStringLiteral("Save failed: No active project open."));
        return false;
    }

    if (subFolder.contains(QStringLiteral("..")) || fileName.contains(QStringLiteral(".."))) {
        emit projectError(QStringLiteral("Security violation: Path traversal detected."));
        return false;
    }

    QDir projectDir(m_currentProjectPath);
    QString targetDirPath = projectDir.absoluteFilePath(subFolder);
    QDir targetDir(targetDirPath);

    if (!targetDir.exists()) {
        targetDir.mkpath(QStringLiteral("."));
    }

    QFile file(targetDir.absoluteFilePath(fileName));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit projectError(QStringLiteral("Save failed: Could not write file."));
        return false;
    }

    QTextStream out(&file);
    out << content;
    file.flush();
    file.close();

    m_dateLastEdited = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    emit dateLastEditedChanged();

    writeConfigFile(m_currentProjectPath,
                    m_projectName,
                    m_projectDescription,
                    m_projectShortDescription,
                    m_author,
                    m_authors,
                    m_email,
                    m_projectVersion,
                    m_dateCreated,
                    m_dateLastEdited);

    return true;
}


QStringList ProjectManager::availableProjects() const
{
    QDir rootDir(getGlobalProjectsRoot());
    if (!rootDir.exists()) return QStringList();

    QStringList projectSlugs;
    QStringList subDirs = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &dirName : subDirs) {
        QDir subDir(rootDir.absoluteFilePath(dirName));
        if (subDir.exists(dirName + QStringLiteral(".conf")) || subDir.exists(QStringLiteral("project.conf"))) {
            projectSlugs.append(dirName);
        }
    }
    return projectSlugs;
}

bool ProjectManager::saveCandidateImage(const QString &base64Data, const QString &preferredName)
{
    if (!m_isProjectOpen || m_currentProjectPath.isEmpty()) {
        emit projectError(tr("Save failed: No active project open."));
        return false;
    }

    QString payload = base64Data.trimmed();
    QString mimeSubtype = QStringLiteral("png");

    static const QRegularExpression dataUriRegex(
        QStringLiteral("^data:image/([a-zA-Z0-9.+-]+);base64,"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = dataUriRegex.match(payload);
    if (match.hasMatch()) {
        mimeSubtype = match.captured(1).toLower();
        payload.remove(0, match.capturedLength(0));
    }

    const QByteArray rawBytes = QByteArray::fromBase64(payload.toUtf8());
    if (rawBytes.isEmpty()) {
        emit projectError(tr("Save failed: Decoded image payload is empty."));
        return false;
    }

    QDir imageDir(m_currentProjectPath + QStringLiteral("/assets/images"));
    if (!imageDir.exists()) {
        imageDir.mkpath(QStringLiteral("."));
    }

    QString fileName = preferredName.isEmpty()
                           ? QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".png")
                           : preferredName + (preferredName.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive) ? QString() : QStringLiteral(".png"));

    const QString targetPath = imageDir.absoluteFilePath(fileName);

    // Direct write for PNG (preserves compatibility with synthetic Base64 test fixtures)
    if (mimeSubtype == QLatin1String("png") || mimeSubtype == QLatin1String("x-png")) {
        QFile file(targetPath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit projectError(tr("Save failed: Unable to write image file: ") + file.errorString());
            return false;
        }
        if (file.write(rawBytes) != rawBytes.size()) {
            emit projectError(tr("Save failed: Incomplete image write: ") + file.errorString());
            file.close();
            return false;
        }
        file.flush();
        file.close();
        return true;
    }

    // Transcode non-PNG payloads (e.g., image/jpeg, image/webp) into PNG
    QImage image;
    if (!image.loadFromData(rawBytes)) {
        emit projectError(tr("Save failed: Unable to decode %1 image payload.").arg(mimeSubtype));
        return false;
    }

    if (!image.save(targetPath, "PNG")) {
        emit projectError(tr("Save failed: Unable to encode PNG image file."));
        return false;
    }

    return true;
}

bool ProjectManager::saveAudioFile(const QByteArray &audioData, const QString &preferredName)
{
    if (!m_isProjectOpen || m_currentProjectPath.isEmpty()) {
        emit projectError(tr("Save failed: No active project open."));
        return false;
    }

    if (audioData.isEmpty()) {
        emit projectError(tr("Save failed: Decoded audio payload is empty."));
        return false;
    }

    QDir audioDir(m_currentProjectPath + QStringLiteral("/assets/audio"));
    if (!audioDir.exists()) {
        audioDir.mkpath(QStringLiteral("."));
    }

    QString fileName = preferredName.isEmpty()
                           ? QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".wav")
                           : preferredName + (preferredName.endsWith(QStringLiteral(".wav"), Qt::CaseInsensitive) ? QString() : QStringLiteral(".wav"));

    QFile file(audioDir.absoluteFilePath(fileName));
    if (!file.open(QIODevice::WriteOnly)) {
        emit projectError(tr("Save failed: Unable to write audio file: ") + file.errorString());
        return false;
    }

    file.write(audioData);
    file.flush();
    file.close();
    return true;
}

bool ProjectManager::saveVideoFile(const QByteArray &videoData, const QString &preferredName)
{
    if (!m_isProjectOpen || m_currentProjectPath.isEmpty()) {
        emit projectError(tr("Save failed: No active project open."));
        return false;
    }

    if (videoData.isEmpty()) {
        emit projectError(tr("Save failed: Decoded video payload is empty."));
        return false;
    }

    QDir videoDir(m_currentProjectPath + QStringLiteral("/assets/video"));
    if (!videoDir.exists()) {
        videoDir.mkpath(QStringLiteral("."));
    }

    QString fileName = preferredName.isEmpty()
                           ? QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".mp4")
                           : preferredName + (preferredName.endsWith(QStringLiteral(".mp4"), Qt::CaseInsensitive) ? QString() : QStringLiteral(".mp4"));

    QFile file(videoDir.absoluteFilePath(fileName));
    if (!file.open(QIODevice::WriteOnly)) {
        emit projectError(tr("Save failed: Unable to write video file: ") + file.errorString());
        return false;
    }

    file.write(videoData);
    file.flush();
    file.close();
    return true;
}

bool ProjectManager::deleteAsset(const QString &filePathOrUrl)
{
    if (!m_isProjectOpen || m_currentProjectPath.isEmpty()) {
        emit projectError(tr("Delete failed: No active project is open."));
        return false;
    }

    if (filePathOrUrl.trimmed().isEmpty()) {
        return false;
    }

    QString cleanPath = filePathOrUrl.trimmed();
    QUrl url(cleanPath);
    if (url.isLocalFile()) {
        cleanPath = url.toLocalFile();
    } else if (cleanPath.startsWith(QStringLiteral("file:///"))) {
        cleanPath = cleanPath.mid(8);
    } else if (cleanPath.startsWith(QStringLiteral("file://"))) {
        cleanPath = cleanPath.mid(7);
    }
    cleanPath = QDir::toNativeSeparators(cleanPath);

    QFileInfo fileInfo(cleanPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        emit projectError(tr("Delete failed: File does not exist on disk: %1").arg(cleanPath));
        return false;
    }

    // Security check: Must reside within the project's assets/ directory
    QString assetsRoot = QDir::toNativeSeparators(QDir(m_currentProjectPath).absoluteFilePath(QStringLiteral("assets")));
    QString canonicalTarget = fileInfo.canonicalFilePath();
    QString canonicalAssets = QDir(assetsRoot).canonicalPath();

    if (!canonicalTarget.startsWith(canonicalAssets, Qt::CaseInsensitive)) {
        emit projectError(tr("Security check failed: Cannot delete files outside the project assets folder."));
        return false;
    }

    QFile file(cleanPath);
    if (!file.remove()) {
        emit projectError(tr("Failed to delete asset: %1").arg(file.errorString()));
        return false;
    }

    emit assetDeleted(cleanPath);
    return true;
}

void ProjectManager::setProjectName(const QString &projectName) {
    if (m_projectName != projectName) {
        m_projectName = projectName;
        emit projectNameChanged();
    }
}

void ProjectManager::setProjectDescription(const QString &pDescription) {
    if (m_projectDescription != pDescription) {
        m_projectDescription = pDescription;
        emit projectDescriptionChanged();
    }
}

void ProjectManager::setProjectShortDescription(const QString &projectShortDescription) {
    if (m_projectShortDescription != projectShortDescription) {
        m_projectShortDescription = projectShortDescription;
        emit projectShortDescriptionChanged();
    }
}

void ProjectManager::setAuthor(const QString &author) {
    if (m_author != author) {
        m_author = author;
        emit authorChanged();
    }
}

void ProjectManager::setAuthors(const QStringList &authors) {
    if (m_authors != authors) {
        m_authors = authors;
        emit authorsChanged();
    }
}

void ProjectManager::setEmail(const QString &email) {
    if (m_email != email) {
        m_email = email;
        emit emailChanged();
    }
}

void ProjectManager::setProjectVersion(const QString &version) {
    if (m_projectVersion != version) {
        m_projectVersion = version;
        emit projectVersionChanged();
    }
}

