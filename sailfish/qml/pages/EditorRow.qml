import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: row

    property string icon
    default property alias content: holder.data

    width: parent.width
    height: holder.height

    Icon {
        id: rowIcon
        x: Theme.horizontalPageMargin
        // Level with the first line of the field.
        y: (Theme.itemSizeSmall - height) / 2
        source: row.icon
        sourceSize: Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
    }

    Item {
        id: holder
        anchors {
            left: rowIcon.right
            right: parent.right
        }
        height: childrenRect.height
    }
}
