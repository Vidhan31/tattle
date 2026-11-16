#include "TranscribeWorker.h"

#include "LoggingCategories.h"

#include "WavDecoder.h"

#include <QFile>
#include <QFileInfo>
#include <QThread>

#include <algorithm>
#include <vector>

#include <transcribe.h>

using namespace Qt::StringLiterals;

TranscribeWorker::TranscribeWorker(QObject* parent)
    : QObject(parent) { }

TranscribeWorker::~TranscribeWorker() {
    cleanupContext();
}

void TranscribeWorker::cleanupContext() {
    if (m_session) {
        transcribe_session_free(m_session);
        m_session = nullptr;
    }
    if (m_model) {
        transcribe_model_free(m_model);
        m_model = nullptr;
    }
    m_activeDevice.clear();
    m_maxAudioMs = 0;
}

void TranscribeWorker::loadModel(uint64_t loadRequestId, const QString& modelPath, bool useGpu) {
    if (loadRequestId > m_cancelledLoadRequestId.load(std::memory_order_relaxed)) {
        resetAbort();
    }

    if (isLoadAborted(loadRequestId)) {
        qCDebug(lcSpeech) << "TranscribeWorker: Model load request" << loadRequestId << "cancelled prior to processing";
        emit modelLoaded(loadRequestId, modelPath, false, tr("Model loading cancelled"), QString());
        return;
    }

    cleanupContext();

    if (!QFile::exists(modelPath)) {
        qCWarning(lcSpeech) << "TranscribeWorker: Model file does not exist at:" << modelPath;
        emit modelLoaded(loadRequestId, modelPath, false, tr("Model file not found at %1").arg(modelPath), QString());
        return;
    }

    qCDebug(lcSpeech) << "TranscribeWorker: Initializing transcribe context from" << modelPath
                      << "(requested GPU:" << useGpu << ", loadRequestId:" << loadRequestId << ")";

    transcribe_model_load_params lparams;
    transcribe_model_load_params_init(&lparams);
    lparams.backend = useGpu ? TRANSCRIBE_BACKEND_AUTO : TRANSCRIBE_BACKEND_CPU;
    lparams.no_mmap = false;

    bool gpuFallback = false;
    const QByteArray pathUtf8 = modelPath.toUtf8();
    transcribe_status status = transcribe_model_load_file(pathUtf8.constData(), &lparams, &m_model);

    if (status == TRANSCRIBE_ERR_BACKEND && useGpu && !isLoadAborted(loadRequestId)) {
        qCWarning(lcSpeech) << "TranscribeWorker: GPU initialization failed for" << modelPath
                            << ". Falling back to CPU inference...";
        gpuFallback = true;
        lparams.backend = TRANSCRIBE_BACKEND_CPU;
        status = transcribe_model_load_file(pathUtf8.constData(), &lparams, &m_model);
    }

    if (isLoadAborted(loadRequestId)) {
        qCDebug(lcSpeech) << "TranscribeWorker: Model loading cancelled for request" << loadRequestId;
        cleanupContext();
        emit modelLoaded(loadRequestId, modelPath, false, tr("Model loading cancelled"), QString());
        return;
    }

    if (status != TRANSCRIBE_OK || !m_model) {
        qCWarning(lcSpeech) << "TranscribeWorker: transcribe_model_load_file failed for" << modelPath
                            << "with code:" << status;
        cleanupContext();
        emit modelLoaded(
            loadRequestId, modelPath, false,
            tr("Failed to initialize model context: %1").arg(QString::fromUtf8(transcribe_status_string(status))),
            QString());
        return;
    }

    transcribe_session_params sparams;
    transcribe_session_params_init(&sparams);
    sparams.n_threads = std::clamp(QThread::idealThreadCount(), 1, 4);

    status = transcribe_session_init(m_model, &sparams, &m_session);
    if (status != TRANSCRIBE_OK || !m_session) {
        qCWarning(lcSpeech) << "TranscribeWorker: transcribe_session_init failed with code:" << status;
        cleanupContext();
        emit modelLoaded(loadRequestId, modelPath, false,
                         tr("Failed to initialize transcription session: %1")
                             .arg(QString::fromUtf8(transcribe_status_string(status))),
                         QString());
        return;
    }

    transcribe_set_abort_callback(
        m_session,
        [](void* userData) -> bool {
            auto* self = static_cast<TranscribeWorker*>(userData);
            return self && self->isAborted(self->m_runningRequestId.load(std::memory_order_acquire));
        },
        this);

    transcribe_capabilities caps;
    transcribe_capabilities_init(&caps);
    if (transcribe_model_get_capabilities(m_model, &caps) == TRANSCRIBE_OK) {
        m_maxAudioMs = caps.max_audio_ms;
    }

    const char* backend = transcribe_model_backend(m_model);
    const QString backendStr = backend ? QString::fromUtf8(backend).toUpper() : u"CPU"_s;
    if (backendStr == u"VULKAN"_s) {
        m_activeDevice = u"GPU (Vulkan)"_s;
    } else if (backendStr.contains(u"CUDA"_s)) {
        m_activeDevice = u"GPU (CUDA)"_s;
    } else if (backendStr.contains(u"METAL"_s)) {
        m_activeDevice = u"GPU (Metal)"_s;
    } else if (gpuFallback) {
        m_activeDevice = u"CPU (Fallback)"_s;
    } else {
        m_activeDevice = u"CPU"_s;
    }

    qCDebug(lcSpeech) << "TranscribeWorker: Model loaded successfully. Active device:" << m_activeDevice;
    emit modelLoaded(loadRequestId, modelPath, true, QString(), m_activeDevice);
}

void TranscribeWorker::unloadModel(uint64_t unloadRequestId) {
    cancel();
    cleanupContext();
    qCDebug(lcSpeech) << "TranscribeWorker: Model context unloaded";
    emit modelUnloaded(unloadRequestId);
}

void TranscribeWorker::resetAbort() {
    m_abortRequested.store(false, std::memory_order_release);
}

void TranscribeWorker::cancel(uint64_t requestId) {
    if (requestId == 0) {
        m_abortRequested.store(true, std::memory_order_release);
    } else {
        uint64_t current = m_cancelledRequestId.load(std::memory_order_relaxed);
        while (current < requestId && !m_cancelledRequestId.compare_exchange_weak(
                                          current, requestId, std::memory_order_release, std::memory_order_relaxed)) { }
    }
}

void TranscribeWorker::cancelLoad(uint64_t loadRequestId) {
    if (loadRequestId == 0) {
        m_abortRequested.store(true, std::memory_order_release);
    } else {
        uint64_t current = m_cancelledLoadRequestId.load(std::memory_order_relaxed);
        while (current < loadRequestId &&
               !m_cancelledLoadRequestId.compare_exchange_weak(current, loadRequestId, std::memory_order_release,
                                                               std::memory_order_relaxed)) { }
    }
}

bool TranscribeWorker::isAborted(uint64_t requestId) const {
    if (m_abortRequested.load(std::memory_order_acquire)) {
        return true;
    }
    if (QThread::currentThread() && QThread::currentThread()->isInterruptionRequested()) {
        return true;
    }
    const uint64_t effectiveRequestId =
        (requestId != 0) ? requestId : m_runningRequestId.load(std::memory_order_acquire);
    if (effectiveRequestId > 0 && effectiveRequestId <= m_cancelledRequestId.load(std::memory_order_acquire)) {
        return true;
    }
    return false;
}

bool TranscribeWorker::isLoadAborted(uint64_t loadRequestId) const {
    if (m_abortRequested.load(std::memory_order_acquire)) {
        return true;
    }
    if (QThread::currentThread() && QThread::currentThread()->isInterruptionRequested()) {
        return true;
    }
    if (loadRequestId > 0 && loadRequestId <= m_cancelledLoadRequestId.load(std::memory_order_acquire)) {
        return true;
    }
    return false;
}

void TranscribeWorker::transcribe(uint64_t requestId, const QByteArray& wavData, const QString& language,
                                  const QString& prompt) {
    m_runningRequestId.store(requestId, std::memory_order_release);
    struct RunningGuard {
        std::atomic<uint64_t>& var;
        ~RunningGuard() { var.store(0, std::memory_order_release); }
    } guard {m_runningRequestId};

    if (requestId > 0 && requestId <= m_cancelledRequestId.load(std::memory_order_acquire)) {
        qCDebug(lcSpeech) << "TranscribeWorker: Transcription request" << requestId << "cancelled prior to processing";
        return;
    }

    if (isAborted(requestId)) {
        qCDebug(lcSpeech) << "TranscribeWorker: Transcription request" << requestId << "cancelled prior to processing";
        return;
    }

    if (!m_model || !m_session) {
        qCWarning(lcSpeech) << "TranscribeWorker: Transcribe called but model or session is not initialized";
        emit transcriptionFailed(requestId, tr("Offline speech model is not loaded"));
        return;
    }

    std::vector<float> pcmf32;
    if (!WavDecoder::decode(wavData, pcmf32)) {
        qCWarning(lcSpeech) << "TranscribeWorker: Invalid or unsupported audio format in WAV payload";
        emit transcriptionFailed(requestId, tr("Invalid audio data: unable to extract PCM samples"));
        return;
    }

    if (isAborted(requestId)) {
        qCDebug(lcSpeech) << "TranscribeWorker: Transcription request" << requestId << "cancelled prior to inference";
        return;
    }

    const int64_t audioDurationMs = static_cast<int64_t>((pcmf32.size() * 1000ULL) / 16000ULL);
    if (m_maxAudioMs > 0 && audioDurationMs > m_maxAudioMs) {
        qCWarning(lcSpeech) << "TranscribeWorker: Input audio duration" << audioDurationMs << "ms exceeds model limit"
                            << m_maxAudioMs << "ms";
        emit transcriptionFailed(requestId, tr("Audio duration (%1s) exceeds model limit (%2s)")
                                                .arg(audioDurationMs / 1000.0, 0, 'f', 1)
                                                .arg(m_maxAudioMs / 1000.0, 0, 'f', 1));
        return;
    }

    transcribe_run_params rparams;
    transcribe_run_params_init(&rparams);
    rparams.task = TRANSCRIBE_TASK_TRANSCRIBE;
    rparams.timestamps = TRANSCRIBE_TIMESTAMPS_NONE;
    rparams.pnc = TRANSCRIBE_PNC_MODE_DEFAULT;
    rparams.itn = TRANSCRIBE_ITN_MODE_DEFAULT;
    rparams.diarize = TRANSCRIBE_DIARIZE_MODE_DEFAULT;

    QByteArray langUtf8;
    if (!language.isEmpty() && language != u"auto"_s) {
        langUtf8 = language.toUtf8();
        rparams.language = langUtf8.constData();
    } else {
        rparams.language = nullptr;
    }

    QByteArray promptUtf8;
    if (!prompt.isEmpty()) {
        promptUtf8 = prompt.toUtf8();
        rparams.prompt = promptUtf8.constData();
    }

    const int sampleCount = static_cast<int>(pcmf32.size());
    qCDebug(lcSpeech) << "TranscribeWorker: Running inference for request" << requestId << "on" << sampleCount
                      << "samples (~" << (sampleCount / 16000.0F)
                      << "s of audio) with lang:" << (rparams.language ? rparams.language : "auto");

    const transcribe_status status =
        transcribe_run(m_session, pcmf32.data(), static_cast<int>(pcmf32.size()), &rparams);

    if (isAborted(requestId) || status == TRANSCRIBE_ERR_ABORTED || transcribe_was_aborted(m_session)) {
        qCDebug(lcSpeech) << "TranscribeWorker: Transcription request" << requestId
                          << "cancelled during/after inference";
        return;
    }

    if (status != TRANSCRIBE_OK) {
        qCWarning(lcSpeech) << "TranscribeWorker: transcribe_run failed with return code:" << status;
        emit transcriptionFailed(
            requestId, tr("Speech inference failed (%1)").arg(QString::fromUtf8(transcribe_status_string(status))));
        return;
    }

    const char* text = transcribe_full_text(m_session);
    const QString transcription = text ? QString::fromUtf8(text).trimmed() : QString();

    qCDebug(lcSpeech) << "TranscribeWorker: Transcription request" << requestId << "completed successfully ("
                      << transcription.size() << "chars)";
    emit transcriptionFinished(requestId, transcription);
}
