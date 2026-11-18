#include "CompositeSttEngine.h"

#include "AudioRecorder.h"
#include "LoggingCategories.h"

#include "common/SettingsKeys.h"
#include "AbstractCloudSttClient.h"
#include "AbstractOfflineSttClient.h"
#include "SettingsManager.h"
#include "SystemHealthMonitor.h"
#include "TranscribeModelManager.h"

#include <QDebug>
#include <QSettings>

using namespace Qt::StringLiterals;

CompositeSttEngine::CompositeSttEngine(QObject* parent)
    : AbstractSttClient(parent) {
    if (auto* inst = SettingsManager::instance()) {
        setSettingsManager(inst);
    } else {
        loadSettings();
    }
}

void CompositeSttEngine::setSettingsManager(SettingsManager* settings) {
    if (m_settingsManager == settings) {
        return;
    }
    if (m_settingsManager) {
        disconnect(m_settingsManager, nullptr, this, nullptr);
    }
    m_settingsManager = settings;
    if (m_settingsManager) {
        connect(m_settingsManager, &SettingsManager::speechConfigChanged, this,
                [this](const SettingsManager::SpeechConfig& config) {
                    const auto targetBackend = (config.backend == SettingsManager::TranscriptionBackend::TranscribeCpp)
                                                   ? TranscriptionBackend::TranscribeCpp
                                                   : TranscriptionBackend::Cloud;
                    if (!m_autoSwitchedToOffline || targetBackend == TranscriptionBackend::TranscribeCpp) {
                        setActiveBackend(targetBackend, false);
                    }

                    if (m_autoSwitchOffline != config.autoSwitchOffline) {
                        m_autoSwitchOffline = config.autoSwitchOffline;
                        emit autoSwitchOfflineChanged();
                        if (!m_autoSwitchOffline && m_autoSwitchedToOffline && !isBusy()) {
                            restoreOnlineBackend();
                        }
                    }
                });
    }
    loadSettings();
}

SettingsManager* CompositeSttEngine::settingsManager() const noexcept {
    return m_settingsManager;
}

void CompositeSttEngine::loadSettings() {
    int backendInt = 0;
    bool autoSwitch = true;

    if (m_settingsManager) {
        const auto config = m_settingsManager->speechConfig();
        backendInt = (config.backend == SettingsManager::TranscriptionBackend::TranscribeCpp) ? 1 : 0;
        autoSwitch = config.autoSwitchOffline;
    } else {
        const QSettings settings;
        backendInt = settings.value(SettingsKeys::kSpeechTranscriptionBackend.toString(), 0).toInt();
        const QString backendStr = settings.value(SettingsKeys::kSpeechBackend.toString(), QString()).toString();
        if (!backendStr.isEmpty()) {
            backendInt = (backendStr == u"TranscribeCpp"_s || backendStr == u"WhisperCpp"_s) ? 1 : 0;
        }
        autoSwitch = settings.value(SettingsKeys::kSpeechAutoSwitchOffline.toString(), true).toBool();
    }

    const auto targetBackend = (backendInt == 1) ? TranscriptionBackend::TranscribeCpp : TranscriptionBackend::Cloud;

    if (!m_autoSwitchedToOffline || targetBackend == TranscriptionBackend::TranscribeCpp) {
        if (m_activeBackend != targetBackend) {
            m_activeBackend = targetBackend;
            emit activeBackendChanged();
        }
    }

    if (m_autoSwitchOffline != autoSwitch) {
        m_autoSwitchOffline = autoSwitch;
        emit autoSwitchOfflineChanged();
        if (!m_autoSwitchOffline && m_autoSwitchedToOffline && !isBusy()) {
            restoreOnlineBackend();
        }
    }

    if (m_offlineClient) {
        m_offlineClient->loadSettings();
    }
}

void CompositeSttEngine::setCloudClient(AbstractCloudSttClient* client) {
    if (m_cloudClient == client) {
        return;
    }

    if (m_cloudClient) {
        disconnect(m_cloudClient, &AbstractSttClient::transcriptionReady, this,
                   &CompositeSttEngine::onActiveClientTranscriptionReady);
        disconnect(m_cloudClient, &AbstractSttClient::errorOccurred, this, &CompositeSttEngine::onActiveClientError);
        disconnect(m_cloudClient, &AbstractSttClient::busyChanged, this, &AbstractSttClient::busyChanged);
        disconnect(m_cloudClient, &AbstractSttClient::readyChanged, this, &AbstractSttClient::readyChanged);
    }

    m_cloudClient = client;

    if (m_cloudClient) {
        connect(m_cloudClient, &AbstractSttClient::transcriptionReady, this,
                &CompositeSttEngine::onActiveClientTranscriptionReady);
        connect(m_cloudClient, &AbstractSttClient::errorOccurred, this, &CompositeSttEngine::onActiveClientError);
        connect(m_cloudClient, &AbstractSttClient::busyChanged, this, &AbstractSttClient::busyChanged);
        connect(m_cloudClient, &AbstractSttClient::readyChanged, this, &AbstractSttClient::readyChanged);

        if (m_active && m_activeBackend == TranscriptionBackend::Cloud) {
            m_cloudClient->activate();
        }
    }

    emit cloudClientChanged(m_cloudClient);
    emit readyChanged();
}

AbstractCloudSttClient* CompositeSttEngine::cloudClient() const noexcept {
    return m_cloudClient;
}

void CompositeSttEngine::setTranscribeClient(AbstractOfflineSttClient* client) {
    if (m_offlineClient == client) {
        return;
    }

    if (m_offlineClient) {
        disconnect(m_offlineClient, &AbstractSttClient::transcriptionReady, this,
                   &CompositeSttEngine::onActiveClientTranscriptionReady);
        disconnect(m_offlineClient, &AbstractSttClient::errorOccurred, this, &CompositeSttEngine::onActiveClientError);
        disconnect(m_offlineClient, &AbstractSttClient::busyChanged, this, &AbstractSttClient::busyChanged);
        disconnect(m_offlineClient, &AbstractSttClient::readyChanged, this, &AbstractSttClient::readyChanged);
        disconnect(m_offlineClient, &AbstractOfflineSttClient::modelLoadFailed, this,
                   &CompositeSttEngine::onOfflineModelLoadFailed);
        disconnect(m_offlineClient, &AbstractOfflineSttClient::modelStatusChanged, this,
                   &CompositeSttEngine::modelStatusChanged);
        disconnect(m_offlineClient, &AbstractOfflineSttClient::computeDeviceChanged, this,
                   &CompositeSttEngine::computeDeviceChanged);
    }

    m_offlineClient = client;

    if (m_offlineClient) {
        connect(m_offlineClient, &AbstractSttClient::transcriptionReady, this,
                &CompositeSttEngine::onActiveClientTranscriptionReady);
        connect(m_offlineClient, &AbstractSttClient::errorOccurred, this, &CompositeSttEngine::onActiveClientError);
        connect(m_offlineClient, &AbstractSttClient::busyChanged, this, &AbstractSttClient::busyChanged);
        connect(m_offlineClient, &AbstractSttClient::readyChanged, this, &AbstractSttClient::readyChanged);
        connect(m_offlineClient, &AbstractOfflineSttClient::modelLoadFailed, this,
                &CompositeSttEngine::onOfflineModelLoadFailed);
        connect(m_offlineClient, &AbstractOfflineSttClient::modelStatusChanged, this,
                &CompositeSttEngine::modelStatusChanged);
        connect(m_offlineClient, &AbstractOfflineSttClient::computeDeviceChanged, this,
                &CompositeSttEngine::computeDeviceChanged);

        if (m_active && m_activeBackend == TranscriptionBackend::TranscribeCpp) {
            m_offlineClient->activate();
        }
    }

    emit readyChanged();
}

AbstractOfflineSttClient* CompositeSttEngine::transcribeClient() const noexcept {
    return m_offlineClient;
}

void CompositeSttEngine::setModelManager(TranscribeModelManager* manager) {
    m_modelManager = manager;
}

TranscribeModelManager* CompositeSttEngine::modelManager() const noexcept {
    return m_modelManager;
}

void CompositeSttEngine::setHealthMonitor(SystemHealthMonitor* monitor) {
    if (m_healthMonitor == monitor) {
        return;
    }

    if (m_healthMonitor) {
        disconnect(m_healthMonitor, &SystemHealthMonitor::networkOnlineChanged, this,
                   &CompositeSttEngine::onNetworkOnlineChanged);
    }

    m_healthMonitor = monitor;

    if (m_healthMonitor) {
        connect(m_healthMonitor, &SystemHealthMonitor::networkOnlineChanged, this,
                &CompositeSttEngine::onNetworkOnlineChanged);
    }
}

SystemHealthMonitor* CompositeSttEngine::healthMonitor() const noexcept {
    return m_healthMonitor;
}

void CompositeSttEngine::setAudioRecorder(AudioRecorder* recorder) {
    if (m_recorder == recorder) {
        return;
    }

    if (m_recorder) {
        disconnect(m_recorder, &AudioRecorder::recordingChanged, this,
                   &CompositeSttEngine::checkPendingOfflineOrOnlineSwitch);
        disconnect(m_recorder, &AudioRecorder::recordingChanged, this, &AbstractSttClient::busyChanged);
    }

    m_recorder = recorder;

    if (m_recorder) {
        connect(m_recorder, &AudioRecorder::recordingChanged, this,
                &CompositeSttEngine::checkPendingOfflineOrOnlineSwitch);
        connect(m_recorder, &AudioRecorder::recordingChanged, this, &AbstractSttClient::busyChanged);
    }
}

AudioRecorder* CompositeSttEngine::audioRecorder() const noexcept {
    return m_recorder;
}

CompositeSttEngine::TranscriptionBackend CompositeSttEngine::activeBackend() const {
    return m_activeBackend;
}

void CompositeSttEngine::setActiveBackend(TranscriptionBackend backend, bool persistSettings) {
    if (m_activeBackend == backend) {
        if (persistSettings) {
            if (m_settingsManager) {
                m_settingsManager->setBackend(m_activeBackend == TranscriptionBackend::TranscribeCpp
                                                  ? SettingsManager::TranscriptionBackend::TranscribeCpp
                                                  : SettingsManager::TranscriptionBackend::Cloud);
            } else {
                QSettings settings;
                settings.setValue(SettingsKeys::kSpeechTranscriptionBackend.toString(), static_cast<int>(backend));
                settings.setValue(SettingsKeys::kSpeechBackend.toString(),
                                  m_activeBackend == TranscriptionBackend::TranscribeCpp ? u"TranscribeCpp"_s
                                                                                         : u"Cloud"_s);
            }
            if (m_autoSwitchedToOffline || m_pendingOfflineSwitch || m_pendingOnlineSwitch) {
                m_autoSwitchedToOffline = false;
                m_pendingOfflineSwitch = false;
                m_pendingOnlineSwitch = false;
                emit autoSwitchedToOfflineChanged();
            }
        }

        if (m_active && m_activeBackend == TranscriptionBackend::TranscribeCpp) {
            if (m_offlineClient) {
                if (!m_offlineClient->isModelLoaded() && m_offlineClient->isModelInstalled() &&
                    !m_offlineClient->isExplicitlyUnloaded()) {
                    m_offlineClient->activate();
                }
            }
        }
        return;
    }

    if (isBusy()) {
        cancel();
    }

    if (auto* oldClient = activeSttClient()) {
        oldClient->deactivate();
    }

    m_activeBackend = backend;

    if (persistSettings) {
        if (m_settingsManager) {
            m_settingsManager->setBackend(m_activeBackend == TranscriptionBackend::TranscribeCpp
                                              ? SettingsManager::TranscriptionBackend::TranscribeCpp
                                              : SettingsManager::TranscriptionBackend::Cloud);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kSpeechTranscriptionBackend.toString(), static_cast<int>(backend));
            settings.setValue(SettingsKeys::kSpeechBackend.toString(),
                              m_activeBackend == TranscriptionBackend::TranscribeCpp ? u"TranscribeCpp"_s : u"Cloud"_s);
        }
        m_autoSwitchedToOffline = false;
        m_pendingOfflineSwitch = false;
        m_pendingOnlineSwitch = false;
        emit autoSwitchedToOfflineChanged();
    }

    if (m_active) {
        if (auto* newClient = activeSttClient()) {
            newClient->activate();
        }
    }

    emit activeBackendChanged();
    emit readyChanged();
    emit busyChanged();
}

bool CompositeSttEngine::autoSwitchOffline() const {
    return m_autoSwitchOffline;
}

void CompositeSttEngine::setAutoSwitchOffline(bool enable) {
    if (m_autoSwitchOffline == enable) {
        return;
    }

    m_autoSwitchOffline = enable;
    if (m_settingsManager) {
        m_settingsManager->setAutoSwitchOffline(enable);
    } else {
        QSettings settings;
        settings.setValue(SettingsKeys::kSpeechAutoSwitchOffline.toString(), enable);
    }
    emit autoSwitchOfflineChanged();

    if (!m_autoSwitchOffline) {
        m_pendingOfflineSwitch = false;
        m_pendingOnlineSwitch = false;
        if (m_autoSwitchedToOffline && !isBusy()) {
            restoreOnlineBackend();
        }
    }
}

bool CompositeSttEngine::isAutoSwitchedToOffline() const {
    return m_autoSwitchedToOffline;
}

AbstractSttClient* CompositeSttEngine::activeSttClient() const {
    if (m_activeBackend == TranscriptionBackend::TranscribeCpp) {
        return m_offlineClient;
    }
    return m_cloudClient;
}

void CompositeSttEngine::transcribe(const QByteArray& wavData) {
    m_lastWavData = wavData;
    auto* client = activeSttClient();
    if (!client) {
        setLastError(u"No active STT client available"_s);
        emit errorOccurred(lastError());
        return;
    }

    m_isBusy = true;
    emit busyChanged();

    client->transcribe(wavData);
}

void CompositeSttEngine::cancel() {
    if (auto* client = activeSttClient()) {
        client->cancel();
    }
    m_isBusy = false;
    emit busyChanged();
    checkPendingOfflineOrOnlineSwitch();
}

void CompositeSttEngine::retryLast() {
    if (m_lastWavData.isEmpty()) {
        return;
    }
    transcribe(m_lastWavData);
}

bool CompositeSttEngine::isReady() const {
    auto* client = activeSttClient();
    return client ? client->isReady() : false;
}

bool CompositeSttEngine::isBusy() const {
    const bool recorderBusy = m_recorder ? m_recorder->recording() : false;
    auto* client = activeSttClient();
    const bool clientBusy = client ? client->isBusy() : false;
    return recorderBusy || clientBusy || m_isBusy;
}

bool CompositeSttEngine::handlesSmartFormatting() const {
    auto* client = activeSttClient();
    return client ? client->handlesSmartFormatting() : false;
}

QString CompositeSttEngine::lastError() const {
    if (!m_lastError.isEmpty()) {
        return m_lastError;
    }
    auto* client = activeSttClient();
    return client ? client->lastError() : QString();
}

void CompositeSttEngine::activate() {
    m_active = true;
    if (auto* client = activeSttClient()) {
        client->activate();
    }
}

void CompositeSttEngine::deactivate() {
    m_active = false;
    if (auto* client = activeSttClient()) {
        client->deactivate();
    }
}

void CompositeSttEngine::onActiveClientTranscriptionReady(const QString& text) {
    if (sender() && sender() != activeSttClient()) {
        return;
    }

    m_isBusy = false;
    emit busyChanged();
    emit transcriptionReady(text);
    checkPendingOfflineOrOnlineSwitch();
}

void CompositeSttEngine::onActiveClientError(const QString& error) {
    if (sender() && sender() != activeSttClient()) {
        return;
    }

    m_isBusy = false;
    emit busyChanged();
    setLastError(error);
    emit errorOccurred(error);
    checkPendingOfflineOrOnlineSwitch();
}

void CompositeSttEngine::onOfflineModelLoadFailed(const QString& modelPath, const QString& error) {
    emit modelLoadFailed(modelPath, error);
}

void CompositeSttEngine::onNetworkOnlineChanged(bool online) {
    qCInfo(lcSpeech) << "CompositeSttEngine: Network reachability changed. Online:" << online;
    if (!m_autoSwitchOffline) {
        return;
    }

    if (!online) {
        m_pendingOnlineSwitch = false;
        if (m_activeBackend != TranscriptionBackend::Cloud) {
            return;
        }

        if (isBusy()) {
            m_pendingOfflineSwitch = true;
            emit networkDisconnectedDuringSession();
        } else {
            promptOfflineSwitch();
        }
    } else {
        m_pendingOfflineSwitch = false;
        if (!m_autoSwitchOffline || !m_autoSwitchedToOffline || m_activeBackend == TranscriptionBackend::Cloud) {
            return;
        }

        if (isBusy()) {
            qCInfo(lcSpeech) << "CompositeSttEngine: Network restored while busy; will suggest cloud switch when idle";
            m_pendingOnlineSwitch = true;
        } else {
            promptOnlineSwitch();
        }
    }
}

void CompositeSttEngine::checkPendingOfflineOrOnlineSwitch() {
    if (isBusy()) {
        return;
    }

    if (m_pendingOfflineSwitch) {
        m_pendingOfflineSwitch = false;
        const bool networkOnline = m_healthMonitor ? m_healthMonitor->isNetworkOnline() : true;
        if (m_autoSwitchOffline && !networkOnline && m_activeBackend == TranscriptionBackend::Cloud) {
            promptOfflineSwitch();
            return;
        }
    }

    if (m_pendingOnlineSwitch) {
        m_pendingOnlineSwitch = false;
        const bool networkOnline = m_healthMonitor ? m_healthMonitor->isNetworkOnline() : true;
        if (m_autoSwitchOffline && m_autoSwitchedToOffline && networkOnline &&
            m_activeBackend == TranscriptionBackend::TranscribeCpp) {
            promptOnlineSwitch();
        }
    }
}

void CompositeSttEngine::promptOfflineSwitch() {
    if (!isOfflineModelAvailable()) {
        qCWarning(lcSpeech) << "CompositeSttEngine: Offline switch prompted, but no transcribe.cpp model is available";
        emit offlineModelRequired();
        return;
    }

    const QString modelName = m_modelManager ? m_modelManager->selectedModelName() : QString();
    qCInfo(lcSpeech) << "CompositeSttEngine: Offline switch suggested with model:" << modelName;
    emit offlineSwitchSuggested(modelName);
}

void CompositeSttEngine::promptOnlineSwitch() {
    qCInfo(lcSpeech) << "CompositeSttEngine: Online connection restored, cloud switch suggested";
    emit onlineSwitchSuggested();
}

void CompositeSttEngine::applyOfflineSwitch() {
    if (!isOfflineModelAvailable()) {
        qCWarning(lcSpeech)
            << "CompositeSttEngine: Switch to offline requested, but no transcribe.cpp model is available";
        emit offlineModelRequired();
        return;
    }

    qCInfo(lcSpeech) << "CompositeSttEngine: Offline model verified. Switching active backend to TranscribeCpp";
    m_autoSwitchedToOffline = true;
    emit autoSwitchedToOfflineChanged();
    setActiveBackend(TranscriptionBackend::TranscribeCpp, false);
    emit failoverTriggered();
}

void CompositeSttEngine::restoreOnlineBackend() {
    qCInfo(lcSpeech) << "CompositeSttEngine: Restoring active backend back to Cloud";
    m_pendingOnlineSwitch = false;
    setActiveBackend(TranscriptionBackend::Cloud, false);
    m_autoSwitchedToOffline = false;
    emit autoSwitchedToOfflineChanged();
    emit onlineRestored();
}

bool CompositeSttEngine::isOfflineModelAvailable() const {
    if (m_modelManager) {
        if (!m_modelManager->isSelectedModelInstalled() && m_modelManager->hasAnyModelInstalled()) {
            const QString fallbackId = m_modelManager->firstInstalledModelId();
            qCInfo(lcSpeech) << "CompositeSttEngine: Selected model not installed. Auto-selecting installed model:"
                             << fallbackId;
            m_modelManager->setSelectedModelId(fallbackId);
        }
        return m_modelManager->isSelectedModelInstalled();
    }

    if (m_offlineClient) {
        return m_offlineClient->isModelInstalled();
    }

    return false;
}

void CompositeSttEngine::setLastError(const QString& error) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
}

void CompositeSttEngine::loadModel(const QString& customPath) {
    if (m_offlineClient) {
        m_offlineClient->loadModel(customPath);
    }
}

void CompositeSttEngine::unloadModel() {
    if (m_offlineClient) {
        m_offlineClient->unloadModel();
    }
}

void CompositeSttEngine::checkModelStatus() {
    if (m_offlineClient) {
        m_offlineClient->checkModelStatus();
    }
}
