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

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Theme.spacingLg

        ApiKeyConfigCard {
            title: qsTr("Meta Model API Key")
            description: qsTr("Required for Meta Musa speech recognition (dev.meta.ai)")
            rowDescription: qsTr("Enter your Meta Model API key")
            apiKey: AppSettings.metaMusaApiKey
            placeholderText: qsTr("LLM_...")
            helpText: qsTr(
                          "API keys available from the Meta AI developer console at dev.meta.ai. Stored securely on your device.")
            linkButtonText: qsTr("Get API Key")
            linkUrl: "https://dev.meta.ai/"
            onSaveRequested: key => {
                AppSettings.metaMusaApiKey = key;
            }
        }

        CustomVocabularyCard {
            title: qsTr("Custom Keywords")
            description: qsTr("Help recognition on specialized terms, abbreviations, and product names")
            fieldLabel: qsTr("Keywords")
            placeholderText: qsTr(
                                 "Comma-separated or newline-separated keywords, e.g. Tattle, Wayland, Qt6, CMake, PyTorch...")
            vocabulary: AppSettings.metaMusaKeywords
            maxCharacters: 0
            onVocabularySaved: text => {
                AppSettings.metaMusaKeywords = text;
            }
            onVocabularyCleared: {
                AppSettings.metaMusaKeywords = "";
            }
        }
    }
}
