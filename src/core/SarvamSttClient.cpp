#include "SarvamSttClient.h"

#include "LoggingCategories.h"

#include "common/SettingsKeys.h"
#include "ApiKeyStore.h"
#include "CloudResponseParser.h"
#include "NetworkTlsHelper.h"
#include "SettingsManager.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QRegularExpression>
#include <QSettings>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

#ifndef TATTLE_VERSION
#define TATTLE_VERSION "0.0.0-dev"
#endif

namespace {
SarvamSttClient* s_sarvamInstance = nullptr;

QString sarvamUserAgent() {
    const QString appVer = QCoreApplication::applicationVersion().isEmpty() ? u"" TATTLE_VERSION ""_s
                                                                            : QCoreApplication::applicationVersion();
    return u"Tattle/%1 (Linux; Qt %2)"_s.arg(appVer, QString::fromUtf8(QT_VERSION_STR));
}
} // namespace

SarvamSttClient::SarvamSttClient(QObject* parent)
    : BaseCloudSttClient(u"Sarvam STT"_s, u"Sarvam AI"_s, 15s, parent) {
    if (!s_sarvamInstance) {
        s_sarvamInstance = this;
    }

    auto* defaultStore = new ApiKeyStore(this);
    defaultStore->setStorageKeys(u"Tattle"_s, u"sarvam_api_key"_s, SettingsKeys::kSarvamSettingsKey.toString());
    setKeyStore(defaultStore);
    m_ownsKeyStore = true;

    if (auto* inst = SettingsManager::instance()) {
        setSettingsManager(inst);
    } else {
        loadSettings();
    }
}

SarvamSttClient::~SarvamSttClient() {
    if (s_sarvamInstance == this) {
        s_sarvamInstance = nullptr;
    }
}

SarvamSttClient* SarvamSttClient::instance() {
    return s_sarvamInstance;
}

bool SarvamSttClient::handlesSmartFormatting() const {
    return m_mode != u"verbatim"_s && m_stylePreset != u"none"_s;
}

std::optional<std::chrono::milliseconds> SarvamSttClient::maxAudioDuration() const {
    return std::chrono::seconds(30);
}

double SarvamSttClient::extractWavDurationSeconds(const QByteArray& wavData) {
    if (wavData.size() < 44) {
        return 0.0;
    }
    if (!wavData.startsWith("RIFF") || wavData.mid(8, 4) != "WAVE") {
        return 0.0;
    }
    const auto* dataPtr = reinterpret_cast<const quint8*>(wavData.constData());
    const quint32 byteRate = static_cast<quint32>(dataPtr[28]) | (static_cast<quint32>(dataPtr[29]) << 8) |
                             (static_cast<quint32>(dataPtr[30]) << 16) | (static_cast<quint32>(dataPtr[31]) << 24);
    if (byteRate == 0) {
        return 0.0;
    }
    const quint32 pcmSize = (wavData.size() > 44) ? static_cast<quint32>(wavData.size() - 44) : 0;
    return static_cast<double>(pcmSize) / static_cast<double>(byteRate);
}

void SarvamSttClient::setSettingsManager(SettingsManager* settings) {
    if (m_settingsManager == settings) {
        return;
    }
    if (m_settingsManager) {
        disconnect(m_settingsManager, nullptr, this, nullptr);
    }
    m_settingsManager = settings;
    if (m_settingsManager) {
        connect(m_settingsManager, &SettingsManager::sarvamSttConfigChanged, this,
                [this](const SettingsManager::SarvamSttConfig& config) {
                    if (m_selectedModel != config.model) {
                        m_selectedModel = config.model;
                        emit selectedModelChanged();
                    }
                    if (m_language != config.language) {
                        m_language = config.language;
                        emit languageChanged();
                    }
                    if (m_mode != config.mode) {
                        m_mode = config.mode;
                        emit modeChanged();
                    }
                    if (m_stylePreset != config.stylePreset) {
                        m_stylePreset = config.stylePreset;
                        emit stylePresetChanged();
                    }
                    if (m_styleRules != config.styleRules) {
                        m_styleRules = config.styleRules;
                        emit styleRulesChanged();
                    }
                    if (m_customVocabulary != config.customVocabulary) {
                        m_customVocabulary = config.customVocabulary;
                        emit customVocabularyChanged();
                    }
                });
    }
    loadSettings();
}

void SarvamSttClient::loadSettings() {
    QString model;
    QString lang = kDefaultLanguage.toString();
    QString modeVal = kDefaultMode.toString();
    QString presetVal = kDefaultStylePreset.toString();
    QString rules;
    QString vocab;

    if (m_settingsManager) {
        const auto& config = m_settingsManager->sarvamSttConfig();
        model = config.model;
        lang = config.language;
        modeVal = config.mode;
        presetVal = config.stylePreset;
        rules = config.styleRules;
        vocab = config.customVocabulary;
    } else {
        const QSettings settings;
        model = settings.value(SettingsKeys::kSarvamModel.toString(), QString()).toString().trimmed();
        lang = settings.value(SettingsKeys::kSarvamLanguage.toString(), kDefaultLanguage.toString()).toString();
        modeVal = settings.value(SettingsKeys::kSarvamMode.toString(), kDefaultMode.toString()).toString();
        if (modeVal != u"transcribe"_s && modeVal != u"codemix"_s && modeVal != u"translit"_s &&
            modeVal != u"verbatim"_s) {
            modeVal = kDefaultMode.toString();
        }
        presetVal =
            settings.value(SettingsKeys::kSarvamStylePreset.toString(), kDefaultStylePreset.toString()).toString();
        if (presetVal != u"clean"_s && presetVal != u"light"_s && presetVal != u"none"_s) {
            presetVal = kDefaultStylePreset.toString();
        }
        rules = settings.value(SettingsKeys::kSarvamStyleRules.toString(), QString()).toString();
        vocab = settings.value(SettingsKeys::kSarvamCustomVocabulary.toString(), QString()).toString();
    }

    if (m_selectedModel != model) {
        m_selectedModel = model;
        emit selectedModelChanged();
    }
    if (m_language != lang) {
        m_language = lang;
        emit languageChanged();
    }
    if (m_mode != modeVal) {
        m_mode = modeVal;
        emit modeChanged();
    }
    if (m_stylePreset != presetVal) {
        m_stylePreset = presetVal;
        emit stylePresetChanged();
    }
    if (m_styleRules != rules) {
        m_styleRules = rules;
        emit styleRulesChanged();
    }
    if (m_customVocabulary != vocab) {
        m_customVocabulary = vocab;
        emit customVocabularyChanged();
    }

    if (m_keyStore) {
        m_keyStore->loadApiKey();
    }
}

QString SarvamSttClient::selectedModel() const {
    return m_selectedModel;
}

void SarvamSttClient::setSelectedModel(const QString& model) {
    const QString trimmed = model.trimmed();
    if (m_selectedModel != trimmed) {
        m_selectedModel = trimmed;
        if (m_settingsManager) {
            m_settingsManager->setSarvamModel(m_selectedModel);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kSarvamModel.toString(), m_selectedModel);
            settings.sync();
        }
        emit selectedModelChanged();
    }
}

QString SarvamSttClient::language() const {
    return m_language;
}

void SarvamSttClient::setLanguage(const QString& lang) {
    const QString trimmed = lang.trimmed().isEmpty() ? kDefaultLanguage.toString() : lang.trimmed();
    if (m_language != trimmed) {
        m_language = trimmed;
        if (m_settingsManager) {
            m_settingsManager->setSarvamLanguage(m_language);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kSarvamLanguage.toString(), m_language);
            settings.sync();
        }
        emit languageChanged();
    }
}

QString SarvamSttClient::mode() const {
    return m_mode;
}

void SarvamSttClient::setMode(const QString& mode) {
    QString trimmed = mode.trimmed().toLower();
    if (trimmed != u"transcribe"_s && trimmed != u"codemix"_s && trimmed != u"translit"_s && trimmed != u"verbatim"_s) {
        trimmed = kDefaultMode.toString();
    }
    if (m_mode != trimmed) {
        m_mode = trimmed;
        if (m_settingsManager) {
            m_settingsManager->setSarvamMode(m_mode);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kSarvamMode.toString(), m_mode);
            settings.sync();
        }
        emit modeChanged();
    }
}

QString SarvamSttClient::stylePreset() const {
    return m_stylePreset;
}

void SarvamSttClient::setStylePreset(const QString& preset) {
    QString trimmed = preset.trimmed().toLower();
    if (trimmed != u"clean"_s && trimmed != u"light"_s && trimmed != u"none"_s) {
        trimmed = kDefaultStylePreset.toString();
    }
    if (m_stylePreset != trimmed) {
        m_stylePreset = trimmed;
        if (m_settingsManager) {
            m_settingsManager->setSarvamStylePreset(m_stylePreset);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kSarvamStylePreset.toString(), m_stylePreset);
            settings.sync();
        }
        emit stylePresetChanged();
    }
}

QString SarvamSttClient::styleRules() const {
    return m_styleRules;
}

void SarvamSttClient::setStyleRules(const QString& rules) {
    if (m_styleRules != rules) {
        m_styleRules = rules;
        if (m_settingsManager) {
            m_settingsManager->setSarvamStyleRules(m_styleRules);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kSarvamStyleRules.toString(), m_styleRules);
            settings.sync();
        }
        emit styleRulesChanged();
    }
}

QString SarvamSttClient::customVocabulary() const {
    return m_customVocabulary;
}

void SarvamSttClient::setCustomVocabulary(const QString& vocab) {
    if (m_customVocabulary != vocab) {
        m_customVocabulary = vocab;
        if (m_settingsManager) {
            m_settingsManager->setSarvamCustomVocabulary(m_customVocabulary);
        } else {
            QSettings settings;
            settings.setValue(SettingsKeys::kSarvamCustomVocabulary.toString(), m_customVocabulary);
            settings.sync();
        }
        emit customVocabularyChanged();
    }
}

void SarvamSttClient::transcribe(const QByteArray& wavData) {
    const double dur = extractWavDurationSeconds(wavData);
    if (dur > kMaxClipDurationSeconds) {
        qWarning() << m_providerLogName << "Audio duration" << dur << "s exceeds 30-second limit";
        setLastError(tr("Audio exceeds Sarvam's 30-second limit (%1s). Please keep dictation under 30 seconds.")
                         .arg(QString::number(dur, 'f', 1)),
                     ErrorCategory::GeneralError);
        emit errorOccurred(m_lastError);
        return;
    }
    BaseCloudSttClient::transcribe(wavData);
}

void SarvamSttClient::sendTranscribeRequest() {
    if (!isApiKeySet() || m_lastWavData.isEmpty() || m_cancelled) {
        setBusy(false);
        return;
    }

    setBusy(true);
    setLastError({});

    const QString langToUse = m_language.trimmed();
    const QString modeToUse = m_mode.trimmed().isEmpty() ? kDefaultMode.toString() : m_mode.trimmed();
    const QString presetToUse =
        m_stylePreset.trimmed().isEmpty() ? kDefaultStylePreset.toString() : m_stylePreset.trimmed();

    qCDebug(lcNetwork) << "Preparing Sarvam Dictate request -> Lang:" << langToUse << "Mode:" << modeToUse
                       << "Style preset:" << presetToUse << "Audio size:" << m_lastWavData.size() << "bytes"
                       << "(Retry attempt:" << m_retryCount << ")";

    auto* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       u"form-data; name=\"file\"; filename=\"audio.wav\""_s);
    filePart.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, u"audio/wav"_s);
    filePart.setBody(m_lastWavData);
    multiPart->append(filePart);

    if (!langToUse.isEmpty()) {
        QHttpPart langPart;
        langPart.setHeader(QNetworkRequest::ContentDispositionHeader, u"form-data; name=\"language_code\""_s);
        langPart.setBody(langToUse.toUtf8());
        multiPart->append(langPart);
    }

    QHttpPart modePart;
    modePart.setHeader(QNetworkRequest::ContentDispositionHeader, u"form-data; name=\"mode\""_s);
    modePart.setBody(modeToUse.toUtf8());
    multiPart->append(modePart);

    QJsonObject styleObj;
    styleObj.insert(u"preset"_s, presetToUse);
    if (!m_styleRules.trimmed().isEmpty()) {
        styleObj.insert(u"rules"_s, m_styleRules.trimmed());
    }
    QHttpPart stylePart;
    stylePart.setHeader(QNetworkRequest::ContentDispositionHeader, u"form-data; name=\"style\""_s);
    stylePart.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, u"application/json"_s);
    stylePart.setBody(QJsonDocument(styleObj).toJson(QJsonDocument::Compact));
    multiPart->append(stylePart);

    if (!m_customVocabulary.trimmed().isEmpty()) {
        QJsonArray vocabArr;
        const auto tokens = m_customVocabulary.split(QRegularExpression(u"[,;\n\r]+"_s), Qt::SkipEmptyParts);
        for (const auto& token : tokens) {
            const QString trimmed = token.trimmed();
            if (!trimmed.isEmpty()) {
                QJsonObject item;
                item.insert(u"word"_s, trimmed);
                vocabArr.append(item);
            }
        }
        if (!vocabArr.isEmpty()) {
            QHttpPart vocabPart;
            vocabPart.setHeader(QNetworkRequest::ContentDispositionHeader, u"form-data; name=\"vocabulary\""_s);
            vocabPart.setHeader(QNetworkRequest::KnownHeaders::ContentTypeHeader, u"application/json"_s);
            vocabPart.setBody(QJsonDocument(vocabArr).toJson(QJsonDocument::Compact));
            multiPart->append(vocabPart);
        }
    }

    if (!m_selectedModel.trimmed().isEmpty()) {
        QHttpPart modelPart;
        modelPart.setHeader(QNetworkRequest::ContentDispositionHeader, u"form-data; name=\"model\""_s);
        modelPart.setBody(m_selectedModel.trimmed().toUtf8());
        multiPart->append(modelPart);
    }

    QNetworkRequest request(QUrl(kApiEndpoint.toString()));
    request.setRawHeader("api-subscription-key", apiKey().trimmed().toUtf8());
    request.setRawHeader("User-Agent", sarvamUserAgent().toUtf8());

    auto timer = std::make_shared<QElapsedTimer>();
    timer->start();

    if (!m_nam) {
        m_nam = new QNetworkAccessManager(this);
    }
    NetworkTlsHelper::applySecureTls(request);

    m_currentReply = m_nam->post(request, multiPart);
    multiPart->setParent(m_currentReply);

    const QPointer<SarvamSttClient> self = this;
    connect(m_currentReply, &QNetworkReply::finished, this, [self, timer]() {
        if (!self) {
            return;
        }
        auto* reply = self->m_currentReply.data();
        if (!reply) {
            return;
        }
        reply->deleteLater();
        const qint64 latency = timer->elapsed();
        const CloudApiResponse res = CloudResponseParser::parseReply(reply, latency);
        self->handleApiResponse(res);
    });
}

bool SarvamSttClient::parseTranscript(const CloudApiResponse& res, QString& outText) {
    if (!res.isSuccess || res.json.isEmpty()) {
        return false;
    }

    if (res.json.contains(u"text"_s)) {
        outText = res.json.value(u"text"_s).toString();
        if (outText.isEmpty() && res.json.contains(u"transcript"_s)) {
            outText = res.json.value(u"transcript"_s).toString();
        }
        return true;
    }

    if (res.json.contains(u"transcript"_s)) {
        outText = res.json.value(u"transcript"_s).toString();
        return true;
    }

    return false;
}

AbstractCloudSttClient::ErrorCategory SarvamSttClient::classifyError(const CloudApiResponse& res, QString& outMessage) {
    if (res.httpStatus == 400) {
        const QString bodyStr = QString::fromUtf8(res.rawBody);
        if (bodyStr.contains(u"30 s"_s, Qt::CaseInsensitive) || bodyStr.contains(u"30s"_s, Qt::CaseInsensitive) ||
            bodyStr.contains(u"clip over 30"_s, Qt::CaseInsensitive)) {
            outMessage =
                tr("Audio recording exceeds Sarvam's 30-second limit. Please keep dictation under 30 seconds.");
            return ErrorCategory::GeneralError;
        }
    }
    return classifyCommonError(res, outMessage, m_providerDisplayName);
}
