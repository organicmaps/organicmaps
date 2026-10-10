import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import "downloads.js" as Downloads
import "notices.js" as Toast
import "share.js" as Share

// The main menu, in the order of the Android one. Entries that act on the map return to it first.
Page {
    id: page

    property Item mapPage
    readonly property QtObject map: mapPage.mapItem

    allowedOrientations: Orientation.All

    ShareAction {
        id: shareLocationAction
        mimeType: "text/plain"
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: appInfo.localized("menu")
            }
            MenuRow {
                icon: "image://theme/icon-m-add"
                text: appInfo.localized("placepage_add_place_button")
                onClicked: {
                    // An old map can't be edited: offered for update first.
                    var outdated = map.mapToUpdateForEditing()
                    if (outdated.countryId) {
                        pageStack.push(Qt.resolvedUrl("MessageDialog.qml"), {
                            title: appInfo.localized("contribute_to_osm_update_map"),
                            message: appInfo.localized("contribute_to_osm_update_map_description", [outdated.name]),
                            acceptText: appInfo.localized("download"),
                            acceptAction: function() {
                                Downloads.start(pageStack, function() { map.downloadMap(outdated.countryId) })
                            }
                        })
                        return
                    }
                    pageStack.pop(mapPage)
                    map.startChoosingPosition()
                }
            }
            MenuRow {
                icon: "image://theme/icon-m-cloud-download"
                text: appInfo.localized("download_maps")
                      + (downloads.updateCount > 0 ? " (" + downloads.updateCount + ")" : "")
                onClicked: {
                    // Leaves route planning.
                    if (map.routing.active)
                        map.routing.close()
                    pageStack.push(Qt.resolvedUrl("MapsPage.qml"))
                }
            }
            MenuRow {
                visible: appSettings.donateUrl !== ""
                icon: "../../icons/menu/ic_donate.svg"
                text: appInfo.localized("donate")
                onClicked: appSettings.openDonatePage()
            }
            MenuRow {
                icon: "image://theme/icon-m-setting"
                text: appInfo.localized("settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"), { routing: map.routing })
            }
            MenuRow {
                icon: map.trackRecording ? "../../icons/menu/ic_track_recording_on.svg"
                                         : "../../icons/menu/ic_track_recording_off.svg"
                text: map.trackRecording ? appInfo.localized("stop_track_recording")
                                         : appInfo.localized("start_track_recording")
                // While recording it shows the recording.
                onClicked: {
                    pageStack.pop(mapPage)
                    if (map.trackRecording) {
                        mapPage.showRecording()
                    } else {
                        map.startTrackRecording()
                        Toast.show(appInfo.localized("track_recording"))
                    }
                }
            }
            MenuRow {
                icon: "image://theme/icon-m-share"
                text: appInfo.localized("share_my_location")
                onClicked: {
                    var text = map.myPositionShareText()
                    if (text === "") {
                        Toast.show(appInfo.localized("unknown_current_position"))
                        return
                    }
                    shareLocationAction.resources = [Share.textResource(text, appInfo.localized("share_my_location"))]
                    shareLocationAction.trigger()
                }
            }

            // Required attribution (Organic Maps data license) and the unofficial-port notice.
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                topPadding: Theme.paddingLarge
                text: "Map data © OpenStreetMap and Organic Maps\n"
                      + "Неофициальный порт для ОС Аврора / Unofficial Aurora OS port"
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }
        }

        VerticalScrollDecorator {}
    }
}
