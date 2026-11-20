import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
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
                title: qsTrId("menu")
            }
            MenuRow {
                visible: appSettings.donateUrl !== ""
                icon: "../../icons/menu/ic_donate.svg"
                text: qsTrId("donate")
                onClicked: appSettings.openDonatePage()
            }
            MenuRow {
                icon: "image://theme/icon-m-setting"
                text: qsTrId("settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuRow {
                icon: "image://theme/icon-m-share"
                text: qsTrId("share_my_location")
                onClicked: {
                    var text = map.myPositionShareText()
                    if (text === "") {
                        Toast.show(qsTrId("unknown_current_position"))
                        return
                    }
                    shareLocationAction.resources = [Share.textResource(text, qsTrId("share_my_location"))]
                    shareLocationAction.trigger()
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
