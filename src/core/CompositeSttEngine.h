#pragma once

#include "AbstractSttClient.h"

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QString>

class AudioRecorder;
class AbstractCloudSttClient;
class AbstractOfflineSttClient;
class TranscribeModelManager;
class SystemHealthMonitor;
class SettingsManager;

class CompositeSttEngine : public AbstractSttClient {
    Q_OBJECT

    Q_PROPERTY(TranscriptionBackend activeBackend READ activeBackend WRITE setActiveBackend NOTIFY activeBackendChanged FINAL)
    Q_PROPERTY(bool autoSwitchOffline READ autoSwitchOffline WRITE setAutoSwitchOffline NOTIFY autoSwitchOfflineChanged FINAL)
    Q_PROPERTY(bool isAutoSwitchedToOffline READ isAutoSwitchedToOffline NOTIFY autoSwitchedToOfflineChanged FINAL)

public:
    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class TranscriptionBackend { Cloud, TranscribeCpp };
    Q_ENUM(TranscriptionBackend)

    explicit CompositeSttEngine(QObject* parent = nullptr);
    ~CompositeSttEngine() override = default;

    void setCloudClient(AbstractCloudSttClient* client);
    [[nodiscard]] AbstractCloudSttClient* cloudClient() const noexcept;

    void setTranscribeClient(AbstractOfflineSttClient* client);
    [[nodiscard]] AbstractOfflineSttClient* transcribeClient() const noexcept;

    void setOfflineClient(AbstractOfflineSttClient* client) { setTranscribeClient(client); }
    [[nodiscard]] AbstractOfflineSttClient* offlineClient() const noexcept { return transcribeClient(); }

    void setModelManager(TranscribeModelManager* manager);
    [[nodiscard]] TranscribeModelManager* modelManager() const noexcept;

    void setHealthMonitor(SystemHealthMonitor* monitor);
    [[nodiscard]] SystemHealthMonitor* healthMonitor() const noexcept;

    void setAudioRecorder(AudioRecorder* recorder);
    [[nodiscard]] AudioRecorder* audioRecorder() const noexcept;

    void setSettingsManager(SettingsManager* settings);
    [[nodiscard]] SettingsManager* settingsManager() const noexcept;

    [[nodiscard]] TranscriptionBackend activeBackend() const;
    void setActiveBackend(TranscriptionBackend backend, bool persistSettings = true);

    [[nodiscard]] bool autoSwitchOffline() const;
    void setAutoSwitchOffline(bool enable);

    [[nodiscard]] bool isAutoSwitchedToOffline() const;

    void loadSettings();
    void loadModel(const QString& customPath = QString());
    void unloadModel();
    void checkModelStatus();

    [[nodiscard]] AbstractSttClient* activeSttClient() const;

    void transcribe(const QByteArray& wavData) override;
    void cancel() override;
    void retryLast() override;
    [[nodiscard]] bool isReady() const override;
    [[nodiscard]] bool isBusy() const override;
    [[nodiscard]] bool handlesSmartFormatting() const override;
    [[nodiscard]] QString lastError() const override;

    void activate() override;
    void deactivate() override;

    void onNetworkOnlineChanged(bool online);
    void checkPendingOfflineOrOnlineSwitch();
    void applyOfflineSwitch();
    void restoreOnlineBackend();
    void promptOfflineSwitch();
    void promptOnlineSwitch();

signals:
    void activeBackendChanged();
    void autoSwitchOfflineChanged();
    void autoSwitchedToOfflineChanged();
    void cloudClientChanged(AbstractCloudSttClient* client);

    void failoverTriggered();
    void onlineRestored();
    void offlineModelRequired();
    void offlineSwitchSuggested(const QString& modelName);
    void onlineSwitchSuggested();
    void networkDisconnectedDuringSession();
    void modelLoadFailed(const QString& modelPath, const QString& error);
    void modelStatusChanged();
    void computeDeviceChanged();

private:
    void onActiveClientTranscriptionReady(const QString& text);
    void onActiveClientError(const QString& error);
    void onOfflineModelLoadFailed(const QString& modelPath, const QString& error);

    [[nodiscard]] bool isOfflineModelAvailable() const;
    void setLastError(const QString& error);

    QPointer<AbstractCloudSttClient> m_cloudClient;
    QPointer<AbstractOfflineSttClient> m_offlineClient;
    QPointer<TranscribeModelManager> m_modelManager;
    QPointer<SystemHealthMonitor> m_healthMonitor;
    QPointer<AudioRecorder> m_recorder;
    SettingsManager* m_settingsManager = nullptr;

    TranscriptionBackend m_activeBackend = TranscriptionBackend::TranscribeCpp;
    bool m_autoSwitchOffline = true;
    bool m_pendingOfflineSwitch = false;
    bool m_pendingOnlineSwitch = false;
    bool m_autoSwitchedToOffline = false;
    bool m_isBusy = false;
    bool m_active = false;

    QByteArray m_lastWavData;
    QString m_lastError;
};
