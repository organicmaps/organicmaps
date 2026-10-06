import QtQuick 2.6
import Sailfish.Silica 1.0

// A button over the map, sized like other Silica touch targets.
Rectangle {
    id: button

    // One Android dp, for the navigation and downloader layouts that keep the Android sizes: 2.625 px at
    // 420 dpi, where the Silica pixel ratio is 1.5.
    readonly property real androidDp: Theme.pixelRatio * 2.625 / 1.5
    property real size: Theme.itemSizeMedium
    property string source
    // The accessible name.
    property string description
    property bool highlighted
    // A rounded square instead of a circle, for the buttons along the bottom edge.
    property bool square
    property alias busy: busyIndicator.running
    signal clicked()

    width: size
    height: size
    radius: square ? Theme.paddingMedium : width / 2
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
