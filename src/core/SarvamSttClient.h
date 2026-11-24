#pragma once

#include "BaseCloudSttClient.h"

#include <chrono>
#include <optional>

class SarvamSttClient : public BaseCloudSttClient {
    Q_OBJECT

    Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged FINAL)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged FINAL)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged FINAL)
    Q_PROPERTY(QString stylePreset READ stylePreset WRITE setStylePreset NOTIFY stylePresetChanged FINAL)
    Q_PROPERTY(QString styleRules READ styleRules WRITE setStyleRules NOTIFY styleRulesChanged FINAL)
    Q_PROPERTY(QString customVocabulary READ customVocabulary WRITE setCustomVocabulary NOTIFY customVocabularyChanged FINAL)

public:
    explicit SarvamSttClient(QObject* parent = nullptr);
    ~SarvamSttClient() override;

    static SarvamSttClient* instance();

    bool handlesSmartFormatting() const override;
    std::optional<std::chrono::milliseconds> maxAudioDuration() const override;

    QString selectedModel() const;
    void setSelectedModel(const QString& model);

    QString language() const;
    void setLanguage(const QString& lang);

    QString mode() const;
    void setMode(const QString& mode);

    QString stylePreset() const;
    void setStylePreset(const QString& preset);

    QString styleRules() const;
    void setStyleRules(const QString& rules);

    QString customVocabulary() const;
    void setCustomVocabulary(const QString& vocab);

    void setSettingsManager(SettingsManager* settings) override;
    void loadSettings() override;

    void transcribe(const QByteArray& wavData) override;

signals:
    void selectedModelChanged();
    void languageChanged();
    void modeChanged();
    void stylePresetChanged();
    void styleRulesChanged();
    void customVocabularyChanged();

protected:
    void sendTranscribeRequest() override;
    bool parseTranscript(const CloudApiResponse& res, QString& outText) override;
    ErrorCategory classifyError(const CloudApiResponse& res, QString& outMessage) override;

private:
    [[nodiscard]] static double extractWavDurationSeconds(const QByteArray& wavData);

    QString m_selectedModel;
    QString m_language {QStringLiteral("auto")};
    QString m_mode {QStringLiteral("transcribe")};
    QString m_stylePreset {QStringLiteral("clean")};
    QString m_styleRules;
    QString m_customVocabulary;
    bool m_lastTranscriptWasRawFallback = false;

    static constexpr QStringView kDefaultLanguage = u"auto";
    static constexpr QStringView kDefaultMode = u"transcribe";
    static constexpr QStringView kDefaultStylePreset = u"clean";
    static constexpr QStringView kApiEndpoint = u"https://api.sarvam.ai/v2/dictate";
    static constexpr double kMaxClipDurationSeconds = 30.0;
};
