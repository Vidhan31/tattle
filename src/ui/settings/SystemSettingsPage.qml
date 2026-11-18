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

    ListModel {
        id: themeOptionsModel
        ListElement {
            name: "System (Auto)"
            code: "system"
        }
        ListElement {
            name: "Dark Theme"
            code: "dark"
        }
        ListElement {
            name: "Light Theme"
            code: "light"
        }
    }

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
                text: qsTr("System & Privacy")
                variant: "heading"
            }

            StyledText {
                text: qsTr("Configure appearance, text pasting, and clipboard privacy")
                variant: "caption"
                colorRole: "secondary"
            }
        }

        StyledCard {
            title: qsTr("Appearance")
            description: qsTr("Choose your preferred theme or follow your desktop environment")

            PreferenceRow {
                title: qsTr("Theme")
                description: qsTr("System detected: %1 mode").arg(Theme.systemThemeName)
                layoutVertical: false

                StyledComboBox {
                    id: themeCombo
                    Layout.preferredWidth: 180
                    textRole: "name"
                    valueRole: "code"
                    model: themeOptionsModel
                    currentIndex: {
                        const idx = indexOfValue(Theme.themeMode);
                        return idx >= 0 ? idx : 0;
                    }

                    onActivated: index => {
                        Theme.themeMode = currentValue;
                    }
                }
            }
        }

        StyledCard {
            title: qsTr("Direct Text Pasting")
            description: qsTr("Pastes transcribed text directly into the active application")

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        StyledText {
                            text: qsTr("Service Status")
                            variant: "body"
                            customWeight: Font.Medium
                        }

                        StyledText {
                            text: DaemonDiagnosticModel.hasFatalError ? (DaemonDiagnosticModel.fatalErrorMessage.length
                                                                         > 0 ? DaemonDiagnosticModel.fatalErrorMessage :
                                                                               qsTr("Service failed to start")) : (
                                                                            DaemonDiagnosticModel.connected ? qsTr(
                                                                                                                  "Connected to background text paste service") :
                                                                                                              (DaemonDiagnosticModel.statusMessage.length
                                                                                                               > 0 ? DaemonDiagnosticModel.statusMessage :
                                                                                                                     qsTr("Not running — automatic paste unavailable")))
                            variant: "caption"
                            customColor: DaemonDiagnosticModel.connected ? Theme.colorSuccess : (
                                                                               DaemonDiagnosticModel.hasFatalError
                                                                               ? Theme.colorDanger :
                                                                                 Theme.textSecondary)
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                StyledDivider {}

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    Item {
                        Layout.fillWidth: true
                    }

                    StyledButton {
                        id: restartServiceBtn
                        text: DaemonDiagnosticModel.hasFatalError ? qsTr("Restart Service") : (
                                                                        DaemonDiagnosticModel.connected ? qsTr(
                                                                                                              "Restart Service") :
                                                                                                          qsTr("Connect Service"))
                        variant: "primary"
                        size: "small"
                        onClicked: {
                            DaemonDiagnosticModel.restartService();
                        }
                    }

                    StyledButton {
                        id: stopServiceBtn
                        text: qsTr("Stop Service")
                        enabled: DaemonDiagnosticModel.connected
                        variant: "danger"
                        size: "small"
                        onClicked: {
                            DaemonDiagnosticModel.stopDaemon();
                        }
                    }
                }

                StyledDivider {}

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingMd

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            StyledText {
                                text: qsTr("Pre-Paste Delay")
                                variant: "body"
                                customWeight: Font.Medium
                            }

                            StyledText {
                                text: qsTr("Wait before pasting text into the active application")
                                variant: "caption"
                                colorRole: "secondary"
                            }
                        }

                        StyledText {
                            text: DaemonDiagnosticModel.injectionDelay === 0 ? qsTr("0 ms (Immediate)") : qsTr(
                                                                                   "%1 ms").arg(
                                                                                   DaemonDiagnosticModel.injectionDelay)

                            variant: "body"
                            customWeight: Font.Bold
                            colorRole: "accent"
                            Layout.preferredWidth: 120
                            horizontalAlignment: Text.AlignRight
                        }

                        StyledButton {
                            id: resetDelayBtn
                            text: qsTr("Reset")
                            variant: "flat"
                            size: "small"
                            enabled: DaemonDiagnosticModel.injectionDelay !== 200
                            onClicked: {
                                DaemonDiagnosticModel.injectionDelay = 200;
                            }
                        }
                    }

                    StyledSlider {
                        id: delaySlider
                        Layout.fillWidth: true
                        from: 0
                        to: 2000
                        stepSize: 50
                        value: DaemonDiagnosticModel.injectionDelay
                        onMoved: {
                            DaemonDiagnosticModel.injectionDelay = Math.round(delaySlider.value);
                        }
                    }
                }

                StyledDivider {}

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSm

                    StyledIcon {
                        source: "qrc:/qt/qml/Tattle/assets/icons/keyboard.svg"
                        color: Theme.accentColor
                        size: 16
                        Layout.alignment: Qt.AlignVCenter
                    }

                    StyledText {
                        text: qsTr(
                                  "Pastes text using <b>Ctrl+Shift+V</b> keystrokes. Ensure target applications accept Ctrl+Shift+V for pasting.")
                        textFormat: Text.StyledText
                        variant: "caption"
                        colorRole: "secondary"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }
        }

        StyledCard {
            title: qsTr("Clipboard Privacy & Behavior")
            description: qsTr("Control clipboard privacy and text injection behavior on Wayland")

            PreferenceSwitch {
                id: clipboardHistorySwitch
                title: qsTr("Exclude from Clipboard History")
                description: qsTr(
                                 "Prevents clipboard managers (KDE Klipper, GNOME GPaste) from storing your dictated text")
                checked: DaemonDiagnosticModel.preventClipboardHistory
                onToggled: {
                    DaemonDiagnosticModel.preventClipboardHistory = clipboardHistorySwitch.checked;
                }
            }

            StyledDivider {
                visible: DaemonDiagnosticModel.isDevBuild || !DaemonDiagnosticModel.isKde
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm
                visible: DaemonDiagnosticModel.isDevBuild || !DaemonDiagnosticModel.isKde

                StyledText {
                    text: qsTr("Wayland Clipboard Guidance")
                    variant: "body"
                    customWeight: Font.Medium
                }

                StyledText {
                    text: qsTr(
                              "On desktop environments without integrated clipboard history (like GNOME or wlroots compositors), dictation injects text by simulating Ctrl+Shift+V clipboard paste. Tattle automatically restores copied plain text, but non-text clipboard items (such as images, screenshots, or files) cannot be restored and will be lost unless you paste and save them first.\n\nWe strongly recommend installing or enabling a clipboard manager (such as Klipper on KDE Plasma, Clipboard Indicator or GPaste on GNOME, or cliphist on wlroots/Hyprland) to preserve your media clipboard history. Please also ensure your active applications accept Ctrl+Shift+V for pasting.")
                    variant: "caption"
                    colorRole: "secondary"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }
    }
}
