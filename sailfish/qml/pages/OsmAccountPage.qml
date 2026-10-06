import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    Connections {
        target: osmAccount
        onLoginFailed: errorLabel.text = message
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            visible: osmAccount.loggedIn

            MenuItem {
                text: appInfo.localized("logout")
                onClicked: Remorse.popupAction(page, appInfo.localized("logout"), function() { osmAccount.logout() })
            }
        }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: appInfo.localized(osmAccount.loggedIn ? "osm_account" : "login")
            }

            PageLabel {
                visible: !osmAccount.loggedIn
                text: appInfo.localized("login_to_make_edits_visible")
                color: Theme.highlightColor
            }
            PageLabel {
                visible: !osmAccount.loggedIn
                text: appInfo.localized("login_osm_presentation")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
            }
            PageLabel {
                id: errorLabel
                visible: !osmAccount.loggedIn && text !== ""
                color: Theme.errorColor
            }
            // The browser returns to the app when done.
            Item {
                width: parent.width
                height: loginButton.height
                visible: !osmAccount.loggedIn

                Button {
                    id: loginButton
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: !osmAccount.busy
                    text: appInfo.localized("login_osm")
                    onClicked: {
                        errorLabel.text = ""
                        osmAccount.loginInBrowser()
                    }
                }
                BusyIndicator {
                    anchors.centerIn: parent
                    size: BusyIndicatorSize.Medium
                    running: osmAccount.busy
                }
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: !osmAccount.loggedIn
                horizontalAlignment: Text.AlignHCenter
                text: appInfo.localized("no_osm_account")
                color: Theme.secondaryHighlightColor
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: !osmAccount.loggedIn
                text: appInfo.localized("register_at_openstreetmap")
                onClicked: Qt.openUrlExternally(osmAccount.registrationUrl)
            }

            Column {
                width: parent.width
                visible: osmAccount.loggedIn

                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: status === Image.Ready
                    width: Theme.iconSizeExtraLarge
                    height: width
                    sourceSize: Qt.size(width, height)
                    fillMode: Image.PreserveAspectCrop
                    source: osmAccount.imageUrl
                }
                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x
                    text: osmAccount.userName
                    font.pixelSize: Theme.fontSizeLarge
                    color: Theme.highlightColor
                    truncationMode: TruncationMode.Fade
                }
                DetailItem {
                    label: appInfo.localized("editor_profile_changes")
                    value: osmAccount.changesets >= 0 ? osmAccount.changesets : "—"
                }
                DetailItem {
                    label: appInfo.localized("editor_pending_edits")
                    value: osmAccount.pendingEdits
                }
                DetailItem {
                    visible: !isNaN(osmAccount.lastUpload.getTime())
                    label: appInfo.localized("last_upload")
                    value: Format.formatDate(osmAccount.lastUpload, Formatter.DurationElapsed)
                }
                BusyIndicator {
                    anchors.horizontalCenter: parent.horizontalCenter
                    size: BusyIndicatorSize.Small
                    running: osmAccount.busy
                    visible: running
                }
                MenuRow {
                    visible: osmAccount.userName !== ""
                    icon: "image://theme/icon-m-website"
                    text: appInfo.localized("editor_osm_history")
                    onClicked: Qt.openUrlExternally(osmAccount.historyUrl)
                }
                MenuRow {
                    visible: osmAccount.userName !== ""
                    icon: "image://theme/icon-m-note"
                    text: appInfo.localized("editor_osm_notes")
                    onClicked: Qt.openUrlExternally(osmAccount.notesUrl)
                }
                MenuRow {
                    icon: "../../icons/help/ic_openstreetmap.svg"
                    text: appInfo.localized("editor_more_about_osm")
                    onClicked: Qt.openUrlExternally(appInfo.localized("osm_wiki_about_url"))
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
