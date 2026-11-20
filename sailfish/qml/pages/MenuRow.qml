import QtQuick 2.6
import Sailfish.Silica 1.0

ListItem {
    id: row

    property string icon
    property string text
    // Full color icons, such as the categories, would otherwise be tinted to a light gray glyph on a gray circle.
    property bool monochrome: true

    width: parent.width
    contentHeight: Theme.itemSizeMedium
    opacity: enabled ? 1.0 : Theme.opacityLow

    Icon {
        id: rowIcon
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        source: row.icon
        // Theme icons already have this size; SVG files would otherwise render at their nominal size.
        sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
        color: row.monochrome ? Theme.primaryColor : undefined
        highlighted: row.highlighted
    }

    Label {
        anchors {
            left: rowIcon.right
            leftMargin: Theme.paddingLarge
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        text: row.text
        truncationMode: TruncationMode.Fade
        highlighted: row.highlighted
    }
}
