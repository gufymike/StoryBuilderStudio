/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: genericrestdriver.cpp
** Remote inference driver supporting OpenAI, Ollama, and Google Gemini REST endpoints
** across script, image, audio, and video modalities.
**
****************************************************************************/
#include "genericrestdriver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>

GenericRestDriver::GenericRestDriver(const QString &driverId,
                                     ProtocolFlavor flavor,
                                     const QString &assetType,
                                     QObject *parent)
    : AbstractInferenceDriver(parent)
      , m_driverId(driverId)
      , m_flavor(flavor)
      , m_targetAssetType(assetType)
      , m_netManager(new QNetworkAccessManager(this))
      , m_pollTimer(new QTimer(this))
{
    m_pollTimer->setInterval(5000);
    connect(m_pollTimer, &QTimer::timeout, this, &GenericRestDriver::pollVideoOperation);

    initModalityDefaults();
}

GenericRestDriver::~GenericRestDriver()
{
    cancelGeneration();
}

void GenericRestDriver::initModalityDefaults()
{
    m_currentOptions.clear();

    if (m_flavor == Protocol_Gemini) {
        m_baseUrl = QStringLiteral("https://generativelanguage.googleapis.com");
        if (m_targetAssetType == QStringLiteral("image")) {
            m_modelName = QStringLiteral("imagen-3.0-generate-002");
        } else if (m_targetAssetType == QStringLiteral("video")) {
            m_modelName = QStringLiteral("veo-2.0-generate-001");
        } else {
            m_modelName = QStringLiteral("gemini-3.5-flash-lite");
        }
    } else {
        m_baseUrl = QStringLiteral("https://api.openai.com/v1");
        if (m_targetAssetType == QStringLiteral("image")) {
            m_modelName = QStringLiteral("gpt-image-1");
        } else if (m_targetAssetType == QStringLiteral("audio")) {
            m_modelName = QStringLiteral("tts-1");
        } else if (m_targetAssetType == QStringLiteral("video")) {
            m_modelName = QStringLiteral("video-gen-1");
        } else {
            m_modelName = QStringLiteral("gpt-4o");
        }
    }

    if (m_targetAssetType == QStringLiteral("image")) {
        m_currentOptions[QStringLiteral("sample_count")] = 4;
        m_currentOptions[QStringLiteral("aspect_ratio")] = QStringLiteral("16:9");
        m_currentOptions[QStringLiteral("size")]         = QStringLiteral("1024x1024");
    } else if (m_targetAssetType == QStringLiteral("audio")) {
        m_currentOptions[QStringLiteral("voice")]           = QStringLiteral("alloy");
        m_currentOptions[QStringLiteral("speed")]           = 1.0f;
        m_currentOptions[QStringLiteral("response_format")] = QStringLiteral("wav");
    } else if (m_targetAssetType == QStringLiteral("video")) {
        m_currentOptions[QStringLiteral("aspect_ratio")]     = QStringLiteral("16:9");
        m_currentOptions[QStringLiteral("duration_seconds")] = 5;
    } else {
        m_currentOptions[QStringLiteral("temperature")] = 0.7f;
        m_currentOptions[QStringLiteral("top_p")]       = 0.95f;
        m_currentOptions[QStringLiteral("stream")]      = true;
    }
}

QString GenericRestDriver::displayName() const
{
    if (!m_displayName.isEmpty()) return m_displayName;

    if (m_driverId == QStringLiteral("gemini_bridge")) return QStringLiteral("Google Gemini");
    if (m_driverId == QStringLiteral("gemini_image"))  return QStringLiteral("Google Imagen");
    if (m_driverId == QStringLiteral("gemini_audio"))  return QStringLiteral("Google Gemini Audio");
    if (m_driverId == QStringLiteral("gemini_video"))  return QStringLiteral("Google Veo");
    if (m_driverId == QStringLiteral("openai_bridge")) return QStringLiteral("OpenAI");
    if (m_driverId == QStringLiteral("ollama_local"))  return QStringLiteral("Ollama Local");
    return m_driverId;
}

bool GenericRestDriver::isModelLoaded() const
{
    if (m_baseUrl.isEmpty() || m_modelName.isEmpty()) return false;
    if (m_baseUrl.contains(QStringLiteral("localhost")) || m_baseUrl.contains(QStringLiteral("127.0.0.1"))) {
        return true;
    }
    return !m_apiKey.isEmpty();
}

bool GenericRestDriver::loadModel(const QVariantMap &config)
{
    unloadModel();

    if (config.contains(QStringLiteral("displayName"))) m_displayName = config.value(QStringLiteral("displayName")).toString().trimmed();
    if (config.contains(QStringLiteral("baseUrl")))     m_baseUrl = config.value(QStringLiteral("baseUrl")).toString();
    if (config.contains(QStringLiteral("apiKey")))      m_apiKey = config.value(QStringLiteral("apiKey")).toString().trimmed();
    if (config.contains(QStringLiteral("modelName")))   m_modelName = config.value(QStringLiteral("modelName")).toString().trimmed();

    emit modelLoadedChanged(isModelLoaded());
    emit loadedModelNameChanged(m_modelName);
    emit optionsDiscovered();

    qInfo() << "[GenericRestDriver]" << m_driverId << "(" << m_targetAssetType << ") configured for"
            << m_modelName << "@" << m_baseUrl;
    return isModelLoaded();
}

void GenericRestDriver::unloadModel()
{
    cancelGeneration();
    m_apiKey.clear();
    emit modelLoadedChanged(false);
}

void GenericRestDriver::cancelGeneration()
{
    if (m_pollTimer && m_pollTimer->isActive()) {
        m_pollTimer->stop();
    }
    m_pendingOperationId.clear();

    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
    if (m_isBusy) {
        m_isBusy = false;
        emit busyChanged(false);
    }
}

QVariantList GenericRestDriver::availableOptions() const
{
    QVariantList list;

    if (m_targetAssetType == QStringLiteral("image")) {
        list.append(QVariantMap{
            {QStringLiteral("key"), QStringLiteral("sample_count")},
            {QStringLiteral("label"), QStringLiteral("Candidate Count")},
            {QStringLiteral("type"), QStringLiteral("int")},
            {QStringLiteral("defaultValue"), 4},
            {QStringLiteral("minValue"), 1},
            {QStringLiteral("maxValue"), 4},
            {QStringLiteral("description"), QStringLiteral("Number of image candidates to generate.")}
        });
        list.append(QVariantMap{
            {QStringLiteral("key"), QStringLiteral("aspect_ratio")},
            {QStringLiteral("label"), QStringLiteral("Aspect Ratio")},
            {QStringLiteral("type"), QStringLiteral("enum")},
            {QStringLiteral("defaultValue"), QStringLiteral("16:9")},
            {QStringLiteral("choices"), QStringList{QStringLiteral("1:1"), QStringLiteral("16:9"), QStringLiteral("9:16"), QStringLiteral("4:3")}},
            {QStringLiteral("description"), QStringLiteral("Output aspect ratio.")}
        });
        return list;
    }

    if (m_targetAssetType == QStringLiteral("audio")) {
        list.append(QVariantMap{
            {QStringLiteral("key"), QStringLiteral("voice")},
            {QStringLiteral("label"), QStringLiteral("Voice")},
            {QStringLiteral("type"), QStringLiteral("enum")},
            {QStringLiteral("defaultValue"), QStringLiteral("alloy")},
            {QStringLiteral("choices"), QStringList{QStringLiteral("alloy"), QStringLiteral("echo"), QStringLiteral("fable"), QStringLiteral("onyx"), QStringLiteral("nova"), QStringLiteral("shimmer")}},
            {QStringLiteral("description"), QStringLiteral("Voice preset for speech synthesis.")}
        });
        list.append(QVariantMap{
            {QStringLiteral("key"), QStringLiteral("speed")},
            {QStringLiteral("label"), QStringLiteral("Playback Speed")},
            {QStringLiteral("type"), QStringLiteral("float")},
            {QStringLiteral("defaultValue"), 1.0f},
            {QStringLiteral("minValue"), 0.25f},
            {QStringLiteral("maxValue"), 4.0f},
            {QStringLiteral("description"), QStringLiteral("Speech synthesis speed multiplier.")}
        });
        return list;
    }

    if (m_targetAssetType == QStringLiteral("video")) {
        list.append(QVariantMap{
            {QStringLiteral("key"), QStringLiteral("aspect_ratio")},
            {QStringLiteral("label"), QStringLiteral("Aspect Ratio")},
            {QStringLiteral("type"), QStringLiteral("enum")},
            {QStringLiteral("defaultValue"), QStringLiteral("16:9")},
            {QStringLiteral("choices"), QStringList{QStringLiteral("16:9"), QStringLiteral("9:16")}},
            {QStringLiteral("description"), QStringLiteral("Output video aspect ratio.")}
        });
        list.append(QVariantMap{
            {QStringLiteral("key"), QStringLiteral("duration_seconds")},
            {QStringLiteral("label"), QStringLiteral("Duration (sec)")},
            {QStringLiteral("type"), QStringLiteral("int")},
            {QStringLiteral("defaultValue"), 5},
            {QStringLiteral("minValue"), 2},
            {QStringLiteral("maxValue"), 8},
            {QStringLiteral("description"), QStringLiteral("Target video duration in seconds.")}
        });
        return list;
    }

           // Default: script / text
    list.append(QVariantMap{
        {QStringLiteral("key"), QStringLiteral("temperature")},
        {QStringLiteral("label"), QStringLiteral("Temperature")},
        {QStringLiteral("type"), QStringLiteral("float")},
        {QStringLiteral("defaultValue"), 0.7f},
        {QStringLiteral("minValue"), 0.0f},
        {QStringLiteral("maxValue"), 2.0f},
        {QStringLiteral("description"), QStringLiteral("Randomness factor in sampling.")}
    });
    list.append(QVariantMap{
        {QStringLiteral("key"), QStringLiteral("top_p")},
        {QStringLiteral("label"), QStringLiteral("Top-P")},
        {QStringLiteral("type"), QStringLiteral("float")},
        {QStringLiteral("defaultValue"), 0.95f},
        {QStringLiteral("minValue"), 0.0f},
        {QStringLiteral("maxValue"), 1.0f},
        {QStringLiteral("description"), QStringLiteral("Nucleus sampling probability mass.")}
    });
    list.append(QVariantMap{
        {QStringLiteral("key"), QStringLiteral("stream")},
        {QStringLiteral("label"), QStringLiteral("Stream Response")},
        {QStringLiteral("type"), QStringLiteral("bool")},
        {QStringLiteral("defaultValue"), true},
        {QStringLiteral("description"), QStringLiteral("Receive tokens incrementally via Server-Sent Events.")}
    });

    return list;
}

bool GenericRestDriver::setOption(const QString &key, const QVariant &value)
{
    m_currentOptions[key] = value;
    emit optionsChanged();
    return true;
}

QNetworkRequest GenericRestDriver::buildRequest(bool streaming) const
{
    QUrl url;
    if (m_flavor == Protocol_Gemini) {
        if (m_targetAssetType == QStringLiteral("image")) {
            url = QUrl(QStringLiteral("%1/v1beta/models/%2:predict").arg(m_baseUrl, m_modelName));
        } else if (m_targetAssetType == QStringLiteral("video")) {
            url = QUrl(QStringLiteral("%1/v1beta/models/%2:predictLongRunning").arg(m_baseUrl, m_modelName));
        } else if (m_targetAssetType == QStringLiteral("audio")) {
            url = QUrl(QStringLiteral("%1/v1beta/models/%2:generateContent").arg(m_baseUrl, m_modelName));
        } else {
            QString action = streaming ? QStringLiteral("streamGenerateContent?alt=sse") : QStringLiteral("generateContent");
            url = QUrl(QStringLiteral("%1/v1beta/models/%2:%3").arg(m_baseUrl, m_modelName, action));
        }
    } else {
        if (m_targetAssetType == QStringLiteral("image")) {
            url = QUrl(m_baseUrl.endsWith(QStringLiteral("/images/generations")) ? m_baseUrl : m_baseUrl + QStringLiteral("/images/generations"));
        } else if (m_targetAssetType == QStringLiteral("audio")) {
            url = QUrl(m_baseUrl.endsWith(QStringLiteral("/audio/speech")) ? m_baseUrl : m_baseUrl + QStringLiteral("/audio/speech"));
        } else if (m_targetAssetType == QStringLiteral("video")) {
            url = QUrl(m_baseUrl.endsWith(QStringLiteral("/videos/generations")) ? m_baseUrl : m_baseUrl + QStringLiteral("/videos/generations"));
        } else {
            url = QUrl(m_baseUrl.endsWith(QStringLiteral("/chat/completions")) ? m_baseUrl : m_baseUrl + QStringLiteral("/chat/completions"));
        }
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    if (m_flavor == Protocol_Gemini) {
        request.setRawHeader("x-goog-api-key", m_apiKey.toUtf8());
    } else if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_apiKey).toUtf8());
    }

    return request;
}

QByteArray GenericRestDriver::buildOpenAiPayload(const QString &prompt, const QString &systemPrompt,
                                                 const QStringList &files, const QVariantMap &opts, bool streaming) const
{
    QJsonObject root;
    root[QStringLiteral("model")] = m_modelName;
    root[QStringLiteral("stream")] = streaming;
    root[QStringLiteral("temperature")] = opts.value(QStringLiteral("temperature"), 0.7f).toDouble();
    root[QStringLiteral("top_p")] = opts.value(QStringLiteral("top_p"), 0.95f).toDouble();

    QJsonArray messages;
    if (!systemPrompt.isEmpty()) {
        QJsonObject sys;
        sys[QStringLiteral("role")] = QStringLiteral("system");
        sys[QStringLiteral("content")] = systemPrompt;
        messages.append(sys);
    }

    QString fileContext;
    for (const QString &path : files) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QFileInfo fi(path);
            fileContext += QStringLiteral("\n[File: %1]\n%2\n").arg(fi.fileName(), QTextStream(&f).readAll());
        }
    }

    QJsonObject userMsg;
    userMsg[QStringLiteral("role")] = QStringLiteral("user");
    userMsg[QStringLiteral("content")] = fileContext.isEmpty() ? prompt : (fileContext + QStringLiteral("\n") + prompt);
    messages.append(userMsg);

    root[QStringLiteral("messages")] = messages;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QByteArray GenericRestDriver::buildGeminiPayload(const QString &prompt, const QString &systemPrompt,
                                                 const QStringList &files, const QVariantMap &opts) const
{
    QJsonObject root;

    if (!systemPrompt.isEmpty()) {
        QJsonObject sys;
        QJsonArray parts;
        QJsonObject part;
        part[QStringLiteral("text")] = systemPrompt;
        parts.append(part);
        sys[QStringLiteral("parts")] = parts;
        root[QStringLiteral("systemInstruction")] = sys;
    }

    QJsonObject genConfig;
    genConfig[QStringLiteral("temperature")] = opts.value(QStringLiteral("temperature"), 0.7f).toDouble();
    genConfig[QStringLiteral("topP")] = opts.value(QStringLiteral("top_p"), 0.95f).toDouble();
    root[QStringLiteral("generationConfig")] = genConfig;

    QJsonArray contents;
    QJsonObject contentObj;
    contentObj[QStringLiteral("role")] = QStringLiteral("user");

    QJsonArray parts;
    for (const QString &path : files) {
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QJsonObject fPart;
            fPart[QStringLiteral("text")] = QStringLiteral("[Context File: %1]\n%2").arg(QFileInfo(path).fileName(), QTextStream(&f).readAll());
            parts.append(fPart);
        }
    }

    QJsonObject textPart;
    textPart[QStringLiteral("text")] = prompt;
    parts.append(textPart);

    contentObj[QStringLiteral("parts")] = parts;
    contents.append(contentObj);
    root[QStringLiteral("contents")] = contents;

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QByteArray GenericRestDriver::buildImagePayload(const QString &prompt, const QVariantMap &opts) const
{
    int sampleCount = opts.value(QStringLiteral("sample_count"), 4).toInt();
    QString aspectRatio = opts.value(QStringLiteral("aspect_ratio"), QStringLiteral("16:9")).toString();

    QJsonObject root;
    if (m_flavor == Protocol_Gemini) {
        QJsonArray instances;
        QJsonObject inst;
        inst[QStringLiteral("prompt")] = prompt;
        instances.append(inst);

        QJsonObject params;
        params[QStringLiteral("sampleCount")] = sampleCount;
        params[QStringLiteral("aspectRatio")] = aspectRatio;

        root[QStringLiteral("instances")] = instances;
        root[QStringLiteral("parameters")] = params;
    } else {
        root[QStringLiteral("model")] = m_modelName;
        root[QStringLiteral("prompt")] = prompt;
        root[QStringLiteral("n")] = sampleCount;
        root[QStringLiteral("size")] = opts.value(QStringLiteral("size"), QStringLiteral("1024x1024")).toString();
        root[QStringLiteral("response_format")] = QStringLiteral("b64_json");
    }
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QByteArray GenericRestDriver::buildAudioPayload(const QString &prompt, const QVariantMap &opts) const
{
    QJsonObject root;
    if (m_flavor == Protocol_Gemini) {
        QJsonObject genConfig;
        genConfig[QStringLiteral("responseModalities")] = QJsonArray{QStringLiteral("AUDIO")};
        root[QStringLiteral("generationConfig")] = genConfig;

        QJsonObject textPart;
        textPart[QStringLiteral("text")] = prompt;
        QJsonObject contentObj;
        contentObj[QStringLiteral("role")] = QStringLiteral("user");
        contentObj[QStringLiteral("parts")] = QJsonArray{textPart};
        root[QStringLiteral("contents")] = QJsonArray{contentObj};
    } else {
        root[QStringLiteral("model")] = m_modelName;
        root[QStringLiteral("input")] = prompt;
        root[QStringLiteral("voice")] = opts.value(QStringLiteral("voice"), QStringLiteral("alloy")).toString();
        root[QStringLiteral("speed")] = opts.value(QStringLiteral("speed"), 1.0f).toDouble();
        root[QStringLiteral("response_format")] = opts.value(QStringLiteral("response_format"), QStringLiteral("wav")).toString();
    }
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QByteArray GenericRestDriver::buildVideoPayload(const QString &prompt, const QVariantMap &opts) const
{
    QString aspectRatio = opts.value(QStringLiteral("aspect_ratio"), QStringLiteral("16:9")).toString();
    int duration = opts.value(QStringLiteral("duration_seconds"), 5).toInt();

    QJsonObject root;
    if (m_flavor == Protocol_Gemini) {
        QJsonObject inst;
        inst[QStringLiteral("prompt")] = prompt;
        root[QStringLiteral("instances")] = QJsonArray{inst};

        QJsonObject params;
        params[QStringLiteral("aspectRatio")] = aspectRatio;
        params[QStringLiteral("durationSeconds")] = duration;
        root[QStringLiteral("parameters")] = params;
    } else {
        root[QStringLiteral("model")] = m_modelName;
        root[QStringLiteral("prompt")] = prompt;
        root[QStringLiteral("aspect_ratio")] = aspectRatio;
        root[QStringLiteral("duration")] = duration;
    }
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

void GenericRestDriver::generate(const QString &prompt,
                                 const QString &systemPrompt,
                                 const QStringList &attachedFiles,
                                 const QString &outputDirectory,
                                 const QVariantMap &runtimeOverrides)
{
    if (!isModelLoaded()) {
        emit engineError(tr("Remote driver not configured or missing API credentials."));
        return;
    }

    cancelGeneration();

    m_isBusy = true;
    m_currentOutputDir = outputDirectory;
    m_accumulatedText.clear();
    m_accumulatedThink.clear();
    m_streamBuffer.clear();
    m_isInsideThinkTag = false;
    emit busyChanged(true);

    QVariantMap activeOpts = m_currentOptions;
    for (auto it = runtimeOverrides.constBegin(); it != runtimeOverrides.constEnd(); ++it) {
        activeOpts[it.key()] = it.value();
    }

    bool streaming = (m_targetAssetType == QStringLiteral("script")) &&
                     activeOpts.value(QStringLiteral("stream"), true).toBool();

    QNetworkRequest request = buildRequest(streaming);
    QByteArray body;

    if (m_targetAssetType == QStringLiteral("image")) {
        body = buildImagePayload(prompt, activeOpts);
    } else if (m_targetAssetType == QStringLiteral("audio")) {
        body = buildAudioPayload(prompt, activeOpts);
    } else if (m_targetAssetType == QStringLiteral("video")) {
        body = buildVideoPayload(prompt, activeOpts);
    } else {
        body = (m_flavor == Protocol_Gemini)
        ? buildGeminiPayload(prompt, systemPrompt, attachedFiles, activeOpts)
        : buildOpenAiPayload(prompt, systemPrompt, attachedFiles, activeOpts, streaming);
    }

    m_currentReply = m_netManager->post(request, body);

    if (streaming) {
        connect(m_currentReply, &QNetworkReply::readyRead, this, &GenericRestDriver::onReadyRead);
    }
    connect(m_currentReply, &QNetworkReply::finished, this, &GenericRestDriver::onReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, &GenericRestDriver::onNetworkError);
}

void GenericRestDriver::onReadyRead()
{
    if (!m_currentReply) return;

    m_streamBuffer.append(m_currentReply->readAll());

    while (m_streamBuffer.contains('\n')) {
        int idx = m_streamBuffer.indexOf('\n');
        QByteArray rawLine = m_streamBuffer.left(idx).trimmed();
        m_streamBuffer.remove(0, idx + 1);

        if (rawLine.isEmpty() || rawLine.startsWith(':')) continue;

        QString line = QString::fromUtf8(rawLine);
        if (line.startsWith(QStringLiteral("data: "))) {
            QString dataPayload = line.mid(6).trimmed();
            if (dataPayload == QStringLiteral("[DONE]")) continue;
            processSseLine(dataPayload);
        }
    }
}

void GenericRestDriver::processSseLine(const QString &jsonStr)
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    if (m_flavor == Protocol_Gemini) {
        parseGeminiChunk(obj);
    } else {
        parseOpenAiChunk(obj);
    }
}

void GenericRestDriver::parseOpenAiChunk(const QJsonObject &root)
{
    QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) return;

    QJsonObject delta = choices.first().toObject().value(QStringLiteral("delta")).toObject();
    QString chunk = delta.value(QStringLiteral("content")).toString();

    if (delta.contains(QStringLiteral("reasoning_content"))) {
        m_accumulatedThink += delta.value(QStringLiteral("reasoning_content")).toString();
        return;
    }

    if (!chunk.isEmpty()) {
        if (chunk.contains(QStringLiteral("<think>"))) {
            m_isInsideThinkTag = true;
            chunk.remove(QStringLiteral("<think>"));
        }
        if (m_isInsideThinkTag) {
            if (chunk.contains(QStringLiteral("</think>"))) {
                m_isInsideThinkTag = false;
                QStringList parts = chunk.split(QStringLiteral("</think>"));
                m_accumulatedThink += parts.first();
                chunk = parts.size() > 1 ? parts.at(1) : QString();
            } else {
                m_accumulatedThink += chunk;
                return;
            }
        }

        m_accumulatedText += chunk;
        emit tokenGenerated(chunk);
    }
}

void GenericRestDriver::parseGeminiChunk(const QJsonObject &root)
{
    QJsonArray candidates = root.value(QStringLiteral("candidates")).toArray();
    if (candidates.isEmpty()) return;

    QJsonObject content = candidates.first().toObject().value(QStringLiteral("content")).toObject();
    QJsonArray parts = content.value(QStringLiteral("parts")).toArray();

    for (const auto &p : parts) {
        QJsonObject partObj = p.toObject();
        if (partObj.value(QStringLiteral("thought")).toBool() == true) {
            m_accumulatedThink += partObj.value(QStringLiteral("text")).toString();
        } else {
            QString chunk = partObj.value(QStringLiteral("text")).toString();
            m_accumulatedText += chunk;
            emit tokenGenerated(chunk);
        }
    }
}

QVariantMap GenericRestDriver::parseImageResponse(const QByteArray &rawData) const
{
    QVariantList candidates;
    QJsonDocument doc = QJsonDocument::fromJson(rawData);
    if (doc.isObject()) {
        QJsonObject root = doc.object();
        if (m_flavor == Protocol_Gemini) {
            QJsonArray predictions = root.value(QStringLiteral("predictions")).toArray();
            for (const auto &p : predictions) {
                QString b64 = p.toObject().value(QStringLiteral("bytesBase64Encoded")).toString();
                if (!b64.isEmpty()) {
                    candidates.append(QStringLiteral("data:image/png;base64,") + b64);
                }
            }
        } else {
            QJsonArray dataArr = root.value(QStringLiteral("data")).toArray();
            for (const auto &item : dataArr) {
                QString b64 = item.toObject().value(QStringLiteral("b64_json")).toString();
                if (!b64.isEmpty()) {
                    candidates.append(QStringLiteral("data:image/png;base64,") + b64);
                }
            }
        }
    }

    QVariantMap result;
    result[QStringLiteral("candidates")] = candidates;
    result[QStringLiteral("count")]      = candidates.size();
    result[QStringLiteral("category")]   = QStringLiteral("image");
    return result;
}

QVariantMap GenericRestDriver::parseAudioResponse(const QByteArray &rawData, const QString &outputDir) const
{
    QByteArray audioBytes = rawData;

    // Gemini wraps audio in JSON inlineData; OpenAI returns raw binary bytes
    if (m_flavor == Protocol_Gemini) {
        QJsonDocument doc = QJsonDocument::fromJson(rawData);
        if (doc.isObject()) {
            QJsonArray candidates = doc.object().value(QStringLiteral("candidates")).toArray();
            if (!candidates.isEmpty()) {
                QJsonArray parts = candidates.first().toObject().value(QStringLiteral("content")).toObject().value(QStringLiteral("parts")).toArray();
                for (const auto &p : parts) {
                    QJsonObject inlineData = p.toObject().value(QStringLiteral("inlineData")).toObject();
                    if (inlineData.contains(QStringLiteral("data"))) {
                        audioBytes = QByteArray::fromBase64(inlineData.value(QStringLiteral("data")).toString().toUtf8());
                        break;
                    }
                }
            }
        }
    }

    QString targetDir = outputDir.isEmpty()
                            ? QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                            : outputDir;
    QDir().mkpath(targetDir);

    QString ext = m_currentOptions.value(QStringLiteral("response_format"), QStringLiteral("wav")).toString();
    QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
    QString filePath = QDir::cleanPath(targetDir + QStringLiteral("/remote_audio_%1.%2").arg(timestamp, ext));

    QFile out(filePath);
    if (out.open(QIODevice::WriteOnly)) {
        out.write(audioBytes);
        out.close();
    }

    QVariantMap result;
    result[QStringLiteral("filePath")] = filePath;
    result[QStringLiteral("wavBytes")] = audioBytes;
    result[QStringLiteral("category")] = QStringLiteral("audio");
    return result;
}

QString GenericRestDriver::extractVideoOperationId(const QByteArray &rawData) const
{
    QJsonDocument doc = QJsonDocument::fromJson(rawData);
    if (!doc.isObject()) return QString();

    QJsonObject root = doc.object();
    if (root.contains(QStringLiteral("name"))) {
        return root.value(QStringLiteral("name")).toString(); // Gemini long-running operation
    }
    if (root.contains(QStringLiteral("id"))) {
        return root.value(QStringLiteral("id")).toString();   // OpenAI-compatible async job ID
    }
    return QString();
}

void GenericRestDriver::onReplyFinished()
{
    if (!m_currentReply) return;

    const bool hasError = (m_currentReply->error() != QNetworkReply::NoError);
    QByteArray fullPayload = m_currentReply->readAll();
    m_currentReply->deleteLater();
    m_currentReply = nullptr;

    if (hasError) {
        m_isBusy = false;
        emit busyChanged(false);
        return;
    }

    if (m_targetAssetType == QStringLiteral("image")) {
        m_isBusy = false;
        emit busyChanged(false);
        emit generationFinished(parseImageResponse(fullPayload));
        return;
    }

    if (m_targetAssetType == QStringLiteral("audio")) {
        m_isBusy = false;
        emit busyChanged(false);
        emit generationFinished(parseAudioResponse(fullPayload, m_currentOutputDir));
        return;
    }

    if (m_targetAssetType == QStringLiteral("video")) {
        QString opId = extractVideoOperationId(fullPayload);
        if (!opId.isEmpty()) {
            m_pendingOperationId = opId;
            m_pollTimer->start();
            return; // Remain busy until polling completes
        }
        m_isBusy = false;
        emit busyChanged(false);
        QVariantMap result;
        result[QStringLiteral("outputDir")]  = m_currentOutputDir;
        result[QStringLiteral("frameCount")] = 0;
        result[QStringLiteral("fps")]        = 24;
        result[QStringLiteral("category")]   = QStringLiteral("video");
        emit generationFinished(result);
        return;
    }

           // Default: script / text
    if (m_accumulatedText.isEmpty()) {
        QByteArray fallback = !m_streamBuffer.isEmpty() ? m_streamBuffer : fullPayload;
        QJsonDocument doc = QJsonDocument::fromJson(fallback);
        if (doc.isObject()) {
            if (m_flavor == Protocol_Gemini) {
                parseGeminiChunk(doc.object());
            } else {
                QJsonArray choices = doc.object().value(QStringLiteral("choices")).toArray();
                if (!choices.isEmpty()) {
                    m_accumulatedText = choices.first().toObject().value(QStringLiteral("message")).toObject().value(QStringLiteral("content")).toString();
                }
            }
        }
    }

    m_isBusy = false;
    emit busyChanged(false);

    QVariantMap result;
    result[QStringLiteral("text")]      = m_accumulatedText.trimmed();
    result[QStringLiteral("think")]     = m_accumulatedThink.trimmed();
    result[QStringLiteral("timestamp")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    result[QStringLiteral("category")]  = QStringLiteral("script");

    emit generationFinished(result);
}

QString GenericRestDriver::extractVideoDownloadUrl(const QJsonObject &root) const
{
    if (m_flavor == Protocol_Gemini) {
        const QJsonObject resp = root.value(QStringLiteral("response")).toObject();

               // Veo predictLongRunning schema: response.generateVideoResponse.generatedSamples[0].video.uri
        const QJsonArray samples = resp.value(QStringLiteral("generateVideoResponse"))
                                       .toObject()
                                       .value(QStringLiteral("generatedSamples"))
                                       .toArray();
        if (!samples.isEmpty()) {
            const QString uri = samples.first().toObject()
            .value(QStringLiteral("video"))
                .toObject()
                .value(QStringLiteral("uri"))
                .toString();
            if (!uri.isEmpty()) return uri;
        }

               // Fallback Gemini schema: response.videos[0].uri
        const QJsonArray videos = resp.value(QStringLiteral("videos")).toArray();
        if (!videos.isEmpty()) {
            const QString uri = videos.first().toObject().value(QStringLiteral("uri")).toString();
            if (!uri.isEmpty()) return uri;
        }
        return QString();
    }

           // OpenAI-compatible schemas: root.url, root.video_url, or root.data[0].url
    if (root.contains(QStringLiteral("url"))) {
        return root.value(QStringLiteral("url")).toString();
    }
    if (root.contains(QStringLiteral("video_url"))) {
        return root.value(QStringLiteral("video_url")).toString();
    }
    const QJsonArray dataArr = root.value(QStringLiteral("data")).toArray();
    if (!dataArr.isEmpty()) {
        return dataArr.first().toObject().value(QStringLiteral("url")).toString();
    }
    return QString();
}

void GenericRestDriver::pollVideoOperation()
{
    if (m_pendingOperationId.isEmpty()) {
        m_pollTimer->stop();
        return;
    }

    QUrl pollUrl = (m_flavor == Protocol_Gemini)
                       ? QUrl(QStringLiteral("%1/v1beta/%2").arg(m_baseUrl, m_pendingOperationId))
                       : QUrl(QStringLiteral("%1/videos/generations/%2").arg(m_baseUrl, m_pendingOperationId));

    QNetworkRequest req(pollUrl);
    if (m_flavor == Protocol_Gemini) {
        req.setRawHeader("x-goog-api-key", m_apiKey.toUtf8());
    } else if (!m_apiKey.isEmpty()) {
        req.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_apiKey).toUtf8());
    }

    QNetworkReply *reply = m_netManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_pollTimer->stop();
            m_pendingOperationId.clear();
            m_isBusy = false;
            emit busyChanged(false);
            emit engineError(tr("Video polling failed: ") + reply->errorString());
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) return;
        const QJsonObject root = doc.object();

               // Check for remote failure states
        const QString status = root.value(QStringLiteral("status")).toString();
        if (root.contains(QStringLiteral("error")) || status == QStringLiteral("failed")) {
            m_pollTimer->stop();
            m_pendingOperationId.clear();
            m_isBusy = false;
            emit busyChanged(false);
            const QString errMsg = root.value(QStringLiteral("error")).toObject().value(QStringLiteral("message")).toString();
            emit engineError(tr("Remote video generation failed: ") + (errMsg.isEmpty() ? status : errMsg));
            return;
        }

        const bool done = root.value(QStringLiteral("done")).toBool() || (status == QStringLiteral("completed"));
        if (!done) return;

        m_pollTimer->stop();
        m_pendingOperationId.clear();

        const QString downloadUrl = extractVideoDownloadUrl(root);
        if (downloadUrl.isEmpty()) {
            m_isBusy = false;
            emit busyChanged(false);
            emit engineError(tr("Video completed, but no downloadable video URI was returned."));
            return;
        }

        downloadVideoAsset(QUrl(downloadUrl));
    });
}

void GenericRestDriver::downloadVideoAsset(const QUrl &videoUrl)
{
    QNetworkRequest req(videoUrl);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    if (m_flavor == Protocol_Gemini && !m_apiKey.isEmpty()) {
        req.setRawHeader("x-goog-api-key", m_apiKey.toUtf8());
    } else if (!m_apiKey.isEmpty()) {
        req.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_apiKey).toUtf8());
    }

           // Track in m_currentReply so cancelGeneration() can abort an in-flight download
    m_currentReply = m_netManager->get(req);
    connect(m_currentReply, &QNetworkReply::finished, this, [this]() {
        if (!m_currentReply) return;

        const bool hasError = (m_currentReply->error() != QNetworkReply::NoError);
        const QString errStr = m_currentReply->errorString();
        const QByteArray mp4Bytes = m_currentReply->readAll();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;

        m_isBusy = false;
        emit busyChanged(false);

        if (hasError || mp4Bytes.isEmpty()) {
            emit engineError(tr("Failed to download generated video: ") + (hasError ? errStr : tr("Empty response")));
            return;
        }

        const QString targetDir = m_currentOutputDir.isEmpty()
                                      ? QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                                      : m_currentOutputDir;
        QDir().mkpath(targetDir);

        const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
        const QString filePath = QDir::cleanPath(targetDir + QStringLiteral("/remote_video_%1.mp4").arg(timestamp));

        QFile out(filePath);
        if (!out.open(QIODevice::WriteOnly)) {
            emit engineError(tr("Could not write video file to disk: ") + filePath);
            return;
        }
        out.write(mp4Bytes);
        out.close();

        QVariantMap result;
        result[QStringLiteral("filePath")]   = filePath;
        result[QStringLiteral("outputDir")]  = targetDir;
        result[QStringLiteral("frameCount")] = m_currentOptions.value(QStringLiteral("duration_seconds"), 5).toInt() * 24;
        result[QStringLiteral("fps")]        = 24;
        result[QStringLiteral("category")]   = QStringLiteral("video");
        emit generationFinished(result);
    });
}

void GenericRestDriver::onNetworkError(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    if (!m_currentReply) return;
    QString errorStr = m_currentReply->errorString();
    QByteArray errPayload = m_currentReply->readAll();
    if (!errPayload.isEmpty()) {
        errorStr += QStringLiteral(" — Details: ") + QString::fromUtf8(errPayload);
    }

    qWarning() << "[GenericRestDriver] Request failed:" << errorStr;
    emit engineError(errorStr);
}
QString GenericRestDriver::parseVerificationMetadata(const QByteArray &rawData, const QString &orgHeader)
{
    m_verifiedModels.clear();
    QString summary;

    const QJsonDocument doc = QJsonDocument::fromJson(rawData);
    if (doc.isObject()) {
        const QJsonObject root = doc.object();

        if (m_flavor == Protocol_Gemini) {
            if (root.contains(QStringLiteral("models"))) {
                const QJsonArray models = root.value(QStringLiteral("models")).toArray();
                for (const auto &m : models) {
                    QString name = m.toObject().value(QStringLiteral("name")).toString();
                    name.remove(QStringLiteral("models/"));
                    if (!name.isEmpty()) m_verifiedModels.append(name);
                }
                summary = tr("Verified — %1 Gemini models available").arg(m_verifiedModels.size());
            } else if (root.contains(QStringLiteral("displayName"))) {
                const QString disp = root.value(QStringLiteral("displayName")).toString();
                const int inLimit = root.value(QStringLiteral("inputTokenLimit")).toInt();
                m_verifiedModels.append(m_modelName);
                summary = inLimit > 0
                              ? tr("Verified — %1 (%2k context)").arg(disp).arg(inLimit / 1000)
                              : tr("Verified — %1").arg(disp);
            }
        } else {
            const QJsonArray dataArr = root.value(QStringLiteral("data")).toArray();
            for (const auto &item : dataArr) {
                const QString id = item.toObject().value(QStringLiteral("id")).toString();
                if (!id.isEmpty()) m_verifiedModels.append(id);
            }
            summary = m_verifiedModels.isEmpty()
                          ? tr("Authentication Successful")
                          : tr("Verified — %1 models available").arg(m_verifiedModels.size());
            if (!orgHeader.trimmed().isEmpty()) {
                summary += tr(" (Org: %1)").arg(orgHeader.trimmed());
            }
        }
    }

    if (summary.isEmpty()) {
        summary = tr("Authentication Successful.");
    }

    m_connectionVerified = true;
    m_verificationSummary = summary;
    emit verificationInfoChanged();
    return summary;
}

void GenericRestDriver::verifyConnection()
{
    if (!isModelLoaded()) {
        m_connectionVerified = false;
        m_verificationSummary = tr("No API key detected in OS environment.");
        emit verificationInfoChanged();
        emit connectionVerified(false, m_verificationSummary);
        return;
    }

    QUrl url = (m_flavor == Protocol_Gemini)
                   ? QUrl(QStringLiteral("%1/v1beta/models").arg(m_baseUrl))
                   : QUrl(m_baseUrl + QStringLiteral("/models"));

    QNetworkRequest request(url);
    if (m_flavor == Protocol_Gemini) {
        request.setRawHeader("x-goog-api-key", m_apiKey.toUtf8());
    } else if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(m_apiKey).toUtf8());
    }

    QNetworkReply *reply = m_netManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            const QString orgHeader = QString::fromUtf8(reply->rawHeader("openai-organization"));
            const QString msg = parseVerificationMetadata(reply->readAll(), orgHeader);
            emit connectionVerified(true, msg);
        } else {
            m_connectionVerified = false;
            m_verificationSummary = tr("Auth Failed: ") + reply->errorString();
            emit verificationInfoChanged();
            emit connectionVerified(false, m_verificationSummary);
        }
    });
}
