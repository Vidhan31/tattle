pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls as QQC
import QtQuick.Layouts
import Tattle
import "controls"

T.Dialog {
    id: root

    title: qsTr("Clipboard Overwrite Notice")
    modal: true
    dim: true
    parent: T.Overlay.overlay
    width: Math.min(520, (T.Overlay.overlay ? T.Overlay.overlay.width - 32 : 520))
    height: Math.min(530, (T.Overlay.overlay ? T.Overlay.overlay.height - 40 : 530))
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

        QQC.ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            contentHeight: contentContainer.implicitHeight
            QQC.ScrollBar.horizontal.policy: QQC.ScrollBar.AlwaysOff

            Item {
                id: contentContainer
                width: scrollView.availableWidth
                implicitHeight: warningLayout.implicitHeight + (Theme.spacingMd * 2)

                ColumnLayout {
                    id: warningLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: Theme.spacingLg
                    anchors.rightMargin: Theme.spacingLg
                    anchors.topMargin: Theme.spacingMd
                    spacing: Theme.spacingMd

                    StyledText {
                        text: qsTr(
                                  "Tattle pastes transcribed speech directly into your active applications using Ctrl+Shift+V on Wayland.")
                        variant: "body"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.preferredWidth: 0
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: cardContentColumn.implicitHeight + (Theme.spacingMd * 2)
                        color: Theme.cardBgSubtle
                        border.color: Theme.cardBorder
                        border.width: 1
                        radius: Theme.radiusSm

                        ColumnLayout {
                            id: cardContentColumn
                            anchors.fill: parent
                            anchors.margins: Theme.spacingMd
                            spacing: Theme.spacingMd

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                                spacing: Theme.spacingSm

                                StyledIcon {
                                    source: "qrc:/qt/qml/Tattle/assets/icons/check.svg"
                                    color: Theme.colorSuccess
                                    size: 18
                                    Layout.alignment: Qt.AlignTop
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 0
                                    Layout.alignment: Qt.AlignTop
                                    spacing: 2

                                    StyledText {
                                        text: qsTr("Text is Preserved")
                                        customWeight: Font.DemiBold
                                        variant: "body"
                                        customColor: Theme.colorSuccess
                                        Layout.fillWidth: true
                                    }

                                    StyledText {
                                        text: qsTr(
                                                  "Copied plain text is automatically saved and restored immediately after dictation completes.")
                                        variant: "caption"
                                        colorRole: "secondary"
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 0
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 1
                                color: Theme.cardBorder
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                                spacing: Theme.spacingSm

                                StyledIcon {
                                    source: "qrc:/qt/qml/Tattle/assets/icons/info.svg"
                                    color: Theme.colorWarning
                                    size: 18
                                    Layout.alignment: Qt.AlignTop
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 0
                                    Layout.alignment: Qt.AlignTop
                                    spacing: 2

                                    StyledText {
                                        text: qsTr("Images & Media are Overwritten")
                                        customWeight: Font.DemiBold
                                        variant: "body"
                                        customColor: Theme.colorWarning
                                        Layout.fillWidth: true
                                    }

                                    StyledText {
                                        text: qsTr(
                                                  "Images, screenshots, and copied files on your clipboard cannot be restored and will be overwritten.")
                                        variant: "caption"
                                        colorRole: "secondary"
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 0
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 1
                                color: Theme.cardBorder
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                                spacing: Theme.spacingSm

                                StyledIcon {
                                    source: "qrc:/qt/qml/Tattle/assets/icons/keyboard.svg"
                                    color: Theme.accentColor
                                    size: 18
                                    Layout.alignment: Qt.AlignTop
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 0
                                    Layout.alignment: Qt.AlignTop
                                    spacing: 2

                                    StyledText {
                                        text: qsTr("Ctrl+Shift+V App Compatibility")
                                        customWeight: Font.DemiBold
                                        variant: "body"
                                        customColor: Theme.accentColor
                                        Layout.fillWidth: true
                                    }

                                    StyledText {
                                        text: qsTr(
                                                  "Text is pasted by simulating the Ctrl+Shift+V shortcut (standard for terminals, code editors, and plain-text pasting). Please ensure your active application supports Ctrl+Shift+V for pasting.")
                                        variant: "caption"
                                        colorRole: "secondary"
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 0
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                implicitHeight: 1
                                color: Theme.cardBorder
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                                spacing: Theme.spacingSm

                                StyledIcon {
                                    source: "qrc:/qt/qml/Tattle/assets/icons/copy.svg"
                                    color: Theme.accentColor
                                    size: 18
                                    Layout.alignment: Qt.AlignTop
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 0
                                    Layout.alignment: Qt.AlignTop
                                    spacing: 2

                                    StyledText {
                                        text: qsTr("Install a Clipboard Manager")
                                        customWeight: Font.DemiBold
                                        variant: "body"
                                        customColor: Theme.accentColor
                                        Layout.fillWidth: true
                                    }

                                    StyledText {
                                        text: qsTr(
                                                  "To avoid losing copied images, screenshots, or files during transcription, consider installing or enabling a clipboard manager (such as Clipboard Indicator or GPaste on GNOME, Klipper on KDE, or cliphist on wlroots).")
                                        variant: "caption"
                                        colorRole: "secondary"
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                        Layout.preferredWidth: 0
                                    }
                                }
                            }
                        }
                    }

                    StyledText {
                        text: qsTr(
                                  "Before dictating, please paste and save any important copied media files, or install a desktop clipboard manager extension.")
                        variant: "caption"
                        colorRole: "secondary"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.preferredWidth: 0
                    }
                }
            }
        }

        StyledDivider {
            Layout.fillWidth: true
            dividerColor: Theme.cardBorder
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingLg
                anchors.rightMargin: Theme.spacingLg

                Item {
                    Layout.fillWidth: true
                }

                StyledButton {
                    id: understandBtn
                    text: qsTr("I Understand")
                    variant: "primary"
                    size: "medium"
                    onClicked: {
                        DaemonDiagnosticModel.clipboardWarningAcknowledged = true;
                        root.accept();
                    }
                }
            }
        }
    }
}
