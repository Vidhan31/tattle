pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import Tattle
import "controls"

T.Dialog {
    id: root

    title: qsTr("Setup Custom Desktop Shortcut")
    modal: true
    dim: true
    parent: T.Overlay.overlay
    width: Math.min(500, (T.Overlay.overlay ? T.Overlay.overlay.width - 32 : 500))
    height: Math.min(440, (T.Overlay.overlay ? T.Overlay.overlay.height - 40 : 440))
    x: Math.round(((T.Overlay.overlay ? T.Overlay.overlay.width : 960) - width) / 2)
    y: Math.round(((T.Overlay.overlay ? T.Overlay.overlay.height : 660) - height) / 2)
    closePolicy: T.Popup.CloseOnEscape | T.Popup.CloseOnPressOutside
    padding: 0
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    background: Rectangle {
        color: Theme.cardBgElevated
        border.color: Theme.cardBorder
        border.width: 1
        radius: Theme.radiusMd
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingLg
                anchors.rightMargin: Theme.spacingMd

                StyledText {
                    text: root.title
                    variant: "subheading"
                    customWeight: Font.Bold
                    Layout.fillWidth: true
                }

                StyledButton {
                    text: qsTr("✕")
                    variant: "ghost"
                    size: "small"
                    onClicked: root.reject()
                }
            }
        }

        StyledDivider {
            Layout.fillWidth: true
            dividerColor: Theme.cardBorder
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.spacingLg
            spacing: Theme.spacingMd

            StyledText {
                text: qsTr(
                          "Your desktop environment does not support the XDG Global Shortcuts portal directly, but you can easily set up a system-wide hotkey using this command:")
                variant: "body"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 42
                color: Theme.inputBg
                border.color: Theme.inputBorder
                border.width: 1
                radius: Theme.radiusSm

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingSm

                    StyledText {
                        text: "tattle --toggle"
                        fontFamily: "mono"
                        customWeight: Font.Bold
                        colorRole: "accent"
                        Layout.fillWidth: true
                    }

                    StyledButton {
                        id: copyCmdBtn
                        text: qsTr("Copy Command")
                        iconSource: "qrc:/qt/qml/Tattle/assets/icons/copy.svg"
                        variant: "flat"
                        size: "small"
                        onClicked: {
                            TranscriptionModel.copyToClipboard("tattle --toggle");
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                StyledText {
                    text: qsTr("Steps for GNOME:")
                    customWeight: Font.DemiBold
                    variant: "body"
                }

                StyledText {
                    text: qsTr(
                              "1. Open <b>Settings → Keyboard → View and Customize Shortcuts</b>.<br>2. Scroll down to <b>Custom Shortcuts</b> and click <b>+</b>.<br>3. Set Name to <i>Tattle</i>, Command to <code>tattle --toggle</code>, and Shortcut to <b>Ctrl+Shift+Space</b>.")
                    textFormat: Text.StyledText
                    variant: "caption"
                    colorRole: "secondary"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            StyledText {
                text: qsTr(
                          "<b>Note:</b> Custom desktop shortcuts operate in <b>Toggle mode</b>. Push-to-Talk requires desktop support for the <code>org.freedesktop.portal.GlobalShortcuts</code> portal (available on KDE Plasma 6+ and compatible Wayland compositors).")
                textFormat: Text.StyledText
                variant: "caption"
                colorRole: "secondary"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
}
