#include "ApiKeyStore.h"
#include "SarvamSttClient.h"
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

class TestSarvamSttClient : public QObject {
    Q_OBJECT

private:
    static QByteArray createFakeWav(double durationSeconds, quint32 sampleRate = 16000) {
        const quint32 byteRate = sampleRate * 2;
        const auto pcmBytes = static_cast<quint32>(durationSeconds * byteRate);
        const quint32 fileSize = 36 + pcmBytes;

        QByteArray wav(44 + static_cast<qsizetype>(pcmBytes), '\0');
        auto* data = reinterpret_cast<quint8*>(wav.data());

        // "RIFF"
        data[0] = 'R';
        data[1] = 'I';
        data[2] = 'F';
        data[3] = 'F';

        // File size - 8
        data[4] = static_cast<quint8>(fileSize & 0xFF);
        data[5] = static_cast<quint8>((fileSize >> 8) & 0xFF);
        data[6] = static_cast<quint8>((fileSize >> 16) & 0xFF);
        data[7] = static_cast<quint8>((fileSize >> 24) & 0xFF);

        // "WAVE"
        data[8] = 'W';
        data[9] = 'A';
        data[10] = 'V';
        data[11] = 'E';

        // "fmt "
        data[12] = 'f';
        data[13] = 'm';
        data[14] = 't';
        data[15] = ' ';

        // Subchunk1Size = 16
        data[16] = 16;
        data[17] = 0;
        data[18] = 0;
        data[19] = 0;

        // AudioFormat = 1 (PCM)
        data[20] = 1;
        data[21] = 0;

        // NumChannels = 1
        data[22] = 1;
        data[23] = 0;

        // SampleRate
        data[24] = static_cast<quint8>(sampleRate & 0xFF);
        data[25] = static_cast<quint8>((sampleRate >> 8) & 0xFF);
        data[26] = static_cast<quint8>((sampleRate >> 16) & 0xFF);
        data[27] = static_cast<quint8>((sampleRate >> 24) & 0xFF);

        // ByteRate = SampleRate * 2 (16-bit mono)
        data[28] = static_cast<quint8>(byteRate & 0xFF);
        data[29] = static_cast<quint8>((byteRate >> 8) & 0xFF);
        data[30] = static_cast<quint8>((byteRate >> 16) & 0xFF);
        data[31] = static_cast<quint8>((byteRate >> 24) & 0xFF);

        // BlockAlign = 2
        data[32] = 2;
        data[33] = 0;

        // BitsPerSample = 16
        data[34] = 16;
        data[35] = 0;

        // "data"
        data[36] = 'd';
        data[37] = 'a';
        data[38] = 't';
        data[39] = 'a';

        // Data size
        data[40] = static_cast<quint8>(pcmBytes & 0xFF);
        data[41] = static_cast<quint8>((pcmBytes >> 8) & 0xFF);
        data[42] = static_cast<quint8>((pcmBytes >> 16) & 0xFF);
        data[43] = static_cast<quint8>((pcmBytes >> 24) & 0xFF);

        return wav;
    }

private slots:
    void initTestCase() {
        QCoreApplication::setOrganizationName(u"QTranscribeTest"_s);
        QCoreApplication::setApplicationName(u"QTranscribeTest_Sarvam"_s);
    }

    void cleanup() {
        QSettings settings;
        settings.remove(u"Sarvam/Model"_s);
        settings.remove(u"Sarvam/Language"_s);
        settings.remove(u"Sarvam/Mode"_s);
        settings.remove(u"Sarvam/StylePreset"_s);
        settings.remove(u"Sarvam/StyleRules"_s);
        settings.remove(u"Sarvam/CustomVocabulary"_s);
        settings.remove(u"Cloud/Providers/Sarvam/ApiKey"_s);
        settings.remove(u"SarvamTest/ApiKey"_s);
        settings.sync();
    }

    void testDefaultProperties() {
        SarvamSttClient client;
        QCOMPARE(client.selectedModel(), QString());
        QCOMPARE(client.language(), u"auto"_s);
        QCOMPARE(client.mode(), u"transcribe"_s);
        QCOMPARE(client.stylePreset(), u"clean"_s);
        QCOMPARE(client.styleRules(), QString());
        QCOMPARE(client.customVocabulary(), QString());
        QVERIFY(client.handlesSmartFormatting());
        QVERIFY(client.maxAudioDuration().has_value());
        QCOMPARE(client.maxAudioDuration().value(), std::chrono::seconds(30));
        QCOMPARE(client.isReady(), false);
        QCOMPARE(client.isBusy(), false);
    }

    void testPropertyMutationsAndPersistence() {
        SarvamSttClient client;

        QSignalSpy spyModel(&client, &SarvamSttClient::selectedModelChanged);
        QSignalSpy spyLang(&client, &SarvamSttClient::languageChanged);
        QSignalSpy spyMode(&client, &SarvamSttClient::modeChanged);
        QSignalSpy spyPreset(&client, &SarvamSttClient::stylePresetChanged);
        QSignalSpy spyRules(&client, &SarvamSttClient::styleRulesChanged);
        QSignalSpy spyVocab(&client, &SarvamSttClient::customVocabularyChanged);

        client.setSelectedModel(u"dictate-v1-2026-11-04"_s);
        client.setLanguage(u"hi-IN"_s);
        client.setMode(u"codemix"_s);
        client.setStylePreset(u"light"_s);
        client.setStyleRules(u"Format as bullet points."_s);
        client.setCustomVocabulary(u"Tattle, Aadhaar, UPI"_s);

        QCOMPARE(client.selectedModel(), u"dictate-v1-2026-11-04"_s);
        QCOMPARE(client.language(), u"hi-IN"_s);
        QCOMPARE(client.mode(), u"codemix"_s);
        QCOMPARE(client.stylePreset(), u"light"_s);
        QCOMPARE(client.styleRules(), u"Format as bullet points."_s);
        QCOMPARE(client.customVocabulary(), u"Tattle, Aadhaar, UPI"_s);

        QCOMPARE(spyModel.count(), 1);
        QCOMPARE(spyLang.count(), 1);
        QCOMPARE(spyMode.count(), 1);
        QCOMPARE(spyPreset.count(), 1);
        QCOMPARE(spyRules.count(), 1);
        QCOMPARE(spyVocab.count(), 1);

        QSettings settings;
        QCOMPARE(settings.value(u"Sarvam/Language"_s).toString(), u"hi-IN"_s);
        QCOMPARE(settings.value(u"Sarvam/Mode"_s).toString(), u"codemix"_s);
        QCOMPARE(settings.value(u"Sarvam/StylePreset"_s).toString(), u"light"_s);
        QCOMPARE(settings.value(u"Sarvam/StyleRules"_s).toString(), u"Format as bullet points."_s);
        QCOMPARE(settings.value(u"Sarvam/CustomVocabulary"_s).toString(), u"Tattle, Aadhaar, UPI"_s);
    }

    void testHandlesSmartFormattingVariants() {
        SarvamSttClient client;
        client.setMode(u"transcribe"_s);
        client.setStylePreset(u"clean"_s);
        QVERIFY(client.handlesSmartFormatting());

        client.setMode(u"verbatim"_s);
        QCOMPARE(client.handlesSmartFormatting(), false);

        client.setMode(u"transcribe"_s);
        client.setStylePreset(u"none"_s);
        QCOMPARE(client.handlesSmartFormatting(), false);

        client.setMode(u"codemix"_s);
        client.setStylePreset(u"light"_s);
        QVERIFY(client.handlesSmartFormatting());
    }

    void testApiKeyDelegation() {
        QSettings settings;
        settings.setValue(u"SarvamTest/ApiKey"_s, u"sarvam_test_key_12345"_s);
        settings.sync();

        SarvamSttClient client;
        auto* store = new ApiKeyStore(&client);
        store->setStorageKeys(u"QTranscribeTestService"_s, u"test_sarvam_api_key"_s, u"SarvamTest/ApiKey"_s);
        client.setKeyStore(store);

        QCOMPARE(client.isApiKeySet(), false);
        QCOMPARE(client.isReady(), false);

        client.activate();

        QTRY_VERIFY(client.isReady());
        QTRY_COMPARE(client.apiKey(), u"sarvam_test_key_12345"_s);
        QCOMPARE(client.isApiKeySet(), true);
    }

    void testAudioExceeding30SecondsRejected() {
        SarvamSttClient client;
        client.setApiKey(u"valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        client.setNetworkAccessManager(mockNam);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        // 35-second audio
        const QByteArray longWav = createFakeWav(35.0);
        client.transcribe(longWav);

        QCOMPARE(errorSpy.count(), 1);
        QVERIFY(client.lastError().contains(u"30-second limit"_s));
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::GeneralError);
        // Verify no HTTP request was dispatched
        QCOMPARE(mockNam->requestCount, 0);
    }

    void testTranscribeSuccessTranscriptAndText() {
        SarvamSttClient client;
        client.setApiKey(u"valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData =
            R"({"request_id": "req-1", "transcript": "um hello world", "text": "Hello world.", "degraded": null})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy readySpy(&client, &AbstractSttClient::transcriptionReady);

        const QByteArray shortWav = createFakeWav(5.0);
        client.transcribe(shortWav);

        QTRY_COMPARE(readySpy.count(), 1);
        QCOMPARE(readySpy.first().at(0).toString(), u"Hello world."_s);
        QVERIFY(!client.isBusy());
        QCOMPARE(mockNam->requestCount, 1);
    }

    void testTranscribeFallbackToTranscriptWhenTextEmpty() {
        SarvamSttClient client;
        client.setApiKey(u"valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData =
            R"({"request_id": "req-2", "transcript": "Raw speech without cleanup", "text": "", "degraded": "llm_error"})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy readySpy(&client, &AbstractSttClient::transcriptionReady);

        const QByteArray shortWav = createFakeWav(3.0);
        client.transcribe(shortWav);

        QTRY_COMPARE(readySpy.count(), 1);
        QCOMPARE(readySpy.first().at(0).toString(), u"Raw speech without cleanup"_s);
    }

    void testMultipartPayloadAndHeaders() {
        QSettings settings;
        settings.setValue(u"Cloud/Providers/Sarvam/ApiKey"_s, u"test_sub_key_999"_s);
        settings.sync();

        SarvamSttClient client;
        client.setApiKey(u"test_sub_key_999"_s);
        client.setLanguage(u"hi-IN"_s);
        client.setMode(u"codemix"_s);
        client.setStylePreset(u"clean"_s);
        client.setStyleRules(u"Format as bullet."_s);
        client.setCustomVocabulary(u"Zentrix, Velqora"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = R"({"request_id": "req-3", "text": "Done"})";
        client.setNetworkAccessManager(mockNam);

        const QByteArray shortWav = createFakeWav(2.0);
        client.transcribe(shortWav);

        QTRY_COMPARE(mockNam->requestCount, 1);
        QCOMPARE(mockNam->lastRequest.rawHeader("api-subscription-key"), QByteArray("test_sub_key_999"));
        QCOMPARE(mockNam->lastRequest.url().toString(), u"https://api.sarvam.ai/v2/dictate"_s);

        // Inspect multipart body
        const QByteArray body = mockNam->lastOutgoingData;
        QVERIFY(body.contains("name=\"language_code\""));
        QVERIFY(body.contains("hi-IN"));
        QVERIFY(body.contains("name=\"mode\""));
        QVERIFY(body.contains("codemix"));
        QVERIFY(body.contains("name=\"style\""));
        QVERIFY(body.contains("Format as bullet."));
        QVERIFY(body.contains("name=\"vocabulary\""));
        QVERIFY(body.contains("Zentrix"));
        QVERIFY(body.contains("Velqora"));
        QVERIFY(body.contains("filename=\"audio.wav\""));
    }

    void testErrorClassification400DurationLimit() {
        SarvamSttClient client;
        client.setApiKey(u"valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 400;
        mockNam->nextResponseData =
            R"({"error": {"message": "audio that cannot be read, a file over 10,000,000 bytes, or a clip over 30 s", "code": "invalid_request_error"}})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        const QByteArray shortWav = createFakeWav(29.0);
        client.transcribe(shortWav);

        QTRY_COMPARE(errorSpy.count(), 1);
        QVERIFY(client.lastError().contains(u"30-second limit"_s));
    }

    void testErrorClassification403InvalidKey() {
        SarvamSttClient client;
        client.setApiKey(u"wrong_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 403;
        mockNam->nextResponseData =
            R"({"error": {"message": "Access denied due to invalid subscription key.", "code": "invalid_api_key_error"}})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        client.transcribe(createFakeWav(2.0));

        QTRY_VERIFY(client.isApiKeyInvalid());
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::InvalidApiKey);
        QCOMPARE(errorSpy.count(), 1);
    }

    void testErrorClassification429RateLimit() {
        SarvamSttClient client;
        client.setApiKey(u"valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 429;
        mockNam->nextHeaders.append({QByteArray("retry-after"), QByteArray("12")});
        mockNam->nextResponseData = R"({"error": {"message": "Rate limit exceeded", "code": "rate_limit_exceeded_error"}})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        client.transcribe(createFakeWav(2.0));

        QTRY_VERIFY(client.isRateLimited());
        QCOMPARE(client.retrySecondsRemaining(), 12);
        QCOMPARE(client.errorCategory(), AbstractCloudSttClient::ErrorCategory::RateLimited);
    }

    void testCancelResetsState() {
        SarvamSttClient client;
        client.setApiKey(u"valid_key"_s);

        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = R"({"text": "Late result"})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy readySpy(&client, &AbstractSttClient::transcriptionReady);

        client.transcribe(createFakeWav(2.0));
        QVERIFY(client.isBusy());

        client.cancel();
        QCOMPARE(client.isBusy(), false);
        QCOMPARE(client.isCancelled(), true);

        QTest::qWait(50);
        QCOMPARE(readySpy.count(), 0);
    }
};

QTEST_GUILESS_MAIN(TestSarvamSttClient)
#include "test_sarvam_stt_client.moc"
