pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Tattle

ColumnLayout {
    id: root

    signal navigateRequested(string target)
    signal clipboardWarningRequested

    property bool offlineSwitchBannerDismissed: false

    Connections {
        target: DaemonClient
        function onNetworkOnlineChanged() {
            root.offlineSwitchBannerDismissed = false;
        }
    }

    readonly property bool showOfflineSwitchPrompt: !DaemonClient.networkOnline && DaemonClient.activeBackend
                                                    === DaemonClient.Cloud && DaemonClient.autoSwitchOffline
                                                    && ModelCatalog.hasDownloadedModels && !offlineSwitchBannerDismissed
    readonly property bool showOfflineNoModelWarning: !DaemonClient.networkOnline && DaemonClient.activeBackend
                                                      === DaemonClient.Cloud && !ModelCatalog.hasDownloadedModels

    readonly property bool showClipboardWarning: DaemonDiagnosticModel.clipboardWarningRequired &&
                                                 !DaemonDiagnosticModel.clipboardBannerDismissed
    readonly property bool showApiKeyWarning: DaemonClient.activeBackend === DaemonClient.Cloud &&
                                              !DaemonClient.cloudApiKeySet
    readonly property bool showInvalidApiKeyWarning: DaemonClient.activeBackend === DaemonClient.Cloud
                                                     && DaemonClient.cloudApiKeyInvalid
    readonly property bool showRateLimitedWarning: DaemonClient.activeBackend === DaemonClient.Cloud
                                                   && DaemonClient.cloudRateLimited
    readonly property bool showMissingModelWarning: DaemonClient.activeBackend === DaemonClient.TranscribeCpp &&
                                                    !DaemonClient.isModelInstalled
    readonly property bool showLoadingModelWarning: DaemonClient.activeBackend === DaemonClient.TranscribeCpp
                                                    && DaemonClient.isModelInstalled && !DaemonClient.isModelLoaded
    readonly property bool showNoMicWarning: !DaemonClient.microphoneAvailable
    readonly property bool showFatalErrorWarning: DaemonDiagnosticModel.hasFatalError
    readonly property bool showDictationErrorWarning: DaemonClient.dictationState === DaemonClient.Error && (
                                                          DaemonClient.activeBackend !== DaemonClient.Cloud || (
                                                              !DaemonClient.cloudApiKeyInvalid &&
                                                              !DaemonClient.cloudRateLimited))

    readonly property bool hasVisibleBanners: (showOfflineSwitchPrompt || showOfflineNoModelWarning
                                               || showClipboardWarning || showApiKeyWarning || showInvalidApiKeyWarning
                                               || showRateLimitedWarning || showMissingModelWarning
                                               || showLoadingModelWarning || showNoMicWarning || showFatalErrorWarning
                                               || showDictationErrorWarning)

    visible: hasVisibleBanners
    Layout.fillWidth: true
    Layout.preferredHeight: hasVisibleBanners ? -1 : 0
    implicitHeight: hasVisibleBanners ? -1 : 0

    spacing: Theme.spacingMd

    StatusBanner {
        id: offlineSwitchBanner
        visible: root.showOfflineSwitchPrompt
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        bannerType: "warning"
        title: qsTr("Internet Connection Offline")
        message: {
            const name = ModelCatalog.selectedModelName;
            return name.length > 0 ? qsTr(
                                         "Cloud speech transcription is unavailable. Switch to offline model (%1)?").arg(
                                         name) : qsTr(
                                         "Cloud speech transcription is unavailable. Switch to offline speech model?");
        }
        actionText: {
            const name = ModelCatalog.selectedModelName;
            return name.length > 0 ? qsTr("Switch to %1").arg(name) : qsTr("Switch to Offline");
        }
        actionEnabled: !DaemonClient.isBusy
        onActionClicked: {
            if (!DaemonClient.isBusy) {
                DaemonClient.activeBackend = DaemonClient.TranscribeCpp;
            }
        }
        secondaryActionText: qsTr("Dismiss")
        onSecondaryActionClicked: {
            root.offlineSwitchBannerDismissed = true;
        }
    }

    StatusBanner {
        id: offlineNoModelBanner
        visible: root.showOfflineNoModelWarning
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        bannerType: "warning"
        title: qsTr("Internet Connection Offline")
        message: qsTr("Cloud speech transcription is unavailable. Download a model to use offline transcription.")
        actionText: qsTr("Download Model")
        onActionClicked: root.navigateRequested("offline")
    }

    StatusBanner {
        id: clipboardBanner
        visible: root.showClipboardWarning
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
        visible: root.showApiKeyWarning
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
        visible: root.showInvalidApiKeyWarning
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
        visible: root.showRateLimitedWarning
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
        visible: root.showMissingModelWarning
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
        visible: root.showLoadingModelWarning
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
        visible: root.showNoMicWarning
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
        visible: root.showFatalErrorWarning
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
        visible: root.showDictationErrorWarning
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
