/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: settingsmanager.cpp
** Manages persistent workspace configuration and environment security.
**
****************************************************************************/
#include "settingsmanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QProcess>
#include <QDebug>

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
    initialize();
}

SettingsManager::SettingsManager(const QString &customConfigPath, QObject *parent)
    : QObject(parent)
{
    initialize(customConfigPath);
}

void SettingsManager::initialize(const QString &customConfigPath)
{
    QString iniFilePath;
    if (!customConfigPath.isEmpty()) {
        iniFilePath = customConfigPath;
        QFileInfo fi(iniFilePath);
        QDir dir = fi.dir();
        if (!dir.exists()) {
            dir.mkpath(QStringLiteral("."));
        }
    } else {
        QString appDataDir = defaultBaseDir();
        QDir dir(appDataDir);
        if (!dir.exists()) {
            dir.mkpath(QStringLiteral("."));
        }
        iniFilePath = dir.absoluteFilePath(QStringLiteral("workspace_config.ini"));
    }

    qInfo() << "[SettingsManager] working workspace_config.ini: " << iniFilePath;
    m_settings = std::make_unique<QSettings>(iniFilePath, QSettings::IniFormat);

    checkEnvironmentKeys();
    rescanAll();
}

SettingsManager::~SettingsManager() = default;

QString SettingsManager::defaultBaseDir() const {
    QString appData = QDir::fromNativeSeparators(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        );
    if (!appData.endsWith(QStringLiteral("storyBuilderStudio"), Qt::CaseInsensitive)) {
        appData += QStringLiteral("/storyBuilderStudio");
    }
    return QDir::toNativeSeparators(appData);
}

void SettingsManager::checkEnvironmentKeys() {
    emit geminiKeyStatusChanged();
}

bool SettingsManager::hasGeminiKey() const {
    QString key = qEnvironmentVariable("GEMINI_API_KEY").trimmed();
    if (key.isEmpty() && m_settings) {
        key = m_settings->value(QStringLiteral("ApiKeys/GeminiApiKey"), QString()).toString().trimmed();
    }
    return !key.isEmpty();
}

QString SettingsManager::baseModelsDir() const {
    return m_settings->value(QStringLiteral("Paths/BaseModelsDir"), defaultBaseDir()).toString();
}

void SettingsManager::setBaseModelsDir(const QString &dir) {
    QString clean = QDir::toNativeSeparators(dir.trimmed());
    if (baseModelsDir() == clean || clean.isEmpty()) return;

    m_settings->setValue(QStringLiteral("Paths/BaseModelsDir"), clean);
    emit baseModelsDirChanged();

    // Propagate cleanly to standard sub-paths
    setScriptModelsDir(QDir::toNativeSeparators(clean + QStringLiteral("/text")));
    setImagesRootDir(QDir::toNativeSeparators(clean + QStringLiteral("/images")));
    setAudioRootDir(QDir::toNativeSeparators(clean + QStringLiteral("/audio")));
    setVideoRootDir(QDir::toNativeSeparators(clean + QStringLiteral("/video")));

    QString potentialT5 = QDir::toNativeSeparators(clean + QStringLiteral("/images/flux/t5-v1_1-xxl-encoder-Q5_K_M.gguf"));
    if (QFile::exists(potentialT5)) {
        setGlobalT5Path(potentialT5);
    }

    rescanAll();
}

QVariantMap SettingsManager::scanModelDirectory(const QString &baseDir, const QString &t5Path) {
    QVariantMap result;
    QDir root(baseDir);

    auto countFiles = [&](const QString &sub, const QStringList &filters) -> int {
        QDir d(root.filePath(sub));
        if (!d.exists()) return 0;
        return d.entryList(filters, QDir::Files).size();
    };

    result[QStringLiteral("textCount")]  = countFiles(QStringLiteral("text"),  {QStringLiteral("*.gguf"), QStringLiteral("*.bin")});
    result[QStringLiteral("imageCount")] = countFiles(QStringLiteral("images"), {QStringLiteral("*.safetensors"), QStringLiteral("*.gguf"), QStringLiteral("*.ckpt")});
    result[QStringLiteral("audioCount")] = countFiles(QStringLiteral("audio"), {QStringLiteral("*.onnx"), QStringLiteral("*.gguf"), QStringLiteral("*.bin")});
    result[QStringLiteral("videoCount")] = countFiles(QStringLiteral("video"), {QStringLiteral("*.safetensors"), QStringLiteral("*.gguf")});

    QFileInfo t5Info(t5Path);
    result[QStringLiteral("hasT5")] = t5Info.exists() && t5Info.isFile();

    return result;
}

QString SettingsManager::geminiKeyStatusText() const {
    return hasGeminiKey() ? tr("✔ GEMINI_API_KEY Configured") : tr("✖ Missing GEMINI_API_KEY in Environment");
}

void SettingsManager::rescanAll() {
    // 1. Script / Text Models
    QDir txtDir(scriptModelsDir());
    if (txtDir.exists()) {
        QStringList filters{QStringLiteral("*.gguf"), QStringLiteral("*.bin"), QStringLiteral("*.safetensors")};
        QStringList files = txtDir.entryList(filters, QDir::Files | QDir::Readable, QDir::Name);
        QStringList subdirs = txtDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        m_availableScriptModels = files + subdirs;
    } else {
        m_availableScriptModels.clear();
    }

    // 2. Images Models
    QDir imgDir(imagesRootDir());
    if (imgDir.exists()) {
        m_availableImageModelDirs = imgDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    } else {
        m_availableImageModelDirs.clear();
    }

    // 3. Audio Models (Speech + audio.cpp GGUF directly under audioRootDir)
    QString audioBase = audioRootDir();
    QDir speechDir(audioBase + QStringLiteral("/speech"));
    m_availableSpeechModels = speechDir.exists()
                                  ? speechDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)
                                  : QStringList();

    QDir audioCppDir(audioCppModelsDir());
    if (audioCppDir.exists()) {
        m_availableAudioCppModels = audioCppDir.entryList(QStringList() << QStringLiteral("*.gguf"), QDir::Files | QDir::Readable, QDir::Name);
    } else {
        m_availableAudioCppModels.clear();
    }

    // 4. Video Models
    QDir vidDir(videoRootDir());
    if (vidDir.exists()) {
        m_availableVideoModelDirs = vidDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    } else {
        m_availableVideoModelDirs.clear();
    }

    emit availableAudioCppModelsChanged();
    emit availableAudioSubmodelsChanged();
    emit audioProvidersChanged();
    emit scriptProvidersChanged();
    emit imageProvidersChanged();
    emit videoProvidersChanged();
}

void SettingsManager::refreshScannedDirectories() {
    rescanAll();
    checkEnvironmentKeys();
}

QVariantList SettingsManager::getProvidersForCategory(const QString &category, const QString &subCategory) const {
    Q_UNUSED(subCategory);
    QVariantList list;

    // Built-in Cloud
    if (category == QStringLiteral("script")) {
        QVariantMap gemini;
        gemini[QStringLiteral("name")] = QStringLiteral("Gemini Bridge");
        gemini[QStringLiteral("url")] = QStringLiteral("https://generativelanguage.googleapis.com");
        gemini[QStringLiteral("isBuiltIn")] = true;
        gemini[QStringLiteral("type")] = QStringLiteral("gemini");
        gemini[QStringLiteral("localModel")] = QString();
        list.append(gemini);

        QVariantMap openai;
        openai[QStringLiteral("name")] = QStringLiteral("OpenAI");
        openai[QStringLiteral("url")] = QString();
        openai[QStringLiteral("isBuiltIn")] = true;
        openai[QStringLiteral("type")] = QStringLiteral("stub");
        openai[QStringLiteral("localModel")] = QString();
        list.append(openai);
    }

    // Local Models Scanned
    QStringList localDirs;
    if (category == QStringLiteral("script")) {
        localDirs = m_availableScriptModels;
    } else if (category == QStringLiteral("images")) {
        localDirs = m_availableImageModelDirs;
    } else if (category == QStringLiteral("video")) {
        localDirs = m_availableVideoModelDirs;
    } else if (category == QStringLiteral("audio")) {
        localDirs = m_availableSpeechModels;
    }

    for (const QString &dirName : localDirs) {
        QVariantMap localItem;
        localItem[QStringLiteral("name")] = QStringLiteral("Local: %1").arg(dirName);
        localItem[QStringLiteral("url")] = QString();
        localItem[QStringLiteral("isBuiltIn")] = true;
        localItem[QStringLiteral("type")] = QStringLiteral("local");
        localItem[QStringLiteral("localModel")] = dirName;
        list.append(localItem);
    }

    // Custom Providers from INI
    QString key = QStringLiteral("CustomProviders/") + category;
    QString rawJson = m_settings->value(key, QStringLiteral("[]")).toString();
    QJsonDocument doc = QJsonDocument::fromJson(rawJson.toUtf8());
    if (doc.isArray()) {
        for (const QJsonValue &v : doc.array()) {
            QJsonObject obj = v.toObject();
            QVariantMap item;
            item[QStringLiteral("name")]       = obj.value(QStringLiteral("name")).toString();
            item[QStringLiteral("url")]        = obj.value(QStringLiteral("url")).toString();
            item[QStringLiteral("modelName")]  = obj.value(QStringLiteral("modelName")).toString();
            item[QStringLiteral("envVar")]     = obj.value(QStringLiteral("envVar")).toString();
            item[QStringLiteral("isBuiltIn")]  = false;
            item[QStringLiteral("type")]       = QStringLiteral("custom");
            item[QStringLiteral("localModel")] = QString();
            list.append(item);
        }
    }

    return list;
}

QVariantList SettingsManager::scriptProviders() const { return getProvidersForCategory(QStringLiteral("script")); }
QVariantList SettingsManager::imageProviders() const { return getProvidersForCategory(QStringLiteral("images")); }
QVariantList SettingsManager::audioProviders() const { return getProvidersForCategory(QStringLiteral("audio")); }
QVariantList SettingsManager::videoProviders() const { return getProvidersForCategory(QStringLiteral("video")); }

void SettingsManager::saveCustomProviders(const QString &category, const QVariantList &list) {
    QJsonArray arr;
    for (const QVariant &v : list) {
        QVariantMap map = v.toMap();
        if (!map.value(QStringLiteral("isBuiltIn"), false).toBool()) {
            QJsonObject obj;
            obj[QStringLiteral("name")]      = map.value(QStringLiteral("name")).toString();
            obj[QStringLiteral("url")]       = map.value(QStringLiteral("url")).toString();
            obj[QStringLiteral("modelName")] = map.value(QStringLiteral("modelName")).toString();
            obj[QStringLiteral("envVar")]    = map.value(QStringLiteral("envVar")).toString();
            arr.append(obj);
        }
    }
    QString key = QStringLiteral("CustomProviders/") + category;
    m_settings->setValue(key, QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

bool SettingsManager::addCustomProvider(const QString &category, const QString &name,
                                        const QString &url, const QString &modelName,
                                        const QString &envVar) {
    QString trimmedName = name.trimmed();
    QString trimmedUrl = url.trimmed();
    if (trimmedName.isEmpty() || trimmedUrl.isEmpty()) return false;

    QVariantList current = getProvidersForCategory(category);
    for (const QVariant &v : current) {
        if (v.toMap().value(QStringLiteral("name")).toString().compare(trimmedName, Qt::CaseInsensitive) == 0) {
            return false;
        }
    }

    QVariantMap custom;
    custom[QStringLiteral("name")]      = trimmedName;
    custom[QStringLiteral("url")]       = trimmedUrl;
    custom[QStringLiteral("modelName")] = modelName.trimmed();
    custom[QStringLiteral("envVar")]    = envVar.trimmed().toUpper();
    custom[QStringLiteral("isBuiltIn")] = false;
    custom[QStringLiteral("type")]      = QStringLiteral("custom");
    current.append(custom);

    saveCustomProviders(category, current);

    if (category == QStringLiteral("script"))      { emit scriptProvidersChanged(); setActiveScriptProvider(trimmedName); }
    else if (category == QStringLiteral("images")) { emit imageProvidersChanged();  setActiveImageProvider(trimmedName); }
    else if (category == QStringLiteral("video"))  { emit videoProvidersChanged();  setActiveVideoProvider(trimmedName); }
    else if (category == QStringLiteral("audio"))  { emit audioProvidersChanged();  setActiveAudioProvider(trimmedName); }

    emit apiNetworkSettingsChanged();
    return true;
}

bool SettingsManager::removeCustomProvider(const QString &category, const QString &name) {
    QVariantList current = getProvidersForCategory(category);
    bool removed = false;
    for (int i = current.size() - 1; i >= 0; --i) {
        QVariantMap map = current[i].toMap();
        if (!map.value(QStringLiteral("isBuiltIn"), false).toBool() && map.value(QStringLiteral("name")).toString() == name) {
            current.removeAt(i);
            removed = true;
        }
    }
    if (removed) {
        saveCustomProviders(category, current);
        if (category == QStringLiteral("script"))      emit scriptProvidersChanged();
        else if (category == QStringLiteral("images")) emit imageProvidersChanged();
        else if (category == QStringLiteral("video"))  emit videoProvidersChanged();
        else if (category == QStringLiteral("audio"))  emit audioProvidersChanged();

        const QString assetType = (category == QStringLiteral("images")) ? QStringLiteral("image") : category;
        const QString driverId  = QStringLiteral("custom_%1_%2").arg(assetType, name.trimmed().toLower().replace(' ', '_'));
        emit customProviderRemoved(driverId);
        emit apiNetworkSettingsChanged();
    }
    return removed;
}

QVariantList SettingsManager::verifyModelManifest(const QString &category, const QString &modelName, const QString &subCategory) const
{
    Q_UNUSED(subCategory);
    QVariantList manifest;
    if (modelName.isEmpty()) return manifest;

    QString cleanName = modelName;
    if (cleanName.startsWith(QStringLiteral("Local: "), Qt::CaseInsensitive)) {
        cleanName = cleanName.mid(7).trimmed();
    }

    auto addFileEntry = [&](const QString &label, const QString &fullPath, bool found) {
        QVariantMap item;
        item[QStringLiteral("label")] = label;
        item[QStringLiteral("path")] = fullPath;
        item[QStringLiteral("found")] = found;
        manifest.append(item);
    };

    if (category == QStringLiteral("images")) {
        QString modelDir = QDir::toNativeSeparators(imagesRootDir() + QStringLiteral("/") + cleanName);
        QDir dir(modelDir);
        QStringList diff = dir.entryList(QStringList() << QStringLiteral("*.gguf") << QStringLiteral("*diffusion*.safetensors"), QDir::Files);
        addFileEntry(QStringLiteral("Diffusion Model (.gguf)"), diff.isEmpty() ? modelDir + QStringLiteral("/*.gguf") : dir.filePath(diff.first()), !diff.isEmpty());
        addFileEntry(QStringLiteral("CLIP-L Encoder"), dir.filePath(QStringLiteral("clip_l.safetensors")), QFile::exists(dir.filePath(QStringLiteral("clip_l.safetensors"))));
        QString vaePath = dir.filePath(QStringLiteral("ae.safetensors"));
        if (!QFile::exists(vaePath)) vaePath = dir.filePath(QStringLiteral("vae.safetensors"));
        addFileEntry(QStringLiteral("VAE Decoder"), vaePath, QFile::exists(vaePath));
        addFileEntry(QStringLiteral("Shared T5-XXL Encoder"), globalT5Path(), QFile::exists(globalT5Path()));
    }
    else if (category == QStringLiteral("video")) {
        QString modelDir = QDir::toNativeSeparators(videoRootDir() + QStringLiteral("/") + cleanName);
        QDir dir(modelDir);

        QString ditPath;
        QStringList candidates = dir.entryList(QStringList() << QStringLiteral("*.gguf") << QStringLiteral("*.safetensors"), QDir::Files | QDir::Readable, QDir::Name);
        for (const QString &f : candidates) {
            QString lower = f.toLower();
            if (!lower.startsWith(QStringLiteral("t5")) && !lower.startsWith(QStringLiteral("umt5")) && !lower.contains(QStringLiteral("vae"))) {
                ditPath = dir.filePath(f);
                break;
            }
        }
        addFileEntry(QStringLiteral("DiT Weights"), ditPath.isEmpty() ? modelDir + QStringLiteral("/*model*") : ditPath, !ditPath.isEmpty());

        QString vaePath;
        QStringList vaes = dir.entryList(QStringList() << QStringLiteral("*vae*.safetensors") << QStringLiteral("*vae*.bin"), QDir::Files | QDir::Readable, QDir::Name);
        if (!vaes.isEmpty()) {
            vaePath = dir.filePath(vaes.first());
        }
        addFileEntry(QStringLiteral("Video VAE"), vaePath.isEmpty() ? modelDir + QStringLiteral("/*vae*") : vaePath, !vaePath.isEmpty());

        QString t5Path;
        QStringList t5s = dir.entryList(QStringList() << QStringLiteral("*umt5*.gguf") << QStringLiteral("*umt5*.safetensors"), QDir::Files | QDir::Readable, QDir::Name);
        if (!t5s.isEmpty()) {
            t5Path = dir.filePath(t5s.first());
            addFileEntry(QStringLiteral("Dedicated UMT5 Encoder"), t5Path, true);
        } else {
            t5Path = globalT5Path();
            addFileEntry(QStringLiteral("Shared T5-XXL Encoder"), t5Path, QFile::exists(t5Path));
        }
    }
    else if (category == QStringLiteral("audio")) {
        QString modelDir = QDir::toNativeSeparators(audioRootDir() + QStringLiteral("/speech/") + cleanName);
        QDir dir(modelDir);
        QStringList onnx = dir.entryList(QStringList() << QStringLiteral("*.onnx"), QDir::Files);
        addFileEntry(QStringLiteral("TTS ONNX Model (.onnx)"), onnx.isEmpty() ? modelDir + QStringLiteral("/*.onnx") : dir.filePath(onnx.first()), !onnx.isEmpty());
        addFileEntry(QStringLiteral("Voices Data (voices.bin)"), dir.filePath(QStringLiteral("voices.bin")), QFile::exists(dir.filePath(QStringLiteral("voices.bin"))));
        addFileEntry(QStringLiteral("Tokenizer (tokens.txt)"), dir.filePath(QStringLiteral("tokens.txt")), QFile::exists(dir.filePath(QStringLiteral("tokens.txt"))));
        addFileEntry(QStringLiteral("Phonemizer Data (espeak-ng-data)"), dir.filePath(QStringLiteral("espeak-ng-data")), QDir(dir.filePath(QStringLiteral("espeak-ng-data"))).exists());
    }
    else if (category == QStringLiteral("script")) {
        QString target = QDir::toNativeSeparators(scriptModelsDir() + QStringLiteral("/") + cleanName);
        QFileInfo fi(target);
        if (fi.isDir()) {
            QDir dir(target);
            QStringList weights = dir.entryList(QStringList() << QStringLiteral("*.gguf") << QStringLiteral("*.safetensors"), QDir::Files);
            addFileEntry(QStringLiteral("LLM Weights"), weights.isEmpty() ? target + QStringLiteral("/*.gguf") : dir.filePath(weights.first()), !weights.isEmpty());
            addFileEntry(QStringLiteral("Tokenizer / Config"), dir.filePath(QStringLiteral("tokenizer.json")), QFile::exists(dir.filePath(QStringLiteral("tokenizer.json"))));
        } else {
            addFileEntry(QStringLiteral("Model File (.gguf)"), target, fi.exists());
        }
    }
    return manifest;
}

QString SettingsManager::activeAudioCategory() const {
    return m_settings->value(QStringLiteral("Audio/ActiveCategory"), QStringLiteral("speech")).toString();
}
void SettingsManager::setActiveAudioCategory(const QString &cat) {
    if (activeAudioCategory() == cat || cat.isEmpty()) return;
    m_settings->setValue(QStringLiteral("Audio/ActiveCategory"), cat);
    emit activeAudioCategoryChanged();
    emit audioProvidersChanged();
}

QStringList SettingsManager::availableSpeechModels() const { return m_availableSpeechModels; }

bool SettingsManager::isFirstRun() const { return m_settings->value(QStringLiteral("System/IsFirstRun"), true).toBool(); }
void SettingsManager::setIsFirstRun(bool firstRun) {
    if (isFirstRun() == firstRun) return;
    m_settings->setValue(QStringLiteral("System/IsFirstRun"), firstRun);
    emit isFirstRunChanged();
}

QString SettingsManager::defaultSavePath() const {
    QString fallback = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return m_settings->value(QStringLiteral("Paths/DefaultSavePath"), fallback).toString();
}
void SettingsManager::setDefaultSavePath(const QString &path) {
    if (defaultSavePath() == path || path.isEmpty()) return;
    m_settings->setValue(QStringLiteral("Paths/DefaultSavePath"), path);
    emit defaultSavePathChanged();
}

int SettingsManager::editorFontSize() const { return m_settings->value(QStringLiteral("Editor/FontSize"), 16).toInt(); }
void SettingsManager::setEditorFontSize(int size) {
    if (editorFontSize() == size || size < 8 || size > 72) return;
    m_settings->setValue(QStringLiteral("Editor/FontSize"), size);
    emit editorFontSizeChanged();
}

QString SettingsManager::activeTheme() const { return m_settings->value(QStringLiteral("UI/ActiveTheme"), QStringLiteral("Fusion")).toString(); }
void SettingsManager::setActiveTheme(const QString &themeName) {
    if (activeTheme() == themeName || themeName.isEmpty()) return;
    m_settings->setValue(QStringLiteral("UI/ActiveTheme"), themeName);
    emit activeThemeChanged();
}

QStringList SettingsManager::recentFiles() const { return m_settings->value(QStringLiteral("History/RecentFiles")).toStringList(); }
QString SettingsManager::getStorageLocation() const { return m_settings->fileName(); }

QString SettingsManager::globalT5Path() const {
    QString fallback = defaultBaseDir() + QStringLiteral("/images/flux/t5-v1_1-xxl-encoder-Q5_K_M.gguf");
    return m_settings->value(QStringLiteral("Global/T5Path"), fallback).toString();
}
void SettingsManager::setGlobalT5Path(const QString &path) {
    if (globalT5Path() == path || path.isEmpty()) return;
    m_settings->setValue(QStringLiteral("Global/T5Path"), path);
    emit globalT5PathChanged();
}

QString SettingsManager::scriptModelsDir() const {
    return m_settings->value(QStringLiteral("Script/ModelsDir"), defaultBaseDir() + QStringLiteral("/text")).toString();
}
void SettingsManager::setScriptModelsDir(const QString &dir) {
    QString clean = QDir::toNativeSeparators(dir.trimmed());
    if (scriptModelsDir() == clean || clean.isEmpty()) return;
    m_settings->setValue(QStringLiteral("Script/ModelsDir"), clean);
    emit scriptModelsDirChanged();
    rescanAll();
}

QString SettingsManager::imagesRootDir() const {
    return m_settings->value(QStringLiteral("Images/RootDir"), defaultBaseDir() + QStringLiteral("/images")).toString();
}
void SettingsManager::setImagesRootDir(const QString &dir) {
    QString clean = QDir::toNativeSeparators(dir.trimmed());
    if (imagesRootDir() == clean || clean.isEmpty()) return;
    m_settings->setValue(QStringLiteral("Images/RootDir"), clean);
    emit imagesRootDirChanged();
    rescanAll();
}

QString SettingsManager::audioRootDir() const {
    return m_settings->value(QStringLiteral("Audio/RootDir"), defaultBaseDir() + QStringLiteral("/audio")).toString();
}
void SettingsManager::setAudioRootDir(const QString &dir) {
    QString clean = QDir::toNativeSeparators(dir.trimmed());
    if (audioRootDir() == clean || clean.isEmpty()) return;
    m_settings->setValue(QStringLiteral("Audio/RootDir"), clean);
    emit audioRootDirChanged();
    rescanAll();
}

QString SettingsManager::videoRootDir() const {
    return m_settings->value(QStringLiteral("Video/RootDir"), defaultBaseDir() + QStringLiteral("/video")).toString();
}
void SettingsManager::setVideoRootDir(const QString &dir) {
    QString clean = QDir::toNativeSeparators(dir.trimmed());
    if (videoRootDir() == clean || clean.isEmpty()) return;
    m_settings->setValue(QStringLiteral("Video/RootDir"), clean);
    emit videoRootDirChanged();
    rescanAll();
}

QString SettingsManager::activeScriptProvider() const {
    return m_settings->value(QStringLiteral("Providers/ActiveScript"), QStringLiteral("Gemini Bridge")).toString();
}
void SettingsManager::setActiveScriptProvider(const QString &name) {
    if (activeScriptProvider() == name) return;
    m_settings->setValue(QStringLiteral("Providers/ActiveScript"), name);
    emit activeScriptProviderChanged();
}

QString SettingsManager::activeImageProvider() const {
    return m_settings->value(QStringLiteral("Providers/ActiveImage"), QStringLiteral("Local: flux")).toString();
}
void SettingsManager::setActiveImageProvider(const QString &name) {
    if (activeImageProvider() == name) return;
    m_settings->setValue(QStringLiteral("Providers/ActiveImage"), name);
    emit activeImageProviderChanged();
}

QString SettingsManager::activeAudioProvider() const {
    return m_settings->value(QStringLiteral("Providers/ActiveAudio"), QStringLiteral("Local: Kokoro")).toString();
}
void SettingsManager::setActiveAudioProvider(const QString &name) {
    if (activeAudioProvider() == name) return;
    m_settings->setValue(QStringLiteral("Providers/ActiveAudio"), name);
    emit activeAudioProviderChanged();
}

QString SettingsManager::activeVideoProvider() const {
    return m_settings->value(QStringLiteral("Providers/ActiveVideo"), QStringLiteral("Local: wan")).toString();
}
void SettingsManager::setActiveVideoProvider(const QString &name) {
    if (activeVideoProvider() == name) return;
    m_settings->setValue(QStringLiteral("Providers/ActiveVideo"), name);
    emit activeVideoProviderChanged();
}

QString SettingsManager::scriptAuthorStyle() const {
    return m_settings->value(QStringLiteral("script/authorStyle"), QStringLiteral("You are a professional screenplay and narrative writer.")).toString();
}
void SettingsManager::setScriptAuthorStyle(const QString &style) {
    QString current = scriptAuthorStyle();
    if (current != style) {
        m_settings->setValue(QStringLiteral("script/authorStyle"), style);
        emit scriptAuthorStyleChanged();
    }
}

QVariantList SettingsManager::externalTools() const {
    QString rawJson = m_settings->value(QStringLiteral("ExternalTools/List"), QStringLiteral("[]")).toString();
    QJsonDocument doc = QJsonDocument::fromJson(rawJson.toUtf8());
    QVariantList list;
    if (doc.isArray()) {
        for (const QJsonValue &val : doc.array()) {
            list.append(val.toObject().toVariantMap());
        }
    }
    return list;
}

void SettingsManager::addExternalTool(const QString &type, const QString &name, const QString &path) {
    QVariantList list = externalTools();
    QVariantMap tool;
    tool[QStringLiteral("type")] = type.isEmpty() ? QStringLiteral("audio") : type;
    tool[QStringLiteral("name")] = name.isEmpty() ? QStringLiteral("New Tool") : name;
    tool[QStringLiteral("path")] = path;
    list.append(tool);

    QJsonArray arr;
    for (const auto &item : list) {
        arr.append(QJsonObject::fromVariantMap(item.toMap()));
    }
    m_settings->setValue(QStringLiteral("ExternalTools/List"), QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    emit externalToolsChanged();
}

void SettingsManager::updateExternalTool(int index, const QString &type, const QString &name, const QString &path) {
    QVariantList list = externalTools();
    if (index < 0 || index >= list.size()) return;

    QVariantMap tool = list[index].toMap();
    tool[QStringLiteral("type")] = type;
    tool[QStringLiteral("name")] = name;
    tool[QStringLiteral("path")] = path;
    list[index] = tool;

    QJsonArray arr;
    for (const auto &item : list) {
        arr.append(QJsonObject::fromVariantMap(item.toMap()));
    }
    m_settings->setValue(QStringLiteral("ExternalTools/List"), QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    emit externalToolsChanged();
}

void SettingsManager::removeExternalTool(int index) {
    QVariantList list = externalTools();
    if (index < 0 || index >= list.size()) return;

    list.removeAt(index);
    QJsonArray arr;
    for (const auto &item : list) {
        arr.append(QJsonObject::fromVariantMap(item.toMap()));
    }
    m_settings->setValue(QStringLiteral("ExternalTools/List"), QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    emit externalToolsChanged();
}

bool SettingsManager::launchExternalTool(int index, const QString &assetFilePath) {
    QVariantList list = externalTools();
    if (index < 0 || index >= list.size()) return false;

    QString exePath = list[index].toMap().value(QStringLiteral("path")).toString().trimmed();
    if (exePath.isEmpty() || !QFile::exists(exePath)) {
        return false;
    }

    QString nativeAssetPath = QDir::toNativeSeparators(assetFilePath);
    QStringList args;
    if (!nativeAssetPath.isEmpty()) {
        args << nativeAssetPath;
    }
    return QProcess::startDetached(exePath, args);
}

QString SettingsManager::audioCppModelsDir() const {
    return m_settings->value(QStringLiteral("Audio/AudioCppModelsDir"), audioRootDir()).toString();
}

void SettingsManager::setAudioCppModelsDir(const QString &dir) {
    if (audioCppModelsDir() == dir || dir.isEmpty()) return;
    m_settings->setValue(QStringLiteral("Audio/AudioCppModelsDir"), dir);
    emit audioCppModelsDirChanged();
    rescanAll();
}

void SettingsManager::resetDefaults() {
    setImagesRootDir(defaultBaseDir() + QStringLiteral("/images"));
    setVideoRootDir(defaultBaseDir() + QStringLiteral("/video"));
    setAudioRootDir(defaultBaseDir() + QStringLiteral("/audio"));
    setScriptModelsDir(defaultBaseDir() + QStringLiteral("/text"));
    setGlobalT5Path(defaultBaseDir() + QStringLiteral("/images/flux/t5-v1_1-xxl-encoder-Q5_K_M.gguf"));
    setActiveScriptProvider(QStringLiteral("Gemini Bridge"));
    setActiveImageProvider(QStringLiteral("Local: flux"));
    setActiveAudioCategory(QStringLiteral("speech"));
    setActiveAudioProvider(QStringLiteral("Local: Kokoro"));
    setActiveVideoProvider(QStringLiteral("Local: wan"));
    rescanAll();
}

QVariantList SettingsManager::getExternalToolsForType(const QString &type) const {
    QVariantList filtered;
    QVariantList all = externalTools();
    for (const QVariant &item : all) {
        QVariantMap map = item.toMap();
        if (map.value(QStringLiteral("type")).toString().compare(type, Qt::CaseInsensitive) == 0) {
            filtered.append(map);
        }
    }
    return filtered;
}

bool SettingsManager::launchToolByName(const QString &toolName, const QString &assetFilePath) {
    QVariantList all = externalTools();
    for (const QVariant &item : all) {
        QVariantMap map = item.toMap();
        if (map.value(QStringLiteral("name")).toString().compare(toolName, Qt::CaseInsensitive) == 0) {
            QString exePath = map.value(QStringLiteral("path")).toString().trimmed();
            if (exePath.isEmpty() || !QFile::exists(exePath)) {
                return false;
            }
            QStringList args;
            if (!assetFilePath.isEmpty()) {
                args << QDir::toNativeSeparators(assetFilePath);
            }
            return QProcess::startDetached(exePath, args);
        }
    }
    return false;
}

void SettingsManager::addRecentFile(const QString &filePath) {
    if (filePath.trimmed().isEmpty()) return;

    QString cleanPath = QDir::toNativeSeparators(filePath.trimmed());
    if (!QFile::exists(cleanPath)) return;

    QStringList recents = recentFiles();
    recents.removeAll(cleanPath);
    recents.prepend(cleanPath);

    while (recents.size() > m_maxRecentFiles) {
        recents.removeLast();
    }

    if (m_settings) {
        m_settings->setValue(QStringLiteral("History/RecentFiles"), recents);
        m_settings->sync();
    }
    emit recentFilesChanged();
}

void SettingsManager::clearRecentFiles() {
    if (m_settings) {
        m_settings->remove(QStringLiteral("History/RecentFiles"));
        m_settings->sync();
    }
    emit recentFilesChanged();
}

bool SettingsManager::isDriverConfigured(const QString &driverId, const QString &modelDirOrName) const
{
    if (driverId == QStringLiteral("gemini_bridge") || driverId == QStringLiteral("openai_bridge")) {
        return !resolveApiKey(driverId).isEmpty();
    }
    if (driverId == QStringLiteral("ollama_local")) {
        return true;
    }

    if (driverId == QStringLiteral("llama_cpp")) {
        QString model = modelDirOrName.isEmpty() ? activeScriptProvider() : modelDirOrName;
        QString modelClean = model.remove(QStringLiteral("Local:")).trimmed();
        QVariantList manifest = verifyModelManifest(QStringLiteral("script"), modelClean);
        for (const QVariant &item : manifest) {
            QVariantMap m = item.toMap();
            if (m.value(QStringLiteral("label")).toString().contains(QStringLiteral("LLM")) && m.value(QStringLiteral("found")).toBool()) {
                return true;
            }
        }
        return false;
    }

    if (driverId == QStringLiteral("sd_image")) {
        QString model = modelDirOrName.isEmpty() ? activeImageProvider() : modelDirOrName;
        QString modelClean = model.remove(QStringLiteral("Local:")).trimmed();
        QVariantList manifest = verifyModelManifest(QStringLiteral("images"), modelClean);
        bool hasDiff = false, hasClip = false;
        for (const QVariant &item : manifest) {
            QVariantMap m = item.toMap();
            if (!m.value(QStringLiteral("found")).toBool()) continue;
            QString label = m.value(QStringLiteral("label")).toString();
            if (label.contains(QStringLiteral("Diffusion"))) hasDiff = true;
            if (label.contains(QStringLiteral("CLIP-L"))) hasClip = true;
        }
        return hasDiff && hasClip && !globalT5Path().isEmpty() && QFile::exists(globalT5Path());
    }

    if (driverId == QStringLiteral("sd_video")) {
        QString model = modelDirOrName.isEmpty() ? activeVideoProvider() : modelDirOrName;
        QString modelClean = model.remove(QStringLiteral("Local:")).trimmed();
        QVariantList manifest = verifyModelManifest(QStringLiteral("video"), modelClean);
        bool hasDit = false, hasVae = false;
        for (const QVariant &item : manifest) {
            QVariantMap m = item.toMap();
            if (!m.value(QStringLiteral("found")).toBool()) continue;
            QString label = m.value(QStringLiteral("label")).toString();
            if (label.contains(QStringLiteral("DiT"))) hasDit = true;
            if (label.contains(QStringLiteral("VAE"))) hasVae = true;
        }
        return hasDit && hasVae;
    }

    if (driverId == QStringLiteral("kokoro_tts")) {
        QString kokoroDir = audioRootDir() + QStringLiteral("/speech/Kokoro");
        return QFile::exists(kokoroDir + QStringLiteral("/model.onnx")) &&
               QFile::exists(kokoroDir + QStringLiteral("/voices.bin")) &&
               QFile::exists(kokoroDir + QStringLiteral("/tokens.txt"));
    }

    if (driverId == QStringLiteral("audio_cpp")) {
        QString model = modelDirOrName.isEmpty()
        ? m_settings->value(QStringLiteral("Audio/ActiveAudioCppModel"), QString()).toString()
        : modelDirOrName;
        if (model.isEmpty() && !availableAudioCppModels().isEmpty()) {
            model = availableAudioCppModels().first();
        }
        if (model.endsWith(QStringLiteral(".gguf"), Qt::CaseInsensitive)) {
            model.chop(5);
        }
        QString filePath = audioRootDir() + QStringLiteral("/") + model + QStringLiteral(".gguf");
        return QFile::exists(filePath);
    }

    return false;
}

QVariantMap SettingsManager::getDriverLoadConfig(const QString &driverId, const QString &modelDirOrName) const
{
    QVariantMap config;

    if (driverId == QStringLiteral("gemini_bridge")) {
        config[QStringLiteral("apiKey")] = resolveApiKey(driverId);
        config[QStringLiteral("baseUrl")] = QStringLiteral("https://generativelanguage.googleapis.com");
        config[QStringLiteral("modelName")] = QStringLiteral("gemini-3.5-flash-lite");
        return config;
    }
    if (driverId == QStringLiteral("openai_bridge")) {
        config[QStringLiteral("apiKey")] = resolveApiKey(driverId);
        config[QStringLiteral("baseUrl")] = QStringLiteral("https://api.openai.com/v1");
        config[QStringLiteral("modelName")] = QStringLiteral("gpt-4o");
        return config;
    }
    if (driverId == QStringLiteral("ollama_local")) {
        config[QStringLiteral("apiKey")] = QString();
        config[QStringLiteral("baseUrl")] = QStringLiteral("http://localhost:11434/v1");
        config[QStringLiteral("modelName")] = modelDirOrName.isEmpty() ? QStringLiteral("qwen2.5:7b") : modelDirOrName;
        return config;
    }

    if (driverId == QStringLiteral("llama_cpp")) {
        QString model = modelDirOrName.isEmpty() ? activeScriptProvider() : modelDirOrName;
        QString modelClean = model.remove(QStringLiteral("Local:")).trimmed();
        QVariantList manifest = verifyModelManifest(QStringLiteral("script"), modelClean);
        for (const QVariant &item : manifest) {
            QVariantMap m = item.toMap();
            if (m.value(QStringLiteral("label")).toString().contains(QStringLiteral("LLM")) && m.value(QStringLiteral("found")).toBool()) {
                config[QStringLiteral("modelPath")] = m.value(QStringLiteral("path")).toString();
                break;
            }
        }
        return config;
    }

    if (driverId == QStringLiteral("sd_image")) {
        QString model = modelDirOrName.isEmpty() ? activeImageProvider() : modelDirOrName;
        QString modelClean = model.remove(QStringLiteral("Local:")).trimmed();
        QVariantList manifest = verifyModelManifest(QStringLiteral("images"), modelClean);
        for (const QVariant &item : manifest) {
            QVariantMap m = item.toMap();
            if (!m.value(QStringLiteral("found")).toBool()) continue;
            QString label = m.value(QStringLiteral("label")).toString();
            if (label.contains(QStringLiteral("Diffusion"))) config[QStringLiteral("diffusionModelPath")] = m.value(QStringLiteral("path")).toString();
            else if (label.contains(QStringLiteral("CLIP-L"))) config[QStringLiteral("clipLPath")] = m.value(QStringLiteral("path")).toString();
            else if (label.contains(QStringLiteral("VAE"))) config[QStringLiteral("vaePath")] = m.value(QStringLiteral("path")).toString();
        }
        config[QStringLiteral("t5xxlPath")] = globalT5Path();
        config[QStringLiteral("mode")] = QStringLiteral("image");
        return config;
    }

    if (driverId == QStringLiteral("sd_video")) {
        QString model = modelDirOrName.isEmpty() ? activeVideoProvider() : modelDirOrName;
        QString modelClean = model.remove(QStringLiteral("Local:")).trimmed();
        QVariantList manifest = verifyModelManifest(QStringLiteral("video"), modelClean);
        for (const QVariant &item : manifest) {
            QVariantMap m = item.toMap();
            if (!m.value(QStringLiteral("found")).toBool()) continue;
            QString label = m.value(QStringLiteral("label")).toString();
            if (label.contains(QStringLiteral("DiT"))) config[QStringLiteral("diffusionModelPath")] = m.value(QStringLiteral("path")).toString();
            else if (label.contains(QStringLiteral("VAE"))) config[QStringLiteral("vaePath")] = m.value(QStringLiteral("path")).toString();
            else if (label.contains(QStringLiteral("Encoder"))) config[QStringLiteral("t5xxlPath")] = m.value(QStringLiteral("path")).toString();
        }
        if (!config.contains(QStringLiteral("t5xxlPath")) || config[QStringLiteral("t5xxlPath")].toString().isEmpty()) {
            config[QStringLiteral("t5xxlPath")] = globalT5Path();
        }
        config[QStringLiteral("engineType")] = model.contains(QStringLiteral("wan"), Qt::CaseInsensitive) ? QStringLiteral("wan") : QStringLiteral("ltx");
        config[QStringLiteral("mode")] = QStringLiteral("video");
        return config;
    }

    if (driverId == QStringLiteral("kokoro_tts")) {
        QString kokoroDir = audioRootDir() + QStringLiteral("/speech/Kokoro");
        config[QStringLiteral("modelDir")] = kokoroDir;
        config[QStringLiteral("modelName")] = QStringLiteral("model.onnx");
        return config;
    }

    if (driverId == QStringLiteral("audio_cpp")) {
        QString model = modelDirOrName.isEmpty()
        ? m_settings->value(QStringLiteral("Audio/ActiveAudioCppModel"), QString()).toString()
        : modelDirOrName;
        if (model.isEmpty() && !availableAudioCppModels().isEmpty()) {
            model = availableAudioCppModels().first();
        }
        if (model.endsWith(QStringLiteral(".gguf"), Qt::CaseInsensitive)) {
            model.chop(5);
        }
        config[QStringLiteral("modelPath")] = audioRootDir() + QStringLiteral("/") + model + QStringLiteral(".gguf");
        config[QStringLiteral("modelFamily")] = getAudioCppFamily(model);
        config[QStringLiteral("task")] = QStringLiteral("gen");
        return config;
    }

    return config;
}

QStringList SettingsManager::availableAudioCppModels() const
{
    QDir dir(audioRootDir());
    if (!dir.exists()) return {};

    QStringList filters;
    filters << QStringLiteral("*.gguf");
    QStringList files = dir.entryList(filters, QDir::Files | QDir::Readable, QDir::Name);

    for (int i = 0; i < files.size(); ++i) {
        if (files[i].endsWith(QStringLiteral(".gguf"), Qt::CaseInsensitive)) {
            files[i].chop(5);
        }
    }
    return files;
}

QString SettingsManager::getAudioCppFamily(const QString &modelName) const
{
    QString lower = modelName.toLower();
    if (lower.contains(QStringLiteral("stable-audio")) || lower.contains(QStringLiteral("stable_audio"))) {
        return QStringLiteral("stable_audio");
    }
    if (lower.contains(QStringLiteral("musicgen"))) {
        return QStringLiteral("musicgen");
    }
    if (lower.contains(QStringLiteral("audioldm"))) {
        return QStringLiteral("audioldm");
    }
    return QStringLiteral("generic");
}

QString SettingsManager::resolveApiKey(const QString &driverId, const QString &customEnvName) const
{
    QProcessEnvironment sysEnv = QProcessEnvironment::systemEnvironment();

    if (driverId.startsWith(QStringLiteral("gemini_"))) {
        if (sysEnv.contains(QStringLiteral("GEMINI_API_KEY"))) return sysEnv.value(QStringLiteral("GEMINI_API_KEY"));
        if (sysEnv.contains(QStringLiteral("GOOGLE_API_KEY"))) return sysEnv.value(QStringLiteral("GOOGLE_API_KEY"));
        return QString();
    }
    if (driverId.startsWith(QStringLiteral("openai_"))) {
        return sysEnv.value(QStringLiteral("OPENAI_API_KEY"));
    }
    if (!customEnvName.isEmpty()) {
        QString upper = customEnvName.trimmed().toUpper();

        if (!upper.endsWith(QStringLiteral("_API_KEY")) && !upper.endsWith(QStringLiteral("_TOKEN"))) {
            qWarning() << "[Security] Rejected env var name:" << customEnvName
                       << "- Must end with '_API_KEY' or '_TOKEN'.";
            return QString();
        }

        static const QStringList blacklisted = {
            QStringLiteral("AWS_SECRET_ACCESS_KEY"), QStringLiteral("SSH_AUTH_SOCK"), QStringLiteral("GITHUB_TOKEN"),
            QStringLiteral("AZURE_CLIENT_SECRET"), QStringLiteral("PATH"), QStringLiteral("SYSTEMROOT"), QStringLiteral("USERPROFILE"), QStringLiteral("HOME")
        };
        if (blacklisted.contains(upper)) {
            qWarning() << "[Security] Blocked sensitive system env var:" << upper;
            return QString();
        }

        return sysEnv.value(upper);
    }

    return QString();
}

QString SettingsManager::ollamaEndpoint() const {
    return m_settings->value(QStringLiteral("Network/OllamaEndpoint"), QStringLiteral("http://127.0.0.1:11434/v1")).toString();
}

void SettingsManager::setOllamaEndpoint(const QString &endpoint) {
    if (ollamaEndpoint() != endpoint) {
        m_settings->setValue(QStringLiteral("Network/OllamaEndpoint"), endpoint);
        emit ollamaEndpointChanged();
        emit apiNetworkSettingsChanged();
    }
}

QString SettingsManager::ollamaModelName() const {
    return m_settings->value(QStringLiteral("Network/OllamaModelName"), QStringLiteral("qwen2.5:7b")).toString();
}

void SettingsManager::setOllamaModelName(const QString &modelName) {
    if (ollamaModelName() != modelName) {
        m_settings->setValue(QStringLiteral("Network/OllamaModelName"), modelName);
        emit ollamaModelNameChanged();
        emit apiNetworkSettingsChanged();
    }
}

QVariantMap SettingsManager::getRemoteDriverConfig(const QString &category, const QString &driverId) const
{
    QVariantMap result;
    const QString assetType = (category == QStringLiteral("images")) ? QStringLiteral("image") : category;

    if (driverId.startsWith(QStringLiteral("custom_"))) {
        const QVariantList providers = getProvidersForCategory(category);
        for (const QVariant &v : providers) {
            const QVariantMap map = v.toMap();
            if (map.value(QStringLiteral("isBuiltIn"), false).toBool()) continue;

            const QString name = map.value(QStringLiteral("name")).toString();
            const QString slug = QStringLiteral("custom_%1_%2").arg(assetType, name.trimmed().toLower().replace(' ', '_'));
            if (slug == driverId) {
                result[QStringLiteral("name")]      = name;
                result[QStringLiteral("baseUrl")]   = map.value(QStringLiteral("url")).toString();
                result[QStringLiteral("modelName")] = map.value(QStringLiteral("modelName")).toString();
                result[QStringLiteral("envVar")]    = map.value(QStringLiteral("envVar")).toString();
                return result;
            }
        }
        return result;
    }

           // Built-in remote driver overrides (e.g., gemini_bridge, gemini_image, openai_bridge)
    const QString prefix = QStringLiteral("RemoteOverrides/%1/").arg(driverId);
    if (m_settings->contains(prefix + QStringLiteral("baseUrl"))) {
        result[QStringLiteral("baseUrl")] = m_settings->value(prefix + QStringLiteral("baseUrl")).toString();
    }
    if (m_settings->contains(prefix + QStringLiteral("modelName"))) {
        result[QStringLiteral("modelName")] = m_settings->value(prefix + QStringLiteral("modelName")).toString();
    }
    return result;
}

bool SettingsManager::saveRemoteDriverConfig(const QString &category,
                                             const QString &driverId,
                                             const QString &baseUrl,
                                             const QString &modelName,
                                             const QString &envVar)
{
    const QString cleanUrl   = baseUrl.trimmed();
    const QString cleanModel = modelName.trimmed();
    const QString cleanEnv   = envVar.trimmed().toUpper();
    if (cleanUrl.isEmpty() || cleanModel.isEmpty()) return false;

    const QString assetType = (category == QStringLiteral("images")) ? QStringLiteral("image") : category;

    if (driverId.startsWith(QStringLiteral("custom_"))) {
        QVariantList current = getProvidersForCategory(category);
        bool updated = false;

        for (int i = 0; i < current.size(); ++i) {
            QVariantMap map = current[i].toMap();
            if (map.value(QStringLiteral("isBuiltIn"), false).toBool()) continue;

            const QString name = map.value(QStringLiteral("name")).toString();
            const QString slug = QStringLiteral("custom_%1_%2").arg(assetType, name.trimmed().toLower().replace(' ', '_'));
            if (slug == driverId) {
                map[QStringLiteral("url")]       = cleanUrl;
                map[QStringLiteral("modelName")] = cleanModel;
                map[QStringLiteral("envVar")]    = cleanEnv;
                current[i] = map;
                updated = true;
                break;
            }
        }

        if (!updated) return false;
        saveCustomProviders(category, current);
        if (category == QStringLiteral("script"))      emit scriptProvidersChanged();
        else if (category == QStringLiteral("images")) emit imageProvidersChanged();
        else if (category == QStringLiteral("video"))  emit videoProvidersChanged();
        else if (category == QStringLiteral("audio"))  emit audioProvidersChanged();
    } else {
        const QString prefix = QStringLiteral("RemoteOverrides/%1/").arg(driverId);
        m_settings->setValue(prefix + QStringLiteral("baseUrl"), cleanUrl);
        m_settings->setValue(prefix + QStringLiteral("modelName"), cleanModel);
        m_settings->sync();
    }

    emit apiNetworkSettingsChanged();
    return true;
}
