pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Templates as T
import QtQuick.Layouts
import Tattle
import ".."
import "../controls"

Item {
    id: root

    implicitWidth: 500
    implicitHeight: contentColumn.implicitHeight
    height: contentColumn.implicitHeight

    ColumnLayout {
        id: contentColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: Theme.spacingMd

        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingXs
            Layout.bottomMargin: Theme.spacingXs
            spacing: 2

            StyledText {
                text: qsTr("Offline Transcription")
                variant: "heading"
            }

            StyledText {
                text: qsTr("Configure on-device speech recognition models and hardware acceleration")
                variant: "caption"
                colorRole: "secondary"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        StyledCard {
            id: libraryCard
            title: qsTr("Speech Model Library")
            description: qsTr("Download and manage speech recognition models")

            readonly property int whisperTotal: (ModelCatalog.revision >= 0) ? ModelCatalog.totalCountForPrefix(
                                                                                   "whisper-") : 0
            readonly property int moonshineTotal: (ModelCatalog.revision >= 0) ? ModelCatalog.totalCountForPrefix(
                                                                                     "moonshine-") : 0
            readonly property int parakeetTotal: (ModelCatalog.revision >= 0) ? ModelCatalog.totalCountForPrefix(
                                                                                    "parakeet-") : 0
            readonly property int allTotal: (ModelCatalog.revision >= 0) ? ModelCatalog.totalCountForPrefix("") : 0

            property string selectedFamily: {
                const downloading = ModelCatalog.downloadingModelId;
                if (downloading.length > 0) {
                    if (downloading.startsWith("whisper-"))
                    return "whisper";
                    if (downloading.startsWith("moonshine-"))
                    return "moonshine";
                    if (downloading.startsWith("parakeet-"))
                    return "parakeet";
                }
                const active = DaemonClient.modelFileName;
                if (active.includes("moonshine"))
                return "moonshine";
                if (active.includes("parakeet"))
                return "parakeet";
                return "whisper";
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: errorLayout.implicitHeight + Theme.spacingSm * 2
                radius: Theme.radiusSm
                color: Theme.statusDangerBg
                border.color: Theme.statusDangerBorder
                border.width: 1
                visible: ModelCatalog.lastError.length > 0

                RowLayout {
                    id: errorLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: Theme.spacingSm
                    spacing: Theme.spacingSm

                    StyledText {
                        text: ModelCatalog.lastError
                        variant: "caption"
                        customColor: Theme.colorDanger
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    StyledButton {
                        text: qsTr("Dismiss")
                        size: "small"
                        variant: "flat"
                        onClicked: {
                            ModelCatalog.clearLastError();
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                StyledButton {
                    text: qsTr("Whisper (%1)").arg(libraryCard.whisperTotal)
                    size: "small"
                    variant: libraryCard.selectedFamily === "whisper" ? "primary" : "secondary"
                    onClicked: {
                        libraryCard.selectedFamily = "whisper";
                    }
                }

                StyledButton {
                    text: qsTr("Moonshine (%1)").arg(libraryCard.moonshineTotal)
                    size: "small"
                    variant: libraryCard.selectedFamily === "moonshine" ? "primary" : "secondary"
                    onClicked: {
                        libraryCard.selectedFamily = "moonshine";
                    }
                }

                StyledButton {
                    text: qsTr("Parakeet (%1)").arg(libraryCard.parakeetTotal)
                    size: "small"
                    variant: libraryCard.selectedFamily === "parakeet" ? "primary" : "secondary"
                    onClicked: {
                        libraryCard.selectedFamily = "parakeet";
                    }
                }

                StyledButton {
                    text: qsTr("All (%1)").arg(libraryCard.allTotal)
                    size: "small"
                    variant: libraryCard.selectedFamily === "all" ? "primary" : "secondary"
                    onClicked: {
                        libraryCard.selectedFamily = "all";
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            Flow {
                id: cardsFlow
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                readonly property int responsiveColumns: {
                    if (cardsFlow.width >= 860)
                    return 3;
                    if (cardsFlow.width >= 540)
                    return 2;
                    return 1;
                }

                Repeater {
                    model: ModelCatalog

                    delegate: Rectangle {
                        id: modelCard
                        required property int index
                        required property string modelId
                        required property string name
                        required property string fileName
                        required property string sizeFormatted
                        required property string memoryFormatted
                        required property string description
                        required property bool isInstalled
                        required property bool isSelected
                        required property bool isDownloading
                        required property real progress
                        required property string speedFormatted
                        required property string installedSizeFormatted
                        required property bool canDelete

                        readonly property bool inFamily: {
                            if (libraryCard.selectedFamily === "all") {
                                return true;
                            }
                            return modelId.startsWith(libraryCard.selectedFamily + "-");
                        }

                        visible: inFamily
                        width: Math.floor((cardsFlow.width - (cardsFlow.responsiveColumns - 1) * cardsFlow.spacing)
                                          / cardsFlow.responsiveColumns)
                        implicitHeight: Math.max(148, cardInnerLayout.implicitHeight + Theme.spacingMd * 2)
                        radius: Theme.radiusSm
                        color: modelCard.isSelected ? Theme.selectedBg : (cardHover.containsMouse
                                                                          ? Theme.cardBgElevated : Theme.controlBg)
                        border.color: modelCard.isSelected ? Theme.accentColor : (cardHover.containsMouse
                                                                                  ? Theme.cardBorderHover :
                                                                                    Theme.controlBorder)
                        border.width: modelCard.isSelected ? 1.5 : 1

                        Behavior on color {
                            ColorAnimation {
                                duration: Theme.animFast
                            }
                        }
                        Behavior on border.color {
                            ColorAnimation {
                                duration: Theme.animFast
                            }
                        }

                        MouseArea {
                            id: cardHover
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }

                        ColumnLayout {
                            id: cardInnerLayout
                            anchors.fill: parent
                            anchors.margins: Theme.spacingMd
                            spacing: Theme.spacingXs

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacingSm

                                StyledText {
                                    text: modelCard.name.replace(" (English)", "").replace(" (Multilingual)", "")
                                    variant: "body"
                                    customWeight: Font.DemiBold
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }

                                Rectangle {
                                    implicitHeight: 18
                                    implicitWidth: langLabel.implicitWidth + 8
                                    radius: Theme.radiusXs
                                    color: Theme.cardBgSubtle
                                    border.color: Theme.controlBorder
                                    border.width: 1
                                    Layout.alignment: Qt.AlignTop

                                    StyledText {
                                        id: langLabel
                                        anchors.centerIn: parent
                                        text: modelCard.name.toLowerCase().includes("english") ? qsTr("English") : qsTr(
                                                                                                     "Multilingual")
                                        variant: "caption"
                                        colorRole: "secondary"
                                    }
                                }
                            }

                            StyledText {
                                text: (modelCard.isInstalled && modelCard.installedSizeFormatted.length > 0
                                       ? modelCard.installedSizeFormatted : modelCard.sizeFormatted) + " • "
                                      + modelCard.memoryFormatted
                                variant: "caption"
                                colorRole: "secondary"
                            }

                            StyledText {
                                text: modelCard.description
                                variant: "caption"
                                colorRole: "secondary"
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            Item {
                                Layout.fillHeight: true
                            }

                            Item {
                                Layout.fillWidth: true
                                implicitHeight: 28

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 3
                                    visible: modelCard.isDownloading

                                    RowLayout {
                                        Layout.fillWidth: true

                                        StyledText {
                                            text: qsTr("%1%").arg(Math.round(modelCard.progress * 100))
                                            variant: "caption"
                                            customWeight: Font.DemiBold
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }

                                        StyledText {
                                            text: modelCard.speedFormatted
                                            variant: "caption"
                                            colorRole: "secondary"
                                        }

                                        StyledButton {
                                            text: qsTr("Cancel")
                                            size: "small"
                                            variant: "danger"
                                            onClicked: {
                                                ModelCatalog.cancelDownload(modelCard.modelId);
                                            }
                                        }
                                    }

                                    T.ProgressBar {
                                        Layout.fillWidth: true
                                        implicitHeight: 4
                                        from: 0.0
                                        to: 1.0
                                        value: Math.min(Math.max(modelCard.progress, 0.0), 1.0)
                                        background: Rectangle {
                                            implicitHeight: 4
                                            radius: Theme.radiusCircle
                                            color: Theme.controlBg
                                        }
                                        contentItem: Item {
                                            implicitHeight: 4
                                            Rectangle {
                                                anchors.left: parent.left
                                                anchors.top: parent.top
                                                anchors.bottom: parent.bottom
                                                width: parent.width * Math.min(Math.max(modelCard.progress, 0.0), 1.0)
                                                radius: Theme.radiusCircle
                                                color: Theme.accentColor
                                            }
                                        }
                                    }
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: Theme.spacingSm
                                    visible: !modelCard.isDownloading && modelCard.isSelected
                                             && DaemonClient.isModelLoaded

                                    StyledText {
                                        text: qsTr("Active")
                                        variant: "caption"
                                        customWeight: Font.DemiBold
                                        customColor: Theme.accentColor
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                    }

                                    StyledButton {
                                        text: qsTr("Unload")
                                        size: "small"
                                        variant: "ghost"
                                        visible: DaemonClient.isModelLoaded
                                        enabled: !DaemonClient.isModelBusy && !DaemonClient.isBusy
                                        onClicked: {
                                            DaemonClient.unloadModel();
                                        }
                                    }

                                    StyledButton {
                                        iconSource: "qrc:/qt/qml/Tattle/assets/icons/trash.svg"
                                        size: "small"
                                        variant: "danger"
                                        visible: modelCard.canDelete
                                        enabled: !DaemonClient.isModelBusy && !DaemonClient.isBusy
                                        onClicked: {
                                            ModelCatalog.deleteModel(modelCard.modelId);
                                        }
                                    }
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: Theme.spacingSm
                                    visible: !modelCard.isDownloading && modelCard.isInstalled && (
                                                 !modelCard.isSelected || !DaemonClient.isModelLoaded)

                                    StyledButton {
                                        text: modelCard.isSelected ? qsTr("Load") : qsTr("Activate")
                                        size: "small"
                                        variant: modelCard.isSelected ? "primary" : "secondary"
                                        Layout.fillWidth: true
                                        enabled: !DaemonClient.isModelBusy && !DaemonClient.isBusy
                                        onClicked: {
                                            if (!modelCard.isSelected) {
                                                ModelCatalog.setSelectedModelId(modelCard.modelId);
                                            }
                                            DaemonClient.loadModel();
                                        }
                                    }

                                    StyledButton {
                                        iconSource: "qrc:/qt/qml/Tattle/assets/icons/trash.svg"
                                        size: "small"
                                        variant: "danger"
                                        visible: modelCard.canDelete
                                        enabled: !DaemonClient.isModelBusy && !DaemonClient.isBusy
                                        onClicked: {
                                            ModelCatalog.deleteModel(modelCard.modelId);
                                        }
                                    }
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: Theme.spacingSm
                                    visible: !modelCard.isDownloading && !modelCard.isInstalled

                                    StyledButton {
                                        text: qsTr("Download")
                                        size: "small"
                                        variant: "primary"
                                        Layout.fillWidth: true
                                        enabled: !ModelCatalog.isDownloadingAny
                                        onClicked: {
                                            ModelCatalog.startDownload(modelCard.modelId);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        StyledCard {
            title: qsTr("Hardware Acceleration")
            description: qsTr("GPU compute offloading status and inference backend diagnostics")

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMd

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    StyledText {
                        text: qsTr("Inference Engine Device")
                        variant: "body"
                        customWeight: Font.Medium
                    }

                    StyledText {
                        text: DaemonClient.isVulkanSupported ? qsTr("Vulkan GPU acceleration is enabled in this build") :
                                                               qsTr("Running via optimized CPU SIMD intrinsics (AVX2/AVX)")
                        variant: "caption"
                        colorRole: "secondary"
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }

                StateBadge {
                    text: DaemonClient.computeDevice
                    statusType: DaemonClient.isVulkanSupported ? "accent" : "neutral"
                }
            }
        }
    }
}
