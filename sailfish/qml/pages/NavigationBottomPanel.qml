import QtQuick 2.6
import Sailfish.Silica 1.0

// The numbers stay on the map; the actions are in a pulley menu pulled up from the panel, as on Android.
Item {
    id: panel

    property var navigation
    // MapItem.routing.
    property QtObject routing
    // MapButton.androidDp.
    property real androidDp
    signal stopClicked()
    signal settingsClicked()
    signal voiceSettingsClicked()

    property alias radius: background.radius

    height: column.height + Theme.paddingMedium

    // Outside the flickable, whose content Silica dims while the menu is pulled: it follows the content up and
    // stays behind the menu.
    Rectangle {
        id: background
        y: -flickable.pulled
        width: parent.width
        height: parent.height + flickable.pulled
        color: Theme.rgba(Theme.overlayBackgroundColor, 0.95)
    }

    SilicaFlickable {
        id: flickable
        readonly property real pulled: Math.max(0, contentY - originY)

        anchors.fill: parent
        contentHeight: height
        flickableDirection: Flickable.VerticalFlick

        PushUpMenu {
            id: menu

            // Stops without asking.
            MenuItem {
                text: appInfo.localized("navigation_stop_button")
                onClicked: panel.stopClicked()
            }
            // Without a voice it leads to the voice settings, which tell how to get one.
            MenuItem {
                visible: appInfo.voiceSupported
                text: !panel.routing.voiceAvailable ? appInfo.localized("pref_tts_enable_title")
                    : panel.routing.voiceEnabled ? appInfo.localized("navigation_mute_voice")
                    : appInfo.localized("navigation_unmute_voice")
                onClicked: {
                    if (panel.routing.voiceAvailable)
                        panel.routing.voiceEnabled = !panel.routing.voiceEnabled
                    else
                        panel.voiceSettingsClicked()
                }
            }
            MenuItem {
                text: appInfo.localized("settings")
                onClicked: panel.settingsClicked()
            }
        }

        Column {
            id: column
            width: parent.width
            topPadding: Theme.paddingMedium

            // As layout_nav_bottom_numbers: speed, time and distance columns of at least nav_numbers_side_min_width,
            // with the free space split 0.5 : 1.25 : 1.25 : 0.5 around them.
            Row {
                id: numbers
                readonly property real columnWidth: 90 * panel.androidDp
                readonly property real freeWidth: Math.max(0, parent.width - speedView.width - timeColumn.width - distanceNumber.width)

                x: freeWidth * 0.5 / 3.5
                width: parent.width - x
                height: Theme.itemSizeLarge
                spacing: freeWidth * 1.25 / 3.5

                // Red over the speed limit, and red behind it over a speed camera limit.
                Rectangle {
                    id: speedView
                    readonly property bool camAlert: !!navigation.speedCamLimitExceeded

                    width: Math.max(numbers.columnWidth, speedNumber.width + 2 * Theme.paddingSmall)
                    height: parent.height
                    radius: Theme.paddingSmall
                    color: camAlert ? "#f51e30" : "transparent"

                    NavigationNumber {
                        id: speedNumber
                        anchors.horizontalCenter: parent.horizontalCenter
                        value: navigation.speed || ""
                        units: navigation.speedUnits || ""
                        valueColor: speedView.camAlert ? Theme.lightPrimaryColor
                                  : navigation.speedLimitExceeded ? "#f51e30" : Theme.primaryColor
                        unitsColor: speedView.camAlert ? Theme.lightPrimaryColor : Theme.secondaryColor
                    }
                }
                Column {
                    id: timeColumn
                    width: Math.max(numbers.columnWidth, implicitWidth)
                    anchors.verticalCenter: parent.verticalCenter

                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter

                        Label {
                            id: hoursLabel
                            visible: (navigation.hoursLeft || 0) > 0
                            text: navigation.hoursLeft || ""
                            color: Theme.highlightColor
                            font.pixelSize: Theme.fontSizeExtraLarge
                            font.bold: true
                        }
                        Label {
                            visible: (navigation.hoursLeft || 0) > 0
                            anchors.baseline: hoursLabel.baseline
                            text: (navigation.hourUnits || "") + " "
                            color: Theme.highlightColor
                        }
                        Label {
                            id: minutesLabel
                            text: navigation.minutesLeft !== undefined ? navigation.minutesLeft : ""
                            color: Theme.highlightColor
                            font.pixelSize: Theme.fontSizeExtraLarge
                            font.bold: true
                        }
                        Label {
                            anchors.baseline: minutesLabel.baseline
                            text: navigation.minuteUnits || ""
                            color: Theme.highlightColor
                        }
                    }
                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: navigation.arrival || ""
                        color: Theme.secondaryColor
                    }
                }
                NavigationNumber {
                    id: distanceNumber
                    width: Math.max(numbers.columnWidth, implicitWidth)
                    value: navigation.distanceLeftValue || ""
                    units: navigation.distanceLeftUnits || ""
                }
            }

            Rectangle {
                width: parent.width
                height: Theme.paddingSmall / 2
                color: Theme.rgba(Theme.primaryColor, 0.2)

                Rectangle {
                    width: parent.width * (navigation.progress || 0)
                    height: parent.height
                    color: Theme.highlightColor
                }
            }
        }
    }
}
