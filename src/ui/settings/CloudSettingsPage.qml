pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Tattle
import "../controls"

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

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingXs
            Layout.bottomMargin: Theme.spacingSm
            spacing: 4

            StyledText {
                text: qsTr("Cloud Transcription")
                variant: "heading"
            }

            StyledText {
                text: qsTr("Configure cloud speech recognition providers, API keys, and text cleanup")
                variant: "caption"
                colorRole: "secondary"
            }
        }

        StyledCard {
            title: qsTr("Cloud Provider")
            description: qsTr("Choose the cloud provider for speech transcription and text cleanup")

            PreferenceRow {
                title: qsTr("Active Provider")
                description: CloudProviderModel.activeProviderDescription
                layoutVertical: false

                StyledComboBox {
                    id: providerCombo
                    Layout.preferredWidth: 220
                    textRole: "name"
                    valueRole: "providerId"
                    model: CloudProviderModel
                    currentIndex: CloudProviderModel.activeProviderIndex

                    onActivated: index => {
                        CloudProviderModel.activeProviderIndex = index;
                    }
                }
            }
        }

        Loader {
            id: providerSettingsLoader
            Layout.fillWidth: true
            source: CloudProviderModel.activeProviderComponent
        }
    }
}
