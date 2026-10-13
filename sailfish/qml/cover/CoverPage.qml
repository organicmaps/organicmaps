import QtQuick 2.6
import Sailfish.Silica 1.0

CoverBackground {
    id: cover

    property Item mapPage
    readonly property QtObject map: mapPage ? mapPage.mapItem : null
    readonly property var position: map ? map.positionInfo : ({})
    readonly property string mode: !map ? "idle"
        : map.trackRecording ? "recording"
        : downloads.inProgress && downloads.downloadingName !== "" ? "download"
        : "idle"

    signal activateRequested()

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

        Icon {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: cover.mode === "recording" || cover.mode === "download"
            width: Theme.iconSizeLarge
            height: width
            sourceSize: Qt.size(width, height)
            source: cover.mode === "recording" ? "../../icons/menu/ic_track_recording_status.svg"
                                               : "image://theme/icon-m-cloud-download"
            color: cover.mode === "recording" ? Theme.errorColor : Theme.primaryColor
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            visible: text !== ""
            text: {
                switch (cover.mode) {
                case "recording": return appInfo.localized("track_recording_title")
                case "download": return Math.round(downloads.downloadingProgress * 100) + "%"
                default: return position.address || ""
                }
            }
            font.pixelSize: cover.mode === "idle" ? Theme.fontSizeMedium : Theme.fontSizeLarge
            font.bold: cover.mode === "download"
            wrapMode: Text.Wrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }
        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            visible: text !== ""
            text: {
                switch (cover.mode) {
                case "recording": return map.recordingSummary
                case "download": return downloads.downloadingName
                default: return position.coordinates || ""
                }
            }
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
            text: {
                if (cover.mode !== "idle")
                    return ""
                return [position.altitude, position.speed].filter(function(part) { return !!part }).join("   ")
            }
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryHighlightColor
        }
        Rectangle {
            visible: cover.mode === "download"
            width: parent.width
            height: Theme.paddingSmall / 2
            color: Theme.rgba(Theme.primaryColor, 0.2)

            Rectangle {
                width: parent.width * downloads.downloadingProgress
                height: parent.height
                color: Theme.highlightColor
            }
        }
    }

    CoverActionList {
        enabled: cover.mode === "recording"
        CoverAction {
            iconSource: "image://theme/icon-cover-favorite"
            onTriggered: map.saveAndStopTrackRecording()
        }
    }
    CoverActionList {
        enabled: cover.mode === "download"
        CoverAction {
            iconSource: "image://theme/icon-cover-cancel"
            onTriggered: downloads.cancelAll()
        }
    }
    CoverActionList {
        enabled: cover.mode === "idle"
        CoverAction {
            iconSource: "image://theme/icon-cover-search"
            onTriggered: {
                cover.activateRequested()
                mapPage.openSearchFromCover()
            }
        }
    }
}
