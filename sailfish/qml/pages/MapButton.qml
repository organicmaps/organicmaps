import QtQuick 2.6
import Sailfish.Silica 1.0

// A button over the map, sized like other Silica touch targets.
Rectangle {
    id: button

    property real size: Theme.itemSizeMedium
    property string source
    // The accessible name.
    property string description
    property bool highlighted
    property alias busy: busyIndicator.running
    signal clicked()

    width: size
    height: size
    radius: width / 2
    color: Theme.rgba(Theme.overlayBackgroundColor, Theme.opacityOverlay)
    opacity: enabled ? 1.0 : Theme.opacityLow

    // The Silica press feedback, as on list items.
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        visible: iconButton.down
        color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
    }

    IconButton {
        id: iconButton
        anchors.fill: parent
        icon.source: button.source
        icon.width: Theme.iconSizeMedium
        icon.height: Theme.iconSizeMedium
        icon.sourceSize: Qt.size(Theme.iconSizeMedium, Theme.iconSizeMedium)
        icon.color: Theme.primaryColor
        highlighted: button.highlighted || down
        Accessible.name: button.description
        onClicked: button.clicked()
    }

    BusyIndicator {
        id: busyIndicator
        anchors.centerIn: parent
        size: BusyIndicatorSize.Small
        running: false
    }
}
