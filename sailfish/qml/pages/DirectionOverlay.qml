import QtQuick 2.6
import Sailfish.Silica 1.0

MouseArea {
    id: overlay

    // The MapItem.placePage object.
    property QtObject placePage
    property bool shown

    readonly property bool landscape: width > height
    readonly property real arrowSize: Math.min(width, height) * 0.5

    anchors.fill: parent
    visible: opacity > 0
    opacity: shown ? 1.0 : 0.0
    Behavior on opacity { FadeAnimation {} }
    onClicked: shown = false

    Connections {
        target: overlay.placePage
        onChanged: if (!overlay.placePage.open) overlay.shown = false
    }

    // Fixed colors whatever the ambience.
    Rectangle {
        anchors.fill: parent
        color: "#BB000000"
    }

    // Portrait: name above and distance below the arrow; landscape: texts beside it.
    Image {
        id: arrow
        anchors {
            centerIn: overlay.landscape ? undefined : parent
            verticalCenter: overlay.landscape ? parent.verticalCenter : undefined
            left: overlay.landscape ? parent.left : undefined
            leftMargin: 2 * Theme.horizontalPageMargin
        }
        width: overlay.arrowSize
        height: width
        sourceSize: Qt.size(width, height)
        source: "../../icons/placepage/ic_direction_fullscreen.png"
        rotation: Math.max(0, overlay.placePage.azimuth)
        opacity: overlay.placePage.azimuth >= 0 ? 1.0 : Theme.opacityLow
    }

    Column {
        id: header
        x: overlay.landscape ? arrow.x + arrow.width + 2 * Theme.horizontalPageMargin : Theme.horizontalPageMargin
        width: overlay.width - x - Theme.horizontalPageMargin
        anchors {
            bottom: overlay.landscape ? parent.verticalCenter : arrow.top
            bottomMargin: Theme.paddingLarge
        }
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: overlay.placePage.title
            color: "white"
            font.pixelSize: Theme.fontSizeExtraLarge
            wrapMode: Text.Wrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }
        Label {
            width: parent.width
            visible: text !== ""
            horizontalAlignment: Text.AlignHCenter
            text: overlay.placePage.subtitle
            color: "#99FFFFFF"
            font.pixelSize: Theme.fontSizeMedium
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }
    }

    Column {
        x: header.x
        width: header.width
        anchors {
            top: overlay.landscape ? parent.verticalCenter : arrow.bottom
            topMargin: Theme.paddingLarge
        }
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: overlay.placePage.distance
            color: "white"
            font.pixelSize: Theme.fontSizeHuge
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: overlay.placePage.bearing
            color: "white"
            font.pixelSize: Theme.fontSizeExtraLarge
        }
    }
}
