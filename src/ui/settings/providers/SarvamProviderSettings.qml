pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Tattle
import "../../controls"
import ".."

Item {
    id: root

    implicitWidth: 500
    implicitHeight: contentColumn.implicitHeight

    ListModel {
        id: sarvamLanguagesModel
        ListElement {
            name: "Auto-detect (Default)"
            code: "auto"
        }
        ListElement {
            name: "English (India) (en-IN)"
            code: "en-IN"
        }
        ListElement {
            name: "Hindi (hi-IN)"
            code: "hi-IN"
        }
        ListElement {
            name: "Bengali (bn-IN)"
            code: "bn-IN"
        }
        ListElement {
            name: "Gujarati (gu-IN)"
            code: "gu-IN"
        }
        ListElement {
            name: "Kannada (kn-IN)"
            code: "kn-IN"
        }
        ListElement {
            name: "Malayalam (ml-IN)"
            code: "ml-IN"
        }
        ListElement {
            name: "Marathi (mr-IN)"
            code: "mr-IN"
        }
        ListElement {
            name: "Odia (od-IN)"
            code: "od-IN"
        }
        ListElement {
            name: "Punjabi (pa-IN)"
            code: "pa-IN"
        }
        ListElement {
            name: "Tamil (ta-IN)"
            code: "ta-IN"
        }
        ListElement {
            name: "Telugu (te-IN)"
            code: "te-IN"
        }
    }

    ListModel {
        id: sarvamModesModel
        ListElement {
            name: "Transcribe (Native Indic script)"
            code: "transcribe"
        }
        ListElement {
            name: "Code-mix (e.g. Hinglish)"
            code: "codemix"
        }
        ListElement {
            name: "Transliterate (Latin / Roman script)"
            code: "translit"
        }
        ListElement {
            name: "Verbatim (Word-for-word transcript)"
            code: "verbatim"
        }
    }

    ListModel {
        id: sarvamStylePresetsModel
        ListElement {
            name: "Clean (Remove fillers & format output)"
            code: "clean"
        }
        ListElement {
            name: "Light (Mild cleanup)"
            code: "light"
        }
        ListElement {
            name: "None (Raw speech output)"
            code: "none"
        }
    }

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Theme.spacingLg

        ApiKeyConfigCard {
            title: qsTr("Sarvam AI API Key")
            description: qsTr("Required for Sarvam Dictate speech recognition and transliteration")
            rowDescription: qsTr("Enter your Sarvam API subscription key")
            apiKey: AppSettings.sarvamApiKey
            placeholderText: qsTr("Enter your API key...")
            helpText: qsTr("API subscription keys available at dashboard.sarvam.ai. Stored securely on your device.")
            linkButtonText: qsTr("Get API Key")
            linkUrl: "https://dashboard.sarvam.ai/"
            onSaveRequested: key => {
                AppSettings.sarvamApiKey = key;
            }
        }

        StyledCard {
            title: qsTr("Language & Script Options")
            description: qsTr("Configure the spoken language, script format, and text cleanup preset")

            PreferenceRow {
                title: qsTr("Spoken Language")
                description: qsTr("Select the spoken language or use Auto-detect.")
                layoutVertical: true

                StyledComboBox {
                    id: langCombo
                    Layout.fillWidth: true
                    textRole: "name"
                    valueRole: "code"
                    model: sarvamLanguagesModel
                    currentIndex: {
                        const idx = indexOfValue(AppSettings.sarvamLanguage);
                        return idx >= 0 ? idx : 0;
                    }

                    onActivated: index => {
                        AppSettings.sarvamLanguage = currentValue;
                    }
                }
            }

            StyledDivider {}

            PreferenceRow {
                title: qsTr("Script & Transliteration Mode")
                description: qsTr(
                                 "Choose whether to transcribe in native Indic script, mixed script (Hinglish), or transliterate into English/Latin letters.")
                layoutVertical: true

                StyledComboBox {
                    id: modeCombo
                    Layout.fillWidth: true
                    textRole: "name"
                    valueRole: "code"
                    model: sarvamModesModel
                    currentIndex: {
                        const idx = indexOfValue(AppSettings.sarvamMode);
                        return idx >= 0 ? idx : 0;
                    }

                    onActivated: index => {
                        AppSettings.sarvamMode = currentValue;
                    }
                }
            }

            StyledDivider {}

            PreferenceRow {
                title: qsTr("Style Preset")
                description: qsTr(
                                 "Clean mode automatically eliminates 'um's, repeats, and false starts while applying smart punctuation.")
                layoutVertical: true

                StyledComboBox {
                    id: styleCombo
                    Layout.fillWidth: true
                    textRole: "name"
                    valueRole: "code"
                    model: sarvamStylePresetsModel
                    currentIndex: {
                        const idx = indexOfValue(AppSettings.sarvamStylePreset);
                        return idx >= 0 ? idx : 0;
                    }

                    onActivated: index => {
                        AppSettings.sarvamStylePreset = currentValue;
                    }
                }
            }

            StyledDivider {}

            PreferenceRow {
                title: qsTr("30-Second Duration Limit")
                description: qsTr(
                                 "Sarvam Dictate clip API is optimized for short dictation (up to 30s). Recordings reaching 30 seconds are automatically stopped and transcribed.")
                layoutVertical: true
            }
        }

        CustomVocabularyCard {
            title: qsTr("Custom Formatting Rules")
            description: qsTr(
                             "Custom natural language instructions for text output (e.g., 'Translate to English' or 'Format each task as a bullet')")
            fieldLabel: qsTr("Formatting Instructions")
            placeholderText: qsTr(
                                 "e.g. Translate into English. Format each task as a bullet point starting with '- '...")
            vocabulary: AppSettings.sarvamStyleRules
            maxCharacters: 500
            onVocabularySaved: text => {
                AppSettings.sarvamStyleRules = text;
            }
            onVocabularyCleared: {
                AppSettings.sarvamStyleRules = "";
            }
        }

        CustomVocabularyCard {
            title: qsTr("Custom Vocabulary")
            description: qsTr(
                             "Improve recognition of specialized Indic names, brands, acronyms, or technical terminology")
            fieldLabel: qsTr("Keywords & Names")
            placeholderText: qsTr(
                                 "Comma-separated or newline-separated keywords, e.g. Velqora, Bengaluru, Aadhaar, UPI, Tattle...")
            vocabulary: AppSettings.sarvamCustomVocabulary
            maxCharacters: 1000
            onVocabularySaved: text => {
                AppSettings.sarvamCustomVocabulary = text;
            }
            onVocabularyCleared: {
                AppSettings.sarvamCustomVocabulary = "";
            }
        }
    }
}
