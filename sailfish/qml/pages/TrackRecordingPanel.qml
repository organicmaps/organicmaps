import QtQuick 2.6
import Sailfish.Silica 1.0

// Takes the place of the place page while recording.
MapPanel {
    id: panel

    // The MapItem.
    property QtObject map

    modal: false
    spacing: 0

    onOpenChanged: if (open) map.placePage.close()
    Connections {
        target: panel.map
        onTrackRecordingChanged: if (!panel.map.trackRecording) panel.open = false
    }
    Connections {
        target: panel.map.placePage
        onChanged: if (panel.map.placePage.open) panel.open = false
    }

    Item {
        width: parent.width
        height: header.height + 2 * Theme.paddingLarge

        Column {
            id: header
            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                right: closeButton.left
                verticalCenter: parent.verticalCenter
            }

            Label {
                width: parent.width
                text: appInfo.localized("track_recording_title")
                font.pixelSize: Theme.fontSizeLarge
                color: Theme.highlightColor
                truncationMode: TruncationMode.Fade
            }
            Label {
                width: parent.width
                text: panel.map.recordingSummary
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryHighlightColor
            }
        }
        IconButton {
            id: closeButton
            anchors {
                right: parent.right
                rightMargin: Theme.paddingMedium
                verticalCenter: parent.verticalCenter
            }
            icon.source: "image://theme/icon-m-cancel"
            onClicked: panel.open = false
        }
    }

    ElevationChart {
        height: Theme.itemSizeExtraLarge
        elevation: panel.map.recordingElevation
    }

    Row {
        width: parent.width

        PlaceAction {
            width: parent.width / 2
            icon: "image://theme/icon-m-delete"
            text: appInfo.localized("delete")
            onClicked: Remorse.popupAction(panel.parent, appInfo.localized("delete"), function() {
                panel.map.stopTrackRecording()
            })
        }
        // Saves under the default name, once there is something to save. Greyed out until then, so that Delete
        // isn't taken for the only way to stop.
        PlaceAction {
            width: parent.width / 2
            enabled: panel.map.recordingElevation.length > 0
            icon: "image://theme/icon-m-device-download"
            text: appInfo.localized("save")
            onClicked: panel.map.saveAndStopTrackRecording()
        }
    }
}
