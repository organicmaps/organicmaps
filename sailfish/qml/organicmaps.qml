import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import Nemo.KeepAlive 1.2
import Nemo.Notifications 1.0
import app.organicmaps 1.0
import "cover"
import "pages"
import "pages/notices.js" as Toast
import "pages/notifications.js" as Notifications

ApplicationWindow {
    id: appWindow

    property Item mapPage

    initialPage: Component {
        MapPage {
            id: mapPageItem
            Component.onCompleted: appWindow.mapPage = mapPageItem
        }
    }
    cover: Component {
        CoverPage {
            mapPage: appWindow.mapPage
            onActivateRequested: appWindow.activate()
        }
    }
    allowedOrientations: defaultAllowedOrientations
    Component.onDestruction: downloadNotification.close()

    ShareAction {
        id: fileShare
    }
    Connections {
        target: bookmarksIO
        onExportReady: {
            fileShare.mimeType = mimeType
            fileShare.resources = [fileUrl]
            fileShare.trigger()
        }
        onExportFailed: Toast.show(message)
        onImportFinished: Toast.show(message)
        onBackupFinished: if (!success) Toast.show(appInfo.localized("backup_failed"))
    }

    Connections {
        target: urlHandler
        onActivated: activate()
        onCancelDownloadsRequested: downloads.cancelAll()
    }

    // Retries until the user is back on the map, e.g. after a start by a link.
    Timer {
        running: true
        repeat: true
        interval: 3000
        onTriggered: {
            if (!downloads.shouldOfferUpdate() || appSettings.downloadPermission() === AppSettings.DownloadDenied) {
                stop()
                return
            }
            if (pageStack.depth === 1 && !pageStack.busy) {
                stop()
                // Accepting also confirms a download over mobile data, which the dialog explains then.
                downloads.setUpdateOffered()
                var mobile = appSettings.downloadPermission() === AppSettings.DownloadAsk
                pageStack.push(Qt.resolvedUrl("pages/MessageDialog.qml"), {
                    title: appInfo.localized("whats_new_auto_update_title"),
                    message: appInfo.localized("whats_new_auto_update_message")
                             + (mobile ? "\n\n" + appInfo.localized("download_over_mobile_header") + " "
                                         + appInfo.localized("download_over_mobile_message") : ""),
                    acceptText: appInfo.localized("whats_new_auto_update_button_size", [downloads.updateSize]),
                    cancelText: appInfo.localized("later"),
                    acceptAction: function() { downloads.updateAll() }
                })
            }
        }
    }

    // Keep the device from suspending while maps download, so a blanked screen doesn't stall them.
    KeepAlive {
        enabled: downloads.inProgress
    }

    // No preview, so progress updates stay silent.
    Notification {
        id: downloadNotification
        appName: "Organic Maps"
        appIcon: appInfo.name
        summary: appInfo.localized("downloader_downloading") + " " + downloads.downloadingName
        progress: downloads.downloadingProgress
        remoteActions: [Notifications.openApp(),
                        Notifications.action("cancel", appInfo.localized("cancel"), "cancelDownloads")]
    }
    // Progress comes often; the notification follows it every few seconds.
    Timer {
        running: downloads.inProgress && downloads.downloadingName !== ""
        repeat: true
        triggeredOnStart: true
        interval: 3000
        onTriggered: downloadNotification.publish()
    }
    Connections {
        target: downloads
        onInProgressChanged: if (!downloads.inProgress) downloadNotification.close()
        // Failures are shown in the app; notify only when it is in the background.
        onDownloadFailed: {
            if (Qt.application.state === Qt.ApplicationActive)
                return
            failedNotification.body = appInfo.localized("download_country_failed", [name])
            failedNotification.previewBody = failedNotification.body
            failedNotification.publish()
        }
    }
    Notification {
        id: failedNotification
        appName: "Organic Maps"
        appIcon: appInfo.name
        summary: "Organic Maps"
        previewSummary: summary
        remoteActions: [Notifications.openApp()]
    }
}
