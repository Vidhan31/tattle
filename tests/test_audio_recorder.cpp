#include "AudioRecorder.h"
#include "SarvamSttClient.h"
#include "mock_network_access_manager.h"

#include <QAudioFormat>
#include <QBuffer>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QTest>
#include <QThread>

using namespace Qt::StringLiterals;

class TestableAudioRecorder : public AudioRecorder {
    Q_OBJECT

public:
    using AudioRecorder::AudioRecorder;

    void beginCapture(QIODevice* ioDevice, const QAudioFormat& format) { startTestCapture(ioDevice, format); }
};

class TestAudioRecorder : public QObject {
    Q_OBJECT

private:
    static QAudioFormat makePcmFormat(int sampleRate = 16000, int channels = 1) {
        QAudioFormat format;
        format.setSampleRate(sampleRate);
        format.setChannelCount(channels);
        format.setSampleFormat(QAudioFormat::Int16);
        return format;
    }

private slots:
    void testDelayedAutoStopCapsWavToMaxDurationFrames() {
        TestableAudioRecorder recorder;
        recorder.setMaxRecordingDuration(std::chrono::milliseconds(50));

        // Stereo Int16 at 16 kHz -> 4 bytes/frame, 64,000 bytes/sec.
        // 50 ms = 800 frames = 3,200 bytes. Provide 120 ms (7,680 bytes).
        const QAudioFormat format = makePcmFormat(16000, 2);
        QByteArray pcm(7680, '\x11');
        QBuffer buffer(&pcm);
        QVERIFY(buffer.open(QIODevice::ReadOnly));

        QSignalSpy finishedSpy(&recorder, &AudioRecorder::recordingFinished);
        QSignalSpy maxDurSpy(&recorder, &AudioRecorder::maxDurationReached);

        recorder.beginCapture(&buffer, format);
        // Simulate event-loop stall so the 50 ms single-shot timer fires late.
        QThread::msleep(80);
        QTRY_COMPARE(finishedSpy.count(), 1);

        QCOMPARE(maxDurSpy.count(), 1);
        const QByteArray wav = finishedSpy.first().at(0).toByteArray();
        QCOMPARE(wav.size(), 44 + 3200);
        QCOMPARE((wav.size() - 44) % format.bytesPerFrame(), 0);
    }

    void testDelayedStopAcceptedBySarvam30SecondLimit() {
        TestableAudioRecorder recorder;
        recorder.setMaxRecordingDuration(std::chrono::seconds(30));

        // 16 kHz mono Int16 -> 32,000 bytes/sec. Provide 30.5 s (976,000 bytes).
        const QAudioFormat format = makePcmFormat(16000, 1);
        QByteArray pcm(976000, '\x05');
        QBuffer buffer(&pcm);
        QVERIFY(buffer.open(QIODevice::ReadOnly));

        QSignalSpy finishedSpy(&recorder, &AudioRecorder::recordingFinished);
        recorder.beginCapture(&buffer, format);
        recorder.stopRecording();

        QCOMPARE(finishedSpy.count(), 1);
        const QByteArray wav = finishedSpy.first().at(0).toByteArray();
        QCOMPARE(wav.size(), 44 + 960000);

        SarvamSttClient client;
        client.setApiKey(u"valid_key"_s);
        auto* mockNam = new MockNetworkAccessManager(&client);
        mockNam->nextStatusCode = 200;
        mockNam->nextResponseData = R"({"request_id": "req-cap", "text": "Capped audio ok."})";
        client.setNetworkAccessManager(mockNam);

        QSignalSpy readySpy(&client, &AbstractSttClient::transcriptionReady);
        QSignalSpy errorSpy(&client, &AbstractSttClient::errorOccurred);

        client.transcribe(wav);

        QTRY_COMPARE(readySpy.count(), 1);
        QCOMPARE(errorSpy.count(), 0);
        QCOMPARE(mockNam->requestCount, 1);
        QCOMPARE(readySpy.first().at(0).toString(), u"Capped audio ok."_s);
    }
};

QTEST_GUILESS_MAIN(TestAudioRecorder)
#include "test_audio_recorder.moc"
