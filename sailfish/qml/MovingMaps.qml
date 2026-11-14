import QtQuick 2.6
import Sailfish.Silica 1.0

// Shown while the maps move to another folder at start.
ApplicationWindow {
    id: window

    property real progress

    initialPage: Component {
        Page {
            ProgressCircle {
                anchors.centerIn: parent
                width: Theme.itemSizeHuge
                height: width
                value: window.progress
            }
            Label {
                anchors.centerIn: parent
                text: Math.round(window.progress * 100) + "%"
                color: Theme.highlightColor
            }
        }
    }
    cover: undefined
    allowedOrientations: defaultAllowedOrientations
}
