#pragma once

#include <QStringView>

namespace SettingsKeys {

inline constexpr QStringView kSpeechBackend = u"Speech/Backend";
inline constexpr QStringView kSpeechTranscriptionBackend = u"Speech/TranscriptionBackend";
inline constexpr QStringView kSpeechRecordingMode = u"Speech/RecordingMode";
inline constexpr QStringView kSpeechAutoSwitchOffline = u"Speech/AutoSwitchOffline";

inline constexpr QStringView kTranscribeSelectedModel = u"Transcribe/SelectedModel";

inline constexpr QStringView kCloudActiveProvider = u"Cloud/ActiveProvider";

inline constexpr QStringView kGroqModel = u"Groq/Model";
inline constexpr QStringView kGroqLanguage = u"Groq/Language";
inline constexpr QStringView kGroqCustomPrompt = u"Groq/CustomPrompt";

inline constexpr QStringView kGeminiModel = u"Gemini/Model";
inline constexpr QStringView kGeminiMode = u"Gemini/Mode";
inline constexpr QStringView kGeminiCustomVocabulary = u"Gemini/CustomVocabulary";

inline constexpr QStringView kMetaMusaModel = u"MetaMusa/Model";
inline constexpr QStringView kMetaMusaKeywords = u"MetaMusa/Keywords";

inline constexpr QStringView kSarvamModel = u"Sarvam/Model";
inline constexpr QStringView kSarvamLanguage = u"Sarvam/Language";
inline constexpr QStringView kSarvamMode = u"Sarvam/Mode";
inline constexpr QStringView kSarvamStylePreset = u"Sarvam/StylePreset";
inline constexpr QStringView kSarvamStyleRules = u"Sarvam/StyleRules";
inline constexpr QStringView kSarvamCustomVocabulary = u"Sarvam/CustomVocabulary";

inline constexpr QStringView kGroqLlmEnabled = u"Groq/LlmEnabled";
inline constexpr QStringView kGroqLlmModel = u"Groq/LlmModel";
inline constexpr QStringView kGroqLlmPreset = u"Groq/LlmPreset";
inline constexpr QStringView kGroqLlmCustomPrompt = u"Groq/LlmCustomPrompt";
inline constexpr QStringView kGroqLlmTemperature = u"Groq/LlmTemperature";

inline constexpr QStringView kAudioSoundEnabled = u"Audio/SoundEnabled";

inline constexpr QStringView kNotificationsEnabled = u"notifications/enabled";
inline constexpr QStringView kNotificationsTranscription = u"notifications/transcription";
inline constexpr QStringView kNotificationsModel = u"notifications/model";
inline constexpr QStringView kNotificationsAudio = u"notifications/audio";
inline constexpr QStringView kNotificationsFailover = u"notifications/failover";
inline constexpr QStringView kNotificationsSystem = u"notifications/system";

inline constexpr QStringView kClipboardPreventHistory = u"Clipboard/PreventHistory";
inline constexpr QStringView kTypingPreInjectionDelayMs = u"Typing/PreInjectionDelayMs";

inline constexpr QStringView kUiThemeMode = u"ui/themeMode";

inline constexpr QStringView kGroqSettingsKey = u"Cloud/Providers/Groq/ApiKey";
inline constexpr QStringView kGroqLegacySettingsKey = u"Groq/ApiKey";
inline constexpr QStringView kGeminiSettingsKey = u"Cloud/Providers/Gemini/ApiKey";
inline constexpr QStringView kMetaMusaSettingsKey = u"Cloud/Providers/MetaMusa/ApiKey";
inline constexpr QStringView kSarvamSettingsKey = u"Cloud/Providers/Sarvam/ApiKey";

} // namespace SettingsKeys
