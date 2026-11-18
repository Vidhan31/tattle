pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Tattle
import "../controls"

StyledCard {
    id: root

    title: qsTr("Transcription Mode")
    description: qsTr("Choose between fast cloud transcription and private on-device processing")

    GridLayout {
        Layout.fillWidth: true
        columns: root.width >= 560 ? 2 : 1
        columnSpacing: Theme.spacingMd
        rowSpacing: Theme.spacingMd

        Rectangle {
            id: cloudOptionCard
            Layout.fillWidth: true
            Layout.preferredHeight: cloudContent.implicitHeight + (Theme.spacingMd * 2)
            radius: Theme.radiusMd
            color: isSelected ? Theme.selectedBg : (cloudMouse.containsMouse ? Theme.cardBgElevated :
                                                                               Theme.cardBgSubtle)

            border.color: isSelected ? Theme.accentColor : (cloudMouse.containsMouse ? Theme.cardBorderHover :
                                                                                       Theme.cardBorder)
            border.width: isSelected ? 2 : 1

            readonly property bool isSelected: DaemonClient.activeBackend === DaemonClient.Cloud

            Behavior on color {
                ColorAnimation {
                    duration: Theme.animFast
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on border.color {
                ColorAnimation {
                    duration: Theme.animFast
                    easing.type: Easing.OutCubic
                }
            }

            ColumnLayout {
                id: cloudContent
                anchors.fill: parent
                anchors.margins: Theme.spacingMd
                spacing: Theme.spacingSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    Rectangle {
                        implicitWidth: 32
                        implicitHeight: 32
                        radius: Theme.radiusSm
                        color: cloudOptionCard.isSelected ? Theme.sidebarItemSelected : Theme.controlBg

                        StyledIcon {
                            anchors.centerIn: parent
                            source: "qrc:/qt/qml/Tattle/assets/icons/sparkles.svg"
                            size: 16
                            color: cloudOptionCard.isSelected ? Theme.accentColor : Theme.textSecondary
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        StyledText {
                            text: qsTr("Cloud Transcription")
                            variant: "body"
                            customWeight: Font.DemiBold
                        }

                        StyledText {
                            text: qsTr("%1 Transcription").arg(CloudProviderModel.activeProviderName)
                            variant: "caption"
                            colorRole: "secondary"
                        }
                    }
                }

                StyledDivider {
                    dividerColor: Theme.cardBorder
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    StyledText {
                        text: qsTr("• Ultra-fast cloud speech recognition (<1s)")
                        variant: "caption"
                        colorRole: "secondary"
                    }

                    StyledText {
                        text: qsTr("• Text cleanup & smart formatting")
                        variant: "caption"
                        colorRole: "secondary"
                    }

                    StyledText {
                        text: qsTr("• Multiple provider options (Groq & Gemini)")
                        variant: "caption"
                        colorRole: "secondary"
                    }
                }
            }

            MouseArea {
                id: cloudMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (DaemonClient.activeBackend !== DaemonClient.Cloud) {
                        DaemonClient.activeBackend = DaemonClient.Cloud;
                    }
                }
            }
        }

        Rectangle {
            id: offlineOptionCard
            Layout.fillWidth: true
            Layout.preferredHeight: offlineContent.implicitHeight + (Theme.spacingMd * 2)
            radius: Theme.radiusMd
            color: isSelected ? Theme.selectedBg : (offlineMouse.containsMouse ? Theme.cardBgElevated :
                                                                                 Theme.cardBgSubtle)

            border.color: isSelected ? Theme.accentColor : (offlineMouse.containsMouse ? Theme.cardBorderHover :
                                                                                         Theme.cardBorder)
            border.width: isSelected ? 2 : 1

            readonly property bool isSelected: DaemonClient.activeBackend === DaemonClient.TranscribeCpp

            Behavior on color {
                ColorAnimation {
                    duration: Theme.animFast
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on border.color {
                ColorAnimation {
                    duration: Theme.animFast
                    easing.type: Easing.OutCubic
                }
            }

            ColumnLayout {
                id: offlineContent
                anchors.fill: parent
                anchors.margins: Theme.spacingMd
                spacing: Theme.spacingSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    Rectangle {
                        implicitWidth: 32
                        implicitHeight: 32
                        radius: Theme.radiusSm
                        color: offlineOptionCard.isSelected ? Theme.sidebarItemSelected : Theme.controlBg

                        StyledIcon {
                            anchors.centerIn: parent
                            source: "qrc:/qt/qml/Tattle/assets/icons/bolt.svg"
                            size: 16
                            color: offlineOptionCard.isSelected ? Theme.accentColor : Theme.textSecondary
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1

                        StyledText {
                            text: qsTr("Offline Transcription")
                            variant: "body"
                            customWeight: Font.DemiBold
                        }

                        StyledText {
                            text: qsTr("Local Speech Model")
                            variant: "caption"
                            colorRole: "secondary"
                        }
                    }
                }

                StyledDivider {
                    dividerColor: Theme.cardBorder
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    StyledText {
                        text: qsTr("• 100% private on-device transcription")
                        variant: "caption"
                        colorRole: "secondary"
                    }

                    StyledText {
                        text: qsTr("• Zero internet connection needed")
                        variant: "caption"
                        colorRole: "secondary"
                    }

                    StyledText {
                        text: qsTr("• Uses downloaded speech models")
                        variant: "caption"
                        colorRole: "secondary"
                    }
                }
            }

            MouseArea {
                id: offlineMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (DaemonClient.activeBackend !== DaemonClient.TranscribeCpp) {
                        DaemonClient.activeBackend = DaemonClient.TranscribeCpp;
                    }
                }
            }
        }
    }

    StyledDivider {
        Layout.fillWidth: true
        dividerColor: Theme.cardBorder
    }

    PreferenceSwitch {
        Layout.fillWidth: true
        title: qsTr("Automatic Offline Switching")
        description: qsTr("Automatically switch to offline speech model when internet connection is lost")
        checked: DaemonClient.autoSwitchOffline
        onToggled: DaemonClient.autoSwitchOffline = checked
    }
}
