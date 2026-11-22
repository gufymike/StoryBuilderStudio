/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: abstractinferencedriver.h
** The abstract base contract for all local and remote AI inference backends.
**
****************************************************************************/
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QtQml/QQmlEngine>

/**
 * @brief Schema descriptor for model-specific parameters (CFG, steps, temp, etc.)
 */
struct DriverOptionDescriptor {
    QString key;          // Internal ID ("steps", "cfg_scale", "temperature")
    QString label;        // Display name ("Inference Steps")
    QString type;         // "int", "float", "bool", "string", "enum"
    QVariant defaultValue;
    QVariant minValue;    // For int/float sliders
    QVariant maxValue;
    QStringList choices;  // For enum ComboBoxes (e.g., voices or schedulers)
    QString description;
};

/**
 * @class AbstractInferenceDriver
 * @brief The abstract base contract for all local and remote AI inference backends.
 *
 * This class normalizes interactions across vastly different AI modalities (text, audio,
 * image, video) and execution environments (local C++ DLL plugins vs. remote REST APIs).
 * It exposes state and capability properties directly to the QML engine.
 */
class AbstractInferenceDriver : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("AbstractInferenceDriver is an interface and cannot be directly instantiated in QML")

    // --- Standard State Properties ---
    /**
     * @property driverId
     * @brief The unique internal system identifier for this driver (e.g., "llama_cpp", "gemini_bridge").
     */
    Q_PROPERTY(QString driverId READ driverId CONSTANT)
    /**
     * @property displayName
     * @brief The formatted, user-facing name displayed in the UI dropdowns.
     */
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    /**
     * @property targetAssetType
     * @brief used by the ui to know which tab and controls to show.
     */
    Q_PROPERTY(QString targetAssetType READ targetAssetType CONSTANT)
    /**
     * @property isModelLoaded
     * @brief a bool to check if the model is loaded but may not be busy.
     */
    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY modelLoadedChanged)
    /**
     * @property isBusy
     * @brief a bool to check if the model is busy/working.
     */
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(QString loadedModelName READ loadedModelName NOTIFY loadedModelNameChanged)

    // --- Dynamic Options Manifest ---
    Q_PROPERTY(QVariantList availableOptions READ availableOptions NOTIFY optionsDiscovered)
    Q_PROPERTY(QVariantMap currentOptions READ currentOptions NOTIFY optionsChanged)

public:
    explicit AbstractInferenceDriver(QObject *parent = nullptr) : QObject(parent) {}
    ~AbstractInferenceDriver() override = default;

    // --- Mandatory Metadata Identifiers ---
    virtual QString driverId() const = 0;           // e.g., "llama_cpp", "flux_sd", "kokoro_tts", "audiocpp"
    virtual QString displayName() const = 0;        // e.g., "Llama CPP", "Flux", "Kokoro", "Audio CPP"
    virtual QString targetAssetType() const = 0;    // e.g., "script", "image", "audio", "video"

    // --- Mandatory Lifecycle Contract ---
    virtual bool isModelLoaded() const = 0;
    virtual bool isBusy() const = 0;
    virtual QString loadedModelName() const = 0;

    /**
     * @brief Load model weights into memory.
     * @param config Map of paths and loading flags (e.g. {"modelPath": "...", "nCtx": 4096})
     */
    Q_INVOKABLE virtual bool loadModel(const QVariantMap &config) = 0;

    /**
     * @brief Free all hardware resources, contexts, and VRAM/RAM buffers.
     */
    Q_INVOKABLE virtual void unloadModel() = 0;

    /**
     * @brief Abort any running generation asynchronously.
     */
    Q_INVOKABLE virtual void cancelGeneration() = 0;

    // --- Dynamic Parameter Contract (Push / Pull) ---
    /**
     * @brief Returns the list of parameter descriptors that this engine exposes.
     */
    Q_INVOKABLE virtual QVariantList availableOptions() const = 0;

    /**
     * @brief Returns current active options as key-value pairs.
     */
    Q_INVOKABLE virtual QVariantMap currentOptions() const = 0;

    /**
     * @brief Push a specific option update from the QML UI to the C++ runtime.
     */
    Q_INVOKABLE virtual bool setOption(const QString &key, const QVariant &value) = 0;

    /**
     * @brief Bulk push options from QML.
     */
    Q_INVOKABLE virtual void setOptions(const QVariantMap &options) {
        for (auto it = options.begin(); it != options.end(); ++it) {
            setOption(it.key(), it.value());
        }
    }

    // --- Unified Generation Entrypoint ---
    /**
     * @brief Asynchronously executes an inference generation request.
     *
     * Implementations must emit tokenGenerated() for streaming outputs and
     * generationFinished() upon total completion.
     *
     * @param prompt The primary user instruction or script text.
     * @param systemPrompt The overarching directive or system instruction.
     * @param attachedFiles List of absolute file paths to local contextual assets.
     * @param outputDirectory The target directory for generated media (audio/image/video).
     * @param runtimeOverrides Key-value map overriding default engine parameters for this specific run.
     */
    Q_INVOKABLE virtual void generate(const QString &prompt,
                                      const QString &systemPrompt = QString(),
                                      const QStringList &attachedFiles = QStringList(),
                                      const QString &outputDirectory = QString(),
                                      const QVariantMap &runtimeOverrides = QVariantMap()) = 0;

    // checks the driver and/or network connection.
    Q_INVOKABLE virtual void verifyConnection() = 0;

signals:
    // Lifecycle
    void modelLoadedChanged(bool loaded);
    void busyChanged(bool busy);
    void loadedModelNameChanged(const QString &name);
    void engineError(const QString &errorMessage);

    // Options
    void optionsDiscovered();
    void optionsChanged();

    // Stream & Progress
    void tokenGenerated(const QString &token); // For streaming LLMs
    void progressUpdated(int currentStep, int totalSteps, float stepLatency);

    // Standardized Result Payload
    // Script: {"text": str, "think": str, "timestamp": str}
    // Image:  {"candidates": [base64_strings], "count": int}
    // Audio:  {"filePath": str, "wavBytes": QByteArray}
    // Video:  {"outputDir": str, "frameCount": int, "fps": int}
    void generationFinished(const QVariantMap &resultPayload);

    void connectionVerified(bool success, const QString &message);
};

#define AbstractInferenceDriver_iid "com.gufymike.StoryBuilderStudio.AbstractInferenceDriver/1.0"
Q_DECLARE_INTERFACE(AbstractInferenceDriver, AbstractInferenceDriver_iid)
