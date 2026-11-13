import QtQuick 2.6
import Sailfish.Silica 1.0

Grid {
    id: grid

    property real size: Theme.itemSizeExtraSmall
    property int selectedIndex: -1
    signal colorClicked(int index)

    x: Theme.horizontalPageMargin
    width: parent.width - 2 * x
    spacing: Theme.paddingMedium
    columns: Math.max(1, Math.floor((width + spacing) / (size + spacing)))

    Repeater {
        model: appInfo.bookmarkColors

        ColorDot {
            width: grid.size
            color: modelData
            border.width: index === grid.selectedIndex ? Theme.paddingSmall : 0
            border.color: Theme.highlightColor

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                visible: mouseArea.pressed
                color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
            }
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                onClicked: grid.colorClicked(index)
            }
        }
    }
}
