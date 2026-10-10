import QtQuick 2.6
import Sailfish.Silica 1.0
import "notices.js" as Toast

Page {
    allowedOrientations: Orientation.All
    // After a move the app only closes: the maps are gone from the folder it still uses.
    backNavigation: !mapsStorage.moving && !mapsStorage.moved
    showNavigationIndicator: backNavigation
    onStatusChanged: if (status === PageStatus.Activating) mapsStorage.refresh()

    Connections {
        target: mapsStorage
        onMoveFinished: if (!success) Toast.showLong(appInfo.localized("move_maps_error"))
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        visible: !mapsStorage.moving && !mapsStorage.moved
        model: mapsStorage.locations

        header: Column {
            width: listView.width

            PageHeader {
                title: appInfo.localized("maps_storage")
            }
            PageLabel {
                text: appInfo.localized("maps_storage_summary")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
                bottomPadding: Theme.paddingLarge
            }
            DetailItem {
                label: appInfo.localized("maps_storage_downloaded")
                value: mapsStorage.downloadedSize
            }
        }

        delegate: ListItem {
            id: item
            contentHeight: Theme.itemSizeMedium
            highlighted: down || modelData.current
            onClicked: {
                if (modelData.current)
                    return
                if (!mapsStorage.canMove()) {
                    Toast.showLong(appInfo.localized("cant_change_this_setting"))
                    return
                }
                var path = modelData.path
                remorseAction(appInfo.localized("move_maps"), function() {
                    // Something may have started writing into the folder meanwhile.
                    if (!mapsStorage.moveTo(path))
                        Toast.showLong(appInfo.localized("cant_change_this_setting"))
                })
            }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                anchors.verticalCenter: parent.verticalCenter

                Label {
                    width: parent.width
                    text: modelData.name
                    highlighted: item.highlighted
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    width: parent.width
                    text: modelData.details
                    font.pixelSize: Theme.fontSizeSmall
                    color: item.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                }
            }
        }

        VerticalScrollDecorator {}
    }

    BusyLabel {
        running: mapsStorage.moving
        text: appInfo.localized("wait_several_minutes")
    }

    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.horizontalPageMargin
        visible: mapsStorage.moved
        spacing: Theme.paddingLarge

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: appInfo.localized("maps_storage_restart")
            wrapMode: Text.Wrap
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeLarge
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: appInfo.localized("close")
            onClicked: Qt.quit()
        }
    }
}
