import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    property var navigation
    // MapButton.androidDp, for the sizes of the Android turn card and speed limit.
    property real androidDp
    // As layout-land/layout_nav_top.xml, the speed limit and lanes follow the turn card in a row below the street
    // bar, which keeps the left column short. The street bar stays right of the turn card, as in portrait.
    property bool landscape
    // margin_half, as between the Android turn card, the turn after it, the speed limit and the lanes.
    readonly property real margin: 8 * androidDp
    readonly property real streetBottom: streetBar.visible ? streetBar.height : 0
    // Readable on the ambience highlight color of the cards, which can be light or dark.
    readonly property color cardTextColor: {
        var c = Theme.highlightBackgroundColor
        return 0.299 * c.r + 0.587 * c.g + 0.114 * c.b > 0.6 ? Theme.darkPrimaryColor : Theme.lightPrimaryColor
    }

    height: Math.max(turnColumn.height, speedLimit.visible ? speedLimit.y + speedLimit.height : 0,
                     rightHeight)
    // The street and lanes bars, all that reaches the right edge; the controls there only need to clear these.
    readonly property real rightHeight: lanesBar.visible ? lanesBar.y + lanesBar.height
                                      : streetBottom

    Column {
        id: turnColumn
        spacing: margin

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
    }

    // Below the turn cards in portrait, right of them in landscape.
    RoadSign {
        id: speedLimit
        visible: !!navigation.speedLimit
        x: landscape ? turnColumn.width + margin : 0
        y: landscape ? streetBottom + margin : turnColumn.height + margin
        width: 60 * androidDp
        text: navigation.speedLimit || ""
        alert: !!navigation.speedLimitExceeded
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

                // Road numbers as filled shields; exit numbers outlined, unlike any road shield.
                Item {
                    readonly property bool exit: !!modelData.exit
                    readonly property bool shield: !!modelData.shield || exit

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
                        border.width: parent.exit ? Theme.dp(2) : Theme.dp(1)
                        border.color: parent.exit ? Theme.primaryColor : modelData.textColor || "transparent"
                    }
                    Label {
                        id: partLabel
                        x: parent.shield ? Theme.paddingSmall : 0
                        width: parent.shield ? implicitWidth : parent.width
                        text: modelData.text
                        font.bold: parent.shield
                        color: parent.shield && !parent.exit ? modelData.textColor : Theme.primaryColor
                        truncationMode: TruncationMode.Fade
                    }
                }
            }
        }
    }

    Rectangle {
        id: lanesBar

        readonly property var lanes: navigation.lanes || []

        // Centered under the street bar; in landscape only pushed right when they would reach the speed limit.
        readonly property real minX: landscape && speedLimit.visible ? speedLimit.x + speedLimit.width + margin : 0

        visible: lanes.length > 0
        x: Math.max(minX, streetBar.x + (streetBar.width - width) / 2)
        y: landscape ? streetBottom + margin : streetBar.visible ? streetBottom + Theme.paddingSmall : 0
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
