#include "ApiKeyStore.h"
#include "MetaMusaSttClient.h"
#include "AbstractCloudSttClient.h"
#include "mock_network_access_manager.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

class TestMetaMusaSttClient : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName(u"QTranscribeTest"_s);
        QCoreApplication::setApplicationName(u"QTranscribeTest_MetaMusa"_s);
    }

    void cleanup() {
        QSettings settings;
        settings.remove(u"MetaMusa/Model"_s);
        settings.remove(u"MetaMusa/Keywords"_s);
        settings.remove(u"Cloud/Providers/MetaMusa/ApiKey"_s);
        settings.remove(u"MetaMusaTest/ApiKey"_s);
        settings.sync();
    }

    void testDefaultProperties() {
        MetaMusaSttClient client;
        QCOMPARE(client.selectedModel(), u"muse-voice-transcribe-1.0"_s);
        QCOMPARE(client.keywords(), QString());
        QVERIFY(client.handlesSmartFormatting());
        QCOMPARE(client.isReady(), false);
        QCOMPARE(client.isBusy(), false);
    }

    void testPropertyMutationsAndPersistence() {
        MetaMusaSttClient client;

        QSignalSpy spyModel(&client, &MetaMusaSttClient::selectedModelChanged);
        QSignalSpy spyKeywords(&client, &MetaMusaSttClient::keywordsChanged);

        client.setSelectedModel(u"muse-voice-transcribe-1.0"_s);
        client.setKeywords(u"Tattle, Wayland, PyTorch"_s);

        QCOMPARE(client.selectedModel(), u"muse-voice-transcribe-1.0"_s);
        QCOMPARE(client.keywords(), u"Tattle, Wayland, PyTorch"_s);

        QCOMPARE(spyKeywords.count(), 1);

        QSettings settings;
        QCOMPARE(settings.value(u"MetaMusa/Keywords"_s).toString(), u"Tattle, Wayland, PyTorch"_s);
    }

    void testApiKeyDelegationAndActivateLazyLoad() {
        QSettings settings;
        settings.setValue(u"MetaMusaTest/ApiKey"_s, u"LLM_test_key_12345"_s);
        settings.sync();

        MetaMusaSttClient client;
        auto* store = new ApiKeyStore(&client);
        store->setStorageKeys(u"QTranscribeTestService"_s, u"test_meta_musa_api_key"_s, u"MetaMusaTest/ApiKey"_s);
        client.setKeyStore(store);

        QCOMPARE(client.isApiKeySet(), false);
        QCOMPARE(client.isReady(), false);

        client.activate();

        QTRY_VERIFY(client.isReady());
        QTRY_COMPARE(client.apiKey(), u"LLM_test_key_12345"_s);
        QCOMPARE(client.isApiKeySet(), true);
    }

    void testSetAndClearApiKey() {
        MetaMusaSttClient client;
        auto* store = new ApiKeyStore(&client);
        store->setStorageKeys(u"QTranscribeTestService"_s, u"test_meta_musa_api_key"_s, u"MetaMusaTest/ApiKey"_s);
        client.setKeyStore(store);

        QSignalSpy spyKeyChanged(&client, &MetaMusaSttClient::apiKeyChanged);
        QSignalSpy spyKeySetChanged(&client, &AbstractCloudSttClient::apiKeySetChanged);

        client.setApiKey(u"  LLM_direct_assigned_key  "_s);

        QCOMPARE(client.apiKey(), u"LLM_direct_assigned_key"_s);
        QCOMPARE(client.isApiKeySet(), true);
        QCOMPARE(client.isReady(), true);
        QCOMPARE(spyKeyChanged.count(), 1);
        QCOMPARE(spyKeySetChanged.count(), 1);

        client.setApiKey(QString());
        QCOMPARE(client.apiKey(), QString());
        QCOMPARE(client.isApiKeySet(), false);
        QCOMPARE(client.isReady(), false);
        QCOMPARE(spyKeyChanged.count(), 2);
        QCOMPARE(spyKeySetChanged.count(), 2);
    }

    void testTranscribeEmptyAudioFailsImmediately() {
        MetaMusaSttClient client;
        client.setApiKey(u"valid_key"_s);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);
        client.transcribe(QByteArray());

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(), u"No audio data to transcribe"_s);
        QCOMPARE(client.lastError(), u"No audio data to transcribe"_s);
    }

    void testTranscribeWithoutApiKeyFails() {
        MetaMusaSttClient client;

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);
        client.transcribe(QByteArray("fakeAudioPayload"));

        QCOMPARE(errorSpy.count(), 1);
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::InvalidApiKey);
        QVERIFY(!client.isApiKeySet());
        QVERIFY(!client.isReady());
    }

    void testTranscribeSuccessTranscript() {
        MetaMusaSttClient client;
        client.setApiKey(u"LLM_valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = R"({"sessionId": "12345", "transcript": "Hello world from Meta Musa."})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy readySpy(&client, &AbstractSttClient::transcriptionReady);
        QSignalSpy busySpy(&client, &AbstractSttClient::busyChanged);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_COMPARE(readySpy.count(), 1);
        QCOMPARE(readySpy.first().at(0).toString(), u"Hello world from Meta Musa."_s);
        QVERIFY(!client.isBusy());
        QCOMPARE(mockNam->requestCount, 1);
    }

    void testTranscribeEmptyTranscriptReturnsError() {
        MetaMusaSttClient client;
        client.setApiKey(u"LLM_valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = R"({"sessionId": "12345", "transcript": ""})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_COMPARE(errorSpy.count(), 1);
        QCOMPARE(errorSpy.first().at(0).toString(), u"Meta Musa API returned empty transcription"_s);
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::GeneralError);
    }

    void testTranscribeBillingError402() {
        MetaMusaSttClient client;
        client.setApiKey(u"LLM_valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 402;
        mockNam->nextResponseData = R"({"error": {"message": "Billing account is not configured", "type": "billing_error", "code": "billing_not_configured"}})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_COMPARE(errorSpy.count(), 1);
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::BillingError);
    }

    void testTranscribeUnauthorized401() {
        MetaMusaSttClient client;
        client.setApiKey(u"LLM_invalid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 401;
        mockNam->nextResponseData = R"({"error": {"message": "Invalid API key provided", "type": "authentication_error", "code": "invalid_api_key"}})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy keyInvalidSpy(&client, &AbstractCloudSttClient::isApiKeyInvalidChanged);
        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_VERIFY(client.isApiKeyInvalid());
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::InvalidApiKey);
        QCOMPARE(errorSpy.count(), 1);
    }

    void testTranscribeRateLimit429WithRetryAfter() {
        MetaMusaSttClient client;
        client.setApiKey(u"LLM_valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 429;
        mockNam->nextHeaders.append({QByteArray("retry-after"), QByteArray("15")});
        mockNam->nextResponseData = R"({"error": {"message": "Rate limit exceeded", "type": "rate_limit_error", "code": "rate_limit_exceeded"}})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);
        QSignalSpy rateSpy(&client, &AbstractCloudSttClient::isRateLimitedChanged);

        client.transcribe(QByteArray("RIFFfakeWavData"));

        QTRY_VERIFY(client.isRateLimited());
        QCOMPARE(client.retrySecondsRemaining(), 15);
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::RateLimited);
        QVERIFY(errorSpy.count() >= 1);
    }

    void testCancelResetsState() {
        QSettings settings;
        settings.setValue(u"Cloud/Providers/MetaMusa/ApiKey"_s, u"LLM_valid_key"_s);
        settings.sync();

        MetaMusaSttClient client;
        client.setApiKey(u"LLM_valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = R"({"sessionId": "123", "transcript": "Ignored transcript"})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy readySpy(&client, &AbstractSttClient::transcriptionReady);
        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        client.transcribe(QByteArray("RIFFfakeWavData"));
        QVERIFY(client.isBusy());
        QCOMPARE(mockNam->requestCount, 1);

        client.cancel();
        QCOMPARE(client.isBusy(), false);
        QCOMPARE(client.isCancelled(), true);

        QTest::qWait(50);
        QCOMPARE(readySpy.count(), 0);
        QCOMPARE(errorSpy.count(), 0);
        QCOMPARE(mockNam->requestCount, 1);

        // Also test cancelling while a retry is pending stops the retry and ignores subsequent responses
        mockNam->nextStatusCode = 500;
        mockNam->nextResponseData = R"({"error": "Internal Server Error"})";
        client.transcribe(QByteArray("RIFFfakeWavData"));
        QVERIFY(client.isBusy());
        QCOMPARE(mockNam->requestCount, 2);

        // Allow initial 500 request to finish and schedule a retry
        QTest::qWait(20);
        QVERIFY(client.isBusy());

        client.cancel();
        QCOMPARE(client.isBusy(), false);
        QCOMPARE(client.isCancelled(), true);

        // Verify no retry request is dispatched afterward
        QTest::qWait(50);
        QCOMPARE(readySpy.count(), 0);
        QCOMPARE(errorSpy.count(), 0);
        QCOMPARE(mockNam->requestCount, 2);
    }

    void testKeywordsSentInMultipart() {
        QSettings settings;
        settings.setValue(u"Cloud/Providers/MetaMusa/ApiKey"_s, u"LLM_valid_key"_s);
        settings.sync();

        MetaMusaSttClient client;
        client.setApiKey(u"LLM_valid_key"_s);
        client.setKeywords(u"Tattle, Wayland, Voice"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = R"({"sessionId": "123", "transcript": "Testing keywords."})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy readySpy(&client, &AbstractSttClient::transcriptionReady);

        const QByteArray audioPayload("RIFFfakeWavData");
        client.transcribe(audioPayload);

        QTRY_COMPARE(readySpy.count(), 1);
        QCOMPARE(mockNam->requestCount, 1);

        QCOMPARE(mockNam->lastOperation, QNetworkAccessManager::PostOperation);
        QCOMPARE(mockNam->lastRequest.url(), QUrl(u"https://api.meta.ai/v1/asr/transcribe"_s));
        QCOMPARE(mockNam->lastRequest.rawHeader("authorization"), QByteArray("Bearer LLM_valid_key"));
        QCOMPARE(mockNam->lastRequest.rawHeader("accept"), QByteArray("application/json"));
        QVERIFY(mockNam->lastRequest.rawHeader("content-type").startsWith("multipart/form-data; boundary="));

        const QByteArray& body = mockNam->lastOutgoingData;
        const int firstLineEnd = body.indexOf("\r\n");
        QVERIFY(firstLineEnd > 0);
        const QByteArray boundary = body.left(firstLineEnd);
        QVERIFY(boundary.startsWith("--"));

        QList<QByteArray> sections;
        qsizetype pos = 0;
        while (pos < body.size()) {
            const qsizetype nextPos = body.indexOf(boundary, pos);
            if (nextPos == -1) {
                sections.append(body.mid(pos));
                break;
            }
            sections.append(body.mid(pos, nextPos - pos));
            pos = nextPos + boundary.size();
        }
        QVERIFY(sections.size() >= 3);

        auto parsePart = [](const QByteArray& rawPart) {
            QByteArray content = rawPart;
            if (content.startsWith("\r\n")) {
                content = content.mid(2);
            }
            const int sep = content.indexOf("\r\n\r\n");
            if (sep == -1) {
                return qMakePair(QByteArray(), QByteArray());
            }
            QByteArray headers = content.left(sep);
            QByteArray partBody = content.mid(sep + 4);
            if (partBody.endsWith("\r\n")) {
                partBody.chop(2);
            }
            return qMakePair(headers, partBody);
        };

        const auto [reqHeaders, reqBody] = parsePart(sections.at(1));
        QVERIFY(!reqHeaders.isEmpty());
        QVERIFY(reqHeaders.contains("name=\"request\""));
        QVERIFY(reqHeaders.toLower().contains("content-type: application/json"));

        const auto jsonDoc = QJsonDocument::fromJson(reqBody);
        QVERIFY(!jsonDoc.isNull());
        QVERIFY(jsonDoc.isObject());
        const QJsonObject jsonObj = jsonDoc.object();
        QCOMPARE(jsonObj.value(u"model"_s).toString(), u"muse-voice-transcribe-1.0"_s);
        QCOMPARE(jsonObj.value(u"audioEncoding"_s).toString(), u"WAV"_s);
        QCOMPARE(jsonObj.value(u"mode"_s).toString(), u"PUSH_TO_TALK"_s);

        const QJsonArray keywordsArr = jsonObj.value(u"keywords"_s).toArray();
        QCOMPARE(keywordsArr.size(), 3);
        QCOMPARE(keywordsArr.at(0).toString(), u"Tattle"_s);
        QCOMPARE(keywordsArr.at(1).toString(), u"Wayland"_s);
        QCOMPARE(keywordsArr.at(2).toString(), u"Voice"_s);

        const auto [audioHeaders, audioBody] = parsePart(sections.at(2));
        QVERIFY(!audioHeaders.isEmpty());
        QVERIFY(audioHeaders.contains("name=\"audio\""));
        QVERIFY(audioHeaders.contains("filename=\"audio.wav\""));
        QVERIFY(audioHeaders.toLower().contains("content-type: audio/wav"));
        QCOMPARE(audioBody, audioPayload);
    }
};

QTEST_GUILESS_MAIN(TestMetaMusaSttClient)
#include "test_meta_musa_stt_client.moc"
