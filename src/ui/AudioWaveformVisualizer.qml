pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Tattle

Item {
    id: root

    property bool active: false
    property bool busy: false
    property real audioLevel: 0.0

    readonly property real pitch: 6.0
    readonly property int barCount: Math.max(16, Math.floor(root.width / pitch))
    readonly property real maxBarHeight: 30
    readonly property real minBarHeight: 2

    implicitWidth: 200
    implicitHeight: maxBarHeight + 6

    property var history: []

    function ensureHistorySize() {
        var count = root.barCount;
        var arr = root.history.slice(0);
        while (arr.length < count) {
            arr.unshift(0.0);
        }
        while (arr.length > count) {
            arr.shift();
        }
        root.history = arr;
    }

    onBarCountChanged: ensureHistorySize()

    Component.onCompleted: ensureHistorySize()

    property real currentEnergy: 0.0
    property real phase: 0.0

    Timer {
        id: streamTimer
        interval: 35
        repeat: true
        running: root.active || root.busy
        onTriggered: {
            if (root.active) {
                var raw = Math.max(0.0, root.audioLevel);
                var norm = Math.min(1.0, Math.pow(raw * 3.5, 0.75));
                root.currentEnergy = Math.max(norm, root.currentEnergy * 0.72);

                var newArr = root.history.slice(1);
                newArr.push(root.currentEnergy);
                root.history = newArr;
            } else if (root.busy) {
                root.phase += 0.16;
                var busyArr = [];
                for (var i = 0; i < root.barCount; i++) {
                    var wave = (Math.sin(i * 0.35 - root.phase) + 1.0) / 2.0;
                    busyArr.push(wave * 0.55);
                }
                root.history = busyArr;
            }
        }
    }

    onActiveChanged: {
        root.currentEnergy = 0.0;
        var freshArr = [];
        for (var i = 0; i < root.barCount; i++) {
            freshArr.push(0.0);
        }
        root.history = freshArr;
    }

    onBusyChanged: {
        if (!root.busy && !root.active) {
            root.currentEnergy = 0.0;
            var resetArr = [];
            for (var i = 0; i < root.barCount; i++) {
                resetArr.push(0.0);
            }
            root.history = resetArr;
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 2

        Repeater {
            model: root.barCount
            delegate: Item {
                id: barDelegate
                required property int index
                Layout.fillWidth: true
                Layout.fillHeight: true

                readonly property real energy: root.history[index] !== undefined ? root.history[index] : 0.0
                readonly property real targetHeight: {
                    if (root.active || root.busy) {
                        return Math.max(root.minBarHeight, Math.min(root.maxBarHeight, root.minBarHeight + (energy * (root.maxBarHeight
                                                                                                                      - root.minBarHeight))));
                    }
                    return root.minBarHeight;
                }

                Rectangle {
                    anchors.centerIn: parent
                    width: Math.max(2, Math.min(3.5, parent.width - 1))
                    height: barDelegate.targetHeight
                    radius: 1

                    color: {
                        if (root.active) {
                            return Theme.colorDanger;
                        }
                        if (root.busy) {
                            return Theme.colorWarning;
                        }
                        return Theme.textTertiary;
                    }

                    opacity: {
                        if (root.active) {
                            return 0.3 + ((barDelegate.index + 1) / root.barCount) * 0.7;
                        }
                        if (root.busy) {
                            return 0.45 + (barDelegate.energy * 0.55);
                        }
                        return 0.18;
                    }

                    Behavior on height {
                        NumberAnimation {
                            duration: 45
                            easing.type: Easing.OutQuad
                        }
                    }

                    Behavior on color {
                        ColorAnimation {
                            duration: Theme.animNormal
                        }
                    }
                }
            }
        }
    }
}
