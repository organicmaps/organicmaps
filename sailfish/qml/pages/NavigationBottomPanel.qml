import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    id: panel

    property var navigation
    // MapItem.routing.
    property QtObject routing
    // MapButton.androidDp.
    property real androidDp
    signal stopClicked()
    signal settingsClicked()
    signal voiceSettingsClicked()

    height: column.height + Theme.paddingMedium
    color: Theme.rgba(Theme.overlayBackgroundColor, 0.95)

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

        Row {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            height: Theme.itemSizeMedium
            spacing: Theme.paddingLarge

            IconButton {
                visible: appInfo.voiceSupported
                anchors.verticalCenter: parent.verticalCenter
                icon.source: panel.routing.voiceEnabled ? "image://theme/icon-m-speaker-on"
                                                         : "image://theme/icon-m-speaker-mute"
                Accessible.name: appInfo.localized("pref_tts_enable_title")
                // Without a voice it leads to the voice settings, which tell how to get one.
                onClicked: {
                    if (panel.routing.voiceAvailable)
                        panel.routing.voiceEnabled = !panel.routing.voiceEnabled
                    else
                        panel.voiceSettingsClicked()
                }
            }
            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "image://theme/icon-m-setting"
                Accessible.name: appInfo.localized("settings")
                onClicked: panel.settingsClicked()
            }
            // Stops without asking.
            Button {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - x
                color: Theme.errorColor
                highlightColor: Theme.errorColor
                highlightBackgroundColor: Theme.errorColor
                text: appInfo.localized("navigation_stop_button").toUpperCase()
                onClicked: panel.stopClicked()
            }
        }
    }
}
