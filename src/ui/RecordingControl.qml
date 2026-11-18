pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import Tattle
import "controls"

StyledCard {
    id: root

    signal clipboardWarningRequested
    signal shortcutGuideRequested

    customRadius: Theme.radiusMd
    customBorderColor: {
        if (DaemonClient.recording) {
            return Theme.colorDanger;
        }
        if (DaemonClient.isBusy) {
            return Theme.colorWarning;
        }
        return Theme.cardBorder;
    }
    customBorderWidth: DaemonClient.recording ? 2 : 1
    padding: Theme.spacingMd

    Behavior on customBorderColor {
        ColorAnimation {
            duration: Theme.animNormal
            easing.type: Easing.OutCubic
        }
    }

    property int recordingSeconds: 0

    Timer {
        id: recordingTimer
        interval: 1000
        repeat: true
        running: DaemonClient.recording
        onTriggered: {
            root.recordingSeconds += 1;
        }
    }

    Connections {
        target: DaemonClient
        function onRecordingChanged() {
            if (DaemonClient.recording) {
                root.recordingSeconds = 0;
            }
        }
    }

    function formatDuration(totalSecs: int): string {
        const mins = Math.floor(totalSecs / 60);
        const secs = totalSecs % 60;
        const mStr = mins < 10 ? "0" + mins : "" + mins;
        const sStr = secs < 10 ? "0" + secs : "" + secs;
        return mStr + ":" + sStr;
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.spacingSm

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMd

            T.Button {
                id: recordBtn
                implicitWidth: 52
                implicitHeight: 52
                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: DaemonClient.recording ? qsTr("Stop Dictation") : qsTr("Start Dictation")
                Accessible.description: qsTr("Toggle speech recognition recording")
                enabled: DaemonClient.canRecord
                scale: recordBtn.down ? 0.94 : (recordBtn.hovered ? 1.05 : 1.0)

                Behavior on scale {
                    NumberAnimation {
                        duration: Theme.animFast
                        easing.type: Easing.OutCubic
                    }
                }

                background: Rectangle {
                    radius: recordBtn.width / 2
                    color: {
                        if (!recordBtn.enabled) {
                            return Theme.controlBg;
                        }
                        if (DaemonClient.recording) {
                            return recordBtn.down ? Theme.buttonDangerBgPressed : (recordBtn.hovered
                                                                                   ? Theme.buttonDangerBgHover :
                                                                                     Theme.buttonDangerBg);
                        }
                        if (DaemonClient.isBusy) {
                            return Theme.colorWarning;
                        }
                        return recordBtn.down ? Theme.buttonPrimaryBgPressed : (recordBtn.hovered
                                                                                ? Theme.buttonPrimaryBgHover :
                                                                                  Theme.buttonPrimaryBg);
                    }
                    border.color: recordBtn.visualFocus ? Theme.focusRingColor : Theme.cardBorder
                    border.width: recordBtn.visualFocus ? Theme.focusRingWidth : 1

                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.animFast
                        }
                    }
                }

                contentItem: Item {
                    anchors.fill: parent

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24
                        source: "qrc:/qt/qml/Tattle/assets/icons/mic.svg"
                        sourceSize.width: 24
                        sourceSize.height: 24
                        visible: !DaemonClient.recording && !DaemonClient.isBusy
                        smooth: true
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 20
                        height: 20
                        source: "qrc:/qt/qml/Tattle/assets/icons/stop.svg"
                        sourceSize.width: 20
                        sourceSize.height: 20
                        visible: DaemonClient.recording
                        smooth: true
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 22
                        height: 22
                        source: "qrc:/qt/qml/Tattle/assets/icons/spinner.svg"
                        sourceSize.width: 22
                        sourceSize.height: 22
                        visible: DaemonClient.isBusy && !DaemonClient.recording
                        smooth: true

                        RotationAnimation on rotation {
                            running: DaemonClient.isBusy && !DaemonClient.recording
                            loops: Animation.Infinite
                            from: 0
                            to: 360
                            duration: 1000
                        }
                    }
                }

                onClicked: {
                    if (!DaemonClient.recording && DaemonDiagnosticModel.clipboardWarningRequired) {
                        root.clipboardWarningRequested();
                    } else {
                        DaemonClient.toggleRecording();
                    }
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                spacing: 3

                StyledText {
                    text: {
                        if (DaemonClient.recording) {
                            return qsTr("Recording…");
                        }
                        if (DaemonClient.isBusy) {
                            return qsTr("Transcribing…");
                        }
                        if (DaemonClient.dictationState === DaemonClient.Error) {
                            return qsTr("Error");
                        }
                        if (DaemonClient.statusMessage.length > 0) {
                            return DaemonClient.statusMessage;
                        }
                        return qsTr("Ready to record");
                    }
                    variant: "subheading"
                    customWeight: Font.DemiBold
                    customColor: {
                        if (DaemonClient.recording) {
                            return Theme.colorDanger;
                        }
                        if (DaemonClient.isBusy) {
                            return Theme.colorWarning;
                        }
                        if (DaemonClient.dictationState === DaemonClient.Error) {
                            return Theme.colorDanger;
                        }
                        return Theme.textPrimary;
                    }
                }

                StyledText {
                    text: {
                        if (DaemonClient.recording) {
                            return root.formatDuration(root.recordingSeconds) + " • " + qsTr(
                                "Press Ctrl+Shift+Space to stop");
                        }
                        if (DaemonClient.isBusy) {
                            return qsTr("Converting speech audio to text…");
                        }
                        return DaemonClient.recordingMode === DaemonClient.PushToTalk ? qsTr(
                                                                                            "Hold Ctrl+Shift+Space to talk") :
                                                                                        qsTr("Press Ctrl+Shift+Space to start / stop");
                    }
                    variant: "caption"
                    colorRole: "secondary"
                }
            }

            AudioWaveformVisualizer {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: Theme.spacingMd
                active: DaemonClient.recording
                busy: DaemonClient.isBusy && !DaemonClient.recording
                audioLevel: DaemonClient.audioLevel
            }
        }

        SystemStatusFooter {
            Layout.fillWidth: true
            onShortcutGuideRequested: root.shortcutGuideRequested()
        }
    }
}
