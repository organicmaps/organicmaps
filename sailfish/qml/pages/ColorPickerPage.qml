import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    property string title
    // The color in use as "#rrggbb", ringed when it is a preset.
    property string current
    property var chosen

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: page.title
            }
            ColorGrid {
                size: Theme.itemSizeSmall
                spacing: Theme.paddingLarge
                selectedIndex: appInfo.bookmarkColors.indexOf(page.current)
                onColorClicked: {
                    page.chosen(index)
                    pageStack.pop()
                }
            }
        }
    }
}
