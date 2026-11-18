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
                text: qsTr("Shortcuts & Hotkeys")
                variant: "heading"
            }

            StyledText {
                text: qsTr("Configure system-wide keyboard shortcuts and desktop hotkeys")
                variant: "caption"
                colorRole: "secondary"
            }
        }

        StyledCard {
            title: qsTr("Global Shortcut")
            description: qsTr("System-wide keyboard shortcut to start and stop dictation")

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
                            text: qsTr("Shortcut Status")
                            variant: "body"
                            customWeight: Font.Medium
                        }

                        StyledText {
                            text: DaemonClient.shortcutStatusMessage.length > 0 ? DaemonClient.shortcutStatusMessage :
                                                                                  qsTr("Registered via Desktop Portal (Ctrl+Shift+Space)")
                            variant: "caption"
                            customColor: DaemonClient.shortcutAvailable ? Theme.colorSuccess : (
                                                                              DaemonClient.shortcutSupported
                                                                              ? Theme.colorWarning :
                                                                                Theme.textSecondary)
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                StyledDivider {}

                StyledCard {
                    Layout.fillWidth: true
                    variant: "subtle"
                    padding: Theme.spacingMd

                    ColumnLayout {
                        id: customShortcutColumn
                        Layout.fillWidth: true
                        spacing: Theme.spacingSm

                        StyledText {
                            text: qsTr("Desktop Environment Custom Shortcut")
                            variant: "caption"
                            customWeight: Font.DemiBold
                        }

                        StyledText {
                            text: qsTr(
                                      "If your desktop does not support portal shortcuts (e.g. GNOME, Sway), bind this command to <b>Ctrl+Shift+Space</b> in your desktop's keyboard settings:")
                            textFormat: Text.StyledText
                            variant: "caption"
                            colorRole: "secondary"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spacingSm

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 32
                                color: Theme.inputBg
                                border.color: Theme.inputBorder
                                border.width: 1
                                radius: Theme.radiusSm

                                StyledText {
                                    anchors.centerIn: parent
                                    text: "tattle --toggle"
                                    fontFamily: "mono"
                                    variant: "caption"
                                    customWeight: Font.Bold
                                    colorRole: "accent"
                                }
                            }

                            StyledButton {
                                id: copyCliBtn
                                text: qsTr("Copy")
                                iconSource: "qrc:/qt/qml/Tattle/assets/icons/copy.svg"
                                size: "small"
                                onClicked: {
                                    DictationPadModel.copyTextToClipboard("tattle --toggle");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
