// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QJsonObject>
#include <QtQml/QQmlEngine>

/**
 * @class ProjectManager
 * @brief Manages project lifecycle, folder scaffolding, asset serialization, and configurations.
 *
 * Exposes a QML-friendly API to create, open, save, and close narrative projects.
 * It automatically handles directory scaffolding (assets/scripts, assets/images, etc.)
 * and secures file I/O operations against path traversal vulnerabilities.
 */
class ProjectManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    /**
     * @property currentProjectPath
     * @brief Absolute path to the currently active project directory.
     * @notifier currentProjectPathChanged
     */
    Q_PROPERTY(QString currentProjectPath READ currentProjectPath NOTIFY currentProjectPathChanged)

    /**
     * @property projectName
     * @brief The display name of the currently active project.
     * @notifier projectNameChanged
     */
    Q_PROPERTY(QString projectName READ projectName WRITE setProjectName NOTIFY projectNameChanged)
    Q_PROPERTY(QString projectDescription READ projectDescription WRITE setProjectDescription NOTIFY projectDescriptionChanged)
    Q_PROPERTY(QString projectShortDescription READ projectShortDescription WRITE setProjectShortDescription NOTIFY projectShortDescriptionChanged)
    Q_PROPERTY(QString projectVersion READ projectVersion WRITE setProjectVersion NOTIFY projectVersionChanged)
    Q_PROPERTY(QString author READ author WRITE setAuthor NOTIFY authorChanged)
    Q_PROPERTY(QStringList authors READ authors WRITE setAuthors NOTIFY authorsChanged)
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QString dateCreated READ dateCreated NOTIFY dateCreatedChanged)
    Q_PROPERTY(QString dateLastEdited READ dateLastEdited NOTIFY dateLastEditedChanged)
    Q_PROPERTY(bool isProjectOpen READ isProjectOpen NOTIFY isProjectOpenChanged)
    Q_PROPERTY(QStringList availableProjects READ availableProjects NOTIFY availableProjectsChanged)

public:
    explicit ProjectManager(QObject *parent = nullptr);
    explicit ProjectManager(const QString &customProjectsRoot, QObject *parent = nullptr);
    ~ProjectManager() override = default;

    /**
     * @brief Creates a new project, scaffolds the folder structure, and writes the configuration.
     * @param folderPath The target absolute directory for the new project.
     * @param projectName Display name.
     * @param description Long-form project description.
     * @param shortDescription Brief summary or logline.
     * @param author Primary author name.
     * @param authors List of co-authors.
     * @param email Contact email.
     * @param version Project version string.
     * @return True if scaffolding and configuration writing succeeded, false otherwise.
     */
    Q_INVOKABLE bool createProject(const QString &folderPath,
                                   const QString &projectName,
                                   const QString &description,
                                   const QString &shortDescription,
                                   const QString &author,
                                   const QStringList &authors,
                                   const QString &email,
                                   const QString &version = QStringLiteral("1.0"));

    /**
     * @brief Opens an existing project and loads its configuration into memory.
     * @param pathOrSlug Absolute path or folder name within the global projects root.
     * @return True if opened successfully, false if path is invalid or metadata is missing.
     */
    Q_INVOKABLE bool openProject(const QString &pathOrSlug);
    Q_INVOKABLE void closeProject();
    Q_INVOKABLE bool saveCurrentProjectData(const QString &subFolder, const QString &fileName, const QString &content);
    Q_INVOKABLE QStringList availableProjects() const;
    Q_INVOKABLE bool saveCandidateImage(const QString &base64Data, const QString &preferredName = QString());
    Q_INVOKABLE bool saveAudioFile(const QByteArray &audioData, const QString &preferredName = QString());
    Q_INVOKABLE bool saveVideoFile(const QByteArray &videoData, const QString &preferredName = QString());
    Q_INVOKABLE bool deleteAsset(const QString &filePathOrUrl);

    Q_INVOKABLE void setProjectName(const QString &projectName);
    Q_INVOKABLE void setProjectDescription(const QString &pDescription);
    Q_INVOKABLE void setProjectShortDescription(const QString &projectShortDescription);
    Q_INVOKABLE void setAuthor(const QString &author);
    Q_INVOKABLE void setAuthors(const QStringList &authors);
    Q_INVOKABLE void setEmail(const QString &email);
    Q_INVOKABLE void setProjectVersion(const QString &version);

    // Overrides global projects root for testing isolation
    void setCustomProjectsRoot(const QString &path);
    QString getGlobalProjectsRoot() const;

    QString currentProjectPath() const { return m_currentProjectPath; }
    QString projectName() const { return m_projectName; }
    QString projectDescription() const { return m_projectDescription; }
    QString projectShortDescription() const { return m_projectShortDescription; }
    QString projectVersion() const { return m_projectVersion; }
    QString author() const { return m_author; }
    QStringList authors() const { return m_authors; }
    QString email() const { return m_email; }
    QString dateCreated() const { return m_dateCreated; }
    QString dateLastEdited() const { return m_dateLastEdited; }
    bool isProjectOpen() const { return m_isProjectOpen; }

signals:
    void currentProjectPathChanged();
    void projectNameChanged();
    void projectDescriptionChanged();
    void projectShortDescriptionChanged();
    void projectVersionChanged();
    void authorChanged();
    void authorsChanged();
    void emailChanged();
    void dateCreatedChanged();
    void dateLastEditedChanged();
    void isProjectOpenChanged();
    void projectError(const QString &errorMsg);
    void availableProjectsChanged();
    void assetDeleted(const QString &filePath);

private:
    QString slugify(const QString &name) const;
    bool writeConfigFile(const QString &projectDirPath, const QString &name, const QString &description, const QString &shortDescription, const QString &author, const QStringList &authors, const QString &email, const QString &version, const QString &dateCreated, const QString &dateLastEdited);


    QString m_customProjectsRoot;
    QString m_currentProjectPath;
    QString m_projectName;
    QString m_projectDescription;
    QString m_projectShortDescription;
    QString m_projectVersion{QStringLiteral("1.0")};
    QString m_author;
    QStringList m_authors;
    QString m_email;
    QString m_dateCreated;
    QString m_dateLastEdited;
    bool m_isProjectOpen{false};
};

