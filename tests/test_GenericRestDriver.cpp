// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>
#include <QFile>
#include "app/engine/remote/genericrestdriver.h"

class Test_GenericRestDriver : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Driver Initialization & Identification
    void testOpenAiDefaults();
    void testGeminiDefaults();
    void testCustomDisplayName();

    // Configuration & Auth State
    void testLocalhostDoesNotRequireApiKey();
    void testRemoteRequiresApiKey();
    void testOptionMutation();
    void testParseVerificationMetadataGeminiAndOpenAi();

    // Script Payload & Request Builders
    void testOpenAiPayloadStructure();
    void testGeminiPayloadStructure();
    void testOpenAiUrlAndHeaders();
    void testGeminiUrlAndHeaders();

    // SSE Chunk Parsers & Think Extraction
    void testOpenAiSseTokenParsing();
    void testOpenAiReasoningContentParsing();
    void testOpenAiInlineThinkTagParsing();
    void testGeminiThoughtParsing();

    // Multimodal: Image, Audio, Video
    void testImageModalityRoutingAndParsing();
    void testAudioModalityRoutingAndFileSave();
    void testVideoModalityRoutingAndOperationExtraction();

private:
    QString m_testSessionDir;
};

void Test_GenericRestDriver::initTestCase()
{
    QCoreApplication::setApplicationName(QStringLiteral("StoryBuilderStudio"));
    QCoreApplication::setOrganizationName(QStringLiteral("5150wtmw"));

    QString baseTemp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString sessionUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_testSessionDir = QDir::cleanPath(baseTemp + "/" + sessionUuid);
    QDir().mkpath(m_testSessionDir);
}

void Test_GenericRestDriver::cleanupTestCase()
{
    if (!m_testSessionDir.isEmpty() && QDir(m_testSessionDir).exists()) {
        QDir(m_testSessionDir).removeRecursively();
    }
}

void Test_GenericRestDriver::testOpenAiDefaults()
{
    GenericRestDriver driver(QStringLiteral("openai_test"), GenericRestDriver::Protocol_OpenAI);
    QCOMPARE(driver.driverId(), QStringLiteral("openai_test"));
    QCOMPARE(driver.targetAssetType(), QStringLiteral("script"));
    QCOMPARE(driver.flavor(), GenericRestDriver::Protocol_OpenAI);
    QCOMPARE(driver.baseUrl(), QStringLiteral("https://api.openai.com/v1"));
    QCOMPARE(driver.loadedModelName(), QStringLiteral("gpt-4o"));
    QVERIFY2(!driver.isModelLoaded(), "Remote OpenAI driver should not report loaded without an API key.");
}

void Test_GenericRestDriver::testGeminiDefaults()
{
    GenericRestDriver driver(QStringLiteral("gemini_test"), GenericRestDriver::Protocol_Gemini);
    QCOMPARE(driver.driverId(), QStringLiteral("gemini_test"));
    QCOMPARE(driver.flavor(), GenericRestDriver::Protocol_Gemini);
    QCOMPARE(driver.baseUrl(), QStringLiteral("https://generativelanguage.googleapis.com"));
    QCOMPARE(driver.loadedModelName(), QStringLiteral("gemini-3.5-flash-lite"));
    QVERIFY2(!driver.isModelLoaded(), "Remote Gemini driver should not report loaded without an API key.");
}

void Test_GenericRestDriver::testLocalhostDoesNotRequireApiKey()
{
    GenericRestDriver driver(QStringLiteral("ollama_local"), GenericRestDriver::Protocol_OpenAI);
    QVariantMap config;
    config[QStringLiteral("baseUrl")] = QStringLiteral("http://localhost:11434/v1");
    config[QStringLiteral("modelName")] = QStringLiteral("llama3.2");

    QVERIFY2(driver.loadModel(config), "Localhost endpoint failed to load without an API key.");
    QVERIFY2(driver.isModelLoaded(), "Localhost driver did not transition to loaded state.");
}

void Test_GenericRestDriver::testRemoteRequiresApiKey()
{
    GenericRestDriver driver(QStringLiteral("gemini_remote"), GenericRestDriver::Protocol_Gemini);
    QVariantMap config;
    config[QStringLiteral("modelName")] = QStringLiteral("gemini-3.5-flash-lite");

    QVERIFY2(!driver.loadModel(config), "Security Failure: Remote driver loaded without an API key.");

    config[QStringLiteral("apiKey")] = QStringLiteral("AIzaSyFakeKeyTest123");
    QVERIFY2(driver.loadModel(config), "Remote driver failed to load with a valid API key.");
    QVERIFY2(driver.isModelLoaded(), "Remote driver state did not update to loaded.");

    driver.unloadModel();
    QVERIFY2(!driver.isModelLoaded(), "unloadModel() did not clear loaded state.");
}

void Test_GenericRestDriver::testOptionMutation()
{
    GenericRestDriver driver(QStringLiteral("openai_opt"), GenericRestDriver::Protocol_OpenAI);
    QSignalSpy spy(&driver, &GenericRestDriver::optionsChanged);

    QVERIFY2(driver.setOption(QStringLiteral("temperature"), 1.1f), "setOption() returned false.");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(driver.currentOptions().value(QStringLiteral("temperature")).toFloat(), 1.1f);
}

void Test_GenericRestDriver::testOpenAiPayloadStructure()
{
    GenericRestDriver driver(QStringLiteral("openai_payload"), GenericRestDriver::Protocol_OpenAI);
    QVariantMap opts;
    opts[QStringLiteral("temperature")] = 0.5;
    opts[QStringLiteral("top_p")] = 0.8;

    QByteArray json = driver.buildOpenAiPayload(
        QStringLiteral("Hello world"),
        QStringLiteral("You are a helpful assistant"),
        QStringList(),
        opts,
        true);

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QVERIFY2(doc.isObject(), "OpenAI payload is not a valid JSON object.");
    QJsonObject root = doc.object();

    QCOMPARE(root.value(QStringLiteral("model")).toString(), QStringLiteral("gpt-4o"));
    QCOMPARE(root.value(QStringLiteral("stream")).toBool(), true);
    QCOMPARE(root.value(QStringLiteral("temperature")).toDouble(), 0.5);

    QJsonArray messages = root.value(QStringLiteral("messages")).toArray();
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages[0].toObject().value(QStringLiteral("role")).toString(), QStringLiteral("system"));
    QCOMPARE(messages[1].toObject().value(QStringLiteral("content")).toString(), QStringLiteral("Hello world"));
}

void Test_GenericRestDriver::testGeminiPayloadStructure()
{
    GenericRestDriver driver(QStringLiteral("gemini_payload"), GenericRestDriver::Protocol_Gemini);
    QVariantMap opts;
    opts[QStringLiteral("temperature")] = 0.2;

    QByteArray json = driver.buildGeminiPayload(
        QStringLiteral("Write a poem"),
        QStringLiteral("Be concise"),
        QStringList(),
        opts);

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QVERIFY2(doc.isObject(), "Gemini payload is not a valid JSON object.");
    QJsonObject root = doc.object();

    QVERIFY2(root.contains(QStringLiteral("systemInstruction")), "Gemini payload missing systemInstruction.");
    QVERIFY2(root.contains(QStringLiteral("contents")), "Gemini payload missing contents.");
    QCOMPARE(root.value(QStringLiteral("generationConfig")).toObject().value(QStringLiteral("temperature")).toDouble(), 0.2);
}

void Test_GenericRestDriver::testOpenAiUrlAndHeaders()
{
    GenericRestDriver driver(QStringLiteral("openai_req"), GenericRestDriver::Protocol_OpenAI);
    QVariantMap config;
    config[QStringLiteral("baseUrl")] = QStringLiteral("https://api.openai.com/v1");
    config[QStringLiteral("apiKey")] = QStringLiteral("sk-mockkey");
    driver.loadModel(config);

    QNetworkRequest req = driver.buildRequest(true);
    QCOMPARE(req.url().toString(), QStringLiteral("https://api.openai.com/v1/chat/completions"));
    QCOMPARE(req.rawHeader("Authorization"), QByteArray("Bearer sk-mockkey"));
}

void Test_GenericRestDriver::testGeminiUrlAndHeaders()
{
    GenericRestDriver driver(QStringLiteral("gemini_req"), GenericRestDriver::Protocol_Gemini);
    QVariantMap config;
    config[QStringLiteral("apiKey")] = QStringLiteral("gemini-mockkey");
    driver.loadModel(config);

    QNetworkRequest req = driver.buildRequest(true);
    QVERIFY2(req.url().toString().contains(QStringLiteral("streamGenerateContent?alt=sse")),
             "Gemini streaming URL missing streamGenerateContent?alt=sse.");
    QCOMPARE(req.rawHeader("x-goog-api-key"), QByteArray("gemini-mockkey"));
}

void Test_GenericRestDriver::testOpenAiSseTokenParsing()
{
    GenericRestDriver driver(QStringLiteral("openai_sse"), GenericRestDriver::Protocol_OpenAI);
    QSignalSpy tokenSpy(&driver, &GenericRestDriver::tokenGenerated);

    driver.processSseLine(QStringLiteral(R"({"choices":[{"delta":{"content":"Once upon a time"}}]})"));
    QCOMPARE(tokenSpy.count(), 1);
    QCOMPARE(tokenSpy.takeFirst().at(0).toString(), QStringLiteral("Once upon a time"));
}

void Test_GenericRestDriver::testOpenAiReasoningContentParsing()
{
    GenericRestDriver driver(QStringLiteral("openai_think"), GenericRestDriver::Protocol_OpenAI);
    QSignalSpy tokenSpy(&driver, &GenericRestDriver::tokenGenerated);

    driver.processSseLine(QStringLiteral(R"({"choices":[{"delta":{"reasoning_content":"Step 1"}}]})"));
    QCOMPARE(tokenSpy.count(), 0);

    driver.processSseLine(QStringLiteral(R"({"choices":[{"delta":{"content":"Answer"}}]})"));
    QCOMPARE(tokenSpy.count(), 1);
}

void Test_GenericRestDriver::testOpenAiInlineThinkTagParsing()
{
    GenericRestDriver driver(QStringLiteral("openai_inline"), GenericRestDriver::Protocol_OpenAI);
    QSignalSpy tokenSpy(&driver, &GenericRestDriver::tokenGenerated);

    driver.processSseLine(QStringLiteral(R"({"choices":[{"delta":{"content":"<think>Internal"}}]})"));
    driver.processSseLine(QStringLiteral(R"({"choices":[{"delta":{"content":" reasoning</think>Final answer"}}]})"));

    QCOMPARE(tokenSpy.count(), 1);
    QCOMPARE(tokenSpy.takeFirst().at(0).toString(), QStringLiteral("Final answer"));
}

void Test_GenericRestDriver::testGeminiThoughtParsing()
{
    GenericRestDriver driver(QStringLiteral("gemini_chunk"), GenericRestDriver::Protocol_Gemini);
    QSignalSpy tokenSpy(&driver, &GenericRestDriver::tokenGenerated);

    driver.processSseLine(QStringLiteral(R"({"candidates":[{"content":{"parts":[{"text":"Thinking...","thought":true}]}}]})"));
    QCOMPARE(tokenSpy.count(), 0);

    driver.processSseLine(QStringLiteral(R"({"candidates":[{"content":{"parts":[{"text":"Hello human"}]}}]})"));
    QCOMPARE(tokenSpy.count(), 1);
    QCOMPARE(tokenSpy.takeFirst().at(0).toString(), QStringLiteral("Hello human"));
}

void Test_GenericRestDriver::testImageModalityRoutingAndParsing()
{
    // 1. OpenAI Image
    GenericRestDriver openAiImg(QStringLiteral("openai_image"), GenericRestDriver::Protocol_OpenAI, QStringLiteral("image"));
    QVariantMap cfg;
    cfg[QStringLiteral("apiKey")] = QStringLiteral("sk-img");
    QVERIFY2(openAiImg.loadModel(cfg), "OpenAI image driver failed to load.");
    QCOMPARE(openAiImg.buildRequest(false).url().toString(), QStringLiteral("https://api.openai.com/v1/images/generations"));

    QByteArray openAiResp = R"({"data":[{"b64_json":"AAAA"},{"b64_json":"BBBB"}]})";
    QVariantMap parsedOpenAi = openAiImg.parseImageResponse(openAiResp);
    QCOMPARE(parsedOpenAi.value(QStringLiteral("count")).toInt(), 2);
    QVERIFY2(parsedOpenAi.value(QStringLiteral("candidates")).toList().first().toString().startsWith(QStringLiteral("data:image/png;base64,")),
             "OpenAI image candidate missing data URI prefix.");

           // 2. Gemini / Imagen
    GenericRestDriver geminiImg(QStringLiteral("gemini_image"), GenericRestDriver::Protocol_Gemini, QStringLiteral("image"));
    QVERIFY2(geminiImg.loadModel(cfg), "Gemini image driver failed to load.");
    QVERIFY2(geminiImg.buildRequest(false).url().toString().endsWith(QStringLiteral(":predict")),
             "Gemini image URL should target :predict endpoint.");

    QByteArray geminiResp = R"({"predictions":[{"bytesBase64Encoded":"CCCC"}]})";
    QVariantMap parsedGemini = geminiImg.parseImageResponse(geminiResp);
    QCOMPARE(parsedGemini.value(QStringLiteral("count")).toInt(), 1);
    QCOMPARE(parsedGemini.value(QStringLiteral("candidates")).toList().first().toString(),
             QStringLiteral("data:image/png;base64,CCCC"));
}

void Test_GenericRestDriver::testAudioModalityRoutingAndFileSave()
{
    GenericRestDriver openAiAudio(QStringLiteral("openai_audio"), GenericRestDriver::Protocol_OpenAI, QStringLiteral("audio"));
    QVariantMap cfg;
    cfg[QStringLiteral("apiKey")] = QStringLiteral("sk-audio");
    QVERIFY2(openAiAudio.loadModel(cfg), "OpenAI audio driver failed to load.");
    QCOMPARE(openAiAudio.buildRequest(false).url().toString(), QStringLiteral("https://api.openai.com/v1/audio/speech"));

    QByteArray mockWav("RIFF\x24\x00\x00\x00WAVEfmt ");
    QVariantMap parsed = openAiAudio.parseAudioResponse(mockWav, m_testSessionDir);

    QString savedPath = parsed.value(QStringLiteral("filePath")).toString();
    QVERIFY2(!savedPath.isEmpty() && QFile::exists(savedPath), "Audio response was not written to output directory.");
    QCOMPARE(parsed.value(QStringLiteral("category")).toString(), QStringLiteral("audio"));
}

void Test_GenericRestDriver::testVideoModalityRoutingAndOperationExtraction()
{
    GenericRestDriver geminiVid(QStringLiteral("gemini_video"), GenericRestDriver::Protocol_Gemini, QStringLiteral("video"));
    QVariantMap cfg;
    cfg[QStringLiteral("apiKey")] = QStringLiteral("gemini-vid-key");
    QVERIFY2(geminiVid.loadModel(cfg), "Gemini video driver failed to load.");
    QVERIFY2(geminiVid.buildRequest(false).url().toString().endsWith(QStringLiteral(":predictLongRunning")),
             "Gemini video URL should target :predictLongRunning endpoint.");

    QByteArray opResp = R"({"name":"models/veo-2.0-generate-001/operations/op-12345"})";
    QString opId = geminiVid.extractVideoOperationId(opResp);
    QCOMPARE(opId, QStringLiteral("models/veo-2.0-generate-001/operations/op-12345"));

    // Verify Gemini Veo download URI extraction
    const QJsonObject geminiDone = QJsonDocument::fromJson(
           R"({"done":true,"response":{"generateVideoResponse":{"generatedSamples":[{"video":{"uri":"https://generativelanguage.googleapis.com/v1beta/files/vid123:download?alt=media"}}]}}})"
           ).object();
    QCOMPARE(geminiVid.extractVideoDownloadUrl(geminiDone),
             QStringLiteral("https://generativelanguage.googleapis.com/v1beta/files/vid123:download?alt=media"));

           // Verify OpenAI-compatible download URL extraction
    GenericRestDriver openAiVid(QStringLiteral("custom_video"), GenericRestDriver::Protocol_OpenAI, QStringLiteral("video"));
    const QJsonObject openAiDone = QJsonDocument::fromJson(
           R"({"status":"completed","data":[{"url":"http://127.0.0.1:8080/outputs/clip_01.mp4"}]})"
           ).object();
    QCOMPARE(openAiVid.extractVideoDownloadUrl(openAiDone),
             QStringLiteral("http://127.0.0.1:8080/outputs/clip_01.mp4"));
}

void Test_GenericRestDriver::testCustomDisplayName()
{
    GenericRestDriver driver(QStringLiteral("custom_image_local_sd"), GenericRestDriver::Protocol_OpenAI, QStringLiteral("image"));
    QCOMPARE(driver.displayName(), QStringLiteral("custom_image_local_sd"));

    QVariantMap cfg;
    cfg[QStringLiteral("displayName")] = QStringLiteral("Local SD Server");
    cfg[QStringLiteral("baseUrl")]     = QStringLiteral("http://127.0.0.1:7860/v1");
    cfg[QStringLiteral("modelName")]   = QStringLiteral("flux1-schnell");

    QVERIFY2(driver.loadModel(cfg), "Failed to load custom local REST driver.");
    QCOMPARE(driver.displayName(), QStringLiteral("Local SD Server"));
}

void Test_GenericRestDriver::testParseVerificationMetadataGeminiAndOpenAi()
{
    // 1. Gemini model list
    GenericRestDriver gemini(QStringLiteral("gemini_verify"), GenericRestDriver::Protocol_Gemini);
    QSignalSpy geminiSpy(&gemini, &GenericRestDriver::verificationInfoChanged);

    const QByteArray geminiResp = R"({"models":[{"name":"models/gemini-2.5-flash"},{"name":"models/imagen-3.0-generate-002"}]})";
    const QString geminiSummary = gemini.parseVerificationMetadata(geminiResp);

    QCOMPARE(geminiSpy.count(), 1);
    QCOMPARE(gemini.isConnectionVerified(), true);
    QCOMPARE(gemini.verifiedModels().size(), 2);
    QVERIFY(geminiSummary.contains(QStringLiteral("2 Gemini models available")));

           // 2. OpenAI model list + org header
    GenericRestDriver openai(QStringLiteral("openai_verify"), GenericRestDriver::Protocol_OpenAI);
    const QByteArray openaiResp = R"({"data":[{"id":"gpt-4o"},{"id":"tts-1"},{"id":"gpt-image-1"}]})";
    const QString openaiSummary = openai.parseVerificationMetadata(openaiResp, QStringLiteral("org-5150wtmw"));

    QCOMPARE(openai.isConnectionVerified(), true);
    QCOMPARE(openai.verifiedModels().size(), 3);
    QVERIFY(openaiSummary.contains(QStringLiteral("3 models available")));
    QVERIFY(openaiSummary.contains(QStringLiteral("Org: org-5150wtmw")));
}

QTEST_MAIN(Test_GenericRestDriver)
#include "test_GenericRestDriver.moc"
