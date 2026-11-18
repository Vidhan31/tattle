pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Tattle
import "../controls"

Item {
    id: root

    signal navigateRequested(string target)

    implicitWidth: 500
    implicitHeight: contentColumn.implicitHeight

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Theme.spacingLg

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingXs
            Layout.bottomMargin: Theme.spacingSm
            spacing: 4

            StyledText {
                text: qsTr("Transcription Engine")
                variant: "heading"
            }

            StyledText {
                text: qsTr("Select your active speech recognition backend and configure automatic offline fallback")
                variant: "caption"
                colorRole: "secondary"
            }
        }

        EngineModeSwitcherCard {
            Layout.fillWidth: true
        }

        StyledCard {
            title: qsTr("Detailed Configuration")
            description: DaemonClient.activeBackend === DaemonClient.Cloud ? qsTr(
                                                                                 "Configure API credentials, cloud providers, and LLM text enhancement") :
                                                                             qsTr("Manage on-device Whisper models, downloads, and hardware acceleration")

            StyledButton {
                text: DaemonClient.activeBackend === DaemonClient.Cloud ? qsTr("Configure Cloud Transcription") : qsTr(
                                                                              "Configure Offline Transcription")
                iconSource: DaemonClient.activeBackend === DaemonClient.Cloud
                            ? "qrc:/qt/qml/Tattle/assets/icons/cloud.svg" : "qrc:/qt/qml/Tattle/assets/icons/cpu.svg"
                variant: "secondary"
                Layout.fillWidth: true
                onClicked: root.navigateRequested(DaemonClient.activeBackend === DaemonClient.Cloud ? "cloud" :
                                                                                                      "offline")
            }
        }
    }
}
