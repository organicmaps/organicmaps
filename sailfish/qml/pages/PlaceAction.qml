import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: action

    property string icon
    property alias text: label.text

    height: Theme.itemSizeMedium
    opacity: enabled ? 1.0 : Theme.opacityLow

    Column {
        anchors.centerIn: parent
        width: parent.width

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            source: action.icon
            sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
            // The route icons are single color bitmaps.
            color: Theme.primaryColor
            highlighted: action.highlighted
        }
        Label {
            id: label
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeTiny
            truncationMode: TruncationMode.Fade
            highlighted: action.highlighted
        }
    }
}
