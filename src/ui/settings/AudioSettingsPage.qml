pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
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
                text: qsTr("Audio & Recording")
                variant: "heading"
            }

            StyledText {
                text: qsTr("Configure microphone input, recording mode, and feedback sounds")
                variant: "caption"
                colorRole: "secondary"
            }
        }

        StyledCard {
            title: qsTr("Microphone Test")
            description: qsTr("Check microphone input level and audio capture")

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMd

                    StyledText {
                        text: qsTr("Input Level:")
                        variant: "body"
                        customWeight: Font.Medium
                    }

                    T.ProgressBar {
                        Layout.fillWidth: true
                        implicitHeight: 8
                        from: 0.0
                        to: 1.0
                        value: Math.min(1.0, Math.max(0.0, DaemonClient.microphoneTestLevel))

                        background: Rectangle {
                            radius: Theme.radiusCircle
                            color: Theme.controlBg
                            border.color: Theme.controlBorder
                            border.width: 1
                        }

                        contentItem: Item {
                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: parent.width * Math.min(1.0, Math.max(0.0, DaemonClient.microphoneTestLevel))
                                radius: Theme.radiusCircle

                                color: {
                                    if (DaemonClient.microphoneTestLevel > 0.85)
                                    return Theme.colorDanger;
                                    if (DaemonClient.microphoneTestLevel > 0.6)
                                    return Theme.colorWarning;
                                    return Theme.colorSuccess;
                                }

                                Behavior on width {
                                    NumberAnimation {
                                        duration: 60
                                        easing.type: Easing.OutQuad
                                    }
                                }
                            }
                        }
                    }

                    StyledText {
                        text: qsTr("%1%").arg(Math.round(DaemonClient.microphoneTestLevel * 100))
                        variant: "caption"
                        customWeight: Font.Bold
                        Layout.preferredWidth: 36
                        horizontalAlignment: Text.AlignRight
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    StyledText {
                        text: DaemonClient.microphoneTestActive ? qsTr("Recording active — speak into microphone") :
                                                                  qsTr("Microphone idle")
                        variant: "caption"
                        customColor: DaemonClient.microphoneTestActive ? Theme.colorSuccess : Theme.textSecondary
                        Layout.fillWidth: true
                    }

                    StyledButton {
                        id: testMicBtn
                        text: DaemonClient.microphoneTestActive ? qsTr("Stop Test") : qsTr("Test Microphone")
                        enabled: !DaemonClient.isBusy || DaemonClient.microphoneTestActive
                        size: "small"
                        variant: DaemonClient.microphoneTestActive ? "danger" : "secondary"
                        onClicked: {
                            if (DaemonClient.microphoneTestActive) {
                                DaemonClient.stopMicrophoneTest();
                            } else {
                                DaemonClient.startMicrophoneTest();
                            }
                        }
                    }
                }
            }
        }

        StyledCard {
            title: qsTr("Recording Mode")
            description: qsTr("Choose how global shortcuts and recording triggers activate speech transcription")

            StatusBanner {
                visible: !DaemonClient.pushToTalkSupported
                bannerType: "warning"
                title: qsTr("Push-to-Talk Unavailable on Current Desktop")
                message: qsTr(
                             "Push-to-Talk requires key press and release tracking via the XDG Global Shortcuts portal (org.freedesktop.portal.GlobalShortcuts). Your desktop environment does not support this portal, so dictation is restricted to Toggle mode.")
            }

            GridLayout {
                Layout.fillWidth: true
                columns: root.width >= 560 ? 2 : 1
                columnSpacing: Theme.spacingMd
                rowSpacing: Theme.spacingMd

                Rectangle {
                    id: toggleModeCard
                    Layout.fillWidth: true
                    Layout.preferredHeight: toggleContent.implicitHeight + (Theme.spacingMd * 2)
                    radius: Theme.radiusMd
                    color: isSelected ? Theme.selectedBg : (toggleMouse.containsMouse ? Theme.cardBgElevated :
                                                                                        Theme.cardBgSubtle)

                    border.color: isSelected ? Theme.accentColor : (toggleMouse.containsMouse ? Theme.cardBorderHover :
                                                                                                Theme.cardBorder)
                    border.width: isSelected ? 2 : 1

                    readonly property bool isSelected: DaemonClient.recordingMode === DaemonClient.Toggle

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
                        id: toggleContent
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
                                color: toggleModeCard.isSelected ? Theme.sidebarItemSelected : Theme.controlBg

                                StyledIcon {
                                    anchors.centerIn: parent
                                    source: "qrc:/qt/qml/Tattle/assets/icons/speech.svg"
                                    size: 16
                                    color: toggleModeCard.isSelected ? Theme.accentColor : Theme.textSecondary
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1

                                StyledText {
                                    text: qsTr("Toggle Mode")
                                    variant: "body"
                                    customWeight: Font.DemiBold
                                }

                                StyledText {
                                    text: qsTr("Press to start / stop")
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
                                text: qsTr("• Press shortcut or button once to start recording")
                                variant: "caption"
                                colorRole: "secondary"
                            }

                            StyledText {
                                text: qsTr("• Press again to stop recording and transcribe")
                                variant: "caption"
                                colorRole: "secondary"
                            }

                            StyledText {
                                text: qsTr("• Compatible with all desktop environments")
                                variant: "caption"
                                colorRole: "secondary"
                            }
                        }
                    }

                    MouseArea {
                        id: toggleMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            DaemonClient.recordingMode = DaemonClient.Toggle;
                        }
                    }
                }

                Rectangle {
                    id: pttModeCard
                    Layout.fillWidth: true
                    Layout.preferredHeight: pttContent.implicitHeight + (Theme.spacingMd * 2)
                    radius: Theme.radiusMd
                    opacity: DaemonClient.pushToTalkSupported ? 1.0 : 0.6
                    color: isSelected ? Theme.selectedBg : (pttMouse.containsMouse && DaemonClient.pushToTalkSupported
                                                            ? Theme.cardBgElevated : Theme.cardBgSubtle)
                    border.color: isSelected ? Theme.accentColor : (pttMouse.containsMouse
                                                                    && DaemonClient.pushToTalkSupported
                                                                    ? Theme.cardBorderHover : Theme.cardBorder)
                    border.width: isSelected ? 2 : 1

                    readonly property bool isSelected: DaemonClient.recordingMode === DaemonClient.PushToTalk

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
                        id: pttContent
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
                                color: pttModeCard.isSelected ? Theme.sidebarItemSelected : Theme.controlBg

                                StyledIcon {
                                    anchors.centerIn: parent
                                    source: "qrc:/qt/qml/Tattle/assets/icons/mic.svg"
                                    size: 16
                                    color: pttModeCard.isSelected ? Theme.accentColor : Theme.textSecondary
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1

                                StyledText {
                                    text: qsTr("Push-to-Talk")
                                    variant: "body"
                                    customWeight: Font.DemiBold
                                }

                                StyledText {
                                    text: qsTr("Hold shortcut to speak")
                                    variant: "caption"
                                    colorRole: "secondary"
                                }
                            }

                            StateBadge {
                                text: qsTr("Unavailable")
                                statusType: "warning"
                                visible: !DaemonClient.pushToTalkSupported
                                showDot: false
                            }
                        }

                        StyledDivider {
                            dividerColor: Theme.cardBorder
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            StyledText {
                                text: qsTr("• Hold shortcut down while speaking")
                                variant: "caption"
                                colorRole: "secondary"
                            }

                            StyledText {
                                text: qsTr("• Release shortcut to immediately transcribe")
                                variant: "caption"
                                colorRole: "secondary"
                            }

                            StyledText {
                                text: qsTr("• Requires XDG Global Shortcuts portal support")
                                variant: "caption"
                                colorRole: "secondary"
                            }
                        }
                    }

                    MouseArea {
                        id: pttMouse
                        anchors.fill: parent
                        hoverEnabled: DaemonClient.pushToTalkSupported
                        cursorShape: DaemonClient.pushToTalkSupported ? Qt.PointingHandCursor : Qt.ArrowCursor
                        enabled: DaemonClient.pushToTalkSupported
                        onClicked: {
                            if (DaemonClient.pushToTalkSupported) {
                                DaemonClient.recordingMode = DaemonClient.PushToTalk;
                            }
                        }
                    }
                }
            }
        }

        StyledCard {
            title: qsTr("Sound Effects")
            description: qsTr("Play sounds when dictation starts and stops")

            PreferenceSwitch {
                id: soundSwitch
                title: qsTr("Feedback Sounds")
                description: qsTr("Play sound effects on recording start and completion")
                checked: AppSettings.soundEnabled
                onToggled: {
                    AppSettings.soundEnabled = soundSwitch.checked;
                }
            }

            StyledDivider {}

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm
                opacity: AppSettings.soundEnabled ? 1.0 : 0.5
                enabled: AppSettings.soundEnabled

                StyledText {
                    text: qsTr("Preview:")
                    variant: "caption"
                    colorRole: "secondary"
                }

                Item {
                    Layout.fillWidth: true
                }

                StyledButton {
                    id: playStartBtn
                    text: qsTr("Play Start Sound")
                    size: "small"
                    onClicked: DaemonClient.playStartSound()
                }

                StyledButton {
                    id: playStopBtn
                    text: qsTr("Play Stop Sound")
                    size: "small"
                    onClicked: DaemonClient.playStopSound()
                }
            }
        }
    }
}
