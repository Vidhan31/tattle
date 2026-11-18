pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Tattle

ColumnLayout {
    id: root

    signal navigateRequested(string target)
    signal clipboardWarningRequested

    readonly property bool hasVisibleBanners: (clipboardBanner.visible || apiKeyBanner.visible
                                               || invalidApiKeyBanner.visible || rateLimitedBanner.visible
                                               || missingModelBanner.visible || loadingModelBanner.visible
                                               || noMicBanner.visible || fatalErrorBanner.visible
                                               || dictationErrorBanner.visible)

    visible: hasVisibleBanners
    Layout.fillWidth: true
    Layout.preferredHeight: hasVisibleBanners ? -1 : 0
    implicitHeight: hasVisibleBanners ? -1 : 0

    spacing: Theme.spacingMd

    StatusBanner {
        id: clipboardBanner
        visible: DaemonDiagnosticModel.clipboardWarningRequired && !DaemonDiagnosticModel.clipboardBannerDismissed
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        bannerType: "warning"
        title: qsTr("Clipboard & Paste Notice")
        message: qsTr(
                     "Dictation pastes text via Ctrl+Shift+V. Text is restored, but images/media are overwritten unless saved or using a clipboard manager (e.g., Klipper, GPaste, Clipboard Indicator).")
        actionText: qsTr("Learn More")
        onActionClicked: root.clipboardWarningRequested()
        secondaryActionText: qsTr("Dismiss")
        onSecondaryActionClicked: {
            DaemonDiagnosticModel.clipboardBannerDismissed = true;
        }
    }

    StatusBanner {
        id: apiKeyBanner
        visible: DaemonClient.activeBackend === DaemonClient.Cloud && !DaemonClient.cloudApiKeySet
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        bannerType: "warning"
        title: qsTr("%1 API Key Required").arg(CloudProviderModel.activeProviderName)
        message: qsTr("Configure your %1 API key in Settings to begin cloud speech transcription.").arg(
                     CloudProviderModel.activeProviderName)
        actionText: qsTr("Configure API Key")
        onActionClicked: root.navigateRequested("cloud")
    }

    StatusBanner {
        id: invalidApiKeyBanner
        visible: DaemonClient.activeBackend === DaemonClient.Cloud && DaemonClient.cloudApiKeyInvalid
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        bannerType: "warning"
        title: qsTr("Invalid %1 API Key").arg(CloudProviderModel.activeProviderName)
        message: DaemonClient.cloudLastError.length > 0 ? DaemonClient.cloudLastError : qsTr(
                                                              "Authentication failed. Please check your %1 API key.").arg(
                                                              CloudProviderModel.activeProviderName)
        actionText: qsTr("Configure API Key")
        onActionClicked: root.navigateRequested("cloud")
        secondaryActionText: qsTr("Dismiss")
        onSecondaryActionClicked: DaemonClient.clearError()
    }

    StatusBanner {
        id: rateLimitedBanner
        visible: DaemonClient.activeBackend === DaemonClient.Cloud && DaemonClient.cloudRateLimited
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        bannerType: "warning"
        title: qsTr("%1 Rate Limit Reached").arg(CloudProviderModel.activeProviderName)
        message: qsTr("Auto-retrying in %1s…").arg(DaemonClient.cloudRetrySecondsRemaining)
        actionText: qsTr("Dismiss")
        onActionClicked: DaemonClient.clearError()
    }

    StatusBanner {
        id: missingModelBanner
        visible: DaemonClient.activeBackend === DaemonClient.TranscribeCpp && !DaemonClient.isModelInstalled
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        bannerType: "warning"
        title: qsTr("Offline Speech Model Missing")
        message: qsTr("Download %1 to start offline transcription.").arg(DaemonClient.modelFileName)
        actionText: qsTr("Offline Settings")
        onActionClicked: root.navigateRequested("offline")
    }

    StatusBanner {
        id: loadingModelBanner
        visible: DaemonClient.activeBackend === DaemonClient.TranscribeCpp && DaemonClient.isModelInstalled &&
                 !DaemonClient.isModelLoaded
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        bannerType: "info"
        title: qsTr("Loading Speech Model")
        message: qsTr("Loading offline speech recognition model…")
        actionText: qsTr("Offline Settings")
        onActionClicked: root.navigateRequested("offline")
    }

    StatusBanner {
        id: noMicBanner
        visible: !DaemonClient.microphoneAvailable
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        bannerType: "warning"
        title: qsTr("No Microphone Detected")
        message: qsTr("Please connect a microphone or check your audio permissions in system settings.")
        actionText: qsTr("Audio Settings")
        onActionClicked: root.navigateRequested("audio")
    }

    StatusBanner {
        id: fatalErrorBanner
        visible: DaemonDiagnosticModel.hasFatalError
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        bannerType: "danger"
        title: qsTr("Text Paste Service Stopped")
        message: DaemonDiagnosticModel.fatalErrorMessage.length > 0 ? DaemonDiagnosticModel.fatalErrorMessage : qsTr(
                                                                          "The background text paste service encountered an error. Automatic paste is unavailable until restarted.")
        actionText: qsTr("Restart Service")
        onActionClicked: DaemonDiagnosticModel.restartService()
        secondaryActionText: qsTr("Settings")
        onSecondaryActionClicked: root.navigateRequested("system")
    }

    StatusBanner {
        id: dictationErrorBanner
        visible: DaemonClient.dictationState === DaemonClient.Error && (DaemonClient.activeBackend
                                                                        !== DaemonClient.Cloud || (
                                                                            !DaemonClient.cloudApiKeyInvalid &&
                                                                            !DaemonClient.cloudRateLimited))
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        bannerType: "danger"
        title: DaemonClient.activeBackend === DaemonClient.TranscribeCpp ? qsTr("Offline Transcription Failed") : qsTr(
                                                                               "Transcription Failed")
        message: DaemonClient.lastError.length > 0 ? DaemonClient.lastError : qsTr(
                                                         "An error occurred during transcription.")
        actionText: qsTr("Retry Transcription")
        onActionClicked: DaemonClient.retryTranscription()
        secondaryActionText: qsTr("Dismiss")
        onSecondaryActionClicked: DaemonClient.clearError()
    }
}
