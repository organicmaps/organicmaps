import QtQuick 2.6
import Sailfish.Silica 1.0

// Shows or hides a list or track on the map.
IconButton {
    property bool shown

    icon.source: Qt.resolvedUrl("../../icons/bookmarks/" + (shown ? "ic_show.svg" : "ic_hide.svg"))
    icon.sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
}
