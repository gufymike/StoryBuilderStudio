// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QIcon>

#include "applogger.h"
#include "settingsmanager.h"
#include "engineregistry.h"
#include "genericrestdriver.h"
#include "projectmanager.h"
#include "storyboardmanager.h"
#include "seriescontainer.h"
#include "promptmanager.h"
#include "inferenceestimator.h"
#include "prompthistorystore.h"
#include "audittrailstore.h"
#include "storydocument.h"
#include "storyboardexporter.h"

// Workspace Models
#include "projectdirectorymodel.h"
#include "audiowaveformmodel.h"
#include "scriptsdirectorymodel.h"
#include "audiodirectorymodel.h"
#include "imagedirectorymodel.h"
#include "videodirectorymodel.h"
#include "abstractinferencedriver.h"
#include "abstractassetdirectorymodel.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral("storyBuilderStudio"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Story Builder Studio"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("gufymike.com"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));
    AppLogger::initLogging();
    Q_INIT_RESOURCE(default_prompts);
    Q_INIT_RESOURCE(resources);
    QGuiApplication app(argc, argv);
    app.setDesktopFileName(QStringLiteral("StoryBuilderStudio"));
    QIcon appIcon(QStringLiteral(":/app.png"));
    if (appIcon.isNull()) {
        appIcon = QIcon(QStringLiteral(":/app.ico"));
    }
    app.setWindowIcon(appIcon);

    QQmlApplicationEngine engine;

    // 1. Fetch active theme
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataDir);
    QString iniFilePath = dir.absoluteFilePath(QStringLiteral("workspace_config.ini"));

    QSettings storedSettings(iniFilePath, QSettings::IniFormat);
    QString activeTheme = storedSettings.value(QStringLiteral("UI/ActiveTheme"), QStringLiteral("Basic")).toString();
    QQuickStyle::setStyle(activeTheme);

    // 2. Instantiate Containers, Stores, & Managers
    auto *settingsManager = new SettingsManager(&app);
    auto *seriesContainer = new SeriesContainer(&app);
    auto *inferenceEstimator = new InferenceEstimator(&app);
    auto *projectManager = new ProjectManager(&app);
    auto *storyBoardManager = new StoryBoardManager(&app);
    auto *promptManager = new PromptManager(&app);

    PromptHistoryStore *promptHistoryStore = new PromptHistoryStore(&app);
    AuditTrailStore *auditTrailStore = new AuditTrailStore(&app);

    // 3. Initialize Engine Registry Hub
    auto *registry = new EngineRegistry(&app);

    // setup network drivers
    auto syncNetworkDrivers = [settingsManager, registry]() {
        auto syncBuiltIn = [registry, settingsManager](const QString &id,
                                                       const QString &cat,
                                                       const QString &assetType,
                                                       GenericRestDriver::ProtocolFlavor flavor,
                                                       const QString &apiKey,
                                                       const QString &defaultModel,
                                                       const QString &defaultBaseUrl = QString()) {
            AbstractInferenceDriver *d = registry->getDriver(id);
            if (!d) {
                d = new GenericRestDriver(id, flavor, assetType, registry);
                registry->registerDriver(d);
            }

            const QVariantMap saved = settingsManager->getRemoteDriverConfig(cat, id);
            QVariantMap cfg;
            if (!apiKey.isEmpty()) {
                cfg[QStringLiteral("apiKey")] = apiKey;
            }
            cfg[QStringLiteral("modelName")] = saved.value(QStringLiteral("modelName"), defaultModel).toString();

            const QString baseUrl = saved.value(QStringLiteral("baseUrl"), defaultBaseUrl).toString();
            if (!baseUrl.isEmpty()) {
                cfg[QStringLiteral("baseUrl")] = baseUrl;
            }
            d->loadModel(cfg);
        };

               // 1. Built-in Google Gemini drivers
        const QString geminiKey = settingsManager->property("geminiApiKey").toString();
        syncBuiltIn(QStringLiteral("gemini_bridge"), QStringLiteral("script"), QStringLiteral("script"),
                    GenericRestDriver::Protocol_Gemini, geminiKey, QStringLiteral("gemini-3.5-flash-lite"));
        syncBuiltIn(QStringLiteral("gemini_image"),  QStringLiteral("images"), QStringLiteral("image"),
                    GenericRestDriver::Protocol_Gemini, geminiKey, QStringLiteral("imagen-3.0-generate-002"));
        syncBuiltIn(QStringLiteral("gemini_audio"),  QStringLiteral("audio"),  QStringLiteral("audio"),
                    GenericRestDriver::Protocol_Gemini, geminiKey, QStringLiteral("gemini-3.8-flash-tts"));
        syncBuiltIn(QStringLiteral("gemini_video"),  QStringLiteral("video"),  QStringLiteral("video"),
                    GenericRestDriver::Protocol_Gemini, geminiKey, QStringLiteral("veo-2.0-generate-001"));

               // 2. Built-in OpenAI drivers
        const QString openAiKey = settingsManager->property("openAiApiKey").toString();
        syncBuiltIn(QStringLiteral("openai_bridge"), QStringLiteral("script"), QStringLiteral("script"),
                    GenericRestDriver::Protocol_OpenAI, openAiKey, QStringLiteral("gpt-4o"));
        syncBuiltIn(QStringLiteral("openai_image"),  QStringLiteral("images"), QStringLiteral("image"),
                    GenericRestDriver::Protocol_OpenAI, openAiKey, QStringLiteral("gpt-image-1"));
        syncBuiltIn(QStringLiteral("openai_audio"),  QStringLiteral("audio"),  QStringLiteral("audio"),
                    GenericRestDriver::Protocol_OpenAI, openAiKey, QStringLiteral("tts-1"));

               // 3. Built-in Ollama local driver
        syncBuiltIn(QStringLiteral("ollama_local"),  QStringLiteral("script"), QStringLiteral("script"),
                    GenericRestDriver::Protocol_OpenAI, QString(),
                    settingsManager->ollamaModelName(), settingsManager->ollamaEndpoint());

               // 4. Dynamically spawn & sync all user-configured OpenAI-compatible endpoints
        const QList<QPair<QString, QString>> categories = {
            {QStringLiteral("script"), QStringLiteral("script")},
            {QStringLiteral("images"), QStringLiteral("image")},
            {QStringLiteral("audio"),  QStringLiteral("audio")},
            {QStringLiteral("video"),  QStringLiteral("video")}
        };

        for (const auto &[settingsCat, assetType] : categories) {
            const QVariantList providers = settingsManager->getProvidersForCategory(settingsCat);
            for (const QVariant &v : providers) {
                const QVariantMap p = v.toMap();
                if (p.value(QStringLiteral("type")).toString() != QStringLiteral("custom")) {
                    continue;
                }

                const QString name     = p.value(QStringLiteral("name")).toString().trimmed();
                const QString url      = p.value(QStringLiteral("url")).toString();
                const QString model    = p.value(QStringLiteral("modelName")).toString();
                const QString envVar   = p.value(QStringLiteral("envVar")).toString();
                const QString driverId = QStringLiteral("custom_%1_%2").arg(assetType, name.toLower().replace(' ', '_'));

                AbstractInferenceDriver *driver = registry->getDriver(driverId);
                if (!driver) {
                    driver = new GenericRestDriver(driverId, GenericRestDriver::Protocol_OpenAI, assetType, registry);
                    registry->registerDriver(driver);
                }

                QVariantMap cfg;
                cfg[QStringLiteral("displayName")] = name;
                cfg[QStringLiteral("baseUrl")]     = url;
                cfg[QStringLiteral("modelName")]   = model;
                if (!envVar.isEmpty()) {
                    cfg[QStringLiteral("apiKey")] = settingsManager->resolveApiKey(driverId, envVar);
                }
                driver->loadModel(cfg);
            }
        }
    };

           // Hot-reload drivers when API keys or custom providers change
    QObject::connect(settingsManager, &SettingsManager::apiNetworkSettingsChanged, registry, syncNetworkDrivers);

           // Unregister custom REST drivers immediately when removed in Settings
    QObject::connect(settingsManager, &SettingsManager::customProviderRemoved, registry, &EngineRegistry::unregisterDriver);

           // Defer initial sync until after the event loop starts so plugin discovery has finished
    QMetaObject::invokeMethod(&app, syncNetworkDrivers, Qt::QueuedConnection);

    // =========================================================================
    // 7. Register Module Singletons (PascalCase strictly required by Qt QML)
    // =========================================================================
    qmlRegisterSingletonInstance("StoryBuilderStudio", 1, 0, "AuditTrailStore", auditTrailStore);
    qmlRegisterSingletonInstance("StoryBuilderStudio", 1, 0, "SettingsManager", settingsManager);
    qmlRegisterSingletonInstance("StoryBuilderStudio", 1, 0, "EngineRegistry", registry);
    qmlRegisterSingletonInstance("StoryBuilderStudio", 1, 0, "ProjectManager", projectManager);
    qmlRegisterSingletonInstance("StoryBuilderStudio", 1, 0, "StoryBoardManager", storyBoardManager);
    qmlRegisterSingletonInstance("StoryBuilderStudio", 1, 0, "SeriesContainer", seriesContainer);
    qmlRegisterSingletonInstance("StoryBuilderStudio", 1, 0, "PromptManager", promptManager);
    qmlRegisterSingletonInstance("StoryBuilderStudio", 1, 0, "PromptHistoryStore", promptHistoryStore);
    qmlRegisterSingletonInstance("StoryBuilderStudio", 1, 0, "InferenceEstimator", inferenceEstimator);

    // =========================================================================
    // 8. Global Root Context Properties (camelCase for clean QML bindings)
    // =========================================================================
    auto *rootCtx = engine.rootContext();

    // Singletons exposed in camelCase
    rootCtx->setContextProperty(QStringLiteral("settingsManager"), settingsManager);
    rootCtx->setContextProperty(QStringLiteral("engineRegistry"), registry);
    rootCtx->setContextProperty(QStringLiteral("projectManager"), projectManager);
    rootCtx->setContextProperty(QStringLiteral("storyBoardManager"), storyBoardManager);
    rootCtx->setContextProperty(QStringLiteral("seriesContainer"), seriesContainer);
    rootCtx->setContextProperty(QStringLiteral("promptManager"), promptManager);
    rootCtx->setContextProperty(QStringLiteral("inferenceEstimator"), inferenceEstimator);

    // Operational Stores & Paths
    rootCtx->setContextProperty(QStringLiteral("applicationDirPath"), QCoreApplication::applicationDirPath());
    rootCtx->setContextProperty(QStringLiteral("promptHistoryStore"), promptHistoryStore);
    rootCtx->setContextProperty(QStringLiteral("auditTrailStore"), auditTrailStore);

    // 9. QML Type Registrations
    qmlRegisterType<StoryDocument>("StoryBuilderStudio", 1, 0, "StoryDocument");
    qmlRegisterType<StoryBoardExporter>("StoryBuilderStudio", 1, 0, "StoryBoardExporter");
    qmlRegisterType<ProjectDirectoryModel>("StoryBuilderStudio", 1, 0, "ProjectDirectoryModel");
    qmlRegisterType<AudioWaveformModel>("StoryBuilderStudio", 1, 0, "AudioWaveformModel");
    qmlRegisterType<ScriptsDirectoryModel>("StoryBuilderStudio", 1, 0, "ScriptsDirectoryModel");
    qmlRegisterType<AudioDirectoryModel>("StoryBuilderStudio", 1, 0, "AudioDirectoryModel");
    qmlRegisterType<ImageDirectoryModel>("StoryBuilderStudio", 1, 0, "ImageDirectoryModel");
    qmlRegisterType<VideoDirectoryModel>("StoryBuilderStudio", 1, 0, "VideoDirectoryModel");

    qmlRegisterUncreatableType<AbstractInferenceDriver>(
        "StoryBuilderStudio", 1, 0, "AbstractInferenceDriver",
        QStringLiteral("AbstractInferenceDriver is an abstract interface contract.")
        );
    qmlRegisterUncreatableType<AbstractAssetDirectoryModel>(
        "StoryBuilderStudio", 1, 0, "AbstractAssetDirectoryModel",
        QStringLiteral("AbstractAssetDirectoryModel is an abstract interface contract.")
        );

    // connect promptHistoryStore->projecPath t0 projectManager->projectPath
    QObject::connect(projectManager, &ProjectManager::currentProjectPathChanged,
                     promptHistoryStore, [projectManager, promptHistoryStore]() {
                         promptHistoryStore->setProjectPath(projectManager->currentProjectPath());
                     });

    QObject::connect(projectManager, &ProjectManager::currentProjectPathChanged,
                     storyBoardManager, [projectManager, storyBoardManager]() {
                         storyBoardManager->setEpisodePath(projectManager->currentProjectPath());
                     });
    // Prime the path immediately in case a project is already loaded
    if (projectManager->isProjectOpen()) {
        promptHistoryStore->setProjectPath(projectManager->currentProjectPath());
        storyBoardManager->setEpisodePath(projectManager->currentProjectPath());
    }

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
        );

    engine.loadFromModule("StoryBuilderStudio", "Main");

    return QGuiApplication::exec();
}
