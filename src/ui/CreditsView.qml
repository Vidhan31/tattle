pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tattle
import "controls"

Item {
    id: root

    implicitWidth: 620
    implicitHeight: 540

    component ComponentItem: ColumnLayout {
        id: compItem
        property string title: ""
        property string url: ""
        property string urlLabel: "GitHub"
        property string description: ""
        property string badgeText: ""

        Layout.fillWidth: true
        spacing: Theme.spacingXs

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSm

            StyledText {
                text: compItem.title
                variant: "body"
                customWeight: Font.Medium
            }

            StateBadge {
                text: compItem.badgeText
                statusType: "neutral"
                visible: compItem.badgeText.length > 0
                pixelSize: Theme.fontSizeSmall
            }

            Item {
                Layout.fillWidth: true
            }

            StyledText {
                text: '<a href="' + compItem.url + '" style="color: ' + Theme.accentColor.toString()
                      + '; text-decoration: none;">' + compItem.urlLabel + ' ↗</a>'
                textFormat: Text.StyledText
                variant: "caption"
                customWeight: Font.Medium
                onLinkActivated: link => Qt.openUrlExternally(link)

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.NoButton
                }
            }
        }

        StyledText {
            text: compItem.description
            variant: "caption"
            colorRole: "secondary"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    ScrollView {
        id: creditsScrollView
        anchors.fill: parent
        clip: true

        ColumnLayout {
            width: creditsScrollView.availableWidth
            spacing: Theme.spacingMd

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacingXs
                Layout.bottomMargin: Theme.spacingSm
                spacing: 4

                StyledText {
                    text: qsTr("About")
                    variant: "heading"
                }

                StyledText {
                    text: qsTr("Application information, components, and open-source libraries")
                    variant: "caption"
                    colorRole: "secondary"
                }
            }

            StyledCard {
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMd

                    Image {
                        source: "qrc:/qt/qml/Tattle/assets/tattle_logo.svg"
                        sourceSize: Qt.size(44, 44)
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44
                        Layout.alignment: Qt.AlignVCenter
                        smooth: true
                        mipmap: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingXs

                        RowLayout {
                            spacing: Theme.spacingSm

                            StyledText {
                                text: qsTr("Tattle")
                                variant: "heading"
                            }

                            StateBadge {
                                text: Qt.application.version ? "v" + Qt.application.version : "v1.0.0"
                                statusType: "accent"
                            }

                            StateBadge {
                                text: qsTr("GPL-3.0-or-later")
                                statusType: "neutral"
                            }
                        }

                        StyledText {
                            text: qsTr("Fast and modern voice dictation app for Wayland.")
                            variant: "body"
                            colorRole: "secondary"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        StyledText {
                            text: '<a href="https://github.com/Vidhan31/tattle" style="color: '
                                  + Theme.accentColor.toString()
                                  + '; text-decoration: none;">github.com/Vidhan31/tattle ↗</a>'
                            textFormat: Text.StyledText
                            variant: "caption"
                            customWeight: Font.Medium
                            onLinkActivated: link => Qt.openUrlExternally(link)

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                acceptedButtons: Qt.NoButton
                            }
                        }
                    }
                }
            }

            StyledCard {
                Layout.fillWidth: true

                StyledText {
                    text: qsTr("Core Frameworks & Libraries")
                    variant: "subheading"
                    Layout.bottomMargin: Theme.spacingXs
                }

                ComponentItem {
                    title: qsTr("transcribe.cpp")
                    badgeText: qsTr("MIT")
                    url: "https://github.com/handy-computer/transcribe.cpp"
                    urlLabel: "handy-computer/transcribe.cpp"
                    description: qsTr(
                                     "High-performance C/C++ offline speech recognition engine for Whisper, Parakeet, Moonshine and Granite GGUF models with GPU acceleration.")
                }

                StyledDivider {}

                ComponentItem {
                    title: qsTr("Qt 6 Framework")
                    badgeText: qsTr("LGPL-3.0 / GPL-3.0")
                    url: "https://github.com/qt/qtbase"
                    urlLabel: "qt/qtbase"
                    description: qsTr(
                                     "Cross-platform GUI framework, declarative QML / Qt Quick presentation layer, Multimedia audio recording, Network, and D-Bus IPC.")
                }

                StyledDivider {}

                ComponentItem {
                    title: qsTr("QtKeychain")
                    badgeText: qsTr("BSD-3-Clause")
                    url: "https://github.com/frankosterfeld/qtkeychain"
                    urlLabel: "frankosterfeld/qtkeychain"
                    description: qsTr(
                                     "Platform-independent Qt library providing secure credential storage using Linux Secret Service (freedesktop.org) and KWallet.")
                }
            }

            StyledCard {
                Layout.fillWidth: true

                StyledText {
                    text: qsTr("Artwork & Icons")
                    variant: "subheading"
                    Layout.bottomMargin: Theme.spacingXs
                }

                ComponentItem {
                    title: qsTr("Font Awesome Free")
                    badgeText: qsTr("CC BY 4.0 / SIL OFL")
                    url: "https://github.com/FortAwesome/Font-Awesome"
                    urlLabel: "FortAwesome/Font-Awesome"
                    description: qsTr("Vector icon assets for UI controls and action buttons.")
                }
            }
        }
    }
}
