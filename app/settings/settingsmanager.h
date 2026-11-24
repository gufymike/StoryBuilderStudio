// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QSettings>
#include <QtQml/QQmlEngine>
#include <memory>

/**
 * @class SettingsManager
 * @brief Manages persistent workspace configuration, model directories, and inference providers.
 *
 * Operates as a QML Singleton. This class bridges the UI and the underlying INI file.
 * It is responsible for scanning the local filesystem for AI models, managing custom
 * REST provider endpoints, tracking recent files, and securely resolving environment variables
 * for API keys (strictly enforcing whitelists to prevent malicious path traversal).
 */
class SettingsManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    /**
     * @property isFirstRun
     * @brief Flag indicating if the application is being launched for the very first time.
     * @notifier isFirstRunChanged
     */
    Q_PROPERTY(bool isFirstRun READ isFirstRun WRITE setIsFirstRun NOTIFY isFirstRunChanged)
    Q_PROPERTY(QString defaultSavePath READ defaultSavePath WRITE setDefaultSavePath NOTIFY defaultSavePathChanged)
    Q_PROPERTY(int editorFontSize READ editorFontSize WRITE setEditorFontSize NOTIFY editorFontSizeChanged)
    Q_PROPERTY(QString activeTheme READ activeTheme WRITE setActiveTheme NOTIFY activeThemeChanged)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged)
    Q_PROPERTY(QString baseModelsDir READ baseModelsDir WRITE setBaseModelsDir NOTIFY baseModelsDirChanged)

    // Shared Universal Transformer (T5-XXL)
    Q_PROPERTY(QString globalT5Path READ globalT5Path WRITE setGlobalT5Path NOTIFY globalT5PathChanged)

    // External Tools Registry
    Q_PROPERTY(QVariantList externalTools READ externalTools NOTIFY externalToolsChanged)

    // Providers
    Q_PROPERTY(QVariantList scriptProviders READ scriptProviders NOTIFY scriptProvidersChanged)
    Q_PROPERTY(QVariantList imageProviders READ imageProviders NOTIFY imageProvidersChanged)
    Q_PROPERTY(QVariantList audioProviders READ audioProviders NOTIFY audioProvidersChanged)
    Q_PROPERTY(QVariantList videoProviders READ videoProviders NOTIFY videoProvidersChanged)

    // Active Provider per Domain
    Q_PROPERTY(QString activeScriptProvider READ activeScriptProvider WRITE setActiveScriptProvider NOTIFY activeScriptProviderChanged)
    Q_PROPERTY(QString activeImageProvider READ activeImageProvider WRITE setActiveImageProvider NOTIFY activeImageProviderChanged)
    Q_PROPERTY(QString activeAudioProvider READ activeAudioProvider WRITE setActiveAudioProvider NOTIFY activeAudioProviderChanged)
    Q_PROPERTY(QString activeVideoProvider READ activeVideoProvider WRITE setActiveVideoProvider NOTIFY activeVideoProviderChanged)

    // Audio Subsystem & Models
    Q_PROPERTY(QString activeAudioCategory READ activeAudioCategory WRITE setActiveAudioCategory NOTIFY activeAudioCategoryChanged)
    Q_PROPERTY(QString audioCppModelsDir READ audioCppModelsDir WRITE setAudioCppModelsDir NOTIFY audioCppModelsDirChanged)
    Q_PROPERTY(QStringList availableAudioCppModels READ availableAudioCppModels NOTIFY availableAudioCppModelsChanged)
    Q_PROPERTY(QStringList availableSpeechModels READ availableSpeechModels NOTIFY availableAudioSubmodelsChanged)

    // Gemini API Key Verification
    Q_PROPERTY(bool hasGeminiKey READ hasGeminiKey NOTIFY geminiKeyStatusChanged)
    Q_PROPERTY(QString geminiKeyStatusText READ geminiKeyStatusText NOTIFY geminiKeyStatusChanged)

    // Parent Directories
    Q_PROPERTY(QString scriptModelsDir READ scriptModelsDir WRITE setScriptModelsDir NOTIFY scriptModelsDirChanged)
    Q_PROPERTY(QString imagesRootDir READ imagesRootDir WRITE setImagesRootDir NOTIFY imagesRootDirChanged)
    Q_PROPERTY(QString audioRootDir READ audioRootDir WRITE setAudioRootDir NOTIFY audioRootDirChanged)
    Q_PROPERTY(QString videoRootDir READ videoRootDir WRITE setVideoRootDir NOTIFY videoRootDirChanged)

    // Script Author Style
    Q_PROPERTY(QString scriptAuthorStyle READ scriptAuthorStyle WRITE setScriptAuthorStyle NOTIFY scriptAuthorStyleChanged)

    Q_PROPERTY(QString geminiApiKey READ geminiApiKey NOTIFY apiNetworkSettingsChanged)
    Q_PROPERTY(QString openAiApiKey READ openAiApiKey NOTIFY apiNetworkSettingsChanged)
    Q_PROPERTY(QString ollamaEndpoint READ ollamaEndpoint WRITE setOllamaEndpoint NOTIFY apiNetworkSettingsChanged)
    Q_PROPERTY(QString ollamaModelName READ ollamaModelName WRITE setOllamaModelName NOTIFY ollamaModelNameChanged)



public:
    explicit SettingsManager(QObject *parent = nullptr);
    explicit SettingsManager(const QString &customConfigPath, QObject *parent = nullptr);
    ~SettingsManager() override;

    bool isFirstRun() const;
    QString defaultSavePath() const;
    int editorFontSize() const;
    QString activeTheme() const;
    QStringList recentFiles() const;
    QString globalT5Path() const;
    QVariantList externalTools() const;
    QString baseModelsDir() const;

    QVariantList scriptProviders() const;
    QVariantList imageProviders() const;
    QVariantList audioProviders() const;
    QVariantList videoProviders() const;

    QString activeScriptProvider() const;
    QString activeImageProvider() const;
    QString activeAudioProvider() const;
    QString activeVideoProvider() const;

    QString activeAudioCategory() const;
    QString audioCppModelsDir() const;
    QStringList availableAudioCppModels() const;
    QStringList availableSpeechModels() const;

    QString geminiApiKey() const { return resolveApiKey(QStringLiteral("gemini_bridge")); }
    QString openAiApiKey() const { return resolveApiKey(QStringLiteral("openai_bridge")); }
    QString ollamaEndpoint() const;
    QString ollamaModelName() const;

    bool hasGeminiKey() const;
    QString geminiKeyStatusText() const;

    QString scriptModelsDir() const;
    QString imagesRootDir() const;
    QString audioRootDir() const;
    QString videoRootDir() const;

    QString scriptAuthorStyle() const;

public slots:
    void setIsFirstRun(bool firstRun);
    void setDefaultSavePath(const QString &path);
    void setEditorFontSize(int size);
    void setActiveTheme(const QString &themeName);
    void setGlobalT5Path(const QString &path);
    void setBaseModelsDir(const QString &path);

    void setActiveScriptProvider(const QString &name);
    void setActiveImageProvider(const QString &name);
    void setActiveAudioProvider(const QString &name);
    void setActiveVideoProvider(const QString &name);
    void setActiveAudioCategory(const QString &cat);

    void setScriptModelsDir(const QString &dir);
    void setImagesRootDir(const QString &dir);
    void setAudioRootDir(const QString &dir);
    void setAudioCppModelsDir(const QString &dir);
    void setVideoRootDir(const QString &dir);
    void setScriptAuthorStyle(const QString &style);

    // External Tools Management
    Q_INVOKABLE void addExternalTool(const QString &type, const QString &name, const QString &path);
    Q_INVOKABLE void updateExternalTool(int index, const QString &type, const QString &name, const QString &path);
    Q_INVOKABLE void removeExternalTool(int index);
    Q_INVOKABLE bool launchExternalTool(int index, const QString &assetFilePath);
    Q_INVOKABLE QVariantList getExternalToolsForType(const QString &type) const;
    Q_INVOKABLE bool launchToolByName(const QString &toolName, const QString &assetFilePath);

    Q_INVOKABLE QVariantList getProvidersForCategory(const QString &category, const QString &subCategory = QString()) const;
    Q_INVOKABLE bool removeCustomProvider(const QString &category, const QString &name);
    Q_INVOKABLE void checkEnvironmentKeys();

    Q_INVOKABLE QVariantList verifyModelManifest(const QString &category, const QString &modelName, const QString &subCategory = QString()) const;
    Q_INVOKABLE void resetDefaults();
    Q_INVOKABLE void refreshScannedDirectories();
    Q_INVOKABLE QString getStorageLocation() const;

    Q_INVOKABLE void addRecentFile(const QString &filePath);
    Q_INVOKABLE void clearRecentFiles();
    Q_INVOKABLE bool removeKnownSeriesPath(const QString &seriesPath);

    // Driver Validation & Config Builder
    void setOllamaEndpoint(const QString &endpoint);
    void setOllamaModelName(const QString &modelName);

    Q_INVOKABLE bool isDriverConfigured(const QString &driverId, const QString &modelDirOrName = QString()) const;
    Q_INVOKABLE QVariantMap getDriverLoadConfig(const QString &driverId, const QString &modelDirOrName = QString()) const;

    Q_INVOKABLE QString getAudioCppFamily(const QString &modelName) const;
    Q_INVOKABLE QString resolveApiKey(const QString &driverId, const QString &customEnvName = QString()) const;
    Q_INVOKABLE QVariantMap scanModelDirectory(const QString &baseDir, const QString &t5Path);
    Q_INVOKABLE bool addCustomProvider(const QString &category,
                                       const QString &name,
                                       const QString &url,
                                       const QString &modelName = QString(),
                                       const QString &envVar = QString());
    Q_INVOKABLE QVariantMap getRemoteDriverConfig(const QString &category, const QString &driverId) const;
    Q_INVOKABLE bool saveRemoteDriverConfig(const QString &category,
                                            const QString &driverId,
                                            const QString &baseUrl,
                                            const QString &modelName,
                                            const QString &envVar = QString());
signals:
    void isFirstRunChanged();
    void defaultSavePathChanged();
    void editorFontSizeChanged();
    void activeThemeChanged();
    void recentFilesChanged();
    void globalT5PathChanged();
    void externalToolsChanged();
    void baseModelsDirChanged();

    void scriptProvidersChanged();
    void imageProvidersChanged();
    void audioProvidersChanged();
    void videoProvidersChanged();

    void customProviderRemoved(const QString &driverId);
    void activeScriptProviderChanged();
    void activeImageProviderChanged();
    void activeAudioProviderChanged();
    void activeVideoProviderChanged();
    void activeAudioCategoryChanged();
    void audioCppModelsDirChanged();
    void availableAudioCppModelsChanged();
    void availableAudioSubmodelsChanged();
    void ollamaEndpointChanged();
    void ollamaModelNameChanged();
    void apiNetworkSettingsChanged();
    void geminiKeyStatusChanged();

    void scriptModelsDirChanged();
    void imagesRootDirChanged();
    void audioRootDirChanged();
    void videoRootDirChanged();

    void scriptAuthorStyleChanged();

private:
    void initialize(const QString &customConfigPath = QString());
    QString defaultBaseDir() const;
    void rescanAll();
    void saveCustomProviders(const QString &category, const QVariantList &list);

    QStringList m_availableScriptModels;
    QStringList m_availableImageModelDirs;
    QStringList m_availableVideoModelDirs;
    QStringList m_availableAudioCppModels;
    QStringList m_availableSpeechModels;

    std::unique_ptr<QSettings> m_settings;
    const int m_maxRecentFiles = 5;
};

