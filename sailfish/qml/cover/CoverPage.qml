import QtQuick 2.6
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    property Item mapPage
    readonly property QtObject map: mapPage ? mapPage.mapItem : null
    readonly property var position: map ? map.positionInfo : ({})

    Row {
        id: header
        x: Theme.paddingLarge
        y: Theme.paddingMedium
        spacing: Theme.paddingSmall

        Image {
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.iconSizeSmall
            height: width
            sourceSize: Qt.size(width, height)
            source: "../../icons/help/logo.svg"
        }
        Label {
            anchors.verticalCenter: parent.verticalCenter
            text: "Organic Maps"
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
        }
    }

    Column {
        anchors {
            top: header.bottom
            topMargin: Theme.paddingMedium
            left: parent.left
            leftMargin: Theme.paddingLarge
            right: parent.right
            rightMargin: Theme.paddingLarge
        }
        spacing: Theme.paddingSmall

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            visible: text !== ""
            text: position.address || ""
            font.pixelSize: Theme.fontSizeMedium
            wrapMode: Text.Wrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            visible: text !== ""
            text: position.coordinates || ""
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryColor
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            visible: text !== ""
            text: [position.altitude, position.speed].filter(function(part) { return !!part }).join("   ")
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryHighlightColor
        }
    }
}
