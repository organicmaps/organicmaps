import QtQuick 2.6
import Sailfish.Silica 1.0

// The default glass background is too transparent over the map, so it is layered over an opaque base.
DockedPanel {
    default property alias content: contentColumn.data
    property alias spacing: contentColumn.spacing
    // Closes on a press outside, as modal does, but without modal's dimming of the whole window over the map.
    property bool closesOnPressOutside

    width: parent.width
    height: contentColumn.height + Theme.paddingLarge
    dock: Dock.Bottom
    modal: false

    background: Item {
        Rectangle {
            anchors.fill: parent
            color: Theme.overlayBackgroundColor
        }
        PanelBackground {
            anchors.fill: parent
        }
    }

    InverseMouseArea {
        anchors.fill: parent
        enabled: closesOnPressOutside && open
        stealPress: true
        onPressedOutside: open = false
    }

    Column {
        id: contentColumn
        width: parent.width
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.paddingLarge
        spacing: Theme.paddingLarge
    }
}
