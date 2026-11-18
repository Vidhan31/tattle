pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import Tattle
import "controls"

Item {
    id: root

    signal navigateRequested(string target)

    implicitWidth: 620
    implicitHeight: 540

    property bool showCopySuccess: false

    Timer {
        id: copyFeedbackTimer
        interval: 2000
        repeat: false
        onTriggered: {
            root.showCopySuccess = false;
        }
    }

    ShortcutGuideDialog {
        id: shortcutGuideDialog
    }

    ClipboardWarningDialog {
        id: clipboardWarningDialog
        onAccepted: {
            DaemonClient.startRecording();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingMd

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingXs
            Layout.bottomMargin: Theme.spacingSm
            spacing: 4

            StyledText {
                text: qsTr("Dictate")
                variant: "heading"
            }

            StyledText {
                text: qsTr("Real-time voice dictation and transcription pad")
                variant: "caption"
                colorRole: "secondary"
            }
        }

        SystemDiagnosticsColumn {
            Layout.fillWidth: true
            onNavigateRequested: target => root.navigateRequested(target)
            onClipboardWarningRequested: clipboardWarningDialog.open()
        }

        RecordingControl {
            Layout.fillWidth: true
            onClipboardWarningRequested: clipboardWarningDialog.open()
            onShortcutGuideRequested: shortcutGuideDialog.open()
        }

        StyledCard {
            id: dictationPadCard
            Layout.fillWidth: true
            Layout.fillHeight: true
            customRadius: Theme.radiusMd
            padding: 0
            clip: true

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 42
                    color: Theme.cardBgElevated

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMd
                        anchors.rightMargin: Theme.spacingMd
                        spacing: Theme.spacingMd

                        RowLayout {
                            spacing: Theme.spacingSm
                            Layout.alignment: Qt.AlignVCenter

                            StyledIcon {
                                source: "qrc:/qt/qml/Tattle/assets/icons/speech.svg"
                                size: 16
                                color: Theme.accentColor
                                Layout.alignment: Qt.AlignVCenter
                            }

                            StyledText {
                                text: qsTr("Dictation Pad")
                                variant: "body"
                                customWeight: Font.DemiBold
                                Layout.alignment: Qt.AlignVCenter
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                        }

                        StyledText {
                            text: qsTr("%1 words • %2 characters").arg(DictationPadModel.wordCount).arg(
                                      DictationPadModel.charCount)
                            variant: "caption"
                            colorRole: "secondary"
                            Layout.alignment: Qt.AlignVCenter
                        }

                        StyledButton {
                            id: copyPadBtn
                            text: root.showCopySuccess ? qsTr("Copied!") : qsTr("Copy")
                            iconSource: root.showCopySuccess ? "qrc:/qt/qml/Tattle/assets/icons/check.svg" :
                                                               "qrc:/qt/qml/Tattle/assets/icons/copy.svg"
                            enabled: DictationPadModel.charCount > 0
                            size: "small"
                            variant: root.showCopySuccess ? "primary" : "secondary"
                            customAccentColor: root.showCopySuccess ? Theme.colorSuccess : "transparent"
                            Layout.alignment: Qt.AlignVCenter
                            onClicked: {
                                DictationPadModel.copyToClipboard();
                                root.showCopySuccess = true;
                                copyFeedbackTimer.restart();
                            }
                        }

                        StyledButton {
                            id: clearPadBtn
                            text: qsTr("Clear")
                            iconSource: "qrc:/qt/qml/Tattle/assets/icons/trash.svg"
                            enabled: DictationPadModel.charCount > 0
                            size: "small"
                            variant: "secondary"
                            Layout.alignment: Qt.AlignVCenter
                            onClicked: {
                                DictationPadModel.clear();
                            }
                        }
                    }
                }

                StyledDivider {}

                ScrollView {
                    id: textScrollView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    T.TextArea {
                        id: padTextArea
                        text: DictationPadModel.text
                        placeholderText: qsTr("Text will appear here when dictating…")
                        placeholderTextColor: Theme.textPlaceholder
                        font.pixelSize: Theme.fontSizeBody
                        color: Theme.textPrimary
                        selectionColor: Theme.accentColor
                        selectedTextColor: Theme.textOnAccent
                        wrapMode: Text.WordWrap
                        selectByMouse: true
                        leftPadding: Theme.spacingMd
                        rightPadding: Theme.spacingMd
                        topPadding: Theme.spacingMd
                        bottomPadding: Theme.spacingMd

                        background: Rectangle {
                            color: "transparent"
                        }

                        onTextChanged: {
                            if (activeFocus && text !== DictationPadModel.text) {
                                DictationPadModel.text = text;
                            }
                        }
                    }
                }

                Connections {
                    target: DictationPadModel
                    function onTextChanged() {
                        if (padTextArea.text !== DictationPadModel.text) {
                            const wasAtEnd = padTextArea.cursorPosition === padTextArea.text.length;
                            padTextArea.text = DictationPadModel.text;
                            if (wasAtEnd || !padTextArea.activeFocus) {
                                padTextArea.cursorPosition = padTextArea.text.length;
                            }
                        }
                    }
                }
            }
        }
    }
}
