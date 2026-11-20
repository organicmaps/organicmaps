import QtQuick 2.6
import Sailfish.Silica 1.0
import app.organicmaps 1.0
import "notices.js" as Toast

Page {
    id: page

    // Follows the Sailfish ambience.
    readonly property bool mapIsDark: Theme.colorScheme === Theme.LightOnDark
    // The clearance for the camera cutout in portrait; the compass goes below it.
    readonly property real topControlsBottom: topInset + buttonMargin
    // Silica keeps landscape pages clear of the camera notch.
    readonly property real topInset: page.orientation === Orientation.Portrait ? Screen.topCutout.height : 0
    readonly property real buttonMargin: Theme.paddingMedium

    objectName: "mapPage"
    allowedOrientations: Orientation.All
    backNavigation: false

    onMapIsDarkChanged: appSettings.applyMapAppearance(mapIsDark)
    Component.onCompleted: appSettings.applyMapAppearance(mapIsDark)

    MapItem {
        id: map
        // Silica keeps landscape pages clear of the camera cutout, at the left in Landscape and the right in
        // LandscapeInverted; the map alone is drawn under it.
        anchors {
            fill: parent
            leftMargin: -leftInset
            rightMargin: -rightInset
        }
        leftInset: page.orientation === Orientation.Landscape ? Screen.topCutout.height : 0
        rightInset: page.orientation === Orientation.LandscapeInverted ? Screen.topCutout.height : 0
        bottomWidgetsOffset: page.buttonMargin
        topWidgetsOffset: page.topControlsBottom
    }

    Connections {
        target: map
        onEngineCreated: urlHandler.processPending()
        onNotice: Toast.showLong(message)
    }

    MapButton {
        anchors {
            top: parent.top
            topMargin: page.topInset + page.buttonMargin
            left: parent.left
            leftMargin: page.buttonMargin
        }
        source: "image://theme/icon-m-levels"
        description: qsTrId("layers_title")
        highlighted: map.enabledLayers !== 0 || layersPanel.open
        onClicked: layersPanel.open = true
    }

    // Positions follow Android's layout-h400dp/map_buttons_layout_regular.xml and layout-land, in Silica sizes.
    Column {
        id: rightButtons
        anchors {
            right: parent.right
            rightMargin: page.buttonMargin
            // Portrait: map_buttons_bottom_margin plus the padding from the bottom edge. Landscape: my position
            // is in the bottom right corner.
            bottom: parent.bottom
            bottomMargin: page.isPortrait ? 3 * myPositionButton.size : page.buttonMargin
        }
        spacing: page.buttonMargin

        // Buttons that would reach above topControlsBottom are hidden. Only faded out, so that the
        // column keeps its layout.
        function fits(button) {
            return y + button.y >= page.topControlsBottom + page.buttonMargin
        }

        MapButton {
            id: zoomInButton
            enabled: rightButtons.fits(zoomInButton)
            opacity: enabled ? 1.0 : 0.0
            source: "image://theme/icon-m-add"
            description: qsTrId("zoom_in")
            onClicked: map.zoomIn()
        }
        MapButton {
            id: zoomOutButton
            enabled: rightButtons.fits(zoomOutButton)
            opacity: enabled ? 1.0 : 0.0
            source: "image://theme/icon-m-remove"
            description: qsTrId("zoom_out")
            onClicked: map.zoomOut()
        }
        // A wider gap between the zoom buttons and my position in portrait.
        Item {
            visible: page.isPortrait
            width: 1
            height: Theme.paddingLarge
        }
        // Indexed by MapItem.MyPositionMode.
        MapButton {
            id: myPositionButton
            readonly property var icons: ["", "ic_location_off", "ic_not_follow", "ic_follow", "ic_follow_and_rotate"]

            enabled: rightButtons.fits(myPositionButton)
            opacity: enabled ? 1.0 : 0.0
            description: qsTrId("core_my_position")

            source: map.myPositionMode === MapItem.PendingPosition
                    ? "" : Qt.resolvedUrl("../../icons/myposition/" + icons[map.myPositionMode] + ".svg")
            highlighted: map.myPositionMode === MapItem.Follow || map.myPositionMode === MapItem.FollowAndRotate
            busy: map.myPositionMode === MapItem.PendingPosition
            onClicked: map.switchMyPositionMode()
        }
    }

    MapPanel {
        id: layersPanel
        closesOnPressOutside: true

        Item {
            width: parent.width
            height: closeButton.height

            Label {
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    right: closeButton.left
                    verticalCenter: parent.verticalCenter
                }
                text: qsTrId("layers_title")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
                truncationMode: TruncationMode.Fade
            }

            IconButton {
                id: closeButton
                anchors.right: parent.right
                anchors.rightMargin: Theme.paddingMedium
                icon.source: "image://theme/icon-m-cancel"
                onClicked: layersPanel.open = false
            }
        }

        Row {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x

            Repeater {
                id: layersRepeater
                // Order and labels of the Android layers sheet.
                model: [
                    { layer: MapItem.Outdoors, icon: "ic_layers_outdoors",
                      text: qsTrId("button_layer_outdoor") },
                    // A bitmap on Android too.
                    { layer: MapItem.Isolines, icon: "ic_layers_isoline", format: "webp",
                      text: qsTrId("button_layer_terrain") },
                    { layer: MapItem.Hiking, icon: "ic_layers_hiking",
                      text: qsTrId("button_layer_hiking") },
                    { layer: MapItem.Cycling, icon: "ic_layers_cycling",
                      text: qsTrId("button_layer_cycling") },
                    { layer: MapItem.Subway, icon: "ic_layers_subway",
                      text: qsTrId("button_layer_subway") }
                ]

                LayerButton {
                    width: parent.width / layersRepeater.count
                    source: Qt.resolvedUrl("../../icons/layers/" + modelData.icon
                                           + (page.mapIsDark ? "_night" : "") + "." + (modelData.format || "svg"))
                    text: modelData.text
                    checked: (map.enabledLayers & (1 << modelData.layer)) !== 0
                    isNew: (map.newLayers & (1 << modelData.layer)) !== 0
                    onClicked: {
                        var enable = !checked
                        map.setLayerEnabled(modelData.layer, enable)
                        if (!enable)
                            return
                        if (modelData.layer === MapItem.Isolines && map.isolinesNeedZoom())
                            Toast.show(qsTrId("isolines_toast_zooms_1_10"))
                        else if ((modelData.layer === MapItem.Hiking || modelData.layer === MapItem.Cycling)
                                 && map.needUpdateForRoutes())
                            Toast.showLong(qsTrId("routes_update_maps_text"))
                    }
                }
            }
        }

        // The map shows the same attribution only for a while after the start.
        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - 2 * x
            text: "© <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a>"
                  + " © <a href=\"https://organicmaps.app/\">Organic Maps</a>"
            textFormat: Text.StyledText
            linkColor: Theme.highlightColor
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
            wrapMode: Text.Wrap
            onLinkActivated: Qt.openUrlExternally(link)
        }
    }
}
