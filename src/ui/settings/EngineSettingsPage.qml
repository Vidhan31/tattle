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
                text: qsTr("Transcription Mode")
                variant: "heading"
            }

            StyledText {
                text: qsTr("Choose how your speech is converted to text, and set up automatic offline mode")
                variant: "caption"
                colorRole: "secondary"
            }
        }

        EngineModeSwitcherCard {
            Layout.fillWidth: true
        }

        StyledCard {
            title: qsTr("More Settings")
            description: DaemonClient.activeBackend === DaemonClient.Cloud ? qsTr(
                                                                                 "Set up your cloud account, choose a provider, and configure text cleanup") :
                                                                             qsTr("Manage downloaded speech models and hardware acceleration")

            StyledButton {
                text: DaemonClient.activeBackend === DaemonClient.Cloud ? qsTr("Cloud Settings") : qsTr(
                                                                              "Offline Settings")
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
