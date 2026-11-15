#include "TranscribeSttClient.h"

#include "LoggingCategories.h"

#include "TranscribeModelManager.h"
#include "TranscribeWorker.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

TranscribeSttClient::TranscribeSttClient(QObject* parent)
    : AbstractOfflineSttClient(parent)
    , m_worker(new TranscribeWorker()) {
    m_worker->moveToThread(&m_workerThread);

    connect(this, &TranscribeSttClient::requestLoadModel, m_worker, &TranscribeWorker::loadModel, Qt::QueuedConnection);
    connect(this, &TranscribeSttClient::requestUnloadModel, m_worker, &TranscribeWorker::unloadModel,
            Qt::QueuedConnection);
    connect(this, &TranscribeSttClient::requestTranscribe, m_worker, &TranscribeWorker::transcribe,
            Qt::QueuedConnection);

    connect(m_worker, &TranscribeWorker::modelLoaded, this, &TranscribeSttClient::onWorkerModelLoaded,
            Qt::QueuedConnection);
    connect(m_worker, &TranscribeWorker::modelUnloaded, this, &TranscribeSttClient::onWorkerModelUnloaded,
            Qt::QueuedConnection);
    connect(m_worker, &TranscribeWorker::transcriptionFinished, this,
            &TranscribeSttClient::onWorkerTranscriptionFinished, Qt::QueuedConnection);
    connect(m_worker, &TranscribeWorker::transcriptionFailed, this, &TranscribeSttClient::onWorkerTranscriptionFailed,
            Qt::QueuedConnection);

    if (auto* app = QCoreApplication::instance()) {
        connect(app, &QCoreApplication::aboutToQuit, this, [this]() {
            cancel();
            if (m_worker) {
                m_worker->cancel(0);
            }
        });
    }

    m_workerThread.setObjectName(u"TranscribeInferenceThread"_s);
    m_workerThread.start();

    QDir().mkpath(modelsDirectory());
}

TranscribeSttClient::~TranscribeSttClient() {
    cancel();
    if (m_worker) {
        m_worker->cancel(0);
    }
    m_workerThread.requestInterruption();
    m_workerThread.quit();
    if (!m_workerThread.wait()) {
        qCWarning(lcSpeech) << "TranscribeSttClient: Failed waiting for worker thread to exit";
    }
    delete m_worker;
    m_worker = nullptr;
}

void TranscribeSttClient::setModelManager(TranscribeModelManager* manager) {
    if (m_modelManager == manager) {
        return;
    }

    if (m_modelManager) {
        disconnect(m_modelManager, &TranscribeModelManager::selectedModelChanged, this,
                   &TranscribeSttClient::checkModelStatus);
        disconnect(m_modelManager, &TranscribeModelManager::modelStatusChanged, this,
                   &TranscribeSttClient::checkModelStatus);
    }

    m_modelManager = manager;

    if (m_modelManager) {
        connect(m_modelManager, &TranscribeModelManager::selectedModelChanged, this, [this]() {
            m_explicitlyUnloaded = false;
            checkModelStatus();
        });
        connect(m_modelManager, &TranscribeModelManager::modelStatusChanged, this,
                &TranscribeSttClient::checkModelStatus);
    }

    checkModelStatus();
}

TranscribeModelManager* TranscribeSttClient::modelManager() const {
    return m_modelManager;
}

QString TranscribeSttClient::modelsDirectory() const {
    if (m_modelManager) {
        return m_modelManager->modelsDirectory();
    }
    const QString genericData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return genericData + u"/tattle/models"_s;
}

QString TranscribeSttClient::modelFileName() const {
    const QString path = resolveModelPath();
    return QFileInfo(path).fileName();
}

QString TranscribeSttClient::resolveModelPath() const {
    if (m_modelManager) {
        return m_modelManager->selectedModelPath();
    }

    QString primary = modelsDirectory() + u"/whisper-base.en-Q8_0.gguf"_s;
    if (QFile::exists(primary)) {
        return primary;
    }

    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString fallback = appData + u"/models/whisper-base.en-Q8_0.gguf"_s;
    if (QFile::exists(fallback)) {
        return fallback;
    }

    return primary;
}

bool TranscribeSttClient::isModelInstalled() const {
    return QFile::exists(resolveModelPath());
}

bool TranscribeSttClient::isModelLoaded() const {
    return m_modelLoaded;
}

QString TranscribeSttClient::modelPath() const {
    return resolveModelPath();
}

void TranscribeSttClient::loadSettings() {
    checkModelStatus();
}

QString TranscribeSttClient::loadedModelPath() const {
    return m_loadedModelPath;
}

QString TranscribeSttClient::computeDevice() const {
    return m_computeDevice.isEmpty() ? u"None (Model Unloaded)"_s : m_computeDevice;
}

QString TranscribeSttClient::lastError() const {
    return m_lastError;
}

bool TranscribeSttClient::isVulkanSupported() {
#if defined(GGML_USE_VULKAN)
    return true;
#else
    return false;
#endif
}

bool TranscribeSttClient::isReady() const {
    return m_modelLoaded && !m_busy && (m_loadedModelPath == resolveModelPath()) && QFile::exists(m_loadedModelPath);
}

bool TranscribeSttClient::isBusy() const {
    return m_busy;
}

void TranscribeSttClient::activate() {
    m_active = true;
    m_explicitlyUnloaded = false;
    if (isModelInstalled() && (!m_modelLoaded || m_loadedModelPath != resolveModelPath()) &&
        m_activeLoadRequestId == 0) {
        loadModel();
    }
    emit readyChanged();
}

void TranscribeSttClient::deactivate() {
    m_active = false;
    cancel();
    unloadModel();
    emit readyChanged();
}

void TranscribeSttClient::checkModelStatus() {
    const QString targetPath = resolveModelPath();

    if (m_modelLoaded) {
        if (!m_loadedModelPath.isEmpty() && !QFile::exists(m_loadedModelPath)) {
            qCDebug(lcSpeech) << "TranscribeSttClient: Loaded model file no longer exists, unloading:"
                              << m_loadedModelPath;
            unloadModel();
        } else if (m_loadedModelPath != targetPath) {
            if (isModelInstalled()) {
                if (m_active && !m_explicitlyUnloaded) {
                    if (m_activeLoadRequestId == 0 || m_pendingLoadPath != targetPath) {
                        qCDebug(lcSpeech)
                            << "TranscribeSttClient: Model selection changed, reloading to target:" << targetPath;
                        loadModel(targetPath);
                    }
                }
            } else {
                qCDebug(lcSpeech) << "TranscribeSttClient: Selected model not installed, unloading current model";
                unloadModel();
            }
        }
    } else if (m_active && isModelInstalled() && !m_explicitlyUnloaded) {
        if (m_activeLoadRequestId == 0 || m_pendingLoadPath != targetPath) {
            qCDebug(lcSpeech) << "TranscribeSttClient: Installed model detected, loading target:" << targetPath;
            loadModel(targetPath);
        }
    }

    emit modelStatusChanged();
    emit readyChanged();
}

void TranscribeSttClient::loadModel(const QString& customPath) {
    m_explicitlyUnloaded = false;
    m_activeUnloadRequestId = 0;
    const QString path = customPath.isEmpty() ? resolveModelPath() : customPath;
    if (m_worker && m_activeLoadRequestId > 0) {
        m_worker->cancelLoad(m_activeLoadRequestId);
    }
    const uint64_t loadId = m_nextLoadRequestId++;
    m_activeLoadRequestId = loadId;
    m_pendingLoadPath = path;

    if (!QFile::exists(path)) {
        const QString err = tr("Model file not found at %1. Please download it in Model Settings.").arg(path);
        setLastError(err);
        m_activeLoadRequestId = 0;
        m_pendingLoadPath.clear();
        m_modelLoaded = false;
        m_loadedModelPath.clear();
        m_computeDevice.clear();
        emit modelStatusChanged();
        emit readyChanged();
        emit modelLoadFailed(path, err);
        return;
    }

    setLastError({});
    emit requestLoadModel(loadId, path, true);
}

void TranscribeSttClient::unloadModel() {
    m_explicitlyUnloaded = true;
    if (m_worker && m_activeLoadRequestId > 0) {
        m_worker->cancelLoad(m_activeLoadRequestId);
    }
    m_activeLoadRequestId = 0;
    m_pendingLoadPath.clear();
    const uint64_t unloadId = m_nextUnloadRequestId++;
    m_activeUnloadRequestId = unloadId;
    m_modelLoaded = false;
    m_loadedModelPath.clear();
    m_computeDevice.clear();
    cancel();
    if (m_worker) {
        m_worker->cancel(0);
    }
    emit requestUnloadModel(unloadId);
}

void TranscribeSttClient::transcribe(const QByteArray& wavData) {
    if (m_busy) {
        qCDebug(lcSpeech) << "TranscribeSttClient: transcribe ignored — inference already in progress";
        return;
    }

    if (!isReady()) {
        qCWarning(lcSpeech) << "TranscribeSttClient: transcribe called when not ready (loaded:" << m_modelLoaded << ")";
        const QString err = tr("Offline speech model is not loaded or ready");
        setLastError(err);
        emit errorOccurred(err);
        return;
    }

    if (wavData.isEmpty()) {
        qCWarning(lcSpeech) << "TranscribeSttClient: Empty audio payload passed to transcribe";
        const QString err = tr("No audio data to transcribe");
        setLastError(err);
        emit errorOccurred(err);
        return;
    }

    const uint64_t reqId = m_nextRequestId++;
    m_activeRequestId = reqId;
    setBusy(true);
    setLastError({});

    if (m_worker) {
        m_worker->resetAbort();
    }

    emit requestTranscribe(reqId, wavData, QString(), QString());
}

void TranscribeSttClient::cancel() {
    if (m_busy || m_activeRequestId != 0) {
        const uint64_t cancelledId = m_activeRequestId;
        m_activeRequestId = 0;
        if (m_worker) {
            m_worker->cancel(cancelledId);
        }
        setBusy(false);
    }
}

void TranscribeSttClient::onWorkerModelLoaded(uint64_t loadRequestId, const QString& modelPath, bool success,
                                              const QString& error, const QString& activeDevice) {
    if (loadRequestId == 0 || loadRequestId != m_activeLoadRequestId) {
        qCDebug(lcSpeech) << "TranscribeSttClient: Discarding stale model load result for request" << loadRequestId
                          << "path:" << modelPath << "(active load request:" << m_activeLoadRequestId << ")";
        return;
    }

    m_activeLoadRequestId = 0;
    m_pendingLoadPath.clear();
    m_modelLoaded = success;
    m_computeDevice = activeDevice;
    if (!success) {
        m_loadedModelPath.clear();
        const QString err = error.isEmpty() ? tr("Failed to load speech model") : error;
        setLastError(err);
        emit modelLoadFailed(modelPath, err);
    } else {
        m_loadedModelPath = modelPath;
        setLastError({});
    }

    emit modelStatusChanged();
    emit computeDeviceChanged();
    emit readyChanged();

    if (m_active && !m_explicitlyUnloaded && isModelInstalled()) {
        const QString currentTarget = resolveModelPath();
        if (currentTarget != modelPath) {
            loadModel(currentTarget);
        }
    }
}

void TranscribeSttClient::onWorkerModelUnloaded(uint64_t unloadRequestId) {
    if (m_activeLoadRequestId > 0 || (unloadRequestId != 0 && unloadRequestId != m_activeUnloadRequestId)) {
        qCDebug(lcSpeech) << "TranscribeSttClient: Discarding stale model unloaded event for request"
                          << unloadRequestId;
        return;
    }

    m_activeLoadRequestId = 0;
    m_modelLoaded = false;
    m_loadedModelPath.clear();
    m_computeDevice.clear();
    emit modelStatusChanged();
    emit computeDeviceChanged();
    emit readyChanged();
}

void TranscribeSttClient::onWorkerTranscriptionFinished(uint64_t requestId, const QString& text) {
    if (requestId == 0 || requestId != m_activeRequestId) {
        qCDebug(lcSpeech) << "TranscribeSttClient: Discarding stale transcription finished for request" << requestId
                          << "(active request:" << m_activeRequestId << ")";
        return;
    }

    m_activeRequestId = 0;
    setBusy(false);

    if (text.isEmpty()) {
        const QString err = tr("Model returned empty transcription");
        setLastError(err);
        emit errorOccurred(err);
        return;
    }

    setLastError({});
    emit transcriptionReady(text);
}

void TranscribeSttClient::onWorkerTranscriptionFailed(uint64_t requestId, const QString& error) {
    if (requestId == 0 || requestId != m_activeRequestId) {
        qCDebug(lcSpeech) << "TranscribeSttClient: Discarding stale transcription failure for request" << requestId
                          << "(active request:" << m_activeRequestId << ")";
        return;
    }

    m_activeRequestId = 0;
    setBusy(false);
    setLastError(error);
    emit errorOccurred(error);
}

void TranscribeSttClient::setBusy(bool busy) {
    if (m_busy != busy) {
        m_busy = busy;
        emit busyChanged();
        emit readyChanged();
    }
}

void TranscribeSttClient::setLastError(const QString& error) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
}
