import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tattle

Item {
    id: root

    implicitHeight: contentColumn.implicitHeight

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        spacing: Theme.spacingMd

        ColumnLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: Theme.spacingSm
            spacing: 4

            StyledText {
                text: qsTr("Notifications")
                variant: "heading"
            }

            StyledText {
                text: qsTr("Configure system notifications for transcription, downloads, hardware, and engine failover")
                variant: "caption"
                colorRole: "secondary"
            }
        }

        StyledCard {
            title: qsTr("Desktop Notifications")
            description: qsTr("Master switch to enable or disable all system notifications")

            PreferenceSwitch {
                id: masterNotificationSwitch
                title: qsTr("Enable Desktop Notifications")
                description: qsTr("Show system notifications via desktop notification daemon (D-Bus)")
                checked: AppSettings.notificationsEnabled
                onToggled: {
                    AppSettings.notificationsEnabled = masterNotificationSwitch.checked;
                }
            }
        }

        StyledCard {
            title: qsTr("Notification Categories")
            description: qsTr("Control which events deliver desktop notifications")
            enabled: AppSettings.notificationsEnabled
            opacity: AppSettings.notificationsEnabled ? 1.0 : 0.6

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                PreferenceSwitch {
                    id: transcriptionSwitch
                    title: qsTr("Dictation & Transcription Alerts")
                    description: qsTr(
                                     "Notify on transcription errors, max recording duration limit (5m), and LLM formatting fallbacks")
                    checked: AppSettings.notificationsTranscriptionEnabled
                    onToggled: {
                        AppSettings.notificationsTranscriptionEnabled = transcriptionSwitch.checked;
                    }
                }

                StyledDivider {}

                PreferenceSwitch {
                    id: modelSwitch
                    title: qsTr("Offline Speech Model Downloads")
                    description: qsTr("Notify when speech model downloads complete, fail, or encounter load errors")
                    checked: AppSettings.notificationsModelEnabled
                    onToggled: {
                        AppSettings.notificationsModelEnabled = modelSwitch.checked;
                    }
                }

                StyledDivider {}

                PreferenceSwitch {
                    id: audioSwitch
                    title: qsTr("Audio & Microphone Alerts")
                    description: qsTr("Notify when microphones are disconnected, switched to fallback, or unavailable")
                    checked: AppSettings.notificationsAudioEnabled
                    onToggled: {
                        AppSettings.notificationsAudioEnabled = audioSwitch.checked;
                    }
                }

                StyledDivider {}

                PreferenceSwitch {
                    id: failoverSwitch
                    title: qsTr("Engine & Network Failover")
                    description: qsTr(
                                     "Notify when auto-switching between Cloud and Offline engines due to internet state")
                    checked: AppSettings.notificationsFailoverEnabled
                    onToggled: {
                        AppSettings.notificationsFailoverEnabled = failoverSwitch.checked;
                    }
                }

                StyledDivider {}

                PreferenceSwitch {
                    id: systemSwitch
                    title: qsTr("System Service & Daemon Alerts")
                    description: qsTr("Notify if the background key injection daemon stops or encounters fatal errors")
                    checked: AppSettings.notificationsSystemEnabled
                    onToggled: {
                        AppSettings.notificationsSystemEnabled = systemSwitch.checked;
                    }
                }
            }
        }
    }
}
