#pragma once

#include "common/SettingsKeys.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>

class SettingsManager : public QObject {
    Q_OBJECT

public:
    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class TranscriptionBackend { Cloud = 0, TranscribeCpp };
    Q_ENUM(TranscriptionBackend)

    // NOLINTNEXTLINE(performance-enum-size) QML-registered enums need int backing for qmlcachegen AOT
    enum class RecordingMode { Toggle = 0, PushToTalk };
    Q_ENUM(RecordingMode)

    struct SpeechConfig {
        TranscriptionBackend backend = TranscriptionBackend::Cloud;
        RecordingMode recordingMode = RecordingMode::Toggle;
        bool autoSwitchOffline = true;
        bool operator==(const SpeechConfig&) const = default;
    };

    struct GroqSttConfig {
        QString model = QStringLiteral("whisper-large-v3-turbo");
        QString language;
        QString customPrompt;
        bool operator==(const GroqSttConfig&) const = default;
    };

    struct GeminiSttConfig {
        QString model = QStringLiteral("gemini-3.5-transcribe");
        QString mode = QStringLiteral("smart");
        QString customVocabulary;
        bool operator==(const GeminiSttConfig&) const = default;
    };

    struct MetaMusaSttConfig {
        QString model = QStringLiteral("muse-voice-transcribe-1.0");
        QString keywords;
        bool operator==(const MetaMusaSttConfig&) const = default;
    };

    struct LlmConfig {
        bool enabled = false;
        QString model = QStringLiteral("openai/gpt-oss-20b");
        QString preset = QStringLiteral("grammar");
        QString customPrompt;
        double temperature = 0.1;
        bool operator==(const LlmConfig&) const = default;
    };

    struct TextInjectorConfig {
        bool preventClipboardHistory = true;
        int injectionDelayMs = 200;
        bool operator==(const TextInjectorConfig&) const = default;
    };

    struct AudioFeedbackConfig {
        bool soundEnabled = true;
        bool operator==(const AudioFeedbackConfig&) const = default;
    };

    struct NotificationConfig {
        bool notificationsEnabled = true;
        bool transcriptionNotificationsEnabled = true;
        bool modelNotificationsEnabled = true;
        bool audioNotificationsEnabled = true;
        bool failoverNotificationsEnabled = true;
        bool systemNotificationsEnabled = true;
        bool operator==(const NotificationConfig&) const = default;
    };

    struct CloudRouterConfig {
        QString activeProvider = QStringLiteral("groq");
        bool operator==(const CloudRouterConfig&) const = default;
    };

    struct TranscribeConfig {
        QString selectedModel;
        bool operator==(const TranscribeConfig&) const = default;
    };

    explicit SettingsManager(QObject* parent = nullptr);
    ~SettingsManager() override;

    static SettingsManager* instance();
    static void setInstance(SettingsManager* inst);

    void load();
    void save() const;

    [[nodiscard]] const SpeechConfig& speechConfig() const noexcept { return m_speechConfig; }
    [[nodiscard]] const GroqSttConfig& groqSttConfig() const noexcept { return m_groqSttConfig; }
    [[nodiscard]] const GeminiSttConfig& geminiSttConfig() const noexcept { return m_geminiSttConfig; }
    [[nodiscard]] const MetaMusaSttConfig& metaMusaSttConfig() const noexcept { return m_metaMusaSttConfig; }
    [[nodiscard]] const LlmConfig& llmConfig() const noexcept { return m_llmConfig; }
    [[nodiscard]] const TextInjectorConfig& textInjectorConfig() const noexcept { return m_textInjectorConfig; }
    [[nodiscard]] const AudioFeedbackConfig& audioFeedbackConfig() const noexcept { return m_audioFeedbackConfig; }
    [[nodiscard]] const NotificationConfig& notificationConfig() const noexcept { return m_notificationConfig; }
    [[nodiscard]] const CloudRouterConfig& cloudRouterConfig() const noexcept { return m_cloudRouterConfig; }
    [[nodiscard]] const TranscribeConfig& transcribeConfig() const noexcept { return m_transcribeConfig; }

    void setSpeechConfig(const SpeechConfig& config);
    void setGroqSttConfig(const GroqSttConfig& config);
    void setGeminiSttConfig(const GeminiSttConfig& config);
    void setMetaMusaSttConfig(const MetaMusaSttConfig& config);
    void setLlmConfig(const LlmConfig& config);
    void setTextInjectorConfig(const TextInjectorConfig& config);
    void setAudioFeedbackConfig(const AudioFeedbackConfig& config);
    void setNotificationConfig(const NotificationConfig& config);
    void setCloudRouterConfig(const CloudRouterConfig& config);
    void setTranscribeConfig(const TranscribeConfig& config);

    void setBackend(TranscriptionBackend backend);
    void setRecordingMode(RecordingMode mode);
    void setAutoSwitchOffline(bool enable);

    void setGroqModel(const QString& model);
    void setGroqLanguage(const QString& language);
    void setGroqCustomPrompt(const QString& prompt);

    void setGeminiModel(const QString& model);
    void setGeminiMode(const QString& mode);
    void setGeminiCustomVocabulary(const QString& vocabulary);

    void setMetaMusaModel(const QString& model);
    void setMetaMusaKeywords(const QString& keywords);

    void setLlmEnabled(bool enabled);
    void setLlmModel(const QString& model);
    void setLlmPreset(const QString& preset);
    void setLlmCustomPrompt(const QString& prompt);
    void setLlmTemperature(double temp);

    void setPreventClipboardHistory(bool prevent);
    void setInjectionDelay(int delayMs);

    void setSoundEnabled(bool enabled);
    void setNotificationsEnabled(bool enabled);
    void setNotificationsTranscriptionEnabled(bool enabled);
    void setNotificationsModelEnabled(bool enabled);
    void setNotificationsAudioEnabled(bool enabled);
    void setNotificationsFailoverEnabled(bool enabled);
    void setNotificationsSystemEnabled(bool enabled);
    void setActiveProvider(const QString& providerId);
    void setTranscribeSelectedModel(const QString& modelId);

    void setSetting(const QString& key, const QVariant& value);
    [[nodiscard]] QVariant getSetting(const QString& key, const QVariant& defaultValue = {}) const;
    [[nodiscard]] QVariantMap allSettings() const;

signals:
    void speechConfigChanged(const SettingsManager::SpeechConfig& config);
    void groqSttConfigChanged(const SettingsManager::GroqSttConfig& config);
    void geminiSttConfigChanged(const SettingsManager::GeminiSttConfig& config);
    void metaMusaSttConfigChanged(const SettingsManager::MetaMusaSttConfig& config);
    void llmConfigChanged(const SettingsManager::LlmConfig& config);
    void textInjectorConfigChanged(const SettingsManager::TextInjectorConfig& config);
    void audioFeedbackConfigChanged(const SettingsManager::AudioFeedbackConfig& config);
    void notificationConfigChanged(const SettingsManager::NotificationConfig& config);
    void cloudRouterConfigChanged(const SettingsManager::CloudRouterConfig& config);
    void transcribeConfigChanged(const SettingsManager::TranscribeConfig& config);
    void settingChanged(const QString& key, const QVariant& value);

private:
    static QString validateGroqModel(const QString& model);
    static QString validateGeminiModel(const QString& model);
    static QString validateGeminiMode(const QString& mode);
    static QString validateMetaMusaModel(const QString& model);
    static QString validateLlmModel(const QString& model);
    static QString validateLlmPreset(const QString& preset);
    static double validateLlmTemperature(double temp);
    static int validateInjectionDelay(int delayMs);

    SpeechConfig m_speechConfig;
    GroqSttConfig m_groqSttConfig;
    GeminiSttConfig m_geminiSttConfig;
    MetaMusaSttConfig m_metaMusaSttConfig;
    LlmConfig m_llmConfig;
    TextInjectorConfig m_textInjectorConfig;
    AudioFeedbackConfig m_audioFeedbackConfig;
    NotificationConfig m_notificationConfig;
    CloudRouterConfig m_cloudRouterConfig;
    TranscribeConfig m_transcribeConfig;

    static inline SettingsManager* s_instance = nullptr;
};
