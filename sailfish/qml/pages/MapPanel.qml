import QtQuick 2.6
import Sailfish.Silica 1.0

// The default glass background is too transparent over the map, so it is layered over an opaque base.
DockedPanel {
    default property alias content: contentColumn.data
    property alias spacing: contentColumn.spacing

    width: parent.width
    height: contentColumn.height + Theme.paddingLarge
    dock: Dock.Bottom
    modal: true

    background: Item {
        Rectangle {
            anchors.fill: parent
            color: Theme.overlayBackgroundColor
        }
        PanelBackground {
            anchors.fill: parent
        }
    }

    Column {
        id: contentColumn
        width: parent.width
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.paddingLarge
        spacing: Theme.paddingLarge
    }
}
