import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    property var navigation
    // MapButton.androidDp, for the sizes of the Android turn card and speed limit.
    property real androidDp
    // Readable on the ambience highlight color of the cards, which can be light or dark.
    readonly property color cardTextColor: {
        var c = Theme.highlightBackgroundColor
        return 0.299 * c.r + 0.587 * c.g + 0.114 * c.b > 0.6 ? Theme.darkPrimaryColor : Theme.lightPrimaryColor
    }

    height: Math.max(turnColumn.height, streetBar.height + lanesBar.height)

    Column {
        id: turnColumn
        // margin_half, as between the Android turn card, the turn after it and the speed limit.
        spacing: 8 * androidDp

        // nav_next_turn_frame wide, with a nav_next_turn_sign arrow.
        Rectangle {
            width: 88 * androidDp
            height: turnContent.height + 2 * Theme.paddingMedium
            radius: Theme.paddingMedium
            color: Theme.highlightBackgroundColor

            Column {
                id: turnContent
                anchors.centerIn: parent

                Icon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 64 * androidDp
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: navigation.turnIcon ? "../../icons/navigation/" + navigation.turnIcon : ""
                    color: cardTextColor
                }
                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: navigation.distanceToTurn || ""
                    color: cardTextColor
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                }
            }
        }
        Rectangle {
            visible: !!navigation.nextTurnIcon
            width: 88 * androidDp
            height: 32 * androidDp
            radius: Theme.paddingSmall
            color: Theme.rgba(Theme.highlightBackgroundColor, 0.8)

            Icon {
                anchors.centerIn: parent
                width: 28 * androidDp
                height: width
                sourceSize: Qt.size(width, height)
                source: navigation.nextTurnIcon ? "../../icons/navigation/" + navigation.nextTurnIcon : ""
                color: cardTextColor
            }
        }
        RoadSign {
            visible: !!navigation.speedLimit
            width: 60 * androidDp
            text: navigation.speedLimit || ""
            alert: !!navigation.speedLimitExceeded
        }
    }

    Rectangle {
        id: streetBar
        visible: !!navigation.street
        anchors {
            left: turnColumn.right
            leftMargin: Theme.paddingMedium
            right: parent.right
        }
        height: streetRow.height + 2 * Theme.paddingMedium
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.overlayBackgroundColor, 0.85)
        clip: true

        Row {
            id: streetRow
            anchors.centerIn: parent
            width: Math.min(implicitWidth, parent.width - 2 * Theme.paddingMedium)
            spacing: Theme.paddingSmall

            Repeater {
                model: navigation.streetParts || []

                Item {
                    readonly property bool shield: !!modelData.shield

                    width: shield ? shieldRect.width : Math.min(partLabel.implicitWidth,
                                                                streetBar.width - 2 * Theme.paddingMedium)
                    height: partLabel.height

                    Rectangle {
                        id: shieldRect
                        visible: parent.shield
                        anchors.verticalCenter: parent.verticalCenter
                        width: partLabel.implicitWidth + 2 * Theme.paddingSmall
                        height: partLabel.height
                        radius: Theme.paddingSmall / 2
                        color: modelData.color || "transparent"
                        border.width: Theme.dp(1)
                        border.color: modelData.textColor || "transparent"
                    }
                    Label {
                        id: partLabel
                        x: parent.shield ? Theme.paddingSmall : 0
                        width: parent.shield ? implicitWidth : parent.width
                        text: modelData.text
                        font.bold: parent.shield
                        color: parent.shield ? modelData.textColor : Theme.primaryColor
                        truncationMode: TruncationMode.Fade
                    }
                }
            }
        }
    }

    Rectangle {
        id: lanesBar

        readonly property var lanes: navigation.lanes || []

        visible: lanes.length > 0
        anchors {
            top: streetBar.visible ? streetBar.bottom : parent.top
            topMargin: streetBar.visible ? Theme.paddingSmall : 0
            horizontalCenter: streetBar.horizontalCenter
        }
        width: lanesRow.width + 2 * Theme.paddingMedium
        height: visible ? Theme.iconSizeMedium + 2 * Theme.paddingSmall : 0
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.highlightBackgroundColor, 0.9)

        Row {
            id: lanesRow
            anchors.centerIn: parent

            Repeater {
                model: lanesBar.lanes

                Icon {
                    width: Theme.iconSizeMedium
                    height: width
                    sourceSize: Qt.size(width, height)
                    source: "../../icons/navigation/" + modelData.icon
                    color: cardTextColor
                    opacity: modelData.active ? 1.0 : 0.38
                }
            }
        }
    }
}
